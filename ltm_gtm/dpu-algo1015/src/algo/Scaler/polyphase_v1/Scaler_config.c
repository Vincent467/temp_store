#include "sw_base.hpp"

#define FACTOR_PRECISION 24
#define INTERPOLATION 1
#define DUMP_TABLE 0


/* --- built-in pipeline constants --- */
#define PPC_T              32
#define PPC_OVERLAP         4   /* Lanczos a=4 kernel margin, handled by SCL internally */

static inline int32_t div_ceil_i32(int32_t A, int32_t B)
{
    assert(B > 0);

    if (A >= 0) {
        return A / B + (A % B != 0);
    }

    return A / B;
}

API_EXPORT uint64_t calc_factor(int input_size, int output_size)
{
    if (output_size > 1)
        return ((uint64_t)(input_size) << FACTOR_PRECISION) / (uint64_t)(output_size);
    return (uint64_t)1 << FACTOR_PRECISION;
}

API_EXPORT int calc_initial_offset(int input_size, int output_size)
{
    if (input_size == output_size)
        return 0;

    uint64_t f = calc_factor(input_size, output_size);

#if INTERPOLATION
    int io = f / 2 - (1 << (FACTOR_PRECISION - 1));
#else
    int io = f / 2 - (1 << (FACTOR_PRECISION - 1)) + (1 << (FACTOR_PRECISION - 6));
#endif
    return  io;
}

static float _LinearFilter(float x)
{
    if (x < 0.)
        x = -x;
    if (x > 1.)
        return 0.;
    return 1. - x;
}

static double _CMitchellFilter(double t)
{
    //    const double B = 1.0 / 3.0; //Mitchell proposal
    //    const double C = 1.0 / 3.0;
    const double B = 0.;
    const double C = 0.75;   //OpenCV bi-cubic
    double tt = t * t;
    if (t < 0)
        t = -t;

    if (t < 1.0)
    {
        t = (((12.0 - 9.0 * B - 6.0 * C) * (t * tt))
            + ((-18.0 + 12.0 * B + 6.0 * C) * tt)
            + (6.0 - 2 * B));
        return(t / 6.0);
    }
    else if (t < 2.0)
    {
        t = (((-1.0 * B - 6.0 * C) * (t * tt))
            + ((6.0 * B + 30.0 * C) * tt)
            + ((-12.0 * B - 48.0 * C) * t)
            + (8.0 * B + 24 * C));
        return t / 6.0;
    }

    return 0.0;
}

static float _SincFilter(
    float x,
    int32_t radius)
{
    float pit, pitd, f1, f2, result;
    float fRadius = (float)radius;

    if (x == 0.0f)
    {
        result = 1.0f;
    }
    else if ((x < -fRadius) || (x > fRadius))
    {
        result = 0.0f;
    }
    else
    {
        pit = (float)(3.14159265358979323846f * x);
        pitd = (float)(pit / fRadius);

        f1 = (float)(sin(pit) / pit);
        f2 = (float)(sin(pitd) / pitd);

        result = (float)(f1 * f2);
    }

    return result;
}

