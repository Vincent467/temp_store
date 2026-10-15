#ifndef __MAP_HDR_H
#define __MAP_HDR_H
#include <stdint.h>
#include "wrapperConfig.h"

#define TM_DATA_MUN 33
#define PRINT 0

typedef enum _dtest_status {
    DTEST_NOT_SUPPORT = -2,
    DTEST_FAILURE = -1,
    DTEST_SUCCESS
} dtest_status;

/*export API*/
dtest_status dpu_algo_api_hdr(
    void *algo_config,
    void *hw_config);

#endif