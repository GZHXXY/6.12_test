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
#include "music/music.h"

#define LOG_TAG_CONST       UI
#define LOG_TAG     		"[UI-ACTION]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"


#if TCFG_UI_ENABLE && (!TCFG_LUA_ENABLE)

#ifdef CONFIG_UI_STYLE_JL_ENABLE


#define STYLE_NAME  JL


typedef struct menu_callback {
    int sel_id;
    int (*exit_check)();
    int window;
    int (*enter_check)(int window);
} MENU_CB;

const static MENU_CB menu_cb[] = {
    {MENU_ALARM_LAYOUT, NULL, ID_WINDOW_ALARM, NULL},
    {MENU_BARO_LAYOUT,  NULL, ID_WINDOW_BARO, NULL},
    {MENU_BLOOD_LAYOUT, NULL, ID_WINDOW_BLOOD_OXYGEN, NULL},
    {MENU_BRAETHE_LAYOUT, NULL, ID_WINDOW_BREATH_TRAIN, NULL},
    {MENU_CALL_RECOED_LAYOUT, NULL, ID_WINDOW_CALLRECORD, NULL},
    {MENU_CARD_BAG_LAYOUT, NULL, 0, NULL},
    {MENU_COMPASS_LAYOUT, NULL, ID_WINDOW_COMPASS, NULL},
    {MENU_FIND_PHONE_LAYOUT, NULL, ID_WINDOW_FINDPHONE, NULL},
    {MENU_FLASHLIGHT_LAYOUT, NULL, ID_WINDOW_FLASHLIGHT, NULL},
    {MENU_HEART_LAYOUT, NULL, ID_WINDOW_HEART, NULL},
    {MENU_MESS_LAYOUT,  NULL, ID_WINDOW_MESS, NULL},
    {MENU_MUSIC_LAYOUT, NULL, ID_WINDOW_MUSIC, NULL},
    {MENU_PHONEBOOK_LAYOUT, NULL,  ID_WINDOW_PHONEBOOK, NULL},
    {MENU_PRESSURE_LAYOUT, NULL, ID_WINDOW_PRESSURE, NULL},
    {MENU_SET_LAYOUT, NULL,  ID_WINDOW_SET, NULL},
    {MENU_SLEEP_LAYOUT, NULL,  ID_WINDOW_SLEEP, NULL},
    {MENU_SPORT_RECORD_LAYOUT, NULL, ID_WINDOW_ACTIVERECORD, NULL},
    {MENU_STOP_WATCH_LAYOUT, NULL,  ID_WINDOW_STOPWATCH, NULL},
    {MENU_TIME_CLOCK_LAYOUT, NULL,  ID_WINDOW_CALCULAGRAPH, NULL},
    {MENU_TRAIN_LAYOUT, NULL,  ID_WINDOW_TRAIN, NULL},
    {MENU_TRAIN_RECORD_LAYOUT, NULL, ID_WINDOW_SPORT_RECORD, NULL},
    {MENU_TRAIN_STATUS_LAYOUT, NULL,  ID_WINDOW_TRAIN_STATUS, NULL},
    {MENU_WEATHER_LAYOUT, NULL, ID_WINDOW_WEATHER, NULL},
    {MENU_PC_LAYOUT, NULL, ID_WINDOW_PC, NULL},
};

static int menu_in_sel(struct ui_grid *grid)
{
    u32 sel_item = ui_grid_cur_item(grid);

    if (sel_item >= grid->avail_item_num) {
        return false;
    }


    int id = grid->item[sel_item].elm.id;
    log_info(">>>>>>>>>>>>>>sel_item=%d,id=%x", sel_item, id);
    for (int i = 0; i < (sizeof(menu_cb) / sizeof(menu_cb[0])); i++) {
        if (menu_cb[i].sel_id == id) {
            log_info("menu layout id = %x, window = %x\n", id, menu_cb[i].window);
            if (menu_cb[i].window) {
                if (menu_cb[i].enter_check && (!menu_cb[i].enter_check(menu_cb[i].window))) {
                    return false;
                }
                UI_HIDE_CURR_WINDOW();
                UI_SHOW_WINDOW(menu_cb[i].window);
                ui_return_page_push(PAGE_7);
            }
            return true;
            break;
        }
    }
    return false;
}




static u32 item_memory;
static void menu_item_get(struct ui_grid *grid)
{
    item_memory = ui_grid_get_hindex(grid);
    if (item_memory >= grid->avail_item_num) {
        item_memory = 0;
    }
}
static void menu_item_set(struct ui_grid *grid, int item)
{
    ui_grid_set_hi_index(grid, item);
}

static int menu_switch_ontouch(void *ctr, struct element_touch_event *e)
{
    struct ui_grid *grid = (struct ui_grid *)ctr;
    int sel_item;
    static u8 move_flag = 0;
    switch (e->event) {
    case ELM_EVENT_TOUCH_R_MOVE:
        log_info("line:%d", __LINE__);
        app_task_put_key_msg(KEY_CHANGE_PAGE, 1);
        break;
    case ELM_EVENT_TOUCH_L_MOVE:
        log_info("line:%d", __LINE__);
        app_task_put_key_msg(KEY_CHANGE_PAGE, 0);
        break;
    case ELM_EVENT_TOUCH_MOVE:
        move_flag = 1;
        return false;//不接管消息
        break;
    case ELM_EVENT_TOUCH_DOWN:
        move_flag = 0;
        return false;//不接管消息
        break;
    case ELM_EVENT_TOUCH_UP:
        if (move_flag) {
            move_flag = 0;
            return false;//不接管消息
        }
        return menu_in_sel(grid);
        break;
    default:
        return false;
        break;
    }
    return true;//接管消息
}




static int menu_enter_onchane(void *ctr, enum element_change_event e, void *arg)
{
    struct ui_grid *grid = (struct ui_grid *)ctr;
    switch (e) {
    case ON_CHANGE_INIT:
        ui_grid_set_slide_direction(grid, SCROLL_DIRECTION_UD);
        menu_item_set(grid, item_memory);
        break;
    case ON_CHANGE_RELEASE:
        menu_item_get(grid);
        break;
    case ON_CHANGE_FIRST_SHOW:
        break;
    default:
        return false;
    }
    return false;
}



REGISTER_UI_EVENT_HANDLER(VERTLIST)
.onchange = menu_enter_onchane,
 .onkey = NULL,
  .ontouch = menu_switch_ontouch,
};
REGISTER_UI_EVENT_HANDLER(BASEFORM_41)
.onchange = menu_enter_onchane,
 .onkey = NULL,
  .ontouch = NULL,//menu_switch_ontouch,
};










#endif
#endif /* #if (!TCFG_LUA_ENABLE) */