uint32_t CalculatePhase(float fScale, const uint32_t taps, float* phases)
{
    LOGD("[%s,%d] CalculatePhase: fScale=%f, taps=%d\n", __FUNCTION__, __LINE__, fScale, taps);

    /* Adjust the factor for magnification. */
    if (fScale > 1.0f)
    {
        fScale = 1.0f;
    }
    LOGD("[%s,%d] CalculatePhase: adjusted fScale=%f\n", __FUNCTION__, __LINE__, fScale);

    /* Init the subpixel offset. */
    float fSubpixelOffset = 0.0f;

    /* Calculate the subpixel step. */
    float fSubpixelStep = (float)(1.0f / ((float)SW_SAMPLE_BINCOUNT));

    int32_t kernelHalf = taps >> 1;
    int32_t phases_offset = 0;

    for (uint32_t subpixelPos = 0; subpixelPos < SW_TABLE_BINCOUNT; subpixelPos++)
    {
        /* Compute weights. */
        for (uint32_t kernelPos = 0; kernelPos < taps; kernelPos++)
        {
            if (taps == 1)
            {
                phases[phases_offset + kernelPos] = 1.0f;
            }
            else
            {
                /* Compute the x position for filter function. */
                float fX = ((float)(kernelHalf - (int32_t)kernelPos - 1) + fSubpixelOffset) * fScale;
                phases[phases_offset + kernelPos] = fX;
            }

        }

        phases_offset += taps;


        /* Advance to the next subpixel. */
        fSubpixelOffset = fSubpixelOffset + fSubpixelStep;
    }

#if DUMP_TABLE
    char filename[128];
    sprintf(filename, "Scaler_Phase_%dtap.csv", taps);
    FILE *fp = fopen(filename, "w");
    for (uint32_t subpixelPos = 0; subpixelPos < SW_TABLE_BINCOUNT; subpixelPos++)
    {
        for (uint32_t kernelPos = 0; kernelPos < taps; kernelPos++)
        {
            uint32_t idx = subpixelPos * taps + kernelPos;
            fprintf(fp, "%.5f,", phases[idx]);
        }
        fprintf(fp, "\n");
    }
    fclose(fp);
#endif // 1

    return 0;
}

uint32_t CalculateCoefficients(const uint32_t ScalerMode, const uint32_t taps, float* phases, float* coefficients)
{
    int32_t kernelHalf = (int32_t)(taps >> 1);
    for (uint32_t subpixelPos = 0; subpixelPos < SW_TABLE_BINCOUNT; subpixelPos++)
        for (uint32_t kernelPos = 0; kernelPos < taps; kernelPos++)
        {
            uint32_t idx = subpixelPos * taps + kernelPos;
            switch (ScalerMode)
            {
            case SW_SCALER_BI_LINEAR:
                coefficients[idx] = (float)_LinearFilter(phases[idx]);
                break;
            case SW_SCALER_BI_CUBIC:
                coefficients[idx] = (float)_CMitchellFilter(phases[idx]);
                break;
            case SW_SCALER_LANCZOS:
                coefficients[idx] = (float)_SincFilter(phases[idx], kernelHalf);
                break;
            default:
                break;
            }

        }

#if DUMP_TABLE
    char filename[128];
    switch (ScalerMode) {
    case SW_SCALER_BI_LINEAR:
        sprintf(filename, "Scaler_%s_coefficient_%dtap.csv", "BI_LINEAR", taps);
        break;
    case SW_SCALER_BI_CUBIC:
        sprintf(filename, "Scaler_%s_coefficient_%dtap.csv", "BI_CUBIC", taps);
        break;
    case SW_SCALER_LANCZOS:
        sprintf(filename, "Scaler_%s_coefficient_%dtap.csv", "LANCZOS", taps);
        break;
    default:
        break;
    }

    FILE *fp = fopen(filename, "w");
    for (uint32_t subpixelPos = 0; subpixelPos < SW_TABLE_BINCOUNT; subpixelPos++)
    {
        for (uint32_t kernelPos = 0; kernelPos < taps; kernelPos++)
        {
            uint32_t idx = subpixelPos * taps + kernelPos;
            fprintf(fp, "%.5f,", coefficients[idx]);
        }
        fprintf(fp, "\n");
    }
    fclose(fp);
#endif // 1

    return 0;
}

