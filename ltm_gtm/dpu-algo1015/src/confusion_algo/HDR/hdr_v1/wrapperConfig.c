#include "sw_base.hpp"

#include "transfer.h"
#include "curve_fitting.h"
#include "wrapperTm.h"
#include "hdr_def.h"
#include "hdr_vivid.h"
#include "curve_lut.h"

//#include "interp.h"
//#include "wrapperTm.h"
//#include "wrapperCurveHunt.h"
#include "basefunc.h"
//#include "hdr_def.h"


static U01 PRINT = 0;
static U01 fPRINT = 0;
static FILE *fLog = NULL;

typedef struct {
    uint32_t EotfCount;
    uint32_t EotfSteps[16];
    uint32_t EotfPoints[16];
    uint32_t corQuanY;
    double(*EotfCurve)(double);
}EotfConfig;

typedef struct {
    uint32_t OetfCount;
    uint32_t OetfSteps[16];
    uint32_t OetfPoints[16];
    uint32_t corQuanY;
    double(*OetfCurve)(double);
}OetfConfig;

//the bitwidths of the following corQuanY are all 24
EotfConfig eo_pq_config_12bit = {
    4,
    { 128, 64, 32, 16, 8 },
    { 2048, 2560, 3072, 3712 },
    38813,
    EOTF_PQ
};

EotfConfig eo_pq_config_10bit = {
    4,
    { 32, 16, 8, 4, 2},
    { 512, 640, 768, 928 },
    155929,
    EOTF_PQ
};

OetfConfig oe_pq_config_24bit_65entry = {
     8,
    { 16,  256,  512, 4096, 16384, 65536, 262144, 1048576, 2097152 },
    { 256,  512, 8192, 32768, 131072, 524288, 2097152, 4194304 },
    0,
    OETF_PQ
};

OetfConfig oe_pq_config_24bit_129entry = {
     9,
    { 4, 128,  256,  512, 4096, 16384, 65536, 131072, 524288, 1048576 },
    { 256,  512, 1024, 8192, 32768, 131072, 524288, 2097152, 4194304 },
    0,
    OETF_PQ
};

OetfConfig oe_pq_config_20bit_65entry = {
     8,
    { 1, 16, 32, 256, 1024, 4096, 16384, 65536, 131072 },
    { 16, 32, 512, 2048, 8192, 32768, 131072, 262144 },
    0,
    OETF_PQ
};

EotfConfig eo_hlg_config_12bit = {
    2,
    { 64, 32, 16},
    { 2048, 3072 },
    22139,
    EOTF_HLG
};

EotfConfig eo_hlg_config_10bit = {
    2,
    { 16, 8, 4},
    { 512, 768 },
    88800,
    EOTF_HLG
};

OetfConfig oe_hlg_config_24bit_65entry = {
    7,
    { 1024,  16384, 32768, 65536, 131072, 262144, 1048576, 2097152 },
    { 32768, 65536, 131072, 262144, 1048576, 4194304, 8388608 },
    0,
    OETF_HLG
};

OetfConfig oe_hlg_config_24bit_129entry = {
    7,
    { 256,  8192, 32768, 65536, 131072, 262144, 524288, 1048576 },
    { 16384,131072, 262144, 1048576, 2097152, 4194304, 8388608 },
    0,
    OETF_HLG
};

OetfConfig oe_hlg_config_20bit_65entry = {
    7,
    { 64,  1024, 2048, 4096, 8192, 16384, 65536, 131072 },
    { 2048,  4096, 8192, 16384, 65536, 262144, 524288 },
    0,
    OETF_HLG
};

EotfConfig eo_170m_config_12bit = {
    0,
    { 32},
    { 0},
    8249,
    EOTF_170M
};

EotfConfig eo_170m_config_10bit = {
    0,
    { 8},
    { 0},
    33035,
    EOTF_170M
};

OetfConfig oe_170m_config_24bit_65entry = {
    5,
    { 65536, 32768,   65536, 131072, 262144, 524288 },
    { 262144, 524288,   1048576, 2097152, 6291456 },
    0,
    OETF_170M
};

OetfConfig oe_170m_config_24bit_129entry = {
    5,
    { 65536, 4096,   65536, 131072, 262144, 524288 },
    { 262144, 524288,   1048576, 3145728, 8388608 },
    0,
    OETF_170M
};

OetfConfig oe_170m_config_20bit_65entry = {
    5,
    { 4096,  2048,  4096,   8192,    16384,    32768},
    { 16384,  32768,  65536,  131072,  393216},
    0,
    OETF_170M
};

EotfConfig eo_srgb_config_12bit = {
    0,
    { 32},
    { 0},
    9273,
    EOTF_sRGB
};

EotfConfig eo_srgb_config_10bit = {
    0,
    { 8},
    { 0},
    37139,
    EOTF_sRGB
};

OetfConfig oe_srgb_config_24bit_65entry = {
    6,
    { 16384,  32768,   65536,   131072,   262144,   524288,    1048576 },
    { 131072,  524288,  1048576,  2097152,  4194304,  12582912 },
    0,
    OETF_sRGB,
};

OetfConfig oe_srgb_config_24bit_129entry = {
    6,
    { 8192, 16384,  32768,   65536,   131072,   262144,   524288 },
    { 131072,  524288,  1048576,  2097152,  4194304,  12582912 },
    0,
    OETF_sRGB,
};

OetfConfig oe_srgb_config_20bit_65entry = {
    6,
    { 1024,  2048,   4096,   8192,   16384,   32768, 65536},
    { 8192,  32768,  65536,  131072,  262144,  786432},
    0,
    OETF_sRGB,
};

EotfConfig eo_degamma22_config_12bit = {
    0,
    { 32},
    { 0},
    8972,
    EOTF_DeGamma2_2
};

EotfConfig eo_degamma22_config_10bit = {
    0,
    { 8 },
    { 0},
    35932,
    EOTF_DeGamma2_2
};

OetfConfig oe_gamma22_config_24bit_65entry = {
    8,
    { 512,  4096, 8192, 65536,  131072, 262144, 524288, 1048576, 2097152 },
    { 8192, 16384, 131072, 524288, 1048576, 2097152, 8388608,12582912 },
    0,
    OETF_Gamma2_2
};

OetfConfig oe_gamma22_config_24bit_129entry = {
    7,
    { 128, 4096, 8192, 32768, 131072, 262144, 524288, 1048576 },
    { 8192, 16384, 131072, 524288, 2097152, 4194304, 8388608 },
    0,
    OETF_Gamma2_2
};

OetfConfig oe_gamma22_config_20bit_65entry = {
    7,
    { 64,  1024, 2048, 4096, 8192,  16384, 32768, 65536},
    { 2048,  4096,  8192, 16384, 65536, 131072, 262144, },
    0,
    OETF_Gamma2_2
};

EotfConfig eo_degamma26_config_12bit = {
    0,
    { 32},
    { 0},
    8972,
    EOTF_DeGamma2_6
};

EotfConfig eo_degamma26_config_10bit = {
    0,
    { 8 },
    { 0},
    35932,
    EOTF_DeGamma2_6
};

OetfConfig oe_gamma26_config_24bit_65entry = {
    7,
    { 1024,  16384, 32768, 65536,  131072, 262144, 524288, 1048576 },
    { 32768, 65536, 131072, 262144, 1048576, 2097152, 4194304 },
    0,
    OETF_Gamma2_6
};

OetfConfig oe_gamma26_config_24bit_129entry = {
    8,
    { 32, 512, 2048, 16384, 32768,  131072, 262144, 524288, 1048576 },
    { 2048, 4096, 32768, 131072, 524288, 1048576, 2097152, 8388608 },
    0,
    OETF_Gamma2_6
};

OetfConfig oe_gamma26_config_20bit_65entry = {
    7,
    { 64,  1024, 2048, 4096, 8192,  16384, 32768, 65536},
    { 2048,  4096,  8192, 16384, 65536, 131072, 262144, },
    0,
    OETF_Gamma2_6
};

// TODO This mactro may be wrong in line.
#define AQNUMBEROFBITS(Value) \
    ((Value) < 0x00000002) ?  1 : \
    ((Value) < 0x00000004) ?  2 : \
    ((Value) < 0x00000008) ?  3 : \
    ((Value) < 0x00000010) ?  4 : \
    ((Value) < 0x00000020) ?  5 : \
    ((Value) < 0x00000040) ?  6 : \
    ((Value) < 0x00000080) ?  7 : \
    ((Value) < 0x00000100) ?  8 : \
    ((Value) < 0x00000200) ?  9 : \
    ((Value) < 0x00000400) ? 10 : \
    ((Value) < 0x00000800) ? 11 : \
    ((Value) < 0x00001000) ? 12 : \
    ((Value) < 0x00002000) ? 13 : \
    ((Value) < 0x00004000) ? 14 : \
    ((Value) < 0x00008000) ? 15 : \
    ((Value) < 0x00010000) ? 16 : \
    ((Value) < 0x00020000) ? 17 : \
    ((Value) < 0x00040000) ? 18 : \
    ((Value) < 0x00080000) ? 19 : \
    ((Value) < 0x00100000) ? 20 : \
    ((Value) < 0x00200000) ? 21 : \
    ((Value) < 0x00400000) ? 22 : \
    ((Value) < 0x00800000) ? 23 : \
    ((Value) < 0x01000000) ? 24 : \
    ((Value) < 0x02000000) ? 25 : \
    ((Value) < 0x04000000) ? 26 : \
    ((Value) < 0x08000000) ? 27 : \
    ((Value) < 0x10000000) ? 28 : \
    ((Value) < 0x20000000) ? 29 : \
    ((Value) < 0x40000000) ? 30 : \
    ((Value) < 0x80000000) ? 31 : \
                             32


U32 LoadBin_HDR_Eotf(
    const char *filePath,
    U32 *RegCount, U32 *RegPoint, U32 *RegStep, U32 *RegData)
{
    FILE *LutBin = fopen(filePath, "rb");
    if (NULL == LutBin) { printf(" [%s:%d] '%s' open failed!\n", __FUNCTION__, __LINE__, filePath); exit(EXIT_FAILURE); }
    uint32_t i = 0;

    U32 Count = 0;
    U32 Point[16] = { 0 };
    U32 Step[16] = { 0 };
    U32 Entry = 0;

    fread(&Count, sizeof(U32), 1, LutBin);
    if (0 < Count) fread(Point, sizeof(U32), Count, LutBin);
    fread(Step, sizeof(U32), Count + 1, LutBin);

    U32 MaxEntry = 129;
    int nmemb = 0;
    for (i = 0; i < MaxEntry; i++)
    {
        nmemb = fread(RegData + i, sizeof(U32), 1, LutBin);
        if (1 != nmemb) break;
        Entry++;
    }

    //    printf(" Count:0x%x(%d)\n", Count, Count);
    //    printf(" Point[%d]: [ ", Count);
    //    for(i = 0; i < Count; i++)
    //        printf("%d, ", Point[i]);
    //    printf("]\n");
    //    printf(" Step[%d]: [ ", Count+1);
    //    for(i = 0; i < Count+1; i++)
    //        printf("%d, ", Step[i]);
    //    printf("]\n");

    *RegCount = Count;
    for (i = 0; i < Count; i++)
        RegPoint[i] = Point[i];
    for (i = 0; i < Count + 1; i++)
        RegStep[i] = Step[i];

    //    printf(" [Reg] RegCount: %d\n", *RegCount);
    //    printf("       Entry: %d\n", Entry);
    //    printf("       RegPoint[%d]: [ ", *RegCount);
    //    for(i = 0; i < *RegCount; i++)
    //        printf("%d, ", RegPoint[i]);
    //    printf("]\n");
    //    printf("       RegStep[%d]: [ ", *RegCount+1);
    //    for(i = 0; i < *RegCount+1; i++)
    //        printf("%d, ", RegStep[i]);
    //    printf("]\n");
    //    printf("       RegData[%d]: [ ",Entry);
    //    for(i = 0; i < Entry; i++)
    //        printf("%d, ", RegData[i]);
    //    printf("]\n");


    if (NULL != LutBin) { fclose(LutBin); LutBin = NULL; }

    return Entry;
}

