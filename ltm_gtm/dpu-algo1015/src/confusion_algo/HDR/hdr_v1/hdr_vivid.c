#include "sw_base.hpp"
#include "hdr_vivid.h"


//******************************************************************************
// Basic Definitions
//******************************************************************************

#define SDR_ENABLED                               1

#define HLGtoSDRStatic                              0
#define HLG_STATIC_BT2020toBT709                    0    // for implementation of Appendix C of T/UWA 005.1-2022 V1.2



//******************************************************************************
// CUVA HDR Definitions
//******************************************************************************
#define system_start_code_BIT        8
#define minimum_maxrgb_BIT           12
#define average_maxrgb_BIT           12
#define variance_maxrgb_BIT          12
#define maximum_maxrgb_BIT           12
#define tone_mapping_mode_BIT        1
#define tone_mapping_param_num_BIT   1
#define targeted_system_display_BIT  12
#define Base_flag_BIT                1
#define Base_param_m_p_BIT           14
#define Base_param_m_m_BIT           6
#define Base_param_m_a_BIT           10
#define Base_param_m_b_BIT           10
#define Base_param_m_n_BIT           6
#define Base_param_K1_BIT            2
#define Base_param_K2_BIT            2
#define Base_param_K3_BIT            4
#define Base_param_Delta_mode_BIT    3
#define Base_param_Delta_BIT         7
#define P3Spline_flag_BIT            1
#define P3Spline_num_BIT             1
#define P3Spline_TH_mode_BIT         2
#define P3Spline_TH_MB_BIT           8
#define P3Spline_TH_OFFSET_BIT       2
#define P3Spline_TH1_BIT             12
#define P3Spline_TH2_BIT             10
#define P3Spline_TH3_BIT             10
#define P3Spline_Strength_BIT        8
#define color_saturation_BIT         1
#define color_saturation_num_BIT     3
#define color_saturation_gain_BIT    8
#define CUVAHDR_METADATA_BASE_S      13
#define CUVAHDR_METADATA_BASE_B      10


double m_maxEtemp_store;
double m_inputMaxEtemp_store;
double m_maxE;
double m_inputMaxE;
double m_minE;
double m_inputMinE;

double maximum_maxrgb_noLine;
double minimum_maxrgb_noLine;
double average_maxrgb_noLine;
double variance_maxrgb_noLine;

double TML;
double TML_linear;
double RML;
double RML_linear;

bool isPQ;

typedef struct CuvaCurve
{
    double m_p;
    double m_m;
    double m_a;
    double m_b;
    double m_n;
    double K1;
    double K2;
    double K3;
    int curve_mintiao;
    double TH1, TH2, TH3;
    double md1, mc1, mb1, ma1;
    double md2, mc2, mb2, ma2;
    double DARKcurble_S1;
    double DARKcurble_offset;
    int curve_mintiao_high_area;
    double TH1_HIGH, TH2_HIGH, TH3_HIGH;
    double md1_high, mc1_high, mb1_high, ma1_high;
    double md2_high, mc2_high, mb2_high, ma2_high;
    int high_area_flag;
    double curve_adjust;
    double m_p_T;
    double m_a_T;
}CuvaCurve;


static inline int iMin(int a, int b) {
    return ((a) < (b)) ? (a) : (b);
}

static inline int iMax(int a, int b) {
    return ((a) > (b)) ? (a) : (b);
}

static inline int iClip(int x, int low, int high) {
    x = iMax(x, low);
    x = iMin(x, high);

    return x;
}

static inline float fMin(float a, float b) {
    return ((a) < (b)) ? (a) : (b);
}

static inline float fMax(float a, float b) {
    return ((a) > (b)) ? (a) : (b);
}

static inline float fClip(float x, float low, float high) {
    x = fMax(x, low);
    x = fMin(x, high);

    return x;
}

static inline double dMin(double a, double b) {
    return ((a) < (b)) ? (a) : (b);
}

static inline double dMax(double a, double b) {
    return ((a) > (b)) ? (a) : (b);
}

static inline double dClip(double x, double low, double high) {
    x = dMax(x, low);
    x = dMin(x, high);

    return x;
}

static inline float fAbs(float x) {
    return ((x) < 0) ? -(x) : (x);
}

static inline float fSign(float x) {
    return ((x) < 0.0) ? -1.0f : 1.0f;
}

static inline float fRound(float x) {
    return (fSign(x) * floor((fAbs(x) + 0.5f)));
}


// __UtilitiesHLGStatic_H__
const float KP1 = 0.5247;
const float KP2 = 0.7518;
const float maxDL = 0.638285;
const float maxSL = 0.7518;
#define x0               KP1
#define x1               maxSL
#define y0               KP1
#define y1               maxDL
#define y0delta          1
#define y1delta          0
#define x1Minusx0Square  ((x1-x0)*(x1-x0))
#define x1Minusx0Spline  (x1Minusx0Square*(x1-x0))

static double hmt(double x) {
    double alpha0, alpha1, beta0, beta1;

    alpha0 = (x1 - 3 * x0 + 2 * x)*(x1 - x)*(x1 - x) / x1Minusx0Spline;
    alpha1 = (3 * x1 - x0 - 2 * x)*(x - x0)*(x - x0) / x1Minusx0Spline;
    beta0 = (x - x0)*(x - x1)*(x - x1) / x1Minusx0Square;
    beta1 = (x - x0)*(x - x0)*(x - x1) / x1Minusx0Square;

    return y0 * alpha0 + y1 * alpha1 + y0delta * beta0 + y1delta * beta1;
}

static double ftm(double e) {                                 //equation (161) of T/UWA 005.1-2022 V1.2
    if (e <= KP1) {
        return e;
    }
    else if ((e > KP1) && (e < KP2)) {
        return hmt(e);
    }
    else {
        return maxDL;
    }
}
// __UtilitiesHLGStatic_H__


//inv_EOTF_PQ
static inline double PQinverse(double value)
{
    double a1 = (2610.0) / (4096.0 * 4.0);
    double a2 = (2523.0 * 128.0) / 4096.0;
    double b1 = (3424.0) / 4096.0;
    double b2 = (2413.0 *  32.0) / 4096.0;
    double b3 = (2392.0 *  32.0) / 4096.0;
    value = dClip(value, 0, 1.0);
    double tempValue = pow(value, a1);
    return (float)(pow(((b2 *(tempValue)+b1) / (1.0 + b3 * (tempValue))), a2));
}

//EOTF_PQ
static inline double PQforward(double value)
{
    double a1 = (2610.0) / (4096.0 * 4.0);
    double a2 = (2523.0 * 128.0) / 4096.0;
    double b1 = (3424.0) / 4096.0;
    double b2 = (2413.0 *  32.0) / 4096.0;
    double b3 = (2392.0 *  32.0) / 4096.0;
    value = dClip(value, 0, 1.0);
    double tempValue = pow(value, (1.0 / a2));
    return (pow(dMax(0.0, (tempValue - b1)) / (b2 - b3 * tempValue), (1.0 / a1)));
}

static inline double HLGforward(double value)
{
    double m_a = 0.17883277;
    double m_b = 0.02372241;
    double m_c = 1.00429347;
    return (value <= 0.5 ? (value*value / 3.0) : (exp((value - m_c) / m_a) + m_b));
}

#if SDR_ENABLED
static inline double Gammainverse(double value)
{
    double E;
    value = dClip(value, 0.0, 1.0);
    E = pow(value, 1.0 / 2.2);
    E = dClip(E, 0, 1.0);
    return E;
}
static inline double GammaForward(double value)
{
    double E;
    value = dClip(value, 0.0, 1.0);
    E = pow(value, 2.2); //see 005.1 chap 11.2,step g) gamma is 2.2 
    E = dClip(E, 0, 1.0);
    return E;
}
#endif


