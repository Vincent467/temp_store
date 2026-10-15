#ifndef __TRANSFER_H
#define __TRANSFER_H

typedef enum _TransferFunction
{
    TF_Linear                  = 0,

    TF_Eotf_PQ                 = 1,
    TF_Eotf_HLG                = 2,
    TF_Eotf_170M               = 3,
    TF_Eotf_sRGB               = 4,
/*    TF_Eotf_DeGamma1_8         = 4, */
    TF_Eotf_DeGamma2_2         = 5,
/*    TF_Eotf_DeGamma2_5         = 6, */
/*    TF_Eotf_DeGamma2_6         = 7, */


    TF_Oetf_PQ                 = 11,
    TF_Oetf_HLG                = 12,
    TF_Oetf_170M               = 13,
    TF_Oetf_sRGB               = 14,
/*    TF_Oetf_Gamma1_8           = 14, */
    TF_Oetf_Gamma2_2           = 15,
/*    TF_Oetf_Gamma2_5           = 16, */
/*    TF_Oetf_Gamma2_6           = 17, */

    TF_END                     = 20
} TypeTF;


#ifdef __cplusplus
extern "C" {
#endif

double Transfer_Linear(double value);
double EOTF_NULL(double value);
double OETF_NULL(double value);

double EOTF_DeGamma1_8(double value);
double OETF_Gamma1_8(double value);

double EOTF_DeGamma2_2(double value);
double OETF_Gamma2_2(double value);

double EOTF_DeGamma2_5(double value);
double OETF_Gamma2_5(double value);

double EOTF_DeGamma2_6(double value);
double OETF_Gamma2_6(double value);

double EOTF_PQ(double value);
double OETF_PQ(double value);

double EOTF_170M(double value);
double OETF_170M(double value);

double EOTF_sRGB(double value);
double OETF_sRGB(double value);

double EOTF_HLG(double value);
double OETF_HLG(double value);

double ACESFilm(double x);
double ACESFilm_Amp(double value);

double OOTF_TM(const double * const OotfCurve, U32 OotfCurveNum, double value);

double (*TF_MapFrom_Type(TypeTF typeTF))(double);
void TFName_MapFrom_Type(TypeTF typeTF, char *tfName);


#ifdef __cplusplus
}
#endif



#endif /* __TRANSFER_H */