void LoadBin_HDR_Ccm(
    const char *filePath,
    S32 *RegCoef, U32 *RegOffset)
{
    FILE *LutBin = fopen(filePath, "rb");
    if (NULL == LutBin) { printf(" [%s:%d] '%s' open failed!\n", __FUNCTION__, __LINE__, filePath); exit(EXIT_FAILURE); }
    uint32_t i = 0;

    fread(RegCoef, sizeof(U32), 9, LutBin);
    fread(RegOffset, sizeof(U32), 3, LutBin);

    //    printf(" [Reg] RegCoef[%d]: [ ", 9);
    //    for(i = 0; i < 9; i++)
    //        printf("%d, ", RegCoef[i]);
    //    printf("]\n");
    //    printf("       RegOffset[%d]: [ ", 3);
    //    for(i = 0; i < 3; i++)
    //        printf("%d, ", RegOffset[i]);
    //    printf("]\n");


    if (NULL != LutBin) { fclose(LutBin); LutBin = NULL; }
}

void LoadBin_HDR_Tm(
    const char *filePath,
    U32 *RegMode, U32 *RegWeight, U32 *RegCoef, U32 *RegPoint, U32 *RegData)
{
    FILE *LutBin = fopen(filePath, "rb");
    if (NULL == LutBin) { printf(" [%s:%d] '%s' open failed!\n", __FUNCTION__, __LINE__, filePath); exit(EXIT_FAILURE); }
    uint32_t i = 0;

    U32 Mode = 0;
    U32 Weight = 0;
    U32 Coef[3] = { 0 };

    fread(&Mode, sizeof(U32), 1, LutBin);
    fread(&Weight, sizeof(U32), 1, LutBin);
    fread(&Coef, sizeof(U32), 3, LutBin);
    fread(RegPoint, sizeof(U32), 33, LutBin);
    fread(RegData, sizeof(U32), 33, LutBin);

    //    printf(" Count:0x%x(%d)\n", Count, Count);
    //    printf(" Point[%d]: [ ", Count);
    //    for(i = 0; i < Count; i++)
    //        printf("%d, ", Point[i]);
    //    printf("]\n");
    //    printf(" Step[%d]: [ ", Count+1);
    //    for(i = 0; i < Count+1; i++)
    //        printf("%d, ", Step[i]);
    //    printf("]\n");

    *RegMode = Mode;
    *RegWeight = Weight;
    for (i = 0; i < 3; i++)
        RegCoef[i] = Coef[i];

    //    printf(" [Reg] RegMode:%d\n", *RegMode);
    //    printf("       RegWeight:%d\n", *RegWeight);
    //    printf("       RegCoef:[0:%d 1:%d 2:%d]\n", RegCoef[0], RegCoef[1], RegCoef[2]);
    //    printf("       RegPoint[%d]: [ ", 31);
    //    for(i=0; i < 31; i++)
    //        printf("%d, ", RegPoint[i]);
    //    printf("]\n");
    //    printf("       RegData[%d]: [ ", 33);
    //    for(i=0; i < 33; i++)
    //        printf("%d, ", RegData[i]);
    //    printf("]\n");

    if (NULL != LutBin) { fclose(LutBin); LutBin = NULL; }
}

U32 LoadBin_HDR_Oetf(
    const char *filePath,
    U32 *RegCount, U32 *RegPoint, U32 *RegStep, U32 *RegData)
{
    FILE *LutBin = fopen(filePath, "rb");
    if (NULL == LutBin) { printf(" [%s:%d] '%s' open failed!\n", __FUNCTION__, __LINE__, filePath); exit(EXIT_FAILURE); }
    uint32_t i = 0;

    U32 Count = 0;
    U32 Entry = 0;

    fread(&Count, sizeof(U32), 1, LutBin);
    if (0 < Count) fread(RegPoint, sizeof(U32), Count, LutBin);
    fread(RegStep, sizeof(U32), Count + 1, LutBin);

    U32 MaxEntry = 129;
    int nmemb = 0;
    for (i = 0; i < MaxEntry; i++)
    {
        nmemb = fread(RegData + i, sizeof(U32), 1, LutBin);
        if (1 != nmemb) break;
        Entry++;
    }

    //    printf(" Count:0x%x(%d)\n", Count, Count);
    //    printf(" Point[%d]: [ ", Count);
    //    for(i = 0; i < Count; i++)
    //        printf("%d, ", Point[i]);
    //    printf("]\n");
    //    printf(" Step[%d]: [ ", Count+1);
    //    for(i = 0; i < Count+1; i++)
    //        printf("%d, ", Step[i]);
    //    printf("]\n");

    *RegCount = Count;

    //    printf(" [Reg] RegCount: %d\n", *RegCount);
    //    printf("       Entry: %d\n", Entry);
    //    printf("       RegPoint[%d]: [ ", *RegCount);
    //    for(i = 0; i < *RegCount; i++)
    //        printf("%d, ", RegPoint[i]);
    //    printf("]\n");
    //    printf("       RegStep[%d]: [ ", *RegCount+1);
    //    for(i = 0; i < *RegCount+1; i++)
    //        printf("%d, ", RegStep[i]);
    //    printf("]\n");
    //    printf("       RegData[%d]: [ ",Entry);
    //    for(i = 0; i < Entry; i++)
    //        printf("%d, ", RegData[i]);
    //    printf("]\n");

    if (NULL != LutBin) { fclose(LutBin); LutBin = NULL; }

    return Entry;
}

void CurveFun1_BuildLut2Ys(
    IN double(*const CurveFun)(double), IN U32 quanX, IN U32 quanY,
    IN U32 LutLen, IN const U32 * const LutX, IN const U32 * const LutS,
    OUT U32 *LutNum, OUT U32 *LutY)
{
    ASSERT(NULL != CurveFun);
    ASSERT(0 < quanX);
    ASSERT(0 < quanY);
    ASSERT(0 < LutLen);
    ASSERT(NULL != LutX);
    ASSERT(NULL != LutS);
    ASSERT(NULL != LutNum);
    ASSERT(NULL != LutY);

    uint32_t i = 0, j = 0;
    U32 X = 0;
    double Xk = 0;
    U32 StepN = 0;
    U32 num = 0;

    X = LutX[0];
    Xk = (double)X / quanX;
    Xk = Clamp_double(Xk, (double)0, (double)1.0);
    LutY[0] = (U32)(CurveFun(Xk) * quanY + 0.5);
    num++;

    for (i = 0; i < LutLen - 1; i++)
    {
        if (3 == LutS[i]) continue;

        StepN = (LutX[i + 1] - LutX[i]) / LutS[i];

        for (j = 0; j < StepN; j++)
        {
            X = LutX[i] + (LutS[i] * (j + 1));
            Xk = (double)X / quanX;
            Xk = Clamp_double(Xk, (double)0, (double)1.0);
            LutY[num] = (U32)(CurveFun(Xk) * quanY + 0.5);
            num++;
        }
    }

    *LutNum = num;
}

U32 CurveFun1_BuildRegLut2Ys(
    IN double(*const CurveFun)(double),
    IN U32 quanX, IN U32 quanY,
    IN U32 inBit, IN U32 RegCount, IN const U32 * const RegPoint, IN const U32 * const RegStep,
    OUT U32 *RegData)
{
    uint32_t i = 0;
    U32 LutLen = RegCount + 2;
    U32 *LutX = (U32 *)malloc(sizeof(U32) * LutLen);
    if (NULL == LutX) { printf(" [%s:%d] LutX memory allocation failed!\n", __FUNCTION__, __LINE__); exit(EXIT_FAILURE); }
    LutX[0] = 0;
    for (i = 0; i < RegCount; i++)
        LutX[i + 1] = RegPoint[i];
    LutX[LutLen - 1] = 1 << inBit;

    U32 Entry;
    CurveFun1_BuildLut2Ys(
        CurveFun, quanX, quanY,
        LutLen, LutX, RegStep,
        &Entry, RegData);

    if (NULL != LutX) { free(LutX); LutX = NULL; }
    return Entry;
}

void fPrintLut2Table(
    IN FILE *logfile, IN const char * const name,
    IN U32 LutLen, IN const U32 * const LutX, IN const U32 * const LutS, IN const U32 * const LutY)
{
    ASSERT(NULL != name);
    ASSERT(NULL != LutX);
    ASSERT(NULL != LutS);
    ASSERT(NULL != LutY);

    uint32_t i = 0, j = 0;
    U32 StepN = 0;
    U32 X = 0;
    U32 num = 0;

    fprintf(logfile, "==================[ %s ]===============\n", name);
    if (0 < LutLen)
    {
        fprintf(logfile, " [index]: LutX --- LutY\n");
        fprintf(logfile, " [%d]: %d --- %d\n", 0, LutX[0], LutY[0]);
        num++;

        for (i = 0; i < LutLen - 1; i++)
        {
            if (3 == LutS[i]) continue;

            StepN = (LutX[i + 1] - LutX[i]) / LutS[i];

            for (j = 0; j < StepN; j++)
            {
                X = LutX[i] + (LutS[i] * (j + 1));
                fprintf(logfile, " [%d]: %d --- %d\n", num, X, LutY[num]);
                num++;
            }
        }
    }
    else
        fprintf(logfile, "None");

    fprintf(logfile, "---------------------------------\n");
}


void fPrintTable_Lut2Reg(
    IN FILE *logfile, IN const char * const name, IN U32 inBit,
    IN U32 RegCount, IN const U32 * const RegPoint, IN const U32 * const RegStep, IN const U32 * const RegData)
{
    U32 LutLen = RegCount + 2;
    U32 *LutX = (U32 *)malloc(sizeof(U32) * LutLen);
    if (NULL == LutX) { printf(" [%s:%d] LutX memory allocation failed!\n", __FUNCTION__, __LINE__); exit(EXIT_FAILURE); }
    uint32_t i = 0;

    LutX[0] = 0;
    for (i = 0; i < LutLen - 2; i++)
        LutX[i + 1] = RegPoint[i];
    LutX[LutLen - 1] = (1 << inBit);

    fPrintLut2Table(logfile, name, LutLen, LutX, RegStep, RegData);

    if (NULL != LutX) { free(LutX); LutX = NULL; }
}


