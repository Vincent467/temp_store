#include "sw_base.hpp"
#include "mapGTM.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "vs_drm.h"

dtest_status dpu_algo_api_gtm_cabc(uint32_t fr, bool temporal_smooth,
				  uint32_t Strength, uint32_t layerWidth,
				  uint32_t layerHeight, void *ReadParams,
				  void *cabcOut, void *hw_config)
{
	int i;
	dtest_status ret = DTEST_SUCCESS;
	struct drm_vs_gtm *gtm = hw_config;
	struct drm_vs_gtm_cabc_out *cabc = cabcOut;
	struct drm_vs_ltm_hw_readout *ReadParam = ReadParams;

	SW_GTM_ReadOut sw_GtmReadparams = { 0 };
	SW_GTM_Registers sw_reg_params = { 0 };

	GtmConfg gtm_Config;
	GtmCabcOut cabc_Out;

	/* the readout of last frame, the histogram is in the readout blob */
	memcpy(sw_GtmReadparams.luma_avg, ReadParam->avg_luma, sizeof(sw_GtmReadparams.luma_avg));
	memcpy(sw_GtmReadparams.Histogram, ReadParam->histogram,
	       sizeof(sw_GtmReadparams.Histogram));
	sw_GtmReadparams.sd_sum = ReadParam->sd_sum;
	sw_GtmReadparams.wopr_sum = ReadParam->wopr_sum;

	/* the user config of current frame */
	gtm_Config.fr = fr;
	gtm_Config.input_w = layerWidth;
	gtm_Config.input_h = layerHeight;
	gtm_Config.Strength = Strength;
	gtm_Config.temporal_smooth = temporal_smooth;

	get_cabc_gain_lut(&sw_reg_params, sw_GtmReadparams, gtm_Config, &cabc_Out);

	/* map SW_GTM_Registers to drm_vs_gtm, the reg config reuse ltm v2 */
	/* luma coef[0..2] -> RGBY coef, coef[3] -> lightness weight, coef[4] -> gray weight */
	gtm->gtm_luma.enable = true;
	gtm->gtm_luma.mode = VS_LTM_LUMA_MIXED;
	gtm->gtm_luma.luma_blending_enable = sw_reg_params.luma_blending_enable;
	for (i = 0; i < 5; i++)
		gtm->gtm_luma.coef[i] = sw_reg_params.luma_coef[i];

	/* grid width/height and hist grid depth */
	gtm->grid_size.enable = true;
	gtm->grid_size.width = sw_reg_params.grid_width;
	gtm->grid_size.height = sw_reg_params.grid_height;
	gtm->grid_size.hist_depth = sw_reg_params.hist_grid_depth;

	/* hist enable, hist grid scale and block ratio lr */
	gtm->gtm_hist_set.enable = sw_reg_params.enable_hist;
	gtm->gtm_hist_set.grid_scale[0] = sw_reg_params.hist_grid_x_scale;
	gtm->gtm_hist_set.grid_scale[1] = sw_reg_params.hist_grid_y_scale;
	gtm->gtm_hist_set.block_ratio_lr = sw_reg_params.block_ratio_lr;

	/* affine lut, 17 entries */
	gtm->gtm_af_luts.enable = true;
	gtm->gtm_af_luts.entry_cnt = SW_CURVE_DEPTH;
	for (i = 0; i < SW_CURVE_DEPTH; i++)
		gtm->gtm_af_luts.ltm_af_lut[i] = sw_reg_params.affine_lut[i];

	/* dither table: [X00, X01, X10, X11] -> low = X00 | X01 << 16, high = X10 | X11 << 16 */
	gtm->gtm_dither.dither_enable = sw_reg_params.enable_dither;
	gtm->gtm_dither.table_low[0] = sw_reg_params.dither_table_r[0] |
				       (sw_reg_params.dither_table_r[1] << 16);
	gtm->gtm_dither.table_high[0] = sw_reg_params.dither_table_r[2] |
					(sw_reg_params.dither_table_r[3] << 16);
	gtm->gtm_dither.table_low[1] = sw_reg_params.dither_table_g[0] |
				       (sw_reg_params.dither_table_g[1] << 16);
	gtm->gtm_dither.table_high[1] = sw_reg_params.dither_table_g[2] |
					(sw_reg_params.dither_table_g[3] << 16);
	gtm->gtm_dither.table_low[2] = sw_reg_params.dither_table_b[0] |
				       (sw_reg_params.dither_table_b[1] << 16);
	gtm->gtm_dither.table_high[2] = sw_reg_params.dither_table_b[2] |
					(sw_reg_params.dither_table_b[3] << 16);

	/* roi, cabc not use the down scaler so gtm_ds.enable keep 0 */
	gtm->gtm_ds.roi_x_start = sw_reg_params.roi_x_start;
	gtm->gtm_ds.roi_y_start = sw_reg_params.roi_y_start;
	gtm->gtm_ds.roi_x_end = sw_reg_params.roi_x_end;
	gtm->gtm_ds.roi_y_end = sw_reg_params.roi_y_end;

	/* slice affine grid and non roi mapping way */
	gtm->af_slice.enable = sw_reg_params.enable_slice_affine_grid;
	gtm->af_slice.non_roi_mapping_way = sw_reg_params.non_roi_mapping_way;

	/* color protect */
	gtm->color_protect_enable = sw_reg_params.enable_color_protect;

	/* wopr blending and threshold */
	gtm->sre_wopr.blending_enable = sw_reg_params.wopr_blending_enable;
	gtm->sre_wopr.wopr_thr = sw_reg_params.wopr_thr;

	/* sd blending and thresholds */
	gtm->gtm_sd.blending_enable = sw_reg_params.sd_blending_enable;
	gtm->gtm_sd.r_thr = sw_reg_params.sd_r_thr;
	gtm->gtm_sd.g_thr = sw_reg_params.sd_g_thr;
	gtm->gtm_sd.b_thr = sw_reg_params.sd_b_thr;
	gtm->gtm_sd.delta_minmax_thr = sw_reg_params.sd_delta_minmax_thr;
	gtm->gtm_sd.delta_rg_thr = sw_reg_params.sd_delta_rg_thr;

	/* the cabc output for backlight control */
	cabc->cabc_duty = cabc_Out.cabc_duty;
	cabc->backlight_reduce_coff = cabc_Out.backlight_reduce_coff;

	return ret;
}
