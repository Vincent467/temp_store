#include "sw_base.hpp"
#include "wrapperConfig.h"
#include "mapHDR.h"
#include "vs_drm.h"

static dtest_status hdr_eotf(struct drm_vs_hdr_algo_programmable *config,
			     struct drm_vs_xstep_lut *hw, bool enable, U32 inBit, U32 outBit)
{
	dtest_status ret = DTEST_SUCCESS;
	U04 Eotf_Switch =
		config->mode; /* 0:Disable 1:ProgConf 2:Eotf_PQ 3:Eotf_HLG 4:Eotf_170M 5:Eotf_sRGB 6:Eotf_DeGamma2_2 */
	U32 EotfCount = 0;
	U32 EotfPoint[9] = { 0 };
	U32 EotfStep[10] = { 0 };
	U32 EotfData[129] = { 0 };
	U32 Entry = 0;

	if (enable) {
		if (PRINT)
			printf("\n[Eotf] Enable\n");

		if (outBit != 24) {
			printf("%s: The HDR eotf outbit %d not matched 24bit! \n", __func__,
			       outBit);
			return DTEST_NOT_SUPPORT;
		}

		if (inBit != 12 && inBit != 10) {
			printf("%s: The HDR eotf inbit %d not matched 10 or 12bit! \n", __func__,
			       inBit);
			return DTEST_NOT_SUPPORT;
		}

		Entry = Set_Eotfto24Config(Eotf_Switch, config->programmable_csv, inBit, &EotfCount,
					   EotfPoint, EotfStep, EotfData);

		if (PRINT) {
			printf("       Count: %d\n", EotfCount);
			printf("       Steps: ");
			for (U32 i = 0; i < EotfCount + 1; i++)
				printf("%d, ", EotfStep[i]);
			printf("\n       Points: ");
			for (U32 i = 0; i < EotfCount; i++)
				printf("%d, ", EotfPoint[i]);
			printf("\n       Entry: ");
			for (U32 i = 0; i < Entry; i++)
				printf("%d, ", EotfData[i]);
		}

		/* output hw config */
		hw->entry_cnt = Entry;
		hw->seg_cnt = (U32)EotfCount + 1; /* sync to driver configure the register */
		/* the  last point of SegmentPoint just use to calculate table*/
		for (U06 i = 0; i < EotfCount; i++)
			hw->seg_point[i] = (U32)EotfPoint[i];
		for (U06 i = 0; i < EotfCount + 1; i++)
			hw->seg_step[i] = (U32)EotfStep[i];
		for (U06 i = 0; i < hw->entry_cnt; i++)
			hw->data[i] = (U32)EotfData[i];

	} else {
		if (PRINT)
			printf("\n[Eotf] Disable\n");
	}

	return ret;
}

static dtest_status hdr_gamut(struct drm_vs_hdr_algo_programmable *config, struct drm_vs_ccm *hw,
			      bool enable)
{
	dtest_status ret = DTEST_SUCCESS;
	U04 CCM_Switch =
		config->mode; /* 0: Disable; 1: ProgConf; 2: Rec.709-->Rec.2020; 3: Rec.2020-->Rec.709; 4: Rec.2020-->DisplayP3; 5: DisplayP3-->sRGB; 6: DisplayP3-->Rec.2020; 7: sRGB-->DisplayP3. */
	S32 CcmCoef[9] = { 0 };
	U32 CcmOffset[3] = { 0 };

	if (enable) {
		if (PRINT)
			printf("\n[CCM] Enable\n");
		Set_CcmConfig(CCM_Switch, config->programmable_csv, CcmCoef, CcmOffset);

		if (PRINT) {
			printf("\n      Coeffs(S): ");
			for (U32 i = 0; i < 9; i++)
				printf("%d, ", CcmCoef[i]);
			printf("\n      Offsets(S): ");
			for (U32 i = 0; i < 3; i++)
				printf("%d, ", CcmOffset[i]);
			printf("\n");
		}

		for (U32 i = 0; i < 9; i++)
			hw->coef[i] = CcmCoef[i];
		hw->offset[0] = CcmOffset[0];
		hw->offset[1] = CcmOffset[1];
		hw->offset[2] = CcmOffset[2];

	} else {
		if (PRINT)
			printf("\n[CCM] Disable\n");
	}
	return ret;
}

