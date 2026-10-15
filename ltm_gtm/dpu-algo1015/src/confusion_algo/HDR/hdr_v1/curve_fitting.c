#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <string.h>

#include "curve_fitting.h"

#define PRINTF_INTERP_POINTS 0
#define PRINTF_INTERP_STEPS 0

/*
 * A segment structure representing a segment between two endpoints i and j,
 * along with the index k of the point with the maximum error and the corresponding error value.
 */
typedef struct {
    int i, j;        /* segment endpoints */
    int k;           /* index of the point with maximum error */
    int step;        /* length of the sub-segment */
    double err;      /* maximum absolute error */
    int from_merge;  /* 1 if this segment came from a merge, cannot be split */
} Segment;

/*
 * A simple max-heap structure for managing segments,
 * ordered by the maximum error (err).
 */
typedef struct {
    Segment *data;   /* array of segments */
    int size;        /* current number of segments in the heap */
    int capacity;    /* maximum capacity of the heap */
} MaxHeap;

/* Initialize the max-heap with a given capacity */
void heap_init(MaxHeap *h, int capacity) {
    h->data = (Segment *)malloc(sizeof(Segment) * capacity);  // allocate space for segments
    h->size = 0;  // starting size is 0
    h->capacity = capacity;  // set the maximum capacity
}

/* Insert a segment into the heap */
void heap_push(MaxHeap *h, Segment s) {
    if (h->size == h->capacity) return;  // check for capacity limitation
    int idx = h->size++;  // place the new segment at the end and increment the size
    h->data[idx] = s;  // insert the segment
    /* sift up to maintain heap property */
    while (idx > 0) {
        int parent = (idx - 1) >> 1;
        // swap with parent if current segment has a larger error
        if (h->data[parent].err >= h->data[idx].err) break;
        Segment tmp = h->data[parent];
        h->data[parent] = h->data[idx];
        h->data[idx] = tmp;
        idx = parent;  // move up to parent's index
    }
}

/* Remove and return the segment with the largest error from the heap */
Segment heap_pop(MaxHeap *h) {
    Segment top = h->data[0];  // store the top element
    h->data[0] = h->data[--h->size];  // replace top with the last element
    int idx = 0;
    /* sift down to maintain heap property */
    while (1) {
        int left = 2 * idx + 1;  // left child index
        int right = left + 1;    // right child index
        int largest = idx;  // assume current index is largest

        // find the largest among the current node and its children
        if (left < h->size && h->data[left].err > h->data[largest].err)
            largest = left;
        if (right < h->size && h->data[right].err > h->data[largest].err)
            largest = right;
        if (largest == idx) break;  // if current index is largest, exit
        // swap the segments
        Segment tmp = h->data[idx];
        h->data[idx] = h->data[largest];
        h->data[largest] = tmp;
        idx = largest;  // move down to the largest found index
    }
    return top;  // return the segment with the largest error
}

/* Compute the absolute error at point k for the line segment through (i,y[i]) and (j,y[j]) */
static inline double point_error(double *y, int i, int j, int k) {
    double yi = y[i];  // y-coordinate at i
    double yj = y[j];  // y-coordinate at j
    double L = j - i;  // length of the segment
    double interp = yi + (yj - yi) * (k - i) / L;  // linear interpolation
    return fabs(y[k] - interp);  // return absolute error
}

/* Compute the absolute error gain at point k for the line that uses gain values */
static inline double point_error_gain(double *x, double *y, double *gain, int i, int j, int k) {
    double gain_i = gain[i];      // gain value at i
    double gain_j = gain[j];      // gain value at j
    double L = j - i;             // length of the segment
    double interp_gain = gain_i + (gain_j - gain_i) * (k - i) / L;  // interpolate gain
    return fabs(y[k] - x[k] * interp_gain);  // return absolute error based on gains
}

