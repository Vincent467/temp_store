#include "sw_base.hpp"
#include "mapGTM.h"
#include <stdio.h>
#include <time.h>
#include "vs_drm.h"

dtest_status dpu_algo_api_gtm_cabc(uint32_t fr, bool temporal_smooth,
				  uint32_t Strength, uint32_t layerWidth,
				  uint32_t layerHeight, void *ReadParams,
				  uint32_t *histogram, void *cabcOut,
				  void *hw_config)
{
	dtest_status ret = DTEST_SUCCESS;
	struct drm_vs_gtm *gtm = hw_config;
	struct drm_vs_ltm_hw_readout *ReadParam = ReadParams;
	
	SW_GTM_ReadOut sw_GtmReadparams = { 0 };
	SW_GTM_Registers sw_reg_params = { 0 };

	GtmConfg gtm_Config;
	GtmCabcOut cabc_Out;

	memcpy(sw_GtmReadparams.luma_avg, ReadParam->avg_luma, sizeof(sw_GtmReadparams.luma_avg));
	memcpy(sw_GtmReadparams.Histogram, histogram, sizeof(sw_GtmReadparams.Histogram));
	sw_GtmReadparams.sd_sum = ReadParam->sd_sum;
	sw_GtmReadparams.wopr_sum = ReadParam->wopr_sum;
	gtm_Config.fr = fr;
	gtm_Config.input_w = layerWidth;
	gtm_Config.input_h = layerHeight;
	gtm_Config.Strength = Strength;
	gtm_Config.temporal_smooth = temporal_smooth;

	get_cabc_gain_lut(&sw_reg_params, sw_GtmReadparams, gtm_Config, &cabc_Out);

	return ret;

}
				  
