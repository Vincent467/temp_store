#include "sw_base.hpp"

#include "lut/p3_to_sRGB_ccm_9x9x9.h"
#include "lut/p3_to_sRGB_ccm_17x17x17.h"
#include "lut/p3_to_sRGB_improve_k0_8_s0_6_9x9x9.h"
#include "lut/p3_to_sRGB_improve_k0_8_s0_6_17x17x17.h"
#include "lut/p3_to_sRGB_perceptual_9x9x9.h"
#include "lut/p3_to_sRGB_perceptual_17x17x17.h"
#include "lut/sRGB_to_p3_ccm_9x9x9.h"
#include "lut/sRGB_to_p3_ccm_17x17x17.h"
#include "lut/sRGB_to_p3_improve_k0_8_s0_4_9x9x9.h"
#include "lut/sRGB_to_p3_improve_k0_8_s0_4_17x17x17.h"
#include "lut/sRGB_to_p3_perceptual_9x9x9.h"
#include "lut/sRGB_to_p3_perceptual_17x17x17.h"


API_EXPORT void Set_LUT3D_Config(
    IN uint32_t LUT3D_Mode, 
    IN uint32_t Gamut_Mode, 
    IN uint32_t bitwidth,
    IN const char *LUT3D_Programmable, 
    OUT LUT3D_SW_Config* LUT3D_Config)
{
    assert(LUT3D_Mode_START < LUT3D_Mode && LUT3D_Mode < LUT3D_Mode_END);

    LUT3D_Config->LUT3DMode = LUT3D_Mode;

    if (Gamut_Mode == LUT3D_Gamut_Mode_Programmable)
    {
        const char *dot = strchr(LUT3D_Programmable, '.');
        if (dot == NULL || strcmp(dot + 1, "csv") != 0) {
            printf("Unrecognized file name!");
            exit(EXIT_FAILURE);
        }

        FILE *ThreeDLutCSV14b = fopen(LUT3D_Programmable, "r");
        if (ThreeDLutCSV14b == NULL) {
            printf("Open file %s failed!\n", LUT3D_Programmable);
            exit(EXIT_FAILURE);
        }

        char Line[50];
        char *token;
        int index = 0;
        fgets(Line, sizeof(Line), ThreeDLutCSV14b);
        switch (LUT3D_Mode)
        {
        case LUT3D_Mode_9_9_9:
        {
            while (fgets(Line, sizeof(Line), ThreeDLutCSV14b)) {
                Line[strcspn(Line, "\r\n")] = 0;

                token = strtok(Line, ",");
                unsigned int colnum = 0;
                while (token != NULL) {
                    if (colnum >= 3) break;

                    LUT3D_Config->Lut3D_9_9_9[index][colnum] = (int16_t)(atof(token) * ((1 << bitwidth) - 1) + 0.5);

                    token = strtok(NULL, ",");
                    colnum++;
                }
                index++;
                if (index == INDEX_NUM_9_9_9) { break; }

            }
        }
        break;
        case LUT3D_Mode_17_17_17:
        {
            while (fgets(Line, sizeof(Line), ThreeDLutCSV14b)) {
                Line[strcspn(Line, "\r\n")] = 0;

                token = strtok(Line, ",");
                unsigned int colnum = 0;
                while (token != NULL) {
                    if (colnum >= 3) { break; }

                    LUT3D_Config->Lut3D_17_17_17[index][colnum] = (int16_t)(atof(token) * ((1 << bitwidth) - 1) + 0.5);

                    token = strtok(NULL, ",");
                    colnum++;
                }
                index++;
                if (index == INDEX_NUM_17_17_17) { break; }

            }
        }
        break;
        default:
            break;

        }

        fclose(ThreeDLutCSV14b);
    }
    else
    {
        const float* temp_ptr0 = NULL;
        const float* temp_ptr1 = NULL;
        const float* temp_ptr2 = NULL;

        if (LUT3D_Mode == LUT3D_Mode_9_9_9)
        {
            if (Gamut_Mode == LUT3D_Gamut_Mode_SRGBtoP3_CCM)
            {
                temp_ptr0 = sRGB_to_p3_ccm_9x9x9_0; temp_ptr1 = sRGB_to_p3_ccm_9x9x9_1; temp_ptr2 = sRGB_to_p3_ccm_9x9x9_2;
            }
            else if (Gamut_Mode == LUT3D_Gamut_Mode_SRGBtoP3_IMPROVE)
            {
                temp_ptr0 = sRGB_to_p3_improve_k0_8_s0_4_9x9x9_0; temp_ptr1 = sRGB_to_p3_improve_k0_8_s0_4_9x9x9_1; temp_ptr2 = sRGB_to_p3_improve_k0_8_s0_4_9x9x9_2;
            }
            else if (Gamut_Mode == LUT3D_Gamut_Mode_SRGBtoP3_PERCEPTUAL)
            {
                temp_ptr0 = sRGB_to_p3_perceptual_9x9x9_0; temp_ptr1 = sRGB_to_p3_perceptual_9x9x9_1; temp_ptr2 = sRGB_to_p3_perceptual_9x9x9_2;
            }
            else if (Gamut_Mode == LUT3D_Gamut_Mode_P3toSRGB_CCM)
            {
                temp_ptr0 = p3_to_sRGB_ccm_9x9x9_0; temp_ptr1 = p3_to_sRGB_ccm_9x9x9_1; temp_ptr2 = p3_to_sRGB_ccm_9x9x9_2;
            }
            else if (Gamut_Mode == LUT3D_Gamut_Mode_P3toSRGB_IMPROVE)
            {
                temp_ptr0 = p3_to_sRGB_improve_k0_8_s0_6_9x9x9_0; temp_ptr1 = p3_to_sRGB_improve_k0_8_s0_6_9x9x9_1; temp_ptr2 = p3_to_sRGB_improve_k0_8_s0_6_9x9x9_2;
            }
            else if (Gamut_Mode == LUT3D_Gamut_Mode_P3toSRGB_PERCEPTUAL)
            {
                temp_ptr0 = p3_to_sRGB_perceptual_9x9x9_0; temp_ptr1 = p3_to_sRGB_perceptual_9x9x9_1; temp_ptr2 = p3_to_sRGB_perceptual_9x9x9_2;

            }
            else
            {
                LOGE("unsupported combination LUT3D_Mode:%d,Gamut_Mode:%d\n", LUT3D_Mode, Gamut_Mode);
                assert(0);
                return;
            }

            if (temp_ptr0 == NULL || temp_ptr1 == NULL || temp_ptr2 == NULL)
            {
                LOGE("One or more LUT pointers are NULL for LUT3D_Mode:%d,Gamut_Mode:%d\n",
                     LUT3D_Mode, Gamut_Mode);
                assert(0);
                return;
            }

            for (int i = 0; i < INDEX_NUM_9_9_9; i++)
            {
                LUT3D_Config->Lut3D_9_9_9[i][0] = (int16_t)(temp_ptr0[i] * ((1 << bitwidth) - 1) + 0.5);
                LUT3D_Config->Lut3D_9_9_9[i][1] = (int16_t)(temp_ptr1[i] * ((1 << bitwidth) - 1) + 0.5);
                LUT3D_Config->Lut3D_9_9_9[i][2] = (int16_t)(temp_ptr2[i] * ((1 << bitwidth) - 1) + 0.5);
            }
        }
        else if (LUT3D_Mode == LUT3D_Mode_17_17_17)
        {
            if (Gamut_Mode == LUT3D_Gamut_Mode_SRGBtoP3_CCM)
            {
                temp_ptr0 = sRGB_to_p3_ccm_17x17x17_0; temp_ptr1 = sRGB_to_p3_ccm_17x17x17_1; temp_ptr2 = sRGB_to_p3_ccm_17x17x17_2;

            }
            else if (Gamut_Mode == LUT3D_Gamut_Mode_SRGBtoP3_IMPROVE)
            {
                temp_ptr0 = sRGB_to_p3_improve_k0_8_s0_4_17x17x17_0; temp_ptr1 = sRGB_to_p3_improve_k0_8_s0_4_17x17x17_1; temp_ptr2 = sRGB_to_p3_improve_k0_8_s0_4_17x17x17_2;

            }
            else if (Gamut_Mode == LUT3D_Gamut_Mode_SRGBtoP3_PERCEPTUAL)
            {
                temp_ptr0 = sRGB_to_p3_perceptual_17x17x17_0; temp_ptr1 = sRGB_to_p3_perceptual_17x17x17_1; temp_ptr2 = sRGB_to_p3_perceptual_17x17x17_2;

            }
            else if (Gamut_Mode == LUT3D_Gamut_Mode_P3toSRGB_CCM)
            {
                temp_ptr0 = p3_to_sRGB_ccm_17x17x17_0; temp_ptr1 = p3_to_sRGB_ccm_17x17x17_1; temp_ptr2 = p3_to_sRGB_ccm_17x17x17_2;

            }
            else if (Gamut_Mode == LUT3D_Gamut_Mode_P3toSRGB_IMPROVE)
            {
                temp_ptr0 = p3_to_sRGB_improve_k0_8_s0_6_17x17x17_0; temp_ptr1 = p3_to_sRGB_improve_k0_8_s0_6_17x17x17_1; temp_ptr2 = p3_to_sRGB_improve_k0_8_s0_6_17x17x17_2;

            }
            else if (Gamut_Mode == LUT3D_Gamut_Mode_P3toSRGB_PERCEPTUAL)
            {
                temp_ptr0 = p3_to_sRGB_perceptual_17x17x17_0; temp_ptr1 = p3_to_sRGB_perceptual_17x17x17_1; temp_ptr2 = p3_to_sRGB_perceptual_17x17x17_2;

            }
            else
            {
                LOGE("unsupported combination LUT3D_Mode:%d,Gamut_Mode:%d\n", LUT3D_Mode, Gamut_Mode);
                assert(0);
                return;
            }

            if (temp_ptr0 == NULL || temp_ptr1 == NULL || temp_ptr2 == NULL)
            {
                LOGE("One or more LUT pointers are NULL for LUT3D_Mode:%d,Gamut_Mode:%d\n",
                     LUT3D_Mode, Gamut_Mode);
                assert(0);
                return;
            }

            for (int i = 0; i < INDEX_NUM_17_17_17; i++)
            {
                LUT3D_Config->Lut3D_17_17_17[i][0] = (int16_t)(temp_ptr0[i] * ((1 << bitwidth) - 1) + 0.5);
                LUT3D_Config->Lut3D_17_17_17[i][1] = (int16_t)(temp_ptr1[i] * ((1 << bitwidth) - 1) + 0.5);
                LUT3D_Config->Lut3D_17_17_17[i][2] = (int16_t)(temp_ptr2[i] * ((1 << bitwidth) - 1) + 0.5);
            }
        }
        else
        {
            LOGE("unsupported LUT3D_Mode:%d\n", LUT3D_Mode);
            assert(0);
        }
    }

    return;
}

API_EXPORT void Set_LUT3D_Config_packed(IN LUT3D_User_Config* LUT3D_User_Config, OUT LUT3D_SW_Config* LUT3D_Config)
{
    Set_LUT3D_Config(
        LUT3D_User_Config->LUT3D_Mode, 
        LUT3D_User_Config->LUT3D_Gamut_Mode, 
        LUT3D_User_Config->bitwidth, 
        LUT3D_User_Config->LUT3D_Programmable, 
        LUT3D_Config);
}
