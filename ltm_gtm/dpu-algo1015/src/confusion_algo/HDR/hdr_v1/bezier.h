#ifndef __BEZIER_H
#define __BEZIER_H

#ifdef __cplusplus
extern "C" {
#endif

U32 factorial(U32 n);
U32 BinomialCoef(U32 N, U32 k);
double NBezier(const double *P, int lenP, double t);
double NBezier_deCasteljau(const double *P, int lenP, double t);

void Bezier_Wrapper(void* EBZParms, double* x, double* y, int len_x);

#ifdef __cplusplus
}
#endif



#endif

