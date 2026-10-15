#include "sw_base.hpp"
#include "hdr10plus.h"

#include "hdr_def.h"

typedef struct _BasisOOTF_Params
{
    //==============================
    // Knee-Point (KP) parameters
    //==============================
    // KP ramp base thresholds (two bounds KP # 1 and KP # 2 are computed)
    float SY1_V1;
    float SY1_V2;
    float SY1_T1;
    float SY1_T2;

    float SY2_V1;
    float SY2_V2;
    float SY2_T1;
    float SY2_T2;

    // KP mixing gain (final KP from bounds KP # 1 and KP # 2 as a function of scene percentile)
    float KP_G_V1;
    float KP_G_V2;
    float KP_G_T1;
    float KP_G_T2;

    //==============================
    // P coefficient parameters
    //==============================
    // Thresholds of minimum bound of P1 coefficient
    float P1_LIMIT_V1;
    float P1_LIMIT_V2;
    float P1_LIMIT_T1;
    float P1_LIMIT_T2;

    // Thresholds to compute relative shape of curve (P2~P9 coefficient) by pre-defined bounds - as a function of scene percentile
    float P2To9_T1;
    float P2To9_T2;

    // Defined relative shape bounds (P2~P9 coefficient) for a given maximum TM dynamic compression (eg : 20x )
    float P2ToP9_MAX1[ORDER - 2];
    float P2ToP9_MAX2[ORDER - 2];

    // Ps mixing gain (obtain all Ps coefficients) - as a function of TM dynamic compression ratio
    float PS_G_T1;
    float PS_G_T2;

    // Post-processing : Reduce P1/P2 (to enhance mid tone) for high TM dynamic range compression cases
    float LOW_SY_T1;
    float LOW_SY_T2;
    float LOW_K_T1;
    float LOW_K_T2;
    float RED_P1_V1;
    float RED_P1_T1;
    float RED_P1_T2;
    float RED_P2_V1;
    float RED_P2_T1;
    float RED_P2_T2;
}BasisOOTF_Params;


BasisOOTF_Params Default_BasisOOTF_Params()
{
    BasisOOTF_Params params;
    // KP ramp base thresholds (two bounds KP # 1 and KP # 2 are computed)
    params.SY1_V1 = 0.0;
    params.SY1_V2 = 0.3;
    params.SY1_T1 = 0.22;
    params.SY1_T2 = 1;
    params.SY2_V1 = 0;
    params.SY2_V2 = 0.2;
    params.SY2_T1 = 0.25;
    params.SY2_T2 = 0.95;

    // KP mixing gain (final KP from bounds KP # 1 and KP # 2 as a function of scene percentile)
    params.KP_G_V1 = 1;
    params.KP_G_V2 = 0.05;
    params.KP_G_T1 = 0.05;
    params.KP_G_T2 = 0.5;

    // Thresholds of minimum bound of P1 coefficient
    params.P1_LIMIT_V1 = 0.92;
    params.P1_LIMIT_V2 = 0.98;
    params.P1_LIMIT_T1 = 0.01;
    params.P1_LIMIT_T2 = 0.1;

    // Thresholds to compute relative shape of curve (P2~P9 coefficient) by pre-defined bounds - as a function of scene percentile
    params.P2To9_T1 = 0.05;
    params.P2To9_T2 = 0.55;
    float P2ToP9_MAX1_init[ORDER - 2] = { 0.5582, 0.6745, 0.7703, 0.8231, 0.8729, 0.9130, 0.9599, 0.9844 };
    float P2ToP9_MAX2_init[ORDER - 2] = { 0.4839, 0.6325, 0.7253, 0.7722, 0.8201, 0.8837, 0.9208, 0.9580 };
    for (int i = 0; i < (ORDER - 2); i++) {
        params.P2ToP9_MAX1[i] = P2ToP9_MAX1_init[i];
        params.P2ToP9_MAX2[i] = P2ToP9_MAX2_init[i];
    }

    // Ps mixing gain (obtain all Ps coefficients) - as a function of TM dynamic compression ratio
    params.PS_G_T1 = 0.125;
    params.PS_G_T2 = 0.95;

    // Post-processing : Reduce P1/P2 (to enhance mid tone) for high TM dynamic range compression cases
    params.LOW_SY_T1 = 0.005;
    params.LOW_SY_T2 = 0.04;
    params.LOW_K_T1 = 0.12;
    params.LOW_K_T2 = 0.4;
    params.RED_P1_V1 = 0.65;
    params.RED_P1_T1 = 0.1;
    params.RED_P1_T2 = 0.75;
    params.RED_P2_V1 = 0.8;
    params.RED_P2_T1 = 0.1;
    params.RED_P2_T2 = 0.75;
    return params;
}


