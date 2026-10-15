#include "sw_base.hpp"

#include "bezier.h"

U32 factorial(U32 n)
{
    if (n > 1)
        return n * factorial(n-1);
    else
        return 1;
}

U32 BinomialCoef(U32 N, U32 k)
{
    U32 result = factorial(N) / ( factorial(k) * factorial(N-k) );

    return result;
}

double NBezier(const double *P, int lenP, double t)
{
    assert(0 <= t && t <= 1);
    assert(2 <= lenP);

    int N = lenP -1;
    double BDk=0;
    double BNt=0;
    int k=0;

    for(k=0; k < N+1; k++)
    {
        BDk = (double)BinomialCoef(N, k) * ( pow(t,k) * pow((1-t),(N-k)) );
        BNt = BNt + (BDk * P[k]);
    }

    return BNt;
}

double NBezier_deCasteljau(const double *P, int lenP, double t)
{
    assert(0 <= t && t <= 1);
    assert(2 <= lenP);

    double* temp = (double*)malloc(lenP * sizeof(double));
    assert(NULL != temp);

    memcpy(temp, P, lenP * sizeof(double));
    
    for(int k = 1; k < lenP; ++k)
        for (int i = 0; i < lenP - k; ++i)
        {
            temp[i] = (1 - t) * temp[i] + t * temp[i + 1];
        }

    double out = temp[0];
    free(temp);
    return out;
}

void Bezier_Wrapper(void* EBZParms, double* x, double* y, int len_x)
{
    assert(NULL != EBZParms);
    EBZCurveParameters* BezierParms = (EBZCurveParameters*)EBZParms;
    double Ks = BezierParms->Sx;
    double Kf = BezierParms->Sy;

    int lenP = BezierParms->order + 1;
    double *P = (double*)malloc(lenP * sizeof(double));
    P[0] = 0;
    P[lenP - 1] = 1.0;
    memcpy(P + 1, BezierParms->vPCoeff, (BezierParms->order - 1) * sizeof(double));

    for (int i = 0; i < len_x; ++i)
    {
        double s = x[i];

        double t = 0;
        double result = 0;

        if (0 == s)
            result = 0;
        else if (0 < s && s <= Ks)
            result = Kf / Ks * s;
        else if (Ks < s && s <= 1)
        {
            t = (s - Ks) / (1 - Ks);
            //result = Kf + (1 - Kf) * NBezier(P, lenP, t); 
            result = Kf + (1 - Kf) * NBezier_deCasteljau(P, lenP, t);
        }
        y[i] = result;
    }
    free(P);
    return;
}







