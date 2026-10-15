#ifndef __CURVE_FITTING_H__
#define __CURVE_FITTING_H__

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct
    {
        double* x;
        double* y;
        double* gain;
        int len_y;
        int* len_p;
        int* pts_x;
        int* pts_step;
        int* len_entry;
        int fit_mode;
        int fit_dichotomy;
    }fit_params;

    void curve_fitting(fit_params* params);

    void linear_interpolate(double* y, int len, int len_out, double* y_out);

    void np_interp(double* fx, double* fy, int f_len, double* x, double* in_y, int x_len);

#ifdef __cplusplus
}
#endif

#endif /* __WRAPPER_CURVE_HUNT_H */


