/******************************************************************************\
|* Copyright (c) 2008 by Vivante Corporarion.  All Rights Reserved.           *|
|*                                                                            *|
|* The material in this file is confidential and contains trade secrets of    *|
|* of Vivante Corporation.  This is proprietary information owned by Vivante  *|
|* Corporation.  No part of this work may be disclosed, reproduced, copied,   *|
|* transmitted, or used in any way for any purpose, without the express       *|
|* written permission of Vivante Corporation.                                 *|
|*                                                                            *|
|* Main C-Model include file.                                                 *|
|*                                                                            *|
\******************************************************************************/

#ifndef __SW_BASE__
#define __SW_BASE__

#ifdef _WIN32
#pragma warning(push, 3)
#endif

#ifndef ANON
#   define ANON(n)
#endif

#ifndef IN
#   define IN
#endif

#ifndef OUT
#   define OUT
#endif

#ifndef IO
#define IO
#endif

#ifndef FALSE
#   define FALSE            0
#endif

#ifndef TRUE
#   define TRUE             1
#endif

#ifndef NULL
#   define NULL             0
#endif


#if defined(_WIN32)
#define API_EXPORT __declspec(dllexport)
#elif defined(__linux__)
#define API_EXPORT __attribute__((visibility("default")))
#else
#define API_EXPORT
#endif

#ifndef ASSERT
#ifdef REGISTER_TEST
#   define ASSERT(exp)
#else
#   define ASSERT(exp) \
        do \
        { \
            if (!(exp)) \
            { \
                printf( \
                    "ASSERT in %s.%s(%d): %s\n", \
                    __FILE__, __FUNCTION__, __LINE__, #exp \
                    ); \
                exit(EXIT_FAILURE); \
            } \
        } \
        while (0)
#endif /* REGISTER_TEST */
#endif /* ASSERT */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <math.h>
#include <float.h>
#include <time.h>

#ifndef ABS
#define ABS(x) ((x)<0 ? -(x):(x))
#endif // !ABS

#ifndef MIN
#define MIN(a,b) (a>b?b:a)
#endif

#ifndef MAX
#define MAX(a,b) (a>b?a:b)
#endif

#ifdef  _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif //  _WIN32

#include "aqcmbitfield.h"

#include "parse_csv.h"

#include "algo_log.h"

// generate by makefile
#include "sw_def.hpp"

// SW inc
// it should be at bottom
#include "sw_modules.hpp"

#endif /* AQCM_H */
