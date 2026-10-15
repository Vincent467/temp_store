#include "sw_base.hpp"

static U01 PRINT = 0;
static U01 fPRINT = 0;
static FILE *fLog = NULL;

double EOTF_De_PQ(double value)
{
	assert(0 <= value && value <= 1);

	double m1 = 2610.0 / 4096.0 / 4.0;
	double m2 = 2523.0 / 4096.0 * 128.0;
	double c1 = 3424.0 / 4096.0;
	double c2 = 2413.0 / 4096.0 * 32.0;
	double c3 = 2392.0 / 4096.0 * 32.0;
	/*double m1 = 0.1593017578125; */
	/*double m2 = 78.84375; */
	/*double c1 = 0.8359375; */
	/*double c2 = 18.8515625; */
	/*double c3 = 18.6875; */

	double out_value = 0;

	double tempValue = pow(value, (1.0 / m2));
	out_value = pow(fmax(0.0, (tempValue - c1)) / (c2 - c3 * tempValue), (1.0 / m1));

	return out_value;
}


double DeGamma_Clamp_double(
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


double EOTF_De_HLG(double value)
{
	assert(0 <= value && value <= 1);
	/*
	//    double LB = 0;
	//    double LW = 1000;
	//    double r = 1.2 + 0.42*log10(LW/1000.0);
	//    double beta = pow( 3*pow((LB / LW), (double)1.0/r), (double)1.0/2);
	//
	//    value = max_double(0, (1.0 - beta)*value + beta);
	//
	//    value = Clamp_double(value, 0, 1.0);
	//    printf(" LB:%f LW:%f r:%f beta:%f --> value:%f\n", LB, LW, r, beta, value);
	*/
	/*float a = 0.17883277, b = 0.28466892, c = 0.55991073; */
	double a = 0.17883277f;
	double b = 1 - 4 * a;
	double c = (double)(0.5 - a * log(4 * a));

	double out_value;

	if (0 <= value && value <= 0.5)
		out_value = value * value / 3.0;
	else
		out_value = (exp((value - c) / a) + b) / 12.0;

	out_value = DeGamma_Clamp_double(out_value, 0, 1);

	return out_value;
}

double EOTF_De_170M(double value)
{
	assert(0 <= value && value <= 1);

	double result = 0;

	if (0 <= value && value < 0.0812)
	{
		result = value / 4.5;
	}
	else if (0.0812 <= value && value <= 1)
	{
		result = pow(((value + 0.099) / 1.099), (1.0 / 0.45));
	}
	else
	{
		LOGE("[%s:%d] value(%f) error\'\n", __FUNCTION__, __LINE__, value);
		exit(EXIT_FAILURE);
	}

	return result;
}

double EOTF_De_sRGB(double value)
{
	assert(0 <= value && value <= 1);

	double result = 0;

	if (0 <= value && value < 0.04045)
	{
		result = value / 12.92;
	}
	else if (0.04045 <= value && value <= 1)
	{
		result = pow((value + 0.055) / 1.055, 2.4);
	}
	else
	{
		printf("[%s:%d] value(%f) error\'\n", __FUNCTION__, __LINE__, value);
		exit(EXIT_FAILURE);
	}

	return result;
}

double EOTF_De_DeGamma2_2(double value)
{
	assert(0 <= value && value <= 1);

	return pow(value, 2.2);
}

double EOTF_De_DeGamma2_6(double value)
{
    assert(0 <= value && value <= 1);

    return pow(value, 2.6);
}

typedef struct {
    uint32_t DeGCount;
    uint32_t DeGSteps[16];
    uint32_t DeGPoints[16];
    uint32_t corQuanY;
    double(*DeGCurve)(double);
}DegammaConfig;

DegammaConfig DeG_pq_config_In12 = {
    4,
    { 128, 64, 32, 16, 8 },
    { 2048, 2560, 3072, 3712 },
    0,
    EOTF_De_PQ
};

DegammaConfig DeG_pq_config_In10 = {
    4,
    { 32, 16, 8, 4, 2 },
    { 512, 640, 768, 928 },
    0,
    EOTF_De_PQ
};

DegammaConfig DeG_pq_config_In8 = {
    2,
    { 4, 2, 1},
    { 128, 192 },
    0,
    EOTF_De_PQ
};

DegammaConfig DeG_pq_config_In8_Entry65 = {
    4,
    { 16, 8, 4, 2, 1},
    { 128, 160, 192, 232 },
    0,
    EOTF_De_PQ
};

DegammaConfig DeG_hlg_config_In12 = {
    4,
    { 128, 64, 32, 16, 8 },
    { 1024, 2048, 3072, 3968 },
    0,
    EOTF_De_HLG
};

DegammaConfig DeG_hlg_config_In10 = {
    3,
    { 32, 8, 4, 1 },
    { 512, 832, 992 },
    0,
    EOTF_De_HLG
};

DegammaConfig DeG_hlg_config_In8 = {
    2,
    { 8, 2, 1},
    { 128, 208, 248 },
    0,
    EOTF_De_HLG
};

DegammaConfig DeG_hlg_config_In8_Entry65 = {
    2,
    { 8, 4, 2},
    { 128, 192 },
    0,
    EOTF_De_HLG
};

DegammaConfig DeG_170m_config_In12 = {
    0,
    { 32 },
    { 0 },
    0,
    EOTF_De_170M
};

DegammaConfig DeG_170m_config_In10 = {
    0,
    { 8 },
    { 0 },
    0,
    EOTF_De_170M
};

DegammaConfig DeG_170m_config_In8 = {
    0,
    { 2 },
    { 0 },
    0,
    EOTF_De_170M
};

DegammaConfig DeG_170m_config_In8_Entry65 = {
    0,
    { 4 },
    { 0 },
    0,
    EOTF_De_170M
};

DegammaConfig DeG_srgb_config_In12 = {
    0,
    { 32 },
    { 0 },
    0,
    EOTF_De_sRGB
};

DegammaConfig DeG_srgb_config_In10 = {
    0,
    { 8 },
    { 0 },
    0,
    EOTF_De_sRGB
};

DegammaConfig DeG_srgb_config_In8 = {
    0,
    { 2 },
    { 0 },
    0,
    EOTF_De_sRGB
};

DegammaConfig DeG_srgb_config_In8_Entry65 = {
    0,
    { 4 },
    { 0 },
    0,
    EOTF_De_sRGB
};

DegammaConfig DeG_2_2_config_In12 = {
    0,
    { 32 },
    { 0 },
    0,
    EOTF_De_DeGamma2_2
};

DegammaConfig DeG_2_2_config_In10 = {
    0,
    { 8 },
    { 0 },
    0,
    EOTF_De_DeGamma2_2
};

DegammaConfig DeG_2_2_config_In8 = {
    0,
    { 2 },
    { 0 },
    0,
    EOTF_De_DeGamma2_2
};

DegammaConfig DeG_2_2_config_In8_Entry65 = {
    0,
    { 4 },
    { 0 },
    0,
    EOTF_De_DeGamma2_2
};


DegammaConfig DeG_2_6_config_In12 = {
    0,
    { 32 },
    { 0 },
    0,
    EOTF_De_DeGamma2_6
};

DegammaConfig DeG_2_6_config_In10 = {
    0,
    { 8 },
    { 0 },
    0,
    EOTF_De_DeGamma2_6
};

DegammaConfig DeG_2_6_config_In8 = {
    0,
    { 2 },
    { 0 },
    0,
    EOTF_De_DeGamma2_6
};

DegammaConfig DeG_2_6_config_In8_Entry65 = {
    0,
    { 4 },
    { 0 },
    0,
    EOTF_De_DeGamma2_6
};



void DeGamma_PrintLut2Table(
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

	LOGD("==================[ %s ]===============\n", name);
	if (0 < LutLen)
	{
		LOGD(" [index]: LutX --- LutY\n");
		LOGD(" [%d]: %d --- %d\n", 0, LutX[0], LutY[0]);
		num++;

		for (i = 0; i < LutLen - 1; i++)
		{
			if (3 == LutS[i]) continue;

			StepN = (LutX[i + 1] - LutX[i]) / LutS[i];

			for (j = 0; j < StepN; j++)
			{
				X = LutX[i] + (LutS[i] * (j + 1));
				LOGD(" [%d]: %d --- %d\n", num, X, LutY[num]);
				num++;
			}
		}
	}
	else
		LOGD("None");

	LOGD("---------------------------------\n");
}

void DeGamma_CurveFun1_BuildLut2Ys(
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
	Xk = DeGamma_Clamp_double(Xk, (double)0, (double)1.0);
	LutY[0] = (U32)(CurveFun(Xk) * quanY + 0.5);
	num++;

	for (i = 0; i < LutLen - 1; i++)
	{
		if (3 == LutS[i] || 0 == LutS[i]) continue;

		StepN = (LutX[i + 1] - LutX[i]) / LutS[i];

		for (j = 0; j < StepN; j++)
		{
			X = LutX[i] + (LutS[i] * (j + 1));
			Xk = (double)X / quanX;
			Xk = DeGamma_Clamp_double(Xk, (double)0, (double)1.0);
			LutY[num] = (U32)(CurveFun(Xk) * quanY + 0.5);
			num++;
		}
	}

	*LutNum = num;
}

U32 DeGamma_CurveFun1_BuildRegLut2Ys(
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
	DeGamma_CurveFun1_BuildLut2Ys(
		CurveFun, quanX, quanY,
		LutLen, LutX, RegStep,
		&Entry, RegData);

	if (NULL != LutX) { free(LutX); LutX = NULL; }
	return Entry;
}

void DeGamma_GetStrSuffix(const char *str, char *suffix)
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


void DeGamma_fPrintLut2Table(
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

API_EXPORT void DeGamma_fPrintTable_Lut2Reg(
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

	DeGamma_fPrintLut2Table(logfile, name, LutLen, LutX, RegStep, RegData);

	if (NULL != LutX) { free(LutX); LutX = NULL; }
}

U32 DeGamma_LoadBin_DReG(
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

API_EXPORT void DeGamma_PrintTable_Lut2Reg(
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

	DeGamma_PrintLut2Table(name, LutLen, LutX, RegStep, RegData);

	if (NULL != LutX) { free(LutX); LutX = NULL; }
}

API_EXPORT U32 Set_DeGConfig(
    IN U04 Switch,
    IN U04 BitMode,
    IN U04 EntryMode,
    IN const char *ConfBin,
    OUT U32 *RegCount, OUT U32 *RegPoint, OUT U32 *RegStep, OUT U32 *RegData)
{
    assert(Switch <= DeGamma_Mode_END);
    assert(BitMode <= DeGamma_SW_BITMode_END);

    if (DeGamma_Mode_Disable == Switch)
    {
        if (PRINT) printf(" [DeG] Disable\n");
        if (fPRINT) fprintf(fLog, " [DeG] Disable\n");
        return 0;
    }
    else
    {
        if (PRINT)
        {
            printf(" [DeG] Enable\n");
            printf(" DeG_Switch: %d\n", Switch);
        }
        if (fPRINT)
        {
            fprintf(fLog, " [DeG] Enable\n");
            fprintf(fLog, " DeG_Switch: %d\n", Switch);
        }
    }

    U32 inBit = 0;
    U32 outBit = 0;
    if (BitMode == DeGamma_SW_BITMode_In10_Out14)
    {
        inBit = 10;
        outBit = 14;
    }
    else if (BitMode == DeGamma_SW_BITMode_In10_Out17)
    {
        inBit = 10;
        outBit = 17;
    }
    else if (BitMode == DeGamma_SW_BITMode_In12_Out17)
    {
        inBit = 12;
        outBit = 17;
    }
    else if (BitMode == DeGamma_SW_BITMode_In12_Out14)
    {
        inBit = 12;
        outBit = 14;
    }
    else if (BitMode == DeGamma_SW_BITMode_In8_Out12)
    {
        inBit = 8;
        outBit = 12;
    }
    else if (BitMode == DeGamma_SW_BITMode_In8_Out10)
    {
        inBit = 8;
        outBit = 10;
    }
    else
    {
        LOGE("unsupport bitmode:%d in Set_DeGConfig.\n", BitMode);
        assert(0);
    }
    U32 Entry = 0;
    U32 i = 0;

    assert(NULL != RegCount);
    assert(NULL != RegPoint);
    assert(NULL != RegStep);
    assert(NULL != RegData);

    U32 inMaxValue = (1 << inBit) - 1;
    U32 outMaxValue = (1 << outBit) - 1;

    if (DeGamma_Mode_ProgConf == Switch)
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
        DeGamma_GetStrSuffix(ConfBin, suffix);
        assert(!strcmp(suffix, "bin"));

        Entry = DeGamma_LoadBin_DReG(
            ConfBin,
            RegCount, RegPoint, RegStep, RegData);
    }
    else
    {
        DegammaConfig DeGconfig = { 0 };

        switch (Switch)
        {
        case DeGamma_Mode_DeG_PQ:
        {
            if (PRINT) printf(" (PQ)\n");
            if (fPRINT) fprintf(fLog, " (PQ)\n");

            if ((BitMode == DeGamma_SW_BITMode_In10_Out14 || BitMode == DeGamma_SW_BITMode_In10_Out17) && EntryMode == DeGamma_SW_Entry_129)
                DeGconfig = DeG_pq_config_In10;
            else if ((BitMode == DeGamma_SW_BITMode_In12_Out14 || BitMode == DeGamma_SW_BITMode_In12_Out17) && EntryMode == DeGamma_SW_Entry_129)
                DeGconfig = DeG_pq_config_In12;
            else if (BitMode == DeGamma_SW_BITMode_In8_Out12 && EntryMode == DeGamma_SW_Entry_129)
                DeGconfig = DeG_pq_config_In8;
            else if ((BitMode == DeGamma_SW_BITMode_In8_Out10 || BitMode == DeGamma_SW_BITMode_In8_Out12) && EntryMode == DeGamma_SW_Entry_65)
                DeGconfig = DeG_pq_config_In8_Entry65;
            else
            {
                LOGE("unsupported combination of mode and entry\n");
                assert(0);
            }

        }
        break;

        case DeGamma_Mode_DeG_HLG:
        {
            if (PRINT) printf(" (HLG)\n");
            if (fPRINT) fprintf(fLog, " (HLG)\n");

            if ((BitMode == DeGamma_SW_BITMode_In10_Out14 || BitMode == DeGamma_SW_BITMode_In10_Out17) && EntryMode == DeGamma_SW_Entry_129)
                DeGconfig = DeG_hlg_config_In10;
            else if ((BitMode == DeGamma_SW_BITMode_In12_Out14 || BitMode == DeGamma_SW_BITMode_In12_Out17) && EntryMode == DeGamma_SW_Entry_129)
                DeGconfig = DeG_hlg_config_In12;
            else if (BitMode == DeGamma_SW_BITMode_In8_Out12 && EntryMode == DeGamma_SW_Entry_129)
                DeGconfig = DeG_hlg_config_In8;
            else if ((BitMode == DeGamma_SW_BITMode_In8_Out10 || BitMode == DeGamma_SW_BITMode_In8_Out12) && EntryMode == DeGamma_SW_Entry_65)
                DeGconfig = DeG_hlg_config_In8_Entry65;
            else
            {
                LOGE("unsupported combination of mode and entry\n");
                assert(0);
            }

        }
        break;

        case DeGamma_Mode_DeG_170M:
        {
            if (PRINT) printf(" (170M)\n");
            if (fPRINT) fprintf(fLog, " (170M)\n");

            if ((BitMode == DeGamma_SW_BITMode_In10_Out14 || BitMode == DeGamma_SW_BITMode_In10_Out17) && EntryMode == DeGamma_SW_Entry_129)
                DeGconfig = DeG_170m_config_In10;
            else if ((BitMode == DeGamma_SW_BITMode_In12_Out14 || BitMode == DeGamma_SW_BITMode_In12_Out17) && EntryMode == DeGamma_SW_Entry_129)
                DeGconfig = DeG_170m_config_In12;
            else if (BitMode == DeGamma_SW_BITMode_In8_Out12 && EntryMode == DeGamma_SW_Entry_129)
                DeGconfig = DeG_170m_config_In8;
            else if ((BitMode == DeGamma_SW_BITMode_In8_Out10 || BitMode == DeGamma_SW_BITMode_In8_Out12) && EntryMode == DeGamma_SW_Entry_65)
                DeGconfig = DeG_170m_config_In8_Entry65;
            else
            {
                LOGE("unsupported combination of mode and entry\n");
                assert(0);
            }

        }
        break;

        case DeGamma_Mode_DeG_sRGB:
        {
            if (PRINT) printf(" (sRGB)\n");
            if (fPRINT) fprintf(fLog, " (sRGB)\n");

            if ((BitMode == DeGamma_SW_BITMode_In10_Out14 || BitMode == DeGamma_SW_BITMode_In10_Out17) && EntryMode == DeGamma_SW_Entry_129)
                DeGconfig = DeG_srgb_config_In10;
            else if ((BitMode == DeGamma_SW_BITMode_In12_Out14 || BitMode == DeGamma_SW_BITMode_In12_Out17) && EntryMode == DeGamma_SW_Entry_129)
                DeGconfig = DeG_srgb_config_In12;
            else if (BitMode == DeGamma_SW_BITMode_In8_Out12 && EntryMode == DeGamma_SW_Entry_129)
                DeGconfig = DeG_srgb_config_In8;
            else if ((BitMode == DeGamma_SW_BITMode_In8_Out10 || BitMode == DeGamma_SW_BITMode_In8_Out12) && EntryMode == DeGamma_SW_Entry_65)
                DeGconfig = DeG_srgb_config_In8_Entry65;
            else
            {
                LOGE("unsupported combination of mode and entry\n");
                assert(0);
            }

        }
        break;

        case DeGamma_Mode_DeG_DeGamma2_2:
        {
            if (PRINT) printf(" (DeGamma2.2)\n");
            if (fPRINT) fprintf(fLog, " (DeGamma2.2)\n");

            if ((BitMode == DeGamma_SW_BITMode_In10_Out14 || BitMode == DeGamma_SW_BITMode_In10_Out17) && EntryMode == DeGamma_SW_Entry_129)
                DeGconfig = DeG_2_2_config_In10;
            else if ((BitMode == DeGamma_SW_BITMode_In12_Out14 || BitMode == DeGamma_SW_BITMode_In12_Out17) && EntryMode == DeGamma_SW_Entry_129)
                DeGconfig = DeG_2_2_config_In12;
            else if (BitMode == DeGamma_SW_BITMode_In8_Out12 && EntryMode == DeGamma_SW_Entry_129)
                DeGconfig = DeG_2_2_config_In8;
            else if ((BitMode == DeGamma_SW_BITMode_In8_Out10 || BitMode == DeGamma_SW_BITMode_In8_Out12) && EntryMode == DeGamma_SW_Entry_65)
                DeGconfig = DeG_2_2_config_In8_Entry65;
            else
            {
                LOGE("unsupported combination of mode and entry\n");
                assert(0);
            }

        }
        break;

        case DeGamma_Mode_DeG_DeGamma2_6:
        {
            if (PRINT) printf(" (DeGamma2.6)\n");
            if (fPRINT) fprintf(fLog, " (DeGamma2.6)\n");

            if ((BitMode == DeGamma_SW_BITMode_In10_Out14 || BitMode == DeGamma_SW_BITMode_In10_Out17)&& EntryMode == DeGamma_SW_Entry_129)
                DeGconfig = DeG_2_6_config_In10;
            else if ((BitMode == DeGamma_SW_BITMode_In12_Out14 || BitMode == DeGamma_SW_BITMode_In12_Out17) && EntryMode == DeGamma_SW_Entry_129)
                DeGconfig = DeG_2_6_config_In12;
            else if (BitMode == DeGamma_SW_BITMode_In8_Out12 && EntryMode == DeGamma_SW_Entry_129)
                DeGconfig = DeG_2_6_config_In8;
            else if ((BitMode == DeGamma_SW_BITMode_In8_Out10 || BitMode == DeGamma_SW_BITMode_In8_Out12) && EntryMode == DeGamma_SW_Entry_65)
                DeGconfig = DeG_2_6_config_In8_Entry65;
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

        *RegCount = DeGconfig.DeGCount;
        for (i = 0; i < DeGconfig.DeGCount + 1; i++)
            RegStep[i] = DeGconfig.DeGSteps[i];
        for (i = 0; i < DeGconfig.DeGCount; i++)
            RegPoint[i] = DeGconfig.DeGPoints[i];

        Entry = DeGamma_CurveFun1_BuildRegLut2Ys(
            DeGconfig.DeGCurve, inMaxValue, outMaxValue,
            inBit, *RegCount, RegPoint, RegStep,
            RegData);
        RegData[Entry - 1] = outMaxValue + DeGconfig.corQuanY;

        if (PRINT) printf("       Entry: %d\n", Entry);
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
        DeGamma_PrintTable_Lut2Reg("DeG Lut", inBit, *RegCount, RegPoint, RegStep, RegData);
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
        DeGamma_fPrintTable_Lut2Reg(fLog, "DeG Lut", inBit, *RegCount, RegPoint, RegStep, RegData);
        fprintf(fLog, "\n");
    }

    return Entry;
}


API_EXPORT void Set_DeGamma_Config(
    IN uint32_t DeGamma_Mode, IN uint32_t DeGamma_BITMode, IN const char *csvPath, OUT U32 *DeGCount, OUT U32 *DeGPoint, OUT U32 *DeGStep, OUT U32 *DeGData)
{
    Set_DeGConfig(
        DeGamma_Mode,
        DeGamma_BITMode,
        DeGamma_SW_Entry_129,
        csvPath,
        DeGCount, DeGPoint, DeGStep, DeGData);
}

API_EXPORT U32 Set_DeG10to14Config(
    IN U04 Switch,
    IN const char *ConfBin,
    OUT U32 *RegCount, OUT U32 *RegPoint, OUT U32 *RegStep, OUT U32 *RegData)
{
    return Set_DeGConfig(
        Switch,
        DeGamma_SW_BITMode_In10_Out14,
        DeGamma_SW_Entry_129,
        ConfBin,
        RegCount, RegPoint, RegStep, RegData);
}


API_EXPORT void Set_DeGamma_Config_packed(IN DeGamma_User_Config* DeGamma_User_Config, OUT DeGamma_SW_Config* DeGamma_Config)
{
    Set_DeGConfig(
        DeGamma_User_Config->DeGamma_Mode, 
        DeGamma_User_Config->DeGamma_BITMode, 
        DeGamma_User_Config->DeGamma_EntryMode,
        DeGamma_User_Config->csvPath, 
        &(DeGamma_Config->DeGCount), 
        DeGamma_Config->DeGPoint,
        DeGamma_Config->DeGStep, 
        DeGamma_Config->DeGData);
}