/* New function: Print the errors of all segments currently in the heap */
void heap_print_errors(MaxHeap *h) {
    printf("--- Heap Status (Size: %d) ---\n", h->size);
    if (h->size == 0) {
        printf("Heap is empty.\n");
    }
    else {
        // Sort segments by starting index (i) for left-to-right display
        Segment *sorted = (Segment *)malloc(sizeof(Segment) * h->size);
        for (int i = 0; i < h->size; i++) {
            sorted[i] = h->data[i];
        }
        for (int i = 0; i < h->size; i++) {
            for (int j = i + 1; j < h->size; j++) {
                if (sorted[i].i > sorted[j].i) {
                    Segment tmp = sorted[i];
                    sorted[i] = sorted[j];
                    sorted[j] = tmp;
                }
            }
        }
        for (int i = 0; i < h->size; ++i) {
            int s = sorted[i].step;
            int is_power_of_2 = (s > 0 && (s & (s - 1)) == 0) ? 1 : 0;
            printf("  [%d]: i=%d, j=%d, step=%d, k=%d, err=%.6f%s\n",
                i, sorted[i].i, sorted[i].j, sorted[i].step, sorted[i].k, sorted[i].err,
                is_power_of_2 ? "" : " [WARNING: step not power of 2!]");
        }
        free(sorted);
    }
    printf("--------------------------------\n");
}

/* Forward declaration of find_max_error */
void find_max_error(double *x, double *y, double *gain, int i, int j, int *out_k, double *out_err, int mode);

/*
 * Find the segment with minimum error in the heap
 * Returns: index of min error segment in heap array, -1 if heap is empty
 */
int find_min_error_segment(MaxHeap* heap, double* out_min_err) {
    if (heap->size == 0) {
        *out_min_err = 0.0;
        return -1;
    }
    int min_idx = 0;
    double min_err = heap->data[0].err;
    for (int i = 1; i < heap->size; i++) {
        if (heap->data[i].err < min_err) {
            min_err = heap->data[i].err;
            min_idx = i;
        }
    }
    *out_min_err = min_err;
    return min_idx;
}

/*
 * Check if two segments can be merged
 * Condition: the longer segment's step must be divisible by the shorter segment's step
 */
int can_merge_segments(Segment* a, Segment* b) {
    int step_a = a->step;
    int step_b = b->step;
    if (step_a == 0 || step_b == 0) return 0;

    int min_step = (step_a < step_b) ? step_a : step_b;
    int max_step = (step_a > step_b) ? step_a : step_b;

    return (max_step % min_step == 0) ? 1 : 0;
}

/*
 * Merge two adjacent segments
 * seg_a and seg_b must be adjacent (a.j == b.i or b.j == a.i)
 * After merge: i = min(a.i, b.i), j = max(a.j, b.j)
 * step = min(a.step, b.step)
 */
Segment merge_two_segments(Segment* a, Segment* b, double* x, double* y, double* gain, int fit_mode) {
    Segment result;

    // Determine which segment is on the left
    if (a->i < b->i) {
        result.i = a->i;
        result.j = b->j;
        result.step = (a->step < b->step) ? a->step : b->step;
    }
    else {
        result.i = b->i;
        result.j = a->j;
        result.step = (a->step < b->step) ? a->step : b->step;
    }

    // Recalculate max error for the merged segment
    find_max_error(x, y, gain, result.i, result.j, &result.k, &result.err, fit_mode);

    return result;
}

/*
 * Find the left and right neighbor indices of target_idx in the heap
 * Note: This finds segments that are adjacent in index space (not heap structure)
 */
void find_neighbors_in_heap(MaxHeap* heap, int target_idx, int* left_idx, int* right_idx) {
    Segment target = heap->data[target_idx];
    *left_idx = -1;
    *right_idx = -1;

    int left_candidate = -1;
    int right_candidate = -1;

    for (int i = 0; i < heap->size; i++) {
        if (i == target_idx) continue;
        Segment seg = heap->data[i];

        // Check if seg ends where target starts (seg is on the left)
        if (seg.j == target.i) {
            if (left_candidate == -1 || seg.j > heap->data[left_candidate].j) {
                left_candidate = i;
            }
        }
        // Check if seg starts where target ends (seg is on the right)
        if (seg.i == target.j) {
            if (right_candidate == -1 || seg.i < heap->data[right_candidate].i) {
                right_candidate = i;
            }
        }
    }

    *left_idx = left_candidate;
    *right_idx = right_candidate;
}

/*
 * Error analysis loop function
 * - Find max error segment, dichotomy split, calculate MaxE
 * - Find min error segment, merge with smaller neighbor, calculate MinE
 * - If MaxE > MinE continue (do merge + split), otherwise restore and exit
 */
