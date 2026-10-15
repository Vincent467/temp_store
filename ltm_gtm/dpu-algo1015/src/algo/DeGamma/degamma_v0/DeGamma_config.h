#ifndef __DEGAMMA_CONFIG_H__
#define __DEGAMMA_CONFIG_H__

enum _DeGamma_Mode
{
	DeGamma_Mode_Disable = 0, // Disable
	DeGamma_Mode_ProgConf = 1,
	DeGamma_Mode_DeG_PQ = 2,
	DeGamma_Mode_DeG_HLG = 3,
	DeGamma_Mode_DeG_170M = 4,
	DeGamma_Mode_DeG_sRGB = 5,
	DeGamma_Mode_DeG_DeGamma2_2 = 6,
    DeGamma_Mode_DeG_DeGamma2_6 = 7,
	DeGamma_Mode_END = 8
};

enum _DeGamma_SW_BitMode
{
    DeGamma_SW_BITMode_START = 0,
    DeGamma_SW_BITMode_In10_Out14 = 1,
    DeGamma_SW_BITMode_In12_Out14 = 2,
    DeGamma_SW_BITMode_In8_Out12 = 3,
    DeGamma_SW_BITMode_In8_Out10 = 4,
    DeGamma_SW_BITMode_In10_Out17 = 5,
    DeGamma_SW_BITMode_In12_Out17 = 6,
    DeGamma_SW_BITMode_END = 7
};

enum _DeGamma_SW_EntryMode
{
    DeGamma_SW_Entry_65 = 1,
    DeGamma_SW_Entry_129 = 2,
};

typedef struct
{
	U32 DeGCount;
	U32 DeGPoint[16];
	U32 DeGStep[16];
	U32 DeGData[129];

}DeGamma_SW_Config;

typedef struct
{
    uint32_t DeGamma_Mode;
    uint32_t DeGamma_BITMode;
    uint32_t DeGamma_EntryMode;
    const char *csvPath;
}DeGamma_User_Config;

#ifdef __cplusplus
extern "C" {
#endif
	
	//being called by DV
    API_EXPORT void Set_DeGamma_Config(
        IN uint32_t DeGamma_Mode, IN uint32_t DeGamma_BITMode, IN const char *csvPath, OUT U32 *DeGCount, OUT U32 *DeGPoint, OUT U32 *DeGStep, OUT U32 *DeGData);

    API_EXPORT void Set_DeGamma_Config_packed(IN DeGamma_User_Config* DeGamma_User_Config, OUT DeGamma_SW_Config* DeGamma_Config);

    //being called by driver
    API_EXPORT U32 Set_DeGConfig(
        IN U04 Switch,
        IN U04 BitMode,
        IN U04 EntryMode,
        IN const char *ConfBin,
        OUT U32 *RegCount, OUT U32 *RegPoint, OUT U32 *RegStep, OUT U32 *RegData);
#ifdef __cplusplus
}
#endif
#endif