void getPercentile_50_99(const LuminanceParameters* sceneLuminanceMeta, float* psll50, float* psll99)
{
    const int npsll = sceneLuminanceMeta->num_percentile;

    // Set output to -1 (invalid)
    *psll50 = -1;
    *psll99 = -1;

    // Find exact percentiles if provided , else interpolate
    float psll50_1 = -1, per50_1 = -1;
    float psll50_2 = -1, per50_2 = -1;
    float psll99_1 = -1, per99_1 = -1;
    float psll99_2 = -1, per99_2 = -1;

    // Search for exact percent index or bounds
    int curPercent = 0, prevPercent = 0;
    float curPsll = 0, prevPsll = 0;
    for (int i = 0; i < npsll; i++) {
        curPercent = sceneLuminanceMeta->vPercentage[i];
        curPsll = sceneLuminanceMeta->vPercentile[i];
        if (curPercent == 50) {
            *psll50 = curPsll;
        }
        else if (*psll50 == -1 && curPercent > 50 && prevPercent < 50) {
            per50_1 = prevPercent;
            per50_2 = curPercent;
            psll50_1 = prevPsll;
            psll50_2 = curPsll;
        }

        if (curPercent == 99) {
            *psll99 = curPsll;
        }
        else if (*psll99 == -1 && curPercent > 99 && prevPercent < 99) {
            per99_1 = prevPercent;
            per99_2 = curPercent;
            psll99_1 = prevPsll;
            psll99_2 = curPsll;
        }
        prevPercent = curPercent;
        prevPsll = curPsll;
    }

    // Interpolated 50% percentile luminance
    if (*psll50 == -1) {
        const float delta = MAX((per50_2 - per50_1), 1.0f);
        *psll50 = psll50_1 + (psll50_2 - psll50_1)*(50 - per50_1) / delta;
    }

    // Interpolated 99% percentile luminance
    if (*psll99 == -1) {
        const float delta = MAX((per99_2 - per99_1), 1.0f);
        *psll99 = psll99_1 + (psll99_2 - psll99_1)*(99.95f - per99_1) / delta;
    }
}

float rampWeight(IN const float v1, IN const float v2, IN const float t1, IN const float t2, IN const float t)
{
    float retVal = v1;
    if (t1 == t2) {
        retVal = (t < t1) ? (v1) : (v2);
    }
    else {
        if (t <= t1) {
            retVal = v1;
        }
        else if (t >= t2) {
            retVal = v2;
        }
        else {
            retVal = v1 + (v2 - v1) / (t2 - t1) * (t - t1);
        }
    }

    return retVal;
}


float calcP1(IN const float sx, IN const float sy, IN const float tgtL, IN const float calcMaxL,
    IN const float p1_limit_t1, IN const float p1_limit_t2, IN const float p1_limit_v1, IN const float p1_limit_v2,
    IN const float low_sy_t1, IN const float low_sy_t2, IN const float low_k_t1, IN const float low_k_t2,
    IN const float red_p1_t1, IN const float red_p1_t2, IN const float red_p1_v1,
    OUT float *p1_red_gain)
{

    const float ax = MIN(sx, 0.9999f);
    const float ay = MIN(sy, 0.9999f);

    const float k = tgtL / MAX(tgtL, calcMaxL);
    const float p1_limit = rampWeight(p1_limit_v2, p1_limit_v1, p1_limit_t1, p1_limit_t2, sy);
    float p1 = MAX(MIN(((1.0f - ax) / (ORDER*(1.0f - ay))) / k, p1_limit), 0.0f);

    // Reduce p1 to make more mid tone details for high compression TM case.
    const float low_sy_g = rampWeight(1.0, 0.0, low_sy_t1, low_sy_t2, sy);
    const float high_k_g = rampWeight(1.0, 0.0, low_k_t1, low_k_t2, k);
    const float high_tm_g = low_sy_g * high_k_g;
    const float red_p1 = rampWeight(1.0, red_p1_v1, red_p1_t1, red_p1_t2, high_tm_g);
    p1 = MIN(MAX(p1*red_p1, P1MIN), p1_limit);

    if (p1_red_gain != NULL) {
        *p1_red_gain = high_tm_g;
    }

    return p1;
}


