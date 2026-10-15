#ifndef __SCALER_CONFIG_HPP__
#define __SCALER_CONFIG_HPP__

#define SW_HOR_KERNEL_SIZE 8
#define SW_VER_KERNEL_SIZE 4
#define SW_SAMPLE_BINCOUNT 32
#define SW_TABLE_BINCOUNT (SW_SAMPLE_BINCOUNT / 2 + 1)

enum SW_SCALER_MODE {
    SW_SCALER_BI_LINEAR = 0,
    SW_SCALER_BI_CUBIC = 1,
    SW_SCALER_LANCZOS = 2,
    SW_SCALER_Programable = 3,
    SW_SCALER_END,
};


typedef struct
{
    // Stretch factor (FILTER_BLT), 15.16 format.
    int32_t InitialOffsetX; //U32
    int32_t InitialOffsetY; //U32
    uint64_t ScaleFactorX; //U39
    uint64_t ScaleFactorY; //U39
    uint8_t  ScalePipeMode; //U01

    // Filter blt states.
    uint16_t HoriKernelArray[SW_TABLE_BINCOUNT*SW_HOR_KERNEL_SIZE + 1]; //U16
    // Filter blt states.
    uint16_t VertiKernelArray[SW_TABLE_BINCOUNT*SW_VER_KERNEL_SIZE + 1];// U16
}SW_Scaler_Config;

typedef struct
{
    uint32_t scaler_mode;
    const char* fileName_hor;
    const char* fileName_ver;
    uint32_t scaler_bitwidth;
    uint32_t srcWidth;
    uint32_t srcHeight;
    uint32_t dstWidth;
    uint32_t dstHeight;
}Scaler_User_Config;

#ifdef __cplusplus
extern "C" {
#endif
    API_EXPORT void Set_Scaler_Config(IN Scaler_User_Config* Scaler_User_Config, OUT SW_Scaler_Config* Params);

    API_EXPORT uint64_t calc_factor(int input_size, int output_size);

    API_EXPORT int calc_initial_offset(int input_size, int output_size);

    /**
     * Returns the number of PPC blocks.
     *
     * @param input_size   Source size (width or height)
     * @param output_size  Destination size (width or height)
     * @param max_window   Hardware SCL input window limit (e.g. 512)
     * @return             Number of blocks
     */
    int get_num_ppc_blocks(int input_size, int output_size, int max_window);

    /**
     * Computes per-block parameters for SCL pipeline (1D blocking).
     *
     * @param[in]  input_size      Source size
     * @param[in]  output_size     Destination size
     * @param[in]  max_window      Hardware SCL input window limit (e.g. 512)
     * @param[out] factor          Scaling factor: (input_size-1)/(output_size-1)
     * @param[out] blk_num         Number of blocks (confirms allocation size)
     * @param[out] initial_offset  Block i: phase of first SCL output pixel in local coords
     * @param[out] in_start        Block i: start of SCL input window in source
     * @param[out] in_size         Block i: size of SCL input window (with overlap)
     * @param[out] out_start       Block i: valid start in destination (after crop)
     * @param[out] scl_out_size    Block i: size of SCL output (with overlap 2*2)
     * @param[out] out_valid_size  Block i: size of valid output (T-aligned except last)
     * @param[out] crop_start      Block i: crop offset within SCL output (typically 2, 0 at edges)
     */
    void compute_ppc_blocks(
        int    input_size,
        int    output_size,
        int    max_window,
        uint64_t *factor,
        int   *blk_num,
        int *initial_offset,
        int   *in_start,
        int   *in_size,
        int   *out_start,
        int   *scl_out_size,
        int   *out_valid_size,
        int   *crop_start);

#ifdef __cplusplus
}
#endif

#endif // !__POLYPHASE__HPP__