#define TPA_NUM      4

double TPA[TPA_NUM][2] = { { 2.5,0.99 },{ 3.5,0.879 },{ 4.5,0.777 },{ 7.5,0.54 } };
double getCuvaBaseParameterAdjust(CuvaCurve *curve)
{
    int index = 0;
    double M_a_T = TPA[0][1];
    for (int i = 0; i < TPA_NUM; i++)
    {
        if (curve->m_p <= TPA[i][0])
        {
            index = i;
            break;
        }
    }
    if ((index == 0) && (curve->m_p <= TPA[0][0])) M_a_T = TPA[0][1];
    else if ((index == 0) && (curve->m_p > TPA[(TPA_NUM - 1)][0])) M_a_T = TPA[(TPA_NUM - 1)][1];
    else
    {
        double temp1 = curve->m_p - TPA[index - 1][0];
        double temp2 = TPA[index][0] - curve->m_p;
        M_a_T = TPA[index][1] * temp1 + TPA[index - 1][1] * temp2;
        M_a_T /= (temp1 + temp2);
    }
    curve->curve_adjust = 0;
    if (curve->m_a > M_a_T)
    {
        curve->m_a = curve->m_a_T = M_a_T;
        curve->curve_adjust = 1;
        curve->m_b = 0;
    }
    return curve->curve_adjust;
}

double getCuvaBaseParameter(CuvaMetadata *metadata,
    double MasterDisplay,
    double MaxDisplay,
    double MinDisplay,
    CuvaCurve *curve,
    double *maxE,
    double *inputE)
{
    maximum_maxrgb_noLine = (double)(metadata->maximum_maxrgb) / 4095;
    minimum_maxrgb_noLine = (double)(metadata->minimum_maxrgb) / 4095;
    average_maxrgb_noLine = (double)(metadata->average_maxrgb) / 4095;
    variance_maxrgb_noLine = (double)(metadata->variance_maxrgb) / 4095;

    double meanVar = average_maxrgb_noLine + variance_maxrgb_noLine / 2;
    curve->m_m = 2.4;
    curve->m_n = 1.0;
    curve->m_b = 0.0;
    curve->K1 = 1.0;
    curve->K2 = 1.0;
    curve->K3 = 1.0;

    if (average_maxrgb_noLine > 0.6)
    {
        curve->m_p = 3.5;
    }
#if SDR_ENABLED
    else if (average_maxrgb_noLine > 0.1 && average_maxrgb_noLine <= 0.6)
#else
    else if (average_maxrgb_noLine > 0.3 && average_maxrgb_noLine <= 0.6)
#endif
    {
#if SDR_ENABLED
        curve->m_p = 6.0 + (average_maxrgb_noLine - 0.1) / (0.6 - 0.1) * (3.5 - 6.0);
#else
        curve->m_p = 4.0 + (average_maxrgb_noLine - 0.3) / (0.6 - 0.3) * (3.5 - 4.0);
#endif
    }
    else
    {
#if SDR_ENABLED
        curve->m_p = 6.0;
#else
        curve->m_p = 4.0;
#endif

    }

    double MaxDisplaySet = MaxDisplay;
    double MinDisplaySet = MinDisplay;
    m_maxE = MaxDisplaySet;
    m_minE = MinDisplaySet;

    m_inputMinE = 0;
    m_inputMaxE = maximum_maxrgb_noLine;
    m_inputMaxE = 0.8*meanVar + 0.2*maximum_maxrgb_noLine;


    double ReferfenceDisplay1600 = MasterDisplay;
    //if(isPQ == false){
            //float Lw = GetLw(metadata);
        //ReferfenceDisplay1600=PQinverse(Lw/10000.0);
    //}
    if (m_inputMaxE > ReferfenceDisplay1600)
    {
        m_inputMaxE = ReferfenceDisplay1600;
    }
    else if (m_inputMaxE < 0.5081)
    {
        m_inputMaxE = 0.5081;
    }
    else
    {
        m_inputMaxE = m_inputMaxE;
    }
    if (m_inputMaxE < m_maxE)
    {
        m_inputMaxE = m_maxE;
    }
#if SDR_ENABLED
    if (m_inputMaxE > 0.75)
#else
    if (m_inputMaxE > 0.9)
#endif
    {
        curve->m_p = curve->m_p + 0.6;
    }
#if SDR_ENABLED
    else if (m_inputMaxE > 0.67 && m_inputMaxE <= 0.75)
#else
    else if (m_inputMaxE > 0.75 && m_inputMaxE <= 0.9)
#endif
    {
#if SDR_ENABLED
        curve->m_p = curve->m_p + 0.3 + (m_inputMaxE - 0.67) / (0.75 - 0.67) * (0.6 - 0.3);
#else
        curve->m_p = curve->m_p + 0.0 + (m_inputMaxE - 0.75) / (0.9 - 0.75) * (0.6 - 0.0);
#endif
    }
    else
    {
#if SDR_ENABLED
        curve->m_p = curve->m_p + 0.3;
#else
        curve->m_p = curve->m_p + 0.0;
#endif
    }

    //get m_a
    double input_minE_TM = pow((curve->m_p * m_inputMinE / ((curve->m_p - 1)*m_inputMinE + 1.0)), curve->m_m);
    double input_maxE_TM = pow((curve->m_p * m_inputMaxE / ((curve->m_p - 1)*m_inputMaxE + 1.0)), curve->m_m);
    curve->m_a = (m_maxE - m_minE) / (input_maxE_TM - input_minE_TM);
    curve->m_b = MinDisplaySet;

    //find curve parameter2 from memadata targetdisplay = MaxDisplay
    for (int i = 0; i < 2; i++)
    {
        double targeted_system_display = (float)(metadata->targeted_system_display_maximum_luminance[i]) / ((1 << targeted_system_display_BIT) - 1);
        int HDRSDRTHMetadata = 2080;
        int targetedSystemDisplay = metadata->targeted_system_display_maximum_luminance[i];
        int MaxDisplayMetadata = (int)(MaxDisplay * ((1 << targeted_system_display_BIT) - 1));
        if ((metadata->Base_flag[i] == 0) || ((targetedSystemDisplay != HDRSDRTHMetadata) && (MaxDisplayMetadata == HDRSDRTHMetadata) && metadata->Base_flag[i]) || ((targetedSystemDisplay == HDRSDRTHMetadata) && (MaxDisplayMetadata != HDRSDRTHMetadata) && metadata->Base_flag[i]))
        {
            continue;
        }
        double targeted_system_display_linear = (float)(10000 * PQforward(targeted_system_display));
        double MaxDisplay_linear = (float)(10000 * PQforward(MaxDisplay));
        double deltai = abs(MaxDisplay_linear - targeted_system_display_linear) / 100;
        deltai = pow(deltai, 0.5);

        double param_m_p = curve->m_p;
        double param_m_m = curve->m_m;
        double param_m_n = curve->m_n;
        double param_K1 = curve->K1;
        double param_K2 = curve->K2;
        double param_K3 = curve->K3;
        if (metadata->Base_flag[i])
        {
            curve->m_p = 10 * (double)((metadata->Base_param_m_p[i])) / ((1 << Base_param_m_p_BIT) - 1);
            curve->m_m = (double)((metadata->Base_param_m_m[i])) / (10);
            curve->m_a = (double)((metadata->Base_param_m_a[i])) / ((1 << Base_param_m_a_BIT) - 1);
            curve->m_b = (double)((metadata->Base_param_m_b[i])) / (((1 << Base_param_m_b_BIT) - 1) * 4);
            curve->m_n = (double)((metadata->Base_param_m_n[i])) / (10);
            curve->K1 = (double)((metadata->Base_param_K1[i]));
            curve->K2 = (double)((metadata->Base_param_K2[i]));
            curve->K3 = (double)((metadata->Base_param_K3[i]));
            if (metadata->Base_param_K3[i] == 2) curve->K3 = maximum_maxrgb_noLine;
        }
        //		if((abs(targeted_system_display_linear - MaxDisplay_linear) <= 1) || (metadata->base_param_Delta_mode[i] == 3)) //for float value, need to compare these 2 values using this way.
        if ((targeted_system_display_linear == MaxDisplay_linear) || (metadata->base_param_Delta_mode[i] == 3)) //for fixed point system, directly compare these 2 values
        {
            break;
        }

        if ((metadata->base_param_Delta_mode[i] == 0) || (metadata->base_param_Delta_mode[i] == 2) || (metadata->base_param_Delta_mode[i] == 4) || (metadata->base_param_Delta_mode[i] == 6))
        {
            double deltaDisplay = (double)((metadata->base_param_Delta[i])) / ((1 << Base_param_Delta_BIT) - 1);
            deltaDisplay = (metadata->base_param_Delta_mode[i] == 2 || metadata->base_param_Delta_mode[i] == 6) ? (-deltaDisplay) : deltaDisplay;

            double weight = deltai * deltaDisplay;
            curve->m_p += weight;
            curve->m_p = dClip(curve->m_p, 3.0, 7.5);
            curve->m_a *= (MaxDisplay - MinDisplay) / targeted_system_display;
            curve->m_b *= (MaxDisplay - MinDisplay) / targeted_system_display; /*20210930 patch*/

            break;
        }
        else if (metadata->base_param_Delta_mode[i] == 1 || (metadata->base_param_Delta_mode[i] == 5))
        {
            double deltaDisplay = (double)((metadata->base_param_Delta[i])) / ((1 << Base_param_Delta_BIT) - 1);
            double weight = deltai * deltaDisplay;
            weight = weight >= 0 ? weight : -weight;
            if (weight > 1) weight = 1;
            double weightp = 1 - weight;
            curve->m_p = weightp * (curve->m_p) + weight * param_m_p;
            curve->m_m = weightp * (curve->m_m) + weight * param_m_m;
            curve->m_n = weightp * (curve->m_n) + weight * param_m_n;
            curve->K1 = weightp * (curve->K1) + weight * param_K1;
            curve->K2 = weightp * (curve->K2) + weight * param_K2;
            curve->K3 = weightp * (curve->K3) + weight * param_K3;

            double input_minE_TM = pow((curve->m_p * pow(m_inputMinE, curve->m_n) / ((curve->K1*curve->m_p - curve->K2)*pow(m_inputMinE, curve->m_n) + curve->K3)), curve->m_m);
            double input_maxE_TM = pow((curve->m_p * pow(m_inputMaxE, curve->m_n) / ((curve->K1*curve->m_p - curve->K2)*pow(m_inputMaxE, curve->m_n) + curve->K3)), curve->m_m);
            curve->m_a = (m_maxE - m_minE) / (input_maxE_TM - input_minE_TM);
            curve->m_b = m_minE; /*20210930 patch*/
            break;
        }
    }
    *maxE = m_maxE;
    *inputE = m_inputMaxE;

    return 1;
}


