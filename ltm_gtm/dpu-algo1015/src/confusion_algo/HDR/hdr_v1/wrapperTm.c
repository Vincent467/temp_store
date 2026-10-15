#include "sw_base.hpp"

#include "wrapperTm.h"
#include "bezier.h"
#include "mobius.h"
#include "reinhard.h"
#include "hable.h"
#include "hdr_def.h"
#include "hdr10plus.h"

void init_x(double* x, uint32_t len)
{
    double temp = 1.0 / (len - 1);
    for (int i = 0; i < len; ++i)
    {
        x[i] = i * temp;
    }
    return;
}


void HLG_OOTF(
    IN U16 MaxDLL, IN double *x, 
    OUT double *y, uint32_t len)
{
    assert(0 <= MaxDLL && MaxDLL <= HDR_MAX_LUMINANCE);
    assert(NULL != y);

    int i = 0;

    double r = 0;
    if (400 <= MaxDLL && MaxDLL <= 2000)
        r = 1.2 + 0.42 * log10((double)MaxDLL / 1000);
    else
        r = 1.2 * pow(1.111, log2((double)MaxDLL / 1000));

    /*    printf(" r:%f\n", r);  */

    for (i = 1; i < len; i++)
    {
        y[i] = pow(x[i], r - 1) * x[i];
    }
    y[0] = y[1];
}

void HLG_NPL_Scaler(
    IN double *y, uint32_t len, 
    IN U16 MaxDLL)
{
    int i = 0;
    double scale = 1000.0 / (double)MaxDLL;
    for (i = 0; i < len; i++)
    {
        y[i] = y[i] * scale;
    }
}
void tm_curve(const Hdr_Algo_Config* Hdr_Config, double* x, double* y, float* normalizerNit, uint32_t len)
{
    uint8_t tm_algo = Hdr_Config->TM_Algo;
    uint8_t tm_std = Hdr_Config->Hdr_Standard;

    assert(TM_STD_PQ <= tm_std && tm_std < TM_STD_End);

    double sceneMaxL = Hdr_Config->MaxCll;
    uint16_t productPeak = Hdr_Config->MaxDll;

    //HDR10Plus
    if (tm_std == TM_STD_HDR10Plus)
    {
        const Hdr10Plus_Config* Hdr10Plus_Parms = &(Hdr_Config->Hdr10Plus_Parms);

        //LuminanceParam
        const LuminanceParameters* LuminanceParam = &(Hdr10Plus_Parms->Hdr10Plus_metadata.vSceneLuminanceParam);

        //sceneMaxL
        const double sceneSourceMaxRGBL = MIN(MAX(LuminanceParam->maxRLuminance, MAX(LuminanceParam->maxGLuminance, LuminanceParam->maxBLuminance)), HDR_MAX_LUMINANCE);
        if (Hdr10Plus_Parms->usePsll99)
        {
            const int lastIndex = MAX(LuminanceParam->num_percentile - 1, 0);
            sceneMaxL = MIN(sceneSourceMaxRGBL, LuminanceParam->vPercentile[lastIndex]);
        }
        else
            sceneMaxL = sceneSourceMaxRGBL;

        //productPeak
        if (Hdr10Plus_Parms->ProductTarget == -1)
            productPeak = Hdr10Plus_Parms->Hdr10Plus_metadata.TargetedSystemDisplayMaximumLuminance;
        else
            productPeak = Hdr10Plus_Parms->ProductTarget;

        //EBZParam
        EBZCurveParameters productCurveParam;

        /* If no reference curve from ST 2094-40 is provided or it is forced to be ignored */
        if (Hdr10Plus_Parms->autoCurce || Hdr10Plus_Parms->Hdr10Plus_metadata.vSceneEBZProvided == 0)
        {
            /* Basis OOTF : Direct calculation of product TM curve from ST-2094 percentile metadata */
            BasisOOTF(LuminanceParam, productPeak, sceneMaxL, &productCurveParam);
        }
            /* If ST 2094-40 scene reference curve is to be used as a guide for this product peak */
        else {
            const float refencePeak = Hdr10Plus_Parms->Hdr10Plus_metadata.TargetedSystemDisplayMaximumLuminance;
            const EBZCurveParameters* EBZParam = &(Hdr10Plus_Parms->Hdr10Plus_metadata.vSceneEBZParam);
            /* Guided OOTF :  TV side converted reference curve parameter for product monitor peak*/
            GuidedOOTF(LuminanceParam, EBZParam, refencePeak, productPeak, sceneMaxL, &productCurveParam);
        }
        Bezier_Wrapper(&productCurveParam, x, y, len);
        *normalizerNit = MIN(MAX(sceneMaxL, productPeak), HDR_MAX_LUMINANCE);
        normalization_xy(*normalizerNit, x, y, TM_ALGO_CURVE_LEN);

    }
    else if (tm_std == TM_STD_HDR10 || tm_std == TM_STD_PQ)
    {
        sceneMaxL = Hdr_Config->MaxCll;
        productPeak = Hdr_Config->MaxDll;

        assert(TM_ALGO_Linear <= tm_algo && tm_algo < TM_ALGO_End);
        if (tm_algo == TM_ALGO_Bezier)
        {
            EBZCurveParameters EBZParms;
            BasisOOTF(NULL, productPeak, sceneMaxL, &EBZParms);
           /* EBZParms.order = 9;
            EBZParms.Sx = 0.02;
            EBZParms.Sy = 0.1666667;
            EBZParms.vPCoeff[0] = 0.83333;
            EBZParms.vPCoeff[1] = 0.85416;
            EBZParms.vPCoeff[2] = 0.875;
            EBZParms.vPCoeff[3] = 0.89583;
            EBZParms.vPCoeff[4] = 0.916666;
            EBZParms.vPCoeff[5] = 0.9375;
            EBZParms.vPCoeff[6] = 0.9583333;
            EBZParms.vPCoeff[7] = 0.979166;*/
            Bezier_Wrapper(&EBZParms, x, y, len);
        }
        else if (tm_algo == TM_ALGO_Mobius)
        {
            mobius(x, y, len, Hdr_Config->Linear_Mobius, sceneMaxL, productPeak);
        }
        else if (tm_algo == TM_ALGO_ReinHard)
        {
            reinhard(x, y, len, Hdr_Config->Contrast_Reinhard, sceneMaxL, productPeak);
        }
        else if (tm_algo == TM_ALGO_Hable)
        {
            hable(x, y, len, sceneMaxL, productPeak);
        }
        else if (tm_algo == TM_ALGO_Linear)
        {
            double k = (double)sceneMaxL / productPeak;
            for (int i = 0; i < len; ++i)
            {
                y[i] = k * x[i];
            }
        }
    }
    else if (tm_std == TM_STD_HLG)
    {
        sceneMaxL = Hdr_Config->MaxCll;
        productPeak = Hdr_Config->MaxDll;
        HLG_OOTF(1000, x, y, len); //fixed gamma1.2 (1000nit reference)
        HLG_NPL_Scaler(y, len, Hdr_Config->MaxDll);
    }
    else if (tm_std == TM_SDR_IN_HDR)
    {
        double k = (double)(Hdr_Config->MaxLuminanceforSDRinHDR) / 10000.0;
        for (int i = 0; i < len; ++i)
        {
            y[i] = k * x[i];
        }
    }

    *normalizerNit = MIN(MAX(sceneMaxL, productPeak), HDR_MAX_LUMINANCE);

}

