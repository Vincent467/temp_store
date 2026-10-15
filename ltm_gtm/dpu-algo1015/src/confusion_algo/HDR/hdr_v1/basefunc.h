#ifndef __BASEFUNC_H
#define __BASEFUNC_H

#ifdef __cplusplus
extern "C" {
#endif

int LoadCSV_Curve(
        const char * const filePath,
        double * const Curve, U32 *CurveNum);

int LoadCsv_TmConf(
        const char *filePath,
        U32 *Mode, double *Weight, double *Coef, double * const Curve, U32 *CurveNum);

int LoadCsv_CcmCoef(
        const char *filePath,
        double *RegCoef, double *RegOffset);

int LoadCsv_CscCoef(
        const char *filePath,
        double *RegCoef, U32 *RegPreOffset, U32 *RegPostOffset);

void GetStrSuffix(const char *str, char *suffix);

U64 max_U64(
    const U64 _left,
    const U64 _right
    );

S64 max_S64(
    const S64 _left,
    const S64 _right
    );

double max_double(
    const double _left,
    const double _right
    );

U64 min_U64(
    const U64 _left,
    const U64 _right
    );

S64 min_S64(
    const S64 _left,
    const S64 _right
    );

double min_double(
    const double _left,
    const double _right
    );

U32 maxArray_U32(
    const U32 * const array,
    const U32 size
    );

U32 maxArray_U64(
    const U64 * const array,
    const U32 size
    );

U32 maxArray_double(
    const double * const array,
    const U32 size
    );

U32 minArray_U32(
    const U32 * const array,
    const U32 size
    );

U32 minArray_U64(
    const U64 * const array,
    const U32 size
    );

U32 minArray_double(
    const double * const array,
    const U32 size
    );

int ExistInArray_U32(
    const U32 * const array,
    const U32 size,
    const U32 value
    );

U64 Clamp_U64(
    const U64 value,
    const U64 low,
    const U64 high);

S64 Clamp_S64(
    const S64 value,
    const S64 low,
    const S64 high);

double Clamp_double(
    double value,
    double low,
    double high);

U64 Lerp(
    const U64 y1,
    const U64 y2,
    const U64 weight);

U64 SHIFTRIGHTROUND(
    const U64 Data,
    const U32 Count);

U64 COMPUTEMASK(
    const U32 Position,
    const U32 Count
    );

U64 GETBITS(
    const U64 Data,
    const U32 Start,
    const U32 End
    );


#ifdef __cplusplus
}
#endif


#endif /* __BASEFUNC_H */
