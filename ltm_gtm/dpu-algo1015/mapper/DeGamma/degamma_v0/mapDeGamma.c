#include "sw_base.hpp"
#include "mapDegamma.h"
#include "vs_drm.h"

dtest_status dpu_algo_api_degamma(uint32_t mode, uint32_t in_bit, uint32_t out_bit, uint32_t size,
				  char *lut_file, void *hw_config, bool enable)
{
	dtest_status ret = DTEST_SUCCESS;
	U32 i;
	struct drm_vs_xstep_lut *hw = hw_config;

	/* 0:Disable 1:ProgConf 2:DeG_PQ 3:DeG_HLG 4:DeG_170M 5:DeG_sRGB 6:DeG_DeGamma2_2 */
	U04 DeG_Switch = mode;
	U04 BitMode;
	U04 EntryMode;
	U32 DeGCount = 0;
	U32 DeGPoint[9] = { 0 };
	U32 DeGStep[10] = { 0 };
	U32 DeGData[129] = { 0 };
	U32 DeGEntry = 0;

	if (enable) {
		if (PRINT)
			printf("\n[DEGAMMA] Enable\n");
		if (mode == 1)
			printf("lut_file: %s\n", lut_file);

		if (in_bit == 10 && out_bit == 14)
			BitMode = DeGamma_SW_BITMode_In10_Out14;
		else if (in_bit == 12 && out_bit == 14)
			BitMode = DeGamma_SW_BITMode_In12_Out14;
		else if (in_bit == 8 && out_bit == 12)
			BitMode = DeGamma_SW_BITMode_In8_Out12;
		else if (in_bit == 8 && out_bit == 10)
			BitMode = DeGamma_SW_BITMode_In8_Out10;
		else if (in_bit == 10 && out_bit == 17)
			BitMode = DeGamma_SW_BITMode_In10_Out17;
		else if (in_bit == 12 && out_bit == 17)
			BitMode = DeGamma_SW_BITMode_In12_Out17;
		else {
			printf("DEGAMMA algo: Not support in_bit:%d, out_bit:%d \n", in_bit,
			       out_bit);
			return DTEST_NOT_SUPPORT;
		}

		if (size == 129)
			EntryMode = DeGamma_SW_Entry_129;
		else if (size == 65)
			EntryMode = DeGamma_SW_Entry_65;
		else {
			printf("DEGAMMA algo: Not support %d entry table \n", size);
			return DTEST_NOT_SUPPORT;
		}

		DeGEntry = Set_DeGConfig(DeG_Switch, BitMode, EntryMode, lut_file, &DeGCount,
					 DeGPoint, DeGStep, DeGData);

		if (PRINT) {
			printf("       mode: %d\n", mode);
			printf("       Count: %d\n", DeGCount);
			printf("       Steps: ");
			for (i = 0; i < DeGCount + 1; i++)
				printf("%d, ", DeGStep[i]);
			printf("\n       Points: ");
			for (i = 0; i < DeGCount; i++)
				printf("%d, ", DeGPoint[i]);
			printf("\n       Entry: %d\n", DeGEntry);
			for (i = 0; i < DeGEntry; i++)
				printf("%d, ", DeGData[i]);
		}

		/* output hw config */
		hw->entry_cnt = DeGEntry;
		/* sync to driver configure the register */
		hw->seg_cnt = (U32)DeGCount + 1;
		/* the  last point of SegmentPoint just use to calculate table*/
		for (i = 0; i < DeGCount; i++)
			hw->seg_point[i] = (U32)DeGPoint[i];
		for (i = 0; i < DeGCount + 1; i++)
			hw->seg_step[i] = (U32)DeGStep[i];
		for (i = 0; i < DeGEntry; i++)
			hw->data[i] = (U32)DeGData[i];
	} else {
		if (PRINT)
			printf("\n[DEGAMMA] Disable\n");
	}

	return ret;
}