void interp(double* x_new, double* y_new, double* x_old, double* y_old, uint32_t len)
{
    for (int i = 0; i < len; ++i)
    {
        double xi = x_new[i];
        if (xi <= x_old[0])
        {
            y_new[i] = y_old[0];
            continue;
        }
        if (xi >= x_old[len - 1])
        {
            y_new[i] = y_old[len - 1];
            continue;
        }

        int left = 0, right = len - 1;
        while (right - left > 1)
        {
            int mid = (left + right) >> 1;
            if (x_old[mid] <= xi)
                left = mid;
            else
                right = mid;
        }
        double x0 = x_old[left], x1 = x_old[left + 1];
        double y0 = y_old[left], y1 = y_old[left + 1];
        double t = (xi - x0) / (x1 - x0);
        y_new[i] = y0 + t * (y1 - y0);
    }

}

//mapping y to 0-10000nits_x
void normalization_xy(float normalizerNit, double* x, double* y, uint32_t len)
{
    assert(len == TM_ALGO_CURVE_LEN);

    double x_new[TM_ALGO_CURVE_LEN] = { 0 };
    double y_new[TM_ALGO_CURVE_LEN] = { 0 };

    float k = normalizerNit / HDR_MAX_LUMINANCE;
    for (int i = 0; i < len; ++i)
    {
        x_new[i] = x[i] * k;
    }

    interp(x, y_new, x_new, y, len);
    memcpy(y, y_new, len * sizeof(double));

    return;
}

//normalize y to 0-10000 nits 
void normalization_y(float normalizerNit, double* y, uint32_t len)
{
    float k = normalizerNit / HDR_MAX_LUMINANCE;
    for (int i = 0; i < len; ++i)
    {
        y[i] = y[i] * k;
    }

    return;
}


void tm_gain(double* x, double* y, double* gain, uint32_t len)
{
    for (int i = 1; i < len; ++i)
    {
        gain[i] = y[i] / x[i];
    }
    gain[0] = gain[1];

    return;
}

void curve_to_lut(double* x, double* gain, uint32_t len_curve, int* pts, 
    uint32_t* lut_point, uint32_t* lut_data, uint32_t inbits, uint32_t len_lut)
{
    ASSERT(inbits >= TM_ALGO_CURVE_BITS);
    int data_gain = 1 << TM_GAIN_QUAN_BIT;
    int point_max = (1 << inbits) - 1;

    for (int i = 0; i < len_lut; ++i)
    {
        lut_data[i] = (uint32_t)(gain[pts[i]] * data_gain + 0.5);
        uint32_t temp_point = (uint32_t)(pts[i] << (inbits - TM_ALGO_CURVE_BITS));
        lut_point[i] = MIN(point_max, temp_point);
    }

    return;
}

