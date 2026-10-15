#ifndef __MAP_SCL_H
#define __MAP_SCL_H
#include <stdint.h>

typedef enum _dtest_status {
	DTEST_NOT_SUPPORT = -2,
	DTEST_FAILURE = -1,
	DTEST_SUCCESS
} dtest_status;

dtest_status dpu_algo_api_scl(
	uint32_t scaler_mode,
	uint32_t srcWidth,
	uint32_t srcHeight,
	uint32_t dstWidth,
	uint32_t dstHeight,
	void *hw_config);

#endif


