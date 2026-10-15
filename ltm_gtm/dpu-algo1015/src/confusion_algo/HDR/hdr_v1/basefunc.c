#include "sw_base.hpp"
#include "basefunc.h"

int LoadCSV_Curve(
        const char * const filePath,
        double * const Curve, U32 *CurveNum)
{
    assert(NULL != filePath);
    assert(NULL != Curve);
    assert(NULL != CurveNum);

    FILE *curveFile = fopen(filePath, "r");
    if( NULL == curveFile ) { printf(" [%s:%d] '%s' open failed!\n", __FUNCTION__, __LINE__, filePath); return EXIT_FAILURE; }

    char Line[100];

    if( NULL != fgets(Line, 100, curveFile) )
    {
        *CurveNum = (U32)atoi(Line);
    }
    else { printf(" [%s:%d] the file '%s' mismatch.\n", __FUNCTION__, __LINE__, filePath); fclose(curveFile); return EXIT_FAILURE; }

    U32 index = 0;
    while( fgets(Line, 100, curveFile) )
    {
        if( 0 == strlen(Line) || '\0' == Line[0] || '\n' == Line[0] ) break;
        Curve[index] = atof(Line);
        index++;
    }

    if( *CurveNum != index )
        { printf(" [%s:%d] the CurveNum(:%d) and number of point(:%d) are inconsistent!\n", __FUNCTION__, __LINE__, *CurveNum, index); fclose(curveFile); return EXIT_FAILURE; }

    if(NULL != curveFile) fclose(curveFile);

    return EXIT_SUCCESS;
}

int LoadCsv_TmConf(
        const char *filePath,
        U32 *Mode, double *Weight, double *Coef, double * const Curve, U32 *CurveNum)
{
    assert(NULL != filePath);
    assert(NULL != Mode);
    assert(NULL != Weight);
    assert(NULL != Coef);
    assert(NULL != Curve);
    assert(NULL != CurveNum);

    FILE *fileCsv = fopen(filePath, "r");
    if( NULL == fileCsv ) 
    { 
        printf(" [%s:%d] '%s' open failed!\n", __FUNCTION__, __LINE__, filePath); 
        return EXIT_FAILURE; 
    }

    char line[3000];
    char *pchr=NULL;
    char *prepchr=NULL;
    int chrlen;
    char data[100];
    int i=0;

    memset(data, 0, sizeof(data));

    // get Mode
    if( NULL != fgets(line, 3000, fileCsv) )
    {
        pchr = strchr(line, ',');
        if(NULL == pchr) { printf(" [%s:%d] the file '%s' mismatch.\n", __FUNCTION__, __LINE__, filePath); fclose(fileCsv); return EXIT_FAILURE; }

        pchr++;

        char* dashPos = strchr(pchr, ',');
        if(NULL == dashPos) { printf(" [%s:%d] the file '%s' mismatch.\n", __FUNCTION__, __LINE__, filePath); fclose(fileCsv); return EXIT_FAILURE; }
        chrlen = dashPos - (pchr);
        if (chrlen<0 || chrlen >= 100) { printf(" [%s:%d] the file '%s' mismatch.\n", __FUNCTION__, __LINE__, filePath); fclose(fileCsv); return EXIT_FAILURE; }

        memcpy( data, pchr, chrlen * sizeof(char) );
        data[chrlen] = '\0';

        *Mode = atoi(data);
    }
    else { printf(" [%s:%d] the file '%s' mismatch.\n", __FUNCTION__, __LINE__, filePath); fclose(fileCsv); return EXIT_FAILURE; }

    // get Weight
    if( NULL != fgets(line, 3000, fileCsv) )
    {
        pchr = strchr(line, ',');
        if(NULL == pchr) { printf(" [%s:%d] the file '%s' mismatch.\n", __FUNCTION__, __LINE__, filePath); fclose(fileCsv); return EXIT_FAILURE; }

        pchr++;
        char* dashPos = strchr(pchr, ',');
        if (NULL == dashPos) { printf(" [%s:%d] the file '%s' mismatch.\n", __FUNCTION__, __LINE__, filePath); fclose(fileCsv); return EXIT_FAILURE; }
        chrlen = dashPos - (pchr);
        if (chrlen < 0) { printf(" [%s:%d] the file '%s' mismatch.\n", __FUNCTION__, __LINE__, filePath); fclose(fileCsv); return EXIT_FAILURE; }

        memcpy( data, pchr, (chrlen+1) * sizeof(char) );
        *Weight = atof(data);
    }
    else { printf(" [%s:%d] the file '%s' mismatch.\n", __FUNCTION__, __LINE__, filePath); fclose(fileCsv); return EXIT_FAILURE; }

    // get Coef
    if( NULL != fgets(line, 3000, fileCsv) )
    {
        pchr = strchr(line, ',');
        if(NULL == pchr) { printf(" [%s:%d] the file '%s' mismatch.\n", __FUNCTION__, __LINE__, filePath); fclose(fileCsv); return EXIT_FAILURE; }

        for(i=0; i < 3; i++)
        {
            pchr++;
            if( '\n' == *pchr || '\0' == *pchr ) break;
            prepchr = pchr;
            pchr = strchr(pchr, ',');
            if(NULL == pchr) { printf(" [%s:%d] the file '%s' mismatch.\n", __FUNCTION__, __LINE__, filePath); fclose(fileCsv); return EXIT_FAILURE; }

            chrlen = pchr - prepchr;
            if( chrlen <= 0 ) break;
            memset( data, 0, sizeof(data) );
            memcpy( data, prepchr, chrlen * sizeof(char) );
            Coef[i] = atof(data);
        }
    }
    else { printf(" [%s:%d] the file '%s' mismatch.\n", __FUNCTION__, __LINE__, filePath); fclose(fileCsv); return EXIT_FAILURE; }

    // get Curve
    if( NULL != fgets(line, 100, fileCsv) )
    {
        *CurveNum = (U32)atoi(line);
    }
    else { printf(" [%s:%d] the file '%s' mismatch.\n", __FUNCTION__, __LINE__, filePath); fclose(fileCsv); return EXIT_FAILURE; }

    U32 index = 0;
    while( fgets(line, 100, fileCsv) )
    {
        if( 0 == strlen(line) || '\0' == line[0] || '\n' == line[0] ) break;
        Curve[index] = atof(line);
        index++;
    }

    if( *CurveNum != index )
        { printf(" [%s:%d] the CurveNum(:%d) and number of point(:%d) are inconsistent!\n", __FUNCTION__, __LINE__, *CurveNum, index); fclose(fileCsv); return EXIT_FAILURE; }

    if(NULL != fileCsv) fclose(fileCsv);

    return EXIT_SUCCESS;
}