void PrintLut2Table(
    IN const char * const name,
    IN U32 LutLen, IN const U32 * const LutX, IN const U32 * const LutS, IN const U32 * const LutY)
{
    ASSERT(NULL != name);
    ASSERT(NULL != LutX);
    ASSERT(NULL != LutS);
    ASSERT(NULL != LutY);

    uint32_t i = 0, j = 0;
    U32 StepN = 0;
    U32 X = 0;
    U32 num = 0;

    printf("==================[ %s ]===============\n", name);
    if (0 < LutLen)
    {
        printf(" [index]: LutX --- LutY\n");
        printf(" [%d]: %d --- %d\n", 0, LutX[0], LutY[0]);
        num++;

        for (i = 0; i < LutLen - 1; i++)
        {
            if (3 == LutS[i]) continue;

            StepN = (LutX[i + 1] - LutX[i]) / LutS[i];

            for (j = 0; j < StepN; j++)
            {
                X = LutX[i] + (LutS[i] * (j + 1));
                printf(" [%d]: %d --- %d\n", num, X, LutY[num]);
                num++;
            }
        }
    }
    else
        printf("None");

    printf("---------------------------------\n");
}

void PrintTable_Lut2Reg(
    IN const char * const name, IN U32 inBit,
    IN U32 RegCount, IN const U32 * const RegPoint, IN const U32 * const RegStep, IN const U32 * const RegData)
{
    U32 LutLen = RegCount + 2;
    U32 *LutX = (U32 *)malloc(sizeof(U32) * LutLen);
    if (NULL == LutX) { printf(" [%s:%d] LutX memory allocation failed!\n", __FUNCTION__, __LINE__); exit(EXIT_FAILURE); }
    uint32_t i = 0;

    LutX[0] = 0;
    LutX[LutLen - 1] = (1 << inBit);
    for (i = 0; i < LutLen - 2; i++)
        LutX[i + 1] = RegPoint[i];

    PrintLut2Table(name, LutLen, LutX, RegStep, RegData);

    if (NULL != LutX) { free(LutX); LutX = NULL; }
}



API_EXPORT void Set_FP16_Config(
    IN U16 MaxCLL,
    IN U01 NegativeFFP16,
    OUT SW_FP16_Config *FP16Config) {
    FP16Config->FP16ShiftBiasFlag = NegativeFFP16;
    if (NegativeFFP16)
        FP16Config->FP16ShiftBits = AQNUMBEROFBITS((MaxCLL + 39) / 80);
    else
        FP16Config->FP16ShiftBits = AQNUMBEROFBITS((MaxCLL - 1) / 80);
    // -1 because AQNUMROFBITS(x) -1 = Floor(Log2(x))
    FP16Config->FP16ShiftBits = FP16Config->FP16ShiftBits - 1 + 2;
}


API_EXPORT U32 Set_Eotfto24Config(
    IN U04 Switch,
    IN const char *ConfBin,
    IN U32 inBit,
    IN U32 outBit,
    OUT SW_EOTF_CONFIG* EO_Config)
{
    assert(Switch <= EO_END);

    if (EO_Disable == Switch)
    {
        if (PRINT) printf(" [Eotf] Disable\n");
        if (fPRINT) fprintf(fLog, " [Eotf] Disable\n");
        return 0;
    }
    else
    {
        if (PRINT)
        {
            printf(" [Eotf] Enable\n");
            printf(" Eotf_Switch: %d\n", Switch);
        }
        if (fPRINT)
        {
            fprintf(fLog, " [Eotf] Enable\n");
            fprintf(fLog, " Eotf_Switch: %d\n", Switch);
        }
    }

    U32 Entry = 0;
    uint32_t i = 0;

    assert(NULL != EO_Config);

    EO_Config->EotfSegmentPoint[0] = 0;
    U32 *RegCount = &(EO_Config->EotfSegmentCount);
    U32 *RegPoint = EO_Config->EotfSegmentPoint + 1;
    U32 *RegStep = EO_Config->EotfSegmentStep;
    U32 *RegData = EO_Config->EotfData;

    U32 inMaxValue = (1 << inBit) - 1;
    U32 outMaxValue = (1 << outBit) - 1;
    
    if (EO_ProgConf == Switch)
    {
        assert(NULL != ConfBin);

        if (PRINT)
        {
            printf(" CurveMode: ProgConf\n");
            printf(" ConfBin: %s\n", ConfBin);
        }
        if (fPRINT)
        {
            fprintf(fLog, " CurveMode: ProgConf\n");
            fprintf(fLog, " ConfBin: %s\n", ConfBin);
        }

        char suffix[4];
        GetStrSuffix(ConfBin, suffix);
        assert(!strcmp(suffix, "bin"));

        Entry = LoadBin_HDR_Eotf(
            ConfBin,
            RegCount, RegPoint, RegStep, RegData);
    }
    else
    {
        EotfConfig Eotf_config;
        switch (Switch)
        {
        case EO_PQ:
        {
            if (PRINT) printf(" (PQ)\n");
            if (fPRINT) fprintf(fLog, " (PQ)\n");
            if (inBit == 12)
                Eotf_config = eo_pq_config_12bit;
            else if (inBit == 10)
                Eotf_config = eo_pq_config_10bit;
        }
        break;

        case EO_HLG:
        {
            if (PRINT) printf(" (HLG)\n");
            if (fPRINT) fprintf(fLog, " (HLG)\n");

            if (inBit == 12)
                Eotf_config = eo_hlg_config_12bit;
            else if (inBit == 10)
                Eotf_config = eo_hlg_config_10bit;
        }
        break;

        case EO_170M:
        {
            if (PRINT) printf(" (170M)\n");
            if (fPRINT) fprintf(fLog, " (170M)\n");

            if (inBit == 12)
                Eotf_config = eo_170m_config_12bit;
            else if (inBit == 10)
                Eotf_config = eo_170m_config_10bit;
        }
        break;

        case EO_sRGB:
        {
            if (PRINT) printf(" (sRGB)\n");
            if (fPRINT) fprintf(fLog, " (sRGB)\n");

            if (inBit == 12)
                Eotf_config = eo_srgb_config_12bit;
            else if (inBit == 10)
                Eotf_config = eo_srgb_config_10bit;
        }
        break;

        case EO_DeGamma2_2:
        {
            if (PRINT) printf(" (DeGamma2.2)\n");
            if (fPRINT) fprintf(fLog, " (DeGamma2.2)\n");

            if (inBit == 12)
                Eotf_config = eo_degamma22_config_12bit;
            else if (inBit == 10)
                Eotf_config = eo_degamma22_config_10bit;
        }
        break;

        case EO_DeGamma2_6:
        {
            if (PRINT) printf(" (DeGamma2.6)\n");
            if (fPRINT) fprintf(fLog, " (DeGamma2.6)\n");

            if (inBit == 12)
                Eotf_config = eo_degamma26_config_12bit;
            else if (inBit == 10)
                Eotf_config = eo_degamma26_config_10bit;
        }
        break;

        default:
            printf("[%s(%d)] Undefined Switch(%d).\n", __FUNCTION__, __LINE__, Switch);
            exit(EXIT_FAILURE);
            break;
        }

        *RegCount = Eotf_config.EotfCount;
        for (i = 0; i < Eotf_config.EotfCount + 1; i++)
            RegStep[i] = Eotf_config.EotfSteps[i];
        for (i = 0; i < Eotf_config.EotfCount; i++)
            RegPoint[i] = Eotf_config.EotfPoints[i];

        //Entry = CurveFun1_BuildRegLut2Ys(
        //    Eotf_config.EotfCurve, inMaxValue, outMaxValue,
        //    inBit, *RegCount, RegPoint, RegStep,
        //    RegData);
        //RegData[Entry - 1] = outMaxValue + Eotf_config.corQuanY;

        Entry = generate_curve_lut(Eotf_config.EotfCurve, outMaxValue,
            RegPoint, RegStep,
            *RegCount, inBit,
            RegData,129);
        RegData[Entry - 1] = outMaxValue + Eotf_config.corQuanY;

        if (PRINT) printf("       EntryR: %d\n", Entry);
        if (fPRINT) fprintf(fLog, "       Entry: %d\n", Entry);
    }

    if (PRINT)
    {
        printf(" [Reg] Count: %d\n", *RegCount);
        printf("       Point: [");
        for (i = 0; i < *RegCount; i++)
            printf("%d, ", RegPoint[i]);
        printf("]\n");
        printf("       Step: [");
        for (i = 0; i < *RegCount + 1; i++)
            printf("%d, ", RegStep[i]);
        printf("]\n");
        printf("       Data: [");
        for (i = 0; i < Entry; i++)
            printf("%d, ", RegData[i]);
        printf("]\n");
        PrintTable_Lut2Reg("Eotf Lut", inBit, *RegCount, RegPoint, RegStep, RegData);
        printf("\n");
    }
    if (fPRINT)
    {
        fprintf(fLog, " [Reg] Count: %d\n", *RegCount);
        fprintf(fLog, "       Point: [");
        for (i = 0; i < *RegCount; i++)
            fprintf(fLog, "%d, ", RegPoint[i]);
        fprintf(fLog, "]\n");
        fprintf(fLog, "       Step: [");
        for (i = 0; i < *RegCount + 1; i++)
            fprintf(fLog, "%d, ", RegStep[i]);
        fprintf(fLog, "]\n");
        fprintf(fLog, "       Data: [");
        for (i = 0; i < Entry; i++)
            fprintf(fLog, "%d, ", RegData[i]);
        fprintf(fLog, "]\n");
        fPrintTable_Lut2Reg(fLog, "Eotf Lut", inBit, *RegCount, RegPoint, RegStep, RegData);
        fprintf(fLog, "\n");
    }

    return Entry;
}


