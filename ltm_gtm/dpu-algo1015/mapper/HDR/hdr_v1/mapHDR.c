#include "sw_base.hpp"
#include "wrapperConfig.h"
#include "mapHDR.h"
#include "vs_drm.h"

#define DEBUG 0

dtest_status dpu_algo_api_hdr(void *algo_config, void *hw_config)
{
	dtest_status ret = DTEST_SUCCESS;
	struct drm_vs_hdr_algo_config *config = algo_config;
	struct drm_vs_hdr *hw = hw_config;
	int i = 0;

	Hdr_Algo_Config hdr_sw = { 0 };
	SW_HDR_Registers hdr_hw = { 0 };

	/* map input algo param */
	hdr_sw.FP16_Enable = config->fp16_enable;
	hdr_sw.DeMultiply = config->de_multiply;
	hdr_sw.EO_Enable = config->eotf_enable;
	hdr_sw.CCM_Enable = config->gamut_enable;
	hdr_sw.TM_Enable = config->tonemapping_enable;
	hdr_sw.OE_Enable = config->oetf_enable;

	hdr_sw.Inbits = config->in_bits;
	hdr_sw.Linearbits = config->linear_bits;
	hdr_sw.linearclipmode = config->linear_clip_mode;
	hdr_sw.Outbits = config->out_bits;

	hdr_sw.EO_mode = config->eotf.mode;
	hdr_sw.EO_ConfBin = config->eotf.programmable_csv;

	hdr_sw.CCM_mode = config->gamut.mode;
	hdr_sw.CCM_ConfBin = config->gamut.programmable_csv;

	hdr_sw.OE_mode = config->oetf.mode;
	hdr_sw.OE_ConfBin = config->oetf.programmable_csv;

	hdr_sw.R2Y_Gamut = config->tonemap.y2r_gamut;
	hdr_sw.Y_LinearWeight = config->tonemap.y_linear_weight;
	hdr_sw.R2Y_Mode = config->tonemap.r2y_mode;
	hdr_sw.Hdr_Standard = config->tonemap.hdr_standard;
	hdr_sw.TM_Algo = config->tonemap.tm_algo;
	hdr_sw.MaxCll = config->tonemap.max_cll;
	hdr_sw.MaxDll = config->tonemap.max_dll;
	hdr_sw.TM_ConfBin = config->tonemap.programmable_csv;
	hdr_sw.MaxLuminanceforSDRinHDR = config->tonemap.sdr_in_hdr_max_luminance;
	hdr_sw.Linear_Mobius = config->tonemap.linear_mobius;
	hdr_sw.Contrast_Reinhard =  config->tonemap.contrast_reinhard;

	/* the struct  drm_vs_hdr_algo_hdr10plus same as Hdr10Plus_Config*/
	memcpy(&hdr_sw.Hdr10Plus_Parms, &config->tonemap.hdr10plus_param, sizeof(Hdr10Plus_Config));

	/* the struct  drm_vs_hdr_algo_hdr_vivid_metadata same as CuvaMetadata*/
	hdr_sw.MasterDisplay = config->tonemap.master_display;
	hdr_sw.Hdr_Vivid_Metadata_Exit = config->tonemap.hdr_vivid_metadata_exist;
	memcpy(&hdr_sw.Hdr_Vivid_Metadata, &config->tonemap.hdr_vivid_metadata, sizeof(CuvaMetadata));

	/* run hdr sw-algo */
	Set_Hdr_Config(hdr_sw, &hdr_hw);

	/* map output algo param */
	hw->demultiply = hdr_hw.EnableDeMultiply;
	hw->eotf_enable = hdr_hw.EnableEotf;
	hw->gamut_map_enable = hdr_hw.EnableCGM;
	hw->tone_map_enable = hdr_hw.EnableTM;
	hw->oetf_enable = hdr_hw.EnableOetf;

	/* EOTF */
	if (hw->eotf_enable) {
		if (DEBUG)
			printf("\n[Eotf] Enable\n");

		if (DEBUG) {
			printf("       Count: %d\n", hdr_hw.EotfConfig.EotfSegmentCount);
			printf("       Steps: ");
			for (i = 0; i < hdr_hw.EotfConfig.EotfSegmentCount + 1; i++)
				printf("%d, ", hdr_hw.EotfConfig.EotfSegmentStep[i]);
			printf("\n       Points: ");
			for (i = 0; i < hdr_hw.EotfConfig.EotfSegmentCount; i++)
				printf("%d, ", hdr_hw.EotfConfig.EotfSegmentPoint[i + 1]);
			printf("\n       Entry: ");
			for (i = 0; i < hdr_hw.EotfConfig.EotfEntry; i++)
				printf("%d, ", hdr_hw.EotfConfig.EotfData[i]);
		}

		/* output hw config */
		hw->eotf_config.entry_cnt = hdr_hw.EotfConfig.EotfEntry;
		/* sync to driver configure the register */
		hw->eotf_config.seg_cnt = hdr_hw.EotfConfig.EotfSegmentCount + 1;
		/* the  last point of SegmentPoint just use to calculate table*/
		for (i = 0; i < hdr_hw.EotfConfig.EotfSegmentCount; i++)
			/* HDR V1 SegmentPoint start with 0, to be compatiable V0,
			 * driver will not be configured from 0.
			 */
			hw->eotf_config.seg_point[i] = hdr_hw.EotfConfig.EotfSegmentPoint[i + 1];
		for (i = 0; i < hdr_hw.EotfConfig.EotfSegmentCount + 1; i++)
			hw->eotf_config.seg_step[i] = hdr_hw.EotfConfig.EotfSegmentStep[i];
		for (i = 0; i < hdr_hw.EotfConfig.EotfEntry; i++)
			hw->eotf_config.data[i] = hdr_hw.EotfConfig.EotfData[i];

	} else {
		if (DEBUG)
			printf("\n[Eotf] Disable\n");
	}

	/* OETF */
	if (hw->oetf_enable) {
		if (DEBUG)
			printf("\n[Oetf] Enable\n");

		if (DEBUG) {
			printf("       Count: %d\n", hdr_hw.OetfConfig.OetfSegmentCount);
			printf("       Steps: ");
			for (i = 0; i < hdr_hw.OetfConfig.OetfSegmentCount + 1; i++)
				printf("%d, ", hdr_hw.OetfConfig.OetfSegmentStep[i]);
			printf("\n       Points: ");
			for (i = 0; i < hdr_hw.OetfConfig.OetfSegmentCount; i++)
				printf("%d, ", hdr_hw.OetfConfig.OetfSegmentPoint[i + 1]);
			printf("\n       Entry: ");
			for (i = 0; i < hdr_hw.OetfConfig.OetfEntry; i++)
				printf("%d, ", hdr_hw.OetfConfig.OetfData[i]);
		}

		/* output hw config */
		hw->oetf_config.entry_cnt = hdr_hw.OetfConfig.OetfEntry;
		/* sync to driver configure the register */
		hw->oetf_config.seg_cnt = hdr_hw.OetfConfig.OetfSegmentCount + 1;
		/* the  last point of SegmentPoint just use to calculate table*/
		for (i = 0; i < hdr_hw.OetfConfig.OetfSegmentCount; i++)
			/* HDR V1 SegmentPoint start with 0, to be compatiable V0,
			 * driver will not be configured from 0.
			 */
			hw->oetf_config.seg_point[i] = hdr_hw.OetfConfig.OetfSegmentPoint[i + 1];
		for (i = 0; i < hdr_hw.OetfConfig.OetfSegmentCount + 1; i++)
			hw->oetf_config.seg_step[i] = hdr_hw.OetfConfig.OetfSegmentStep[i];
		for (i = 0; i < hdr_hw.OetfConfig.OetfEntry; i++)
			hw->oetf_config.data[i] = hdr_hw.OetfConfig.OetfData[i];

	} else {
		if (DEBUG)
			printf("\n[Oetf] Disable\n");
	}

	/* CGM */
	if (hw->gamut_map_enable) {
		if (DEBUG)
			printf("\n[CGM] Enable\n");

		if (DEBUG) {
			printf("\n      Coeffs(S): ");
			for (i = 0; i < 9; i++)
				printf("%d, ", hdr_hw.CGMConfig.CGMCoeff[i]);
			printf("\n      Offsets(S): ");
			for (i = 0; i < 3; i++)
				printf("%d, ", hdr_hw.CGMConfig.CGMCoeffD[i]);
			printf("\n");
		}

		for (i = 0; i < 9; i++)
			hw->gamut_map.coef[i] = hdr_hw.CGMConfig.CGMCoeff[i];
		hw->gamut_map.offset[0] = hdr_hw.CGMConfig.CGMCoeffD[0];
		hw->gamut_map.offset[1] = hdr_hw.CGMConfig.CGMCoeffD[1];
		hw->gamut_map.offset[2] = hdr_hw.CGMConfig.CGMCoeffD[2];

	} else {
		if (DEBUG)
			printf("\n[CGM] Disable\n");
	}

	/* Tone Mapping */
	if (hw->tone_map_enable) {
		if (DEBUG)
			printf("\n[Tonemapping] Enable\n");

		if (DEBUG) {
			printf("\n      TmMode:%d ", hdr_hw.TMConfig.TMMode);
			printf("\n      TmWeight:%d ", hdr_hw.TMConfig.TMWeight);
			printf("\n      TmCoef: ");
			for (i = 0; i < 3; i++)
				printf("%d, ", hdr_hw.TMConfig.TMCoef[i]);
			printf("\n      TmPoint: ");
			for (i = 0; i < 31; i++)
				printf("%d, ", hdr_hw.TMConfig.TMSegmentPoint[i + 1]);
			printf("\n      TmData: ");
			for (i = 0; i < 33; i++)
				printf("%d, ", hdr_hw.TMConfig.TMData[i]);
			printf("\n");
		}

		/* output hw config */
		hw->tone_map.pseudo_y.coef0 = (uint16_t)hdr_hw.TMConfig.TMCoef[0];
		hw->tone_map.pseudo_y.coef1 = (uint16_t)hdr_hw.TMConfig.TMCoef[1];
		hw->tone_map.pseudo_y.coef2 = (uint16_t)hdr_hw.TMConfig.TMCoef[2];
		hw->tone_map.pseudo_y.weight = (uint16_t)hdr_hw.TMConfig.TMWeight;
		hw->tone_map.pseudo_y.y_mode = (uint16_t)hdr_hw.TMConfig.TMMode;
		hw->tone_map.lut.entry_cnt = 33;
		for (i = 0; i < 31; i++)
			/* HDR V1 SegmentPoint start with 0, to be compatiable V0,
			 * driver will not be configured from 0.
			 */
			hw->tone_map.lut.seg_point[i] = hdr_hw.TMConfig.TMSegmentPoint[i + 1];
		for (i = 0; i < 33; i++)
			hw->tone_map.lut.data[i] = hdr_hw.TMConfig.TMData[i];

	} else {
		if (DEBUG)
			printf("\n[Tonemapping] Disable\n");
	}

	return DTEST_SUCCESS;
}
