#ifndef __WRAPPER_TM_H
#define __WRAPPER_TM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

    void init_x(double* x, uint32_t len);

    //input:[0-1], output:[0-1]
    void tm_curve(const Hdr_Algo_Config* Hdr_Config, double* x, double* y, float* normalizerNit, uint32_t len);

    void normalization_xy(float normalizerNit, double* x, double* y, uint32_t len);
    void normalization_y(float normalizerNit, double* y, uint32_t len);
    void tm_gain(double* x, double* y, double* gain, uint32_t len);

    void curve_to_lut(double* x, double* gain, uint32_t len_curve, int* pts,
        uint32_t* lut_point, uint32_t* lut_data, uint32_t outbits, uint32_t len_lut);

#ifdef __cplusplus
}
#endif

#endif /* __WRAPPER_CURVE_HUNT_H */
