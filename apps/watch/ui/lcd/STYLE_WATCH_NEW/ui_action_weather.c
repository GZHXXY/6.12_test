#include "app_config.h"
#include "ui/ui_api.h"
#include "ui/ui.h"
#include "ui/ui_style.h"
#include "app_task.h"
#include "system/timer.h"
#include "app_main.h"
#include "init.h"
#include "key_event_deal.h"
#include "weather/weather.h"
#include "res/font_ascii.h"


#define LOG_TAG_CONST       UI
#define LOG_TAG     		"[UI-ACTION]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"




#if TCFG_UI_ENABLE_WEATHER

#ifndef abs
#define abs(x)  ((x)>0?(x):-(x))
#endif

#if 0
static u8 weather_type[][38] = {
    "晴",
    "少云",
    "晴间多云",
    "多云",
    "阴",
    "有风",
    "平静",
    "大风",
    "飓风",
    "风暴",
    "霾",
    "阵雨",
    "雷阵雨",
    "雷阵雨并伴有冰雹",
    "小雨",
    "中雨",
    "大雨",
    "暴雨",
    "大暴雨",
    "特大暴雨",
    "强阵雨",
    "强雷阵雨",
    "极端阵雨",
    "雨夹雪",
    "雪",
    "阵雪",
    "小雪",
    "中雪",
    "大雪",
    "暴雪",
    "浮尘",
    "扬沙",
    "沙尘暴",
    "强沙尘暴",
    "龙卷风",
    "雾",
    "热",
    "冷",
    "未知",
};
#endif

int update_weather_info_handler(const char *type, u32 arg);

static struct __WEATHER_INFO weather_info;
static u8 temperature_str[5];

struct __WEATHER_INFO *get_weather_info()
{
    return &weather_info;
}

void data_func_attr_weather_set(void *priv, u8 attr, u8 *data, u16 len)
{
    log_info("->%s\n", __FUNCTION__);
    /* g_log_info("============="); */
    /* put_buf(data, len); */
    /* g_log_info("============="); */
    u8 province_name_len;
    u8 city_name_len;
    u8 offset = 0;
    u32 timestamp;

    province_name_len = data[offset];
    /* log_info("province_name_len:%d", province_name_len); */
    offset++;
    if (weather_info.province) {
        free(weather_info.province);
        weather_info.province = 0;
    }
    weather_info.province_name_len = province_name_len + 1;
    weather_info.province = zalloc(province_name_len + 1);
    if (!weather_info.province) {
        log_info("weather_info.province malloc fail");
        return;
    }
    memcpy(weather_info.province, data + offset, province_name_len);
    log_info("province_name_len : %d", weather_info.province_name_len);
    log_info("province:%s\n", weather_info.province);
    put_buf(weather_info.province, weather_info.province_name_len);

    offset += province_name_len;
    city_name_len = data[offset];
    /* log_info("city_name_len:%d", city_name_len); */
    offset++;
    if (weather_info.city) {
        free(weather_info.city);
        weather_info.city = 0;
    }

    weather_info.city_name_len = city_name_len + 1;
    weather_info.city = zalloc(city_name_len + 1);
    if (!weather_info.city) {
        log_info("weather_info.city malloc fail");
        return;
    }
    memcpy(weather_info.city, data + offset, city_name_len);
    /* log_info("city:%s", weather_info.city); */

    offset += city_name_len;
    weather_info.weather = data[offset];
    /* log_info("weather_info.weather:%d", weather_info.weather); */

    offset++;
    weather_info.temperature = data[offset];
    /* log_info("weather_info.temperature:%d", weather_info.temperature); */

    offset++;
    weather_info.humidity = data[offset];
    /* log_info("weather_info.humidity:%d", weather_info.humidity); */

    offset++;
    weather_info.wind_direction = data[offset];
    /* log_info("weather_info.wind_direction:%d", weather_info.wind_direction); */

    offset++;
    weather_info.wind_power = data[offset];
    /* log_info("weather_info.wind_power:%d", weather_info.wind_power); */

    offset++;
    weather_info.update_time = (*(data + offset) << 24) | (*(data + offset + 1) << 16) | (*(data + offset + 2) << 8) | (*(data + offset + 3) << 0);
    timestamp = weather_info.update_time;
    /* log_info("%d年%d月%d日%d时%d分%d秒", ((timestamp >> 26) & 0x3f) + 2010, (timestamp >> 22) & 0xf, (timestamp >> 17) & 0x1f, (timestamp >> 12) & 0x1f, (timestamp >> 6) & 0x3f, timestamp & 0x3f); */

    UI_MSG_POST("weather_info:event=%4", 0);
}


