#include "sw_base.hpp"
#include "mapGTM.h"
#include <stdio.h>
#include <time.h>
#include "vs_drm.h"

dtest_status dpu_algo_api_gtm(uint32_t fr, uint32_t mode, uint32_t vidMode,
		     uint32_t Strength, uint32_t layerWidth,
		     uint32_t layerHeight, void *hw_config)
{
	dtest_status ret = DTEST_SUCCESS;
	struct drm_vs_gtm *gtm = hw_config;

	/*gtm_gamma*/
	gtm->gtm_gamma.enable = ltm_set_gamma(gtm->gtm_gamma.coef);
	if (PRINT) {
		printf("gtm_gamma\n");
		for (int i = 0; i < VS_LTM_XGAMMA_COEF_NUM; i++)
			printf("    data[%d]: %d\n", i, gtm->gtm_gamma.coef[i]);
	}
	/*gtm_degamma*/
	gtm->gtm_degamma.enable = ltm_set_degamma(gtm->gtm_degamma.coef);
	if (PRINT) {
		printf("gtm_degamma\n");
		for (int i = 0; i < VS_LTM_XGAMMA_COEF_NUM; i++)
			printf("    data[%d]: %d\n", i,
			       gtm->gtm_degamma.coef[i]);
	}

	/*gtm_luma*/
	gtm->gtm_luma.enable =
		ltm_set_luma_cal(&(gtm->gtm_luma.mode), gtm->gtm_luma.coef);
	if (PRINT) {
		printf("gtm_luma_coef\n");
		for (int i = 0; i < VS_LTM_LUMA_COEF_NUM; i++)
			printf("    data[%d]: %d\n", i, gtm->gtm_luma.coef[i]);
	}

	/*gtm_tone_adj*/
	gtm->tone_adj.enable = ltm_set_tone_adj(&(gtm->tone_adj.luma_from),
						&(gtm->tone_adj.entry_cnt),
						gtm->tone_adj.data);
	if (PRINT) {
		printf("tone_adj entry_cnt: %d\n", gtm->tone_adj.entry_cnt);
		for (int i = 0; i < VS_MAX_1D_LUT_ENTRY_CNT; i++)
			printf("    data[%d]: %d\n", i, gtm->tone_adj.data[i]);
	}

	/*gtm_color*/
	gtm->gtm_color.enable = ltm_set_color(
		&(gtm->gtm_color.luma_thresh), &(gtm->gtm_color.satu_ctrl),
		gtm->gtm_color.gain, gtm->gtm_color.luma, gtm->gtm_color.satu);

	/*gtm_dither*/
	uint32_t dither_tab_r[4], dither_tab_g[4], dither_tab_b[4];
	gtm->gtm_dither.dither_enable = ltm_set_dither(dither_tab_r, dither_tab_g, dither_tab_b);

	gtm->gtm_dither.table_low[0] = (U32)dither_tab_r[1] << 16 |
				       (U32)dither_tab_r[0];
	gtm->gtm_dither.table_low[1] = (U32)dither_tab_g[1] << 16 |
				       (U32)dither_tab_g[0];
	gtm->gtm_dither.table_low[2] = (U32)dither_tab_b[1] << 16 |
				       (U32)dither_tab_b[0];
	gtm->gtm_dither.table_high[0] = (U32)dither_tab_r[3] << 16 |
					(U32)dither_tab_r[2];
	gtm->gtm_dither.table_high[1] = (U32)dither_tab_g[3] << 16 |
					(U32)dither_tab_g[2];
	gtm->gtm_dither.table_high[2] = (U32)dither_tab_b[3] << 16 |
					(U32)dither_tab_b[2];

	/*gtm_luma_ave*/
	gtm->gtm_luma_set.enable = ltm_set_luma_avg(
		layerWidth, layerHeight, &(gtm->gtm_luma_set.margin_x),
		&(gtm->gtm_luma_set.margin_y), &(gtm->gtm_luma_set.pixel_norm));
	if (PRINT) {
		printf("[gtm_luma_ave] margin_x: %d, margin_y: %d, pixel_norm: %d\n",
		       gtm->gtm_luma_set.margin_x, gtm->gtm_luma_set.margin_y,
		       gtm->gtm_luma_set.pixel_norm);
	}

	/*gtm_ds*/
	uint32_t ds_w, ds_h;
	gtm->gtm_ds.enable =
		ltm_set_ds(layerWidth, layerHeight, &(gtm->gtm_ds.h_norm),
			   &(gtm->gtm_ds.v_norm), &(gtm->gtm_ds.crop_l),
			   &(gtm->gtm_ds.crop_r), &(gtm->gtm_ds.crop_t),
			   &(gtm->gtm_ds.crop_b), &ds_w, &ds_h);
	gtm->gtm_ds.output.w = ds_w;
	gtm->gtm_ds.output.h = ds_h;

	if (PRINT) {
		printf("[gtm_ds] h_norm: %d, v_norm: %d, crop_l:%d, crop_r: %d, crop_t: %d,crop_b: %d, output_h: %d, output_w: %d\n",
		       gtm->gtm_ds.h_norm, gtm->gtm_ds.v_norm,
		       gtm->gtm_ds.crop_l, gtm->gtm_ds.crop_r,
		       gtm->gtm_ds.crop_t, gtm->gtm_ds.crop_b,
		       gtm->gtm_ds.output.h, gtm->gtm_ds.output.w);
	}

	return ret;
}