void AdjustCuvaParameter(double m_maxE,
    double m_inputMaxE,
    CuvaCurve *curve)
{
    if (curve->curve_adjust == 0) return;
    if ((curve->m_m < 2.35) || (curve->m_m > 2.45) || (curve->m_n < 0.95) || (curve->m_n > 1.05))
    {
        return;
    }
    if (m_inputMaxE < m_maxE) m_inputMaxE = m_maxE;
    double temp1 = m_maxE / m_inputMaxE;
    double max1 = (curve->m_p)*pow(m_inputMaxE, curve->m_n);
    double temp = (((curve->m_p)*(curve->K1) - (curve->K2))*(pow(m_inputMaxE, curve->m_n)) + (curve->K3));
    if (temp)  max1 /= temp;
    max1 = (curve->m_a_T)* pow(max1, curve->m_m) + (curve->m_b);
    double temp2 = max1 / m_inputMaxE;

    double WA = temp1 - temp2;
    if (WA < 0) WA = 0;
    WA /= (1 - temp2);

    if (curve->curve_mintiao)
    {
        temp = (1 - (curve->DARKcurble_S1))*WA;
        curve->DARKcurble_S1 += temp;

        double TH1temp = curve->TH1;
        double TH2temp = curve->TH2;
        double TH3temp = curve->TH3;
        temp = (m_inputMaxE - (curve->TH1))*WA;
        curve->TH1 += temp;
        curve->TH2 = curve->TH1 + TH2temp - TH1temp;
        curve->TH3 = curve->TH2 + TH3temp - TH2temp;
        curve->m_b *= (1 - WA);
    }
    else
    {

    }
}

