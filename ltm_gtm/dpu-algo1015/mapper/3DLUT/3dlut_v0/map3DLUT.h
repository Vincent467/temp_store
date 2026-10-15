#ifndef __MAP_3DLUT_H
#define __MAP_3DLUT_H

typedef enum _dtest_status {
	DTEST_NOT_SUPPORT = -2,
	DTEST_FAILURE = -1,
	DTEST_SUCCESS
} dtest_status;

/*export API*/
dtest_status dpu_algo_api_3dlut(uint32_t mode, uint32_t gamut, uint32_t bitwidth, void *hw_lut);

#endif
