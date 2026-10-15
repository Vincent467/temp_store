#include "sw_base.hpp"

static U01 PRINT = 0;
static U01 fPRINT = 0;
static FILE *fLog = NULL;

double OETF_Re_PQ(double value)
{
	assert(0 <= value && value <= 1);

	double m1 = (2610.0) / (4096.0 * 4.0);
	double m2 = (2523.0 * 128.0) / 4096.0;
	double c1 = (3424.0) / 4096.0;
	double c2 = (2413.0 * 32.0) / 4096.0;
	double c3 = (2392.0 * 32.0) / 4096.0;

	double out_value = 0;

	double tempValue = pow(value, m1);
	out_value = (double)(pow(((c2 * (tempValue)+c1) / (1.0 + c3 * (tempValue))), m2));

	return out_value;
}


double ReGamma_Clamp_double(
    double value,
    double low,
    double high)
{
    /* double result=0; */
    /* result = fmax(value, low); */
    /* result = fmin(result, high); */
    /* return result; */

    return fmin(fmax(value, low), high);
}

double OETF_Re_HLG(double value)
{
	assert(0 <= value && value <= 1);

	double a = 0.17883277f;
	double b = 1 - 4 * a;
	double c = (double)(0.5 - a * log(4 * a));

	double out_value = 0;

	if (0 <= value && value <= ((double)1.0 / 12.0))
		out_value = sqrt(3.0 * value);
	else
		out_value = a * log(12.0*value - b) + c;

	out_value = ReGamma_Clamp_double(out_value, 0, 1);

	return out_value;
}

double OETF_Re_170M(double value)
{
	assert(0 <= value && value <= 1);

	double result = 0;

	if (0 <= value && value < 0.018)    /* 0.0812 0821 */
	{
		result = value * 4.5;
	}
	else if (0.018 <= value && value <= 1)
	{
		result = 1.099 * pow(value, 0.45) - 0.099;
	}
	else
	{
		printf("[%s:%d] value(%f) error\'\n", __FUNCTION__, __LINE__, value);
		exit(EXIT_FAILURE);
	}

	return result;
}

double OETF_Re_sRGB(double value)
{
	assert(0 <= value && value <= 1);

	double result = 0;

	if (0 <= value && value < 0.0031308)
	{
		result = value * 12.92;
	}
	else if (0.0031308 <= value && value <= 1)
	{
		result = 1.055 * pow(value, (1.0 / 2.4)) - 0.055;
	}
	else
	{
		printf("[%s:%d] value(%f) error\'\n", __FUNCTION__, __LINE__, value);
		exit(EXIT_FAILURE);
	}

	return result;
}

double OETF_Re_Gamma2_2(double value)
{
	assert(0 <= value && value <= 1);

	return pow(value, 1.0 / 2.2);
}


double OETF_Re_Gamma2_6(double value)
{
    assert(0 <= value && value <= 1);

    return pow(value, 1.0 / 2.6);
}

U32 ReGamma_LoadBin_DReG(
	const char *filePath,
	U32 *RegCount, U32 *RegPoint, U32 *RegStep, U32 *RegData)
{
	FILE *LutBin = fopen(filePath, "rb");
	if (NULL == LutBin) { printf(" [%s:%d] '%s' open failed!\n", __FUNCTION__, __LINE__, filePath); exit(EXIT_FAILURE); }
	U32 i = 0;

    U32 Count = 0;
    U32 Point[16] = { 0 };
    U32 Step[16] = { 0 };
    U32 Data[257] = { 0 };
	U32 Entry = 0;

	fread(&Count, sizeof(U32), 1, LutBin);
    if (Count > 15) { printf(" [%s:%d] Error Count num in '%s'!\n", __FUNCTION__, __LINE__, filePath); fclose(LutBin); exit(EXIT_FAILURE); }
	if (0 < Count) fread(Point, sizeof(U32), Count, LutBin);
	fread(Step, sizeof(U32), Count + 1, LutBin);

	U32 MaxEntry = 257;
	int nmemb = 0;
	for (i = 0; i < MaxEntry; i++)
	{
		nmemb = fread(Data + i, sizeof(U32), 1, LutBin);
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
	for (i = 0; i < Entry; i++)
		RegData[i] = Data[i];

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

void ReGamma_CurveFun1_BuildLut2Ys(
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

	U32 i = 0, j = 0;
	U32 X = 0;
	double Xk = 0;
	U32 StepN = 0;
	U32 num = 0;

	X = LutX[0];
	Xk = (double)X / quanX;
	Xk = ReGamma_Clamp_double(Xk, (double)0, (double)1.0);
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
			Xk = ReGamma_Clamp_double(Xk, (double)0, (double)1.0);
			LutY[num] = (U32)(CurveFun(Xk) * quanY + 0.5);
			num++;
		}
	}

	*LutNum = num;
}

U32 ReGamma_CurveFun1_BuildRegLut2Ys(
	IN double(*const CurveFun)(double),
	IN U32 quanX, IN U32 quanY,
	IN U32 inBit, IN U32 RegCount, IN const U32 * const RegPoint, IN const U32 * const RegStep,
	OUT U32 *RegData)
{
	U32 i = 0;
	U32 LutLen = RegCount + 2;
	U32 *LutX = (U32 *)malloc(sizeof(U32) * LutLen);
    ASSERT(NULL != LutX);
	LutX[0] = 0;
	for (i = 0; i < RegCount; i++)
		LutX[i + 1] = RegPoint[i];
	LutX[LutLen - 1] = 1 << inBit;

	U32 Entry;
	ReGamma_CurveFun1_BuildLut2Ys(
		CurveFun, quanX, quanY,
		LutLen, LutX, RegStep,
		&Entry, RegData);

	if (NULL != LutX) { free(LutX); LutX = NULL; }
	return Entry;
}