API_EXPORT void Set_CcmConfig(
    IN U04 Switch,
    IN const char *ConfBin,
    IN U08 FP16_Enable,
    IN U16 MaxCLL,
    IN U05 FP16U24ShiftBits,
    IN U01 FP16U24ShiftBiasFlag,
    OUT SW_CGM_CONFIG* CCM_Config) {

    assert(CCM_Disable <= Switch && Switch <= CCM_END);

    // Notice that scRGB/sRGB/709 is using the same matrix
    const S19 Rec709toRec2020[9] = { 41117, 21581, 2838, 4529, 60260, 747, 1075, 5767, 58694 };
    const S19 Rec2020toRec709[9] = { 108823, -38509, -4778, -8166, 74246, -544, -1193, -6593, 73315 };
    const S19 Rec2020toP3[9] = { 88049, -18490, -4022, -4280, 70503, -687, 185, -1285, 66636 };
    const S19 P3tosRGB[9] = { 80275, -14739, 0, -2757, 68293, 0, -1287, -5155, 71978 };
    const S19 P3tosRec2020[9] = { 49405, 13014, 3117, 2998, 61720, 818, -79, 1154, 64462 };

    // Notice that scRGB is using the same matrix parameters as sRGB
    const S19 sRGBtoP3[9] = { 53903, 11633, 0, 2176, 63360, 0, 1120, 4746, 59670 };
    const S19 scRGBtosRGB[9] = { 65536, 0, 0, 0, 65536, 0, 0, 0, 65536 };
    const U25 Offset[3] = { 0, 0, 0 };

    S32 *RegCoef = CCM_Config->CGMCoeff;
    S32 *RegOffset = CCM_Config->CGMCoeffD;

    if (CCM_Disable == Switch)
    {
        if (PRINT) printf(" [CCM] Disable\n");
        if (fPRINT) fprintf(fLog, " [CCM] Disable\n");
        return;
    }
    else
    {
        if (PRINT)
        {
            printf(" [CCM] Enable\n");
            printf(" CCM_Switch: %d\n", Switch);
        }
        if (fPRINT)
        {
            fprintf(fLog, " [CCM] Enable\n");
            fprintf(fLog, " CCM_Switch: %d\n", Switch);
        }
    }

    uint32_t i = 0;
    if (CCM_ProgConf == Switch)
    {
        assert(NULL != ConfBin);

        if (PRINT)
        {
            printf(" CCM_Switch: ProgConf\n");
            printf(" ConfBin: %s\n", ConfBin);
        }
        if (fPRINT)
        {
            fprintf(fLog, " CCM_Switch: ProgConf\n");
            fprintf(fLog, " ConfBin: %s\n", ConfBin);
        }

        char suffix[4];
        GetStrSuffix(ConfBin, suffix);
        assert(!strcmp(suffix, "bin"));

        LoadBin_HDR_Ccm(
            ConfBin,
            RegCoef, RegOffset);
    }
    else
    {
        switch (Switch)
        {
        case CCM_Rec709toRec2020:
        {
            if (PRINT) printf(" (Rec709toRec2020)\n");
            if (fPRINT) fprintf(fLog, " (Rec709toRec2020)\n");
            for (i = 0; i < 9; i++)
                RegCoef[i] = Rec709toRec2020[i];
        }
        break;

        case CCM_Rec2020toRec709:
        {
            if (PRINT) printf(" (Rec2020toRec709)\n");
            if (fPRINT) fprintf(fLog, " (Rec2020toRec709)\n");
            for (i = 0; i < 9; i++)
                RegCoef[i] = Rec2020toRec709[i];
        }
        break;

        case CCM_Rec2020toP3:
        {
            if (PRINT) printf(" (Rec2020toP3)\n");
            if (fPRINT) fprintf(fLog, " (Rec2020toP3)\n");
            for (i = 0; i < 9; i++)
                RegCoef[i] = Rec2020toP3[i];
        }
        break;

        case CCM_P3tosRGB:
        {
            if (PRINT) printf(" (P3tosRGB)\n");
            if (fPRINT) fprintf(fLog, " (P3tosRGB)\n");
            for (i = 0; i < 9; i++)
                RegCoef[i] = P3tosRGB[i];
        }
        break;

        case CCM_P3toRec2020:
        {
            if (PRINT) printf(" (P3toRec2020)\n");
            if (fPRINT) fprintf(fLog, " (P3toRec2020)\n");
            for (i = 0; i < 9; i++)
                RegCoef[i] = P3tosRec2020[i];
        }
        break;

        case CCM_sRGBtoP3:
        {
            if (PRINT) printf(" (sRGBtoP3)\n");
            if (fPRINT) fprintf(fLog, " (sRGBtoP3)\n");
            for (i = 0; i < 9; i++)
                RegCoef[i] = sRGBtoP3[i];
        }
        break;

        case CCM_scRGBtosRGB:
        {
            if (PRINT) printf(" (scRGBtosRGB)\n");
            if (fPRINT) fprintf(fLog, " (scRGBtosRGB)\n");
            for (i = 0; i < 9; i++)
                RegCoef[i] = scRGBtosRGB[i];
        }
        break;

        default:
            printf("[%s(%d)] Undefined Switch(%d).\n", __FUNCTION__, __LINE__, Switch);
            exit(EXIT_FAILURE);
            break;
        }

        if (FP16_Enable)
        {
            //// FP16TODO

            //// MaxCLL / 80 = (1.Mantissa) << (Exponent-15)
            //// So if we want to get {1'b1, 10'b[Mantissa]} << (Exponent - ShiftBits)
            //// We can MaxCLL << 15 / 80 >> ShiftBits

            //U32 maxcllInFp16Num = ((U32)((((U64)MaxCLL << 25) / 80) >> FP16U24ShiftBits));
            //U32 negativeBiasInFp16Num = (U32)0x0400 << (14 - FP16U24ShiftBits);

            //U32 alpha;
            //U32 beta;
            //// alpha * maxcllInFp16Num = 1 << 24 - 1
            //// alpha * negativeBiasInFp16Num + (-beta) = 0
            //// else beta = 0

            //// U24.40 / U24 -> U24.40 -> U1.31
            //alpha = (U32)((((U64)((1 << 24) - 1) << 40) / maxcllInFp16Num) >> 9);
            //if (0 == FP16U24ShiftBiasFlag) {
            //    beta = 0;
            //}
            //else {
            //    // U24 * U1.31 = U25.31
            //    // Coeff is 
            //    beta = ((U64)(negativeBiasInFp16Num) * (U64)(alpha)) >> 31;
            //    beta = ~beta;
            //    ++beta;
            //}

            //for (i = 0; i < 9; i++)
            //    RegCoef[i] = (((S64)RegCoef[i]) * alpha) >> 31;

            //for (i = 0; i < 3; i++)
            //    RegOffset[i] = Offset[i] + beta;

			//newFP16S25
			//fp16 RGB U7.17+S
			const U32 maxFP16Num = 16384000U;
			U32 alpha = (U32)((((U64)0X00FFFFFFU<<31)+maxFP16Num/2)/maxFP16Num);
			for (i = 0; i < 9; i++)
			{
				RegCoef[i] = (((S64)RegCoef[i]) * alpha) >> 31;
			}
			for (i = 0; i < 3; i++) 
			{

				RegOffset[i] = Offset[i];
			}
        }
		//keep same as before
        else
        {
            for (i = 0; i < 3; i++)
                RegOffset[i] = Offset[i];
        }
    }

    if (PRINT)
    {
        printf(" [Reg] RegCoef: [");
        for (i = 0; i < 9; i++)
            printf("%d, ", RegCoef[i]);
        printf("]\n");
        printf("       RegOffset: [");
        for (i = 0; i < 3; i++)
            printf("%d, ", RegOffset[i]);
        printf("]\n");
        printf("\n");
    }
    if (fPRINT)
    {
        fprintf(fLog, " [Reg] RegCoef: [");
        for (i = 0; i < 9; i++)
            fprintf(fLog, "%d, ", RegCoef[i]);
        fprintf(fLog, "]\n");
        fprintf(fLog, "       RegOffset: [");
        for (i = 0; i < 3; i++)
            fprintf(fLog, "%d, ", RegOffset[i]);
        fprintf(fLog, "]\n");
        fprintf(fLog, "\n");
    }
}

API_EXPORT void Set_TmConfig(
    IN const Hdr_Algo_Config* Hdr_Config,
    OUT SW_TM_CONFIG* TM_parms)
{
    assert(NULL != Hdr_Config);
    assert(NULL != TM_parms);

    //R2Y
    TM_parms->TMMode = Hdr_Config->R2Y_Mode;
    TM_parms->TMWeight = Hdr_Config->Y_LinearWeight;
    U32* RegCoef = TM_parms->TMCoef;

    //Curve
    U32* RegPoint = TM_parms->TMSegmentPoint;
    U32* RegData = TM_parms->TMData;

    if (TM_ProgCurve == Hdr_Config->TM_Algo)
    {
        assert(NULL != Hdr_Config->TM_ConfBin);

        if (PRINT)
        {
            printf(" CurveMode: ProgConf\n");
            printf(" ConfBin: %s\n", Hdr_Config->TM_ConfBin);
        }
        if (fPRINT)
        {
            fprintf(fLog, " CurveMode: ProgConf\n");
            fprintf(fLog, " ConfBin: %s\n", Hdr_Config->TM_ConfBin);
        }

        char suffix[4];
        GetStrSuffix(Hdr_Config->TM_ConfBin, suffix);
        assert(!strcmp(suffix, "bin"));

        LoadBin_HDR_Tm(
            Hdr_Config->TM_ConfBin,
            &(TM_parms->TMMode),
            &(TM_parms->TMWeight),
            RegCoef, 
            RegPoint, RegData);
    }
    else
    {
        double x[TM_ALGO_CURVE_LEN] = { 0 };
        double y[TM_ALGO_CURVE_LEN] = { 0 };//0-1
        init_x(x, TM_ALGO_CURVE_LEN);
        float normalizerNit = HDR_MAX_LUMINANCE;

        if (Hdr_Config->Hdr_Standard == TM_STD_HDR_Vivid)
        {
            getVividCurve(Hdr_Config, x, y, TM_ALGO_CURVE_LEN);
        }
        else
        {
            tm_curve(Hdr_Config, x, y, &normalizerNit, TM_ALGO_CURVE_LEN);

            if (Hdr_Config->Hdr_Standard != TM_SDR_IN_HDR && 
                Hdr_Config->Hdr_Standard != TM_STD_HDR10Plus &&
                Hdr_Config->TM_Algo != TM_ALGO_Mobius && 
                Hdr_Config->TM_Algo != TM_ALGO_Hable &&
                Hdr_Config->TM_Algo != TM_ALGO_ReinHard &&
                Hdr_Config->Hdr_Standard != TM_STD_HLG
                )
                normalization_xy(normalizerNit, x, y, TM_ALGO_CURVE_LEN);

            if ((Hdr_Config->Hdr_Standard == TM_STD_HDR10 || Hdr_Config->Hdr_Standard == TM_STD_PQ) &&
                Hdr_Config->OE_mode == OE_PQ)
            {
                normalization_y(Hdr_Config->MaxDll, y, TM_ALGO_CURVE_LEN);
            }
        }

        double gain[TM_ALGO_CURVE_LEN] = { 0 };
        tm_gain(x, y, gain, TM_ALGO_CURVE_LEN);

        int pts_x[TM_TABLE_NUM] = { 0 };//0-1

        int lenp = TM_TABLE_NUM;
        int len_entry = 0;
        U32 temp_Step[1024] = { 0 };

        fit_params params = { 0 };
        params.x = x;
        params.y = y;
        params.gain = gain;
        params.len_y = TM_ALGO_CURVE_LEN;
        params.len_p = &lenp;//len_p include min&max
        params.pts_x = pts_x;
        params.pts_step = temp_Step;
        params.len_entry = &len_entry;//0:disable intper step
        params.fit_mode = 1;//0:value;1:gain
        params.fit_dichotomy = 1;

        curve_fitting(&params);

        uint32_t pointsbits = Hdr_Config->Linearbits;
        if (Hdr_Config->linearclipmode == 1)
        {
            pointsbits = 24;
            double k = ((float)(1 << 20)) / (1 << 24);
            for (uint32_t i = 0; i < TM_ALGO_CURVE_LEN; ++i)
            {
                gain[i] = gain[i] * k;
            }

        }
        curve_to_lut(x, gain, TM_ALGO_CURVE_LEN, pts_x, RegPoint, RegData, pointsbits, TM_TABLE_NUM);

        if (pointsbits > Hdr_Config->Linearbits)
        {
            for (uint32_t i = 0; i < TM_TABLE_NUM; ++i)
            {
                if (RegPoint[i] > (1 << Hdr_Config->Linearbits))
                    RegPoint[i] = 1 << Hdr_Config->Linearbits;
            }
        }
#if 0
        FILE* fp_tm_x = fopen("tm_x.txt", "w");
        FILE* fp_tm_y = fopen("tm_y.txt", "w");
        FILE* fp_gain = fopen("tm_gain.txt", "w");
        for (uint32_t i = 0; i < TM_ALGO_CURVE_LEN; ++i)
        {
            fprintf(fp_gain, "%.4f\n", gain[i]);
            fprintf(fp_tm_x, "%.4f\n", x[i]);
            fprintf(fp_tm_y, "%.4f\n", y[i]);

        }
        fclose(fp_gain);
        fclose(fp_tm_y);
        fclose(fp_tm_x);

        FILE* fp_tm_idx = fopen("tm_idx.txt", "w");
        for (uint32_t i = 0; i < TM_TABLE_NUM; ++i)
        {
            fprintf(fp_tm_idx, "%d\n", pts_x[i]);
        }
        fclose(fp_tm_idx);

#endif
        /*
        // Configure the Coeffs
        // coef: BT.601 RGB-->Y [0.299, 0.587, 0.114] --> [1225, 2404, 467]
        // coef: BT.709,sRGB RGB-->Y [0.2126, 0.7152, 0.0722] --> [871,  2929,  296] (default)
        // coef: BT.2020 RGB-->Y [0.2627, 0.678 , 0.0593] --> [1076, 2777,  243]
        // coef: DisplayP3 -->Y [ 0.21, 0.721, 0.069 ] --> [860, 2953, 283]
        */
        if (CS_Rec709 == Hdr_Config->R2Y_Gamut)
        {
            if (PRINT) printf("(RGBtoYUV_Rec709,sRGB)\n");
            if (fPRINT) fprintf(fLog, "(RGBtoYUV_Rec709,sRGB)\n");
            RegCoef[0] = 871;
            RegCoef[1] = 2929;
            RegCoef[2] = 296;
        }
        else if (CS_Rec2020 == Hdr_Config->R2Y_Gamut)
        {
            if (PRINT) printf("(RGBtoYUV_Rec2020)\n");
            if (fPRINT) fprintf(fLog, "(RGBtoYUV_Rec2020)\n");
            RegCoef[0] = 1076;
            RegCoef[1] = 2777;
            RegCoef[2] = 243;
        }
        else if (CS_P3 == Hdr_Config->R2Y_Gamut)
        {
            if (PRINT) printf("(RGBtoYUV_DisplayP3)\n");
            if (fPRINT) fprintf(fLog, "(RGBtoYUV_DisplayP3)\n");
            RegCoef[0] = 860;
            RegCoef[1] = 2953;
            RegCoef[2] = 283;
        }
        else if (CS_Rec601 == Hdr_Config->R2Y_Gamut)
        {
            if (PRINT) printf("(RGBtoYUV_Rec601)\n");
            if (fPRINT) fprintf(fLog, "(RGBtoYUV_Rec601)\n");
            RegCoef[0] = 1225;
            RegCoef[1] = 2404;
            RegCoef[2] = 467;
        }
        else
        {
            if (PRINT) printf("(RGBtoYUV_Rec709,sRGB)\n");
            if (fPRINT) fprintf(fLog, "(RGBtoYUV_Rec709,sRGB)\n");
            RegCoef[0] = 871;
            RegCoef[1] = 2929;
            RegCoef[2] = 296;
        }
    }

    return;
}


