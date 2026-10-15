#include "hable.h"
#include <math.h>
#include <stddef.h>

/* Hable curve: (бн)-E/F */
static double hable_curve(double x)
{
    const double A = 0.15;
    const double B = 0.50;
    const double C = 0.10;
    const double D = 0.20;
    const double E = 0.02;
    const double F = 0.30;

    double num = x * (A * x + C * B) + D * E;
    double den = x * (A * x + B) + D * F;

    return (num / den) - (E / F);
}

void hable(const double* x, double* result, int size, int MaxCLL, int MAXDLL)
{
    const double MaxCLL_d = (double)MaxCLL;
    const double MAXDLL_d = (double)MAXDLL;

    /* peak = MaxCLL / MaxDLL */
    const double peak = MaxCLL_d / MAXDLL_d;

    /* scale = 1.0 / hable_curve(peak) */
    const double denom = hable_curve(peak);
    const double scale = 1.0 / denom;

    for (int i = 0; i < size; ++i)
    {
        /* x = x_in * peak */
        const double xi = x[i] * (10000.0 / (double)MaxCLL)* peak;

        /* result = scale * hable_curve(xi) */
        result[i] = scale * hable_curve(xi);
    }
}