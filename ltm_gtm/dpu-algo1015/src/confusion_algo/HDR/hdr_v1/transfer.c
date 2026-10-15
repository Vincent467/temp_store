#include "sw_base.hpp"

#include "transfer.h"
#include "basefunc.h"
#include "hdr_def.h"

/*=======================================================[EOTF & OETF transfer function]======================================================== */
/*=========================================[ NULL ]========================================= */
double Transfer_Linear(double value)
{
    assert(0 <= value && value <= 1);

    return value;
}

double EOTF_NULL(double value)
{
    assert(0 <= value && value <= 1);

    return value;
}

double OETF_NULL(double value)
{
    assert(0 <= value && value <= 1);

    return value;
}

/*========================================= [gamma 1.8] ========================================= */
double EOTF_DeGamma1_8(double value)
{
    assert(0 <= value && value <= 1);

    return pow(value, 1.8);
}

double OETF_Gamma1_8(double value)
{
    assert(0 <= value && value <= 1);

    return pow(value, 1.0/1.8);
}

/*========================================= [gamma 2.2] ========================================= */
double EOTF_DeGamma2_2(double value)
{
    assert(0 <= value && value <= 1);

    return pow(value, 2.2);
}

double OETF_Gamma2_2(double value)
{
    assert(0 <= value && value <= 1);

    return pow(value, 1.0/2.2);
}

/*========================================= [gamma 2.5] ========================================= */
double EOTF_DeGamma2_5(double value)
{
    assert(0 <= value && value <= 1);

    return pow(value, 2.5);
}

double OETF_Gamma2_5(double value)
{
    assert(0 <= value && value <= 1);

    return pow(value, 1.0/2.5);
}

/*========================================= [gamma 2.6] ========================================= */
double EOTF_DeGamma2_6(double value)
{
    assert(0 <= value && value <= 1);

    return pow(value, 2.6);
}

double OETF_Gamma2_6(double value)
{
    assert(0 <= value && value <= 1);

    return pow(value, 1.0/2.6);
}

/*========================================= [PQ] ========================================= */

/* PQ-SMPTE-ST2084 */
double EOTF_PQ(double value)
{
    assert(0 <= value && value <= 1);

    double m1 = 2610.0 / 4096.0 / 4.0;
    double m2 = 2523.0 / 4096.0 * 128.0;
    double c1 = 3424.0 / 4096.0;
    double c2 = 2413.0 / 4096.0 * 32.0;
    double c3 = 2392.0 / 4096.0 * 32.0;
    /*double m1 = 0.1593017578125; */
    /*double m2 = 78.84375; */
    /*double c1 = 0.8359375; */
    /*double c2 = 18.8515625; */
    /*double c3 = 18.6875; */

    double out_value = 0;

    double tempValue = pow(value, (1.0 / m2));
    out_value = pow( fmax( 0.0, (tempValue - c1) ) / (c2 - c3 * tempValue), (1.0 / m1));

    return out_value;
}

double OETF_PQ(double value)
{
    assert(0 <= value && value <= 1);

    double m1 = (2610.0) / (4096.0 * 4.0);
    double m2 = (2523.0 * 128.0) / 4096.0;
    double c1 = (3424.0) / 4096.0;
    double c2 = (2413.0 * 32.0) / 4096.0;
    double c3 = (2392.0 * 32.0) / 4096.0;

    double out_value = 0;

    double tempValue = pow(value, m1);
    out_value = (double)(pow(((c2 * (tempValue)+c1) / (1.0 + c3 * (tempValue))), m2));

    return out_value;
}


/*========================================= [170M] ========================================= */
/* SMPTE-ST170M */
double EOTF_170M(double value)
{
    assert(0 <= value && value <= 1);

    double result = 0;

    if (0 <= value && value < 0.0812)
    {
        result = value / 4.5;
    }
    else if (0.0812 <= value && value <= 1)
    {
        result = pow(((value + 0.099) / 1.099), (1.0 / 0.45));
    }
    else
    {
        printf("[%s:%d] value(%f) error\'\n", __FUNCTION__, __LINE__, value);
        exit(EXIT_FAILURE);
    }

    return result;
}

