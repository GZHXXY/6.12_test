
#include "app_config.h"
#include "media/includes.h"
#include "audio_config.h"
#include "audio_way.h"
#include "audio_app_stream.h"


#ifndef AUDIO_OUT_WAY_TYPE
#error "no defined AUDIO_OUT_WAY_TYPE"
#endif


struct audio_app_stream {
    struct audio_stream_entry entry;
    u32 out_way;
    u32 cur_sr;
    u8  start;
};

static struct audio_app_stream 	_audio_stream;

void audio_way_resume(void)
{
    /* putchar('r'); */
    if (_audio_stream.start) {
        audio_stream_resume(&_audio_stream.entry);
    }
}

static int audio_app_stream_data_handler(struct audio_stream_entry *entry,
        struct audio_data_frame *in,
        struct audio_data_frame *out)
{
    struct audio_app_stream *stream = container_of(entry, struct audio_app_stream, entry);
    AUDIO_WAY_PEND();
    if (in->stop) {
        if (stream->start) {
            stream->start = 0;
#if (TCFG_AUDIO_DAC_PA_PORT != NO_CONFIG_PORT)
            if (_audio_stream.out_way & AUDIO_WAY_TYPE_DAC) {
                gpio_direction_output(TCFG_AUDIO_DAC_PA_PORT, 0);
            }
#endif /* #if (TCFG_AUDIO_DAC_PA_PORT != NO_CONFIG_PORT) */
            audio_way_stop(_audio_stream.out_way);
            audio_way_ioctrl(_audio_stream.out_way, SNDCTL_IOCTL_POWER_OFF, NULL);
            /* audio_way_close(_audio_stream.out_way); */
        }
        AUDIO_WAY_POST();
        return 0;
    } else {
        if (stream->start == 0) {
            stream->start = 1;
            stream->cur_sr = in->sample_rate;
            /* audio_way_open(_audio_stream.out_way); */
            audio_way_set_sample_rate(_audio_stream.out_way, stream->cur_sr);
            audio_way_ioctrl(_audio_stream.out_way, SNDCTL_IOCTL_POWER_ON, NULL);
            u8 volume = app_audio_get_volume(APP_AUDIO_CURRENT_STATE);
            app_audio_set_volume(APP_AUDIO_CURRENT_STATE, volume, 1);
            /* audio_way_set_gain(_audio_stream.out_way, volume); */
            audio_way_start(_audio_stream.out_way);
#if (TCFG_AUDIO_DAC_PA_PORT != NO_CONFIG_PORT)
            if (_audio_stream.out_way & AUDIO_WAY_TYPE_DAC) {
                gpio_direction_output(TCFG_AUDIO_DAC_PA_PORT, 1);
            }
#endif /* #if (TCFG_AUDIO_DAC_PA_PORT != NO_CONFIG_PORT) */
        }
        if (stream->cur_sr != in->sample_rate) {
            stream->cur_sr = in->sample_rate;
            audio_way_set_sample_rate(_audio_stream.out_way, stream->cur_sr);
        }
    }
    if (in->data_len == 0) {
        AUDIO_WAY_POST();
        return 0;
    }
#if TCFG_USER_EMITTER_ENABLE && (AUDIO_OUT_WAY_TYPE & AUDIO_WAY_TYPE_BT)
    if (_audio_stream.out_way & AUDIO_WAY_TYPE_DAC) {
        extern u8 bt_emitter_audio_get_mute();
        if (bt_emitter_audio_get_mute()) {
            memset(in->data, 0, in->data_len);
        }
    }
#endif /* #if (AUDIO_OUT_WAY_TYPE & AUDIO_WAY_TYPE_BT) */
    int wlen = audio_way_output_write(in->data, in->data_len);
    AUDIO_WAY_POST();
    return wlen;
}

int audio_app_stream_switch_way(u32 close_way, u32 open_way)
{
    AUDIO_WAY_PEND();
    if (close_way) {
        audio_way_close(close_way);
        _audio_stream.out_way &= (~close_way);
#if (TCFG_AUDIO_DAC_PA_PORT != NO_CONFIG_PORT)
        if (close_way & AUDIO_WAY_TYPE_DAC) {
            gpio_direction_output(TCFG_AUDIO_DAC_PA_PORT, 0);
        }
#endif /* #if (TCFG_AUDIO_DAC_PA_PORT != NO_CONFIG_PORT) */
    }
    if (open_way) {
        audio_way_open(open_way);
        _audio_stream.out_way |= open_way;
    }
    _audio_stream.start = 0;
    audio_stream_resume(&_audio_stream.entry);
    AUDIO_WAY_POST();
    y_printf("switch way:0x%x \n", _audio_stream.out_way);
    return 0;
}

void audio_app_stream_init(void)
{
    memset(&_audio_stream, 0, sizeof(struct audio_app_stream));
    _audio_stream.entry.data_handler = audio_app_stream_data_handler;
    _audio_stream.out_way = AUDIO_OUT_WAY_TYPE;
    /* audio_way_open(AUDIO_OUT_WAY_TYPE); */
}

struct audio_stream_entry *audio_app_stream_get_entry(void)
{
    return &_audio_stream.entry;
}


