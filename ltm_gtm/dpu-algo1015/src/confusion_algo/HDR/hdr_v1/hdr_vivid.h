#ifndef __HDR_VIVID_H__
#define __HDR_VIVID_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

    void getVividCurve(const Hdr_Algo_Config* Hdr_Config, double* x, double* y, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* __HDR_VIVID_H__ */