double OETF_170M(double value)
{
    assert(0 <= value && value <= 1);

    double result = 0;

    if (0 <= value && value < 0.018)    /* 0.0812 0821 */
    {
        result = value * 4.5;
    }
    else if (0.018 <= value && value <= 1)
    {
        result = 1.099 * pow(value, 0.45) - 0.099;
    }
    else
    {
        printf("[%s:%d] value(%f) error\'\n", __FUNCTION__, __LINE__, value);
        exit(EXIT_FAILURE);
    }

    return result;
}

/*========================================= [sRGB] ========================================= */
/* sRGB-IEC61966 */
double EOTF_sRGB(double value)
{
    assert(0 <= value && value <= 1);

    double result = 0;

    if (0 <= value && value < 0.04045)
    {
        result = value / 12.92;
    }
    else if (0.04045 <= value && value <= 1)
    {
        result = pow((value + 0.055) / 1.055, 2.4);
    }
    else
    {
        printf("[%s:%d] value(%f) error\'\n", __FUNCTION__, __LINE__, value);
        exit(EXIT_FAILURE);
    }

    return result;
}

double OETF_sRGB(double value)
{
    assert(0 <= value && value <= 1);

    double result = 0;

    if (0 <= value && value < 0.0031308)
    {
        result = value * 12.92;
    }
    else if (0.0031308 <= value && value <= 1)
    {
        result = 1.055 * pow(value, (1.0 / 2.4)) - 0.055;
    }
    else
    {
        printf("[%s:%d] value(%f) error\'\n", __FUNCTION__, __LINE__, value);
        exit(EXIT_FAILURE);
    }

    return result;
}

/*========================================= [HLG] ========================================= */

/* HLG-BT.2100 */
double EOTF_HLG(double value)
{
    assert(0 <= value && value <= 1);
/*
//    double LB = 0;
//    double LW = 1000;
//    double r = 1.2 + 0.42*log10(LW/1000.0);
//    double beta = pow( 3*pow((LB / LW), (double)1.0/r), (double)1.0/2);
//
//    value = max_double(0, (1.0 - beta)*value + beta);
//
//    value = Clamp_double(value, 0, 1.0);
//    printf(" LB:%f LW:%f r:%f beta:%f --> value:%f\n", LB, LW, r, beta, value);
*/
    /*float a = 0.17883277, b = 0.28466892, c = 0.55991073; */
    double a = 0.17883277f;
    double b = 1 - 4* a;
    double c = (double) (0.5 - a * log(4*a));

    double out_value;

    if(0 <= value && value <= 0.5)
        out_value = value*value / 3.0;
    else
        out_value = (exp((value-c)/a) + b) / 12.0;

    out_value = Clamp_double(out_value, 0, 1);

    return out_value;
}

double OETF_HLG(double value)
{
    assert(0 <= value && value <= 1);

    double a = 0.17883277f;
    double b = 1 - 4* a;
    double c = (double) (0.5 - a * log(4*a));

    double out_value = 0;

    if( 0 <= value && value <= ((double)1.0 / 12.0) )
        out_value = sqrt(3.0 * value);
    else
        out_value = a * log(12.0*value - b) + c ;

    out_value = Clamp_double(out_value, 0, 1);

    return out_value;
}

double ACESFilm(double x)
{
    double a = 2.51;
    double b = 0.03;
    double c = 2.43;
    double d = 0.59;
    double e = 0.14;

    double result = (x*(a*x+b)) / (x*(c*x+d)+e);

    return Clamp_double( result, (double)0.0, (double)1.0);
}

double ACESFilm_Amp(double value)
{
    assert(0 <= value && value <= 1);

    double y = ACESFilm(value);
    double ampCoef = 1 / ACESFilm(1.0);
    double result = y * ampCoef;

    return result;
}

double OOTF_TM(const double * const OotfCurve, U32 OotfCurveNum, double value)
{
    assert(NULL != OotfCurve);
    assert(0 <= value && value <= 1);

    double value_f=0, value_a=0;
    double out_value = 0;
    double gap;
    double inter_weight;
    U32 entry=0;

    gap = (double)1.0/(OotfCurveNum -1);
    entry = (U32)(value / gap);
    inter_weight = value - ((double)entry/(OotfCurveNum -1));

    if(0 == value)
        out_value = OotfCurve[0];
    else if(0 < value && value < 1)
    {
        value_f = OotfCurve[entry];
        value_a = OotfCurve[entry + 1];
        out_value = (value_f * (gap - inter_weight) + value_a * inter_weight) / gap;
    }
    else
        out_value = OotfCurve[entry];

    return out_value;
}

