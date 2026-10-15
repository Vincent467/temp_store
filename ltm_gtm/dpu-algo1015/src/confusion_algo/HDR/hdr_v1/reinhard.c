#include "reinhard.h"

void reinhard(const double* x, double* result, int size, double contrast, int MaxCLL, int MaxDLL)
{
    double peak = (double)MaxCLL / MaxDLL;

    if (contrast <= 0)
        contrast = 1e-6;
    if (contrast >= 1)
        contrast = 0.99;

    double offset = (1.0 - contrast) / contrast;
    double scale = (peak + offset) / peak;

    for (int i = 0; i < size; ++i)
    {
        double x_scaled = x[i] * (10000.0 / (double)MaxCLL)* peak;
        x_scaled = x_scaled / (x_scaled + offset);
        result[i] = x_scaled* scale;
    }
    return;

}