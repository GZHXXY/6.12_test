#include "app_config.h"
#include "ui/ui_style.h"
#include "ui/ui.h"
#include "ui/ui_api.h"
#include "app_task.h"
#include "system/timer.h"
#include "device/device.h"
#include "key_event_deal.h"
#include "res/resfile.h"
#include "ui/res_config.h"
#include "ui/ui_resource.h"
#include "system/includes.h"
#include "audio_config.h"
#include "asm/mcpwm.h"
#include "sport/sport_api.h"
#include "sport_data/watch_common.h"

#define LOG_TAG_CONST       UI
#define LOG_TAG     		"[UI-ACTION]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"


#if (!TCFG_LUA_ENABLE)
#ifdef CONFIG_UI_STYLE_JL_ENABLE
#if TCFG_UI_ENABLE_SPORTRECORD

#define STYLE_NAME  JL

static u16 timer = 0;

static void refresh_sport_record_warn(void *priv)
{
    if (!timer) {
        return ;
    }
    timer = 0;
    log_info("%s ", __func__);
    layout_hide(GRID_SPORT_RECORD);
    layout_show(LAYOUT_SPORTRECORD_WARN);
    ui_text_show_index_by_id(TEXT__SPORT_RECORD_WARN, 0);
}

static void refresh_sport_record_data(void *priv)
{
    if (!timer) {
        return ;
    }
    timer = 0;
    struct element *elm = priv;
    log_info("%s ", __func__);
    ui_update_source_by_elm(elm, 1);
    ui_text_show_index_by_id(SPORT_TYPE_TEXT, execise_mode_get());
}

static int TRAIN_VERTLIST_onchange(void *ctrl, enum element_change_event e, void *arge)
{
    struct ui_grid *grid = (struct ui_grid *) ctrl;
    struct element *k;
    switch (e) {
    case ON_CHANGE_INIT_PROBE:
        ui_grid_set_slide_direction(grid, SCROLL_DIRECTION_UD);
        break;
    case ON_CHANGE_INIT:
        sport_file_scan_init();
        watch_sport_analyze();
        if (get_sport_recode_id() != 0) {
            if (!timer) {
                timer = sys_timeout_add(&grid->elm, refresh_sport_record_data, 1000);
            }
        } else {
            if (!timer) {
                timer = sys_timeout_add(&grid->elm, refresh_sport_record_warn, 10);
            }
        }
        log_info("%s %d\n", __func__, __LINE__);
        break;
    case ON_CHANGE_RELEASE:
        if (timer) {
            sys_timer_del(timer);
            timer = 0;
        }
        ui_set_default_handler(NULL, NULL, NULL);
        break;
    case ON_CHANGE_FIRST_SHOW:
        break;
    default:
        break;
    }
    return false;
}

static int TARIN_VERTLIST_ontouch(void *ctrl, struct element_touch_event *e)
{
    struct ui_grid *grid = (struct ui_grid *)ctrl;
    int i;
    static u8 move_flag = 0;
    switch (e->event) {
    case ELM_EVENT_TOUCH_UP:
        if (move_flag) {
            move_flag = 0;
            break;
        }
        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_DOWN:
        move_flag = 0;
        break;
    case ELM_EVENT_TOUCH_MOVE:
        move_flag = 1;
        break;
    case ELM_EVENT_TOUCH_R_MOVE:
        log_info("line:%d", __LINE__);
        app_task_put_key_msg(KEY_CHANGE_PAGE, 1);
        break;
    case ELM_EVENT_TOUCH_L_MOVE:
        log_info("line:%d", __LINE__);
        app_task_put_key_msg(KEY_CHANGE_PAGE, 0);
        break;
    }
    return false;
}

REGISTER_UI_EVENT_HANDLER(GRID_SPORT_RECORD)
.ontouch = TARIN_VERTLIST_ontouch,
 .onkey = NULL,
  .onchange  = TRAIN_VERTLIST_onchange,
};



static int SPORTRECORD_WARN_ontouch(void *ctrl, struct element_touch_event *e)
{
    struct ui_grid *grid = (struct ui_grid *)ctrl;
    int i;
    static u8 move_flag = 0;
    switch (e->event) {
    case ELM_EVENT_TOUCH_UP:
        if (move_flag) {
            move_flag = 0;
            break;
        }
        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_DOWN:
        move_flag = 0;
        break;
    case ELM_EVENT_TOUCH_MOVE:
        move_flag = 1;
        break;
    case ELM_EVENT_TOUCH_R_MOVE:
        log_info("line:%d", __LINE__);
        app_task_put_key_msg(KEY_CHANGE_PAGE, 1);
        break;
    case ELM_EVENT_TOUCH_L_MOVE:
        log_info("line:%d", __LINE__);
        app_task_put_key_msg(KEY_CHANGE_PAGE, 0);
        break;
    }
    return false;
}

REGISTER_UI_EVENT_HANDLER(LAYOUT_SPORTRECORD_WARN)
.ontouch = SPORTRECORD_WARN_ontouch,
 .onkey = NULL,
  .onchange  = NULL,
};



#endif /* #if TCFG_UI_ENABLE_SPORTRECORD */
#endif
#endif /* #if (!TCFG_LUA_ENABLE) */


