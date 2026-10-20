#ifndef __MAP_GTM_H
#define __MAP_GTM_H

#include <stdint.h>
#include <stdbool.h>

typedef enum _dtest_status {
	DTEST_NOT_SUPPORT = -2,
	DTEST_FAILURE = -1,
	DTEST_SUCCESS
} dtest_status;

/*export API: map the gtm cabc sw algo output to drm_vs_gtm regs config*/
dtest_status dpu_algo_api_gtm_cabc(uint32_t fr, bool temporal_smooth,
				  uint32_t Strength, uint32_t layerWidth,
				  uint32_t layerHeight, void *ReadParams,
				  void *cabcOut, void *hw_config);

#endif
