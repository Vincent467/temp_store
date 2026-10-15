#ifndef __MAP_GTM_H
#define __MAP_GTM_H
#include "./mapLTM.h"


dtest_status dpu_algo_api_gtm(
	uint32_t fr,
	uint32_t mode,
	uint32_t vidMode,
	uint32_t Strength,
	uint32_t layerWidth,
	uint32_t layerHeight,
	void* hw_config);

#endif