double low_area_spline(double maximum_maxrgb,
    double average_maxrgb,
    double tone_mapping_param_m_p,
    double tone_mapping_param_m_m,
    double tone_mapping_param_m_a,
    double tone_mapping_param_m_b,
    double tone_mapping_param_m_n,
    double tone_mapping_param_K1,
    double tone_mapping_param_K2,
    double tone_mapping_param_K3,
    double P3Spline_TH_MB,
    double P3Spline_TH[3],
    double P3Spline_Strength,
    double maxDisplay,
    double *md1g,
    double *mc1g,
    double *mb1g,
    double *ma1g,
    double *md2g,
    double *mc2g,
    double *mb2g,
    double *ma2g,
    double *dark,
    double *DARKcurble_offset,
    int *curve_mintiao,
    double *m_a,
    unsigned int base_param_Delta_mode,
    unsigned int Base_flag)
{
    double threshold1 = 0.0;
    double threshold2 = 0.0;
    double threshold3 = 0.0;

    double m_ptemp = tone_mapping_param_m_p;
    double m_mtemp = tone_mapping_param_m_m;
    double m_atemp = tone_mapping_param_m_a;
    double m_btemp = tone_mapping_param_m_b;
    double m_ntemp = tone_mapping_param_m_n;
    double K1temp = tone_mapping_param_K1;
    double K2temp = tone_mapping_param_K2;
    double K3temp = tone_mapping_param_K3;

    double s1 = 1.0;
    if (average_maxrgb > 0.6)
    {
#if SDR_ENABLED
        s1 = 0.9;
#else
        threshold1 = 0.1;
        s1 = 0.96;
#endif
    }
    else if (average_maxrgb > 0.3 && average_maxrgb <= 0.6)
    {
#if SDR_ENABLED
        s1 = 1.0 + (average_maxrgb - 0.3) / (0.6 - 0.3) * (0.9 - 1.0);
#else
        threshold1 = 0.25 + (average_maxrgb - 0.3) / (0.6 - 0.3) * (0.1 - 0.25);
        s1 = 1.0 + (average_maxrgb - 0.3) / (0.6 - 0.3) * (0.96 - 1.0);
#endif
    }
    else
    {
#if SDR_ENABLED
#else
        threshold1 = 0.25;
#endif
        s1 = 1.0;
    }
#if SDR_ENABLED
    threshold1 = 0.0;
#endif
    threshold2 = threshold1 + 0.15;
    threshold3 = threshold2 + (threshold2 - threshold1) / 2.0;

    CuvaCurve curvetemp;
    curvetemp.m_p = tone_mapping_param_m_p;
    curvetemp.m_m = tone_mapping_param_m_m;
    curvetemp.m_b = tone_mapping_param_m_b;
    curvetemp.m_a = tone_mapping_param_m_a;
    curvetemp.m_n = tone_mapping_param_m_n;
    curvetemp.K1 = tone_mapping_param_K1;
    curvetemp.K2 = tone_mapping_param_K2;
    curvetemp.K3 = tone_mapping_param_K3;
    if ((base_param_Delta_mode) < 3 && Base_flag == 1)
    {
        getCuvaBaseParameterAdjust(&curvetemp);
    }
    curvetemp.TH1 = threshold1;
    curvetemp.TH2 = threshold2;
    curvetemp.TH3 = threshold3;
    curvetemp.DARKcurble_S1 = s1;
    curvetemp.curve_mintiao = 1;
    if ((base_param_Delta_mode) < 3 && Base_flag == 1) {
        AdjustCuvaParameter(m_maxE, m_inputMaxE, &curvetemp);
    }
    threshold1 = curvetemp.TH1;
    s1 = curvetemp.DARKcurble_S1;

    threshold2 = threshold1 + 0.15;
    threshold3 = threshold2 + (threshold2 - threshold1) / 2.0;

    double threshold3temp = pow(threshold3, m_ntemp);
    double ValueT3 = (m_ptemp * threshold3temp / ((K1temp*m_ptemp - K2temp)*threshold3temp + K3temp));
    ValueT3 = pow(ValueT3, m_mtemp);
    ValueT3 = m_atemp * ValueT3 + m_btemp;

    double threshold3temp1 = pow(threshold3, (m_ntemp - 1));
    double s2 = (m_ptemp * threshold3temp / ((K1temp*m_ptemp - K2temp)*threshold3temp + K3temp));
    s2 = pow(s2, m_mtemp + 1);
    s2 = (m_atemp*m_mtemp*m_ptemp*K3temp*m_ntemp*threshold3temp1*s2*(1 / pow(threshold3temp*m_ptemp, 2)));

    double a1 = s1 * threshold1 + *DARKcurble_offset;
    double b1 = s1;

    double threshold2temp = pow(threshold2, m_ntemp);
    double a2 = (m_ptemp * threshold2temp / ((K1temp*m_ptemp - K2temp)*threshold2temp + K3temp));
    a2 = pow(a2, m_mtemp);
    a2 = m_atemp * a2 + m_btemp;

    double h1 = threshold2 - threshold1;
    double h2 = threshold3 - threshold2;
    double y1 = a2;

    double y2 = (m_ptemp * threshold3temp / ((K1temp*m_ptemp - K2temp)*threshold3temp + K3temp));
    y2 = pow(y2, m_mtemp);
    y2 = m_atemp * y2 + m_btemp;

#if SDR_ENABLED
#else
    if (threshold3 - threshold1)
    {
        a2 = threshold2 * (y2 - a1) / (threshold3 - threshold1) + (threshold3*a1 - y2 * threshold1) / (threshold3 - threshold1);
    }
    y1 = a2;
#endif

    double b2 = -(3.0*a1*h2 *h2 + 3.0 * a2*h1*h1 - 3.0 * h1* h1 * y2 - 3.0 * h2 * h2 * y1 + h1 * h1 * h2*s2 + b1 * h1*h2 *h2) / (2.0 * h2*(h1 *h1 + h2 * h1));
    double c1 = (3.0*y1 - 2.0*b1*h1 - 3.0*a1 - b2 * h1) / (h1*h1);
    double d1 = (h1*b1 + h1 * b2 + 2 * a1 - 2.0*y1) / (h1*h1*h1);
    double c2 = c1 + 3.0 * d1*h1;
    double d2 = -(y2 - a2 - h2 * s2 + c1 * h2 *h2 + 3 * d1*h1*h2*h2) / (2 * h2* h2* h2);

    P3Spline_TH[0] = threshold1;
    P3Spline_TH[1] = threshold2;
    P3Spline_TH[2] = threshold3;
    *md1g = d1, *mc1g = c1, *mb1g = b1, *ma1g = a1;
    *md2g = d2, *mc2g = c2, *mb2g = b2, *ma2g = a2;
    *dark = s1;

    *curve_mintiao = 1;
    return *curve_mintiao;
}

double spline_area_spec(double maximum_maxrgb,
    double average_maxrgb,
    double tone_mapping_param_m_p,
    double tone_mapping_param_m_m,
    double tone_mapping_param_m_a,
    double *tone_mapping_param_m_b,
    double tone_mapping_param_m_n,
    double tone_mapping_param_K1,
    double tone_mapping_param_K2,
    double tone_mapping_param_K3,
    double P3Spline_TH_MB,
    double P3Spline_TH[3],
    double P3Spline_Strength,
    double maxDisplay,
    double *md1g,
    double *mc1g,
    double *mb1g,
    double *ma1g,
    double *md2g,
    double *mc2g,
    double *mb2g,
    double *ma2g,
    double *dark,
    double *DARKcurble_offset,
    int *curve_mintiao,
    unsigned int base_param_Delta_mode)

{
    double threshold1 = 0.0;
    double threshold2 = 0.0;
    double threshold3 = 0.0;

    double m_ptemp = tone_mapping_param_m_p;
    double m_mtemp = tone_mapping_param_m_m;
    double m_atemp = tone_mapping_param_m_a;
    double m_btemp = *tone_mapping_param_m_b;
    double m_ntemp = tone_mapping_param_m_n;
    double K1temp = tone_mapping_param_K1;
    double K2temp = tone_mapping_param_K2;
    double K3temp = tone_mapping_param_K3;

    double meta_str = P3Spline_Strength;
    double meta_MB = P3Spline_TH_MB;
    double s1 = meta_MB;

    threshold1 = P3Spline_TH[0];
    threshold2 = P3Spline_TH[1];
    threshold3 = P3Spline_TH[2];

    double threshold3temp = pow(threshold3, m_ntemp);
    double threshold3temp1 = pow(threshold3, (m_ntemp - 1));
    double s2 = (m_ptemp * threshold3temp / ((K1temp*m_ptemp - K2temp)*threshold3temp + K3temp));
    s2 = pow(s2, m_mtemp + 1);
    s2 = (m_atemp*m_mtemp*m_ptemp*K3temp*m_ntemp*threshold3temp1*s2*(1 / pow(threshold3temp*m_ptemp, 2)));

    double a1 = s1 * threshold1 + *DARKcurble_offset;
    double b1 = s1;

    double h1 = threshold2 - threshold1;
    double h2 = threshold3 - threshold2;

    double y2 = (m_ptemp * threshold3temp / ((K1temp*m_ptemp - K2temp)*threshold3temp + K3temp));
    y2 = pow(y2, m_mtemp);
    y2 = m_atemp * y2 + m_btemp;

    if (y2 > threshold3&&base_param_Delta_mode != 3 && base_param_Delta_mode != 2 && base_param_Delta_mode != 6)
    {
        m_btemp = m_btemp - (y2 - threshold3);
        y2 = threshold3;
        *tone_mapping_param_m_b = m_btemp;
    }
    double a2 = a1 + (y2 - a1) * (threshold2 - threshold1) / (threshold3 - threshold1) + (y2 - a1) * meta_str / 2;
    if (a2 > threshold2&&base_param_Delta_mode != 3 && base_param_Delta_mode != 2 && base_param_Delta_mode != 6)
    {
        a2 = threshold2;
    }
    double y1 = a2;

    double b2 = -(3.0*a1*h2 *h2 + 3.0 * a2*h1*h1 - 3.0 * h1* h1 * y2 - 3.0 * h2 * h2 * y1 + h1 * h1 * h2*s2 + b1 * h1*h2 *h2) / (2.0 * h2*(h1 *h1 + h2 * h1));

    double c1 = (3.0*y1 - 2.0*b1*h1 - 3.0*a1 - b2 * h1) / (h1*h1);
    double d1 = (h1*b1 + h1 * b2 + 2 * a1 - 2.0*y1) / (h1*h1*h1);
    double c2 = c1 + 3.0 * d1*h1;
    double d2 = -(y2 - a2 - h2 * s2 + c1 * h2 *h2 + 3 * d1*h1*h2*h2) / (2 * h2* h2* h2);

    P3Spline_TH[0] = threshold1;
    P3Spline_TH[1] = threshold2;
    P3Spline_TH[2] = threshold3;
    *md1g = d1, *mc1g = c1, *mb1g = b1, *ma1g = a1;
    *md2g = d2, *mc2g = c2, *mb2g = b2, *ma2g = a2;
    *dark = s1;

    *curve_mintiao = 1;
    return *curve_mintiao;
}