void error_analysis(double* x, double* y, double* gain, int fit_mode, MaxHeap* heap) {
#if PRINTF_INTERP_POINTS
    printf("start error_analysis.\n");
#endif

    const char *exit_reason = "unknown";
    while (1) {
#if PRINTF_INTERP_POINTS
        printf("new round error_analysis.\n");
#endif
        // Step B first: pop max error segment and calculate MaxE
        if (heap->size == 0) {
            exit_reason = "heap empty";
            break;
        }

        Segment seg_max = heap_pop(heap);

        // If this segment came from a merge, don't split it - exit loop
        if (seg_max.from_merge) {
#if PRINTF_INTERP_POINTS
            printf("seg_max(i=%d,j=%d) is from merge, cannot split, exit.\n", seg_max.i, seg_max.j);
#endif
            exit_reason = "seg_max from merge";
            heap_push(heap, seg_max);
            break;
        }

        double original_err = seg_max.err;  // 保存原始最大误差
        int k = (seg_max.i + seg_max.j) / 2;

        // Calculate errors for left and right parts
        double left_err = 0.0, right_err = 0.0;
        int temp_k = 0;

        find_max_error(x, y, gain, seg_max.i, k, &temp_k, &left_err, fit_mode);
        find_max_error(x, y, gain, k, seg_max.j, &temp_k, &right_err, fit_mode);

        double MaxE = (left_err < right_err) ? right_err : left_err;

#if PRINTF_INTERP_POINTS
        printf("seg_max(i=%d,j=%d) original_err=%.6f, MaxE=%.6f (left=%.6f, right=%.6f)\n",
            seg_max.i, seg_max.j, original_err, MaxE, left_err, right_err);
#endif

        // Step A: Sort remaining segments by i (left to right), try merging adjacent pairs
        // seg_max is already popped, not in heap
        double MinE = 0.0;
        int remove_idx[2] = { -1, -1 };      // actual heap indices for rebuild
        int remove_sorted[2] = { -1, -1 };    // sorted positions for print
        Segment best_merged = { 0 };
        int found_mergeable = 0;

        // Build a sorted list of indices by i (ascending - left to right)
        int *sorted_by_i = (int *)malloc(sizeof(int) * heap->size);
        for (int i = 0; i < heap->size; i++) {
            sorted_by_i[i] = i;
        }
        for (int i = 0; i < heap->size; i++) {
            for (int j = i + 1; j < heap->size; j++) {
                if (heap->data[sorted_by_i[j]].i < heap->data[sorted_by_i[i]].i) {
                    int tmp = sorted_by_i[i];
                    sorted_by_i[i] = sorted_by_i[j];
                    sorted_by_i[j] = tmp;
                }
            }
        }

        // Try merging adjacent pairs: (0,1), (1,2), (2,3), ...
        for (int p = 0; p < heap->size - 1; p++) {
            int idx_a = sorted_by_i[p];
            int idx_b = sorted_by_i[p + 1];
            Segment *seg_a = &heap->data[idx_a];
            Segment *seg_b = &heap->data[idx_b];

            // Check if they are actually adjacent (seg_a.j == seg_b.i)
            if (seg_a->j != seg_b->i) {
                continue; // not adjacent, skip
            }

            // Check if can merge
            if (!can_merge_segments(seg_a, seg_b)) {
                continue; // cannot merge, skip
            }

            // Can merge, calculate merged error
            Segment merged_seg = merge_two_segments(seg_a, seg_b, x, y, gain, fit_mode);
#if PRINTF_INTERP_POINTS
            printf("Try merge pair [sorted %d,%d]: seg(i=%d,j=%d) + neighbor(i=%d,j=%d) => merged(i=%d,j=%d,step=%d,err=%.6f)\n",
                p, p + 1, seg_a->i, seg_a->j, seg_b->i, seg_b->j,
                merged_seg.i, merged_seg.j, merged_seg.step, merged_seg.err);
#endif

            if (!found_mergeable || merged_seg.err < MinE) {
                MinE = merged_seg.err;
                best_merged = merged_seg;
                remove_idx[0] = idx_a;      // actual heap index
                remove_idx[1] = idx_b;       // actual heap index
                remove_sorted[0] = p;        // sorted position for print
                remove_sorted[1] = p + 1;    // sorted position for print
                found_mergeable = 1;
            }
        }

        free(sorted_by_i);

        // If no mergeable pair found, restore seg_max and exit
        if (!found_mergeable) {
#if PRINTF_INTERP_POINTS
            printf("No mergeable pair found, restore seg_max and exit.\n");
#endif
            exit_reason = "no mergeable pair";
            heap_push(heap, seg_max);
            break;
        }

#if PRINTF_INTERP_POINTS
        printf("Best merge: sorted[%d,%d] -> i=%d, j=%d, step=%d, err=%.6f\n",
            remove_sorted[0], remove_sorted[1], best_merged.i, best_merged.j, best_merged.step, best_merged.err);
#endif

        // Decision: continue if MinE < MaxE OR MinE < original_err * 0.85
        // Exit only when MinE >= MaxE AND MinE >= original_err * 0.85
        if (MinE >= MaxE && MinE >= original_err * 0.85) {
#if PRINTF_INTERP_POINTS
            printf("MinE=%.6f >= MaxE=%.6f and MinE >= original_err*0.85=%.6f, exit (restore seg_max)\n",
                MinE, MaxE, original_err * 0.85);
#endif
            exit_reason = "MinE >= MaxE && MinE >= original_err*0.85";
            heap_push(heap, seg_max);
            break;
        }

#if PRINTF_INTERP_POINTS
        printf("MinE=%.6f < MaxE=%.6f or MinE < original_err*0.85=%.6f, proceed with merge\n",
            MinE, MaxE, original_err * 0.85);
#endif

        // MinE < original_err: proceed with merge and split
        // Rebuild heap: remove the two segments, add merged
        Segment *temp = (Segment *)malloc(sizeof(Segment) * heap->capacity);
        int temp_size = 0;
        for (int i = 0; i < heap->size; i++) {
            if (i != remove_idx[0] && i != remove_idx[1]) {
                temp[temp_size++] = heap->data[i];
            }
        }
        free(heap->data);
        heap->data = temp;
        heap->size = temp_size;
        // heap->capacity remains unchanged

        best_merged.from_merge = 1;  // mark as merged, cannot be split later
        heap_push(heap, best_merged);

        // Step B: dichotomy split seg_max and add to heap
        Segment left_seg = { seg_max.i, k, 0, k - seg_max.i, 0.0, 0 };
        find_max_error(x, y, gain, left_seg.i, left_seg.j, &left_seg.k, &left_seg.err, fit_mode);
        heap_push(heap, left_seg);

        Segment right_seg = { k, seg_max.j, 0, seg_max.j - k, 0.0, 0 };
        find_max_error(x, y, gain, right_seg.i, right_seg.j, &right_seg.k, &right_seg.err, fit_mode);
        heap_push(heap, right_seg);

#if PRINTF_INTERP_POINTS
        printf("Split seg_max(i=%d,j=%d) into left(i=%d,j=%d) and right(i=%d,j=%d)\n",
            seg_max.i, seg_max.j, left_seg.i, left_seg.j, right_seg.i, right_seg.j);
        heap_print_errors(heap);
#endif
    }

#if PRINTF_INTERP_POINTS
    printf("error_analysis finished (reason: %s).\n", exit_reason);
#endif
}

