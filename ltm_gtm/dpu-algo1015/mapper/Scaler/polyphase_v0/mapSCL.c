#include "mapSCL.h"
#include "sw_base.hpp"
#include "vs_drm.h"

dtest_status dpu_algo_api_scl(uint32_t scaler_mode, uint32_t srcWidth, uint32_t srcHeight,
			      uint32_t dstWidth, uint32_t dstHeight, void *hw_config)
{
	dtest_status ret = DTEST_SUCCESS;
	SW_Scaler_Config hw_param;
	struct drm_vs_scale_config *scale = hw_config;
	Scaler_User_Config sw_config;

	sw_config.scaler_mode = scaler_mode;
	sw_config.srcWidth = srcWidth;
	sw_config.srcHeight = srcHeight;
	sw_config.dstWidth = dstWidth;
	sw_config.dstHeight = dstHeight;

	Set_Scaler_Config(&sw_config, &hw_param);

	scale->factor_x = hw_param.ScaleFactorX;
	scale->factor_y = hw_param.ScaleFactorY;

	scale->initial_offsetx = hw_param.InitialOffsetX;
	scale->initial_offsety = hw_param.InitialOffsetY;

	memcpy(scale->coef_h, hw_param.HoriKernelArray,
	       SW_TABLE_BINCOUNT * SW_HOR_KERNEL_SIZE * sizeof(int16_t));
	memcpy(scale->coef_v, hw_param.VertiKernelArray,
	       SW_TABLE_BINCOUNT * SW_VER_KERNEL_SIZE * sizeof(int16_t));

	return ret;
}