double spline_higharea_spec(double maximum_maxrgb,
    double average_maxrgb,
    double tone_mapping_param_m_p,
    double tone_mapping_param_m_m,
    double tone_mapping_param_m_a,
    double tone_mapping_param_m_b,
    double tone_mapping_param_m_n,
    double tone_mapping_param_K1,
    double tone_mapping_param_K2,
    double tone_mapping_param_K3,
    int   P3Spline_TH_Mode,
    double P3Spline_TH_MB,
    double P3Spline_TH[3],
    double P3Spline_Strength,
    double maxDisplay,
    double *md1g,
    double *mc1g,
    double *mb1g,
    double *ma1g,
    double *md2g,
    double *mc2g,
    double *mb2g,
    double *ma2g,
    double *dark,
    int *curve_mintiao_high_area,
    double Referncedisplay,
    unsigned int base_param_Delta_mode)
{
    double threshold1 = P3Spline_TH[0];
    double threshold2 = P3Spline_TH[1];
    double threshold3 = P3Spline_TH[2];
    double meta_str = P3Spline_Strength;

    double m_ptemp = tone_mapping_param_m_p;
    double m_mtemp = tone_mapping_param_m_m;
    double m_atemp = tone_mapping_param_m_a;
    double m_btemp = tone_mapping_param_m_b;
    double m_ntemp = tone_mapping_param_m_n;
    double K1temp = tone_mapping_param_K1;
    double K2temp = tone_mapping_param_K2;
    double K3temp = tone_mapping_param_K3;

    double threshold1temp = pow(threshold1, m_ntemp);
    double threshold1temp1 = pow(threshold1, m_ntemp - 1);
    double s1 = (m_ptemp * threshold1temp / ((K1temp*m_ptemp - K2temp)*threshold1temp + K3temp));
    s1 = pow(s1, m_mtemp + 1);
    s1 = (m_atemp*m_mtemp*m_ptemp*K3temp*m_ntemp*threshold1temp1*s1*(1 / pow(threshold1temp*m_ptemp, 2)));

    double y1 = (m_ptemp * threshold1temp / ((K1temp*m_ptemp - K2temp)*threshold1temp + K3temp));
    y1 = pow(y1, m_mtemp);
    y1 = m_atemp * y1 + m_btemp;

    double a1 = y1;
    double b1 = s1;

    double y2 = maxDisplay;
    if (base_param_Delta_mode != 3) {
        y2 = maxDisplay;
    }
    if (base_param_Delta_mode == 3) {
        y2 = Referncedisplay;
    }

    if (P3Spline_TH_Mode == 3)
    {
        double max = pow(threshold3, m_ntemp);
        y2 = (m_ptemp * max / ((K1temp*m_ptemp - K2temp)*max + K3temp));
        y2 = pow(y2, m_mtemp);
        y2 = m_atemp * y2 + m_btemp;
    }

    if ((P3Spline_TH_Mode == 1 || P3Spline_TH_Mode == 2) && y2 > threshold3&&base_param_Delta_mode != 2 && base_param_Delta_mode != 3 && base_param_Delta_mode != 6)
    {
        threshold3 = y2;
        threshold2 = threshold1 + (threshold3 - threshold1) / 2.0;
    }
    double h1 = threshold2 - threshold1;
    double h2 = threshold3 - threshold2;

    double a2 = y1 + (y2 - y1)*(threshold2 - threshold1) / (threshold3 - threshold1) + (y2 - y1)* meta_str / 2;

    if ((P3Spline_TH_Mode == 1 || P3Spline_TH_Mode == 2) && a2 > threshold2&& base_param_Delta_mode != 3 && base_param_Delta_mode != 2 && base_param_Delta_mode != 6) {
        a2 = threshold2;
    }

    double s2 = 1.0;
    if ((P3Spline_TH_Mode == 2) || (P3Spline_TH_Mode == 3))
    {
        double threshold1temp = pow(threshold3, m_ntemp);
        double threshold1temp0 = pow(threshold3, m_ntemp - 1);
        s2 = (m_ptemp * threshold1temp / ((K1temp*m_ptemp - K2temp)*threshold1temp + K3temp));
        s2 = pow(s2, m_mtemp + 1);
        s2 = (m_atemp*m_mtemp*m_ptemp*K3temp*m_ntemp*threshold1temp0*s2*(1 / pow(threshold1temp*m_ptemp, 2)));

        if (P3Spline_TH_Mode == 2)
        {
            s2 = s2 - P3Spline_TH_MB;
        }
    }
    else
    {
        double up_T = (y2 - y1) / (threshold3 - threshold2);
        double mid_T = (y2 - y1) / (threshold3 - threshold1);
        double down_T = (y2 - y1)*0.1 / (threshold3 - threshold1);

        down_T = down_T < s1 ? s1 : down_T;
        up_T = up_T < s1 ? s1 : up_T;

        s2 = meta_str >= 0 ? (up_T*meta_str + mid_T * (1 - meta_str)) : (down_T*(-meta_str) + mid_T * (1 + meta_str));
        if (s2 > 1.0) s2 = 1.0;
    }

    if (threshold3 == y2 && ((P3Spline_TH_Mode == 1) || (P3Spline_TH_Mode == 2)) && base_param_Delta_mode != 2 && base_param_Delta_mode != 3 && base_param_Delta_mode != 6) {
        s2 = 1.0;
    }
    double b2 = -(3.0*a1*h2 *h2 + 3.0 * a2*h1*h1 - 3.0 * h1* h1 * y2 - 3.0 * h2 * h2 * a2 + h1 * h1 * h2*s2 + b1 * h1*h2 *h2) / (2.0 * h2*(h1 *h1 + h2 * h1));
    double c1 = (3.0*a2 - 2.0*b1*h1 - 3.0*a1 - b2 * h1) / (h1*h1);
    double d1 = (h1*b1 + h1 * b2 + 2 * a1 - 2.0*a2) / (h1*h1*h1);
    double c2 = c1 + 3.0 * d1*h1;
    double d2 = -(y2 - a2 - h2 * s2 + c1 * h2 *h2 + 3 * d1*h1*h2*h2) / (2 * h2* h2* h2);

    P3Spline_TH[0] = threshold1;
    P3Spline_TH[1] = threshold2;
    P3Spline_TH[2] = threshold3;
    *md1g = d1, *mc1g = c1, *mb1g = b1, *ma1g = a1;
    *md2g = d2, *mc2g = c2, *mb2g = b2, *ma2g = a2;
    *curve_mintiao_high_area = 1;

    return *curve_mintiao_high_area;
}