/*
 * Find the index of the maximum error in the segment defined by (i,j) using
 * a ternary search on a unimodal error function. After narrowing down to a small range,
 * perform a final linear scan to find the exact maximum error point.
 */
void find_max_error(double *x, double *y, double *gain, int i, int j, int *out_k, double *out_err, int mode) {
    int left = i + 1;  // start checking after i
    int right = j - 1; // check up to just before j
    double e = 0, e1 = 0, e2 = 0; // variables for error comparison

    // if there are no points to check, set output to defaults and return
    if (left > right) {
        *out_k = i;  // max error point is at the start
        *out_err = 0.0;  // no error
        return;
    }

    // ternary search to narrow down to a small range
    while (right - left > 3) {
        int m1 = left + (right - left) / 3;  // first middle point
        int m2 = right - (right - left) / 3; // second middle point
        // calculate errors based on the mode
        if (mode == 0) {
            e1 = point_error(y, i, j, m1);
            e2 = point_error(y, i, j, m2);
        }
        else if (mode == 1) {
            e1 = point_error_gain(x, y, gain, i, j, m1);
            e2 = point_error_gain(x, y, gain, i, j, m2);
        }
        else {
            assert(0); // incorrect mode
        }

        // adjust search range based on which error is larger
        if (e1 < e2)
            left = m1 + 1;  // search right
        else
            right = m2 - 1; // search left
    }

    /* final scan over small interval to find exact max error */
    double best_err = -1.0;  // initialize best error to negative
    int best_k = left; // record the best index

    // linear scan to find exact maximum error point within the small interval
    for (int k = left; k <= right; k++) {
        if (mode == 0) {
            e = point_error(y, i, j, k);
        }
        else if (mode == 1) {
            e = point_error_gain(x, y, gain, i, j, k);
        }
        else {
            assert(0); // incorrect mode
        }

        // update best error and corresponding index if a larger error is found
        if (e > best_err) {
            best_err = e;
            best_k = k;
        }
    }
    *out_k = best_k;    // output maximum error index
    *out_err = best_err; // output maximum error value
}

