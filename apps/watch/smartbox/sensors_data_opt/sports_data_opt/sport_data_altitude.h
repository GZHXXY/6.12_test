#ifndef __SMARTBOX_SPORT_DATA_ALTITUDE_H__
#define __SMARTBOX_SPORT_DATA_ALTITUDE_H__

#include "typedef.h"
#include "app_config.h"

u32 sport_data_func_attr_altitude_get(void *priv, u8 attr, u8 sub_mask, u8 *buf, u16 buf_size, u32 offset);
#endif