static dtest_status hdr_tonemapping(struct drm_vs_hdr_algo_tone_map *config,
				    struct drm_vs_tone_map *hw, unsigned char CCM_Switch,
				    bool enable)
{
	dtest_status ret = DTEST_SUCCESS;
	/* TM_Switch: 0:TM_Disable 1:TM_HDR10 2:TM_HLG 3:TM_ProgConf */
	/* Y2R_GAMUT: 0:CG_Rec601 1:CG_Rec709 2:CG_Rec2020 3:CG_P3 4:CG_sRGB. */
	U04 TM_Switch = config->mode;
	U04 Y2R_GAMUT = config->y2r_gamut;
	U16 MaxCLL = config->max_cll;
	U16 MaxDLL = config->max_dll;
	double Ks = config->ks;
	double Kf = config->kf;
	double BezierP[17] = { 0 };
	int LenP = config->len_p;
	for (U08 i = 0; i < LenP; i++) {
		BezierP[i] = config->bezier_p[i];
	}

	U32 TmMode = 0;
	U32 TmWeight = 0;
	U32 TmCoef[3] = { 0 };
	U32 TmPoint[31] = { 0 };
	U32 TmData[33] = { 0 };

	if (enable) {
		if (PRINT)
			printf("\n[Tonemapping] Enable\n");
		Set_TmConfig(TM_Switch, CCM_Switch, Y2R_GAMUT, MaxCLL, MaxDLL, Ks, Kf, BezierP,
			     LenP, config->programmable_csv, &TmMode, &TmWeight, TmCoef, TmPoint,
			     TmData);

		if (PRINT) {
			printf("\n      TmMode:%d ", TmMode);
			printf("\n      TmWeight:%d ", TmWeight);
			printf("\n      TmCoef: ");
			for (U32 i = 0; i < 3; i++)
				printf("%d, ", TmCoef[i]);
			printf("\n      TmPoint: ");
			for (U32 i = 0; i < 31; i++)
				printf("%d, ", TmPoint[i]);
			printf("\n      TmData: ");
			for (U32 i = 0; i < 33; i++)
				printf("%d, ", TmData[i]);
			printf("\n");
		}

		/* output hw config */
		hw->pseudo_y.coef0 = (U16)TmCoef[0];
		hw->pseudo_y.coef1 = (U16)TmCoef[1];
		hw->pseudo_y.coef2 = (U16)TmCoef[2];
		hw->pseudo_y.weight = (U16)TmWeight;
		hw->pseudo_y.y_mode = (U16)TmMode;
		hw->lut.entry_cnt = TM_DATA_MUN;
		for (U32 i = 0; i < TM_DATA_MUN - 2; i++)
			hw->lut.seg_point[i] = (U32)TmPoint[i];
		for (U32 i = 0; i < TM_DATA_MUN; i++)
			hw->lut.data[i] = (U32)TmData[i];

	} else {
		if (PRINT)
			printf("\n[Tonemapping] Disable\n");
	}

	return ret;
}

static dtest_status hdr_oetf(struct drm_vs_hdr_algo_programmable *config,
			     struct drm_vs_xstep_lut *hw, bool enable, U32 bitwidth)
{
	dtest_status ret = DTEST_SUCCESS;
	U04 Oetf_Switch =
		config->mode; /* 0:Disable 1:ProgConf 2:Oetf_PQ 3:Oetf_HLG 4:Oetf_170M 5:Oetf_sRGB 6:Oetf_DeGamma2_2 */

	U32 OetfCount = 0;
	U32 OetfPoint[9] = {
		0
	}; /* regCount in Set_OetfConfig is limited by the array length,if the array length changed, please change the corresponding limit*/
	U32 OetfStep[10] = { 0 };
	U32 OetfData[129] = { 0 };
	U32 Entry = 0;

	if (enable) {
		if (PRINT)
			printf("\n[Oetf] Enable\n");

		if (bitwidth != 24) {
			printf("%s: The HDR oetf bitwidth %d not matched 24bit! \n", __func__,
			       bitwidth);
			return DTEST_NOT_SUPPORT;
		}

		Entry = Set_OetfConfig(Oetf_Switch, config->programmable_csv, &OetfCount, OetfPoint,
				       OetfStep, OetfData);

		if (PRINT) {
			printf("       Count: %d\n", OetfCount);
			printf("       Steps: ");
			for (U32 i = 0;
			     i < OetfCount + 1 && OetfCount <= sizeof(OetfStep) / sizeof(U32); i++)
				printf("%d, ", OetfStep[i]);
			printf("\n       Points: ");
			for (U32 i = 0;
			     i < OetfCount && OetfCount <= sizeof(OetfPoint) / sizeof(U32) + 1; i++)
				printf("%d, ", OetfPoint[i]);
			printf("\n       Entry: ");
			for (U32 i = 0; i < Entry; i++)
				printf("%d, ", OetfData[i]);
		}

		/* output hw config */
		hw->entry_cnt = Entry;
		hw->seg_cnt = (U32)OetfCount + 1; /* sync to driver configure the register */
		/* the  last point of SegmentPoint just use to calculate table*/
		for (U06 i = 0; i < OetfCount && OetfCount <= 9; i++)
			hw->seg_point[i] = (U32)OetfPoint[i];
		for (U06 i = 0; i < OetfCount + 1 && OetfCount <= 9; i++)
			hw->seg_step[i] = (U32)OetfStep[i];
		for (U06 i = 0; i < hw->entry_cnt; i++)
			hw->data[i] = (U32)OetfData[i];

	} else {
		if (PRINT)
			printf("\n[Oetf] Disable\n");
	}

	return ret;
}

dtest_status dpu_algo_api_hdr(void *algo_config, void *hw_config)
{
	dtest_status ret = DTEST_SUCCESS;
	struct drm_vs_hdr_algo_config *config = algo_config;
	struct drm_vs_hdr *hdr = hw_config;
	U32 eotf_inBit, eotf_outBit, oetf_bit;

	eotf_inBit = config->in_bits;
	eotf_outBit = config->linear_bits;
	oetf_bit = config->out_bits;

	/*EOTF*/
	if (hdr_eotf(&config->eotf, &hdr->eotf_config, config->eotf_enable, eotf_inBit,
		     eotf_outBit) != DTEST_SUCCESS)
		ret = DTEST_FAILURE;

	/*GAMUT*/
	if (hdr_gamut(&config->gamut, &hdr->gamut_map, config->gamut_enable) != DTEST_SUCCESS)
		ret = DTEST_FAILURE;

	/*TNOE MAPPING*/
	if (hdr_tonemapping(&config->tonemap, &hdr->tone_map, config->gamut.mode,
			    config->tonemapping_enable) != DTEST_SUCCESS)
		ret = DTEST_FAILURE;

	/*OETF*/
	if (hdr_oetf(&config->oetf, &hdr->oetf_config, config->oetf_enable, oetf_bit) !=
	    DTEST_SUCCESS)
		ret = DTEST_FAILURE;

	return ret;
}
