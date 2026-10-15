#ifndef __REINHARD_H__
#define __REINHARD_H__

#ifdef __cplusplus
extern "C" {
#endif

    void reinhard(const double* x, double* result, int size, double contrast, int MaxCLL, int MaxDLL);
#ifdef __cplusplus
}
#endif
#endif