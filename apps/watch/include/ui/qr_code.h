#ifndef _JL_CODE_H
#define _JL_CODE_H
enum code_result {
    success = 1,
    Input_mode_err,
    EAN_13_input_err,
    EAN_13_output_err,
    CODE_39_input_err,
    CODE_39_input_data_err,
    CODE_39_output_err,
    CODE_128_input_err,
    CODE_128_output_err,
    QR_CODE_input_err,
    QR_CODE_check_ecc_err,
    QR_CODE_version_err,
    QR_CODE_add_ecc_err,
};
typedef struct {
    int l_size;
} jl_code_param_t;
void jl_code_init(int code128_mode, unsigned char qr_version, unsigned char qr_max_version, unsigned char qr_ecc_level, int qr_code_max_input_len_, int qr_buf_size, int img_w);
int jl_code_process(unsigned char mode_t, char *src, int length, int *out_len, int *line_size);
void jl_code_set_info(jl_code_param_t *jl_code_param_);
void jl_code_get_data(int len, int row, unsigned char *outdata);
void jl_code_deinit();
#endif
