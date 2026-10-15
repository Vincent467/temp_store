#include "sw_base.hpp"
#include "mapLTM.h"
#include <stdio.h>
#include <time.h>
#include "vs_drm.h"

dtest_status dpu_algo_api_ltm(uint32_t fr, uint32_t mode, bool temporal_smooth,
			      float Strength, uint32_t layerWidth,
			      uint32_t layerHeight, uint32_t gridWidth,
			      uint32_t gridHeight, double sensitivity,
			      double iir_max_wgt, uint16_t *spatial_kernal,
			      uint32_t (*hist_buf)[64],
			      uint32_t (*cdf_crv_last)[65], void *hw_config)
{
	dtest_status ret = DTEST_SUCCESS;

#if EXECUTION_TIME
	clock_t start_t, end_t;
	double duration_t;
	start_t = clock();
#endif

	struct drm_vs_ltm *ltm = hw_config;
	uint32_t Strength_int = (uint32_t)Strength;

	if (PRINT) {
		printf("Input params:\n");
		printf("    fr:%d\n", fr);
		printf("    mode:%d\n", mode);
		printf("    temporal_smooth:%d\n", temporal_smooth);
		printf("    Strength:%d\n", Strength_int);
		printf("    layerWidth:%d\n", layerWidth);
		printf("    layerHeight:%d\n", layerHeight);
		printf("    gridWidth:%d\n", gridWidth);
		printf("    gridHeight:%d\n", gridHeight);
		printf("    sensitivity:%f\n", sensitivity);
		printf("    iir_max_wgt:%f\n", iir_max_wgt);
	}

	/*ltm_gamma*/
	ltm->ltm_gamma.enable = ltm_set_gamma(ltm->ltm_gamma.coef);
	if (PRINT) {
		printf("ltm_gamma\n");
		for (int i = 0; i < VS_LTM_XGAMMA_COEF_NUM; i++)
			printf("    data[%d]: %d\n", i, ltm->ltm_gamma.coef[i]);
	}

	/*ltm_degamma*/
	ltm->ltm_degamma.enable = ltm_set_degamma(ltm->ltm_degamma.coef);
	if (PRINT) {
		printf("ltm_degamma\n");
		for (int i = 0; i < VS_LTM_XGAMMA_COEF_NUM; i++)
			printf("    data[%d]: %d\n", i,
			       ltm->ltm_degamma.coef[i]);
	}

	/*ltm_luma*/
	ltm->ltm_luma.enable =
		ltm_set_luma_cal(&(ltm->ltm_luma.mode), ltm->ltm_luma.coef);
	if (PRINT) {
		printf("ltm_luma_coef\n");
		for (int i = 0; i < VS_LTM_LUMA_COEF_NUM; i++)
			printf("    data[%d]: %d\n", i, ltm->ltm_luma.coef[i]);
	}

	/*ltm_freq_decomp*/
	ltm->freq_decomp.decomp_enable = ltm_set_freq_decomp(
		&(ltm->freq_decomp.norm), ltm->freq_decomp.coef);
	if (PRINT) {
		printf("ltm_freq_decomp_coef\n");
		for (int i = 0; i < VS_LTM_FREQ_COEF_NUM; i++)
			printf("    data[%d]: %d\n", i,
			       ltm->freq_decomp.coef[i]);
	}

	/*ltm_luma_adj*/
	ltm->luma_adj.enable = ltm_set_guidance_map(&(ltm->luma_adj.entry_cnt),
						    ltm->luma_adj.data);
	if (PRINT) {
		printf("luma_adj entry_cnt: %d\n", ltm->luma_adj.entry_cnt);
		for (int i = 0; i < VS_MAX_1D_LUT_ENTRY_CNT - 1; i++)
			printf("    data[%d]: %d\n", i, ltm->luma_adj.data[i]);
	}

	/*ltm_grid*/
	uint32_t depth, height, width;
	ltm->grid_size.enable =
		ltm_set_grid(gridWidth, gridHeight, &depth, &height, &width);
	ltm->grid_size.depth = depth;
	ltm->grid_size.height = height;
	ltm->grid_size.width = width;

	if (PRINT) {
		printf("[ltm_grid] depth: %d, height: %d, width: %d\n",
		       ltm->grid_size.depth, ltm->grid_size.height,
		       ltm->grid_size.width);
	}

	/*ltm_af_filter*/
	uint32_t slope_bias_index = 0; /*TBD:driver set default 0*/
	if (!ltm_set_af_slpbs(fr, mode, temporal_smooth, Strength_int, layerWidth,
			      layerHeight, gridWidth, gridHeight, hist_buf,
			      cdf_crv_last, sensitivity, iir_max_wgt,
			      spatial_kernal, ltm->af_filter.slope,
			      ltm->af_filter.bias, &(ltm->af_filter.enable),
			      &(ltm->af_filter.weight), &slope_bias_index))
		ret = DTEST_FAILURE;

	if (PRINT) {
		printf("ltm_af_slope\n");
		for (int i = 0; i < VS_LTM_AFFINE_LUT_NUM; i++)
			printf("    data[%d]: %d\n", i,
			       ltm->af_filter.slope[i]);
		printf("ltm_af_bias\n");
		for (int i = 0; i < VS_LTM_AFFINE_LUT_NUM; i++)
			printf("    data[%d]: %d\n", i, ltm->af_filter.bias[i]);
	}

	/*ltm_af_slice*/
	uint32_t scale_x, scale_y, pos_x, pos_y;
	ltm->af_slice.enable =
		ltm_set_af_slice(gridWidth, gridHeight, layerWidth, layerHeight,
				 &scale_x, &scale_y, &pos_x, &pos_y);
	ltm->af_slice.scale[0] = scale_x;
	ltm->af_slice.scale[1] = scale_y;
	ltm->af_slice.scale_half[0] = (U32)(scale_x >> 1);
	ltm->af_slice.scale_half[1] = (U32)(scale_y >> 1);
	ltm->af_slice.start_pos[0] = pos_x;
	ltm->af_slice.start_pos[1] = pos_y;
	if (PRINT) {
		printf("[ltm_af_slice] scale_x: %d, scale_y: %d, pos_x: %d, pos_y: %d\n",
		       scale_x, scale_y, pos_x, pos_y);
	}

	/*ltm_af_trans*/
	ltm->af_trans.enable = ltm_set_af_trans(&(ltm->af_trans.slope_bit),
						&(ltm->af_trans.bias_bit),
						ltm->af_trans.scale);
	if (PRINT) {
		printf("[ltm_af_trans] slope_bit: %d, bias_bit: %d\n",
		       ltm->af_trans.slope_bit, ltm->af_trans.bias_bit);
		for (int i = 0; i < VS_LTM_AFFINE_OUT_SCALE_SIZE; i++)
			printf("    scale[%d]: %d\n", i,
			       ltm->af_trans.scale[i]);
	}

	/*ltm_tone_adj*/
	ltm->tone_adj.enable = ltm_set_tone_adj(&(ltm->tone_adj.luma_from),
						&(ltm->tone_adj.entry_cnt),
						ltm->tone_adj.data);
	if (PRINT) {
		printf("tone_adj entry_cnt: %d\n", ltm->tone_adj.entry_cnt);
		for (int i = 0; i < VS_MAX_1D_LUT_ENTRY_CNT; i++)
			printf("    data[%d]: %d\n", i, ltm->tone_adj.data[i]);
	}

	/*ltm_color*/
	ltm->ltm_color.enable = ltm_set_color(
		&(ltm->ltm_color.luma_thresh), &(ltm->ltm_color.satu_ctrl),
		ltm->ltm_color.gain, ltm->ltm_color.luma, ltm->ltm_color.satu);

	/*ltm_dither*/
	uint32_t dither_tab_r[4], dither_tab_g[4], dither_tab_b[4];
	ltm->ltm_dither.dither_enable =
		ltm_set_dither(dither_tab_r, dither_tab_g, dither_tab_b);

	ltm->ltm_dither.table_low[0] = (U32)dither_tab_r[1] << 16 |
				       (U32)dither_tab_r[0];
	ltm->ltm_dither.table_low[1] = (U32)dither_tab_g[1] << 16 |
				       (U32)dither_tab_g[0];
	ltm->ltm_dither.table_low[2] = (U32)dither_tab_b[1] << 16 |
				       (U32)dither_tab_b[0];
	ltm->ltm_dither.table_high[0] = (U32)dither_tab_r[3] << 16 |
					(U32)dither_tab_r[2];
	ltm->ltm_dither.table_high[1] = (U32)dither_tab_g[3] << 16 |
					(U32)dither_tab_g[2];
	ltm->ltm_dither.table_high[2] = (U32)dither_tab_b[3] << 16 |
					(U32)dither_tab_b[2];

	/*ltm_luma_ave*/
	ltm->ltm_luma_set.enable = ltm_set_luma_avg(
		layerWidth, layerHeight, &(ltm->ltm_luma_set.margin_x),
		&(ltm->ltm_luma_set.margin_y), &(ltm->ltm_luma_set.pixel_norm));
	if (PRINT) {
		printf("[ltm_luma_ave] margin_x: %d, margin_y: %d, pixel_norm: %d\n",
		       ltm->ltm_luma_set.margin_x, ltm->ltm_luma_set.margin_y,
		       ltm->ltm_luma_set.pixel_norm);
	}
	/*ltm_hist_cd*/
	ltm->ltm_cd_set.enable = ltm_set_hist_cd(
		&(ltm->ltm_cd_set.overlap), ltm->ltm_cd_set.coef,
		ltm->ltm_cd_set.thresh, ltm->ltm_cd_set.slope,
		&(ltm->ltm_cd_set.filt_norm), &(ltm->ltm_cd_set.min_wgt));

	/*ltm_local_hist*/
	ltm->ltm_hist_set.enable = ltm_set_local_hist(
		layerWidth, layerHeight, gridWidth, gridHeight,
		&(ltm->ltm_hist_set.overlap), &(ltm->ltm_hist_set.grid_depth),
		&scale_x, &scale_y, &pos_x, &pos_y);
	ltm->ltm_hist_set.overlap = 1;
	ltm->ltm_hist_set.start_pos[0] = pos_x;
	ltm->ltm_hist_set.start_pos[1] = pos_y;
	ltm->ltm_hist_set.grid_scale[0] = scale_x;
	ltm->ltm_hist_set.grid_scale[1] = scale_y;
	if (PRINT) {
		printf("[ltm_local_hist] grid_scale_x: %d, grid_scale_y: %d\n",
		       scale_x, scale_y);
	}

	/*ltm_ds*/
	uint32_t ds_w, ds_h;
	ltm->ltm_ds.enable =
		ltm_set_ds(layerWidth, layerHeight, &(ltm->ltm_ds.h_norm),
			   &(ltm->ltm_ds.v_norm), &(ltm->ltm_ds.crop_l),
			   &(ltm->ltm_ds.crop_r), &(ltm->ltm_ds.crop_t),
			   &(ltm->ltm_ds.crop_b), &ds_w, &ds_h);
	ltm->ltm_ds.output.h = ds_h;
	ltm->ltm_ds.output.w = ds_w;

	if (PRINT) {
		printf("[ltm_ds] h_norm: %d, v_norm: %d, crop_l:%d, crop_r: %d, crop_t: %d,crop_b: %d, output_h: %d, output_w: %d\n",
		       ltm->ltm_ds.h_norm, ltm->ltm_ds.v_norm,
		       ltm->ltm_ds.crop_l, ltm->ltm_ds.crop_r,
		       ltm->ltm_ds.crop_t, ltm->ltm_ds.crop_b,
		       ltm->ltm_ds.output.h, ltm->ltm_ds.output.w);
	}

#if EXECUTION_TIME
	end_t = clock();
	duration_t = (double)(end_t - start_t) / CLOCKS_PER_SEC;
	printf("The LTM soft-algo run time is: %f seconds\n", duration_t);
#endif

	return ret;
}