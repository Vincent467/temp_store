#ifndef R2Y_CONFIG_H
#define R2Y_CONFIG_H

/* Public enums from the original R2Y_Config interface. */
enum _R2Y_Mode
{
    R2Y_Mode_PROGRAMMABLE = 0,
    R2Y_Mode_L2L = 1,
    R2Y_Mode_L2F = 2,
    R2Y_Mode_F2L = 3,
    R2Y_Mode_F2F = 4,
    R2Y_Mode_END = 5
};

enum R2Y_Gamut
{
    R2Y_Gamut_Rec601 = 0,
    R2Y_Gamut_Rec709 = 1,
    R2Y_Gamut_Rec2020 = 2,
    R2Y_Gamut_P3 = 3,
    R2Y_Gamut_sRGB = 4,
    R2Y_Gamut_END = 5
};

/* Keep the SW output fields, types and order unchanged. This is NOT a
 * packed-bitfield structure: coefficient entries contain signed integers. */
typedef struct
{
    /* 12-bit registers are compatible with 10-bit sample data. */
    uint32_t RGBToYUVMode;
    uint32_t RGBToYUVGamut;
    int32_t RGBToYUVCoeff[9];   /* S7.N, N=R2Y_Bitwidth, row-major */
    int32_t RGBToYUVCoeffP[3];  /* S(N+1), preoffset to SUBTRACT */
    int32_t RGBToYUVCoeffD[3];  /* S(N+1), postoffset to ADD */
} R2Y_SW_Config;

typedef struct
{
    uint32_t R2Y_Mode;         /* enum _R2Y_Mode, excluding END */
    uint32_t R2Y_Gamut;        /* enum R2Y_Gamut, excluding END */
    const char *R2Y_Programmable;
    uint32_t R2Y_Bitwidth;     /* Sample bit depth, 8..16; normally 10 or 12 */
    float kb[3];              /* R/G/B brightness, normalized [-0.5, 0.5] */
    float kc[3];              /* R/G/B contrast [0, 2] */
    float ks[2];              /* U/V gains [0.5, 1.25]; Y gain stays 1 */
    uint32_t R2Y_Enable;      /* 1: RGB->YUV; 0: RGB->RGB BSC */
} R2Y_User_Config;

enum {
    R2Y_ERROR_CSV_IO = -4,
    R2Y_ERROR_CSV_FORMAT = -5
};

#ifdef __cplusplus
extern "C" {
#endif

/* R2Y-only entry: copy User Config and set kb={0,0,0}, kc={1,1,1},
 * ks={1,1}, then call Set_R2Y_BSC_Config. Uses the supplied R2Y_Enable,
 * mode, gamut, CSV path and bitwidth. Input is read-only.
 * On error output is untouched; saturated results are committed.
 */

    API_EXPORT void Set_R2Y_Config(
        IN uint32_t R2Y_Mode, IN uint32_t R2Y_Gamut, IN const char *R2Y_Programmable, IN uint32_t R2Y_Bitwidth,
        OUT R2Y_SW_Config* R2Y_Config);

    API_EXPORT void Set_R2Y_Config_packed(IN const R2Y_User_Config *user_config,
        OUT R2Y_SW_Config *R2Y_Config);

/*
 * Full configuration: initparams_CSC -> CSC_BSC_Coeff -> CSC_ParamsToFixed.
 * The User/SW structures retain the fields shown in the existing interface;
 * BSC controls and enable are INPUT STRUCT FIELDS, not function arguments.
 *
 * kb[3] = R/G/B brightness, normalized, each in [-0.5, 0.5].
 * kc[3] = R/G/B contrast, each in [0, 2].
 * ks[2] = U/V gains, each in [0.5, 1.25]; Y gain is fixed at 1.
 * Neutral controls: kb={0,0,0}, kc={1,1,1}, ks={1,1}.
 * Initialize the new input fields explicitly; zero ks is not a neutral gain.
 * Both pointers must be non-null; input/output storage must not overlap.
 *
 * R2Y_Enable=1: generate/read R2Y CSC, then fuse BSC.
 * R2Y_Enable=0: use an identity/zero base and fuse RGB->RGB BSC with the
 *              internal BT709 F2F basis. BSC is still active.
 * R2Y_Mode/Gamut must be valid in either branch and are copied unchanged to
 * RGBToYUVMode/Gamut. They do not override the internal disabled-branch basis.
 * Public mode/gamut values are explicitly mapped to internal CSC enums.
 * R2Y_Gamut_sRGB uses the BT709 CSC primaries and white point.
 * R2Y_Mode_PROGRAMMABLE with R2Y_Enable=1 reads R2Y_Programmable as a CSV path:
 *   Matrix,m00,m01,m02,m10,m11,m12,m20,m21,m22
 *   Preoffset,p0,p1,p2
 *   Postoffset,q0,q1,q2
 * Matrix entries are float coefficients, scaled by 2^R2Y_Bitwidth once. Offsets are
 * ALREADY integer sample-code values for R2Y_Bitwidth: do NOT scale them again.
 * Neutral BSC preserves the CSV offsets exactly. With nonneutral BSC, offsets
 * are fused directly in sample-code units and rounded to integers, not scaled
 * a second time. CSV label matching is case-insensitive; UTF-8 BOM, CRLF,
 * quoted fields and trailing empty columns are supported. Each line <=4095
 * bytes; exactly the three named nonempty rows, in any order, are required.
 * Offsets must be integral S(R2Y_Bitwidth+1) values; missing/duplicate/invalid fields fail.
 * Other modes ignore R2Y_Programmable. Disabled R2Y never opens the CSV and
 * always uses the identity/zero base with the internal BT709 BSC basis.
 *
 * R2Y_Bitwidth controls matrix fractional bits and sample-code units.
 * Matrix and GENERATED normalized offsets use 2^R2Y_Bitwidth: 1024 for
 * 10-bit, 4096 for 12-bit. CSV offsets already have the sample-code unit.
 * CoeffP/D contain signed S(R2Y_Bitwidth+1) values in int32_t fields.
 * Apply: out = round(sum(Coeff * (in - CoeffP)) / 2^R2Y_Bitwidth) + CoeffD.
 * Register packing, clipping, hardware bypass/enable writes are caller-owned.
 *
 * Return 0: success; >0: CSC_FIXED_OVERFLOW_* mask, saturated output committed;
 * <0: invalid input, CSV I/O/format or numeric error, output unchanged.
 */

    API_EXPORT int Set_R2Y_BSC_Config(IN const R2Y_User_Config *user_config,
        OUT R2Y_SW_Config *sw_config);

#ifdef __cplusplus
}
#endif

#endif /* R2Y_CONFIG_H */
