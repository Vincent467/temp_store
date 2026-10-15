#include "mobius.h"

void mobius(const double* x, double* result, int size, double j, int MaxCLL, int MaxDLL)
{
    double peak = (double)MaxCLL / MaxDLL;

    double j_powed = j * j;
    double a = -(j_powed*(peak - 1)) / (j_powed - 2 * j + peak);
    double b = (j_powed - 2 * j*peak + peak) / (peak - 1);
    double scale = (b*b + 2 * b*j + j_powed) / (b - a);

    for (int i = 0; i < size; ++i)
    {
        double x_scaled = x[i] *(10000.0/ (double)MaxCLL)* peak;

        if (x_scaled < j) {
            result[i] = x_scaled;
        }
        else {
            result[i] = scale * (x_scaled + a) / (x_scaled + b);
        }
        if (result[i] < 0) {
            result[i] = 0;
        }
        //else if (result[i] > 1) {
        //    result[i] = 1;
        //}
    }
    return;

}