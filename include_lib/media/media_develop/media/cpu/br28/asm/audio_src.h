/*****************************************************************
>file name : audio_src.h
>author : lichao
>create time : Fri 14 Dec 2018 03:05:49 PM CST
*****************************************************************/
#ifndef _AUDIO_SRC_H_
#define _AUDIO_SRC_H_
#include "system/includes.h"


#define SRC_DATA_MIX    0//各通道数据交叉存放
#define SRC_DATA_SEP    1//各通道数据连续存放

#define SRC_CHI                     2
#define SRC_FILT_POINTS             24

#define SRC_TYPE_NONE               0
#define SRC_TYPE_RESAMPLE           1
#define SRC_TYPE_AUDIO_SYNC         2

#define AUDIO_ONLY_RESAMPLE         1
#define AUDIO_SYNC_RESAMPLE         2

#define AUDIO_SAMPLE_FMT_16BIT      0
#define AUDIO_SAMPLE_FMT_24BIT      1

#define BIND_AUDSYNC                0x10
#define SET_RESAMPLE_TYPE(fmt, type)    (((fmt) << 4) | (type))
#define RESAMPLE_TYPE_TO_FMT(a)         (((a) >> 4) & 0xf)
#define RESAMPLE_TYPE(a)                ((a) & 0xf)

#define INPUT_FRAME_BITS                            18//20 -- 整数位减少可提高单精度浮点的运算精度
#define RESAMPLE_INPUT_BIT_RANGE                    ((1 << INPUT_FRAME_BITS) - 1)
#define RESAMPLE_INPUT_BIT_NUM                      (1 << INPUT_FRAME_BITS)

#define RESAMPLE_HW_24BIT  1
enum audio_src_event {
    SRC_EVENT_INPUT_DONE = 0x0,
    SRC_EVENT_OUTPUT_DONE,
    SRC_EVENT_ALL_DONE,
    SRC_EVENT_GET_OUTPUT_BUF,
    SRC_EVENT_RATE_UPDATE,
    SRC_EVENT_RISE_IRQ,
};

enum audio_src_error_code {
    SRC_BASE_NO_ERROR = 0x0,
    SRC_OUTPUT_NO_BUFF = 0x4,
    SRC_OUTPUT_NOT_COMPLETED,
};

struct audio_src_rate {
    u16 irate;
    u16 orate;
};

struct audio_src_buffer {
    void *addr;
    int len;
};
/*
 * SRC BASE模块接口
 */

// *INDENT-OFF*
struct audio_src_base_handle {
    u8 channels;
    u8 state;
    volatile u8 active;
    u8 start;
    u8 input_malloc;
    u8 rate_update;
	//u8 filt_index;
    //u32 tuned_points;
    struct audio_src_rate rate;
    struct audio_src_buffer input;
    struct audio_src_buffer output;
    struct list_head entry;
    void *event_priv;
    int  (*event_handler)(void *priv, enum audio_src_event event, void *);
    JL_SRC_TypeDef regs;

    s32 *ram0;
    s32 *ram1;
    u16 tdm_adr;
    u8 ramusf_adr;
    u8 ramflt_adr;
    u8 tdm_enable;
    u8 ch_start;
    u8 src_24bit;
    u8 fir_taplen;
    u8 coef_adros;
};
// *INDENT-ON*

void *audio_src_base_open(u8 channel, int in_sample_rate, int out_sample_rate, u8 type);

int audio_src_base_set_output_handler(void *resample,
                                      void *priv,
                                      int (*handler)(void *priv, void *data, int len));

int audio_src_base_set_channel(void *resample, u8 channel);

int audio_src_base_set_in_buffer(void *resample, void *buf, int len);

int audio_src_base_set_input_buff(void *resample, void *buf, int len);

int audio_src_base_resample_config(void *resample, int in_rate, int out_rate);

int audio_src_base_write(void *resample, void *data, int len);

int audio_src_base_stop(void *resample);

int audio_src_base_all_frames(void *resample);

int audio_src_base_input_frames(void *resample);

u32 audio_src_base_out_frames(void *resample);

float audio_src_base_position(void *resample);

int audio_src_base_scale_output(void *resample, int in_sample_rate, int out_sample_rate, int frames);

int audio_src_base_bufferd_frames(void *resample);

int audio_src_base_set_slience(void *resample, u8 slience, int fade_time);

int audio_src_base_wait_irq_callback(void *resample, void *priv, void (*callback)(void *));

void audio_src_base_close(void *resample);

int audio_src_ch_is_running(void *resample);

// *INDENT-OFF*
struct audio_src_handle {
    void *base;
    struct audio_src_buffer output;
    void *output_priv;
    int (*output_handler)(void *priv, void *data, int len);
    u8 *remain_addr;
    int remain_len;
    u8 output_malloc;
};
// *INDENT-ON*

int audio_hw_src_open(struct audio_src_handle *src, u8 channel, u8 type);

int audio_hw_src_set_rate(struct audio_src_handle *src, u16 input_rate, u16 output_rate);

int audio_src_resample_write(struct audio_src_handle *src, void *data, int len);

void audio_src_set_output_handler(struct audio_src_handle *src, void *priv,
                                  int (*handler)(void *, void *, int));

int audio_hw_src_set_input_buffer(struct audio_src_handle *src, void *addr, int len);

int audio_hw_src_set_output_buffer(struct audio_src_handle *src, void *addr, int len);

int audio_hw_src_stop(struct audio_src_handle *src);
int audio_hw_src_active(struct audio_src_handle *src);

void audio_hw_src_close(struct audio_src_handle *src);
int audio_src_set_rise_irq_handler(struct audio_src_handle *src, void *priv, void (*handler)(void *));



#endif