int LoadCsv_CcmCoef(
        const char *filePath,
        double *RegCoef, double *RegOffset)
{
    assert(NULL != filePath);
    assert(NULL != RegCoef);
    assert(NULL != RegOffset);

    FILE *LutCsv = fopen(filePath, "r");
    if( NULL == LutCsv ) { printf(" [%s:%d] '%s' open failed!\n", __FUNCTION__, __LINE__, filePath); return EXIT_FAILURE; }

    char line[3000];
    char *pchr=NULL;
    char *prepchr=NULL;
    int chrlen;
    char data[20];
    int i=0;

    // get Coef
    if( NULL != fgets(line, 3000, LutCsv) )
    {
        pchr = strchr(line, ',');
        if(NULL == pchr) { printf(" [%s:%d] the file '%s' mismatch.\n", __FUNCTION__, __LINE__, filePath); fclose(LutCsv); return EXIT_FAILURE; }

        for(i=0; i < 9; i++)
        {
            pchr++;
            if( '\n' == *pchr || '\0' == *pchr ) { printf(" [%s:%d] the file '%s' mismatch.\n", __FUNCTION__, __LINE__, filePath); fclose(LutCsv); return EXIT_FAILURE; }
            prepchr = pchr;
            pchr = strchr(pchr, ',');
            if(NULL == pchr) { printf(" [%s:%d] the file '%s' mismatch.\n", __FUNCTION__, __LINE__, filePath); fclose(LutCsv); return EXIT_FAILURE; }

            chrlen = pchr - prepchr;
            if( chrlen <= 0 ) { printf(" [%s:%d] the file '%s' mismatch.\n", __FUNCTION__, __LINE__, filePath); fclose(LutCsv); return EXIT_FAILURE; }
            memset( data, 0, sizeof(data) );
            memcpy( data, prepchr, chrlen * sizeof(char) );
            RegCoef[i] = atof(data);
        }
    }
    else { printf(" [%s:%d] the file '%s' mismatch.\n", __FUNCTION__, __LINE__, filePath); fclose(LutCsv); return EXIT_FAILURE; }


    // get Offset
    if( NULL != fgets(line, 3000, LutCsv) )
    {
        pchr = strchr(line, ',');
        if(NULL == pchr) { printf(" [%s:%d] the file '%s' mismatch.\n", __FUNCTION__, __LINE__, filePath); fclose(LutCsv); return EXIT_FAILURE; }

        for(i=0; i < 3; i++)
        {
            pchr++;
            if( '\n' == *pchr || '\0' == *pchr ) { printf(" [%s:%d] the file '%s' mismatch.\n", __FUNCTION__, __LINE__, filePath); fclose(LutCsv); return EXIT_FAILURE; }
            prepchr = pchr;
            pchr = strchr(pchr, ',');
            if(NULL == pchr) { printf(" [%s:%d] the file '%s' mismatch.\n", __FUNCTION__, __LINE__, filePath); fclose(LutCsv); return EXIT_FAILURE; }

            chrlen = pchr - prepchr;
            if( chrlen <= 0 ) { printf(" [%s:%d] the file '%s' mismatch.\n", __FUNCTION__, __LINE__, filePath); fclose(LutCsv); return EXIT_FAILURE; }
            memset( data, 0, sizeof(data) );
            memcpy( data, prepchr, chrlen * sizeof(char) );
            RegOffset[i] = atof(data);
        }
    }
    else { printf(" [%s:%d] the file '%s' mismatch.\n", __FUNCTION__, __LINE__, filePath); fclose(LutCsv);  return EXIT_FAILURE; }

//    printf(" [Reg] RegCoef[%d]: [ ", 9);
//    for(i = 0; i < 9; i++)
//        printf("%f, ", RegCoef[i]);
//    printf("]\n");
//    printf("       RegOffset[%d]: [ ", 3);
//    for(i = 0; i < 3; i++)
//        printf("%f, ", RegOffset[i]);
//    printf("]\n");


    if(NULL != LutCsv) fclose(LutCsv);

    return EXIT_SUCCESS;
}

