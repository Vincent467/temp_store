#ifndef __HDR_DEF__
#define __HDR_DEF__

#define TM_GAIN_QUAN_BIT     16
#define HDR_MAX_LUMINANCE 10000

#define TM_ALGO_CURVE_BITS 12
#define TM_ALGO_CURVE_LEN ((1<<TM_ALGO_CURVE_BITS) + 1)

#define CURVE_MIN_NUM    1
#define CURVE_MAX_NUM    ((1<<14)+1)

#define TM_TABLE_NUM    33


// Fixed BasisOOTF / GudiedOOTF constants
#define ORDER   10
#define NPCOEFF (ORDER - 1)
#define P1MIN   (1.0f / ORDER)

#define META_JSON_2094_100K_PERCENTILE_DIVIDE    10.0
#define META_JSON_2094_EBZ_PCOEFF_BITS           10
#define META_JSON_2094_EBZ_PCOEFF_MAX            ((1 << META_JSON_2094_EBZ_PCOEFF_BITS)-1)
#define META_JSON_2094_EBZ_KNEE_POINT_BITS       12
#define META_JSON_2094_EBZ_KNEE_POINT_MAX        ((1 << META_JSON_2094_EBZ_KNEE_POINT_BITS)-1)


#endif // !__HDR_DEF__