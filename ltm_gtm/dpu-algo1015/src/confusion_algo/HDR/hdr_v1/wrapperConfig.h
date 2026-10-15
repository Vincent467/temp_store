#ifndef __WRAPPER_CONFIG_H
#define __WRAPPER_CONFIG_H

#if defined(_WIN32)
    #define API_EXPORT __declspec(dllexport)
#elif defined(__linux__)
    #define API_EXPORT __attribute__((visibility("default")))
#else
    #define API_EXPORT
#endif

enum _HDR_EOTF
{
    EO_Disable            = 0,
    EO_ProgConf           = 1,
    EO_PQ                 = 2,
    EO_HLG                = 3,
    EO_170M               = 4,
    EO_sRGB               = 5,
    EO_DeGamma2_2         = 6,
    EO_DeGamma2_6         = 7,
    EO_END                = 8
};

enum _GAMUTMAPPING
{
    CCM_Disable           = 0,
    CCM_ProgConf          = 1,
    CCM_Rec709toRec2020   = 2,
    CCM_Rec2020toRec709   = 3,
    CCM_Rec2020toP3       = 4,
    CCM_P3tosRGB          = 5,
    CCM_P3toRec2020       = 6,
    CCM_sRGBtoP3          = 7,
    CCM_scRGBtosRGB       = 8,
    CCM_END
};

enum _HDR_ColorSpace
{
    CS_ProgConf           = 0,
    CS_Rec601             = 1,
    CS_Rec709             = 2,
    CS_Rec2020            = 3,
    CS_P3                 = 4,
    CS_sRGB               = 5,
    CS_END                = 6
};

enum _HDR_TM_Standard
{
    TM_STD_SDR            = 0,
    TM_STD_PQ             = 1,
    TM_STD_HLG            = 2,
    TM_STD_HDR10          = 3,
    TM_STD_HDR10Plus      = 4,
    TM_STD_HDR_Vivid      = 5,
    TM_SDR_IN_HDR         = 6,
    TM_STD_End            = 7
};

enum _HDR_TM_Algo
{
    TM_Disable                 = 0,
    TM_ProgCurve               = 1,
    TM_ALGO_Linear             = 2, // only effective in mode TM_STD_PQ and TM_STD_HDR10,K=MaxDll/MaxCll
    TM_ALGO_Mobius             = 3, // aligned to ffmpeg
    TM_ALGO_Bezier             = 4, 
    TM_ALGO_ReinHard           = 5, // aligned to ffmpeg
    TM_ALGO_Hable              = 6, // aligned to ffmpeg
    TM_ALGO_End                = 7,
};

enum _HDR_OETF
{
    OE_Disable            = 0,
    OE_ProgConf           = 1,
    OE_PQ                 = 2,
    OE_HLG                = 3,
    OE_170M               = 4,
    OE_sRGB               = 5,
    OE_ReGamma2_2         = 6,
    OE_ReGamma2_6         = 7,
    OE_END                = 8
};
enum _HDR_Config
{
    NanoUltra = 0,
    HDR_Typical_cfg = 1,
    HDR_Vivid_PQtoSDR = 2,
    HDR_HLGtoSDR = 3,
    HDR_HDR10toSDR = 4,
    HDR_HDR10PlustoSDR = 5,
    HDR_SDRinHDR = 6,
	HDR_HDRFP16 = 7, //ADD FP16 CONFIG,OETF 129 ENTRY
};

typedef struct _LuminanceParameters {
    double avgRGBLuminance;// Average RGB luminance; likely a scene/frame average luminance statistic.
    double maxRLuminance;// Maximum luminance value of the R channel.
    double maxGLuminance;// Maximum luminance value of the G channel.
    double maxBLuminance;// Maximum luminance value of the B channel.
    int32_t num_percentile;// Number of valid percentile entries used in vPercentile / vPercentage.
    double vPercentile[15];// Array of luminance values corresponding to percentile points.
    int32_t vPercentage[15];// Array of percentile percentages, e.g. 1, 5, 10, 50, 95, etc.
}LuminanceParameters;

