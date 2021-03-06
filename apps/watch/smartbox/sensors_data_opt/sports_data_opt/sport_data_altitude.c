#include "smartbox/config.h"
#include "sport_data_altitude.h"
#include "sport_data_func.h"
#include "sport_data/altitude_presure.h"
#if (SMART_BOX_EN && JL_SMART_BOX_SENSORS_DATA_OPT)
enum {
    SPORT_DATA_REAL_ALTITUDE,
    SPORT_DATA_MIN_ALTITUDE,
    SPORT_DATA_MAX_ALTITUDE,

};
#define ALTITUDE_DATA_MAX_LEN		1200
static u8 *altitude_data = NULL;

u32 sport_data_func_attr_altitude_get(void *priv, u8 attr, u8 sub_mask, u8 *buf, u16 buf_size, u32 offset)
{
    printf("%s", __func__);
    u32 rlen = 0;
    u16 data_len = 0;
    if (NULL == altitude_data) {
        altitude_data = zalloc(ALTITUDE_DATA_MAX_LEN);
    }
    if (sub_mask & BIT(SPORT_DATA_REAL_ALTITUDE)) {
        u32 altitude = get_data_altitude();
        altitude_data[data_len++] = (altitude) & 0xff;
        altitude_data[data_len++] = (altitude >> 8) & 0xff;
        altitude_data[data_len++] = (altitude >> 16) & 0xff;
        altitude_data[data_len++] = (altitude >> 24) & 0xff;

    }

    if (sub_mask & BIT(SPORT_DATA_MIN_ALTITUDE)) {
        u32 altitude = get_min_altitude();
        altitude_data[data_len++] = (altitude) & 0xff;
        altitude_data[data_len++] = (altitude >> 8) & 0xff;
        altitude_data[data_len++] = (altitude >> 16) & 0xff;
        altitude_data[data_len++] = (altitude >> 24) & 0xff;
    }

    if (sub_mask & BIT(SPORT_DATA_MAX_ALTITUDE)) {
        u32 altitude = get_max_altitude();
        altitude_data[data_len++] = (altitude) & 0xff;
        altitude_data[data_len++] = (altitude >> 8) & 0xff;
        altitude_data[data_len++] = (altitude >> 16) & 0xff;
        altitude_data[data_len++] = (altitude >> 24) & 0xff;
    }
    if (0xff == sub_mask) {
        data_len = ALTITUDE_DATA_MAX_LEN;

        memset(altitude_data, 0, data_len);
        data_len = get_day_altitude_data(altitude_data);
        printf("altitude_data_len=%d", data_len);
        for (int i = 0; i < data_len; i++) {
            printf("data=%d", altitude_data[i]);
        }
    }


    rlen = add_one_attr_with_submask_huge(buf, buf_size, offset, attr, sub_mask, altitude_data, data_len);
    // ?????????????????????free??????????????????????????????????????????????????????????????????????????????????????????????????????
    // ?????????????????????NULL?????????????????????NULL????????????free
    sport_data_global_var_deal(priv, attr, sub_mask, altitude_data, data_len, rlen, (void *)&altitude_data);
    return rlen;
}

#endif
