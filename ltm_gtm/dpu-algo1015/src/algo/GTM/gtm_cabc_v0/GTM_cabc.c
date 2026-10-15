#include "sw_base.hpp"

uint16_t gtm_last_curve[17] = { 0,64,128,192,256,320,384,448,512,576,640,704,768,832,896,960,1024 };
double gtm_last_cabc_duty = 1023;
SW_GTM_ReadOut gtm_first_readout = { 0 };

#define ARRAY_SIZE 65
#define EPSILON 1e-12

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif // !min

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif // !max




uint8_t gtm_apply_curve_u8(uint8_t input, uint8_t* x, uint8_t* y, int N)
{
    if (input < x[0])
        return y[0];
    else if (input >= x[N - 1])
        return y[N - 1];
    else
    {
        for (int i = 0; i < N - 1; ++i)
        {
            if (input >= x[i] && input < x[i + 1])
            {
                float slope = ((float)(y[i + 1]) - y[i]) / ((float)(x[i + 1]) - x[i]);
                return round((input - x[i]) * slope + y[i]);
            }
        }
    }
    printf("error input curve\n");
    exit(1);
    return 0;
}

void read_out_update(SW_GTM_ReadOut gtm_readout)
{
    gtm_first_readout = gtm_readout;//1 frame delay
}

int fit_bezier_after_kink(const double x[ARRAY_SIZE],
    const double y[ARRAY_SIZE],
    int kink_idx,
    int end_point,
    double y_new[ARRAY_SIZE])
{
    int i;

    // parameter validation
    if (kink_idx <= 0 || kink_idx >= end_point) {
        return -1;
    }


    // kink point coordinates
    double x0 = x[kink_idx];
    double y0 = y[kink_idx];

    // end point coordinates
    double x1 = x[end_point];
    double y1 = y[end_point];

    // start point is (0,0), slope of the front segment
    double k1 = y0 / x0;   // x0 > 0 since kink point is not the start

    // control point b: x at midpoint of rear segment, y on front-segment line
    double b_x = (x0 + x1) / 2.0;
    double b_y = y0 + k1 * (b_x - x0);
    if (b_y > 1.0) {
        b_y = 1.0;
    }

    // end point c
    double c_x = x1;
    double c_y = y1;

    // coefficients for quadratic: x(t) = A*t^2 + B*t + C, where t in [0,1]
    double A = x0 - 2.0 * b_x + c_x;
    double B = 2.0 * (b_x - x0);
    double C = x0;

    // copy original y to output array
    for (i = 0; i < ARRAY_SIZE; ++i) {
        y_new[i] = y[i];
    }

    // for each point after the kink, solve for t, then compute y(t)
    for (i = kink_idx + 1; i <= end_point; ++i) {
        double x_target = x[i];
        double D_val = C - x_target;

        double t;

        // solve A*t^2 + B*t + D = 0
        if (fabs(A) < EPSILON) {
            // degenerate case (nearly linear)
            if (fabs(B) > EPSILON) {
                t = -D_val / B;
            }
            else {
                t = 0.0;
            }
        }
        else {
            double delta = B * B - 4.0 * A * D_val;
            if (delta < 0.0) {
                delta = 0.0;
            }
            double sqrt_delta = sqrt(delta);
            double t1 = (-B + sqrt_delta) / (2.0 * A);
            double t2 = (-B - sqrt_delta) / (2.0 * A);
            t = (t1 >= 0.0 && t1 <= 1.0) ? t1 : t2;
        }

        // clip to [0, 1]
        if (t < 0.0) t = 0.0;
        if (t > 1.0) t = 1.0;

        // Bezier y(t)
        double one_minus_t = 1.0 - t;
        y_new[i] = one_minus_t * one_minus_t * y0
            + 2.0 * one_minus_t * t * b_y
            + t * t * c_y;
    }

    // ensure exact end point
    y_new[ARRAY_SIZE - 1] = 1.0;

    return 0;
}