typedef struct _EBZCurveParameters {
    int32_t order; //Order of the EBZ curve.numP = MAX(order - 1, 1).
    double Sx; //EBZ curve parameter on the X axis.
    double Sy;// EBZ curve parameter on the Y axis.
    double vPCoeff[15]; // EBZ curve P coefficient array.length(vPCoeff) = order-1
}EBZCurveParameters;

typedef struct _Scene2094Metadata {
    int32_t TargetedSystemDisplayMaximumLuminance;// Maximum luminance of the targeted display system.
    EBZCurveParameters vSceneEBZParam;// EBZ curve parameters for the current scene.
    LuminanceParameters vSceneLuminanceParam;// Luminance statistics for the current scene.
    uint8_t vSceneEBZProvided;// Indicates whether EBZ curve parameters are provided for the current scene.
}Scene2094Metadata;

typedef struct _Hdr10Plus_Config {
    uint8_t autoCurce;// Enables automatic curve generation.If set to 1, EBZ curve parameters are not used.If set to 0, automatically generate curve.
    uint8_t usePsll99; //1：sceneMaxL = min(sceneSourceMaxRGBL, vPercentile[num_percentile - 1])；0：sceneMaxL = sceneSourceMaxRGBL
    int32_t ProductTarget;//productTarget:if not provided, set to "-1" which means product target = 2094-40 reference target(from json)
    Scene2094Metadata Hdr10Plus_metadata;//Hdr10Plus metadata
}Hdr10Plus_Config;

typedef struct _CuvaMetadata
{
    unsigned int system_start_code;
    unsigned int minimum_maxrgb;
    unsigned int average_maxrgb;
    unsigned int variance_maxrgb;
    unsigned int maximum_maxrgb;
    unsigned int tone_mapping_mode;
    unsigned int tone_mapping_param_num;
    unsigned int targeted_system_display_maximum_luminance[2];
    unsigned int Base_flag[4];
    unsigned int Base_param_m_p[2];
    unsigned int Base_param_m_m[2];
    unsigned int Base_param_m_a[2];
    unsigned int Base_param_m_b[2];
    unsigned int Base_param_m_n[2];
    unsigned int Base_param_K1[2];
    unsigned int Base_param_K2[2];
    unsigned int Base_param_K3[2];
    unsigned int base_param_Delta_mode[2];
    unsigned int base_param_Delta[2];
    unsigned int P3Spline_flag[2];
    unsigned int P3Spline_num[2];
    unsigned int P3Spline_TH_mode[2][4];
    unsigned int P3Spline_TH_MB[2][4];
    unsigned int P3Spline_TH[2][4][3];
    unsigned int P3Spline_Strength[2][4];
    unsigned int color_saturation_mapping_flag;
    unsigned int color_saturation_num;
    unsigned int color_saturation_gain[16];
}CuvaMetadata;//Reference T/UWA 005.1

typedef struct _Hdr_Algo_Config {
    //config
    uint8_t FP16_Enable; //hw disable
    uint8_t DeMultiply;
    uint8_t EO_Enable;
    uint8_t CCM_Enable;
    uint8_t TM_Enable;
    uint8_t OE_Enable;

    //****VERSION CONTROL PARAMS*****
    uint8_t Eotf_First;
    uint8_t TM_First;

    //bitwidths
    uint8_t Inbits;
    uint8_t Linearbits; // follow hw spec hdr pipelin linear bitwidth
    uint8_t linearclipmode; //enable or disable, only for <Linearbits> = 20
    uint8_t Outbits;
    uint8_t oetfentrymode;//0:65, 1:129
    //****VERSION CONTROL PARAMS*****

    //EOTF
    uint8_t EO_mode;
    char* EO_ConfBin;

    //CCM
    uint8_t CCM_mode;
    char* CCM_ConfBin;

    //Tone Mapping
    uint8_t R2Y_Gamut;
    uint16_t Y_LinearWeight; // only for R2Y_Mode is blend, 10bit fix point: weight * luma + (1024 - weight) * max_rgb
    uint8_t R2Y_Mode;//0:luma;1:max rgb;2:blend

    uint8_t Hdr_Standard;
    uint8_t TM_Algo;
    double Linear_Mobius;
    double Contrast_Reinhard;
    uint16_t MaxCll; //for PQ, HDR10, luma nit, source
    uint16_t MaxDll; //for PQ, HDR10, luma nit, dst, display
    uint16_t MaxLuminanceforSDRinHDR; // only valid in mode TM_SDR_IN_HDR,the 1 of SDR will be mapped to MaxLuminanceforSDRinHDR of HDR.
    double MasterDisplay;//for Vivid, if it is unexist, set 923/1023(4000nits) or 769/1023(1000nits) as default value.
    char* TM_ConfBin;
    Hdr10Plus_Config Hdr10Plus_Parms;
    CuvaMetadata Hdr_Vivid_Metadata;
    uint8_t Hdr_Vivid_Metadata_Exit;

    //OETF
    uint8_t OE_mode;
    char* OE_ConfBin;
}Hdr_Algo_Config;