void cuva_hdr_get_3spline(CuvaMetadata *metadata,
    double m_maxE,
    double m_inputMaxE,
    CuvaCurve *curve)
{
    double MaxDisplay = m_maxE;
    double maximum_maxrgbtemp = (double)((metadata->maximum_maxrgb)) / 4095;
    double average_maxrgbtemp = (double)((metadata->average_maxrgb)) / 4095;

    int HDRSDRTHMetadata = 2080;
    int MaxDisplayMetadata = (int)(MaxDisplay * ((1 << targeted_system_display_BIT) - 1));
    int i3spline = -1;
    int ibase = -1;
    double Referncedisplay = 0.67658;

    for (int i = 0; i < 2; i++)
    {
        int targetedSystemDisplay = metadata->targeted_system_display_maximum_luminance[i];
        double target = (double)(metadata->targeted_system_display_maximum_luminance[i]) / ((1 << targeted_system_display_BIT) - 1);
        Referncedisplay = target;
        if (((targetedSystemDisplay == HDRSDRTHMetadata) && (MaxDisplayMetadata == HDRSDRTHMetadata) && metadata->P3Spline_flag[i])
            || ((targetedSystemDisplay != HDRSDRTHMetadata) && (MaxDisplayMetadata != HDRSDRTHMetadata) && metadata->P3Spline_flag[i]))
        {
            i3spline = i;
            break;
        }
    }

    for (int j = 0; j < 2; j++)
    {
        int targetedSystemDisplay = metadata->targeted_system_display_maximum_luminance[j];
        double target = (double)(metadata->targeted_system_display_maximum_luminance[j]) / ((1 << targeted_system_display_BIT) - 1);
        Referncedisplay = target;
        if (((targetedSystemDisplay == HDRSDRTHMetadata) && (MaxDisplayMetadata == HDRSDRTHMetadata))
            || ((targetedSystemDisplay != HDRSDRTHMetadata) && (MaxDisplayMetadata != HDRSDRTHMetadata)))
        {
            ibase = j;
            break;
        }
    }
    int i3splinemode0 = 0;
    if (i3spline >= 0)
    {
        for (int spline_i = 0; spline_i < ((int)metadata->P3Spline_num[i3spline]); spline_i++)
        {
            if (metadata->P3Spline_TH_mode[i3spline][spline_i] == 0)
            {
                i3splinemode0 = 1;
            }
        }
    }
    if (i3spline < 0 || ((i3spline >= 0) && (i3splinemode0 == 0)))
    {
        double P3Spline_TH_MB = 0;
        double P3Spline_TH[3] = { 0,0,0 };
        double P3Spline_Strength = 0;
        double m_a_T = curve->m_a;
        curve->DARKcurble_offset = 0.0;
        unsigned int base_param_Delta_mode = 0;
        unsigned int Base_flag = 0;
        if (i3spline >= 0)
        {
            base_param_Delta_mode = metadata->base_param_Delta_mode[i3spline];
            Base_flag = metadata->Base_flag[i3spline];
        }
        else
        {
            if (ibase >= 0)
            {
                base_param_Delta_mode = metadata->base_param_Delta_mode[ibase];
                Base_flag = metadata->Base_flag[ibase];
            }
        }
        low_area_spline(maximum_maxrgbtemp,
            average_maxrgbtemp,
            curve->m_p,
            curve->m_m,
            curve->m_a,
            curve->m_b,
            curve->m_n,
            curve->K1,
            curve->K2,
            curve->K3,
            P3Spline_TH_MB,
            P3Spline_TH,
            P3Spline_Strength,
            MaxDisplay,
            //output
            &curve->md1,
            &curve->mc1,
            &curve->mb1,
            &curve->ma1,
            &curve->md2,
            &curve->mc2,
            &curve->mb2,
            &curve->ma2,
            &curve->DARKcurble_S1,
            &curve->DARKcurble_offset,
            &curve->curve_mintiao,
            &m_a_T,
            base_param_Delta_mode,
            Base_flag);

        curve->TH1 = P3Spline_TH[0];
        curve->TH2 = P3Spline_TH[1];
        curve->TH3 = P3Spline_TH[2];
    }
    if (i3spline >= 0)
    {
        for (int spline_i = 0; spline_i < ((int)metadata->P3Spline_num[i3spline]); spline_i++)
        {
            double P3Spline_Strength_org = (double)((metadata->P3Spline_Strength[i3spline][spline_i] * 2)) / ((1 << P3Spline_Strength_BIT) - 1) - 1.0;
            int P3Spline_TH_OFFSET_code = metadata->P3Spline_TH_MB[i3spline][spline_i] & ((1 << P3Spline_TH_OFFSET_BIT) - 1);
            int P3Spline_TH_MB_code = (metadata->P3Spline_TH_MB[i3spline][spline_i] >> P3Spline_TH_OFFSET_BIT);
            double P3Spline_TH_MB = (double)((P3Spline_TH_MB_code * 1)) / ((1 << (P3Spline_TH_MB_BIT - P3Spline_TH_OFFSET_BIT)) - 1);
            if (metadata->P3Spline_TH_mode[i3spline][spline_i] == 0) {
                curve->DARKcurble_offset = (double)(P3Spline_TH_OFFSET_code * 0.1) / ((1 << P3Spline_TH_OFFSET_BIT) - 1);
            }
            if (metadata->P3Spline_TH_mode[i3spline][spline_i] != 0)
            {
                P3Spline_TH_MB = (float)((metadata->P3Spline_TH_MB[i3spline][spline_i] * 1.1)) / ((1 << P3Spline_TH_MB_BIT) - 1);
            }
            double TH1temp = (double)((metadata->P3Spline_TH[i3spline][spline_i][0])) / ((1 << P3Spline_TH1_BIT) - 1);
            double TH2temp = (double)((metadata->P3Spline_TH[i3spline][spline_i][1])) / (((1 << P3Spline_TH2_BIT) - 1) * 4) + TH1temp;
            double TH3temp = (double)((metadata->P3Spline_TH[i3spline][spline_i][2])) / (((1 << P3Spline_TH3_BIT) - 1) * 4) + TH2temp;
            double P3Spline_TH[3] = { TH1temp, TH2temp, TH3temp };

            if (metadata->P3Spline_TH_mode[i3spline][spline_i] == 0)
            {
                CuvaCurve curvetemp;
                memcpy(&curvetemp, curve, sizeof(CuvaCurve));
                if ((metadata->base_param_Delta_mode[i3spline]) < 3 && metadata->Base_flag[i3spline] == 1)

                {
                    getCuvaBaseParameterAdjust(&curvetemp);
                }
                curvetemp.TH1 = P3Spline_TH[0];
                curvetemp.TH2 = P3Spline_TH[1];
                curvetemp.TH3 = P3Spline_TH[2];
                curvetemp.DARKcurble_S1 = P3Spline_TH_MB;
                curvetemp.curve_mintiao = 1;
                if ((metadata->base_param_Delta_mode[i3spline]) < 3 && metadata->Base_flag[i3spline] == 1)

                {
                    AdjustCuvaParameter(m_maxE,
                        m_inputMaxE,
                        &curvetemp);
                }
                P3Spline_TH[0] = curvetemp.TH1;
                P3Spline_TH[1] = curvetemp.TH2;
                P3Spline_TH[2] = curvetemp.TH3;
                curve->m_b = curvetemp.m_b;
                P3Spline_TH_MB = curvetemp.DARKcurble_S1;

                double P3Spline_Strength = P3Spline_Strength_org;

                double curve_ready1 = spline_area_spec(maximum_maxrgbtemp,
                    average_maxrgbtemp,
                    curve->m_p,
                    curve->m_m,
                    curve->m_a,
                    &curve->m_b,
                    curve->m_n,
                    curve->K1,
                    curve->K2,
                    curve->K3,
                    P3Spline_TH_MB,
                    P3Spline_TH,
                    P3Spline_Strength,
                    MaxDisplay,
                    //output
                    &curve->md1,
                    &curve->mc1,
                    &curve->mb1,
                    &curve->ma1,
                    &curve->md2,
                    &curve->mc2,
                    &curve->mb2,
                    &curve->ma2,
                    &curve->DARKcurble_S1,
                    &curve->DARKcurble_offset,
                    &curve->curve_mintiao,
                    metadata->base_param_Delta_mode[i3spline]);
                if (curve_ready1)
                {
                    curve->TH1 = P3Spline_TH[0];
                    curve->TH2 = P3Spline_TH[1];
                    curve->TH3 = P3Spline_TH[2];
                }

            }
            else if ((metadata->P3Spline_TH_mode[i3spline][spline_i] == 1) || (metadata->P3Spline_TH_mode[i3spline][spline_i] == 2) || (metadata->P3Spline_TH_mode[i3spline][spline_i] == 3))
            {
                double maxContent_in = maximum_maxrgbtemp;
                maxContent_in = m_maxE > maxContent_in ? m_maxE : maxContent_in;
                {
                    double P3Spline_Strength = P3Spline_Strength_org;

                    double threshold1temp = P3Spline_TH[0];
                    double threshold2temp = P3Spline_TH[1];
                    double threshold3temp = P3Spline_TH[2];

                    if (curve->curve_mintiao)
                    {
                        if (threshold3temp <= curve->TH3)
                        {
                            continue;
                        }
                        else if (threshold1temp < curve->TH3)
                        {
                            threshold1temp = curve->TH3;
                            threshold2temp = (threshold1temp + threshold3temp) / 2;
                        }
                    }

                    P3Spline_TH[0] = threshold1temp;
                    P3Spline_TH[1] = threshold2temp;
                    P3Spline_TH[2] = threshold3temp;

                    double curve_ready1 = spline_higharea_spec(maximum_maxrgbtemp,
                        average_maxrgbtemp,
                        curve->m_p,
                        curve->m_m,
                        curve->m_a,
                        curve->m_b,
                        curve->m_n,
                        curve->K1,
                        curve->K2,
                        curve->K3,
                        metadata->P3Spline_TH_mode[i3spline][spline_i],
                        P3Spline_TH_MB,
                        P3Spline_TH,
                        P3Spline_Strength,
                        MaxDisplay,
                        //output
                        &curve->md1_high,
                        &curve->mc1_high,
                        &curve->mb1_high,
                        &curve->ma1_high,
                        &curve->md2_high,
                        &curve->mc2_high,
                        &curve->mb2_high,
                        &curve->ma2_high,
                        &curve->DARKcurble_S1,
                        &curve->curve_mintiao_high_area,
                        Referncedisplay,
                        metadata->base_param_Delta_mode[i3spline]);

                    if (curve_ready1)
                    {
                        curve->TH1_HIGH = P3Spline_TH[0];
                        curve->TH2_HIGH = P3Spline_TH[1];
                        curve->TH3_HIGH = P3Spline_TH[2];
                        if (metadata->P3Spline_TH_mode[i3spline][spline_i] == 3)
                        {
                            curve->high_area_flag = 1;
                        }
                    }
                }

            }
        }
    }

}