int LoadCsv_CscCoef(
        const char *filePath,
        double *RegCoef, U32 *RegPreOffset, U32 *RegPostOffset)
{
    assert(NULL != filePath);
    assert(NULL != RegCoef);
    assert(NULL != RegPreOffset);
    assert(NULL != RegPostOffset);

    FILE *LutCsv = fopen(filePath, "r");
    if( NULL == LutCsv ) { printf(" [%s:%d] '%s' open failed!\n", __FUNCTION__, __LINE__, filePath); return EXIT_FAILURE; }

    char line[3000];
    char *pchr=NULL;
    char *prepchr=NULL;
    int chrlen;
    char data[20];
    int Status = EXIT_SUCCESS;
    int i=0;

    // get Coef
    if( NULL != fgets(line, 3000, LutCsv) )
    {
        pchr = strchr(line, ',');
        if(NULL == pchr) { Status = EXIT_FAILURE; goto File_Error; }

        for(i=0; i < 9; i++)
        {
            pchr++;
            if( '\n' == *pchr || '\0' == *pchr ) { Status = EXIT_FAILURE; goto File_Error; }
            prepchr = pchr;
            pchr = strchr(pchr, ',');
            if(NULL == pchr)
            {
                if( 8 == i )
                {
                    RegCoef[i] = atof(prepchr);
                }
                else { Status = EXIT_FAILURE; goto File_Error; }
            }
            else
            {
                chrlen = pchr - prepchr;
                if( chrlen <= 0 ) { Status = EXIT_FAILURE; goto File_Error; }
                memset( data, 0, sizeof(data) );
                memcpy( data, prepchr, chrlen * sizeof(char) );
                RegCoef[i] = atof(data);
            }
        }
    }
    else { Status = EXIT_FAILURE; goto File_Error; }


    // get PreOffset
    if( NULL != fgets(line, 3000, LutCsv) )
    {
        pchr = strchr(line, ',');
        if(NULL == pchr) { Status = EXIT_FAILURE; goto File_Error; }

        for(i=0; i < 3; i++)
        {
            pchr++;
            if( '\n' == *pchr || '\0' == *pchr ) { Status = EXIT_FAILURE; goto File_Error; }
            prepchr = pchr;
            pchr = strchr(pchr, ',');
            if(NULL == pchr) { Status = EXIT_FAILURE; goto File_Error; }

            chrlen = pchr - prepchr;
            if( chrlen <= 0 ) { Status = EXIT_FAILURE; goto File_Error; }
            memset( data, 0, sizeof(data) );
            memcpy( data, prepchr, chrlen * sizeof(char) );
            RegPreOffset[i] = atoi(data);
        }
    }
    else { Status = EXIT_FAILURE; goto File_Error; }


    // get PostOffset
    if( NULL != fgets(line, 3000, LutCsv) )
    {
        pchr = strchr(line, ',');
        if(NULL == pchr) { Status = EXIT_FAILURE; goto File_Error; }

        for(i=0; i < 3; i++)
        {
            pchr++;
            if( '\n' == *pchr || '\0' == *pchr ) { Status = EXIT_FAILURE; goto File_Error; }
            prepchr = pchr;
            pchr = strchr(pchr, ',');
            if(NULL == pchr) { Status = EXIT_FAILURE; goto File_Error; }

            chrlen = pchr - prepchr;
            if( chrlen <= 0 ) { Status = EXIT_FAILURE; goto File_Error; }
            memset( data, 0, sizeof(data) );
            memcpy( data, prepchr, chrlen * sizeof(char) );
            RegPostOffset[i] = atoi(data);
        }
    }
    else { Status = EXIT_FAILURE; goto File_Error; }


//    printf(" [Reg] RegCoef[%d]: [ ", 9);
//    for(i = 0; i < 9; i++)
//        printf("%f, ", RegCoef[i]);
//    printf("]\n");
//    printf("       RegPreOffset[%d]: [ ", 3);
//    for(i = 0; i < 3; i++)
//        printf("%f, ", RegPreOffset[i]);
//    printf("]\n");
//    printf("       RegPostOffset[%d]: [ ", 3);
//    for(i = 0; i < 3; i++)
//        printf("%f, ", RegPostOffset[i]);
//    printf("]\n");

File_Error:
    if( EXIT_FAILURE == Status ) printf(" [Error]: the file '%s' mismatch.\n", filePath);

    if(NULL != LutCsv) fclose(LutCsv);

    return Status;
}

