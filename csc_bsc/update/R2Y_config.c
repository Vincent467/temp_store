#include "sw_base.hpp"
#include "CSC_BSC.h"
#include "CSC.h"

/* Public R2Y mode values 1..4 correspond to these internal CSC modes. */
static const CSC_Mode csc_modes[] = {
    CSC_L2L, CSC_L2F, CSC_F2L, CSC_F2F
};

/* sRGB and Rec709 share the CSC chromaticities and white point. */
static const CSC_Gamut csc_gamuts[] = {
    CSC_BT601, CSC_BT709, CSC_BT2020, CSC_P3_D65, CSC_BT709
};

/* Coefficient CSV only: one record per physical line, no multiline cells. */
static int read_line(FILE *file, char line[4096], int *present)
{
    size_t length = 0;
    int ch;
    while ((ch = fgetc(file)) != EOF && ch != '\n') {
        if (ch == 0 || length >= 4095) return R2Y_ERROR_CSV_FORMAT;
        line[length++] = (char)ch;
    }
    if (ferror(file)) return R2Y_ERROR_CSV_IO;
    line[length] = '\0';
    *present = ch != EOF || length != 0;
    return CSC_SUCCESS;
}

/* Returns 1 for a field, 0 at end of record, -1 for malformed quoting. */
static int next_field(char **cursor, char **field)
{
    char *p, *start, *end, *next;
    if (*cursor == NULL) return 0;
    p = *cursor;
    while (isspace((unsigned char)*p)) ++p;
    if (*p == '"') {
        start = ++p;
        end = start;
        for (;;) {
            if (*p == '\0') return -1;
            if (*p == '"') {
                if (p[1] == '"') {
                    *end++ = '"'; p += 2;
                } else {
                    ++p;
                    break;
                }
            } else {
                *end++ = *p++;
            }
        }
        while (isspace((unsigned char)*p)) ++p;
        if (*p != ',' && *p != '\0') return -1;
        next = *p == ',' ? p + 1 : NULL;
        *end = '\0';
    } else {
        start = p;
        while (*p != ',' && *p != '\0') {
            if (*p == '"') return -1;
            ++p;
        }
        next = *p == ',' ? p + 1 : NULL;
        end = p;
        while (end > start && isspace((unsigned char)end[-1])) --end;
        *end = '\0';
    }
    *cursor = next;
    *field = start;
    return 1;
}

static int label_equal(const char *a, const char *b)
{
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        ++a; ++b;
    }
    return *a == *b;
}

static int parse_row(char *line, float matrix[3][3], int32_t pre[3],
                      int32_t post[3], unsigned int bitwidth, unsigned int *seen)
{
    char *cursor = line;
    char *field;
    unsigned int flag;
    int count, index, state;
    if (next_field(&cursor, &field) != 1) return R2Y_ERROR_CSV_FORMAT;
    if (*field == '\0') {
        while ((state = next_field(&cursor, &field)) == 1) {
            if (*field != '\0') return R2Y_ERROR_CSV_FORMAT;
        }
        return state == 0 ? CSC_SUCCESS : R2Y_ERROR_CSV_FORMAT;
    }
    if (label_equal(field, "Matrix")) { flag = 1u; count = 9; }
    else if (label_equal(field, "Preoffset")) { flag = 2u; count = 3; }
    else if (label_equal(field, "Postoffset")) { flag = 4u; count = 3; }
    else return R2Y_ERROR_CSV_FORMAT;
    if ((*seen & flag) != 0u) return R2Y_ERROR_CSV_FORMAT;

    for (index = 0; index < count; ++index) {
        char *end;
        double value;
        if (next_field(&cursor, &field) != 1 || *field == '\0')
            return R2Y_ERROR_CSV_FORMAT;
        errno = 0;
        value = strtod(field, &end);
        if (end == field) return R2Y_ERROR_CSV_FORMAT;
        while (isspace((unsigned char)*end)) ++end;
        if (*end != '\0' || errno == ERANGE || !isfinite(value))
            return R2Y_ERROR_CSV_FORMAT;
        if (flag == 1u) {
            if (fabs(value) > FLT_MAX) return R2Y_ERROR_CSV_FORMAT;
            matrix[index / 3][index % 3] = (float)value;
        } else {
            /* These are already quantized S(bitwidth+1) sample-code values. */
            if (value < CSC_OFFSET_FIXED_MIN(bitwidth) || value > CSC_OFFSET_FIXED_MAX(bitwidth)
                || trunc(value) != value) return R2Y_ERROR_CSV_FORMAT;
            if (flag == 2u) pre[index] = (int32_t)value;
            else post[index] = (int32_t)value;
        }
    }
    while ((state = next_field(&cursor, &field)) == 1) {
        if (*field != '\0') return R2Y_ERROR_CSV_FORMAT;
    }
    if (state != 0) return R2Y_ERROR_CSV_FORMAT;
    *seen |= flag;
    return CSC_SUCCESS;
}

