#ifndef __MAP_LTM_H
#define __MAP_LTM_H
#include "LTM.h"

#define PRINT 0

typedef enum _dtest_status {
	DTEST_NOT_SUPPORT = -2,
	DTEST_FAILURE = -1,
	DTEST_SUCCESS
} dtest_status;

dtest_status dpu_algo_api_ltm(
	uint32_t fr,
	uint32_t mode,
	bool temporal_smooth,
	float Strength,
	uint32_t layerWidth,
	uint32_t layerHeight,
	uint32_t gridWidth,
	uint32_t gridHeight,
	double sensitivity,
	double iir_max_wgt,
	uint16_t *spatial_kernal,
	uint32_t (*hist_buf)[64],
	uint32_t (*cdf_crv_last)[65],
	void *hw_config);

#endif

