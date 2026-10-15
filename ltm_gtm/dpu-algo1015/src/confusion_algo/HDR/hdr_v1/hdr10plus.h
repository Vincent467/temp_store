#ifndef __HDR10PLUS_H_
#define __HDR10PLUS_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

    void BasisOOTF(const LuminanceParameters* sceneLuminanceMeta, const float productPeak, const float sourceMaxL, 
        OUT EBZCurveParameters* sceneProductCurveParam);

    void GuidedOOTF(const LuminanceParameters* sceneLuminanceMeta, const EBZCurveParameters* sceneRefCurveParam, const float referencePeak,
        const float productPeak, const float sceneSourceMaxL, OUT EBZCurveParameters* sceneProductCurveParam);

#ifdef __cplusplus
}
#endif

#endif /* __WRAPPER_CURVE_HUNT_H */