uint32_t FixCoefficients(const uint32_t taps, float* coeffis, uint16_t* coeffis_i)
{

    for (uint32_t subpixelPos = 0; subpixelPos < SW_TABLE_BINCOUNT; subpixelPos++)
    {
        float fWeightSum = 0;
        uint16_t weightSum = 0;
        int16_t adjustCount = 0, adjustFrom = 0;
        int16_t adjustment = 0;

        //float sum
        for (uint32_t kernelPos = 0; kernelPos < taps; kernelPos++)
        {
            uint32_t idx = subpixelPos * taps + kernelPos;
            fWeightSum += coeffis[idx];
        }

        /* Adjust weights so that the sum will be 1.0. */
        for (uint32_t kernelPos = 0; kernelPos < taps; kernelPos++)
        {
            uint32_t idx = subpixelPos * taps + kernelPos;

            /* Normalize the current weight. */
            float fWeight = coeffis[idx] / fWeightSum;

            /* Convert the weight to fixed point and store in the table. */
            if (fWeight == 0.0f)
            {
                coeffis_i[idx] = 0x0000;
            }
            else if (fWeight >= 1.0f)
            {
                coeffis_i[idx] = 0x4000;
            }
            else if (fWeight <= -1.0f)
            {
                coeffis_i[idx] = 0xC000;
            }
            else
            {
                coeffis_i[idx] = (int16_t)(fWeight * 16384.0f);
            }

            weightSum += coeffis_i[idx];

        }

        /* Adjust the fixed point coefficients. */
        adjustCount = 0x4000 - weightSum;
        if (adjustCount < 0)
        {
            adjustCount = -adjustCount;
            adjustment = -1;
        }
        else
        {
            adjustment = 1;
        }

        adjustFrom = (taps - adjustCount) / 2;//add lost from floor rounding

        for (uint32_t kernelPos = 0; kernelPos < (uint32_t)(adjustCount); kernelPos++)
        {
            uint32_t idx = subpixelPos * taps + kernelPos;

            coeffis_i[adjustFrom + idx] += adjustment;
        }
    }
    return 0;
}

uint32_t CalculateSyncTable(
    float  fScale,
    const uint32_t ScalerMode,
    const uint32_t KernelSize,
    uint16_t* kernelArray)
{
    LOGD("[%s,%d] CalculateSyncTable: fScale=%f, ScalerMode=%d, KernelSize=%d\n", __FUNCTION__, __LINE__, fScale, ScalerMode, KernelSize);

    float* phases = (float*)calloc(SW_TABLE_BINCOUNT*KernelSize, sizeof(float));
    float* coefficients = (float*)calloc(SW_TABLE_BINCOUNT*KernelSize, sizeof(float));
    if (phases == NULL || coefficients == NULL)
    {
        LOGE("calloc allocation failure.");
        ASSERT(0);
    }

    CalculatePhase(fScale, KernelSize, phases);

    CalculateCoefficients(ScalerMode, KernelSize, phases, coefficients);

    FixCoefficients(KernelSize, coefficients, kernelArray);

    free(phases);
    free(coefficients);

    /* Success. */
    return 1;
}

