#ifndef __REGAMMA_CONFIG_H__
#define __REGAMMA_CONFIG_H__

enum _ReGamma_Mode
{
	ReGamma_Mode_Disable = 0, // Disable
	ReGamma_Mode_ProgConf = 1,
	ReGamma_Mode_ReG_PQ = 2,
	ReGamma_Mode_ReG_HLG = 3,
	ReGamma_Mode_ReG_170M = 4,
	ReGamma_Mode_ReG_sRGB = 5,
	ReGamma_Mode_ReG_ReGamma2_2 = 6,
    ReGamma_Mode_ReG_ReGamma2_6 = 7,
	ReGamma_Mode_END = 8
};

enum _ReGamma_SW_BitMode
{
    ReGamma_SW_Mode_START = 0,
    ReGamma_SW_Mode_BIT14 = 1,
    ReGamma_SW_Mode_BIT14_ROI = 2,
    ReGamma_SW_Mode_BIT12 = 3,
    ReGamma_SW_Mode_BIT12To8 = 4,
    ReGamma_SW_Mode_BIT10To8 = 5,
    ReGamma_SW_Mode_BIT8 = 6,
    ReGamma_SW_Mode_BIT17To12 = 7,
    ReGamma_SW_Mode_BIT17To14 = 8,
    ReGamma_SW_Mode_END
};

enum _ReGamma_SW_LutMode
{
    SW_REGAMMA_LUTMODE_3CHANNEL = 0,
    SW_REGAMMA_LUTMODE_1CHANNEL = 1,
};

enum _ReGamma_SW_EntryMode
{
    ReGamma_SW_Entry_65  = 1,
    ReGamma_SW_Entry_129 = 2,
    ReGamma_SW_Entry_257 = 3,
};

typedef struct
{
	U32 ReGCountR;
	U32 ReGPointR[16];
	U32 ReGStepR[16];
	U32 ReGDataR[257];
	U32 ReGEntryR;

	U32 ReGCountG;
	U32 ReGPointG[16];
	U32 ReGStepG[16];
	U32 ReGDataG[257];
	U32 ReGEntryG;

	U32 ReGCountB;
	U32 ReGPointB[16];
	U32 ReGStepB[16];
	U32 ReGDataB[257];
	U32 ReGEntryB;
	
}ReGamma_SW_Config;

typedef struct
{
    uint32_t ReGamma_Mode;
    uint32_t ReGamma_BitMode;
    uint32_t ReGamma_LutMode;
    uint32_t ReGamma_EntryMode;
    const char *csvPathR;
    const char *csvPathG;
    const char *csvPathB;
}ReGamma_User_Config;

#ifdef __cplusplus
extern "C" {
#endif

    API_EXPORT void Set_ReGamma_Config(IN ReGamma_User_Config* ReGamma_User_Config, OUT ReGamma_SW_Config* ReGamma_Config);

    // 14bits regamma, being called by driver&DV
    API_EXPORT void Set_ReGConfig(
        IN U04 Switch,
        IN const char *ConfBinR,
        IN const char *ConfBinG,
        IN const char *ConfBinB,
        OUT U32 *RegCountR, OUT U32 *RegPointR, OUT U32 *RegStepR, OUT U32 *RegDataR, OUT U32 *EntryR,
        OUT U32 *RegCountG, OUT U32 *RegPointG, OUT U32 *RegStepG, OUT U32 *RegDataG, OUT U32 *EntryG,
        OUT U32 *RegCountB, OUT U32 *RegPointB, OUT U32 *RegStepB, OUT U32 *RegDataB, OUT U32 *EntryB);

	// being called by driver
	API_EXPORT void Set_ReGConfig_core(
		IN U04 Switch,
		IN U32 BitMode,
		IN U32 LutMode,
		IN U32 EntryMode,
		IN const char *ConfBinR,
		IN const char *ConfBinG,
		IN const char *ConfBinB,
		OUT U32 *RegCountR, OUT U32 *RegPointR, OUT U32 *RegStepR, OUT U32 *RegDataR, OUT U32 *EntryR,
		OUT U32 *RegCountG, OUT U32 *RegPointG, OUT U32 *RegStepG, OUT U32 *RegDataG, OUT U32 *EntryG,
		OUT U32 *RegCountB, OUT U32 *RegPointB, OUT U32 *RegStepB, OUT U32 *RegDataB, OUT U32 *EntryB);

#ifdef __cplusplus
}
#endif
#endif