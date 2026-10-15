#ifndef __GTM_H
#define __GTM_H


#define SW_MAX_W_GRID     8
#define SW_MAX_H_GRID     8
#define SW_CURVE_DEPTH     17
#define SW_MAX_GRID_NUM     (SW_MAX_W_GRID*SW_MAX_H_GRID)

typedef struct
{

    int Strength;
    uint32_t fr;          //current frame index
    uint32_t temporal_smooth;    //vid_mod:   0:single frame    1:mutiple frame
    uint32_t input_w;     //image width
    uint32_t input_h;    //image height
}GtmConfg;

typedef struct
{

    double cabc_duty;
    double backlight_reduce_coff;

}GtmCabcOut;

typedef struct 
{


    uint16_t luma_coef[5];


    uint8_t grid_width;

    uint8_t grid_height;

    uint16_t hist_grid_x_scale;

    uint16_t hist_grid_y_scale;


    uint16_t affine_lut[SW_CURVE_DEPTH];


    bool enable_dither;

    uint32_t dither_table_b[4];
    uint32_t dither_table_g[4];
    uint32_t dither_table_r[4];


    uint16_t roi_x_start;

    uint16_t roi_y_start;

    uint16_t roi_x_end;

    uint16_t roi_y_end;



    bool enable_hist;


    uint8_t hist_grid_depth;

    uint8_t non_roi_mapping_way;

    bool enable_slice_affine_grid;
    bool enable_color_protect;

    bool wopr_blending_enable;
    uint8_t wopr_thr;

    bool sd_blending_enable;
    uint8_t sd_r_thr;
    uint8_t sd_g_thr;
    uint8_t sd_b_thr;
    uint8_t sd_delta_minmax_thr;
    uint8_t sd_delta_rg_thr;

    bool luma_blending_enable;

    uint32_t block_ratio_lr;
}SW_GTM_Registers;


typedef struct 
{
    U32 Histogram[64];

    U25 wopr_sum; //u25
    U25 sd_sum; //u8*64
    U08 luma_avg[SW_MAX_GRID_NUM]; //u8*64
}SW_GTM_ReadOut;


#ifdef __cplusplus
extern "C" {
#endif

    API_EXPORT void get_cabc_gain_lut(OUT SW_GTM_Registers* reg_params, IN SW_GTM_ReadOut readout_params, IN GtmConfg user_config, OUT GtmCabcOut* cabcOut);



#ifdef __cplusplus
}
#endif

#endif