API_EXPORT void Set_Scaler_Config(IN Scaler_User_Config* Scaler_User_Config, OUT SW_Scaler_Config* Params)
{
    uint32_t scaler_mode = Scaler_User_Config->scaler_mode;
    uint32_t srcWidth = Scaler_User_Config->srcWidth;
    uint32_t srcHeight = Scaler_User_Config->srcHeight;
    uint32_t dstWidth = Scaler_User_Config->dstWidth;
    uint32_t dstHeight = Scaler_User_Config->dstHeight;

    LOGI("scale (%d, %d) -> (%d, %d)\n", srcWidth, srcHeight, dstWidth, dstHeight);
    switch (scaler_mode) {
    case SW_SCALER_BI_LINEAR:
        LOGI("scaler_mode:BI_LINEAR\n");
        break;
    case SW_SCALER_BI_CUBIC:
        LOGI("scaler_mode:BI_CUBIC\n");
        break;
    case SW_SCALER_LANCZOS:
        LOGI("scaler_mode:LANCZOS\n");
        break;
    default:
        break;
    }

    if (scaler_mode == SW_SCALER_Programable)
    {
        int line_count_hor = 0;
        int line_count_ver = 0;

        VariableLine* lines_hor = NULL;
        VariableLine* lines_ver = NULL;

        lines_hor = parse_csv_only_data(Scaler_User_Config->fileName_hor, 0, 0, &line_count_hor);
        lines_ver = parse_csv_only_data(Scaler_User_Config->fileName_ver, 0, 0, &line_count_ver);

        if (lines_hor == NULL || lines_ver == NULL)
        {
            LOGE("parse_csv_only_data failure.");
            ASSERT(0);
        }

        float coefficients_hor[SW_TABLE_BINCOUNT * SW_HOR_KERNEL_SIZE] = { 0 };
        float coefficients_ver[SW_TABLE_BINCOUNT * SW_VER_KERNEL_SIZE] = { 0 };

        if (line_count_hor != SW_TABLE_BINCOUNT || line_count_ver!= SW_TABLE_BINCOUNT || lines_hor->count != SW_HOR_KERNEL_SIZE || lines_ver->count != SW_VER_KERNEL_SIZE)
            LOGE("Error Scaler Programable file context\n");
        for (int i = 0; i < SW_TABLE_BINCOUNT;++i)
            for (int j = 0; j < SW_HOR_KERNEL_SIZE; ++j)
            {
                coefficients_hor[i*SW_HOR_KERNEL_SIZE +j] = lines_hor[i].vars[j];
                if (j < SW_VER_KERNEL_SIZE)
                {
                    coefficients_ver[i * SW_VER_KERNEL_SIZE + j] = lines_ver[i].vars[j];
                }
            }

        FixCoefficients(SW_HOR_KERNEL_SIZE, coefficients_hor, Params->HoriKernelArray);
        FixCoefficients(SW_VER_KERNEL_SIZE, coefficients_ver, Params->VertiKernelArray);

        free_variable_lines(lines_hor,line_count_hor);
        free_variable_lines(lines_ver,line_count_ver);

    }
    else
    {
        CalculateSyncTable(((float)dstWidth) / ((float)srcWidth), scaler_mode, SW_HOR_KERNEL_SIZE, Params->HoriKernelArray);
        CalculateSyncTable(((float)dstHeight) / ((float)srcHeight), scaler_mode, SW_VER_KERNEL_SIZE, Params->VertiKernelArray);
    }


    //printf("[%s,%d] Params->HoriKernelArray ptr: %p\n", __FUNCTION__, __LINE__, (void*)(Params->HoriKernelArray));

    Params->ScaleFactorX = calc_factor(srcWidth, dstWidth);
    Params->ScaleFactorY = calc_factor(srcHeight, dstHeight);

    Params->InitialOffsetX = calc_initial_offset(srcWidth, dstWidth);
    Params->InitialOffsetY = calc_initial_offset(srcHeight, dstHeight);

    LOGI("[%s,%d] Set_Scaler_Config Done\n", __FUNCTION__, __LINE__);

}

int get_num_ppc_blocks(int input_size, int output_size, int max_window)
{
    if (input_size <= max_window)
        return 1;

    uint64_t factor = calc_factor(input_size, output_size);
    int in_overlap = MAX(2 * div_ceil_i32(input_size, output_size), PPC_OVERLAP);

    int max_n = (int)(((int64_t)(max_window - 2 * in_overlap) << FACTOR_PRECISION)
        / ((int64_t)PPC_T * (int64_t)factor));
    if (max_n < 1) max_n = 0;
    int N = 1;  /* minimum: one T-block */

    for (int n = 1; n <= max_n; n++) {
        int in_valid = div_ceil_i32(PPC_T * n * input_size / 2, output_size);
        if (2 * in_overlap + in_valid > max_window) break;
        N = n;
    }

    return div_ceil_i32(output_size, PPC_T * N);
}