double (*TF_MapFrom_Type(TypeTF typeTF))(double)
{
    switch(typeTF)
    {
    case TF_Linear:
        return Transfer_Linear;
        break;

    case TF_Eotf_PQ:
        return EOTF_PQ;
        break;
    case TF_Eotf_HLG:
        return EOTF_HLG;
        break;
    case TF_Eotf_170M:
        return EOTF_170M;
        break;
    case TF_Eotf_sRGB:
        return EOTF_sRGB;
        break;
/*    case TF_Eotf_DeGamma1_8: */
/*        return EOTF_DeGamma1_8; */
/*        break; */
    case TF_Eotf_DeGamma2_2:
        return EOTF_DeGamma2_2;
        break;
/*    case TF_Eotf_DeGamma2_5: */
/*        return EOTF_DeGamma2_5; */
/*        break; */
/*    case TF_Eotf_DeGamma2_6: */
/*        return EOTF_DeGamma2_6; */
/*        break; */

/*    case TF_Oetf_Null: */
/*        return OETF_NULL; */
/*        break; */
    case TF_Oetf_PQ:
        return OETF_PQ;
        break;
    case TF_Oetf_HLG:
        return OETF_HLG;
        break;
    case TF_Oetf_170M:
        return OETF_170M;
        break;
    case TF_Oetf_sRGB:
        return OETF_sRGB;
        break;
/*    case TF_Oetf_Gamma1_8: */
/*        return OETF_Gamma1_8; */
/*        break; */
    case TF_Oetf_Gamma2_2:
        return OETF_Gamma2_2;
        break;
/*    case TF_Oetf_Gamma2_5: */
/*        return OETF_Gamma2_5; */
/*        break; */
/*    case TF_Oetf_Gamma2_6: */
/*        return OETF_Gamma2_6; */
/*        break; */

    default:
        ASSERT( TF_Linear <= typeTF && typeTF <= TF_END );
        break;
    }
    return NULL;
}

void TFName_MapFrom_Type(TypeTF typeTF, char *tfName)
{
    ASSERT(NULL != tfName);

    switch(typeTF)
    {
    case TF_Linear:
        strcpy(tfName, "TF_Linear");
        break;

    case TF_Eotf_PQ:
        strcpy(tfName, "Eotf_PQ");
        break;
    case TF_Eotf_HLG:
        strcpy(tfName, "Eotf_HLG");
        break;
    case TF_Eotf_170M:
        strcpy(tfName, "Eotf_170M");
        break;
    case TF_Eotf_sRGB:
        strcpy(tfName, "Eotf_sRGB");
        break;
/*    case TF_Eotf_DeGamma1_8: */
/*        strcpy(tfName, "Eotf_DeGamma1.8"); */
/*        break; */
    case TF_Eotf_DeGamma2_2:
        strcpy(tfName, "Eotf_DeGamma2.2");
        break;
/*    case TF_Eotf_DeGamma2_5: */
/*        strcpy(tfName, "Eotf_DeGamma2.5"); */
/*        break; */
/*    case TF_Eotf_DeGamma2_6: */
/*        strcpy(tfName, "Eotf_DeGamma2.6"); */
/*        break; */

/*    case TF_Oetf_Null: */
/*        strcpy(tfName, "Oetf_Null"); */
/*        break; */
    case TF_Oetf_PQ:
        strcpy(tfName, "Oetf_PQ");
        break;
    case TF_Oetf_HLG:
        strcpy(tfName, "Oetf_HLG");
        break;
    case TF_Oetf_170M:
        strcpy(tfName, "Oetf_170M");
        break;
    case TF_Oetf_sRGB:
        strcpy(tfName, "Oetf_sRGB");
        break;
/*    case TF_Oetf_Gamma1_8: */
/*        strcpy(tfName, "Oetf_Gamma1.8"); */
/*        break; */
    case TF_Oetf_Gamma2_2:
        strcpy(tfName, "Oetf_Gamma2.2");
        break;
/*    case TF_Oetf_Gamma2_5: */
/*        strcpy(tfName, "Oetf_Gamma2.5"); */
/*        break; */
/*    case TF_Oetf_Gamma2_6: */
/*        strcpy(tfName, "Oetf_Gamma2.6"); */
/*        break; */

    default:
        ASSERT( TF_Linear <= typeTF && typeTF <= TF_END );
        break;
    }
}