#if TCFG_UI_ENABLE && (!TCFG_LUA_ENABLE)
#ifdef CONFIG_UI_STYLE_JL_ENABLE
#if TCFG_SPI_LCD_ENABLE

#define STYLE_NAME  JL

#define ui_text_for_id(id) \
	({ \
		struct element *elm = ui_core_get_element_by_id(id); \
	 	elm ? container_of(elm, struct ui_text, elm): NULL; \
	 })

#define WEATHER_PAGE        PAGE_66


static const struct uimsg_handl ui_weather_handler[] = {
    { "weather_info",       update_weather_info_handler     },
    { NULL, NULL},      /* 必须以此结尾！ */
};

static void num2string(int num, u8 *buf)
{
    u8 len = 0;
    if (num < 0) {
        buf[len++] = '-';
    }
    num = abs(num);
    if ((num >= 100) && (num <= 999)) {
        buf[len++] = '0' + num / 100;
        buf[len++] = '0' + (num / 10) % 10;
    } else if (num < 10) {

    } else {
        buf[len++] = '0' + num / 10;
    }
    buf[len++] = '0' + num % 10;
    /* buf[len++] = ' '; */
    buf[len++] = '\0';
    /* g_log_info("%s",buf); */
}

int update_weather_info_handler(const char *type, u32 arg)
{
    struct ui_text *text;
    u32 timestamp;
    u8 index_buf;
    static u16 store_buf[2];// ALIGNED(4);
    struct utime time_r;
    struct unumber num;
    int width;
    int height;
    int total_width = 0;
    struct element_css *css;
    struct rect rect;
    struct rect parent_rect;
    char *str;

    css = ui_core_get_element_css(ui_core_get_element_by_id(PIC_WAIT_WEATHER_UPDATE));
    if (weather_info.update_time) {
        css->invisible = true;
    } else {
        css->invisible = false;
    }

    //province
    text = ui_text_for_id(TEXT_PROVINCE);
    ui_text_set_text_attrs(text, weather_info.province, strlen(weather_info.province), FONT_ENCODE_UTF8, 0, FONT_DEFAULT);

    //city
    text = ui_text_for_id(TEXT_CITY);
    ui_text_set_text_attrs(text, weather_info.city, strlen(weather_info.city), FONT_ENCODE_UTF8, 0, FONT_DEFAULT);

    //weather
    text = ui_text_for_id(TEXT_WEATHER);
    index_buf = weather_info.weather;
    ui_text_set_combine_index(text, store_buf, &index_buf, 1);

    //temperature
    num2string(weather_info.temperature, temperature_str);
    num.type = TYPE_STRING;
    num.num_str = temperature_str;
    ui_number_update_by_id(NUM_TEMPERATURE, &num);
    str = temperature_str;
    while (*str) {
        font_ascii_get_width_and_height(*str, &height, &width);
        total_width += width;
        str++;
    }
    ui_core_get_element_abs_rect(ui_core_get_element_by_id(NUM_TEMPERATURE), &rect);
    ui_core_get_element_abs_rect(ui_core_get_element_by_id(PIC_CENTIGRADE)->parent, &parent_rect);
    css = ui_core_get_element_css(ui_core_get_element_by_id(PIC_CENTIGRADE));
    css->left = (rect.left + total_width) * 10000 / parent_rect.width;

    //humidity
    num.type = TYPE_NUM;
    num.numbs = 1;
    num.number[0] = weather_info.humidity;
    ui_number_update_by_id(NUM_HUMIDITY, &num);

    //wind_direction
    ui_text_show_index_by_id(TEXT_WIND_DIRECTION, weather_info.wind_direction);

    //wind_power
    num.type = TYPE_NUM;
    num.numbs = 1;
    num.number[0] = weather_info.wind_power;
    ui_number_update_by_id(NUM_WIND_POWER, &num);

    //update_time
    timestamp = weather_info.update_time;
    time_r.year = ((timestamp >> 26) & 0x3f) + 2010;
    time_r.month = (timestamp >> 22) & 0xf;
    time_r.day = (timestamp >> 17) & 0x1f;
    time_r.hour = (timestamp >> 12) & 0x1f;
    time_r.min = (timestamp >> 6) & 0x3f;
    time_r.sec = timestamp & 0x3f;
    ui_time_update_by_id(TIMER_UPDATE_TIME, &time_r);

    ui_core_redraw(ui_core_get_element_by_id(LAYOUT_WEATHER));

    return 0;
}