/**
 * Computes per-block parameters for the SCL pipeline (1D blocking).
 *
 * factor = (input_size - 1) / (output_size - 1),  GO = 0.5*factor - 0.5.
 *
 * Block layout:
 *   Valid output is divided into blocks of step = T * N pixels (T = 32).
 *   N is the largest integer satisfying:
 *     in_valid = 2 * ceil(step * factor / 2)        (aligned to 2)
 *     2 * in_overlap + in_valid <= max_window       (SCL input window constraint)
 *
 * For each block n:
 *   valid output range  vs = n*step,  ve = min((n+1)*step-1, output_size-1)
 *   phase at boundaries P = vs*factor + GO,  Q = ve*factor + GO
 *   input window  in_start = floor(P) - in_overlap,  in_end = ceil(Q) + in_overlap
 *                  clamped to [0, input_size-1] and capped to max_window
 *   SCL output     os = vs - 2 (clamped to 0), scl_out_size = oe - os + 1
 *   initial_offset = os * factor + GO - in_start
 *   crop_start = vs - os (local offset in SCL output, typically 2)
 *   out_valid_size = ve - vs + 1 (pixel count after crop)
 *
 * Caller uses these per block:
 *   tile = src[in_start : in_start + in_size]
 *   scl_1d(tile, factor, scl_out_size, initial_offset)
 *   dst[out_start : out_start + out_valid_size] = scl_out[crop_start : crop_start + out_valid_size]
 */
void compute_ppc_blocks(
    int    input_size,
    int    output_size,
    int    max_window,
    uint64_t *factor,
    int   *blk_num,
    int   *initial_offset,
    int   *in_start,
    int   *in_size,
    int   *out_start,
    int   *scl_out_size,
    int   *out_valid_size,
    int   *crop_start)
{
    uint64_t f = calc_factor(input_size, output_size);
    int GO = calc_initial_offset(input_size, output_size);

    *factor = f;

    if (input_size <= max_window) {
        *blk_num = 1;
        in_start[0] = 0;
        in_size[0] = input_size;
        out_start[0] = 0;
        scl_out_size[0] = output_size;
        out_valid_size[0] = output_size;
        crop_start[0] = 0;
        initial_offset[0] = GO;
        return;
    }

    int in_overlap = MAX(2 * div_ceil_i32(input_size, output_size), PPC_OVERLAP);

    /* find max N satisfying input constraint */
    int max_n = (int)(((int64_t)(max_window - 2 * in_overlap) << FACTOR_PRECISION)
        / ((int64_t)PPC_T * (int64_t)f));
    if (max_n < 1) max_n = 0;
    int N = 1;

    for (int n = 1; n <= max_n; n++) {
        int in_valid = div_ceil_i32(PPC_T * n * input_size / 2, output_size);
        if (2 * in_overlap + in_valid > max_window) break;
        N = n;
    }

    int step = PPC_T * N;
    int num = div_ceil_i32(output_size, step);
    *blk_num = num;

    for (int n = 0; n < num; n++) {
        /* valid output range (before crop) */
        int vs = n * step;
        int ve = (n + 1) * step - 1;
        if (ve >= output_size) ve = output_size - 1;
        int vw = ve - vs + 1;

        /* phase in fixed-point: vs * factor + GO */
        int64_t P = (int64_t)vs * (int64_t)f + (int64_t)GO;
        int64_t Q = (int64_t)ve * (int64_t)f + (int64_t)GO;

        /* floor / ceil in fixed-point */
        int is_u = (int)(P >> FACTOR_PRECISION) - in_overlap;
        int ie_u = (int)((Q + (1LL << FACTOR_PRECISION) - 1) >> FACTOR_PRECISION) + in_overlap;
        if (ie_u > is_u + max_window - 1)
            ie_u = is_u + max_window - 1;

        /* clamp to source boundaries */
        int il = is_u > 0 ? is_u : 0;
        int ir = ie_u < input_size - 1 ? ie_u : input_size - 1;

        /* SCL output range (clamp out_overlap=2 at image edges) */
        int os = vs - 2;
        if (os < 0) os = 0;
        int oe = ve + 2;
        if (oe >= output_size) oe = output_size - 1;
        int ow = oe - os + 1;

        in_start[n] = il;
        in_size[n] = ir - il + 1;
        out_start[n] = vs;  /* valid start in dst (after crop) */
        scl_out_size[n] = ow;
        crop_start[n] = vs - os;  /* local crop offset within SCL output */
        out_valid_size[n] = vw;
        initial_offset[n] = (int)((int64_t)os * (int64_t)f + (int64_t)GO
            - ((int64_t)il << FACTOR_PRECISION));
    }
}