void ReGamma_GetStrSuffix(const char *str, char *suffix)
{
	assert(NULL != str);
	assert(NULL != suffix);

	int strn = strlen(str) + 1;
	char *s = (char *)malloc(strn * sizeof(char));
	if (NULL == s) { printf(" memory allocation failed.\n"); return; }

	/* memcpy( s, str, strn * sizeof(char) ); */
	strcpy(s, str);

	int offset = strrchr(s, '.') - s;
	/* memcpy( suffix, s +offset +1, (strn -offset) * sizeof(char) ); */
	strcpy(suffix, s + offset + 1);

	/* printf(" str[%d]: %s\n", strn, str); */
	/* printf(" offset: %d\n", offset); */
	/* printf(" suffix: %s\n\n", suffix); */

	if (NULL != s) free(s);
}

void ReGamma_PrintLut2Table(
	IN const char * const name,
	IN U32 LutLen, IN const U32 * const LutX, IN const U32 * const LutS, IN const U32 * const LutY)
{
	ASSERT(NULL != name);
	ASSERT(NULL != LutX);
	ASSERT(NULL != LutS);
	ASSERT(NULL != LutY);

	U32 i = 0, j = 0;
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

API_EXPORT void ReGamma_PrintTable_Lut2Reg(
	IN const char * const name, IN U32 inBit,
	IN U32 RegCount, IN const U32 * const RegPoint, IN const U32 * const RegStep, IN const U32 * const RegData)
{
	U32 LutLen = RegCount + 2;
	U32 *LutX = (U32 *)malloc(sizeof(U32) * LutLen);
    ASSERT(NULL != LutX);

	U32 i = 0;

	LutX[0] = 0;
	LutX[LutLen - 1] = (1 << inBit);
	for (i = 0; i < LutLen - 2; i++)
		LutX[i + 1] = RegPoint[i];

	/*
		printf(" [ATB] LutX[%d]: [ ", LutLen);
		for(i = 0; i < LutLen; i++)
			printf("%5d, ", LutX[i]);
		printf("]\n");
		printf(" [ATB] LutS[%d]: [ ", LutLen-1);
		for(i = 0; i < LutLen-1; i++)
			printf("%5d, ", RegStep[i]);
		printf("]\n");
		printf("     ] LutY[%d]: [ ", LutLen);
		for(i = 0; i < LutLen; i++)
			printf("%5d, ", RegData[i]);
		printf("]\n");
	*/

	ReGamma_PrintLut2Table(name, LutLen, LutX, RegStep, RegData);

	if (NULL != LutX) { free(LutX); LutX = NULL; }
}

void ReGamma_fPrintLut2Table(
	IN FILE *logfile, IN const char * const name,
	IN U32 LutLen, IN const U32 * const LutX, IN const U32 * const LutS, IN const U32 * const LutY)
{
	ASSERT(NULL != name);
	ASSERT(NULL != LutX);
	ASSERT(NULL != LutS);
	ASSERT(NULL != LutY);

	U32 i = 0, j = 0;
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


API_EXPORT void ReGamma_fPrintTable_Lut2Reg(
    IN FILE *logfile, IN const char * const name, IN U32 inBit,
    IN U32 RegCount, IN const U32 * const RegPoint, IN const U32 * const RegStep, IN const U32 * const RegData)
{
    U32 LutLen = RegCount + 2;
    U32 *LutX = (U32 *)malloc(sizeof(U32) * LutLen);
    ASSERT(NULL != LutX);

    U32 i = 0;

    LutX[0] = 0;
    for (i = 0; i < LutLen - 2; i++)
        LutX[i + 1] = RegPoint[i];
    LutX[LutLen - 1] = (1 << inBit);

    ReGamma_fPrintLut2Table(logfile, name, LutLen, LutX, RegStep, RegData);

    if (NULL != LutX) { free(LutX); LutX = NULL; }
}

typedef struct {
    uint32_t ReGCount;
    uint32_t ReGSteps[16];
    uint32_t ReGPoints[16];
    uint32_t corQuanY;
    double(*ReGCurve)(double);
}RegammaConfig;

RegammaConfig ReG_pq_config_14bits_257entry = {
    8,
    { 1, 2, 4, 8, 16, 32, 64, 128, 256 },
    { 32, 64, 128, 256, 1024, 2048, 4096, 8192 },
    0,
    OETF_Re_PQ
};

RegammaConfig ReG_pq_config_12bits_257entry = {
    6,
    { 1, 2, 4, 8, 16, 32, 64},
    { 64, 128, 256, 512, 1024, 2048},
    0,
    OETF_Re_PQ
};

RegammaConfig ReG_pq_config_12to8bits_65entry = {
    8,
    { 1, 2, 4, 8, 16, 32, 64, 128, 256},
    { 8, 16, 32, 64, 256, 512, 1024, 2048},
    0,
    OETF_Re_PQ
};
RegammaConfig ReG_pq_config_10to8bits_129entry = {
    4,
    { 1, 2, 4, 8, 32},
    { 32, 64, 128, 512},
    0,
    OETF_Re_PQ
};
RegammaConfig ReG_pq_config_10to8bits_257entry = {
    3,
    { 1, 2, 4, 8},
    { 64, 128, 512},
    0,
    OETF_Re_PQ
};

RegammaConfig ReG_pq_config_8bits_65entry = {
    3,
     { 1, 2, 4, 8},
     { 16, 32, 128},
     0,
     OETF_Re_PQ
};

RegammaConfig ReG_pq_config_17to12bits_65entry = {
    8,
     { 8, 16, 32, 64, 256, 1024, 2048, 4096, 16384},
     { 64, 128, 256, 1024, 4096, 8192, 16384, 65536},
     0,
     OETF_Re_PQ
};

RegammaConfig ReG_pq_config_17to14bits_257entry = {
    8,
     { 1, 8, 16, 64, 256, 512, 1024, 2048, 4096},
     { 64, 128, 1024, 4096, 8192, 16384, 32768, 65536},
     0,
     OETF_Re_PQ
};

RegammaConfig ReG_hlg_config_14bits_257entry = {
    6,
    { 1, 4, 8, 16, 32, 128, 256 },
    { 32, 64, 256, 512, 4096, 8192 },
    0,
    OETF_Re_HLG
};

RegammaConfig ReG_hlg_config_12bits_257entry = {
    6,
    { 1, 2, 4, 8, 32, 64 },
    { 32, 64, 256, 1024, 2048 },
    0,
    OETF_Re_HLG
};

RegammaConfig ReG_hlg_config_12to8bits_65entry = {
    8,
    { 1, 2, 4, 8, 16, 32, 64, 128, 256},
    { 8, 16, 32, 64, 256, 512, 1024, 2048 },
    0,
    OETF_Re_HLG
};

RegammaConfig ReG_hlg_config_10to8bits_129entry = {
    4,
    { 1, 2, 4, 16, 32 },
    { 32, 64, 256, 512},
    0,
    OETF_Re_HLG
};
RegammaConfig ReG_hlg_config_10to8bits_257entry = {
    3,
    { 1, 2, 4, 16 },
    { 64, 256, 512},
    0,
    OETF_Re_HLG
};
RegammaConfig ReG_hlg_config_8bits_65entry = {
    3,
    { 1, 2, 4, 16 },
    { 16, 64, 128},
    0,
    OETF_Re_HLG
};

RegammaConfig ReG_hlg_config_17to12bits_65entry = {
    6,
    { 8, 64, 512, 1024, 2048, 4096, 16384 },
    { 128, 1024, 4096, 8192, 32768, 65536},
    0,
    OETF_Re_HLG
};

RegammaConfig ReG_hlg_config_17to14bits_257entry = {
    7,
    { 1, 32, 128, 256, 512, 1024, 2048, 4096 },
    { 128, 1024, 2048, 8192, 16384, 32768, 65536},
    0,
    OETF_Re_HLG
};

RegammaConfig ReG_170m_config_14bits_257entry = {
    6,
    { 32, 2, 16, 64, 128, 256, 512 },
    { 256, 512, 1024, 3072, 6144, 12288 },
    0,
    OETF_Re_170M
};

RegammaConfig ReG_170m_config_12bits_257entry = {
    6,
    { 32, 1, 4, 8, 16, 32, 64},
    { 64, 128,256, 512, 1024, 3072 },
    0,
    OETF_Re_170M
};

RegammaConfig ReG_170m_config_12to8bits_65entry = {
    5,
    { 16, 8, 16, 32, 64, 128},
    { 64, 128, 256, 512, 1536 },
    0,
    OETF_Re_170M
};

RegammaConfig ReG_170m_config_10to8bits_129entry = {
    5,
    { 8, 1, 2, 4, 8, 16},
    { 16, 32, 64, 128, 384 },
    0,
    OETF_Re_170M
};

RegammaConfig ReG_170m_config_10to8bits_257entry = {
    4,
    { 16, 1, 2, 4, 8},
    { 16, 64, 192, 384 },
    0,
    OETF_Re_170M
};
RegammaConfig ReG_170m_config_8bits_65entry = {
    4,
    { 4, 1, 2, 4, 8},
    { 4, 16, 48, 96},
    0,
    OETF_Re_170M
};
RegammaConfig ReG_170m_config_17to12bits_65entry = {
    5,
    { 512, 256, 512, 1024, 2048, 4096},
    { 2048, 4096, 8192, 16384, 49152},
    0,
    OETF_Re_170M
};
RegammaConfig ReG_170m_config_17to14bits_257entry = {
    5,
    { 256, 16, 256, 512, 1024, 2048},
    { 2048, 4096, 8192, 24576, 65536},
    0,
    OETF_Re_170M
};

RegammaConfig ReG_srgb_config_14bits_257entry = {
    5,
    {  4, 16, 32, 64, 128, 256 },
    { 256, 512, 2048, 6144, 12288 },
    0,
    OETF_Re_sRGB
};

RegammaConfig ReG_srgb_config_12bits_257entry = {
    6,
    {  1, 2, 4, 8, 16, 32, 64},
    { 32, 128, 256, 512, 1024, 3072},
    0,
    OETF_Re_sRGB
};

RegammaConfig ReG_srgb_config_12to8bits_65entry = {
    6,
    { 4, 8, 16, 32, 64, 128, 256},
    { 32, 128, 256, 512, 1024, 3072},
    0,
    OETF_Re_sRGB
};

RegammaConfig ReG_srgb_config_10to8bits_129entry = {
    5,
    {  1, 2, 4, 8, 16, 32},
    { 32, 64, 128, 384, 768},
    0,
    OETF_Re_sRGB
};

RegammaConfig ReG_srgb_config_10to8bits_257entry = {
    4,
    {  1, 2, 4, 8, 16},
    { 64, 256, 384, 768},
    0,
    OETF_Re_sRGB
};
RegammaConfig ReG_srgb_config_8bits_65entry = {
    4,
    {  1, 2, 4, 8, 16},
     { 16, 64, 96, 192},
     0,
     OETF_Re_sRGB
};
RegammaConfig ReG_srgb_config_17to12bits_65entry = {
    6,
    {  128, 256, 512, 1024, 2048, 4096, 8192},
     { 1024, 4096, 8192, 16384, 32768, 98304},
     0,
     OETF_Re_sRGB
};
RegammaConfig ReG_srgb_config_17to14bits_257entry = {
   6,
    {  32, 64, 128, 256, 512, 1024, 2048},
     { 1024, 4096, 8192, 16384, 32768, 98304},
     0,
     OETF_Re_sRGB
};

RegammaConfig ReG_2_2_config_14bits_257entry = {
    8,
    {  1, 2, 4, 8, 16, 32, 64, 128, 256 },
    { 32, 64, 128, 256, 1024, 2048, 4096, 8192 },
    0,
    OETF_Re_Gamma2_2
};

RegammaConfig ReG_2_2_config_12bits_257entry = {
    5,
    {  1, 2, 4, 8, 16, 32},
    { 32, 64, 256, 512, 1024},
    0,
    OETF_Re_Gamma2_2
};

RegammaConfig ReG_2_2_config_12to8bits_65entry = {
    8,
    { 1, 2, 4, 8, 16, 32, 64, 128, 256},
    { 8, 16, 32, 64, 256, 512, 1024, 2048},
    0,
    OETF_Re_Gamma2_2
};
RegammaConfig ReG_2_2_config_10to8bits_129entry = {
    4,
    { 1, 2, 4, 8, 16},
    { 32, 64, 128, 256},
    0,
    OETF_Re_Gamma2_2
};
RegammaConfig ReG_2_2_config_10to8bits_257entry = {
    3,
    { 1, 2, 4, 8},
    { 64, 128, 512},
    0,
    OETF_Re_Gamma2_2
};
RegammaConfig ReG_2_2_config_8bits_65entry = {
    3,
    { 1, 2, 4, 8},
    { 16, 32, 128},
    0,
    OETF_Re_Gamma2_2
};
RegammaConfig ReG_2_2_config_17to12bits_65entry = {
    8,
    { 4, 32, 64, 512, 1024, 2048, 4096, 8192, 16384},
    { 64, 128, 1024, 4096, 8192, 16384, 65536, 98304},
    0,
    OETF_Re_Gamma2_2
};
RegammaConfig ReG_2_2_config_17to14bits_257entry = {
    8,
    { 1, 8, 16, 128, 256, 512, 1024, 2048, 4096},
    { 64, 128, 1024, 4096, 8192, 16384, 65536, 98304},
    0,
    OETF_Re_Gamma2_2
};


RegammaConfig ReG_2_6_config_14bits_257entry = {
    8,
    {  1, 2, 4, 8, 16, 32, 64, 128, 256 },
    { 32, 64, 128, 256, 1024, 2048, 4096, 8192 },
    0,
    OETF_Re_Gamma2_6
};

RegammaConfig ReG_2_6_config_12bits_257entry = {
    6,
    {  1, 2, 4, 8, 16, 32, 64},
    { 64, 128, 256, 512, 1024, 2048},
    0,
    OETF_Re_Gamma2_6
};

RegammaConfig ReG_2_6_config_12to8bits_65entry = {
    8,
    { 1, 2, 4, 8, 16, 32, 64, 128, 256},
    { 8, 16, 32, 64, 256, 512, 1024, 2048},
    0,
    OETF_Re_Gamma2_6
};
RegammaConfig ReG_2_6_config_10to8bits_129entry = {
    4,
    { 1, 2, 4, 8, 16},
    { 32, 64, 128, 256},
    0,
    OETF_Re_Gamma2_6
};
RegammaConfig ReG_2_6_config_10to8bits_257entry = {
    3,
    { 1, 2, 4, 8},
    { 64, 128, 512},
    0,
    OETF_Re_Gamma2_6
};
RegammaConfig ReG_2_6_config_8bits_65entry = {
    3,
    { 1, 2, 4, 8},
    { 16, 32, 128},
    0,
    OETF_Re_Gamma2_6
};
RegammaConfig ReG_2_6_config_17to12bits_65entry = {
    9,
    { 1, 8, 16, 128, 512, 1024, 2048, 4096, 8192, 16384},
    { 16, 32, 256, 1024, 4096, 8192, 16384, 32768, 65536},
    0,
    OETF_Re_Gamma2_6
};
RegammaConfig ReG_2_6_config_17to14bits_257entry = {
    8,
    { 1, 2, 8, 32, 64, 256, 512, 1024, 2048},
    { 16, 32, 256, 1024, 4096, 8192, 32768, 65536},
    0,
    OETF_Re_Gamma2_6
};

API_EXPORT void Set_ReGConfig_core(
	IN U04 Switch,
    IN U32 BitMode,
    IN U32 LutMode,
    IN U32 EntryMode,
	IN const char *ConfBinR,
	IN const char *ConfBinG,
	IN const char *ConfBinB,
	OUT U32 *RegCountR, OUT U32 *RegPointR, OUT U32 *RegStepR, OUT U32 *RegDataR, OUT U32 *EntryR,
	OUT U32 *RegCountG, OUT U32 *RegPointG, OUT U32 *RegStepG, OUT U32 *RegDataG, OUT U32 *EntryG,
	OUT U32 *RegCountB, OUT U32 *RegPointB, OUT U32 *RegStepB, OUT U32 *RegDataB, OUT U32 *EntryB)
{
	assert(Switch <= ReGamma_Mode_END);
	assert(LutMode == SW_REGAMMA_LUTMODE_3CHANNEL || LutMode == SW_REGAMMA_LUTMODE_1CHANNEL);

	if (ReGamma_Mode_Disable == Switch)
	{
		if (PRINT) printf(" [ReG] Disable\n");
		if (fPRINT) fprintf(fLog, " [ReG] Disable\n");
		return;
	}
	else
	{
        if (PRINT)
        {
            printf(" [ReG] Enable\n");
            printf(" ReG_Switch: %d, LutMode: %d\n", Switch, LutMode);
        }
        if (fPRINT)
        {
            fprintf(fLog, " [ReG] Enable\n");
            fprintf(fLog, " ReG_Switch: %d, LutMode: %d\n", Switch, LutMode);
        }
	}

    U32 InBitWidth;
    U32 OutBitWidth;

    if (BitMode == ReGamma_SW_Mode_BIT14 || BitMode == ReGamma_SW_Mode_BIT14_ROI)
    {
        InBitWidth = 14;
        OutBitWidth = 14;
    }
    else if (BitMode == ReGamma_SW_Mode_BIT12)
    {
        InBitWidth = 12;
        OutBitWidth = 12;
    }
    else if (BitMode == ReGamma_SW_Mode_BIT12To8)
    {
        InBitWidth = 12;
        OutBitWidth = 8;
    }
    else if (BitMode == ReGamma_SW_Mode_BIT10To8)
    {
        InBitWidth = 10;
        OutBitWidth = 8;
    }
    else if (BitMode == ReGamma_SW_Mode_BIT17To12)
    {
        InBitWidth = 17;
        OutBitWidth = 12;
    }
    else if (BitMode == ReGamma_SW_Mode_BIT17To14)
    {
        InBitWidth = 17;
        OutBitWidth = 14;
    }
    else
    {
        LOGE("error ReGamma_BitMode\n");
        return;
    }

	U32 entryR = 0;
	U32 entryG = 0;
	U32 entryB = 0;
	U32 i = 0;

	assert(NULL != RegCountR);
	assert(NULL != RegPointR);
	assert(NULL != RegStepR);
	assert(NULL != RegDataR);
	assert(NULL != RegCountG);
	assert(NULL != RegPointG);
	assert(NULL != RegStepG);
	assert(NULL != RegDataG);
	assert(NULL != RegCountB);
	assert(NULL != RegPointB);
	assert(NULL != RegStepB);
	assert(NULL != RegDataB);

	U32 inMaxValue = (1 << InBitWidth) - 1;
	U32 outMaxValue = (1 << OutBitWidth) - 1;

	if (ReGamma_Mode_ProgConf == Switch)
	{
		assert(NULL != ConfBinR);
		if (LutMode == SW_REGAMMA_LUTMODE_3CHANNEL) {
			assert(NULL != ConfBinG);
			assert(NULL != ConfBinB);
		}

        if (PRINT)
        {
            printf(" CurveMode: ProgConf\n");
            printf(" ConfBinR: %s\n", ConfBinR);
            if (LutMode == SW_REGAMMA_LUTMODE_3CHANNEL) {
                printf(" ConfBinG: %s\n", ConfBinG);
                printf(" ConfBinB: %s\n", ConfBinB);
            }
        }
        if (fPRINT)
        {
            fprintf(fLog, " CurveMode: ProgConf\n");
            fprintf(fLog, " ConfBinR: %s\n", ConfBinR);
            if (LutMode == SW_REGAMMA_LUTMODE_3CHANNEL) {
                fprintf(fLog, " ConfBinG: %s\n", ConfBinG);
                fprintf(fLog, " ConfBinB: %s\n", ConfBinB);
            }
        }

		char suffixR[4] = { 0 };
		ReGamma_GetStrSuffix(ConfBinR, suffixR);
		assert(!strcmp(suffixR, "bin"));

		if (LutMode == SW_REGAMMA_LUTMODE_3CHANNEL) {
			char suffixG[4] = { 0 };
			ReGamma_GetStrSuffix(ConfBinG, suffixG);
			assert(!strcmp(suffixG, "bin"));
			char suffixB[4] = { 0 };
			ReGamma_GetStrSuffix(ConfBinB, suffixB);
			assert(!strcmp(suffixB, "bin"));
		}

		entryR = ReGamma_LoadBin_DReG(
			ConfBinR,
			RegCountR, RegPointR, RegStepR, RegDataR);

		if (LutMode == SW_REGAMMA_LUTMODE_1CHANNEL) {
			*RegCountG = *RegCountR;
			*RegCountB = *RegCountR;

			for (i = 0; i < *RegCountR + 1; i++) {
				RegStepG[i] = RegStepR[i];
				RegStepB[i] = RegStepR[i];
			}

			for (i = 0; i < *RegCountR; i++) {
				RegPointG[i] = RegPointR[i];
				RegPointB[i] = RegPointR[i];
			}

			entryG = entryR;
			entryB = entryR;
			for (i = 0; i < entryR; i++) {
				RegDataG[i] = RegDataR[i];
				RegDataB[i] = RegDataR[i];
			}
		} else {
			entryG = ReGamma_LoadBin_DReG(
				ConfBinG,
				RegCountG, RegPointG, RegStepG, RegDataG);
			entryB = ReGamma_LoadBin_DReG(
				ConfBinB,
				RegCountB, RegPointB, RegStepB, RegDataB);
		}
	}
	else
	{
		RegammaConfig ReGconfig;

		switch (Switch)
		{
		case ReGamma_Mode_ReG_PQ:
        {
            if (PRINT) printf(" (PQ)\n");
            if (fPRINT) fprintf(fLog, " (PQ)\n");

            if ((BitMode == ReGamma_SW_Mode_BIT14 || BitMode == ReGamma_SW_Mode_BIT14_ROI) && EntryMode == ReGamma_SW_Entry_257)
                ReGconfig = ReG_pq_config_14bits_257entry;
            else if ((BitMode == ReGamma_SW_Mode_BIT12) && EntryMode == ReGamma_SW_Entry_257)
                ReGconfig = ReG_pq_config_12bits_257entry;
            else if ((BitMode == ReGamma_SW_Mode_BIT12To8) && EntryMode == ReGamma_SW_Entry_65)
                ReGconfig = ReG_pq_config_12to8bits_65entry;
            else if ((BitMode == ReGamma_SW_Mode_BIT10To8) && EntryMode == ReGamma_SW_Entry_129)
                ReGconfig = ReG_pq_config_10to8bits_129entry;
            else if ((BitMode == ReGamma_SW_Mode_BIT10To8) && EntryMode == ReGamma_SW_Entry_257)
                ReGconfig = ReG_pq_config_10to8bits_257entry;
            else if ((BitMode == ReGamma_SW_Mode_BIT8) && EntryMode == ReGamma_SW_Entry_65)
                ReGconfig = ReG_pq_config_8bits_65entry;
            else if ((BitMode == ReGamma_SW_Mode_BIT17To12) && EntryMode == ReGamma_SW_Entry_65)
                ReGconfig = ReG_pq_config_17to12bits_65entry;
            else if ((BitMode == ReGamma_SW_Mode_BIT17To14) && EntryMode == ReGamma_SW_Entry_257)
                ReGconfig = ReG_pq_config_17to14bits_257entry;
            else
            {
                LOGE("unsupported combination of mode and entry\n");
                assert(0);
            }

        }
        break;
        
		case ReGamma_Mode_ReG_HLG:
        {
            if (PRINT) printf(" (HLG)\n");
            if (fPRINT) fprintf(fLog, " (HLG)\n");

            if ((BitMode == ReGamma_SW_Mode_BIT14 || BitMode == ReGamma_SW_Mode_BIT14_ROI) && EntryMode == ReGamma_SW_Entry_257)
                ReGconfig = ReG_hlg_config_14bits_257entry;
            else if ((BitMode == ReGamma_SW_Mode_BIT12) && EntryMode == ReGamma_SW_Entry_257)
                ReGconfig = ReG_hlg_config_12bits_257entry;
            else if ((BitMode == ReGamma_SW_Mode_BIT12To8) && EntryMode == ReGamma_SW_Entry_65)
                ReGconfig = ReG_hlg_config_12to8bits_65entry;
            else if ((BitMode == ReGamma_SW_Mode_BIT10To8) && EntryMode == ReGamma_SW_Entry_129)
                ReGconfig = ReG_hlg_config_10to8bits_129entry;
            else if ((BitMode == ReGamma_SW_Mode_BIT10To8) && EntryMode == ReGamma_SW_Entry_257)
                ReGconfig = ReG_hlg_config_10to8bits_257entry;
            else if ((BitMode == ReGamma_SW_Mode_BIT8) && EntryMode == ReGamma_SW_Entry_65)
                ReGconfig = ReG_hlg_config_8bits_65entry;
            else if ((BitMode == ReGamma_SW_Mode_BIT17To12) && EntryMode == ReGamma_SW_Entry_65)
                ReGconfig = ReG_hlg_config_17to12bits_65entry;
            else if ((BitMode == ReGamma_SW_Mode_BIT17To14) && EntryMode == ReGamma_SW_Entry_257)
                ReGconfig = ReG_hlg_config_17to14bits_257entry;
            else
            {
                LOGE("unsupported combination of mode and entry\n");
                assert(0);
            }
        }
		break;

		case ReGamma_Mode_ReG_170M:
        {
            if (PRINT) printf(" (170M)\n");
            if (fPRINT) fprintf(fLog, " (170M)\n");

            if ((BitMode == ReGamma_SW_Mode_BIT14 || BitMode == ReGamma_SW_Mode_BIT14_ROI) && EntryMode == ReGamma_SW_Entry_257)
                ReGconfig = ReG_170m_config_14bits_257entry;
            else if ((BitMode == ReGamma_SW_Mode_BIT12) && EntryMode == ReGamma_SW_Entry_257)
                ReGconfig = ReG_170m_config_12bits_257entry;
            else if ((BitMode == ReGamma_SW_Mode_BIT12To8) && EntryMode == ReGamma_SW_Entry_65)
                ReGconfig = ReG_170m_config_12to8bits_65entry;
            else if ((BitMode == ReGamma_SW_Mode_BIT10To8) && EntryMode == ReGamma_SW_Entry_129)
                ReGconfig = ReG_170m_config_10to8bits_129entry;
            else if ((BitMode == ReGamma_SW_Mode_BIT10To8) && EntryMode == ReGamma_SW_Entry_257)
                ReGconfig = ReG_170m_config_10to8bits_257entry;
            else if ((BitMode == ReGamma_SW_Mode_BIT8) && EntryMode == ReGamma_SW_Entry_65)
                ReGconfig = ReG_170m_config_8bits_65entry;
            else if ((BitMode == ReGamma_SW_Mode_BIT17To12) && EntryMode == ReGamma_SW_Entry_65)
                ReGconfig = ReG_170m_config_17to12bits_65entry;
            else if ((BitMode == ReGamma_SW_Mode_BIT17To14) && EntryMode == ReGamma_SW_Entry_257)
                ReGconfig = ReG_170m_config_17to14bits_257entry;
            else
            {
                LOGE("unsupported combination of mode and entry\n");
                assert(0);
            }
        }
		break;

		case ReGamma_Mode_ReG_sRGB:
        {
            if (PRINT) printf(" (sRGB)\n");
            if (fPRINT) fprintf(fLog, " (sRGB)\n");

            if ((BitMode == ReGamma_SW_Mode_BIT14 || BitMode == ReGamma_SW_Mode_BIT14_ROI) && EntryMode == ReGamma_SW_Entry_257)
                ReGconfig = ReG_srgb_config_14bits_257entry;
            else if ((BitMode == ReGamma_SW_Mode_BIT12) && EntryMode == ReGamma_SW_Entry_257)
                ReGconfig = ReG_srgb_config_12bits_257entry;
            else if ((BitMode == ReGamma_SW_Mode_BIT12To8) && EntryMode == ReGamma_SW_Entry_65)
                ReGconfig = ReG_srgb_config_12to8bits_65entry;
            else if ((BitMode == ReGamma_SW_Mode_BIT10To8) && EntryMode == ReGamma_SW_Entry_129)
                ReGconfig = ReG_srgb_config_10to8bits_129entry;
            else if ((BitMode == ReGamma_SW_Mode_BIT10To8) && EntryMode == ReGamma_SW_Entry_257)
                ReGconfig = ReG_srgb_config_10to8bits_257entry;
            else if ((BitMode == ReGamma_SW_Mode_BIT8) && EntryMode == ReGamma_SW_Entry_65)
                ReGconfig = ReG_srgb_config_8bits_65entry;
            else if ((BitMode == ReGamma_SW_Mode_BIT17To12) && EntryMode == ReGamma_SW_Entry_65)
                ReGconfig = ReG_srgb_config_17to12bits_65entry;
            else if ((BitMode == ReGamma_SW_Mode_BIT17To14) && EntryMode == ReGamma_SW_Entry_257)
                ReGconfig = ReG_srgb_config_17to14bits_257entry;
            else
            {
                LOGE("unsupported combination of mode and entry\n");
                assert(0);
            }
        }
		break;

		case ReGamma_Mode_ReG_ReGamma2_2:
        {
            if (PRINT) printf(" (Gamma2.2)\n");
            if (fPRINT) fprintf(fLog, " (Gamma2.2)\n");

            if ((BitMode == ReGamma_SW_Mode_BIT14 || BitMode == ReGamma_SW_Mode_BIT14_ROI) && EntryMode == ReGamma_SW_Entry_257)
                ReGconfig = ReG_2_2_config_14bits_257entry;
            else if ((BitMode == ReGamma_SW_Mode_BIT12) && EntryMode == ReGamma_SW_Entry_257)
                ReGconfig = ReG_2_2_config_12bits_257entry;
            else if ((BitMode == ReGamma_SW_Mode_BIT12To8) && EntryMode == ReGamma_SW_Entry_65)
                ReGconfig = ReG_2_2_config_12to8bits_65entry;
            else if ((BitMode == ReGamma_SW_Mode_BIT10To8) && EntryMode == ReGamma_SW_Entry_129)
                ReGconfig = ReG_2_2_config_10to8bits_129entry;
            else if ((BitMode == ReGamma_SW_Mode_BIT10To8) && EntryMode == ReGamma_SW_Entry_257)
                ReGconfig = ReG_2_2_config_10to8bits_257entry;
            else if ((BitMode == ReGamma_SW_Mode_BIT8) && EntryMode == ReGamma_SW_Entry_65)
                ReGconfig = ReG_2_2_config_8bits_65entry;
            else if ((BitMode == ReGamma_SW_Mode_BIT17To12) && EntryMode == ReGamma_SW_Entry_65)
                ReGconfig = ReG_2_2_config_17to12bits_65entry;
            else if ((BitMode == ReGamma_SW_Mode_BIT17To14) && EntryMode == ReGamma_SW_Entry_257)
                ReGconfig = ReG_2_2_config_17to14bits_257entry;
            else
            {
                LOGE("unsupported combination of mode and entry\n");
                assert(0);
            }
        }
		break;

        case ReGamma_Mode_ReG_ReGamma2_6:
        {
            if (PRINT) printf(" (Gamma2.6)\n");
            if (fPRINT) fprintf(fLog, " (Gamma2.6)\n");

            if ((BitMode == ReGamma_SW_Mode_BIT14 || BitMode == ReGamma_SW_Mode_BIT14_ROI) && EntryMode == ReGamma_SW_Entry_257)
                ReGconfig = ReG_2_6_config_14bits_257entry;
            else if ((BitMode == ReGamma_SW_Mode_BIT12) && EntryMode == ReGamma_SW_Entry_257)
                ReGconfig = ReG_2_6_config_12bits_257entry;
            else if ((BitMode == ReGamma_SW_Mode_BIT12To8) && EntryMode == ReGamma_SW_Entry_65)
                ReGconfig = ReG_2_6_config_12to8bits_65entry;
            else if ((BitMode == ReGamma_SW_Mode_BIT10To8) && EntryMode == ReGamma_SW_Entry_129)
                ReGconfig = ReG_2_6_config_10to8bits_129entry;
            else if ((BitMode == ReGamma_SW_Mode_BIT10To8) && EntryMode == ReGamma_SW_Entry_257)
                ReGconfig = ReG_2_6_config_10to8bits_257entry;
            else if ((BitMode == ReGamma_SW_Mode_BIT8) && EntryMode == ReGamma_SW_Entry_65)
                ReGconfig = ReG_2_6_config_8bits_65entry;
            else if ((BitMode == ReGamma_SW_Mode_BIT17To12) && EntryMode == ReGamma_SW_Entry_65)
                ReGconfig = ReG_2_6_config_17to12bits_65entry;
            else if ((BitMode == ReGamma_SW_Mode_BIT17To14) && EntryMode == ReGamma_SW_Entry_257)
                ReGconfig = ReG_2_6_config_17to14bits_257entry;
            else
            {
                LOGE("unsupported combination of mode and entry\n");
                assert(0);
            }
        }
        break;


		default:
			printf("[%s(%d)] Undefined Switch(%d).\n", __FUNCTION__, __LINE__, Switch);
			exit(EXIT_FAILURE);
			break;
		}

        *RegCountR = ReGconfig.ReGCount;
        *RegCountG = ReGconfig.ReGCount;
        *RegCountB = ReGconfig.ReGCount;


        for (i = 0; i < ReGconfig.ReGCount + 1; i++)
        {
            RegStepR[i] = ReGconfig.ReGSteps[i];
            RegStepG[i] = ReGconfig.ReGSteps[i];
            RegStepB[i] = ReGconfig.ReGSteps[i];
        }
        for (i = 0; i < ReGconfig.ReGCount; i++)
        {
            RegPointR[i] = ReGconfig.ReGPoints[i];
            RegPointG[i] = ReGconfig.ReGPoints[i];
            RegPointB[i] = ReGconfig.ReGPoints[i];
        }

		entryR = ReGamma_CurveFun1_BuildRegLut2Ys(
            ReGconfig.ReGCurve, inMaxValue, outMaxValue,
            InBitWidth, *RegCountR, RegPointR, RegStepR,
			RegDataR);
		RegDataR[entryR - 1] = outMaxValue + ReGconfig.corQuanY;

		entryG = entryR;
		entryB = entryR;
		for (i = 0; i < entryR; i++)
		{
			RegDataG[i] = RegDataR[i];
			RegDataB[i] = RegDataR[i];
		}
	}

    if (PRINT)
    {
        printf(" [Reg] CountR: %d\n", *RegCountR);
        printf("       PointR[%d]: [", *RegCountR);
        for (i = 0; i < *RegCountR; i++)
            printf("%d, ", RegPointR[i]);
        printf("]\n");
        printf("       StepR[%d]: [", *RegCountR + 1);
        for (i = 0; i < *RegCountR + 1; i++)
            printf("%d, ", RegStepR[i]);
        printf("]\n");
        printf("       DataR[%d]: [", entryR);
        for (i = 0; i < entryR; i++)
            printf("%d, ", RegDataR[i]);
        printf("]\n");
        /* PrintTable_Lut2Reg("ReG LutR", InBitWidth, *RegCountR, RegPointR, RegStepR, RegDataR); */
        printf(" [Reg] CountG: %d\n", *RegCountG);
        printf("       PointG[%d]: [", *RegCountG);
        for (i = 0; i < *RegCountG; i++)
            printf("%d, ", RegPointG[i]);
        printf("]\n");
        printf("       StepG[%d]: [", *RegCountG + 1);
        for (i = 0; i < *RegCountG + 1; i++)
            printf("%d, ", RegStepG[i]);
        printf("]\n");
        printf("       DataG[%d]: [", entryG);
        for (i = 0; i < entryG; i++)
            printf("%d, ", RegDataG[i]);
        printf("]\n");
        /* PrintTable_Lut2Reg("ReG LutG", InBitWidth, *RegCountG, RegPointG, RegStepG, RegDataG); */
        printf(" [Reg] CountB: %d\n", *RegCountB);
        printf("       PointB[%d]: [", *RegCountB);
        for (i = 0; i < *RegCountB; i++)
            printf("%d, ", RegPointB[i]);
        printf("]\n");
        printf("       StepB[%d]: [", *RegCountB + 1);
        for (i = 0; i < *RegCountB + 1; i++)
            printf("%d, ", RegStepB[i]);
        printf("]\n");
        printf("       DataB[%d]: [", entryB);
        for (i = 0; i < entryB; i++)
            printf("%d, ", RegDataB[i]);
        printf("]\n");
        ReGamma_PrintTable_Lut2Reg("ReG LutB", InBitWidth, *RegCountB, RegPointB, RegStepB, RegDataB);
        printf("\n");
    }
    if (fPRINT)
    {
        fprintf(fLog, " [Reg] CountR: %d\n", *RegCountR);
        fprintf(fLog, "       PointR[%d]: [", *RegCountR);
        for (i = 0; i < *RegCountR; i++)
            fprintf(fLog, "%d, ", RegPointR[i]);
        fprintf(fLog, "]\n");
        fprintf(fLog, "       StepR[%d]: [", *RegCountR + 1);
        for (i = 0; i < *RegCountR + 1; i++)
            fprintf(fLog, "%d, ", RegStepR[i]);
        fprintf(fLog, "]\n");
        fprintf(fLog, "       DataR[%d]: [", entryR);
        for (i = 0; i < entryR; i++)
            fprintf(fLog, "%d, ", RegDataR[i]);
        fprintf(fLog, "]\n");
        /* PrintTable_Lut2Reg("ReG LutR", InBitWidth, *RegCountR, RegPointR, RegStepR, RegDataR); */
        fprintf(fLog, " [Reg] CountG: %d\n", *RegCountG);
        fprintf(fLog, "       PointG[%d]: [", *RegCountG);
        for (i = 0; i < *RegCountG; i++)
            fprintf(fLog, "%d, ", RegPointG[i]);
        fprintf(fLog, "]\n");
        fprintf(fLog, "       StepG[%d]: [", *RegCountG + 1);
        for (i = 0; i < *RegCountG + 1; i++)
            fprintf(fLog, "%d, ", RegStepG[i]);
        fprintf(fLog, "]\n");
        fprintf(fLog, "       DataG[%d]: [", entryG);
        for (i = 0; i < entryG; i++)
            fprintf(fLog, "%d, ", RegDataG[i]);
        fprintf(fLog, "]\n");
        /* PrintTable_Lut2Reg("ReG LutG", InBitWidth, *RegCountG, RegPointG, RegStepG, RegDataG); */
        fprintf(fLog, " [Reg] CountB: %d\n", *RegCountB);
        fprintf(fLog, "       PointB[%d]: [", *RegCountB);
        for (i = 0; i < *RegCountB; i++)
            fprintf(fLog, "%d, ", RegPointB[i]);
        fprintf(fLog, "]\n");
        fprintf(fLog, "       StepB[%d]: [", *RegCountB + 1);
        for (i = 0; i < *RegCountB + 1; i++)
            fprintf(fLog, "%d, ", RegStepB[i]);
        fprintf(fLog, "]\n");
        fprintf(fLog, "       DataB[%d]: [", entryB);
        for (i = 0; i < entryB; i++)
            fprintf(fLog, "%d, ", RegDataB[i]);
        fprintf(fLog, "]\n");
        ReGamma_fPrintTable_Lut2Reg(fLog, "ReG LutB", InBitWidth, *RegCountR, RegPointR, RegStepR, RegDataR);
        fprintf(fLog, "\n");
    }

	*EntryR = entryR;
	*EntryG = entryG;
	*EntryB = entryB;

}

API_EXPORT void Set_ReGConfig(
    IN U04 Switch,
    IN const char *ConfBinR,
    IN const char *ConfBinG,
    IN const char *ConfBinB,
    OUT U32 *RegCountR, OUT U32 *RegPointR, OUT U32 *RegStepR, OUT U32 *RegDataR, OUT U32 *EntryR,
    OUT U32 *RegCountG, OUT U32 *RegPointG, OUT U32 *RegStepG, OUT U32 *RegDataG, OUT U32 *EntryG,
    OUT U32 *RegCountB, OUT U32 *RegPointB, OUT U32 *RegStepB, OUT U32 *RegDataB, OUT U32 *EntryB)
{
    return Set_ReGConfig_core(
        Switch,
        ReGamma_SW_Mode_BIT14,
        SW_REGAMMA_LUTMODE_3CHANNEL,
        ReGamma_SW_Entry_257,
        ConfBinR,
        ConfBinG,
        ConfBinB,
        RegCountR, RegPointR, RegStepR, RegDataR, EntryR,
        RegCountG, RegPointG, RegStepG, RegDataG, EntryG,
        RegCountB, RegPointB, RegStepB, RegDataB, EntryB);
}

API_EXPORT void Set_ReGamma_Config(IN ReGamma_User_Config* ReGamma_User_Config, OUT ReGamma_SW_Config* ReGamma_Config)
{
    Set_ReGConfig_core(
        ReGamma_User_Config->ReGamma_Mode,
        ReGamma_User_Config->ReGamma_BitMode,
        ReGamma_User_Config->ReGamma_LutMode,
        ReGamma_User_Config->ReGamma_EntryMode,
        ReGamma_User_Config->csvPathR,
        ReGamma_User_Config->csvPathG,
        ReGamma_User_Config->csvPathB,
        &(ReGamma_Config->ReGCountR), ReGamma_Config->ReGPointR, ReGamma_Config->ReGStepR, ReGamma_Config->ReGDataR, &(ReGamma_Config->ReGEntryR),
        &(ReGamma_Config->ReGCountG), ReGamma_Config->ReGPointG, ReGamma_Config->ReGStepG, ReGamma_Config->ReGDataG, &(ReGamma_Config->ReGEntryG),
        &(ReGamma_Config->ReGCountB), ReGamma_Config->ReGPointB, ReGamma_Config->ReGStepB, ReGamma_Config->ReGDataB, &(ReGamma_Config->ReGEntryB));
}