/*  both gcc and g++ is ok. */
void GetStrSuffix(const char *str, char *suffix)
{
    assert(NULL != str);
    assert(NULL != suffix);

    int strn = strlen(str) +1;
    char *s = (char *)malloc( strn * sizeof(char) );
    if (NULL == s) {printf(" memory allocation failed.\n"); return; }

    /* memcpy( s, str, strn * sizeof(char) ); */
    strcpy(s, str);

    int offset = strrchr(s, '.') - s;
    /* memcpy( suffix, s +offset +1, (strn -offset) * sizeof(char) ); */
    strcpy(suffix, s +offset +1);

    /* printf(" str[%d]: %s\n", strn, str); */
    /* printf(" offset: %d\n", offset); */
    /* printf(" suffix: %s\n\n", suffix); */

    if(NULL != s) free(s);
}

/* ------------------------------------------------------------------ */
/*  MIN and MAX templates. */
U64 max_U64(
    const U64 _left,
    const U64 _right
    )
{
    return (_left < _right)
        ? _right
        : _left;
}

S64 max_S64(
    const S64 _left,
    const S64 _right
    )
{
    return (_left < _right)
        ? _right
        : _left;
}

double max_double(
    const double _left,
    const double _right
    )
{
    return (_left < _right)
        ? _right
        : _left;
}

U64 min_U64(
    const U64 _left,
    const U64 _right
    )
{
    return (_right < _left)
        ? _right
        : _left;
}

S64 min_S64(
    const S64 _left,
    const S64 _right
    )
{
    return (_right < _left)
        ? _right
        : _left;
}

double min_double(
    const double _left,
    const double _right
    )
{
    return (_right < _left)
        ? _right
        : _left;
}