CuvaCurve InitCUVAParams(double max_display_luminance, CuvaMetadata metadata, double MasterDisplay, bool pq)
{
    CuvaCurve curve;
    //Initialization
    memset(&curve, 0, sizeof(CuvaCurve));

    isPQ = pq;
    double m_maxEtemp;
    double m_inputMaxEtemp;

    double MaxDisplay = (double)(PQinverse(max_display_luminance / 10000.0));
    //double MinDisplay = (double)(PQinverse(0.05/10000.00)); //0.05 for HDR STB case, 0.1 for SDR STB case. see CUVA_005.3-3-2021 table 3
    double MinDisplay = (double)(PQinverse(0.00 / 10000.00)); //0.05 for HDR STB case, 0.1 for SDR STB case. see CUVA_005.3-3-2021 table 3


    //»ù´¡ÇúÏß²ÎÊý
    getCuvaBaseParameter(&metadata,
        MasterDisplay,
        MaxDisplay,
        MinDisplay,
        //base curve
        &curve,
        &m_maxEtemp,
        &m_inputMaxEtemp);

    //Èý´ÎÑùÌõ²ÎÊý
    cuva_hdr_get_3spline(&metadata,
        m_maxEtemp,
        m_inputMaxEtemp,
        &curve);

    m_inputMaxEtemp_store = m_inputMaxEtemp;
    m_maxEtemp_store = m_maxEtemp;

    MaxDisplay = m_maxEtemp_store;
    TML = MaxDisplay;
    TML_linear = (float)(10000 * PQforward(TML));
    RML = MasterDisplay;
    //if(isPQ == false){
        //float Lw = GetLw(&metadata);
        //RML = PQinverse(Lw/10000.0);
    //}
    RML_linear = (float)(10000 * PQforward(RML));
    if (TML_linear > RML_linear) RML_linear = TML_linear;
    if (TML > RML) RML = TML;

    return curve;
}