API_EXPORT U32 Set_OetfConfig(
    IN U04 Switch,
    IN const char *ConfBin,
    IN U32 inBit,
    IN U32 outBit,
    IN U32 entrymode,
    OUT SW_OETF_CONFIG* EO_Config)
{
    assert(OE_Disable <= Switch && Switch <= OE_END);

    if (OE_Disable == Switch)
    {
        if (PRINT) printf(" [Oetf] Disable\n");
        if (fPRINT) fprintf(fLog, " [Oetf] Disable\n");
        return 0;
    }
    else
    {
        if (PRINT)
        {
            printf(" [Oetf] Enable\n");
            printf(" Oetf_Switch: %d\n", Switch);
        }
        if (fPRINT)
        {
            fprintf(fLog, " [Oetf] Enable\n");
            fprintf(fLog, " Oetf_Switch: %d\n", Switch);
        }
    }

    U32 Entry = 0;
    uint32_t i = 0;

    assert(NULL != EO_Config);
    EO_Config->OetfSegmentPoint[0] = 0;
    U32 *RegCount = &(EO_Config->OetfSegmentCount);
    U32 *RegPoint = EO_Config->OetfSegmentPoint+1;
    U32 *RegStep = EO_Config->OetfSegmentStep;
    U32 *RegData = EO_Config->OetfData;

    U32 inMaxValue = (1 << inBit) - 1;
    U32 outMaxValue = (1 << outBit) - 1;
    U32 corQuanY = outMaxValue;

    if (OE_ProgConf == Switch)
    {
        assert(NULL != ConfBin);

        if (PRINT)
        {
            printf(" CurveMode: ProgConf\n");
            printf(" ConfBin: %s\n", ConfBin);
        }
        if (fPRINT)
        {
            fprintf(fLog, " CurveMode: ProgConf\n");
            fprintf(fLog, " ConfBin: %s\n", ConfBin);
        }

        char suffix[4];
        GetStrSuffix(ConfBin, suffix);
        assert(!strcmp(suffix, "bin"));

        Entry = LoadBin_HDR_Oetf(
            ConfBin,
            RegCount, RegPoint, RegStep, RegData);
    }
    else
    {
        OetfConfig OetfConfig;
        switch (Switch)
        {
        case OE_PQ:
        {
            if (PRINT) printf(" (PQ)\n");
            if (fPRINT) fprintf(fLog, " (PQ)\n");
            if (inBit == 24)
            {
                if (entrymode == 0)
                    OetfConfig = oe_pq_config_24bit_65entry;
                else if(entrymode == 1)
                    OetfConfig = oe_pq_config_24bit_129entry;
            }
            else if (inBit == 20)
                OetfConfig = oe_pq_config_20bit_65entry;
        }
        break;

        case OE_HLG:
        {
            if (PRINT) printf(" (HLG)\n");
            if (fPRINT) fprintf(fLog, " (HLG)\n");
            if (inBit == 24)
            {
                if (entrymode == 0)
                    OetfConfig = oe_hlg_config_24bit_65entry;
                else if (entrymode == 1)
                    OetfConfig = oe_hlg_config_24bit_129entry;
            }
            else if (inBit == 20)
                OetfConfig = oe_hlg_config_20bit_65entry;
        }
        break;

        case OE_170M:
        {
            if (PRINT) printf(" (170M)\n");
            if (fPRINT) fprintf(fLog, " (170M)\n");

            if (inBit == 24)
            {
                if (entrymode == 0)
                    OetfConfig = oe_170m_config_24bit_65entry;
                else if (entrymode == 1)
                    OetfConfig = oe_170m_config_24bit_129entry;
            }
            else if (inBit == 20)
                OetfConfig = oe_170m_config_20bit_65entry;
        }
        break;

        case OE_sRGB:
        {
            if (PRINT) printf(" (sRGB)\n");
            if (fPRINT) fprintf(fLog, " (sRGB)\n");

            if (inBit == 24)
            {
                if (entrymode == 0)
                    OetfConfig = oe_srgb_config_24bit_65entry;
                else if (entrymode == 1)
                    OetfConfig = oe_srgb_config_24bit_129entry;
            }
            else if (inBit == 20)
                OetfConfig = oe_srgb_config_20bit_65entry;
        }
        break;

        case OE_ReGamma2_2:
        {
            if (PRINT) printf(" (Gamma2.2)\n");
            if (fPRINT) fprintf(fLog, " (Gamma2.2)\n");
            if (inBit == 24)
            {
                if (entrymode == 0)
                    OetfConfig = oe_gamma22_config_24bit_65entry;
                else if (entrymode == 1)
                    OetfConfig = oe_gamma22_config_24bit_129entry;
            }
            else if (inBit == 20)
                OetfConfig = oe_gamma22_config_20bit_65entry;
        }
        break;

        case OE_ReGamma2_6:
        {
            if (PRINT) printf(" (Gamma2.6)\n");
            if (fPRINT) fprintf(fLog, " (Gamma2.6)\n");
            if (inBit == 24)
                if (inBit == 24)
                {
                    if (entrymode == 0)
                        OetfConfig = oe_gamma26_config_24bit_65entry;
                    else if (entrymode == 1)
                        OetfConfig = oe_gamma26_config_24bit_129entry;
                }
            else if (inBit == 20)
                OetfConfig = oe_gamma26_config_20bit_65entry;
        }
        break;

        default:
            printf("[%s(%d)] Undefined Switch(%d).\n", __FUNCTION__, __LINE__, Switch);
            exit(EXIT_FAILURE);
            break;
        }


        *RegCount = OetfConfig.OetfCount;
        for (i = 0; i < OetfConfig.OetfCount + 1; i++)
            RegStep[i] = OetfConfig.OetfSteps[i];
        for (i = 0; i < OetfConfig.OetfCount; i++)
            RegPoint[i] = OetfConfig.OetfPoints[i];

        //Entry = CurveFun1_BuildRegLut2Ys(
        //    OetfConfig.OetfCurve, inMaxValue, outMaxValue,
        //    inBit, *RegCount, RegPoint, RegStep,
        //    RegData);
        //RegData[Entry - 1] = outMaxValue + OetfConfig.corQuanY;

        Entry = generate_curve_lut(OetfConfig.OetfCurve, outMaxValue,
            RegPoint, RegStep,
            *RegCount, inBit,
            RegData, 129);
        RegData[Entry - 1] = outMaxValue + OetfConfig.corQuanY;

        if (PRINT) printf("       EntryR: %d\n", Entry);
        if (fPRINT) fprintf(fLog, "       Entry: %d\n", Entry);
    }

    if (PRINT)
    {
        printf(" [Reg] Count: %d\n", *RegCount);
        printf("       Point: [");
        for (i = 0; i < *RegCount; i++)
            printf("%d, ", RegPoint[i]);
        printf("]\n");
        printf("       Step: [");
        for (i = 0; i < *RegCount + 1; i++)
            printf("%d, ", RegStep[i]);
        printf("]\n");
        printf("       Data: [");
        for (i = 0; i < Entry; i++)
            printf("%d, ", RegData[i]);
        printf("]\n");
        PrintTable_Lut2Reg("Oetf Lut", inBit, *RegCount, RegPoint, RegStep, RegData);
        printf("\n");
    }
    if (fPRINT)
    {
        fprintf(fLog, " [Reg] Count: %d\n", *RegCount);
        fprintf(fLog, "       Point: [");
        for (i = 0; i < *RegCount; i++)
            fprintf(fLog, "%d, ", RegPoint[i]);
        fprintf(fLog, "]\n");
        fprintf(fLog, "       Step: [");
        for (i = 0; i < *RegCount + 1; i++)
            fprintf(fLog, "%d, ", RegStep[i]);
        fprintf(fLog, "]\n");
        fprintf(fLog, "       Data: [");
        for (i = 0; i < Entry; i++)
            fprintf(fLog, "%d, ", RegData[i]);
        fprintf(fLog, "]\n");
        fPrintTable_Lut2Reg(fLog, "Oetf Lut", inBit, *RegCount, RegPoint, RegStep, RegData);
        fprintf(fLog, "\n");
    }
    return Entry;
}


