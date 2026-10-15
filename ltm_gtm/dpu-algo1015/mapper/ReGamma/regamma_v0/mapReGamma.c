#include "sw_base.hpp"
#include "mapRegamma.h"
#include "vs_drm.h"

dtest_status dpu_algo_api_regamma(uint32_t mode, uint32_t in_bit, uint32_t out_bit, uint32_t size,
				  char *lut_file_r, char *lut_file_g, char *lut_file_b,
				  void *hw_config, bool enable)
{
	dtest_status ret = DTEST_SUCCESS;
	U32 i;
	struct drm_vs_gamma_lut *hw = hw_config;

	/* 0:Disable 1:ProgConf 2:Eotf_PQ 3:Eotf_HLG 4:Eotf_170M 5:Eotf_sRGB 6:Eotf_ReGamma2_2 */
	U04 ReG_Switch = mode;
	U32 BitMode;
	U32 LutMode = SW_REGAMMA_LUTMODE_3CHANNEL;
	U32 EntryMode;

	U32 ReGCountR = 0;
	U32 ReGPointR[9] = { 0 };
	U32 ReGStepR[10] = { 0 };
	U32 ReGDataR[257] = { 0 };
	U32 ReGEntryR = 0;

	U32 ReGCountG = 0;
	U32 ReGPointG[9] = { 0 };
	U32 ReGStepG[10] = { 0 };
	U32 ReGDataG[257] = { 0 };
	U32 ReGEntryG = 0;

	U32 ReGCountB = 0;
	U32 ReGPointB[9] = { 0 };
	U32 ReGStepB[10] = { 0 };
	U32 ReGDataB[257] = { 0 };
	U32 ReGEntryB = 0;

	if (enable) {
		if (PRINT)
			printf("\n[GAMMA] Enable\n");

		if (in_bit == 14 && out_bit == 14)
			BitMode = ReGamma_SW_Mode_BIT14;
		else if (in_bit == 12 && out_bit == 12)
			BitMode = ReGamma_SW_Mode_BIT12;
		else if (in_bit == 12 && out_bit == 8)
			BitMode = ReGamma_SW_Mode_BIT12To8;
		else if (in_bit == 10 && out_bit == 8)
			BitMode = ReGamma_SW_Mode_BIT10To8;
		else if (in_bit == 8 && out_bit == 8)
			BitMode = ReGamma_SW_Mode_BIT8;
		else if (in_bit == 17 && out_bit == 12)
			BitMode = ReGamma_SW_Mode_BIT17To12;
		else if (in_bit == 17 && out_bit == 14)
			BitMode = ReGamma_SW_Mode_BIT17To14;
		else {
			printf("GAMMA algo: Not support in_bit:%d, out_bit:%d \n", in_bit, out_bit);
			return DTEST_NOT_SUPPORT;
		}

		if (size == 257)
			EntryMode = ReGamma_SW_Entry_257;
		else if (size == 129)
			EntryMode = ReGamma_SW_Entry_129;
		else if (size == 65)
			EntryMode = ReGamma_SW_Entry_65;
		else {
			printf("GAMMA algo: Not support %d entry table \n", size);
			return DTEST_NOT_SUPPORT;
		}
		Set_ReGConfig_core(ReG_Switch, BitMode, LutMode, EntryMode, lut_file_r, lut_file_g,
				   lut_file_b, &ReGCountR, ReGPointR, ReGStepR, ReGDataR,
				   &ReGEntryR, &ReGCountG, ReGPointG, ReGStepG, ReGDataG,
				   &ReGEntryG, &ReGCountB, ReGPointB, ReGStepB, ReGDataB,
				   &ReGEntryB);

		if (PRINT) {
			printf("       mode: %d\n", mode);
			printf("       Count_R: %d\n", ReGCountR);
			printf("       Count_G: %d\n", ReGCountG);
			printf("       Count_B: %d\n", ReGCountB);
			printf("\n       Steps_R: ");
			for (i = 0; i < ReGCountR + 1; i++)
				printf("%d, ", ReGStepR[i]);
			printf("\n       Steps_G: ");
			for (i = 0; i < ReGCountG + 1; i++)
				printf("%d, ", ReGStepG[i]);
			printf("\n       Steps_B: ");
			for (i = 0; i < ReGCountB + 1; i++)
				printf("%d, ", ReGStepB[i]);
			printf("\n       Points_R: ");
			for (i = 0; i < ReGCountR; i++)
				printf("%d, ", ReGPointR[i]);
			printf("\n       Points_G: ");
			for (i = 0; i < ReGCountG; i++)
				printf("%d, ", ReGPointG[i]);
			printf("\n       Points_B: ");
			for (i = 0; i < ReGCountB; i++)
				printf("%d, ", ReGPointB[i]);
			printf("\n       Entry_R: %d\n", ReGEntryR);
			for (i = 0; i < ReGEntryR; i++)
				printf("%d, ", ReGDataR[i]);
			printf("\n       Entry_G: %d\n", ReGEntryG);
			for (i = 0; i < ReGEntryG; i++)
				printf("%d, ", ReGDataG[i]);
			printf("\n       Entry_B: %d\n", ReGEntryB);
			for (i = 0; i < ReGEntryB; i++) {
				printf("%d, ", ReGDataB[i]);
			}
		}

		/* output hw config */
		hw->entry_cnt.r = ReGEntryR;
		hw->seg_cnt.r = (U32)ReGCountR + 1;
		hw->entry_cnt.g = ReGEntryG;
		hw->seg_cnt.g = (U32)ReGCountG + 1;
		hw->entry_cnt.b = ReGEntryB;
		hw->seg_cnt.b = (U32)ReGCountB + 1;

		for (i = 0; i < ReGCountR; i++)
			hw->seg_point[i].r = (U32)ReGPointR[i];
		for (i = 0; i < ReGCountG; i++)
			hw->seg_point[i].g = (U32)ReGPointG[i];
		for (i = 0; i < ReGCountB; i++)
			hw->seg_point[i].b = (U32)ReGPointB[i];

		for (i = 0; i < ReGCountR + 1; i++)
			hw->seg_step[i].r = (U32)ReGStepR[i];
		for (i = 0; i < ReGCountG + 1; i++)
			hw->seg_step[i].g = (U32)ReGStepG[i];
		for (i = 0; i < ReGCountB + 1; i++)
			hw->seg_step[i].b = (U32)ReGStepB[i];
		for (i = 0; i < ReGEntryR; i++) {
			hw->data[i].r = (U32)ReGDataR[i];
		}
		for (i = 0; i < ReGEntryG; i++) {
			hw->data[i].g = (U32)ReGDataG[i];
		}
		for (i = 0; i < ReGEntryB; i++) {
			hw->data[i].b = (U32)ReGDataB[i];
		}
	} else {
		if (PRINT)
			printf("\n[GAMMA] Disable\n");
	}

	return ret;
}