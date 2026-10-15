#ifndef __3DLUT_CONFIG_H__
#define __3DLUT_CONFIG_H__

#define INDEX_NUM_9_9_9 729
#define INDEX_NUM_17_17_17 4913

enum _LUT3D_Mode
{
    LUT3D_Mode_START = 0,
    LUT3D_Mode_9_9_9 = 1,
    LUT3D_Mode_17_17_17 = 2,
    LUT3D_Mode_END = 4
};

enum _LUT3D_Gamut_Mode
{
    LUT3D_Gamut_Mode_START =                0,
    LUT3D_Gamut_Mode_SRGBtoP3_CCM =         1,
    LUT3D_Gamut_Mode_SRGBtoP3_IMPROVE =     2,
    LUT3D_Gamut_Mode_SRGBtoP3_PERCEPTUAL =  3,
    LUT3D_Gamut_Mode_P3toSRGB_CCM =         4,
    LUT3D_Gamut_Mode_P3toSRGB_IMPROVE =     5,
    LUT3D_Gamut_Mode_P3toSRGB_PERCEPTUAL =  6,
    LUT3D_Gamut_Mode_Programmable = 7,
    LUT3D_Gamut_Mode_END   = 12,
};

typedef struct
{
	uint32_t LUT3DMode;
	int16_t Lut3D_17_17_17[17*17*17][3];
	int16_t Lut3D_9_9_9[9*9*9][3];
}LUT3D_SW_Config;

typedef struct
{
    uint32_t LUT3D_Mode;
    uint32_t LUT3D_Gamut_Mode;
    uint32_t bitwidth;
    const char *LUT3D_Programmable;
}LUT3D_User_Config;


#ifdef __cplusplus
extern "C" {
#endif

    API_EXPORT void Set_LUT3D_Config(
        IN uint32_t LUT3D_Mode,
        IN uint32_t Gamut_Mode,
        IN uint32_t bitwidth,
        IN const char *LUT3D_Programmable,
        OUT LUT3D_SW_Config* LUT3D_Config);

    API_EXPORT void Set_LUT3D_Config_packed(IN LUT3D_User_Config* LUT3D_User_Config, OUT LUT3D_SW_Config* LUT3D_Config);
#ifdef __cplusplus
}
#endif
#endif