/* Calculate the number of entries based on the segments in the heap */
int gen_entry_num(MaxHeap* heap) {
    int entries = 0; // initialize entry count
    for (int i = 0; i < heap->size; ++i) {
        Segment seg = heap->data[i];  // get segment
        // count how many entries this segment contributes
        entries += seg.step > 0 ? (seg.j - seg.i) / seg.step : 1;
    }
    entries += 1;  // Adding 1 to count the endpoint of segments
    return entries; // return total count
}

/* Insert new steps based on the segments from the max-heap */
void insert_steps(double* x, double* y, double* gain, int* len_entry, int fit_mode, MaxHeap* heap) {
    int cur_entries = gen_entry_num(heap); // current number of computed entries

    Segment *temp_segments = (Segment *)malloc(sizeof(Segment) * heap->size); // allocate space for temporary segments
    int temp_count = 0;
    //int prf_c = 0;
    while (cur_entries < *len_entry && heap->size > 0) { // while we can insert more entries
#if PRINTF_INTERP_STEPS
        heap_print_errors(heap);
#endif
        Segment s = heap_pop(heap);  // pop the segment with the maximum error
        int step = s.step;
        if (step <= 1) {   // if segment is too small
#if PRINTF_INTERP_STEPS
            printf("step:%d,s.i:%d,s.j:%d\n", step, s.i, s.j);
#endif
            heap_push(heap, s); // push back the segment into the heap
            break;
        }

        int new_step = step >> 1; // reduce the step size

        int entry_increase = (s.j - s.i) / new_step - (s.j - s.i) / step;  // calculate new entries from reduced step
#if PRINTF_INTERP_STEPS
        printf("entry_increase:%d,s.err:%.6f,s.i:%d,s.j:%d,new_step:%d\n", entry_increase, s.err, s.i, s.j, new_step);
#endif
        if (cur_entries + entry_increase > *len_entry) { // if it exceeds allowed entries
            temp_segments[temp_count++] = s; // store it temporarily
#if PRINTF_INTERP_STEPS
            printf("heap_pop\n");
#endif
            continue; // skip processing
        }
        else {
            cur_entries += entry_increase; // update current entries

            double max_error = -1.0; // track the maximum error
            int max_k = 0;
            for (int k = s.i; k < s.j; k += new_step) {
                double error = 0; // error for the current segment
                int temp_k = 0; // index for maximum error point
                // find the max error for this segment portion
                find_max_error(x, y, gain, k, (k + new_step < s.j) ? (k + new_step) : s.j, &temp_k, &error, fit_mode);
                //printf("range:%d %d\n",k,(k + new_step < s.j) ? (k + new_step) : s.j);  

                if (error > max_error) { // if found a larger error
                    max_k = temp_k; // update index k
                    max_error = error; // update the maximum error
                }
            }
            s.k = max_k;
            s.err = max_error;
            s.step = new_step; // update the step size

            heap_push(heap, s); // push the updated segment back into the heap
        }
    }

    for (int i = 0; i < temp_count; i++) {
        heap_push(heap, temp_segments[i]); // restore temporarily saved segments to the heap
    }
    free(temp_segments); // free temporary segment array

    *len_entry = cur_entries; // update the entry count
}