U32 maxArray_U32(
    const U32 * const array,
    const U32 size
    )
{
    U32 max = array[0];
    U32 i=0;
    U32 index=0;

    for(i=1; i < size; i++)
    {
        if(max < array[i])
        {
            max = array[i];
            index = i;
        }
    }

    return index;
}

U32 maxArray_U64(
    const U64 * const array,
    const U32 size
    )
{
    U64 max = array[0];
    U32 i=0;
    U32 index=0;

    for(i=1; i < size; i++)
    {
        if(max < array[i])
        {
            max = array[i];
            index = i;
        }
    }

    return index;
}

U32 maxArray_double(
    const double * const array,
    const U32 size
    )
{
    double max = array[0];
    U32 i=0;
    U32 index=0;

    for(i=1; i < size; i++)
    {
        if(max < array[i])
        {
            max = array[i];
            index = i;
        }
    }

    return index;
}

U32 minArray_U32(
    const U32 * const array,
    const U32 size
    )
{
    U32 min = array[0];
    U32 i=0;
    U32 index=0;

    for(i=1; i < size; i++)
    {
        if(array[i] < min)
        {
            min = array[i];
            index = i;
        }
    }

    return index;
}

U32 minArray_U64(
    const U64 * const array,
    const U32 size
    )
{
    U64 min = array[0];
    U32 i=0;
    U32 index=0;

    for(i=1; i < size; i++)
    {
        if(array[i] < min)
        {
            min = array[i];
            index = i;
        }
    }

    return index;
}

U32 minArray_double(
    const double * const array,
    const U32 size
    )
{
    double min = array[0];
    U32 i=0;
    U32 index=0;

    for(i=1; i < size; i++)
    {
        if(array[i] < min)
        {
            min = array[i];
            index = i;
        }
    }

    return index;
}

int ExistInArray_U32(
    const U32 * const array,
    const U32 size,
    const U32 value
    )
{
    int i;
    for(i=0; i < size; i++)
    {
        if(value == array[i])
        {
            return i;
        }
    }

    return -1;
}

U64 Clamp_U64(
    const U64 value,
    const U64 low,
    const U64 high)
{
    /* U64 result=0; */
    /* result = max_U64(value, low); */
    /* result = min_U64(result, high); */
    /* return result; */
    return min_U64( max_U64(value, low), high);
}

S64 Clamp_S64(
    const S64 value,
    const S64 low,
    const S64 high)
{
    /* U64 result=0; */
    /* result = max_U64(value, low); */
    /* result = min_U64(result, high); */
    /* return result; */
    return min_S64( max_S64(value, low), high);
}

double Clamp_double(
    double value,
    double low,
    double high)
{
    /* double result=0; */
    /* result = fmax(value, low); */
    /* result = fmin(result, high); */
    /* return result; */

    return fmin( fmax(value, low), high);
}

U64 Lerp(
    const U64 y1,
    const U64 y2,
    const U64 weight)
{
    return y1 * (1 - weight) + y2 * weight;
}

U64 SHIFTRIGHTROUND(
    const U64 Data,
    const U32 Count)
{
    return (Data + (1 << (Count-1))) >> Count;
}

/* ------------------------------------------------------------------ */
/*  Compute bit mask. */
U64 COMPUTEMASK(
    const U32 Position,
    const U32 Count
    )
{
    /* Define the result. */
    U64 result;

    /* Compute the mask. */
    result = Count
        ? ((U64) ~0) >> (64 - Count)
        : 0;

    /* Shift to the proper position. */
    result = result << Position;

    /* Return result. */
    return (U64) result;
}

/* ------------------------------------------------------------------ */
/* Extract bits from a value. */
U64 GETBITS(
    const U64 Data,
    const U32 Start,
    const U32 End
    )
{
    /* Sort the end points. */
    const U32 _Start = (U32)min_U64(Start, End);
    const U32 _End   = (U32)max_U64(Start, End);

    /* Determine the size of the field. */
    const U32 _Size = _End - _Start + 1;

    /* Compute the mask. */
    const U64 _Mask = COMPUTEMASK(0, _Size);

    /* Extract the value. */
    const U64 _Value = (Data >> _Start) & _Mask;

    /* Return the result. */
    return _Value;
}

