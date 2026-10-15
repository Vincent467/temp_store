#ifndef __MAP_DEGAMMA_H
#define __MAP_DEGAMMA_H
#include <stdint.h>
#define PRINT 0

typedef enum _dtest_status {
    DTEST_NOT_SUPPORT = -2,
    DTEST_FAILURE = -1,
    DTEST_SUCCESS
} dtest_status;

/*export API*/
dtest_status dpu_algo_api_degamma(
	uint32_t mode,
	uint32_t in_bit,
	uint32_t out_bit,
	uint32_t size,
	char *lut_file,
	void *hw_config,
	bool enable);

#endif