/* Generate output from the segments stored in the heap */
void gen_output_from_heap(int* pts_x, int* pts_steps, int len_y, int* len_p, MaxHeap* heap) {
    int count = 0; // initialization of count

    int *temp_pts_x = (int*)malloc(sizeof(int) * (heap->size * 2)); // temp storage for segment indices
    int *temp_pts_steps = (int*)malloc(sizeof(int) * (heap->size * 2)); // temp storage for segment steps

    for (int i = 0; i < heap->size; ++i) {
        Segment seg = heap->data[i]; // get segment
        temp_pts_steps[count] = seg.step; // store the step
        temp_pts_x[count] = seg.i; // store the starting index
        count++;  // increment count
    }

    // Simple sorting algorithm: sort by segments' starting indices
    for (int i = 0; i < count; i++) {
        for (int j = i + 1; j < count; j++) {
            if (temp_pts_x[i] > temp_pts_x[j]) { // if current index is greater than next
                int tmp1 = temp_pts_x[i]; // swap X indices
                temp_pts_x[i] = temp_pts_x[j];
                temp_pts_x[j] = tmp1;

                int tmp2 = temp_pts_steps[i]; // swap steps
                temp_pts_steps[i] = temp_pts_steps[j];
                temp_pts_steps[j] = tmp2;
            }
        }
    }

    // Copy sorted points back to original arrays and add endpoint
    for (int i = 0; i < count; i++) {
        pts_steps[i] = temp_pts_steps[i]; // copy steps
        pts_x[i] = temp_pts_x[i]; // copy indices
    }
    pts_x[count] = len_y - 1; // add last endpoint
    count++;
    *len_p = count; // update length of points

    // Free temporary arrays
    free(temp_pts_x);
    free(temp_pts_steps);
}

/* Merge points with the same step size */
void merge_same_step_points(int* pts_x, int* pts_steps, int* len_p) {
    int count = 0; // count of unique points

    int *temp_pts_x = (int*)malloc(sizeof(int) * (*len_p)); // temporary storage for merged points
    int *temp_pts_steps = (int*)malloc(sizeof(int) * (*len_p)); // temporary storage for merged steps

    temp_pts_x[0] = pts_x[0]; // start with first point
    temp_pts_steps[0] = pts_steps[0]; // store its step
    count++;

    for (int i = 1; i < *len_p; ++i) {
        // check if the current step is different from the previous one
        if (pts_steps[i] != pts_steps[i - 1]) {
            temp_pts_x[count] = pts_x[i]; // store new point
            temp_pts_steps[count] = pts_steps[i]; // store corresponding step
            count++; // increment count
        }
        else {
            continue; // skip if same as previous
        }
    }

    // Copy merged results back to the original arrays
    for (int i = 0; i < count; i++) {
        pts_steps[i] = temp_pts_steps[i];
        pts_x[i] = temp_pts_x[i];
    }
    *len_p = count; // update total length

    free(temp_pts_x); // free temporary storage
    free(temp_pts_steps);
}

void insert_points(double* x, double* y, double* gain, int len_p, int fit_dichotomy, int* interp_step_flag, int fit_mode, MaxHeap* heap)
{
    /* Iteratively insert points according to the number of points to fit */
    for (int t = 0; t < len_p - 2; t++) {
        Segment s = heap_pop(heap); // pop the top segment
#if PRINTF_INTERP_POINTS
        printf("s.err=%.6f,s.i:%d,s.j:%d\n", s.err, s.i, s.j);
#endif
        // check conditions for termination
        if (s.err == 0 || s.j - s.i <= 1 || s.k == s.i || s.k == s.j) {
            heap_push(heap, s); // push back if conditions are met
            *interp_step_flag = 0; // mark that no more interpolation is needed
            break;
        }
        if (fit_dichotomy) {
            s.k = (s.i + s.j) / 2; // set k to midpoint if using dichotomy
        }

        /* Split segment into left and right parts, pushing back if valid length */
        Segment left_seg = { s.i, s.k, 0, s.k - s.i, 0.0, 0 }; // left segment
        find_max_error(x, y, gain, left_seg.i, left_seg.j, &left_seg.k, &left_seg.err, fit_mode); // find max error
        heap_push(heap, left_seg); // push left segment into heap

        Segment right_seg = { s.k, s.j, 0, s.j - s.k, 0.0, 0 }; // right segment
        find_max_error(x, y, gain, right_seg.i, right_seg.j, &right_seg.k, &right_seg.err, fit_mode); // find max error
        heap_push(heap, right_seg); // push right segment into heap

#if PRINTF_INTERP_POINTS
        heap_print_errors(heap);
#endif
    }
}

