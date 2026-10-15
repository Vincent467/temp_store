#include "sw_base.hpp"
#include "mapLTM.h"
#include "vs_drm.h"
#include <stdio.h>
#include <time.h>

dtest_status dpu_algo_api_ltm_sre(uint32_t fr, bool temporal_smooth,
                                  uint32_t layerWidth, uint32_t layerHeight,
                                  uint32_t gridWidth, uint32_t gridHeight,
                                  double sensitivity, double iir_max_wgt,
                                  uint32_t (*cdf_crv_last)[65],
                                  uint32_t luma_luma_intensity,
                                  void *ReadParams, void *hw_config) {
  dtest_status ret = DTEST_SUCCESS;

#if EXECUTION_TIME
  clock_t start_t, end_t;
  double duration_t;
  start_t = clock();
#endif

  struct drm_vs_ltm *ltm = hw_config;
  struct drm_vs_ltm_hw_readout *ReadParam = ReadParams;

  if (PRINT) {
    printf("SRE mode\n");
    printf("Input params:\n");
    printf("    fr:%d\n", fr);
    printf("    temporal_smooth:%d\n", temporal_smooth);
    printf("    luma_luma_intensity:%d\n", luma_luma_intensity);
    printf("    gridWidth:%d\n", gridWidth);
    printf("    gridHeight:%d\n", gridHeight);
    printf("    layerWidth:%d\n", layerWidth);
    printf("    layerHeight:%d\n", layerHeight);
    printf("    sensitivity:%f\n", sensitivity);
    printf("    iir_max_wgt:%f\n", iir_max_wgt);
  }

  ltm->color_protect_enable = 1;

  /* ltm sre wopr */
  ltm_set_sre_wopr(&(ltm->sre_wopr.blending_enable), &(ltm->sre_wopr.wopr_thr));

  if (PRINT) {
    printf("[ltm_sre_wopr] blending_enable: %d, wopr_thr: %d\n",
           ltm->sre_wopr.blending_enable, ltm->sre_wopr.wopr_thr);
  }

  /* ltm sre sd */
  ltm_set_sre_sd(&(ltm->ltm_sd.blending_enable), &(ltm->ltm_sd.r_thr),
                 &(ltm->ltm_sd.g_thr), &(ltm->ltm_sd.b_thr),
                 &(ltm->ltm_sd.delta_minmax_thr), &(ltm->ltm_sd.delta_rg_thr));

  if (PRINT) {
    printf("[ltm_sre_sd] blending_enable: %d, wopr_thr: %d\n",
           ltm->ltm_sd.blending_enable, ltm->ltm_sd.r_thr, ltm->ltm_sd.g_thr,
           ltm->ltm_sd.b_thr, ltm->ltm_sd.delta_minmax_thr,
           ltm->ltm_sd.delta_rg_thr);
  }

  /*ltm_luma*/
  ltm->ltm_luma.enable =
      ltm_set_luma_cal(&(ltm->ltm_luma.mode), ltm->ltm_luma.coef);
  if (PRINT) {
    printf("ltm_luma_coef\n");
    for (int i = 0; i < VS_LTM_LUMA_COEF_NUM; i++)
      printf("    data[%d]: %d\n", i, ltm->ltm_luma.coef[i]);
  }

  /*ltm_tone_adj*/
  ltm->tone_adj.enable =
      ltm_set_tone_adj(&(ltm->tone_adj.entry_cnt), ltm->tone_adj.data);

  if (PRINT) {
    printf("[ltm_tone_adj] enable: %d\n", ltm->tone_adj.enable);
    if (ltm->tone_adj.enable) {
      printf("tone_adj entry_cnt: %d\n", ltm->tone_adj.entry_cnt);
      for (int i = 0; i < VS_MAX_1D_LUT_ENTRY_CNT; i++)
        printf("    data[%d]: %d\n", i, ltm->tone_adj.data[i]);
    }
  }

  /*ltm_freq_decomp*/
  ltm->freq_decomp.decomp_enable = ltm_set_freq_decomp(ltm->freq_decomp.coef);
  if (PRINT) {
    printf("ltm_freq_decomp_coef\n");
    for (int i = 0; i < VS_LTM_FREQ_COEF_NUM; i++)
      printf("    data[%d]: %d\n", i, ltm->freq_decomp.coef[i]);
  }

  /*ltm_grid*/
  uint32_t height, width;
  ltm->grid_size.enable = ltm_set_grid(gridWidth, gridHeight, &height, &width);
  ltm->grid_size.height = height;
  ltm->grid_size.width = width;

  if (PRINT) {
    printf("[ltm_grid] height: %d, width: %d\n", ltm->grid_size.height,
           ltm->grid_size.width);
  }

  ltm->ltm_luma.luma_blending_enable = 1;
  ltm->sre_wopr.blending_enable = 1;
  ltm->ltm_sd.blending_enable = 1;
  ltm->ad_txt_jud_en = 0;

  /*ltm_sre_af_lut*/
  ltm_set_sre_af_lut(fr, temporal_smooth, luma_luma_intensity, gridWidth,
                     gridHeight, ReadParam->avg_sd, ReadParam->avg_luma,
                     cdf_crv_last, sensitivity, iir_max_wgt,
                     ltm->ltm_af_luts.ltm_af_lut);

  ltm->ltm_af_luts.enable = 1;
  ltm->ltm_af_luts.entry_cnt = 1088;

  if (PRINT) {
    printf("ltm_af_luts, size: %ld\n", sizeof(ltm->ltm_af_luts.ltm_af_lut));
    for (int i = 0; i < ltm->ltm_af_luts.entry_cnt; i++)
      printf("    data[%d]: %d\n", i, ltm->ltm_af_luts.ltm_af_lut[i]);
  }

  /*ltm_dither*/
  uint32_t dither_tab_r[4], dither_tab_g[4], dither_tab_b[4];

  ltm->ltm_dither.dither_enable =
      ltm_set_dither(dither_tab_r, dither_tab_g, dither_tab_b);

  ltm->ltm_dither.table_low[0] =
      (U32)dither_tab_r[1] << 16 | (U32)dither_tab_r[0];
  ltm->ltm_dither.table_low[1] =
      (U32)dither_tab_g[1] << 16 | (U32)dither_tab_g[0];
  ltm->ltm_dither.table_low[2] =
      (U32)dither_tab_b[1] << 16 | (U32)dither_tab_b[0];
  ltm->ltm_dither.table_high[0] =
      (U32)dither_tab_r[3] << 16 | (U32)dither_tab_r[2];
  ltm->ltm_dither.table_high[1] =
      (U32)dither_tab_g[3] << 16 | (U32)dither_tab_g[2];
  ltm->ltm_dither.table_high[2] =
      (U32)dither_tab_b[3] << 16 | (U32)dither_tab_b[2];

  /*ltm_hist_cd*/
  ltm->ltm_cd_set.enable =
      ltm_set_hist_cd(ltm->ltm_cd_set.coef, ltm->ltm_cd_set.thresh,
                      ltm->ltm_cd_set.slope, &(ltm->ltm_cd_set.min_wgt));

  /*ltm_ds*/
  uint32_t ds_w, ds_h;
  ltm->ltm_ds.enable = ltm_set_ds(
      layerWidth, layerHeight, &(ltm->ltm_ds.h_norm), &(ltm->ltm_ds.v_norm),
      &(ltm->ltm_ds.roi_x_start), &(ltm->ltm_ds.roi_x_end),
      &(ltm->ltm_ds.roi_y_start), &(ltm->ltm_ds.roi_y_end),
      &(ltm->ltm_ds.roi_x_len), &(ltm->ltm_ds.roi_y_len), &ds_w, &ds_h);
  ltm->ltm_ds.output.h = ds_h;
  ltm->ltm_ds.output.w = ds_w;

  if (PRINT) {
    printf("[ltm_ds] h_norm: %d, v_norm: %d, roi_x_start:%d, roi_x_end: %d, "
           "roi_y_start: %d,roi_y_end: %d,"
           "roi_x_len: %d,roi_y_len: %d,output_h: %d, output_w: %d\n",
           ltm->ltm_ds.h_norm, ltm->ltm_ds.v_norm, ltm->ltm_ds.roi_x_start,
           ltm->ltm_ds.roi_x_end, ltm->ltm_ds.roi_y_start,
           ltm->ltm_ds.roi_y_end, ltm->ltm_ds.roi_x_len, ltm->ltm_ds.roi_y_len,
           ltm->ltm_ds.output.h, ltm->ltm_ds.output.w);
  }

  /*ltm_af_slice*/
  uint32_t scale_x, scale_y, block_ratio_hr;
  uint8_t non_roi_mapping_way;
  ltm->af_slice.enable = ltm_set_af_slice(
      gridWidth, gridHeight, ltm->ltm_ds.roi_x_len, ltm->ltm_ds.roi_y_len,
      &scale_x, &scale_y, &non_roi_mapping_way, &block_ratio_hr);
  ltm->af_slice.scale[0] = scale_x;
  ltm->af_slice.scale[1] = scale_y;
  ltm->af_slice.scale_half[0] = (U32)(scale_x >> 1);
  ltm->af_slice.scale_half[1] = (U32)(scale_y >> 1);
  ltm->af_slice.non_roi_mapping_way = non_roi_mapping_way;
  ltm->af_slice.block_ratio_hr = block_ratio_hr;

  if (PRINT) {
    printf("[ltm_af_slice] scale_x: %d, scale_y: %d, non_roi_mapping_way: %d, "
           "block_ratio_hr: %d\n",
           scale_x, scale_y, non_roi_mapping_way, block_ratio_hr);
  }

  /*ltm_local_hist*/
  uint32_t hist_overlap_ratio, block_ratio_lr;
  ltm->ltm_hist_set.enable = ltm_set_local_hist(
      gridWidth, gridHeight, ds_w, ds_h, &(ltm->ltm_hist_set.overlap),
      &(ltm->ltm_hist_set.grid_depth), &scale_x, &scale_y, &hist_overlap_ratio,
      &block_ratio_lr);
  ltm->ltm_hist_set.grid_scale[0] = scale_x;
  ltm->ltm_hist_set.grid_scale[1] = scale_y;
  ltm->ltm_hist_set.hist_overlap_ratio = hist_overlap_ratio;
  ltm->ltm_hist_set.block_ratio_lr = block_ratio_lr;
  ltm->grid_size.hist_depth = ltm->ltm_hist_set.grid_depth;
  if (PRINT) {
    printf("[ltm_local_hist] grid_scale_x: %d, grid_scale_y: %d,"
           "hist_overlap_ratio: %d, block_ratio_lr: %d\n",
           scale_x, scale_y, hist_overlap_ratio,
           ltm->ltm_hist_set.block_ratio_lr);
  }

#if EXECUTION_TIME
  end_t = clock();
  duration_t = (double)(end_t - start_t) / CLOCKS_PER_SEC;
  printf("The LTM SRE soft-algo run time is: %f seconds\n", duration_t);
#endif

  return ret;
}