// float maxComp0 = (float)(PQinverse(comp0 / 10000));max = max(maxComp0,maxComp1,maxComp2)
double calc_curve(double max, CuvaCurve *TMP)
{
    double max1 = max;
    if (TMP->curve_mintiao&&TMP->curve_mintiao_high_area)
    {
        if (max <= TMP->TH1) {
            max1 = TMP->DARKcurble_S1 * max + TMP->DARKcurble_offset;
        }
        else if (max > TMP->TH1&&max <= TMP->TH2)
            max1 = TMP->md1 * pow((max - TMP->TH1), 3) + TMP->mc1 * pow((max - TMP->TH1), 2) + TMP->mb1 * pow((max - TMP->TH1), 1) + TMP->ma1;
        else if (max > TMP->TH2&&max <= TMP->TH3)
        {
            max1 = TMP->md2 * pow((max - TMP->TH2), 3) + TMP->mc2 * pow((max - TMP->TH2), 2) + TMP->mb2 * pow((max - TMP->TH2), 1) + TMP->ma2;
        }
        else if (max > TMP->TH1_HIGH&&max <= TMP->TH2_HIGH)
        {
            max1 = TMP->md1_high * pow((max - TMP->TH1_HIGH), 3) + TMP->mc1_high * pow((max - TMP->TH1_HIGH), 2) + TMP->mb1_high * pow((max - TMP->TH1_HIGH), 1) + TMP->ma1_high;
        }
        else if (max > TMP->TH2_HIGH&&max <= TMP->TH3_HIGH)
        {
            max1 = TMP->md2_high * pow((max - TMP->TH2_HIGH), 3) + TMP->mc2_high * pow((max - TMP->TH2_HIGH), 2) + TMP->mb2_high * pow((max - TMP->TH2_HIGH), 1) + TMP->ma2_high;
        }
        else  if ((max > TMP->TH3_HIGH) && (TMP->high_area_flag == 0))
        {
            max1 = (3 * TMP->md2_high * pow((TMP->TH3_HIGH - TMP->TH2_HIGH), 2) + 2 * TMP->mc2_high * pow((TMP->TH3_HIGH - TMP->TH2_HIGH), 1) + TMP->mb2_high)*(max - TMP->TH3_HIGH)
                + TMP->md2_high * pow((TMP->TH3_HIGH - TMP->TH2_HIGH), 3) + TMP->mc2_high * pow((TMP->TH3_HIGH - TMP->TH2_HIGH), 2) + TMP->mb2_high * pow((TMP->TH3_HIGH - TMP->TH2_HIGH), 1) + TMP->ma2_high;
        }
        else
        {
            max = pow(max, TMP->m_n);
            max1 = (TMP->m_p * max / ((TMP->K1*TMP->m_p - TMP->K2)*max + TMP->K3));
            max1 = pow(max1, TMP->m_m);
            max1 = TMP->m_a * max1 + TMP->m_b;
        }
    }
    else if (TMP->curve_mintiao)
    {
        if (max <= TMP->TH1) {
            max1 = TMP->DARKcurble_S1 * max + TMP->DARKcurble_offset;
        }
        else if (max > TMP->TH1&&max <= TMP->TH2)
            max1 = TMP->md1 * pow((max - TMP->TH1), 3) + TMP->mc1 * pow((max - TMP->TH1), 2) + TMP->mb1 * pow((max - TMP->TH1), 1) + TMP->ma1;
        else if (max > TMP->TH2&&max <= TMP->TH3)
            max1 = TMP->md2 * pow((max - TMP->TH2), 3) + TMP->mc2 * pow((max - TMP->TH2), 2) + TMP->mb2 * pow((max - TMP->TH2), 1) + TMP->ma2;
        else
        {
            max = pow(max, TMP->m_n);
            max1 = (TMP->m_p * max / ((TMP->K1*TMP->m_p - TMP->K2)*max + TMP->K3));
            max1 = pow(max1, TMP->m_m);
            max1 = TMP->m_a * max1 + TMP->m_b;
        }
    }
    else if (TMP->curve_mintiao_high_area)
    {
        if (max > TMP->TH1_HIGH&&max <= TMP->TH2_HIGH)
            max1 = TMP->md1_high * pow((max - TMP->TH1_HIGH), 3) + TMP->mc1_high * pow((max - TMP->TH1_HIGH), 2) + TMP->mb1_high * pow((max - TMP->TH1_HIGH), 1) + TMP->ma1_high;
        else if (max > TMP->TH2_HIGH&&max <= TMP->TH3_HIGH)
            max1 = TMP->md2_high * pow((max - TMP->TH2_HIGH), 3) + TMP->mc2_high * pow((max - TMP->TH2_HIGH), 2) + TMP->mb2_high * pow((max - TMP->TH2_HIGH), 1) + TMP->ma2_high;
        else  if ((max > TMP->TH3_HIGH) && (TMP->high_area_flag == 0))
        {
            max1 = (3 * TMP->md2_high * pow((TMP->TH3_HIGH - TMP->TH2_HIGH), 2) + 2 * TMP->mc2_high * pow((TMP->TH3_HIGH - TMP->TH2_HIGH), 1) + TMP->mb2_high)*(max - TMP->TH3_HIGH)
                + TMP->md2_high * pow((TMP->TH3_HIGH - TMP->TH2_HIGH), 3) + TMP->mc2_high * pow((TMP->TH3_HIGH - TMP->TH2_HIGH), 2) + TMP->mb2_high * pow((TMP->TH3_HIGH - TMP->TH2_HIGH), 1) + TMP->ma2_high;
        }
        else
        {
            max = pow(max, TMP->m_n);
            max1 = (TMP->m_p * max / ((TMP->K1 * TMP->m_p - TMP->K2) * max + TMP->K3));
            max1 = pow(max1, TMP->m_m);
            max1 = TMP->m_a * max1 + TMP->m_b;
        }
    }
    else
    {
        max = pow(max, TMP->m_n);
        max1 = (TMP->m_p * max / ((TMP->K1*TMP->m_p - TMP->K2)*max + TMP->K3));
        max1 = pow(max1, TMP->m_m);
        max1 = TMP->m_a * max1 + TMP->m_b;
    }
    return max1;
}

float GetLw(const CuvaMetadata *metadata)
{
    if (metadata->system_start_code == 0x1 || metadata->system_start_code == 0x2) {
        return 1000.0;
    }
    else if (metadata->system_start_code == 0x3 || metadata->system_start_code == 0x4 || metadata->system_start_code == 0x5 || metadata->system_start_code == 0x6 || metadata->system_start_code == 0x7) {
        return (float)(2000.0*(metadata->system_start_code - 2));
    }
    else {
        exit(0);
    }
}

void apply_hlg_ootf_to_tm_curve(double* x, uint32_t len, float peaklum)
{
    for (int i = 0; i < len; ++i)
    {
        double gamma = 1.2 + 0.42 * log10((double)(peaklum / 1000.0));
        x[i] = pow(x[i], gamma)*peaklum / 10000.0;
    }
}

void getVividCurve(const Hdr_Algo_Config* Hdr_Config, double* x, double* y, uint32_t len)
{
    bool pq;
    if (Hdr_Config->EO_mode == EO_PQ)
        pq = true;
    else if (Hdr_Config->EO_mode == EO_HLG)
        pq = false;
    else
        assert(0);

    CuvaCurve curve = InitCUVAParams(Hdr_Config->MaxDll, Hdr_Config->Hdr_Vivid_Metadata, Hdr_Config->MasterDisplay, pq);

    double ScaleFactor = 1.0;
    if (Hdr_Config->OE_mode != OE_PQ)
        ScaleFactor = (double)10000 / Hdr_Config->MaxDll;

    double* temp_x = (double*)malloc(len * sizeof(double));
    memcpy(temp_x, x, len * sizeof(double));

    if (Hdr_Config->Hdr_Vivid_Metadata_Exit == 1)
    {
        if (Hdr_Config->EO_mode == EO_HLG)
        {
            // add HLG OOTF
            // but in HW tonemapping:x is maxrgb;bucause Vivid use maxrgb in tonemapping.
            float peaklum = GetLw(&(Hdr_Config->Hdr_Vivid_Metadata));
            apply_hlg_ootf_to_tm_curve(temp_x, len, peaklum);
        }

        // tone mapping curve
        for (int i = 0; i < len; ++i)
        {
            double non_linear_x = PQinverse(temp_x[i]);
            double non_linear_out_x = calc_curve(non_linear_x, &curve);
            y[i] = PQforward(non_linear_out_x);

            y[i] *= ScaleFactor;

        }
    }
    else
    {
        //input HLG
        if (Hdr_Config->EO_mode != EO_HLG)
        {
            LOGE("in Vivid spec, only HLG can out of metadata\n");
            assert(0);
        }
        // add HLG OOTF
        // but in HW tonemapping:x is luma;
        apply_hlg_ootf_to_tm_curve(temp_x, len, 1000.0);

        // tone mapping curve
        for (int i = 0; i < len; ++i)
        {
            double non_linear_x = PQinverse(temp_x[i]);
            double non_linear_out_x = ftm(non_linear_x);
            y[i] = PQforward(non_linear_out_x);
            y[i] *= 10000.0;
            y[i] = dClip(y[i], 0, 350);
            y[i] /= 350.0;
        }

    }
    free(temp_x);
    return;
}

