

#ifndef CURVE_LUT_H
#define CURVE_LUT_H

#include <stddef.h>
#include <stdint.h>

/* Define CURVE_LUT_EXTERNAL_U32 before including this header if the project
 * already provides U32. U32 must be an unsigned 32-bit integer type.
 */
#ifndef CURVE_LUT_EXTERNAL_U32
typedef uint32_t U32;
#endif

#ifdef __cplusplus
extern "C" {
#endif

    typedef double(*curve_function)(double x);

    enum {
        CURVE_LUT_OK = 0,
        CURVE_LUT_INVALID_ARGUMENT = -1,
        CURVE_LUT_BUFFER_TOO_SMALL = -2,
        CURVE_LUT_INVALID_VALUE = -3,
        CURVE_LUT_OUT_OF_MEMORY = -4
    };

    /* Step 1: Expand the segments into LUT input coordinates.
     * RegPoint contains RegCount INTERIOR endpoints only, excluding 0 and
     * (1 << inBit). Both boundary endpoints are added internally.
     * Interior endpoints must strictly increase within (0, 1 << inBit).
     * RegStep contains RegCount+1 positive entries, one per completed segment;
     * no trailing zero is needed. RegPoint may be NULL when RegCount is 0.
     * Generate each segment over [start, end), then append (1 << inBit).
     * inBit must be in [1, 31] so the final endpoint fits in U32.
     * A step does not have to divide its segment length exactly.
     * Set curve_x=NULL and capacity=0 to query the required entry_count only.
     * If capacity is insufficient, return the required entry_count without
     * writing to curve_x.
     */
    int generate_curve_x(const U32 *RegPoint, const U32 *RegStep,
        U32 RegCount, U32 inBit, U32 *curve_x,
        U32 capacity, U32 *entry_count);

    /* Step 2: Call CurveFun at each node to generate integer LUT values.
     * max_x = curve_x[entry_count-1] must be positive, e.g. 4096 or 16777216.
     * RegData[i] = clip(round_even(CurveFun((double)curve_x[i]/max_x) * maxvalue),
     *                0, maxvalue)
     * maxvalue is the maximum output value, e.g. (UINT32_C(1)<<24)-1.
     * Like np.round, round_even rounds exact .5 ties to the nearest even integer;
     * it does not use C round().
     * NaN returns INVALID_VALUE. Positive/negative infinity is clipped to
     * maxvalue/0, respectively. Entries preceding a NaN may already be written.
     * Input and output buffers must not overlap.
     */
    int generate_curve_data(curve_function CurveFun, U32 maxvalue,
        const U32 *curve_x, U32 entry_count,
        U32 *RegData, U32 capacity);

    /* Combined interface: Execute the two steps above in sequence.
     * RegCount is the number of INTERIOR points, excluding 0 and (1 << inBit).
     * The boundaries are added internally; RegStep must contain RegCount+1
     * positive entries. The expanded LUT entry count is calculated internally.
     * The same input constraints as generate_curve_x apply.
     * Return the actual number of generated entries on success (e.g. 129),
     * or a negative CURVE_LUT_* error code on failure.
     * Entry counts exceeding INT_MAX return CURVE_LUT_INVALID_ARGUMENT.
     * Only RegData is output. Input nodes are allocated and freed internally.
     * RegData must have space for at least capacity U32 entries.
     * To query the required count first, call
     * generate_curve_x(..., NULL, 0, &entry_count) separately.
     */
    int generate_curve_lut(curve_function CurveFun, U32 maxvalue,
        const U32 *RegPoint, const U32 *RegStep,
        U32 RegCount, U32 inBit,
        U32 *RegData, U32 capacity);

#ifdef __cplusplus
}
#endif
#endif