dtest_status dpu_algo_api_ltm_foss(
    uint32_t fr, bool temporal_smooth, float Strength, uint32_t layerWidth,
    uint32_t layerHeight, uint32_t gridWidth, uint32_t gridHeight,
    double sensitivity, double iir_max_wgt, uint32_t (*hist_buf)[64],
    uint32_t (*cdf_crv_last)[65], float foss_Strength,
    float foss_uniform_blend_strength, void *ReadParams, void *hw_config) {
  dtest_status ret = DTEST_SUCCESS;

#if EXECUTION_TIME
  clock_t start_t, end_t;
  double duration_t;
  start_t = clock();
#endif

  struct drm_vs_ltm *ltm = hw_config;
  struct drm_vs_ltm_hw_readout *ReadParam = ReadParams;

  if (PRINT) {
    printf("FOSS mode\n");
    printf("Input params:\n");
    printf("    fr:%d\n", fr);
    printf("    Strength:%f\n", Strength);
    printf("    foss_Strength:%f\n", foss_Strength);
    printf("    foss_uniform_blend_strength:%f\n", foss_uniform_blend_strength);
    printf("    gridWidth:%d\n", gridWidth);
    printf("    gridHeight:%d\n", gridHeight);
    printf("    layerWidth:%d\n", layerWidth);
    printf("    layerHeight:%d\n", layerHeight);
    printf("    sensitivity:%f\n", sensitivity);
    printf("    iir_max_wgt:%f\n", iir_max_wgt);
    printf("	temporal_smooth:%d\n", temporal_smooth);
  }

  ltm->color_protect_enable = 1;

  /*ltm_luma*/
  ltm->ltm_luma.enable =
      ltm_set_luma_cal(&(ltm->ltm_luma.mode), ltm->ltm_luma.coef);
  if (PRINT) {
    printf("ltm_luma_coef\n");
    for (int i = 0; i < VS_LTM_LUMA_COEF_NUM; i++)
      printf("    data[%d]: %d\n", i, ltm->ltm_luma.coef[i]);
  }

  /*ltm_freq_decomp*/
  ltm->freq_decomp.decomp_enable = ltm_set_freq_decomp(ltm->freq_decomp.coef);
  if (PRINT) {
    printf("ltm_freq_decomp_coef\n");
    for (int i = 0; i < VS_LTM_FREQ_COEF_NUM; i++)
      printf("    data[%d]: %d\n", i, ltm->freq_decomp.coef[i]);
  }

  /*ltm_tone_adj*/
  ltm->tone_adj.enable =
      ltm_set_tone_adj(&(ltm->tone_adj.entry_cnt), ltm->tone_adj.data);

  if (PRINT) {
    printf("[ltm_tone_adj] enable: %d\n", ltm->tone_adj.enable);
    if (ltm->tone_adj.enable) {
      printf("tone_adj entry_cnt: %d\n", ltm->tone_adj.entry_cnt);
      for (int i = 0; i < VS_MAX_1D_LUT_ENTRY_CNT; i++)
        printf("    data[%d]: %d\n", i, ltm->tone_adj.data[i]);
    }
  }

  /*ltm_grid*/
  uint32_t height, width;
  ltm->grid_size.enable = ltm_set_grid(gridWidth, gridHeight, &height, &width);
  ltm->grid_size.height = height;
  ltm->grid_size.width = width;

  if (PRINT) {
    printf("[ltm_grid] height: %d, width: %d\n", ltm->grid_size.height,
           ltm->grid_size.width);
  }

  /*ltm_dither*/
  uint32_t dither_tab_r[4], dither_tab_g[4], dither_tab_b[4];

  ltm->ltm_dither.dither_enable =
      ltm_set_dither(dither_tab_r, dither_tab_g, dither_tab_b);

  ltm->ltm_dither.table_low[0] =
      (U32)dither_tab_r[1] << 16 | (U32)dither_tab_r[0];
  ltm->ltm_dither.table_low[1] =
      (U32)dither_tab_g[1] << 16 | (U32)dither_tab_g[0];
  ltm->ltm_dither.table_low[2] =
      (U32)dither_tab_b[1] << 16 | (U32)dither_tab_b[0];
  ltm->ltm_dither.table_high[0] =
      (U32)dither_tab_r[3] << 16 | (U32)dither_tab_r[2];
  ltm->ltm_dither.table_high[1] =
      (U32)dither_tab_g[3] << 16 | (U32)dither_tab_g[2];
  ltm->ltm_dither.table_high[2] =
      (U32)dither_tab_b[3] << 16 | (U32)dither_tab_b[2];

  /*ltm_hist_cd*/
  ltm->ltm_cd_set.enable =
      ltm_set_hist_cd(ltm->ltm_cd_set.coef, ltm->ltm_cd_set.thresh,
                      ltm->ltm_cd_set.slope, &(ltm->ltm_cd_set.min_wgt));

  /*ltm_ds*/
  uint32_t ds_w, ds_h;
  ltm->ltm_ds.enable = ltm_set_ds(
      layerWidth, layerHeight, &(ltm->ltm_ds.h_norm), &(ltm->ltm_ds.v_norm),
      &(ltm->ltm_ds.roi_x_start), &(ltm->ltm_ds.roi_x_end),
      &(ltm->ltm_ds.roi_y_start), &(ltm->ltm_ds.roi_y_end),
      &(ltm->ltm_ds.roi_x_len), &(ltm->ltm_ds.roi_y_len), &ds_w, &ds_h);
  ltm->ltm_ds.output.h = ds_h;
  ltm->ltm_ds.output.w = ds_w;

  if (PRINT) {
    printf("[ltm_ds] h_norm: %d, v_norm: %d, roi_x_start:%d, roi_x_end: %d, "
           "roi_y_start: %d,roi_y_end: %d,"
           "roi_x_len: %d,roi_y_len: %d,output_h: %d, output_w: %d\n",
           ltm->ltm_ds.h_norm, ltm->ltm_ds.v_norm, ltm->ltm_ds.roi_x_start,
           ltm->ltm_ds.roi_x_end, ltm->ltm_ds.roi_y_start,
           ltm->ltm_ds.roi_y_end, ltm->ltm_ds.roi_x_len, ltm->ltm_ds.roi_y_len,
           ltm->ltm_ds.output.h, ltm->ltm_ds.output.w);
  }

  /*ltm_af_slice*/
  uint32_t scale_x, scale_y, block_ratio_hr;
  uint8_t non_roi_mapping_way;
  ltm->af_slice.enable = ltm_set_af_slice(
      gridWidth, gridHeight, ltm->ltm_ds.roi_x_len, ltm->ltm_ds.roi_y_len,
      &scale_x, &scale_y, &non_roi_mapping_way, &block_ratio_hr);
  ltm->af_slice.scale[0] = scale_x;
  ltm->af_slice.scale[1] = scale_y;
  ltm->af_slice.scale_half[0] = (U32)(scale_x >> 1);
  ltm->af_slice.scale_half[1] = (U32)(scale_y >> 1);
  ltm->af_slice.non_roi_mapping_way = non_roi_mapping_way;
  ltm->af_slice.block_ratio_hr = block_ratio_hr;

  if (PRINT) {
    printf("[ltm_af_slice] scale_x: %d, scale_y: %d, non_roi_mapping_way: %d, "
           "block_ratio_hr: %d\n",
           scale_x, scale_y, non_roi_mapping_way, block_ratio_hr);
  }

  /*ltm_local_hist*/
  uint32_t hist_overlap_ratio, block_ratio_lr;
  ltm->ltm_hist_set.enable = ltm_set_local_hist(
      gridWidth, gridHeight, ds_w, ds_h, &(ltm->ltm_hist_set.overlap),
      &(ltm->ltm_hist_set.grid_depth), &scale_x, &scale_y, &hist_overlap_ratio,
      &block_ratio_lr);
  ltm->grid_size.hist_depth = ltm->ltm_hist_set.grid_depth;
  ltm->ltm_hist_set.grid_scale[0] = scale_x;
  ltm->ltm_hist_set.grid_scale[1] = scale_y;
  ltm->ltm_hist_set.hist_overlap_ratio = hist_overlap_ratio;
  ltm->ltm_hist_set.block_ratio_lr = block_ratio_lr;
  if (PRINT) {
    printf("[ltm_local_hist] grid_scale_x: %d, grid_scale_y: %d,"
           "hist_overlap_ratio: %d, block_ratio_lr: %d\n",
           scale_x, scale_y, hist_overlap_ratio, block_ratio_lr);
  }

  /* ltm foss sd */
  ltm_set_foss_sd(&(ltm->ltm_sd.blending_enable), &(ltm->ltm_sd.r_thr),
                  &(ltm->ltm_sd.g_thr), &(ltm->ltm_sd.b_thr),
                  &(ltm->ltm_sd.delta_minmax_thr), &(ltm->ltm_sd.delta_rg_thr));

  if (PRINT) {
    printf("[ltm_foss_sd] blending_enable: %d, wopr_thr: %d\n",
           ltm->ltm_sd.blending_enable, ltm->ltm_sd.r_thr, ltm->ltm_sd.g_thr,
           ltm->ltm_sd.b_thr, ltm->ltm_sd.delta_minmax_thr,
           ltm->ltm_sd.delta_rg_thr);
  }

  ltm->ltm_luma.luma_blending_enable = 1;
  ltm->ltm_sd.blending_enable = 1;
  ltm->ad_txt_jud_en = 1;

  /*ltm_foss_af_lut*/
  foss_set_af_luts(fr, temporal_smooth, Strength, layerWidth, layerHeight,
                   gridWidth, gridHeight, hist_buf, ReadParam->avg_luma,
                   ReadParam->avg_sd, ReadParam->sum_txt, cdf_crv_last,
                   sensitivity, iir_max_wgt, foss_Strength,
                   foss_uniform_blend_strength, ltm->ltm_af_luts.ltm_af_lut);

  ltm->ltm_af_luts.enable = 1;
  ltm->ltm_af_luts.entry_cnt = 1088;

  if (PRINT) {
    printf("ltm_af_luts\n");
    for (int i = 0; i < ltm->ltm_af_luts.entry_cnt; i++)
      printf("    data[%d]: %d\n", i, ltm->ltm_af_luts.ltm_af_lut[i]);
  }

#if EXECUTION_TIME
  end_t = clock();
  duration_t = (double)(end_t - start_t) / CLOCKS_PER_SEC;
  printf("The LTM SRE soft-algo run time is: %f seconds\n", duration_t);
#endif

  return ret;
}

