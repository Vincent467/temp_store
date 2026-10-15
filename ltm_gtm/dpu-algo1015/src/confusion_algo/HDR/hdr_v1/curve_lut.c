#include "curve_lut.h"

#include <math.h>
#include <limits.h>
#include <stdlib.h>

int generate_curve_x(const U32 *RegPoint, const U32 *RegStep,
    U32 RegCount, U32 inBit, U32 *curve_x,
    U32 capacity, U32 *entry_count)
{
    U32 required = 1; /* Include the final endpoint. */
    U32 index = 0;
    U32 i;
    U32 max_x;

    if (entry_count == NULL)
        return CURVE_LUT_INVALID_ARGUMENT;
    *entry_count = 0;
    if ((RegCount > 0 && RegPoint == NULL) || RegStep == NULL || inBit < 1 || inBit > 31)
        return CURVE_LUT_INVALID_ARGUMENT;
    max_x = (uint32_t)1 << inBit;
    if (RegCount > (U32)(max_x - 1))
        return CURVE_LUT_INVALID_ARGUMENT;

    /* Validate arguments and count entries before writing to the buffer. */
    for (i = 0; i <= RegCount; ++i) {
        U32 start = i == 0 ? 0 : RegPoint[i - 1];
        U32 end = i == RegCount ? max_x : RegPoint[i];
        uint64_t width = (uint64_t)end - start;
        U32 count;
        if (end <= start || end > max_x || RegStep[i] <= 0)
            return CURVE_LUT_INVALID_ARGUMENT;
        count = (U32)((width + RegStep[i] - 1) / RegStep[i]);
        if (count > UINT32_MAX - required)
            return CURVE_LUT_INVALID_ARGUMENT;
        required += count;
    }
    *entry_count = required;
    if (curve_x == NULL)
        return capacity == 0 ? CURVE_LUT_OK : CURVE_LUT_INVALID_ARGUMENT;
    if (capacity < required)
        return CURVE_LUT_BUFFER_TOO_SMALL;

    for (i = 0; i <= RegCount; ++i) {
        U32 start = i == 0 ? 0 : RegPoint[i - 1];
        U32 end = i == RegCount ? max_x : RegPoint[i];
        uint64_t x;
        for (x = start; x < end; x += RegStep[i])
            curve_x[index++] = (U32)x;
    }
    curve_x[index] = max_x;
    return CURVE_LUT_OK;
}

int generate_curve_data(curve_function CurveFun, U32 maxvalue,
    const U32 *curve_x, U32 entry_count,
    U32 *RegData, U32 capacity)
{
    U32 i;
    double max_x;
    if (CurveFun == NULL || curve_x == NULL || RegData == NULL ||
        entry_count < 2)
        return CURVE_LUT_INVALID_ARGUMENT;
    if (capacity < entry_count)
        return CURVE_LUT_BUFFER_TOO_SMALL;
    for (i = 1; i < entry_count; ++i) {
        if (curve_x[i] <= curve_x[i - 1])
            return CURVE_LUT_INVALID_ARGUMENT;
    }
    max_x = (double)curve_x[entry_count - 1];

    for (i = 0; i < entry_count; ++i) {
        double x = (double)curve_x[i] / max_x;
        double value = CurveFun(x) * (double)maxvalue;
        double integer_part;
        double fraction;
        if (isnan(value))
            return CURVE_LUT_INVALID_VALUE;
        /* Clip before converting to U32 to avoid out-of-range conversion. */
        if (value <= 0.0) {
            RegData[i] = 0;
            continue;
        }
        if (value >= (double)maxvalue) {
            RegData[i] = maxvalue;
            continue;
        }

        integer_part = floor(value);
        fraction = value - integer_part;
        if (fraction > 0.5 || (fraction == 0.5 && fmod(integer_part, 2.0) != 0.0))
            integer_part += 1.0;
        RegData[i] = integer_part >= (double)maxvalue ? maxvalue : (U32)integer_part;
    }
    return CURVE_LUT_OK;
}

int generate_curve_lut(curve_function CurveFun, U32 maxvalue,
    const U32 *RegPoint, const U32 *RegStep,
    U32 RegCount, U32 inBit,
    U32 *RegData, U32 capacity)
{
    int status;
    U32 entry_count;
    U32 *curve_x;
    if (CurveFun == NULL || RegData == NULL)
        return CURVE_LUT_INVALID_ARGUMENT;

    /* Query the node count before allocating temporary storage. */
    status = generate_curve_x(RegPoint, RegStep, RegCount, inBit, NULL, 0, &entry_count);
    if (status != CURVE_LUT_OK)
        return status;
    if (entry_count > INT_MAX)
        return CURVE_LUT_INVALID_ARGUMENT;
    if (capacity < entry_count)
        return CURVE_LUT_BUFFER_TOO_SMALL;
    if (SIZE_MAX / entry_count < sizeof(*curve_x))
        return CURVE_LUT_INVALID_ARGUMENT;
    curve_x = malloc((size_t)entry_count * sizeof(*curve_x));
    if (curve_x == NULL)
        return CURVE_LUT_OUT_OF_MEMORY;

    /* Step 1: Generate input nodes internally. */
    status = generate_curve_x(RegPoint, RegStep, RegCount, inBit,
        curve_x, entry_count, &entry_count);
    /* Step 2: Evaluate the callback and write only RegData to the caller. */
    if (status == CURVE_LUT_OK)
        status = generate_curve_data(CurveFun, maxvalue, curve_x, entry_count, RegData, capacity);
    free(curve_x);
    if (status != CURVE_LUT_OK)
        return status;
    return (int)entry_count;
}