//***********HDR10+**************
API_EXPORT void set_BZParam_values(OUT EBZCurveParameters* BZParam,
    IN const double sx, IN const double sy, IN const double * ps, IN const int order,
    IN const double sNorm, IN const double pCoeffNorm)
{
    BZParam->Sx = sx / sNorm;
    BZParam->Sy = sy / sNorm;

    BZParam->order = order;
    if (order > 0) {
        const int ncoeff = order - 1;
        for (int i = 0; i < ncoeff; i++) {
            BZParam->vPCoeff[i] = ps[i] / pCoeffNorm;
        }
    }
    return;
}

API_EXPORT void set_Luminance_values(OUT LuminanceParameters* BZParam,
    IN const double avgL, IN const double maxR, IN const double maxG, IN const double maxB,
    IN const double * psll, IN const int * percent, IN const int order, IN const double lumNorm)
{
    BZParam->avgRGBLuminance = avgL / lumNorm;
    BZParam->maxRLuminance = maxR / lumNorm;
    BZParam->maxGLuminance = maxG / lumNorm;
    BZParam->maxBLuminance = maxB / lumNorm;
    BZParam->num_percentile = order;

    for (int i = 0; i < order; i++) {
        BZParam->vPercentage[i] = percent[i];
        BZParam->vPercentile[i] = psll[i] / lumNorm;
    }
    return;
}


Scene2094Metadata Typical_Scene2094Metadata() {
    Scene2094Metadata Hdr10Plus_typical_metadata;

    Hdr10Plus_typical_metadata.TargetedSystemDisplayMaximumLuminance = 400;
    Hdr10Plus_typical_metadata.vSceneEBZProvided = 1;

    //Bezier
    EBZCurveParameters BZParam;
    int Sx = 17;
    int Sy = 64;
    int order = 10;
    double vPCoeff[9] = { 265 ,666,741,800,848,887,920,945,957 };
    set_BZParam_values(&BZParam, Sx, Sy, vPCoeff, order, META_JSON_2094_EBZ_KNEE_POINT_MAX, META_JSON_2094_EBZ_PCOEFF_MAX);

    //Luminance
    LuminanceParameters LuminanceParam;
    int avgRGBLuminance = 4595;
    int maxRLuminance = 9900;
    int maxGLuminance = 9900;
    int maxBLuminance = 9900;
    int num_percentile = 10;
    int vPercentage[10] = { 0,5,10,25,50,75,90,95,98,99 };
    double vPercentile[10] = { 0,1,2,79,2537,9900,9901,9902,9903,9904 };
    set_Luminance_values(&LuminanceParam,avgRGBLuminance, maxRLuminance, maxGLuminance, maxBLuminance, 
        vPercentile, vPercentage, num_percentile, META_JSON_2094_100K_PERCENTILE_DIVIDE);

    Hdr10Plus_typical_metadata.vSceneEBZParam = BZParam;
    Hdr10Plus_typical_metadata.vSceneLuminanceParam = LuminanceParam;

    return Hdr10Plus_typical_metadata;
}