typedef struct _SW_FP16_Config {
    uint8_t FP16ShiftBits;
    uint8_t FP16ShiftBiasFlag;
}SW_FP16_Config;

typedef struct _SW_EOTF_CONFIG
{
    uint32_t EotfEntry;
    uint32_t EotfData[129];
    uint32_t EotfSegmentCount;
    uint32_t EotfSegmentPoint[16];
    uint32_t EotfSegmentStep[16];
}SW_EOTF_CONFIG;

typedef struct _SW_OETF_CONFIG
{
    uint32_t OetfEntry;
    uint32_t OetfData[129];
    uint32_t OetfSegmentCount;
    uint32_t OetfSegmentPoint[16];
    uint32_t OetfSegmentStep[16];
}SW_OETF_CONFIG;

typedef struct _SW_TM_CONFIG
{
    uint32_t TMCoef[3];
    uint32_t TMWeight;
    uint32_t TMMode;
    uint32_t TMData[33];
    uint32_t TMSegmentPoint[33];
}SW_TM_CONFIG;

typedef struct _SW_CGM_CONFIG
{
    int32_t CGMCoeff[9];
    uint32_t CGMCoeffD[3];
}SW_CGM_CONFIG;

typedef struct _SW_HDR_Registers
{
    uint8_t EnableFP16;

    uint8_t EnableDeMultiply;
    uint8_t EnableEotf;
    uint8_t EnableOetf;
    uint8_t EnableTM;

    uint8_t Eotf_First;
    uint8_t TM_First;

    uint8_t EnableCGM;
    uint8_t ExtendBitsMode;
    uint8_t ExtendBitsSeed;

    SW_FP16_Config FP16U24Config; //not use
    SW_EOTF_CONFIG EotfConfig;
    SW_OETF_CONFIG OetfConfig;
    SW_TM_CONFIG TMConfig;
    SW_CGM_CONFIG CGMConfig;
}SW_HDR_Registers;

#ifdef __cplusplus
extern "C" {
#endif

    API_EXPORT void set_BZParam_values(OUT EBZCurveParameters* BZParam,
        IN const double sx, IN const double sy, IN const double * ps, IN const int order,
        IN const double sNorm, IN const double pCoeffNorm);

    API_EXPORT void set_Luminance_values(OUT LuminanceParameters* BZParam,
        IN const double avgL, IN const double maxR, IN const double maxG, IN const double maxB,
        IN const double * psll, IN const int * percent, IN const int order, IN const double lumNorm);

    API_EXPORT U32 Initial_Hdr_Algo_Config(IN uint32_t HDR_Config_mode, OUT Hdr_Algo_Config* Hdr_Config);

    API_EXPORT U32 Set_Hdr_Config(IN Hdr_Algo_Config Hdr_Config, OUT SW_HDR_Registers* Hdr_parms);

#ifdef __cplusplus
}
#endif

#endif    /* __WRAPPER_CONFIG_H */