static int text_weather_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_text *text = (struct ui_text *)_ctrl;
    u8 index_buf;
    static u16 store_buf[2];// ALIGNED(4);

    switch (event) {
    case ON_CHANGE_INIT:
        switch (text->elm.id) {
        case TEXT_PROVINCE:
            if (weather_info.province == 0) {
                ui_text_set_text_attrs(text, "---", strlen("---"), FONT_ENCODE_UTF8, 0, FONT_DEFAULT);
            } else {
                ui_text_set_text_attrs(text, weather_info.province, strlen(weather_info.province), FONT_ENCODE_UTF8, 0, FONT_DEFAULT);
            }
            break;
        case TEXT_CITY:
            if (weather_info.city == 0) {
                ui_text_set_text_attrs(text, "---", strlen("---"), FONT_ENCODE_UTF8, 0, FONT_DEFAULT);
            } else {
                ui_text_set_text_attrs(text, weather_info.city, strlen(weather_info.city), FONT_ENCODE_UTF8, 0, FONT_DEFAULT);
            }
            break;
        case TEXT_WEATHER:
            index_buf = weather_info.weather;
            ui_text_set_combine_index(text, store_buf, &index_buf, 1);
            break;
        case TEXT_WIND_DIRECTION:
            ui_text_set_index(text, weather_info.wind_direction);
            break;
        }
        break;
    case ON_CHANGE_RELEASE:
        break;
    }

    return FALSE;
}
REGISTER_UI_EVENT_HANDLER(TEXT_PROVINCE)
.onchange = text_weather_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_CITY)
.onchange = text_weather_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_WEATHER)
.onchange = text_weather_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_WIND_DIRECTION)
.onchange = text_weather_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};

static int num_weather_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_number *number = (struct ui_number *)_ctrl;
    struct unumber num;

    switch (event) {
    case ON_CHANGE_INIT:
        switch (number->text.elm.id) {
        case NUM_TEMPERATURE:
            num.type = TYPE_STRING;
            num2string(weather_info.temperature, temperature_str);
            num.num_str = temperature_str;
            ui_number_update(number, &num);
            break;
        case NUM_HUMIDITY:
            num.type = TYPE_NUM;
            num.numbs = 1;
            num.number[0] = weather_info.humidity;
            ui_number_update(number, &num);
            break;
        case NUM_WIND_POWER:
            num.type = TYPE_NUM;
            num.numbs = 1;
            num.number[0] = weather_info.wind_power;
            ui_number_update(number, &num);
            break;
        }
        break;
    case ON_CHANGE_RELEASE:
        break;
    }

    return FALSE;
}
REGISTER_UI_EVENT_HANDLER(NUM_TEMPERATURE)
.onchange = num_weather_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(NUM_HUMIDITY)
.onchange = num_weather_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(NUM_WIND_POWER)
.onchange = num_weather_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};

