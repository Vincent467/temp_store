#include "sw_base.hpp"
#include "map3DLUT.h"
#include "vs_drm.h"

#define DEBUG 0

dtest_status dpu_algo_api_3dlut(uint32_t mode, uint32_t gamut, uint32_t bitwidth, void *hw_lut)
{
	int i;
	struct drm_vs_color *lut = hw_lut;
	LUT3D_User_Config usr_config;
	LUT3D_SW_Config sw_config;

	usr_config.LUT3D_Mode = mode;
	usr_config.LUT3D_Gamut_Mode = gamut;
	usr_config.bitwidth = bitwidth;
	usr_config.LUT3D_Programmable = NULL;

	Set_LUT3D_Config_packed(&usr_config, &sw_config);

	if (mode == LUT3D_Mode_17_17_17) {
		for (i = 0; i < INDEX_NUM_17_17_17; i++) {
			lut[i].r = sw_config.Lut3D_17_17_17[i][0];
			lut[i].g = sw_config.Lut3D_17_17_17[i][1];
			lut[i].b = sw_config.Lut3D_17_17_17[i][2];
		}
	} else {
		for (i = 0; i < INDEX_NUM_9_9_9; i++) {
			lut[i].r = sw_config.Lut3D_9_9_9[i][0];
			lut[i].g = sw_config.Lut3D_9_9_9[i][1];
			lut[i].b = sw_config.Lut3D_9_9_9[i][2];
		}
	}

	return DTEST_SUCCESS;
}