CuvaMetadata Typical_CuvaMetadata()
{
    //meta for Resource/hdr_vivid_source/20230718/Y2R_Bt2020L2F/frame_1391_out.bmp PQ to SDR100

    CuvaMetadata metadata;
    metadata.system_start_code = 1;
    metadata.minimum_maxrgb = 431;
    metadata.average_maxrgb = 2183;
    metadata.variance_maxrgb = 1607;
    metadata.maximum_maxrgb = 2987;
    metadata.tone_mapping_mode = 1;
    metadata.tone_mapping_param_num = 2;
    metadata.targeted_system_display_maximum_luminance[0] = 3078;
    metadata.targeted_system_display_maximum_luminance[1] = 2080;
    metadata.Base_flag[0] = 1;
    metadata.Base_flag[1] = 1;
    metadata.Base_flag[2] = 0;
    metadata.Base_flag[3] = 0;
    metadata.Base_param_m_p[0] = 5395;
    metadata.Base_param_m_p[1] = 6492;
    metadata.Base_param_m_m[0] = 24;
    metadata.Base_param_m_m[1] = 24;
    metadata.Base_param_m_a[0] = 1003;
    metadata.Base_param_m_a[1] = 644;
    metadata.Base_param_m_b[0] = 0;
    metadata.Base_param_m_b[1] = 0;
    metadata.Base_param_m_n[0] = 10;
    metadata.Base_param_m_n[1] = 10;
    metadata.Base_param_K1[0] = 1;
    metadata.Base_param_K1[1] = 1;
    metadata.Base_param_K2[0] = 1;
    metadata.Base_param_K2[1] = 1;
    metadata.Base_param_K3[0] = 1;
    metadata.Base_param_K3[1] = 1;
    metadata.base_param_Delta_mode[0] = 6;
    metadata.base_param_Delta_mode[1] = 6;
    metadata.base_param_Delta[0] = 38;
    metadata.base_param_Delta[1] = 6;

    metadata.P3Spline_flag[0] = 1;
    metadata.P3Spline_flag[1] = 1;
    metadata.P3Spline_num[0] = 2;
    metadata.P3Spline_num[1] = 2;
    const int P3Spline_TH_mode[2][4] = { {0,1,0,0},{0,2,0,0} };
    memcpy(metadata.P3Spline_TH_mode, P3Spline_TH_mode, sizeof(P3Spline_TH_mode));
    const int P3Spline_TH_MB[2][4] = { {176,0,0,0},{148,0,0,0} };
    memcpy(metadata.P3Spline_TH_MB, P3Spline_TH_MB, sizeof(P3Spline_TH_MB));
    const int P3Spline_TH[2][4][3] = { { {818,374,374},{2152,409,409},{0,0,0},{0,0,0} },{{818,204,204},{2152,387,387},{0,0,0},{0,0,0}} };
    memcpy(metadata.P3Spline_TH, P3Spline_TH, sizeof(P3Spline_TH));
    const int P3Spline_Strength[2][4] = { {83,112,0,0},{127,163,0,0} };
    memcpy(metadata.P3Spline_Strength, P3Spline_Strength, sizeof(P3Spline_Strength));

    metadata.color_saturation_mapping_flag = 1;
    metadata.color_saturation_num = 2;
    const int color_saturation_gain[16] = {0,24,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    memcpy(metadata.color_saturation_gain, color_saturation_gain, sizeof(color_saturation_gain));

    return metadata;
}

API_EXPORT U32 Initial_Hdr_Algo_Config_Vivid_PQtoSDR(
    OUT Hdr_Algo_Config* Hdr_Config)
{
    //config
    Hdr_Config->FP16_Enable = 0;
    Hdr_Config->DeMultiply = 1;
    Hdr_Config->EO_Enable = 1;
    Hdr_Config->CCM_Enable = 1;
    Hdr_Config->TM_Enable = 1;
    Hdr_Config->OE_Enable = 1;

    //bitwidths
    if (Hdr_Config->FP16_Enable)
        Hdr_Config->Inbits = 16;
    else
        Hdr_Config->Inbits = 12;
    Hdr_Config->Linearbits = 24;
    Hdr_Config->linearclipmode = 0;
    Hdr_Config->Outbits = 12;
    Hdr_Config->oetfentrymode = 0;


    //EOTF
    Hdr_Config->EO_mode = EO_PQ;
    Hdr_Config->EO_ConfBin = NULL;

    //CCM
    Hdr_Config->CCM_mode = CCM_Rec2020toRec709;
    Hdr_Config->CCM_ConfBin = NULL;

    //R2Y
    Hdr_Config->R2Y_Gamut = CS_Rec2020;
    Hdr_Config->Y_LinearWeight = 0;
    Hdr_Config->R2Y_Mode = 0;

    //Tone Mapping
    Hdr_Config->Hdr_Standard = TM_STD_HDR_Vivid;
    Hdr_Config->TM_Algo = TM_ALGO_Mobius;
    Hdr_Config->Linear_Mobius = 0.3;
    Hdr_Config->Contrast_Reinhard = 0.5;
    Hdr_Config->MaxCll = 1000;
    Hdr_Config->MaxDll = 100;
    Hdr_Config->MasterDisplay = 769 / 1023.0;
    Hdr_Config->TM_ConfBin = NULL;

    Hdr_Config->Hdr10Plus_Parms.autoCurce = 1;
    Hdr_Config->Hdr10Plus_Parms.usePsll99 = 1;
    Hdr_Config->Hdr10Plus_Parms.Hdr10Plus_metadata = Typical_Scene2094Metadata();
    Hdr_Config->Hdr10Plus_Parms.ProductTarget = -1;
    //productTarget:if not provided, set to "-1" which means product target = 2094-40 reference target(from json)

    Hdr_Config->Hdr_Vivid_Metadata = Typical_CuvaMetadata();
    Hdr_Config->Hdr_Vivid_Metadata_Exit = 1;

    //OETF
    Hdr_Config->OE_mode = OE_ReGamma2_2;
    Hdr_Config->OE_ConfBin = NULL;


    if (Hdr_Config->Hdr_Standard == TM_STD_HDR_Vivid)
    {
        if (Hdr_Config->Hdr_Vivid_Metadata_Exit == 1)
            Hdr_Config->R2Y_Mode = 1; //max rgb
        if (Hdr_Config->Hdr_Vivid_Metadata_Exit == 0)
            Hdr_Config->R2Y_Mode = 0; //luma
    }

    return 0;
}

API_EXPORT U32 Initial_Hdr_Algo_Config_HLGtoSDR(
    OUT Hdr_Algo_Config* Hdr_Config)
{
    //config
    Hdr_Config->FP16_Enable = 0;
    Hdr_Config->DeMultiply = 1;
    Hdr_Config->EO_Enable = 1;
    Hdr_Config->CCM_Enable = 1;
    Hdr_Config->TM_Enable = 1;
    Hdr_Config->OE_Enable = 1;

    //bitwidths
    if (Hdr_Config->FP16_Enable)
        Hdr_Config->Inbits = 16;
    else
        Hdr_Config->Inbits = 12;
    Hdr_Config->Linearbits = 24;
    Hdr_Config->linearclipmode = 0;
    Hdr_Config->Outbits = 12;
    Hdr_Config->oetfentrymode = 0;


    //EOTF
    Hdr_Config->EO_mode = EO_HLG;
    Hdr_Config->EO_ConfBin = NULL;

    //CCM
    Hdr_Config->CCM_mode = CCM_Rec2020toRec709;
    Hdr_Config->CCM_ConfBin = NULL;

    //R2Y
    Hdr_Config->R2Y_Gamut = CS_Rec2020;
    Hdr_Config->Y_LinearWeight = 0;
    Hdr_Config->R2Y_Mode = 1;

    //Tone Mapping
    Hdr_Config->Hdr_Standard = TM_STD_HDR_Vivid;//The Vivid standard includes a tonemapping method for HLG without metadata.
    Hdr_Config->TM_Algo = TM_ALGO_Mobius;
    Hdr_Config->Linear_Mobius = 0.3;
    Hdr_Config->Contrast_Reinhard = 0.5;
    Hdr_Config->MaxCll = 1000;
    Hdr_Config->MaxDll = 100;
    Hdr_Config->MasterDisplay = 769 / 1023.0;
    Hdr_Config->TM_ConfBin = NULL;

    Hdr_Config->Hdr10Plus_Parms.autoCurce = 1;
    Hdr_Config->Hdr10Plus_Parms.usePsll99 = 1;
    Hdr_Config->Hdr10Plus_Parms.Hdr10Plus_metadata = Typical_Scene2094Metadata();
    Hdr_Config->Hdr10Plus_Parms.ProductTarget = -1;
    //productTarget:if not provided, set to "-1" which means product target = 2094-40 reference target(from json)

    Hdr_Config->Hdr_Vivid_Metadata = Typical_CuvaMetadata();
    Hdr_Config->Hdr_Vivid_Metadata_Exit = 0;

    //OETF
    Hdr_Config->OE_mode = OE_ReGamma2_2;
    Hdr_Config->OE_ConfBin = NULL;


    if (Hdr_Config->Hdr_Standard == TM_STD_HDR_Vivid)
    {
        if (Hdr_Config->Hdr_Vivid_Metadata_Exit == 1)
            Hdr_Config->R2Y_Mode = 1; //max rgb
        if (Hdr_Config->Hdr_Vivid_Metadata_Exit == 0)
            Hdr_Config->R2Y_Mode = 0; //luma
    }

    return 0;
}

API_EXPORT U32 Initial_Hdr_Algo_Config_HDR10toSDR(
    OUT Hdr_Algo_Config* Hdr_Config)
{
    //config
    Hdr_Config->FP16_Enable = 0;
    Hdr_Config->DeMultiply = 1;
    Hdr_Config->EO_Enable = 1;
    Hdr_Config->CCM_Enable = 1;
    Hdr_Config->TM_Enable = 1;
    Hdr_Config->OE_Enable = 1;

    //bitwidths
    if (Hdr_Config->FP16_Enable)
        Hdr_Config->Inbits = 16;
    else
        Hdr_Config->Inbits = 12;
    Hdr_Config->Linearbits = 24;
    Hdr_Config->linearclipmode = 0;
    Hdr_Config->Outbits = 12;
    Hdr_Config->oetfentrymode = 0;


    //EOTF
    Hdr_Config->EO_mode = EO_PQ;
    Hdr_Config->EO_ConfBin = NULL;

    //CCM
    Hdr_Config->CCM_mode = CCM_Rec2020toRec709;
    Hdr_Config->CCM_ConfBin = NULL;

    //R2Y
    Hdr_Config->R2Y_Gamut = CS_Rec709;
    Hdr_Config->Y_LinearWeight = 0;
    Hdr_Config->R2Y_Mode = 1;

    //Tone Mapping
    Hdr_Config->Hdr_Standard = TM_STD_HDR10;
    Hdr_Config->TM_Algo = TM_ALGO_Mobius;
    Hdr_Config->Linear_Mobius = 0.3;
    Hdr_Config->Contrast_Reinhard = 0.5;
    Hdr_Config->MaxCll = 1000;
    Hdr_Config->MaxDll = 200;
    Hdr_Config->MasterDisplay = 769 / 1023.0;
    Hdr_Config->TM_ConfBin = NULL;

    Hdr_Config->Hdr10Plus_Parms.autoCurce = 1;
    Hdr_Config->Hdr10Plus_Parms.usePsll99 = 1;
    Hdr_Config->Hdr10Plus_Parms.Hdr10Plus_metadata = Typical_Scene2094Metadata();
    Hdr_Config->Hdr10Plus_Parms.ProductTarget = -1;
    //productTarget:if not provided, set to "-1" which means product target = 2094-40 reference target(from json)

    Hdr_Config->Hdr_Vivid_Metadata = Typical_CuvaMetadata();
    Hdr_Config->Hdr_Vivid_Metadata_Exit = 1;

    //OETF
    Hdr_Config->OE_mode = OE_ReGamma2_2;
    Hdr_Config->OE_ConfBin = NULL;


    if (Hdr_Config->Hdr_Standard == TM_STD_HDR_Vivid)
    {
        if (Hdr_Config->Hdr_Vivid_Metadata_Exit == 1)
            Hdr_Config->R2Y_Mode = 1; //max rgb
        if (Hdr_Config->Hdr_Vivid_Metadata_Exit == 0)
            Hdr_Config->R2Y_Mode = 0; //luma
    }

    return 0;
}

API_EXPORT U32 Initial_Hdr_Algo_Config_HDR10PlustoSDR(
    OUT Hdr_Algo_Config* Hdr_Config)
{
    //config
    Hdr_Config->FP16_Enable = 0;
    Hdr_Config->DeMultiply = 1;
    Hdr_Config->EO_Enable = 1;
    Hdr_Config->CCM_Enable = 1;
    Hdr_Config->TM_Enable = 1;
    Hdr_Config->OE_Enable = 1;

    //bitwidths
    if (Hdr_Config->FP16_Enable)
        Hdr_Config->Inbits = 16;
    else
        Hdr_Config->Inbits = 12;
    Hdr_Config->Linearbits = 24;
    Hdr_Config->linearclipmode = 0;
    Hdr_Config->Outbits = 12;
    Hdr_Config->oetfentrymode = 0;


    //EOTF
    Hdr_Config->EO_mode = EO_PQ;
    Hdr_Config->EO_ConfBin = NULL;

    //CCM
    Hdr_Config->CCM_mode = CCM_Rec2020toRec709;
    Hdr_Config->CCM_ConfBin = NULL;

    //R2Y
    Hdr_Config->R2Y_Gamut = CS_Rec709;
    Hdr_Config->Y_LinearWeight = 0;
    Hdr_Config->R2Y_Mode = 1;

    //Tone Mapping
    Hdr_Config->Hdr_Standard = TM_STD_HDR10Plus;
    Hdr_Config->TM_Algo = TM_ALGO_Mobius;
    Hdr_Config->Linear_Mobius = 0.3;
    Hdr_Config->Contrast_Reinhard = 0.5;
    Hdr_Config->MaxCll = 1000;
    Hdr_Config->MaxDll = 200;
    Hdr_Config->MasterDisplay = 769 / 1023.0;
    Hdr_Config->TM_ConfBin = NULL;

    Hdr_Config->Hdr10Plus_Parms.autoCurce = 1;
    Hdr_Config->Hdr10Plus_Parms.usePsll99 = 1;
    Hdr_Config->Hdr10Plus_Parms.Hdr10Plus_metadata = Typical_Scene2094Metadata();
    Hdr_Config->Hdr10Plus_Parms.ProductTarget = -1;
    //productTarget:if not provided, set to "-1" which means product target = 2094-40 reference target(from json)

    Hdr_Config->Hdr_Vivid_Metadata = Typical_CuvaMetadata();
    Hdr_Config->Hdr_Vivid_Metadata_Exit = 1;

    //OETF
    Hdr_Config->OE_mode = OE_ReGamma2_2;
    Hdr_Config->OE_ConfBin = NULL;


    if (Hdr_Config->Hdr_Standard == TM_STD_HDR_Vivid)
    {
        if (Hdr_Config->Hdr_Vivid_Metadata_Exit == 1)
            Hdr_Config->R2Y_Mode = 1; //max rgb
        if (Hdr_Config->Hdr_Vivid_Metadata_Exit == 0)
            Hdr_Config->R2Y_Mode = 0; //luma
    }

    return 0;
}

API_EXPORT U32 Initial_Hdr_Algo_Config_SDRinHDR(
    OUT Hdr_Algo_Config* Hdr_Config)
{
    //config
    Hdr_Config->FP16_Enable = 0;
    Hdr_Config->DeMultiply = 0;
    Hdr_Config->EO_Enable = 1;
    Hdr_Config->CCM_Enable = 1;
    Hdr_Config->TM_Enable = 1;
    Hdr_Config->OE_Enable = 1;

    //bitwidths
    Hdr_Config->Inbits = 12;
    Hdr_Config->Linearbits = 24;
    Hdr_Config->linearclipmode = 0;
    Hdr_Config->Outbits = 12;
    Hdr_Config->oetfentrymode = 0;


    //EOTF
    Hdr_Config->EO_mode = EO_sRGB;

    //CCM
    Hdr_Config->CCM_mode = CCM_Rec709toRec2020;

    //R2Y
    Hdr_Config->R2Y_Gamut = CS_Rec2020;
    Hdr_Config->Y_LinearWeight = 0;
    Hdr_Config->R2Y_Mode = 1;

    //Tone Mapping
    Hdr_Config->Hdr_Standard = TM_SDR_IN_HDR;
    Hdr_Config->MaxLuminanceforSDRinHDR = 400;

    //OETF
    Hdr_Config->OE_mode = OE_PQ;

    return 0;
}

API_EXPORT U32 Initial_Hdr_Algo_Config_HDRFP16(
	OUT Hdr_Algo_Config* Hdr_Config)
{
	//config
	Hdr_Config->FP16_Enable = 1;  //NEED ENABLE IN  THERE FP16
	Hdr_Config->DeMultiply = 0;
	Hdr_Config->EO_Enable = 0;  //while FP16==1 must be 0
	Hdr_Config->CCM_Enable = 1;
	Hdr_Config->TM_Enable = 1;
	Hdr_Config->OE_Enable = 1;

	//bitwidths
	if (Hdr_Config->FP16_Enable)
		Hdr_Config->Inbits = 16;
	else
		Hdr_Config->Inbits = 12;
	Hdr_Config->Linearbits = 24;
	Hdr_Config->linearclipmode = 0;
	Hdr_Config->Outbits = 12;
    Hdr_Config->oetfentrymode = 1;


	//EOTF
	Hdr_Config->EO_mode = EO_PQ;
	Hdr_Config->EO_ConfBin = NULL;

	//CCM
	//Hdr_Config->CCM_mode = CCM_Rec709toRec2020;
	Hdr_Config->CCM_mode = CCM_scRGBtosRGB;
	Hdr_Config->CCM_ConfBin = NULL;

    //R2Y
	Hdr_Config->R2Y_Gamut = CS_sRGB;
	Hdr_Config->Y_LinearWeight = 0;
	Hdr_Config->R2Y_Mode = 1;

	//Tone Mapping
	Hdr_Config->Hdr_Standard = TM_STD_HDR10;
	Hdr_Config->TM_Algo = TM_ALGO_Linear;
    Hdr_Config->Linear_Mobius = 0.3;
    Hdr_Config->Contrast_Reinhard = 0.5;
	Hdr_Config->MaxCll = 10000;
	Hdr_Config->MaxDll = 80;
	Hdr_Config->MasterDisplay = 769 / 1023.0;
	Hdr_Config->TM_ConfBin = NULL;

	Hdr_Config->Hdr10Plus_Parms.autoCurce = 1;
	Hdr_Config->Hdr10Plus_Parms.usePsll99 = 1;
	Hdr_Config->Hdr10Plus_Parms.Hdr10Plus_metadata = Typical_Scene2094Metadata();
	Hdr_Config->Hdr10Plus_Parms.ProductTarget = -1;
	//productTarget:if not provided, set to "-1" which means product target = 2094-40 reference target(from json)

	Hdr_Config->Hdr_Vivid_Metadata = Typical_CuvaMetadata();
	Hdr_Config->Hdr_Vivid_Metadata_Exit = 1;

	//OETF
	Hdr_Config->OE_mode = OE_sRGB;
	Hdr_Config->OE_ConfBin = NULL;


	if (Hdr_Config->Hdr_Standard == TM_STD_HDR_Vivid)
	{
		if (Hdr_Config->Hdr_Vivid_Metadata_Exit == 1)
			Hdr_Config->R2Y_Mode = 1; //max rgb
		if (Hdr_Config->Hdr_Vivid_Metadata_Exit == 0)
			Hdr_Config->R2Y_Mode = 0; //luma
	}

	return 0;
}

API_EXPORT U32 Initial_Hdr_Algo_Config_Core(OUT Hdr_Algo_Config* Hdr_Config)
{
    //config
    Hdr_Config->FP16_Enable = 0;
    Hdr_Config->DeMultiply = 0;
    Hdr_Config->EO_Enable = 1;
    Hdr_Config->CCM_Enable = 1;
    Hdr_Config->TM_Enable = 1;
    Hdr_Config->OE_Enable = 1;

    //bitwidths
    if (Hdr_Config->FP16_Enable)
        Hdr_Config->Inbits = 16;
    else
        Hdr_Config->Inbits = 10;
    Hdr_Config->Linearbits = 20;
    Hdr_Config->linearclipmode = 1;
    Hdr_Config->Outbits = 8;
    Hdr_Config->oetfentrymode = 0;


    //EOTF
    Hdr_Config->EO_mode = EO_HLG;
    Hdr_Config->EO_ConfBin = NULL;

    //CCM
    Hdr_Config->CCM_mode = CCM_Rec2020toRec709;
    Hdr_Config->CCM_ConfBin = NULL;

    //R2Y
    Hdr_Config->R2Y_Gamut = CS_Rec709;
    Hdr_Config->Y_LinearWeight = 0;
    Hdr_Config->R2Y_Mode = 0;

    //Tone Mapping
    Hdr_Config->Hdr_Standard = TM_STD_HDR_Vivid;
    Hdr_Config->TM_Algo = TM_ALGO_Mobius;
    Hdr_Config->Linear_Mobius = 0.3;
    Hdr_Config->Contrast_Reinhard = 0.5;
    Hdr_Config->MaxCll = 5000;
    Hdr_Config->MaxDll = 500;
    Hdr_Config->MasterDisplay = 769 / 1023.0;
    Hdr_Config->TM_ConfBin = NULL;

    Hdr_Config->Hdr10Plus_Parms.autoCurce = 1;
    Hdr_Config->Hdr10Plus_Parms.usePsll99 = 1;
    Hdr_Config->Hdr10Plus_Parms.Hdr10Plus_metadata = Typical_Scene2094Metadata();
    Hdr_Config->Hdr10Plus_Parms.ProductTarget = -1;
    //productTarget:if not provided, set to "-1" which means product target = 2094-40 reference target(from json)

    Hdr_Config->Hdr_Vivid_Metadata = Typical_CuvaMetadata();
    Hdr_Config->Hdr_Vivid_Metadata_Exit = 1;

    //OETF
    Hdr_Config->OE_mode = OE_sRGB;
    Hdr_Config->OE_ConfBin = NULL;


    if (Hdr_Config->Hdr_Standard == TM_STD_HDR_Vivid)
    {
        if(Hdr_Config->Hdr_Vivid_Metadata_Exit == 1)
            Hdr_Config->R2Y_Mode = 1; //max rgb
        if (Hdr_Config->Hdr_Vivid_Metadata_Exit == 0)
            Hdr_Config->R2Y_Mode = 0; //luma
    }



    return 0;
}

API_EXPORT U32 Initial_Hdr_Algo_Config(IN uint32_t HDR_Config_mode, OUT Hdr_Algo_Config* Hdr_Config)
{
    if (HDR_Config_mode == HDR_Vivid_PQtoSDR)
    {
        Initial_Hdr_Algo_Config_Vivid_PQtoSDR(Hdr_Config);
        Hdr_Config->Eotf_First = 0;
        Hdr_Config->TM_First = 1;
    }
    else if (HDR_Config_mode == HDR_HLGtoSDR)
    {
        Initial_Hdr_Algo_Config_HLGtoSDR(Hdr_Config);
        Hdr_Config->Eotf_First = 0;
        Hdr_Config->TM_First = 1;
    }
    else if (HDR_Config_mode == HDR_HDR10toSDR)
    {
        Initial_Hdr_Algo_Config_HDR10toSDR(Hdr_Config);
        Hdr_Config->Eotf_First = 0;
        Hdr_Config->TM_First = 0;
    }
    else if (HDR_Config_mode == HDR_HDR10PlustoSDR)
    {
        Initial_Hdr_Algo_Config_HDR10PlustoSDR(Hdr_Config);
        Hdr_Config->Eotf_First = 0;
        Hdr_Config->TM_First = 0;
    }
    else if (HDR_Config_mode == HDR_SDRinHDR)
    {
        Initial_Hdr_Algo_Config_SDRinHDR(Hdr_Config);
        Hdr_Config->Eotf_First = 0;
        Hdr_Config->TM_First = 0;
    }
	else if (HDR_Config_mode == HDR_HDRFP16)
	{
		Initial_Hdr_Algo_Config_HDRFP16(Hdr_Config);
		Hdr_Config->Eotf_First = 1;
		Hdr_Config->TM_First = 0;
	}
    else
    {
        Initial_Hdr_Algo_Config_Core(Hdr_Config);
        Hdr_Config->Eotf_First = 0;
        Hdr_Config->TM_First = 0;

        if (HDR_Config_mode == NanoUltra)
        {
            //bitwidths
            if (Hdr_Config->FP16_Enable)
                Hdr_Config->Inbits = 16;
            else
                Hdr_Config->Inbits = 10;
            Hdr_Config->Linearbits = 20;
            Hdr_Config->linearclipmode = 1;
            Hdr_Config->Outbits = 10;
        }
        else if (HDR_Config_mode == HDR_Typical_cfg)
        {
            //bitwidths
            if (Hdr_Config->FP16_Enable)
                Hdr_Config->Inbits = 16;
            else
                Hdr_Config->Inbits = 12;
            Hdr_Config->Linearbits = 24;
            Hdr_Config->linearclipmode = 0;
            Hdr_Config->Outbits = 24; //Hack because RTL.
        }
        else
        {
            LOGD("unsupported HDR_Config_mode\n");
            assert(0);
        }
    }
    return 0;
}

API_EXPORT U32 Set_Hdr_Config(
    IN Hdr_Algo_Config Hdr_Config,
    OUT SW_HDR_Registers* Hdr_parms)
{
    assert(Hdr_Config.Linearbits == 24);
	if (Hdr_Config.FP16_Enable == 0) {
		assert((Hdr_Config.Inbits == 10 || Hdr_Config.Inbits == 12));
	}
	else {
		assert((Hdr_Config.Inbits == 16)); // TODO V4 add 16
	}
	assert((Hdr_Config.Outbits == 24 || 
            Hdr_Config.Outbits == 12 || 
            Hdr_Config.Outbits == 10 || 
            Hdr_Config.Outbits == 8));

    Hdr_parms->Eotf_First = Hdr_Config.Eotf_First;
    Hdr_parms->TM_First = Hdr_Config.TM_First;

    Hdr_parms->EnableDeMultiply = Hdr_Config.DeMultiply;
    Hdr_parms->ExtendBitsMode = 0;
    Hdr_parms->ExtendBitsSeed = 0;
    uint8_t NegativeFFP16 = 1;
    if (Hdr_Config.FP16_Enable)
    {
        Hdr_parms->EnableFP16 = 1;
        Set_FP16_Config(Hdr_Config.MaxCll,
            NegativeFFP16,
            &(Hdr_parms->FP16U24Config));
    }

    if (Hdr_Config.EO_Enable)
    {
        uint8_t eo_outbits = Hdr_Config.Linearbits;
        if(Hdr_Config.linearclipmode == 1)
            eo_outbits = 24;
        Hdr_parms->EnableEotf = 1;
        Hdr_parms->EotfConfig.EotfEntry = Set_Eotfto24Config(Hdr_Config.EO_mode, Hdr_Config.EO_ConfBin,
            Hdr_Config.Inbits,
            eo_outbits,
            &(Hdr_parms->EotfConfig));
        if (eo_outbits > Hdr_Config.Linearbits)
        {
            for (uint32_t i = 0; i < Hdr_parms->EotfConfig.EotfEntry; ++i)
            {
                if (Hdr_parms->EotfConfig.EotfData[i] > ((1 << (Hdr_Config.Linearbits+1)) - 1))
                    Hdr_parms->EotfConfig.EotfData[i] = ((1 << (Hdr_Config.Linearbits+1)) - 1);
            }
        }
    }

    if (Hdr_Config.CCM_Enable)
    {
        Hdr_parms->EnableCGM = 1;
        Set_CcmConfig(
            Hdr_Config.CCM_mode,
            Hdr_Config.CCM_ConfBin,
            Hdr_Config.FP16_Enable,
            Hdr_Config.MaxCll,
            Hdr_parms->FP16U24Config.FP16ShiftBits,
            Hdr_parms->FP16U24Config.FP16ShiftBiasFlag,
            &(Hdr_parms->CGMConfig));
    }


    //ToneMapping Curve
    if (Hdr_Config.TM_Enable)
    {
        Hdr_parms->EnableTM = 1;
        Set_TmConfig(&Hdr_Config, &(Hdr_parms->TMConfig));
    }

    if (Hdr_Config.OE_Enable)
    {
        Hdr_parms->EnableOetf = 1;
        Hdr_parms->OetfConfig.OetfEntry = Set_OetfConfig(Hdr_Config.OE_mode, Hdr_Config.OE_ConfBin,
            Hdr_Config.Linearbits,
            Hdr_Config.Outbits,
            Hdr_Config.oetfentrymode,
            &(Hdr_parms->OetfConfig));
    }

    return 0;
}