static int time_weather_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_time *time = (struct ui_time *)_ctrl;
    struct utime time_r;
    u32 timestamp;

    switch (event) {
    case ON_CHANGE_INIT:
        timestamp = weather_info.update_time;
        time_r.year = ((timestamp >> 26) & 0x3f) + 2010;
        time_r.month = (timestamp >> 22) & 0xf;
        time_r.day = (timestamp >> 17) & 0x1f;
        time_r.hour = (timestamp >> 12) & 0x1f;
        time_r.min = (timestamp >> 6) & 0x3f;
        time_r.sec = timestamp & 0x3f;
        ui_time_update(time, &time_r);
        break;
    case ON_CHANGE_RELEASE:
        break;
    }

    return FALSE;
}
REGISTER_UI_EVENT_HANDLER(TIMER_UPDATE_TIME)
.onchange = time_weather_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};

static int pic_weather_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_pic *pic = (struct ui_pic *)_ctrl;
    struct element_css *css;
    struct rect rect;
    struct rect parent_rect;
    int total_width = 0;
    int width;
    int height;
    char *str;

    switch (event) {
    case ON_CHANGE_INIT:
        num2string(weather_info.temperature, temperature_str);
        str = temperature_str;
        while (*str) {
            font_ascii_get_width_and_height(*str, &height, &width);
            total_width += width;
            str++;
        }
        ui_core_get_element_abs_rect(ui_core_get_element_by_id(NUM_TEMPERATURE), &rect);
        ui_core_get_element_abs_rect(pic->elm.parent, &parent_rect);
        css = ui_core_get_element_css(&pic->elm);
        css->left = (rect.left + total_width) * 10000 / parent_rect.width;
        break;
    case ON_CHANGE_RELEASE:
        break;
    }

    return FALSE;
}
REGISTER_UI_EVENT_HANDLER(PIC_CENTIGRADE)
.onchange = pic_weather_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};

static int pic_wait_weather_update_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_pic *pic = (struct ui_pic *)_ctrl;
    struct element_css *css;

    switch (event) {
    case ON_CHANGE_INIT:
        css = ui_core_get_element_css(&pic->elm);
        if (weather_info.update_time) {
            css->invisible = true;
        } else {
            css->invisible = false;
        }
        break;
    case ON_CHANGE_RELEASE:
        break;
    }

    return FALSE;
}
REGISTER_UI_EVENT_HANDLER(PIC_WAIT_WEATHER_UPDATE)
.onchange = pic_wait_weather_update_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};

static int layout_weather_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    switch (event) {
    case ON_CHANGE_INIT:
        ui_register_msg_handler(WEATHER_PAGE, ui_weather_handler);//注册消息交互的回调
        break;
    case ON_CHANGE_RELEASE:
        break;
    }

    return FALSE;
}
int layout_weather_ontouch(void *_layout, struct element_touch_event *e)
{
    switch (e->event) {
    case ELM_EVENT_TOUCH_R_MOVE:
        ui_return_page_pop(1);
        break;
    case ELM_EVENT_TOUCH_L_MOVE:
        ui_return_page_pop(0);
        break;
    default:
        break;
    }
    return false;
}
REGISTER_UI_EVENT_HANDLER(LAYOUT_WEATHER)
.onchange = layout_weather_onchange,
 .onkey = NULL,
  .ontouch = layout_weather_ontouch,
};

#endif
#endif
#endif /* #if (!TCFG_LUA_ENABLE) */

#endif /* #if TCFG_UI_ENABLE_WEATHER */