static int read_programmable(const char *path, float matrix[3][3],
                              int32_t pre[3], int32_t post[3], unsigned int bitwidth)
{
    FILE *file;
    char line[4096];
    unsigned int seen = 0;
    int status = CSC_SUCCESS, present, first = 1;
    if (path == NULL || path[0] == '\0') return CSC_ERROR_INVALID_ARGUMENT;
    file = fopen(path, "rb");
    if (file == NULL) return R2Y_ERROR_CSV_IO;
    for (;;) {
        char *record = line;
        status = read_line(file, line, &present);
        if (status != CSC_SUCCESS || !present) break;
        if (first && strlen(line) >= 3 && (unsigned char)line[0] == 0xef
            && (unsigned char)line[1] == 0xbb && (unsigned char)line[2] == 0xbf)
            record += 3;
        first = 0;
        status = parse_row(record, matrix, pre, post, bitwidth, &seen);
        if (status != CSC_SUCCESS) break;
    }
    if (fclose(file) != 0 && status == CSC_SUCCESS) status = R2Y_ERROR_CSV_IO;
    if (status == CSC_SUCCESS && seen != 7u) status = R2Y_ERROR_CSV_FORMAT;
    return status;
}

static int32_t round_code_offset(double value, unsigned int bitwidth, int flag, int *status)
{
    value = round(value);
    if (value < CSC_OFFSET_FIXED_MIN(bitwidth)) {
        *status |= flag;
        return CSC_OFFSET_FIXED_MIN(bitwidth);
    }
    if (value > CSC_OFFSET_FIXED_MAX(bitwidth)) {
        *status |= flag;
        return CSC_OFFSET_FIXED_MAX(bitwidth);
    }
    return (int32_t)value;
}

static int programmable_pipeline(const R2Y_User_Config *user,
                                  int32_t fixed[3][3], int32_t pre[3],
                                  int32_t post[3])
{
    float matrix[3][3], fused[3][3], fp[3], fq[3];
    const float zero[3] = {0,0,0};
    int32_t csv_pre[3], csv_post[3];
    int32_t unused_pre[3], unused_post[3];
    const double sample_scale = (double)(1u << user->R2Y_Bitwidth);
    int row, col, status, neutral = 1;
    status = read_programmable(user->R2Y_Programmable, matrix, csv_pre, csv_post, user->R2Y_Bitwidth);
    if (status != CSC_SUCCESS) return status;

    /* Gain validation and S*M*C remain in the existing BSC interface.
     * Pass zero offsets here; actual CSV offsets stay in sample-code units. */
    status = CSC_BSC_Coeff((const float (*)[3])matrix, zero, zero,
                          user->kb[0], user->kb[1], user->kb[2],
                          user->kc[0], user->kc[1], user->kc[2],
                          user->ks[0], user->ks[1], 1, fused, fp, fq);
    if (status != CSC_SUCCESS) return status;
    /* Quantize ONLY the matrix. CSV offsets never enter a *2^bitwidth path. */
    status = CSC_ParamsToFixed((const float (*)[3])fused, zero, zero,
                              user->R2Y_Bitwidth, fixed, unused_pre, unused_post);
    if (status < 0) return status;
    for (col = 0; col < 3; ++col) {
        if (user->kb[col] != 0.0f || user->kc[col] != 1.0f) neutral = 0;
    }
    if (user->ks[0] != 1.0f || user->ks[1] != 1.0f) neutral = 0;
    for (row = 0; row < 3; ++row) {
        if (neutral) {
            /* Preserve the user's pre/post register values exactly. */
            pre[row] = csv_pre[row];
            post[row] = csv_post[row];
        } else {
            double value = csv_post[row];
            const double saturation = row == 0 ? 1.0 : user->ks[row - 1];
            for (col = 0; col < 3; ++col)
                value -= (double)matrix[row][col] * csv_pre[col];
            pre[row] = round_code_offset(-(double)user->kb[row] * sample_scale, user->R2Y_Bitwidth,
                                         CSC_FIXED_OVERFLOW_PREOFFSET, &status);
            post[row] = round_code_offset(saturation * value, user->R2Y_Bitwidth,
                                          CSC_FIXED_OVERFLOW_POSTOFFSET, &status);
        }
    }
    return status;
}

static void store_config(const R2Y_User_Config *user, const int32_t fixed[3][3],
                          const int32_t pre[3], const int32_t post[3],
                          R2Y_SW_Config *sw_config)
{
    R2Y_SW_Config packed = {0};
    int row, col;
    packed.RGBToYUVMode = user->BSC_enable != 0u
        ? (uint32_t)R2Y_Mode_PROGRAMMABLE : user->R2Y_Mode;
    packed.RGBToYUVGamut = user->R2Y_Gamut;
    for (row = 0; row < 3; ++row) {
        for (col = 0; col < 3; ++col)
            packed.RGBToYUVCoeff[row * 3 + col] = fixed[row][col];
        packed.RGBToYUVCoeffP[row] = pre[row];
        packed.RGBToYUVCoeffD[row] = post[row];
    }
    *sw_config = packed;
}