void BasisOOTF(const LuminanceParameters* sceneLuminanceMeta, const float productPeak, const float sourceMaxL, OUT EBZCurveParameters* sceneProductCurveParam)
{
    const float tgtL = productPeak;
    const float srcL = MAX(tgtL, sourceMaxL);

    // <Algorithm  Parameters>
    BasisOOTF_Params params = Default_BasisOOTF_Params();  // setDefault() - sets to default
    // </Algorithm  Parameters>

    // KP - Knee Point
    float ctrL;
    if (sceneLuminanceMeta != NULL)
    {
        float psll50, psll99;
        getPercentile_50_99(sceneLuminanceMeta, &psll50, &psll99);

        ctrL = psll50 / MAX(psll99, 0.0001f);
    }
    else
    {
        ctrL = 0.25;
    }

    const float k = tgtL / srcL;
    //printf("basisOOTF precision: ctrL=%d,      k=%d\n", (int)(ctrL * 4095), (int)(k * 4095));
    //printf("basisOOTF precision: ctrL=%0.6f,      k=%0.6f\n", ctrL, k);
    const float sy1 = rampWeight(params.SY1_V1, params.SY1_V2, params.SY1_T1, params.SY1_T2, k);
    const float sy2 = rampWeight(params.SY2_V1, params.SY2_V2, params.SY2_T1, params.SY2_T2, k);
    const float kp_g = rampWeight(params.KP_G_V1, params.KP_G_V2, params.KP_G_T1, params.KP_G_T2, ctrL);
    const double sy = kp_g * sy1 + (1 - kp_g)*sy2;
    const double sx = sy * k;

    // P Coefficient
    float high_tm_g = 0;
    const float p1 = calcP1(sx, sy, tgtL, sourceMaxL,
        params.P1_LIMIT_T1, params.P1_LIMIT_T2, params.P1_LIMIT_V1, params.P1_LIMIT_V2,
        params.LOW_SY_T1, params.LOW_SY_T2, params.LOW_K_T1, params.LOW_K_T2,
        params.RED_P1_T1, params.RED_P1_T2, params.RED_P1_V1,
        &high_tm_g);
    //printf("basisOOTF precision: p1=(int) %d, (float) %0.6f\n\n", (int)(p1 * 4095), p1);

    float *ps2To9 = (float*)malloc((NPCOEFF - 1) * sizeof(float));
    for (int i = 0; i < (NPCOEFF - 1); i++) {
        const float g_p29 = rampWeight(params.P2ToP9_MAX2[i], params.P2ToP9_MAX1[i], params.P2To9_T1, params.P2To9_T2, ctrL);
        ps2To9[i] = g_p29 * params.P2ToP9_MAX1[i] + (1.0f - g_p29)*params.P2ToP9_MAX2[i];
        //printf("basisOOTF precision: g= %0.6f, ps2to9[%d] = %0.6f\n", g_p29, i, ps2To9[i]);
    }
    //printf("\n");


    const float ps_g = rampWeight(1, 0, params.PS_G_T1, params.PS_G_T2, k);

    double *pcoeff = (double*)malloc(NPCOEFF * sizeof(double));
    pcoeff[0] = p1;
    for (int i = 1; i < NPCOEFF; i++) {
        const double coeffi = i + 1;
        const double plin = coeffi / ORDER;
        pcoeff[i] = MAX(MIN(ps_g*ps2To9[i - 1] + (1.0 - ps_g)*plin, coeffi*p1), plin);
        //printf("basisOOTF precision: g= %0.6f, pcoeff[%d] = %0.6f\n", ps_g, i, pcoeff[i]);
    }
    const float red_p2 = rampWeight(1.0, params.RED_P2_V1, params.RED_P2_T1, params.RED_P2_T2, high_tm_g);
    pcoeff[1] = MAX(MIN(pcoeff[1] * red_p2, 2.0*p1), 2.0 / ORDER);
    //printf("\nbasisOOTF precision: g= %0.6f, recalc ps2to9[1] = %0.6f\n\n", red_p2, pcoeff[1]);
    set_BZParam_values(sceneProductCurveParam, sx, sy, pcoeff, ORDER, 1.0, 1.0);
    //printf("default P:\n");
    //for (int i = 0; i < ORDER - 1; i++)
        //printf("%2d: %0.4f\n", i, sceneProductCurveParam->vPCoeff[i]);

    free(ps2To9);
    free(pcoeff);
    return;
}