dtest_status dpu_algo_api_ltm(uint32_t fr, uint32_t mode, bool temporal_smooth,
                              float Strength, uint32_t layerWidth,
                              uint32_t layerHeight, uint32_t gridWidth,
                              uint32_t gridHeight, double sensitivity,
                              double iir_max_wgt, uint16_t *spatial_kernal,
                              uint32_t (*hist_buf)[64],
                              uint32_t (*cdf_crv_last)[65], void *hw_config) {
  dtest_status ret = DTEST_SUCCESS;

#if EXECUTION_TIME
  clock_t start_t, end_t;
  double duration_t;
  start_t = clock();
#endif

  struct drm_vs_ltm *ltm = hw_config;

  if (PRINT) {
    printf("LTM mode\n");
    printf("Input params:\n");
    printf("    fr:%d\n", fr);
    printf("    Strength:%f\n", Strength);
    printf("    temporal_smooth:%d\n", temporal_smooth);
    printf("    gridWidth:%d\n", gridWidth);
    printf("    gridHeight:%d\n", gridHeight);
    printf("    layerWidth:%d\n", layerWidth);
    printf("    layerHeight:%d\n", layerHeight);
    printf("    sensitivity:%f\n", sensitivity);
    printf("    iir_max_wgt:%f\n", iir_max_wgt);
  }

  ltm->color_protect_enable = 1;

  /*ltm_luma*/
  ltm->ltm_luma.enable =
      ltm_set_luma_cal(&(ltm->ltm_luma.mode), ltm->ltm_luma.coef);
  if (PRINT) {
    printf("ltm_luma_coef\n");
    for (int i = 0; i < VS_LTM_LUMA_COEF_NUM; i++)
      printf("    data[%d]: %d\n", i, ltm->ltm_luma.coef[i]);
  }

  /*ltm_freq_decomp*/
  ltm->freq_decomp.decomp_enable = ltm_set_freq_decomp(ltm->freq_decomp.coef);
  if (PRINT) {
    printf("ltm_freq_decomp_coef\n");
    for (int i = 0; i < VS_LTM_FREQ_COEF_NUM; i++)
      printf("    data[%d]: %d\n", i, ltm->freq_decomp.coef[i]);
  }

  /*ltm_tone_adj*/
  ltm->tone_adj.enable =
      ltm_set_tone_adj(&(ltm->tone_adj.entry_cnt), ltm->tone_adj.data);
  if (PRINT) {
    printf("[ltm_tone_adj] enable: %d\n", ltm->tone_adj.enable);
    if (ltm->tone_adj.enable) {
      printf("tone_adj entry_cnt: %d\n", ltm->tone_adj.entry_cnt);
      for (int i = 0; i < VS_MAX_1D_LUT_ENTRY_CNT; i++)
        printf("    data[%d]: %d\n", i, ltm->tone_adj.data[i]);
    }
  }

  /*ltm_grid*/
  uint32_t height, width;
  ltm->grid_size.enable = ltm_set_grid(gridWidth, gridHeight, &height, &width);
  ltm->grid_size.height = height;
  ltm->grid_size.width = width;

  if (PRINT) {
    printf("[ltm_grid]\n");
    printf(" height: %d, width: %d\n", ltm->grid_size.height,
           ltm->grid_size.width);
  }

  /*ltm_dither*/
  uint32_t dither_tab_r[4], dither_tab_g[4], dither_tab_b[4];

  ltm->ltm_dither.dither_enable =
      ltm_set_dither(dither_tab_r, dither_tab_g, dither_tab_b);

  ltm->ltm_dither.table_low[0] =
      (U32)dither_tab_r[1] << 16 | (U32)dither_tab_r[0];
  ltm->ltm_dither.table_low[1] =
      (U32)dither_tab_g[1] << 16 | (U32)dither_tab_g[0];
  ltm->ltm_dither.table_low[2] =
      (U32)dither_tab_b[1] << 16 | (U32)dither_tab_b[0];
  ltm->ltm_dither.table_high[0] =
      (U32)dither_tab_r[3] << 16 | (U32)dither_tab_r[2];
  ltm->ltm_dither.table_high[1] =
      (U32)dither_tab_g[3] << 16 | (U32)dither_tab_g[2];
  ltm->ltm_dither.table_high[2] =
      (U32)dither_tab_b[3] << 16 | (U32)dither_tab_b[2];

  /*ltm_hist_cd*/
  ltm->ltm_cd_set.enable =
      ltm_set_hist_cd(ltm->ltm_cd_set.coef, ltm->ltm_cd_set.thresh,
                      ltm->ltm_cd_set.slope, &(ltm->ltm_cd_set.min_wgt));

  if (PRINT) {
    printf("[ltm_hist_cd]\n");
    printf("enable: %d\n", ltm->ltm_cd_set.enable);

    for (int i = 0; i < VS_LTM_CD_COEF_NUM; i++)
      printf("coef:    data[%d]: %d\n", i, ltm->ltm_cd_set.coef[i]);
    for (int i = 0; i < VS_LTM_CD_THRESH_NUM; i++)
      printf("thresh:    data[%d]: %d\n", i, ltm->ltm_cd_set.thresh[i]);
    for (int i = 0; i < VS_LTM_CD_SLOPE_NUM; i++)
      printf("slope:    data[%d]: %d\n", i, ltm->ltm_cd_set.slope[i]);

    printf("min_wgt: %d\n", ltm->ltm_cd_set.min_wgt);
  }

  /*ltm_ds*/
  uint32_t ds_w, ds_h;
  ltm->ltm_ds.enable = ltm_set_ds(
      layerWidth, layerHeight, &(ltm->ltm_ds.h_norm), &(ltm->ltm_ds.v_norm),
      &(ltm->ltm_ds.roi_x_start), &(ltm->ltm_ds.roi_x_end),
      &(ltm->ltm_ds.roi_y_start), &(ltm->ltm_ds.roi_y_end),
      &(ltm->ltm_ds.roi_x_len), &(ltm->ltm_ds.roi_y_len), &ds_w, &ds_h);
  ltm->ltm_ds.output.h = ds_h;
  ltm->ltm_ds.output.w = ds_w;

  if (PRINT) {
    printf("[ltm_ds]\n");
    printf("h_norm: %d, v_norm: %d, roi_x_start:%d, roi_x_end: %d, "
           "roi_y_start: %d,roi_y_end: %d,"
           "roi_x_len: %d,roi_y_len: %d,output_h: %d, output_w: %d\n",
           ltm->ltm_ds.h_norm, ltm->ltm_ds.v_norm, ltm->ltm_ds.roi_x_start,
           ltm->ltm_ds.roi_x_end, ltm->ltm_ds.roi_y_start,
           ltm->ltm_ds.roi_y_end, ltm->ltm_ds.roi_x_len, ltm->ltm_ds.roi_y_len,
           ltm->ltm_ds.output.h, ltm->ltm_ds.output.w);
  }

  /*ltm_af_slice*/
  uint32_t scale_x, scale_y, block_ratio_hr;
  uint8_t non_roi_mapping_way;
  ltm->af_slice.enable = ltm_set_af_slice(
      gridWidth, gridHeight, ltm->ltm_ds.roi_x_len, ltm->ltm_ds.roi_y_len,
      &scale_x, &scale_y, &non_roi_mapping_way, &block_ratio_hr);
  ltm->af_slice.scale[0] = scale_x;
  ltm->af_slice.scale[1] = scale_y;
  ltm->af_slice.scale_half[0] = (U32)(scale_x >> 1);
  ltm->af_slice.scale_half[1] = (U32)(scale_y >> 1);
  ltm->af_slice.non_roi_mapping_way = non_roi_mapping_way;
  ltm->af_slice.block_ratio_hr = block_ratio_hr;

  if (PRINT) {
    printf("[ltm_af_slice]\n");
    printf("scale_x: %d, scale_y: %d, non_roi_mapping_way: %d, block_ratio_hr: "
           "%d\n",
           scale_x, scale_y, non_roi_mapping_way, ltm->af_slice.block_ratio_hr);
  }

  /*ltm_local_hist*/
  uint32_t hist_overlap_ratio, block_ratio_lr;
  ltm->ltm_hist_set.enable = ltm_set_local_hist(
      gridWidth, gridHeight, ds_w, ds_h, &(ltm->ltm_hist_set.overlap),
      &(ltm->ltm_hist_set.grid_depth), &scale_x, &scale_y, &hist_overlap_ratio,
      &block_ratio_lr);
  ltm->grid_size.hist_depth = ltm->ltm_hist_set.grid_depth;
  ltm->ltm_hist_set.grid_scale[0] = scale_x;
  ltm->ltm_hist_set.grid_scale[1] = scale_y;
  ltm->ltm_hist_set.hist_overlap_ratio = hist_overlap_ratio;
  ltm->ltm_hist_set.block_ratio_lr = block_ratio_lr;
  if (PRINT) {
    printf("[ltm_local_hist]\n");
    printf("overlap: %d, grid_depth: %d, grid_scale_x: %d, grid_scale_y: %d,"
           "hist_overlap_ratio: %d, block_ratio_lr: %d\n",
           ltm->ltm_hist_set.overlap, ltm->ltm_hist_set.grid_depth, scale_x,
           scale_y, hist_overlap_ratio, block_ratio_lr);
  }

  /*ltm_af_lut*/
  ltm->ltm_af_luts.enable = ltm_set_af_luts(
      fr, temporal_smooth, Strength, gridWidth, gridHeight, hist_buf,
      cdf_crv_last, sensitivity, iir_max_wgt, ltm->ltm_af_luts.ltm_af_lut);

  ltm->ltm_af_luts.entry_cnt = 1088;

  if (PRINT) {
    printf("[ltm_af_luts]\n");
    for (int i = 0; i < ltm->ltm_af_luts.entry_cnt; i++)
      printf("enable: %d,    data[%d]: %d\n", ltm->ltm_af_luts.enable, i,
             ltm->ltm_af_luts.ltm_af_lut[i]);
  }

#if EXECUTION_TIME
  end_t = clock();
  duration_t = (double)(end_t - start_t) / CLOCKS_PER_SEC;
  printf("The LTM soft-algo run time is: %f seconds\n", duration_t);
#endif

  return ret;
}