API_EXPORT void Set_R2Y_Config(
    IN uint32_t R2Y_Mode, IN uint32_t R2Y_Gamut, IN const char *R2Y_Programmable, IN uint32_t R2Y_Bitwidth,
    OUT R2Y_SW_Config* R2Y_Config)
{
    R2Y_User_Config defaults;
    defaults.R2Y_Mode = R2Y_Mode;
    defaults.R2Y_Gamut = R2Y_Gamut;
    defaults.R2Y_Programmable = R2Y_Programmable;
    defaults.R2Y_Bitwidth = R2Y_Bitwidth;
    int channel;
    for (channel = 0; channel < 3; ++channel) {
        defaults.kb[channel] = 0.0f;
        defaults.kc[channel] = 1.0f;
    }
    defaults.ks[0] = defaults.ks[1] = 1.0f;
    defaults.R2Y_Enable = 1;
    defaults.BSC_enable = 0;
    (void)Set_R2Y_BSC_Config(&defaults, R2Y_Config);
}

API_EXPORT void Set_R2Y_Config_packed(IN const R2Y_User_Config *user_config,
    OUT R2Y_SW_Config *R2Y_Config)
{
    R2Y_User_Config defaults;
    int channel;
    if (user_config == NULL || R2Y_Config == NULL) return;
    defaults = *user_config;
    if (defaults.BSC_enable == 0u) {
        for (channel = 0; channel < 3; ++channel) {
            defaults.kb[channel] = 0.0f;
            defaults.kc[channel] = 1.0f;
        }
        defaults.ks[0] = defaults.ks[1] = 1.0f;
    }
    (void)Set_R2Y_BSC_Config(&defaults, R2Y_Config);
}

API_EXPORT int Set_R2Y_BSC_Config(IN const R2Y_User_Config *user,
    OUT R2Y_SW_Config *sw_config)
{
    float matrix[3][3] = {{1,0,0}, {0,1,0}, {0,0,1}};
    float preoffset[3] = {0,0,0}, postoffset[3] = {0,0,0};
    float fused[3][3], fused_pre[3], fused_post[3];
    int32_t fixed[3][3], fixed_pre[3], fixed_post[3];
    R2Y_User_Config effective;
    int channel;
    int status;
    if (user == NULL || sw_config == NULL || user->R2Y_Enable > 1u
        || user->BSC_enable > 1u)
        return CSC_ERROR_INVALID_ARGUMENT;
    if (user->R2Y_Mode >= (uint32_t)R2Y_Mode_END
        || user->R2Y_Gamut >= (uint32_t)R2Y_Gamut_END
        || user->R2Y_Bitwidth < 8u || user->R2Y_Bitwidth > 16u)
        return CSC_ERROR_INVALID_ARGUMENT;

    effective = *user;
    if (effective.BSC_enable == 0u) {
        for (channel = 0; channel < 3; ++channel) {
            effective.kb[channel] = 0.0f;
            effective.kc[channel] = 1.0f;
        }
        effective.ks[0] = effective.ks[1] = 1.0f;
    }

    if (effective.R2Y_Enable != 0u && effective.R2Y_Mode == R2Y_Mode_PROGRAMMABLE) {
        status = programmable_pipeline(&effective, fixed, fixed_pre, fixed_post);
        if (status < 0) return status;
    } else {
        if (effective.R2Y_Enable != 0u) {
            status = initparams_CSC(CSC_R2Y, csc_gamuts[effective.R2Y_Gamut],
                                   csc_modes[effective.R2Y_Mode - R2Y_Mode_L2L],
                                   matrix, preoffset, postoffset);
            if (status != CSC_SUCCESS) return status;
        }
        status = CSC_BSC_Coeff((const float (*)[3])matrix, preoffset, postoffset,
                              effective.kb[0], effective.kb[1], effective.kb[2],
                              effective.kc[0], effective.kc[1], effective.kc[2],
                              effective.ks[0], effective.ks[1], (int)effective.R2Y_Enable,
                              fused, fused_pre, fused_post);
        if (status != CSC_SUCCESS) return status;
        status = CSC_ParamsToFixed((const float (*)[3])fused, fused_pre, fused_post,
                                  effective.R2Y_Bitwidth, fixed, fixed_pre, fixed_post);
        if (status < 0) return status;
    }

    store_config(&effective, (const int32_t (*)[3])fixed, fixed_pre, fixed_post, sw_config);
    return status;
}