void GuidedOOTF(const LuminanceParameters* sceneLuminanceMeta, const EBZCurveParameters* sceneRefCurveParam, const float referencePeak, const float productPeak, const float sceneSourceMaxL, OUT EBZCurveParameters* sceneProductCurveParam)
{
    const float KP_AT_BYPASS = 0.3;
    BasisOOTF_Params params = Default_BasisOOTF_Params();

    const int order = sceneRefCurveParam->order;
    const int numP = MAX(order - 1, 1);
    const int T0 = referencePeak;
    const int T1 = productPeak;
    //const int T1 = 400;

    const float srcL0 = MAX(sceneSourceMaxL, (float)T0);
    const float srcL = MAX(sceneSourceMaxL, (float)T1);
    const float kmin = 200.0f / srcL;
    const float k0 = T0 / srcL0;
    const float k1 = T1 / srcL;
    const float kmax = 1;

    double *psLinear = (double*)malloc(numP * sizeof(double));
    for (int i = 0; i < numP; i++) {
        psLinear[i] = (double)(i + 1) / order;
        //printf("%d, %f\n", i, psLinear[i]);
    }

    // guided OOTF input,2018.08.30
   /* printf("\n================ guided OOTF input ========================== \n");
    printf("1. knee point                 : (%0.4f,%0.4f)\n", sceneRefCurveParam->Sy);
    printf("2. bezier  order              : %d\n", order);
    printf("3. bezier P coeffs            :\n");
    for (int i = 0; i < numP; i++)
        printf("        %2d : %0.6f\n", i, sceneRefCurveParam->vPCoeff[i]);
    printf("4. refence Peak               : %d nit\n", (int)referencePeak);
    printf("5. product Peak               : %d nit \n", (int)productPeak);
    printf("6. maxSourceL                 : %d nit \n", (int)sceneSourceMaxL);
    printf("7. percentile info            : \n", sceneRefCurveParam->Sy);
    int npsll = sceneLuminanceMeta->num_percentile;
    for (int i = 0; i < npsll; i++)
        printf("        %2d: %4.4f\n", sceneLuminanceMeta->vPercentage[i], sceneLuminanceMeta->vPercentile[i]);

    printf("\n*********************guided calculating... ******************************************* \n");
*/
    if (productPeak < 200.0f) {
        //printf("case: product < minL\n");
        //printf("Error GuidedOOTF : Product peak under 200 nits not supported at this time\n");
        set_BZParam_values(sceneProductCurveParam, 0.0, 0.0, psLinear, order, 1.0, 1.0);
        //for (int i = 0; i < order-1; i++)
        //    printf("%d, %f =%f\n", i, psLinear[i], sceneProductCurveParam->vPCoeff[i]);
        free(psLinear);

        return;
    }

    // Reference Peak = Product Peak
    if (T0 == T1) {
        memcpy(sceneProductCurveParam, sceneRefCurveParam, sizeof(EBZCurveParameters));
        //printf("case: ref=product\n ");
        //printf("do nothing....directly get from metadata.\n");
    }
    // Product Peak >= Scene Max (bypass curve)
    else if (T1 >= sceneSourceMaxL) {
        set_BZParam_values(sceneProductCurveParam, KP_AT_BYPASS, KP_AT_BYPASS, psLinear, order, 1.0, 1.0);

        //printf("case: product > maxL\n");
        //printf(".directly get from psLinear.\n");
    }
    // Typical tone mapping case
    else {
        // Product Peak < Reference Peak -> blend curve with highest TM range compression curve
        double *pscalc = (double*)malloc(numP * sizeof(double));

        double sx, sy;
        if (T1 < T0) {
            const float g = MAX(MIN((k1 - kmin) / (k0 - kmin), 1.0f), 0.0f);
            EBZCurveParameters minCurveParams;
            BasisOOTF(sceneLuminanceMeta, 200.0f, sceneSourceMaxL, &minCurveParams);
            //minCurveParams P need norm
            sy = MAX(MIN(g*sceneRefCurveParam->Sy + (1.0 - g)*minCurveParams.Sy, 1.0), 0.0);
            sx = sy * k1;
            //sceneProductCurveParam.setValues(sx, sy, pscalc, order, 1.0, 1.0);
            //minCurveParams.setValues(sx, sy, minCurveParams.getPCoeffAt(0), order, 1.0, 1.0);
            //printf("case: product < ref\n");
            //printf("pscalc[i]= g*sceneRefCurveParam.getPCoeffAt(i) + (1.0f-g)*minCurveParams.getPCoeffAt(i)\n");
            for (int i = 0; i < numP; i++) {
                pscalc[i] = g * sceneRefCurveParam->vPCoeff[i] + (1.0f - g)*minCurveParams.vPCoeff[i];
                //printf("p-1 => %2d: %f   = %f * %f + (1-%f)* %f \n", i, pscalc[i],
                    //g,
                    //sceneRefCurveParam->vPCoeff[i],
                    //g,
                    //minCurveParams.vPCoeff[i]);
            }
        }
        // Product Peak > Reference Peak -> blend curves with linear curve
        else {
            //printf("case: product > ref\n");
            const float g = MAX(MIN((k1 - k0) / (kmax - k0), 1.0f), 0.0f);
            sy = MAX(MIN(g*KP_AT_BYPASS + (1.0 - g)*sceneRefCurveParam->Sy, 1.0), 0.0);
            sx = sy * k1;
            for (int i = 0; i < numP; i++) {
                pscalc[i] = g * psLinear[i] + (1 - g)*sceneRefCurveParam->vPCoeff[i];
                //printf("p-1 => %2d: %f = %f * %f + (1-%f)* %f \n", i, pscalc[i],
                    //g,
                    //psLinear[i],
                    //g,
                    //sceneRefCurveParam->vPCoeff[i]);
            }
        }

        const float ps1 = calcP1(sx, sy, T1, sceneSourceMaxL,
            params.P1_LIMIT_T1, params.P1_LIMIT_T2, params.P1_LIMIT_V1, params.P1_LIMIT_V2,
            params.LOW_SY_T1, params.LOW_SY_T2, params.LOW_K_T1, params.LOW_K_T2,
            params.RED_P1_T1, params.RED_P1_T2, params.RED_P1_V1,
            NULL);

        pscalc[0] = ps1;  // Recalculate p1 to form smooth connection with knee-point slope
        //printf("p2 => %2d: %f\n", 0, ps1);
        for (int i = 1; i < numP; i++) {
            // Limit remaining P coefficients to form curve below CI line slope
            pscalc[i] = MIN(pscalc[i], (i + 1)*pscalc[0]);
            //printf("v2.3-2 => %2d: %f\n", i, pscalc[i]);
        }

        set_BZParam_values(sceneProductCurveParam, sx, sy, pscalc, order, 1.0, 1.0);

        free(pscalc);
    }

    //printf("\n================ guided OOTF output ========================== \n");
    //printf("1. knee point                 : (%0.4f,%0.4f)\n", sceneProductCurveParam->Sx, sceneProductCurveParam->Sx);
    //printf("2. bezier  order              : %d\n", order);
    //printf("3. bezier P coeffs            :\n");
    //for (int i = 0; i < order; i++)
    //    printf("        %2d: %0.4f\n", i, sceneProductCurveParam->vPCoeff[i]);
    //printf("\n============================================================== \n");

    free(psLinear);
}