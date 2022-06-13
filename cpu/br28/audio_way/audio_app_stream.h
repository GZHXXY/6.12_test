

#ifndef _AUDIO_APP_STREAM_H_
#define _AUDIO_APP_STREAM_H_

#include "audio_stream.h"

void audio_app_stream_init(void);
struct audio_stream_entry *audio_app_stream_get_entry(void);

int audio_app_stream_switch_way(u32 close_way, u32 open_way);

#endif /*_AUDIO_APP_STREAM_H_*/