API_EXPORT void get_cabc_gain_lut(OUT SW_GTM_Registers* reg_params, IN SW_GTM_ReadOut readout_params, IN GtmConfg user_config, OUT GtmCabcOut* cabcOut) {
    //reg_params:   register paramters
    //readout_params:  read out paramters
    //user_config  user config params


  
    uint16_t height = user_config.input_h;
    uint16_t width = user_config.input_w;
    uint8_t grid_height = 8;
    uint8_t grid_width = 8;

    


    //config register

    reg_params->enable_color_protect = 0;
    reg_params->enable_dither = 1;
    reg_params->luma_blending_enable = 1;
    reg_params->enable_hist = 1;
    reg_params->enable_slice_affine_grid = 1;
    reg_params->sd_blending_enable = 1;


    reg_params->grid_height = grid_height;
    reg_params->grid_width = grid_width;
    reg_params->hist_grid_depth = 64;

    reg_params->dither_table_r[0] = 0;
    reg_params->dither_table_r[1] = 2;
    reg_params->dither_table_r[2] = 1;
    reg_params->dither_table_r[3] = 3;

    reg_params->dither_table_g[0] = 1;
    reg_params->dither_table_g[1] = 0;
    reg_params->dither_table_g[2] = 3;
    reg_params->dither_table_g[3] = 2;

    reg_params->dither_table_b[0] = 3;
    reg_params->dither_table_b[1] = 2;
    reg_params->dither_table_b[2] = 1;
    reg_params->dither_table_b[3] = 0;

    reg_params->luma_coef[0] = 0;
    reg_params->luma_coef[1] = 0;
    reg_params->luma_coef[2] = 0;
    reg_params->luma_coef[3] = 64; 
    reg_params->luma_coef[4] = 0;

    

    reg_params->non_roi_mapping_way = 0;
    reg_params->roi_x_start = 0;
    reg_params->roi_x_end = width - 1;
    uint16_t roi_x_len = reg_params->roi_x_end - reg_params->roi_x_start + 1;

    reg_params->roi_y_start = 0;
    reg_params->roi_y_end = height-1;
    uint16_t roi_y_len = reg_params->roi_y_end - reg_params->roi_y_start + 1;

    reg_params->block_ratio_lr = (1LL << 32)*grid_width *grid_height / (roi_x_len*roi_y_len);

    reg_params->hist_grid_x_scale = (1 << 16) * grid_width / roi_x_len;
    reg_params->hist_grid_y_scale = (1 << 16) * grid_height / roi_y_len;


    uint16_t x_17[SW_CURVE_DEPTH] = { 0 };
    uint16_t _smooth_curve[SW_CURVE_DEPTH] = { 0 };
    double _smooth_cabc_duty = 0;

    if (user_config.fr == 0) {
        for (int i = 0; i < SW_CURVE_DEPTH; i++) {
            _smooth_curve[i] = gtm_last_curve[i];
            _smooth_cabc_duty = gtm_last_cabc_duty;
        }
        double bcaklight_reduce_coff = pow(_smooth_cabc_duty / 1023.0, 2.2);
        cabcOut->backlight_reduce_coff = bcaklight_reduce_coff;
        cabcOut->cabc_duty = _smooth_cabc_duty;
        memcpy(reg_params->affine_lut, _smooth_curve, SW_CURVE_DEPTH * sizeof(uint16_t));
        return;
    }

    //cabc curve algorithm
    uint8_t ratio_min = 128;
    uint8_t ratio_max = 120;
    uint16_t cabc_duty = 0;
    
    uint16_t cabc_duty_adjust = 0;
    uint8_t bin_num = reg_params->hist_grid_depth;

    
    uint32_t total_pixel_num = height* width;
    
    U32 histogram[64];
    uint8_t luma_avg_array[SW_MAX_GRID_NUM];
    memcpy(histogram, readout_params.Histogram, 64 * sizeof(U32));
    memcpy(luma_avg_array, readout_params.luma_avg, SW_MAX_GRID_NUM * sizeof(uint8_t));

    uint8_t blend_wgt_curve[2][3] = {
        {0,10,50},
        {0,96,108},
    };

    uint8_t change_bin_curve[2][3] = {
        {0,5,10},
        {0,20,32},
    };



    if (histogram[0] >= ratio_min * total_pixel_num / 128) {
        cabc_duty = 0;
        memcpy(reg_params->affine_lut, gtm_last_curve, SW_CURVE_DEPTH * sizeof(uint16_t));
        return;
    }

    if (histogram[bin_num - 1] >= ratio_max * total_pixel_num / 128) {
        cabc_duty = 1023;
        memcpy(reg_params->affine_lut, gtm_last_curve, SW_CURVE_DEPTH * sizeof(uint16_t));
        return;
    }

    uint8_t strength = min(max(user_config.Strength, 0),32);

    uint32_t pixel_accum = 0;
    //uint8_t change_bin = 0;
    uint16_t luma_max_all = 0;
    //uint16_t p1_max_val = 0;

    bool accum_flag = 1;
    bool max_flag = 1;

    uint8_t p1_point = 0;
    bool p1_flag = 1;

    bool duty_adjust_flag = 1;
    
    uint32_t pixel_ratio_accum = strength * total_pixel_num / 128;
    uint32_t pixel_ratio_duty_adjust = 8 * total_pixel_num / 128;

    uint16_t hist_step = 1024 / bin_num;

    for (int i = bin_num - 1; i >= 0; i--) {
        pixel_accum += histogram[i];
        if (histogram[i] != 0 && max_flag == 1) {
            max_flag = 0;
            luma_max_all = min((i + 1)*hist_step, 1023);     //max luma
        }
        if (pixel_accum > 0.0002*total_pixel_num && p1_flag == 1) {
            p1_flag = 0;
            p1_point = i;
            //p1_max_val = min((i + 1)*hist_step, 1023);  
        }

        if (pixel_accum > pixel_ratio_accum && accum_flag==1) {
            accum_flag = 0;
            float alpha = ((float)pixel_accum - pixel_ratio_accum) / histogram[i];
            float val0 = i * hist_step;
            float val1 = min((i+1) * hist_step,1023);
            cabc_duty = min(round(val0*(1 - alpha) + val1 * alpha),1023);
        }

        if (pixel_accum > pixel_ratio_duty_adjust && duty_adjust_flag == 1) {
            duty_adjust_flag = 0;
            float alpha = ((float)pixel_accum - pixel_ratio_duty_adjust) / histogram[i];
            float val0 = i * hist_step;
            float val1 = min((i + 1) * hist_step, 1023);
            cabc_duty_adjust = min(round(val0*(1 - alpha) + val1 * alpha), 1023);
        }

        //if (pixel_accum > pixel_ratio_change_point && change_flag == 1) {
        //    change_flag = 0;
        //    change_bin = i;
        //}

        
    }

    uint16_t block_avg_luma_max = 0;
    for (int i = 0; i < SW_MAX_GRID_NUM; i++) {
        block_avg_luma_max = max(block_avg_luma_max, luma_avg_array[i]);
    }

    uint8_t diff = min(max((int)block_avg_luma_max - (int)cabc_duty_adjust/4,0),255);
    uint8_t blend_wgt = gtm_apply_curve_u8(diff, blend_wgt_curve[0], blend_wgt_curve[1], 3);
    cabc_duty = max(round((luma_max_all*blend_wgt + cabc_duty * (128 - blend_wgt))/128), 1);
    
    double slope = (1023.0 / cabc_duty);

    double x_65[ARRAY_SIZE] = { 0 };
    double mapping_curve[ARRAY_SIZE] = { 0 };
    for (int i = 0; i < ARRAY_SIZE; i++) {
        x_65[i] = i * (1.0/64.0);
    }

    if (cabc_duty == luma_max_all) {              //no loss
        for (int i = 0; i < ARRAY_SIZE; i++) {
            mapping_curve[i] = min(x_65[i] * slope, 1);
        }
        
    }


    if (cabc_duty < luma_max_all) {             //has loss

        double mapping_curve_t[ARRAY_SIZE] = { 0 };
        uint32_t clip_num = 0;    //how many pixels been clip
        uint8_t clip_bin = (uint8_t)(cabc_duty * 64 / 1024);
        for (int i = clip_bin; i < bin_num; i++) {
            clip_num += histogram[i];
        }
        uint8_t clip_ratio = round(clip_num * 128.0 / total_pixel_num);

        uint8_t change_bin_num = gtm_apply_curve_u8(clip_ratio, change_bin_curve[0], change_bin_curve[1], 3);
        uint8_t change_point = max(clip_bin - change_bin_num, 10);
        change_point = min(change_point, ARRAY_SIZE-2);

        uint8_t change_point_end = round(luma_max_all*64/1024.0)+1;
        change_point_end = min(change_point_end, p1_point + 1);
        change_point_end = max(change_point_end, change_point + 4);
        change_point_end = min(change_point_end, ARRAY_SIZE-1);

        for (int i = 0; i < change_point; i++) {
            mapping_curve_t[i] = min(x_65[i] * slope, 1);
        }

        double slope2 = (1 - mapping_curve_t[change_point - 1]) / (x_65[change_point_end] - x_65[change_point - 1]);

        for (int i = change_point; i < change_point_end + 1; i++) {
            mapping_curve_t[i] = mapping_curve_t[change_point - 1] + (x_65[i] - x_65[change_point - 1])*slope2;
        }

        for (int i = change_point_end + 1; i< ARRAY_SIZE; i++) {
            mapping_curve_t[i] = 1;
        }

        fit_bezier_after_kink(x_65, mapping_curve_t, change_point - 1, change_point_end, mapping_curve);


    }

    for (int i = 1; i < ARRAY_SIZE; i++) {
        mapping_curve[i] = max(mapping_curve[i - 1], mapping_curve[i]);
        mapping_curve[i] = min(mapping_curve[i],1);
    }

    

    for (int i = 0; i < ARRAY_SIZE; i +=4) {
        x_17[(int)(i / 4)] = round(mapping_curve[i]*1024);
    }


    //iir
    if (user_config.fr > 0 && user_config.temporal_smooth == 1) {
        //U16 delta = 0;
        //float iir_max_wgt = 1.0;
        //float iir_min_wgt = 0.0;
        //for (int i = 1; i < SW_CURVE_DEPTH; i++) {
        //    U08 diff = abs(x_17[i] - gtm_last_curve[i]);
        //    delta += diff;
        //}
        //float iir_wgt = delta * (-1.0 / (16 * 120)) + 1;
        //if (iir_wgt > iir_max_wgt) iir_wgt = iir_max_wgt;
        //if (iir_wgt < iir_min_wgt) iir_wgt = iir_min_wgt;

        //for (int i = 0; i < 17; i++) {
        //    _smooth_curve[i] = min(round(iir_wgt * gtm_last_curve[i] + (1 - iir_wgt)*x_17[i]), 1024);
        //    gtm_last_curve[i] = _smooth_curve[i];
        //}
        float iir_wgt = 0.96;
        _smooth_cabc_duty = iir_wgt * gtm_last_cabc_duty + (1 - iir_wgt)*cabc_duty;
        gtm_last_cabc_duty = _smooth_cabc_duty;

        for (int i = 0; i < 17; i++) {
            _smooth_curve[i] = min(round(iir_wgt * gtm_last_curve[i] + (1 - iir_wgt)*x_17[i]), 1024);
            gtm_last_curve[i] = _smooth_curve[i];
        }
    }
    else {
        for (int i = 0; i < 17; i++) {
            _smooth_curve[i] = x_17[i];
            gtm_last_curve[i] = _smooth_curve[i];
            
        }
        _smooth_cabc_duty = cabc_duty;
        gtm_last_cabc_duty = _smooth_cabc_duty;
    }

    double bcaklight_reduce_coff = pow(_smooth_cabc_duty / 1023.0, 2.2);
    cabcOut->backlight_reduce_coff = bcaklight_reduce_coff;
    cabcOut->cabc_duty = _smooth_cabc_duty;
    memcpy(reg_params->affine_lut, _smooth_curve, SW_CURVE_DEPTH * sizeof(uint16_t));

}