/* Main curve fitting function */
void curve_fitting(fit_params* params) {
    double* x_ptr = params->x; // input x values
    double* y_ptr = params->y; // input y values
    double* gain_ptr = params->gain; // input gain values
    int len_y = params->len_y; // length of input y values
    int* len_p = params->len_p; // pointer to output length
    int* pts_x_ptr = params->pts_x; // pointer to output x indices
    int* pts_step_ptr = params->pts_step; // pointer to output step sizes
    int* len_entry_ptr = params->len_entry; // pointer to entry length
    int fit_mode = params->fit_mode; // fitting mode: 0 for value, 1 for gain
    int fit_dichotomy = params->fit_dichotomy; // flag for using dichotomy strategy

    int len_p_value = *len_p; // current length of output points
    int interp_step_flag = *len_entry_ptr > len_p_value ? 1 : 0; // check if interpolation step is needed

    /* Initialize the max-heap */
#if PRINTF_INTERP_POINTS
    printf("start heap_init.\n");
#endif
    MaxHeap heap;
    heap_init(&heap, len_p_value * 2); // set up heap capacity

    /* Push the initial segment covering the entire range [1,M] */
    Segment init = { 0, len_y - 1, 0, len_y - 1, 0.0, 0 }; // initialize the segment with endpoints
    find_max_error(x_ptr, y_ptr, gain_ptr, init.i, init.j, &init.k, &init.err, fit_mode); // find max error for this segment
    heap_push(&heap, init); // push initial segment into heap

#if PRINTF_INTERP_POINTS
    printf("start Iteratively insert points.\n");
#endif

    insert_points(x_ptr, y_ptr, gain_ptr, len_p_value, fit_dichotomy, &interp_step_flag, fit_mode, &heap);

#if PRINTF_INTERP_STEPS
    printf("\ninterp_step_flag:%d\n", interp_step_flag);
#endif   

    // If interpolation steps are required, call the insert_steps method
    if (interp_step_flag) {
        error_analysis(x_ptr, y_ptr, gain_ptr, fit_mode, &heap);
        insert_steps(x_ptr, y_ptr, gain_ptr, len_entry_ptr, fit_mode, &heap);
    }

#if PRINTF_INTERP_STEPS
    printf("final heap:\n");
    heap_print_errors(&heap);
#endif  

    // Generate the output from the segments held in the heap
    gen_output_from_heap(pts_x_ptr, pts_step_ptr, len_y, len_p, &heap);

    //If interpolation steps were added, merge points with same step sizes
    if (interp_step_flag) {
        merge_same_step_points(pts_x_ptr, pts_step_ptr, len_p);
    }

    /* Clean up is important to prevent memory leaks */
    free(heap.data); // free the heap's segment storage
    return;
}


/**
 * Perform linear interpolation.
 *
 * @param y Input array of y-values.
 * @param len Length of the input array.
 * @param len_out Length of the output array.
 * @param y_out Pointer to the output array to be filled with interpolated values.
 */
void linear_interpolate(double* y, int len, int len_out, double* y_out) {
    if (len == len_out)
    {
        memcpy(y_out, y, len * sizeof(double));
        return;
    }

    // Calculate the step size for interpolation
    double step = (double)(len - 1) / (len_out - 1);

    for (int i = 0; i < len_out; i++) {
        // Find the two nearest points in the original array
        double idx = i * step;
        int left_idx = (int)idx;
        int right_idx = left_idx + 1;

        // Prevent out-of-bounds access
        if (right_idx >= len) {
            right_idx = len - 1;
        }

        // Calculate interpolation
        double x0 = y[left_idx];
        double x1 = y[right_idx];
        double alpha = idx - left_idx;

        // Linear interpolation formula
        y_out[i] = x0 + (x1 - x0) * alpha;
    }
}

void np_interp(double* fx, double* fy, int f_len, double* x, double* in_y, int x_len) {
    for (int i = 0; i < x_len; i++) {
        double target = x[i];

        int j = 0;
        while (j < f_len - 1 && fx[j + 1] < target) {
            j++;
        }

        if (j == f_len - 1) {
            in_y[i] = fy[f_len - 1];
        }
        else if (j == 0) {
            in_y[i] = fy[0];
        }
        else {
            double t = (target - fx[j]) / (fx[j + 1] - fx[j]);
            in_y[i] = fy[j] + t * (fy[j + 1] - fy[j]);
        }
    }
}
