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
#include "ui/ui_sys_param.h"
#include "ui/watch_syscfg_manage.h"
#include "font/language_list.h"
#include "bt_common.h"

#define LOG_TAG_CONST       UI
#define LOG_TAG     		"[UI-ACTION]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"



#ifdef CONFIG_UI_STYLE_JL_ENABLE
#if TCFG_UI_ENABLE_SYS_SET
#define STYLE_NAME  JL

REGISTER_UI_STYLE(STYLE_NAME)

#define 	PAGE_SET						PAGE_41
#define 	PAGE_SCREEN_DISP				PAGE_42
#define 	PAGE_VOICE_SET					PAGE_43
#define 	PAGE_SHAKE_LEVEL				PAGE_44
#define 	PAGE_UNDISTURB_MODE				PAGE_45
#define 	PAGE_ABOUT						PAGE_46
#define 	PAGE_TRAIN_SET					PAGE_47
#define 	PAGE_TOUCH_SEND					PAGE_48
#define 	PAGE_USER_GUIDE					PAGE_49
#define 	PAGE_SYS_MENU					PAGE_50
#define 	PAGE_SHORTCUT_KEY				PAGE_69
#define     PAGE_STYLE                      PAGE_75
#define 	PAGE_LANGUAGE					PAGE_85

#define ui_grid_for_id(id) \
	({ \
		struct element *elm = ui_core_get_element_by_id(id); \
	 	elm ? container_of(elm, struct ui_grid, elm): NULL; \
	 })

extern void sys_enter_soft_poweroff(void *priv);

extern void ui_send_event(u16 event, u32 val);
extern int get_elapse_time(struct sys_time *time);
extern int standby_watch_get_style();
extern char *watch_get_item(int style);
extern int watch_get_style();

#if TCFG_UI_SHUT_DOWN_TIME
extern void ui_set_shut_down_time(u16 time);
extern void ui_auto_shut_down_modify(void);
#endif

#if (TCFG_BACKLIGHT_PWM_MODE == 1)
extern int pwm_led_output_clk(u8 gpio, u8 prd, u8 duty);
#endif

extern int UIInfo_w_vm_timer;

static u8 all_day_undisturb_ontouch;
static u8 time_undisturb_ontouch;

const u32 ui_page_list[] = {
    ID_WINDOW_SLEEP,                //??????
    ID_WINDOW_PRESSURE,             //??????
    ID_WINDOW_HEART,                //??????
    ID_WINDOW_MUSIC,                //??????
    ID_WINDOW_WEATHER,              //??????
    ID_WINDOW_ACTIVERECORD,         //????????????
    ID_WINDOW_BARO,                 //???????????????
    ID_WINDOW_BARO,                 //???????????????
    0,
    0,
    ID_WINDOW_CALLRECORD,           //????????????
    ID_WINDOW_PHONEBOOK,            //???????????????
    ID_WINDOW_BLOOD_OXYGEN,         //???????????????
    ID_WINDOW_COMPASS,                              //?????????

};
extern u32 ui_show_page_list[9];

static void get_sys_time(struct sys_time *time)
{
    void *fd = dev_open("rtc", NULL);
    if (!fd) {
        get_elapse_time(time);
        return;
    }
    dev_ioctl(fd, IOCTL_GET_SYS_TIME, (u32)time);
    /* log_info("get_sys_time : %d-%d-%d,%d:%d:%d\n", time->year, time->month, time->day, time->hour, time->min, time->sec); */
    dev_close(fd);
}
//-----------------------??????????????????????????????-------------------------------//

static void ui_default_param_set()
{
    u8 i, k;
    u8 card_set_num = 0;

#if 0
    for (i = 0, k = 0; i < MAX_CARD_SELECT; i++, k++) {
        while (1) {
            if (ui_page_list[k] != 0) {
                card_set_num++;
                ui_sys_param.card_select[i] = k;
                break;
            } else {
                k++;
                if (k > sizeof(ui_page_list) / sizeof(ui_page_list[0])) {
                    break;
                }
            }
        }
        if (k > sizeof(ui_page_list) / sizeof(ui_page_list[0])) {
            break;
        }
    }
#else
    if (ui_page_list[0] != 0) {
        ui_sys_param.card_select[card_set_num++] = 0;
    }
    if (ui_page_list[3] != 0) {
        ui_sys_param.card_select[card_set_num++] = 3;
    }
    if (ui_page_list[5] != 0) {
        ui_sys_param.card_select[card_set_num++] = 5;
    }
    if (ui_page_list[4] != 0) {
        ui_sys_param.card_select[card_set_num++] = 4;
    }
    /* if (ui_page_list[10] != 0) { */
    /* ui_sys_param.card_select[card_set_num++] = 10; */
    /* } */
    /* if (ui_page_list[11] != 0) { */
    /* ui_sys_param.card_select[card_set_num++] = 11; */
    /* } */
#endif

    set_ui_sys_param(CardSetNum, card_set_num);
}

void ui_sysinfo_init()
{
    u8 i;
    u8 card_set_num;
    int ret;
    ret = read_UIInfo_from_vm();
    if (ret < 0) {
        log_info("read_UIInfo_from_vm err");
        ui_default_param_set();
    }
    /* for (i = 0; i < SYS_PARAM_NUM; i++) { */
    /*     y_log_info("label:%d value:%d", ui_sys_param.sys_param_table[i].label, ui_sys_param.sys_param_table[i].value); */
    /* } */
    extern void ui_moto_init(u8 gpio);
    ui_moto_init(TCFG_MOTO_PWM_IO);
    ui_ajust_light((get_ui_sys_param(LightLevel) + 1) * 2);
    ui_set_dark_time(get_ui_sys_param(DarkTime));
    ui_voice_mute(get_ui_sys_param(SysVoiceMute));
    card_set_num = get_ui_sys_param(CardSetNum);
    if (card_set_num != 0) {
        for (i = 0; i < card_set_num; i++) {
            ui_show_page_list[i + 3] = ui_page_list[ui_sys_param.card_select[i]];
            log_info("page:0x%x %d", ui_show_page_list[i + 3], ui_sys_param.card_select[i]);
        }
    }
    ui_page_list_update(ui_show_page_list, card_set_num + 3);
}




#if (TCFG_UI_ENABLE && !TCFG_LUA_ENABLE)
//------------------------------???????????????-------------------------------//

static int set_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_grid *grid = (struct ui_grid *)_ctrl;

    switch (event) {
    case ON_CHANGE_INIT:
        ui_grid_set_slide_direction(grid, SCROLL_DIRECTION_UD);
        set_ui_sys_param(ConnNewPhone, 0);
        break;
    }
    return 0;
}

static int set_ontouch(void *ctr, struct element_touch_event *e)
{
    struct ui_grid *grid = (struct ui_grid *)ctr;
    static u8 touch_action = 0;
    int sel_item;

    switch (e->event) {
    case ELM_EVENT_TOUCH_UP:
        if (touch_action != 1) {
            break;
        }

        sel_item = ui_grid_cur_item(grid);
        if (sel_item <= 0) {
            break;
        }
        switch (sel_item) {
        case 1:
            ui_send_event(KEY_CHANGE_PAGE, BIT(31) | PAGE_24);
            ui_return_page_push(PAGE_SET);
            break;
        case 2://????????????
            set.show_layout = LAYOUT_SCREEN_DISP;
            ui_send_event(KEY_CHANGE_PAGE, BIT(31) | PAGE_SCREEN_DISP);
            break;
        case 3://????????????
            set.show_layout = LAYOUT_VOICE_SET;
            ui_send_event(KEY_CHANGE_PAGE, BIT(31) | PAGE_VOICE_SET);
            break;
        case 4://????????????
            set.show_layout = LAYOUT_UNDISTURB_MODE;
            ui_send_event(KEY_CHANGE_PAGE, BIT(31) | PAGE_UNDISTURB_MODE);
            break;
        case 5://????????????
            ui_send_event(KEY_CHANGE_PAGE, BIT(31) | PAGE_SHORTCUT_KEY);
            break;
        case 6://????????????
            ui_send_event(KEY_CHANGE_PAGE, BIT(31) | PAGE_TRAIN_SET);
            break;
        case 7://????????????
            set.show_layout = LAYOUT_SYS_MENU;
            ui_send_event(KEY_CHANGE_PAGE, BIT(31) | PAGE_SYS_MENU);
            break;
        case 8://????????????
            ui_send_event(KEY_CHANGE_PAGE, BIT(31) | PAGE_USER_GUIDE);
            break;
        case 9://????????????
            ui_send_event(KEY_CHANGE_PAGE, BIT(31) | PAGE_LANGUAGE);
            break;
        case 10://????????????
            ui_send_event(KEY_CHANGE_PAGE, BIT(31) | PAGE_SHAKE_LEVEL);
            break;
        case 11://????????????
            ui_send_event(KEY_CHANGE_PAGE, BIT(31) | PAGE_STYLE);
            break;
        case 12://??????
            ui_send_event(KEY_CHANGE_PAGE, BIT(31) | PAGE_ABOUT);
            break;
        }

        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        touch_action = 2;
        break;
    case ELM_EVENT_TOUCH_DOWN:
        touch_action = 1;
        break;
    case ELM_EVENT_TOUCH_U_MOVE:
        break;
    case ELM_EVENT_TOUCH_D_MOVE:
        break;
    }

    return false;
}
REGISTER_UI_EVENT_HANDLER(VLIST_SET)//??????-????????????
.onchange = set_onchange,
 .onkey = NULL,
  .ontouch = set_ontouch,
};
static int layout_set_ontouch(void *ctr, struct element_touch_event *e)
{
    /* struct layout *layout = (struct layout *)ctr; */

    switch (e->event) {
    case ELM_EVENT_TOUCH_R_MOVE:
        ui_return_page_pop(1);
        break;
    case ELM_EVENT_TOUCH_UP:
        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        break;
    case ELM_EVENT_TOUCH_DOWN:
        break;
    case ELM_EVENT_TOUCH_U_MOVE:
        break;
    case ELM_EVENT_TOUCH_D_MOVE:
        break;
    }

    return false;
}
REGISTER_UI_EVENT_HANDLER(LAYOUT_SET)//??????
.onchange = NULL,
 .onkey = NULL,
  .ontouch = layout_set_ontouch,
};

static int list_text_set_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_text *text = (struct ui_text *)_ctrl;

    switch (event) {
    case ON_CHANGE_INIT:

        switch (text->elm.id) {
        /* case TEXT_SHAKE_LEVEL: */
        /*     ui_text_set_index(text, set.ui_vm_info.shake_level_sel); */
        /*     break; */
        case TEXT_SHORTCUT_KEY:
            log_info("%s %d", __FUNCTION__, __LINE__);
            ui_text_set_index(text, get_ui_sys_param(ShortcutKey));
            break;
        default:
            break;
        }
        break;
    case ON_CHANGE_HIGHLIGHT:
        break;
    default:
        return FALSE;
    }
    return FALSE;
}
/* REGISTER_UI_EVENT_HANDLER(TEXT_SHAKE_LEVEL)//??????-????????????-??????????????????????????????????????? */
/* .onchange = list_text_set_onchange, */
/*  .onkey = NULL, */
/*   .ontouch = NULL, */
/* }; */
REGISTER_UI_EVENT_HANDLER(TEXT_SHORTCUT_KEY)//??????-????????????-???????????????????????????????????????
.onchange = list_text_set_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};

//-----------------------??????????????????????????????---------------------------//

static int page_switch_init(int id)
{
    if (set.show_layout != -1) {
        ui_show(set.show_layout);
    }
    return 0;
}
static int page_screen_disp_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    /* struct window *window = (struct window *)_ctrl; */
    /* log_info("window call = %s id =%x \n", __FUNCTION__, window->elm.id); */

    switch (event) {
    case ON_CHANGE_INIT:
        ui_set_call(page_switch_init, 0);
        break;
    case ON_CHANGE_RELEASE:
        break;
    }
    return 0;
}
/* REGISTER_UI_EVENT_HANDLER(PAGE_SCREEN_DISP)//?????????????????? */
/* .onchange = page_screen_disp_onchange, */
/*  .onkey = NULL, */
/*   .ontouch = NULL, */
/* }; */
/* REGISTER_UI_EVENT_HANDLER(PAGE_VOICE_SET)//?????????????????? */
/* .onchange = page_screen_disp_onchange, */
/*  .onkey = NULL, */
/*   .ontouch = NULL, */
/* }; */
/* REGISTER_UI_EVENT_HANDLER(PAGE_UNDISTURB_MODE)//?????????????????? */
/* .onchange = page_screen_disp_onchange, */
/*  .onkey = NULL, */
/*   .ontouch = NULL, */
/* }; */
REGISTER_UI_EVENT_HANDLER(PAGE_SYS_MENU)//??????????????????
.onchange = page_screen_disp_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};

//-----------------------????????????????????????---------------------------//

u8 get_standby_watch_en()
{
    u8 standby_watch_en;
    if (standby_watch_get_style() != -1) {
        standby_watch_en = true;
    } else {
        standby_watch_en = false;
    }
    return standby_watch_en;
}

static int screen_disp_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_grid *grid = (struct ui_grid *)_ctrl;

    switch (event) {
    case ON_CHANGE_INIT:
        ui_grid_set_slide_direction(grid, SCROLL_DIRECTION_UD);
        break;
    }
    return 0;
}

static int screen_disp_ontouch(void *ctr, struct element_touch_event *e)
{
    struct ui_grid *grid = (struct ui_grid *)ctr;
    static u8 touch_action = 0;
    int sel_item;
    u8 card_set_num;

    switch (e->event) {
    case ELM_EVENT_TOUCH_UP:
        if (touch_action != 1) {
            break;
        }

        sel_item = ui_grid_cur_item(grid);
        if (sel_item <= 0) {
            break;
        }
        switch (sel_item) {
        case 1://????????????
            set.show_layout = LAYOUT_SCREEN_DISP;
            ui_send_event(KEY_CHANGE_PAGE, BIT(31) | PAGE_16);
            break;
        case 2://????????????
            set.show_layout = LAYOUT_SCREEN_DISP;
            ui_send_event(KEY_CHANGE_PAGE, BIT(31) | PAGE_70);
            break;
        case 3://????????????
            card_set_num = get_ui_sys_param(CardSetNum);
            if (card_set_num > 0) {
                set.show_layout = LAYOUT_CARD_SET;
                ui_hide(LAYOUT_SCREEN_DISP);
                ui_show(set.show_layout);
            } else {
                set.show_layout = LAYOUT_CARD_SET_NULL;
                ui_hide(LAYOUT_SCREEN_DISP);
                ui_show(set.show_layout);
            }
            break;
        case 4://????????????
            set.show_layout = LAYOUT_AJUST_BRIGHTNESS;

            ui_hide(LAYOUT_SCREEN_DISP);
            ui_show(LAYOUT_AJUST_BRIGHTNESS);
            break;
        case 5://????????????
            set.show_layout = LAYOUT_HIGH_SET;

            ui_hide(LAYOUT_SCREEN_DISP);
            ui_show(LAYOUT_HIGH_SET);
            break;
        }

        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        touch_action = 2;
        break;
    case ELM_EVENT_TOUCH_DOWN:
        touch_action = 1;
        break;
    case ELM_EVENT_TOUCH_U_MOVE:
        break;
    case ELM_EVENT_TOUCH_D_MOVE:
        break;
    }

    return false;
}
REGISTER_UI_EVENT_HANDLER(VLIST_SCREEN_DISP)//????????????-????????????
.onchange = screen_disp_onchange,
 .onkey = NULL,
  .ontouch = screen_disp_ontouch,
};

static int change_watch_text_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_text *text = (struct ui_text *)_ctrl;
    char *ext_name;
    static char *watch_name;

    switch (event) {
    case ON_CHANGE_INIT:
        ext_name = strrchr(watch_get_item(watch_get_style()), '.');
        watch_name = strrchr(watch_get_item(watch_get_style()), '/') + 1;
        ui_text_set_text_attrs(text, watch_name, strlen(watch_name) - strlen(ext_name), FONT_ENCODE_ANSI, 0, FONT_DEFAULT);
        break;
    case ON_CHANGE_HIGHLIGHT:
        break;
    default:
        return FALSE;
    }
    return FALSE;
}
REGISTER_UI_EVENT_HANDLER(TEXT_WATCH_NAME)//????????????-????????????-????????????(?????????????????????)
.onchange = change_watch_text_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};

static int standby_watch_text_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_text *text = (struct ui_text *)_ctrl;
    char *standby_watch_ext_name;
    static char *standby_watch_name;

    switch (event) {
    case ON_CHANGE_INIT:
        if (standby_watch_get_style() != -1) {
            standby_watch_ext_name = strrchr(watch_get_item(standby_watch_get_style()), '.');
            standby_watch_name = strrchr(watch_get_item(standby_watch_get_style()), '/') + 1;
            ui_text_set_text_attrs(text, standby_watch_name, strlen(standby_watch_name) - strlen(standby_watch_ext_name), FONT_ENCODE_ANSI, 0, FONT_DEFAULT);
        } else {
            ui_text_set_text_attrs(text, "???", strlen("???"), FONT_ENCODE_UTF8, 0, FONT_DEFAULT);
        }
        break;
    case ON_CHANGE_HIGHLIGHT:
        break;
    default:
        return FALSE;
    }
    return FALSE;
}
REGISTER_UI_EVENT_HANDLER(TEXT_STANDBY_WATCH_NAME)//????????????-????????????-????????????(?????????????????????)
.onchange = standby_watch_text_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};

static int light_adjust_text_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_text *text = (struct ui_text *)_ctrl;
    u8 light_ajust_sel;
    /* u16 index_buf[1]; */
    /* static u16 store_buf[2]; */

    switch (event) {
    case ON_CHANGE_INIT:
        light_ajust_sel = get_ui_sys_param(LightLevel);
        ui_text_set_index(text, light_ajust_sel);
        /* index_buf[0] = set.ui_vm_info.light_ajust_sel; */
        /* ui_text_set_combine_index(text, store_buf, index_buf, 1); */
        break;
    case ON_CHANGE_HIGHLIGHT:
        /* ui_text_set_index(text, !!yes); */
        break;
    default:
        return FALSE;
    }
    return FALSE;
}
REGISTER_UI_EVENT_HANDLER(TEXT_LIGHT_AJUST)//????????????-????????????-????????????(?????????????????????)
.onchange = light_adjust_text_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};

static int layout_screen_disp_ontouch(void *ctr, struct element_touch_event *e)
{
    struct layout *layout = (struct layout *)ctr;

    switch (e->event) {
    case ELM_EVENT_TOUCH_R_MOVE:
        set.show_layout = LAYOUT_SCREEN_DISP;
        ui_send_event(KEY_CHANGE_PAGE, BIT(31) | PAGE_SET);
        break;
    case ELM_EVENT_TOUCH_UP:
        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        break;
    case ELM_EVENT_TOUCH_DOWN:
        break;
    case ELM_EVENT_TOUCH_U_MOVE:
        break;
    case ELM_EVENT_TOUCH_D_MOVE:
        break;
    }

    return false;
}

REGISTER_UI_EVENT_HANDLER(LAYOUT_SCREEN_DISP)//????????????
.onchange = NULL,
 .onkey = NULL,
  .ontouch = layout_screen_disp_ontouch,
};

//---------------------------????????????-------------------------------//

static int shortcut_key_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct ui_grid *grid = (struct ui_grid *)ctr;
    switch (e) {
    case ON_CHANGE_INIT:
        ui_grid_set_slide_direction(grid, SCROLL_DIRECTION_UD);
        break;
    case ON_CHANGE_RELEASE:
        break;
    case ON_CHANGE_FIRST_SHOW:
        break;
    default:
        return false;
    }
    return false;
}
static int shortcut_key_ontouch(void *ctr, struct element_touch_event *e)
{
    struct ui_grid *grid = (struct ui_grid *)ctr;
    static u8 move_flag = 0;
    switch (e->event) {
    case ELM_EVENT_TOUCH_R_MOVE:
        ui_send_event(KEY_CHANGE_PAGE, BIT(31) | PAGE_SET);
        break;
    case ELM_EVENT_TOUCH_L_MOVE:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        move_flag = 1;
        break;
    case ELM_EVENT_TOUCH_DOWN:
        move_flag = 0;
        break;
    case ELM_EVENT_TOUCH_UP:
        if (move_flag) {
            move_flag = 0;
            break;
        }
        set_ui_sys_param(ShortcutKey, ui_grid_cur_item(grid));
        ui_send_event(KEY_CHANGE_PAGE, BIT(31) | PAGE_SET);
        return true;//????????????
    default:
        return false;
    }
    return false;//???????????????
}
REGISTER_UI_EVENT_HANDLER(VLIST_SHORTCUT_KEY)
.onchange = shortcut_key_onchange,
 .onkey = NULL,
  .ontouch = shortcut_key_ontouch,
};

//-----------------------------????????????-------------------------------//

static int list_card_set_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_grid *grid = (struct ui_grid *)_ctrl;
    int row, col;
    u8 card_set_num;

    switch (event) {
    case ON_CHANGE_INIT:
        card_set_num = get_ui_sys_param(CardSetNum);
        row = card_set_num + 1;
        col = 1;
        ui_grid_init_dynamic(grid, &row, &col);
        log_info("dynamic_grid %d X %d\n", row, col);

        ui_grid_set_slide_direction(grid, SCROLL_DIRECTION_UD);

        break;
    case ON_CHANGE_RELEASE:

        break;
    }
    return 0;
}
REGISTER_UI_EVENT_HANDLER(VLIST_DY_CARD_SET)//????????????-??????????????????
.onchange = list_card_set_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};

static int list_text_card_set_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_text *text = (struct ui_text *)_ctrl;
    /* u8 index_buf[1]; */
    /* static u16 store_buf[4][2]; */
    /* u8 *store_buf_text; */
    int index;

    switch (event) {
    case ON_CHANGE_INIT:
        break;
    case ON_CHANGE_HIGHLIGHT:
        break;
    case ON_CHANGE_UPDATE_ITEM:

        index = (u32)arg;
        if (index > sizeof(ui_sys_param.card_select)) {
            break;
        }
        switch (text->elm.id) {
        case TEXT_CARD_SET0:
            if (index == 0) {
                ui_text_set_hide_by_id(TEXT_CARD_SET0, true);
                return FALSE;
            } else {
                ui_text_set_hide_by_id(TEXT_CARD_SET0, false);
            }
            /* store_buf_text = store_buf[0]; */
            /* log_info("b0\n"); */
            break;
        case TEXT_CARD_SET1:
            /* store_buf_text = store_buf[1]; */
            /* log_info("b1\n"); */
            break;
        case TEXT_CARD_SET2:
            /* store_buf_text = store_buf[2]; */
            /* log_info("b2\n"); */
            break;
        case TEXT_CARD_SET3:
            /* store_buf_text = store_buf[3]; */
            /* log_info("b3\n"); */
            break;
        default:
            return FALSE;
        }
        if (index > 0) {
            index -= 1;
        }

        /* log_info("myindex %d, %d\n", index, ui_sys_param.card_select[index]); */

        ui_text_set_index(text, ui_sys_param.card_select[index]);
        /* index_buf[0] = set.ui_vm_info.card_select[index]; */
        /* ui_text_set_combine_index(text, store_buf_text, index_buf, 1); */
        break;
    }
    return FALSE;
}
REGISTER_UI_EVENT_HANDLER(TEXT_CARD_SET0)//????????????-????????????-????????????
.onchange = list_text_card_set_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_CARD_SET1)//????????????-????????????-????????????
.onchange = list_text_card_set_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_CARD_SET2)//????????????-????????????-????????????
.onchange = list_text_card_set_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_CARD_SET3)//????????????-????????????-????????????
.onchange = list_text_card_set_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};



static int list_pic_card_set_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_pic *pic = (struct ui_pic *)_ctrl;
    int index;

    switch (event) {
    case ON_CHANGE_INIT:
        break;
    case ON_CHANGE_HIGHLIGHT:
        break;
    case ON_CHANGE_UPDATE_ITEM:
        index = (u32)arg;
        switch (pic->elm.id) {
        case PIC_CARD_SET_UP0:
            if (index == 0) {
                ui_pic_set_hide_by_id(PIC_CARD_SET_UP0, true);
                break;
            } else {
                ui_pic_set_hide_by_id(PIC_CARD_SET_UP0, false);
            }

            if (index == 1) {
                ui_pic_set_image_index(pic, 0);
            } else {
                ui_pic_set_image_index(pic, 1);
            }
            set.vlist_card_index[0] = index;
            break;
        case PIC_CARD_SET_UP1:
            if (index == 1) {
                ui_pic_set_image_index(pic, 0);
            } else {
                ui_pic_set_image_index(pic, 1);
            }
            set.vlist_card_index[1] = index;
            break;
        case PIC_CARD_SET_UP2:
            set.vlist_card_index[2] = index;
            break;
        case PIC_CARD_SET_UP3:
            set.vlist_card_index[3] = index;
            break;
        }

        break;
    }
    return FALSE;
}
static int list_pic_card_set_ontouch(void *ctr, struct element_touch_event *e)
{
    struct ui_pic *pic = (struct ui_pic *)ctr;
    static u8 touch_action = 0;
    int index;
    u8 tmp;
    int item_sel;

    switch (e->event) {
    case ELM_EVENT_TOUCH_UP:
        if (touch_action != 1) {
            break;
        }
        switch (pic->elm.id) {
        case PIC_CARD_SET_UP0:
            /* log_info("a0\n"); */
            index = set.vlist_card_index[0];
            item_sel = 0;
            break;
        case PIC_CARD_SET_UP1:
            index = set.vlist_card_index[1];
            item_sel = 1;
            /* log_info("a1\n"); */
            break;
        case PIC_CARD_SET_UP2:
            index = set.vlist_card_index[2];
            item_sel = 2;
            /* log_info("a2\n"); */
            break;
        case PIC_CARD_SET_UP3:
            index = set.vlist_card_index[3];
            item_sel = 3;
            /* log_info("a3\n"); */
            break;
        default:
            return false;
        }
        /* log_info("aindex %d\n", index); */
        if (index < 2) {
            break;
        }
        index--;
        tmp = ui_sys_param.card_select[index];
        ui_sys_param.card_select[index] = ui_sys_param.card_select[index - 1];
        ui_sys_param.card_select[index - 1] = tmp;
        memset(ui_show_page_list + 3, 0, sizeof(ui_show_page_list) - 3 * sizeof(ui_show_page_list[0]));
        u8 card_set_num = get_ui_sys_param(CardSetNum);
        for (u8 i = 0; i < card_set_num; i++) {
            ui_show_page_list[i + 3] = ui_page_list[ui_sys_param.card_select[i]];
        }
        ui_page_list_update(ui_show_page_list, card_set_num + 3);
        if (UIInfo_w_vm_timer == 0) {
            UIInfo_w_vm_timer = sys_timer_add(NULL, write_UIInfo_to_vm, 1000);
        } else {
            sys_timer_re_run(UIInfo_w_vm_timer);
        }

        if (item_sel > 0) {
            ui_grid_update_by_id_dynamic(VLIST_DY_CARD_SET, item_sel, true);
            ui_grid_update_by_id_dynamic(VLIST_DY_CARD_SET, item_sel - 1, true);
        }

        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        touch_action = 2;
        break;
    case ELM_EVENT_TOUCH_DOWN:
        touch_action = 1;
        return true;
    case ELM_EVENT_TOUCH_U_MOVE:
        break;
    case ELM_EVENT_TOUCH_D_MOVE:
        break;
    }

    return false;
}
REGISTER_UI_EVENT_HANDLER(PIC_CARD_SET_UP0)//????????????-????????????-????????????
.onchange = list_pic_card_set_onchange,
 .onkey = NULL,
  .ontouch = list_pic_card_set_ontouch,
};
REGISTER_UI_EVENT_HANDLER(PIC_CARD_SET_UP1)//????????????-????????????-????????????
.onchange = list_pic_card_set_onchange,
 .onkey = NULL,
  .ontouch = list_pic_card_set_ontouch,
};
REGISTER_UI_EVENT_HANDLER(PIC_CARD_SET_UP2)//????????????-????????????-????????????
.onchange = list_pic_card_set_onchange,
 .onkey = NULL,
  .ontouch = list_pic_card_set_ontouch,
};
REGISTER_UI_EVENT_HANDLER(PIC_CARD_SET_UP3)//????????????-????????????-????????????
.onchange = list_pic_card_set_onchange,
 .onkey = NULL,
  .ontouch = list_pic_card_set_ontouch,
};



static int list_pic_card_add_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_pic *pic = (struct ui_pic *)_ctrl;
    int index;

    switch (event) {
    case ON_CHANGE_INIT:
        break;
    case ON_CHANGE_HIGHLIGHT:
        break;
    case ON_CHANGE_UPDATE_ITEM:
        index = (u32)arg;
        if (index == 0) {
            ui_pic_set_hide_by_id(PIC_CARD_SET_ADD, false);
        } else {
            ui_pic_set_hide_by_id(PIC_CARD_SET_ADD, true);
        }
        break;
    }
    return FALSE;
}

static int list_pic_card_add_ontouch(void *ctr, struct element_touch_event *e)
{
    struct ui_pic *pic = (struct ui_pic *)ctr;
    static u8 touch_action = 0;
    u8 card_set_num;

    switch (e->event) {
    case ELM_EVENT_TOUCH_UP:
        if (touch_action != 1) {
            break;
        }
        card_set_num = get_ui_sys_param(CardSetNum);
        if (card_set_num > MAX_CARD_SELECT) {
            log_info("card_select overflow!!!\n");
            ui_show(LAYOUT_CARD_WARN);
            break;
        }
        set.show_layout = LAYOUT_SELECT_CARD;
        ui_hide(LAYOUT_CARD_SET);
        ui_show(set.show_layout);
        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        touch_action = 2;
        break;
    case ELM_EVENT_TOUCH_DOWN:
        touch_action = 1;
        return true;
    case ELM_EVENT_TOUCH_U_MOVE:
        break;
    case ELM_EVENT_TOUCH_D_MOVE:
        break;
    }
    return false;
}

static int LAYOUT_CARD_WARN_ontouch(void *ctr, struct element_touch_event *e)
{
    static u8 touch_action = 0;
    switch (e->event) {
    case ELM_EVENT_TOUCH_UP:
        if (touch_action != 1) {
            break;
        }
        ui_hide(LAYOUT_CARD_WARN);
        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        break;
    case ELM_EVENT_TOUCH_DOWN:
        touch_action = 1;
        return true;
    case ELM_EVENT_TOUCH_U_MOVE:
        break;
    case ELM_EVENT_TOUCH_D_MOVE:
        break;
    }
    return false;
}




REGISTER_UI_EVENT_HANDLER(LAYOUT_CARD_WARN)//????????????-????????????-????????????(add)
.onchange = NULL,
 .onkey = NULL,
  .ontouch = LAYOUT_CARD_WARN_ontouch,
};


REGISTER_UI_EVENT_HANDLER(PIC_CARD_SET_ADD)//????????????-????????????-????????????(add)
.onchange = list_pic_card_add_onchange,
 .onkey = NULL,
  .ontouch = list_pic_card_add_ontouch,
};


static int list_pic_card_del_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_pic *pic = (struct ui_pic *)_ctrl;
    int index;

    switch (event) {
    case ON_CHANGE_INIT:
        break;
    case ON_CHANGE_HIGHLIGHT:
        break;
    case ON_CHANGE_UPDATE_ITEM:
        index = (u32)arg;

        switch (pic->elm.id) {
        case PIC_CARD_SET_DEL0:
            set.vlist_card_index[0] = index;
            if (index == 0) {
                ui_pic_set_hide_by_id(PIC_CARD_SET_DEL0, true);
            } else {
                ui_pic_set_hide_by_id(PIC_CARD_SET_DEL0, false);
            }
            break;
        case PIC_CARD_SET_DEL1:
            set.vlist_card_index[1] = index;
            break;
        case PIC_CARD_SET_DEL2:
            set.vlist_card_index[2] = index;
            break;
        case PIC_CARD_SET_DEL3:
            set.vlist_card_index[3] = index;
            break;
        default:
            break;
        }

        break;
    }
    return FALSE;
}

static int list_pic_card_del_ontouch(void *ctr, struct element_touch_event *e)
{
    struct ui_pic *pic = (struct ui_pic *)ctr;
    static u8 touch_action = 0;
    int index, i, j;
    int row, col;
    u8 card_set_num;

    switch (e->event) {
    case ELM_EVENT_TOUCH_UP:
        if (touch_action != 1) {
            break;
        }
        card_set_num = get_ui_sys_param(CardSetNum);
        if (card_set_num <= 0) {
            break;
        }
        switch (pic->elm.id) {
        case PIC_CARD_SET_DEL0:
            index = set.vlist_card_index[0];
            break;
        case PIC_CARD_SET_DEL1:
            index = set.vlist_card_index[1];
            break;
        case PIC_CARD_SET_DEL2:
            index = set.vlist_card_index[2];
            break;
        case PIC_CARD_SET_DEL3:
            index = set.vlist_card_index[3];
            break;
        default:
            return false;
        }
        if (index < 1) {
            break;
        }
        index--;
        /* log_info("del_index %d\n", index); */

        u8 card_select_num = sizeof(set.card) - card_set_num;

        for (i = 0; i < card_select_num; i++) {
            if (ui_sys_param.card_select[index] < (set.card[i] - 1)) {
                /* for(j = i;j < (card_select_num - 1);j++) { */
                for (j = (card_select_num - 1); j >= i; j--) {
                    set.card[j + 1] = set.card[j];
                }
                set.card[i] = ui_sys_param.card_select[index] + 1;
                break;
            }
            if ((i == card_select_num - 1) && (card_set_num != 0)) {
                set.card[i + 1] = ui_sys_param.card_select[index] + 1;
            }
        }
        for (i = index; i < (sizeof(ui_sys_param.card_select) - 1); i++) {
            ui_sys_param.card_select[i] = ui_sys_param.card_select[i + 1];
        }
        card_set_num--;
        set_ui_sys_param(CardSetNum, card_set_num);

        memset(ui_show_page_list + 3, 0, sizeof(ui_show_page_list) - 3 * sizeof(ui_show_page_list[0]));
        for (i = 0; i < card_set_num; i++) {
            ui_show_page_list[i + 3] = ui_page_list[ui_sys_param.card_select[i]];
        }
        ui_page_list_update(ui_show_page_list, card_set_num + 3);
        if (UIInfo_w_vm_timer == 0) {
            UIInfo_w_vm_timer = sys_timer_add(NULL, write_UIInfo_to_vm, 1000);
        } else {
            sys_timer_re_run(UIInfo_w_vm_timer);
        }

        row = 1;
        col = 0;
        ui_grid_del_dynamic_by_id(VLIST_DY_CARD_SET, &row, &col, 1);
        /* log_info("row %d, col %d\n", row, col); */
        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        touch_action = 2;
        break;
    case ELM_EVENT_TOUCH_DOWN:
        touch_action = 1;
        return true;
    case ELM_EVENT_TOUCH_U_MOVE:
        break;
    case ELM_EVENT_TOUCH_D_MOVE:
        break;
    }

    return false;
}


REGISTER_UI_EVENT_HANDLER(PIC_CARD_SET_DEL0)//????????????-????????????-????????????(del)
.onchange = list_pic_card_del_onchange,
 .onkey = NULL,
  .ontouch = list_pic_card_del_ontouch,
};
REGISTER_UI_EVENT_HANDLER(PIC_CARD_SET_DEL1)//????????????-????????????-????????????(del)
.onchange = list_pic_card_del_onchange,
 .onkey = NULL,
  .ontouch = list_pic_card_del_ontouch,
};
REGISTER_UI_EVENT_HANDLER(PIC_CARD_SET_DEL2)//????????????-????????????-????????????(del)
.onchange = list_pic_card_del_onchange,
 .onkey = NULL,
  .ontouch = list_pic_card_del_ontouch,
};
REGISTER_UI_EVENT_HANDLER(PIC_CARD_SET_DEL3)//????????????-????????????-????????????(del)
.onchange = list_pic_card_del_onchange,
 .onkey = NULL,
  .ontouch = list_pic_card_del_ontouch,
};

static int list_card_select_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_grid *grid = (struct ui_grid *)_ctrl;
    int row, col;
    u8 card_select_num;
    u8 card_set_num;
    int i, j, k;
    static u8 once = 0;
    static u8 skip = 0;

    switch (event) {
    case ON_CHANGE_INIT:
        card_set_num = get_ui_sys_param(CardSetNum);
        card_select_num = sizeof(set.card) - card_set_num;
        set.make_sure = 0;
        set.last_card_set_num = card_set_num;
        for (i = 0; i < MAX_CARD_BACKUP_SELECT; i++) {
            set.card[i] &= ~BIT(7);
        }

        if (once == 0) {
            once = 1;
            if (card_set_num) {
                memset(set.card, 0x7f, MAX_CARD_BACKUP_SELECT);
                for (i = 0, k = 0; i < MAX_CARD_BACKUP_SELECT - card_set_num; i++, k++) {
                    for (j = 0; j < card_set_num; j++) {
                        if (k == ui_sys_param.card_select[j]) {
                            k++;
                            continue;
                        }
                    }
                    set.card[i] = k + 1;
                }
            } else {
                for (i = 0; i < MAX_CARD_BACKUP_SELECT; i++) {
                    set.card[i] = i + 1;
                }
            }
        }

        row = card_select_num + 2;
        col = 1;
        ui_grid_init_dynamic(grid, &row, &col);
        log_info("dynamic_grid %d X %d\n", row, col);

        ui_grid_set_slide_direction(grid, SCROLL_DIRECTION_UD);

        log_info("card_set_num %d\n\n", card_set_num);

        break;
    case ON_CHANGE_RELEASE:
        set_ui_sys_param(CardSetNum, set.last_card_set_num);
        break;
    }
    return 0;
}
static int list_card_select_ontouch(void *ctr, struct element_touch_event *e)
{
    struct ui_grid *grid = (struct ui_grid *)ctr;
    u8 card_select_num = sizeof(set.card) - set.last_card_set_num;
    u8 max_card_select_index = sizeof(set.card) - set.last_card_set_num - 1;
    int index, i, j;
    static u8 touch_action = 0;
    static u8 move_flag = 0;

    switch (e->event) {
    case ELM_EVENT_TOUCH_UP:

        if (touch_action != 1) {
            set.make_sure = 0;
            break;
        }
        if (!set.make_sure) {
            break;
        }
        index = ui_grid_cur_item_dynamic(grid);
        log_info("\n\n\nddddd %d, %d\n\n\n", index, max_card_select_index);
        if (index != (max_card_select_index + 2)) {
            break;
        }

        u8 card_set_num = get_ui_sys_param(CardSetNum);
        set.last_card_set_num = card_set_num;

        for (i = 0; i < card_select_num; i++) {
            if (move_flag) {
                if (i > 0) {
                    i--;
                }
                move_flag = 0;
            }
            if (set.card[i] & BIT(7)) {
                set.card[i] &= ~BIT(7);
                for (j = i; j < (card_select_num - 1); j++) {
                    set.card[j] = set.card[j + 1];
                }
                set.card[card_select_num - 1] = 0x7f;
                move_flag = 1;
            }
        }

        memset(ui_show_page_list + 3, 0, sizeof(ui_show_page_list) - 3 * sizeof(ui_show_page_list[0]));
        for (u8 i = 0; i < card_set_num; i++) {
            ui_show_page_list[i + 3] = ui_page_list[ui_sys_param.card_select[i]];
        }
        ui_page_list_update(ui_show_page_list, card_set_num + 3);
        if (UIInfo_w_vm_timer == 0) {
            UIInfo_w_vm_timer = sys_timer_add(NULL, write_UIInfo_to_vm, 1000);
        } else {
            sys_timer_re_run(UIInfo_w_vm_timer);
        }
        set.make_sure = 0;
        set.show_layout = LAYOUT_CARD_SET;
        ui_hide(LAYOUT_SELECT_CARD);
        ui_show(set.show_layout);
        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        touch_action = 2;
        break;
    case ELM_EVENT_TOUCH_DOWN:
        touch_action = 1;
        break;
    case ELM_EVENT_TOUCH_U_MOVE:
        break;
    case ELM_EVENT_TOUCH_D_MOVE:
        break;
    }

    return false;
}
REGISTER_UI_EVENT_HANDLER(VLIST_DY_CARD_SELECT)//????????????-??????????????????
.onchange = list_card_select_onchange,
 .onkey = NULL,
  .ontouch = list_card_select_ontouch,
};

static int list_text_card_select_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_text *text = (struct ui_text *)_ctrl;
    u8 index_buf[1];
    /* static u16 store_buf[4][2]; */
    /* u8 *store_buf_text; */
    int index;
    u8 max_card_select_index = sizeof(set.card) - set.last_card_set_num - 1;

    switch (event) {
    case ON_CHANGE_INIT:
        break;
    case ON_CHANGE_HIGHLIGHT:
        break;
    case ON_CHANGE_UPDATE_ITEM:
        index = (u32)arg;
        if (index > (max_card_select_index + 2)) {
            log_info("card overflow0 %d\n", index);
            break;
        }
        switch (text->elm.id) {
        case TEXT_CARD_SELECT0:
            /* store_buf_text = store_buf[0]; */
            break;
        case TEXT_CARD_SELECT1:
            /* store_buf_text = store_buf[1]; */
            break;
        case TEXT_CARD_SELECT2:
            /* store_buf_text = store_buf[2]; */
            break;
        case TEXT_CARD_SELECT3:
            /* store_buf_text = store_buf[3]; */
            break;
        default:
            return FALSE;
        }

        if (index == 0) {
            index_buf[0] = 0;
        } else if (index == (max_card_select_index + 2)) {
            index_buf[0] = sizeof(set.card) + 1;
        } else {
            index -= 1;
            index_buf[0] = set.card[index] & 0x7f;
        }
        /* log_info("\n\n\n\n\n\nqueding %d, %d\n", index, index_buf[0]); */

        ui_text_set_index(text, index_buf[0]);
        /* ui_text_set_combine_index(text, store_buf_text, index_buf, 1); */
        break;
    }
    return FALSE;
}

static int list_text_card_select_ok_ontouch(void *ctr, struct element_touch_event *e)
{
    /* struct ui_text *text = (struct ui_text *)ctr; */

    switch (e->event) {
    case ELM_EVENT_TOUCH_UP:
        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        break;
    case ELM_EVENT_TOUCH_DOWN:
        set.make_sure = 1;
        break;
    case ELM_EVENT_TOUCH_U_MOVE:
        break;
    case ELM_EVENT_TOUCH_D_MOVE:
        break;
    }

    return false;
}
REGISTER_UI_EVENT_HANDLER(TEXT_CARD_SELECT0)//????????????-????????????-????????????
.onchange = list_text_card_select_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_CARD_SELECT1)//????????????-????????????-????????????
.onchange = list_text_card_select_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_CARD_SELECT2)//????????????-????????????-????????????
.onchange = list_text_card_select_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_CARD_SELECT3)//????????????-????????????-????????????
.onchange = list_text_card_select_onchange,
 .onkey = NULL,
  .ontouch = list_text_card_select_ok_ontouch,
};

static int list_pic_card_select_icon_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_pic *pic = (struct ui_pic *)_ctrl;
    int index;
    u8 max_card_select_index = sizeof(set.card) - set.last_card_set_num - 1;

    switch (event) {
    case ON_CHANGE_INIT:
        break;
    case ON_CHANGE_HIGHLIGHT:
        break;
    case ON_CHANGE_UPDATE_ITEM:
        index = (u32)arg;
        if (index > (max_card_select_index + 2)) {
            log_info("card overflow1 %d\n", index);
            break;
        }

        switch (pic->elm.id) {
        case PIC_CARD_SELECT_L0:
            /* log_info("a0 %d\n", index); */
            if (index == 0) {
                ui_pic_set_hide_by_id(PIC_CARD_SELECT_L0, true);
                return FALSE;
            } else {
                ui_pic_set_hide_by_id(PIC_CARD_SELECT_L0, false);
            }
            break;
        case PIC_CARD_SELECT_L1:
            /* log_info("a1 %d\n", index); */
            break;
        case PIC_CARD_SELECT_L2:
            /* log_info("a2 %d\n", index); */
            break;
        case PIC_CARD_SELECT_L3:
            /* log_info("a3 %d\n", index); */
            if (index == (max_card_select_index + 2)) {
                ui_pic_set_hide_by_id(PIC_CARD_SELECT_L3, true);
                return FALSE;
            } else {
                ui_pic_set_hide_by_id(PIC_CARD_SELECT_L3, false);
            }
            break;
        default:
            return false;
        }
        if (index < 1) {
            break;
        }
        index--;

        /* log_info("\n\n\n\n\n\nmyindex %d, %d\n", index, set.card[index] & 0x7f); */
        ui_pic_set_image_index(pic, (set.card[index] & 0x7f) - 1);
        break;
    }
    return FALSE;
}
REGISTER_UI_EVENT_HANDLER(PIC_CARD_SELECT_L0)//????????????-????????????-????????????
.onchange = list_pic_card_select_icon_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(PIC_CARD_SELECT_L1)//????????????-????????????-????????????
.onchange = list_pic_card_select_icon_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(PIC_CARD_SELECT_L2)//????????????-????????????-????????????
.onchange = list_pic_card_select_icon_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(PIC_CARD_SELECT_L3)//????????????-????????????-????????????
.onchange = list_pic_card_select_icon_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};

static int list_pic_card_select_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_pic *pic = (struct ui_pic *)_ctrl;
    int index;
    u8 max_card_select_index = sizeof(set.card) - set.last_card_set_num - 1;

    switch (event) {
    case ON_CHANGE_INIT:
        break;
    case ON_CHANGE_HIGHLIGHT:
        break;
    case ON_CHANGE_UPDATE_ITEM:
        index = (u32)arg;
        if (index > (max_card_select_index + 2)) {
            break;
        }
        switch (pic->elm.id) {
        case PIC_CARD_SELECT_R0:
            set.vlist_card_index[0] = index;
            if (index == 0) {
                ui_pic_set_hide_by_id(PIC_CARD_SELECT_R0, true);
            } else {
                ui_pic_set_hide_by_id(PIC_CARD_SELECT_R0, false);
            }
            break;
        case PIC_CARD_SELECT_R1:
            set.vlist_card_index[1] = index;
            break;
        case PIC_CARD_SELECT_R2:
            set.vlist_card_index[2] = index;
            break;
        case PIC_CARD_SELECT_R3:
            if (index == (max_card_select_index + 2)) {
                ui_pic_set_hide_by_id(PIC_CARD_SELECT_R3, true);
                break;
            } else {
                ui_pic_set_hide_by_id(PIC_CARD_SELECT_R3, false);
            }

            set.vlist_card_index[3] = index;
            break;
        default:
            return FALSE;
        }
        if (index < 1) {
            break;
        }
        index--;

        if (set.card[index] & BIT(7)) {
            ui_pic_set_image_index(pic, 1);
        } else {
            ui_pic_set_image_index(pic, 0);
        }
        break;
    }
    return FALSE;
}

static int list_pic_card_select_ontouch(void *ctr, struct element_touch_event *e)
{
    struct ui_pic *pic = (struct ui_pic *)ctr;
    static u8 touch_action = 0;
    int index, tmp_index, i;
    int row, col;
    u8 card_set_num;

    switch (e->event) {
    case ELM_EVENT_TOUCH_UP:
        if (touch_action != 1) {
            break;
        }

        card_set_num = get_ui_sys_param(CardSetNum);
        if ((card_set_num + 1) > MAX_CARD_SELECT) {
            log_info(">>>>>>>>>%s", __func__);
            ui_show(LAYOUT_CARD_WARN);
            break;
        }
        switch (pic->elm.id) {
        case PIC_CARD_SELECT_R0:
            index = set.vlist_card_index[0];
            break;
        case PIC_CARD_SELECT_R1:
            index = set.vlist_card_index[1];
            break;
        case PIC_CARD_SELECT_R2:
            index = set.vlist_card_index[2];
            break;
        case PIC_CARD_SELECT_R3:
            index = set.vlist_card_index[3];
            break;
        default:
            return false;
        }
        if (index < 1) {
            break;
        }
        index--;

        if (set.card[index] & BIT(7)) {

            set.card[index] &= ~BIT(7);
            tmp_index = 0;

            for (i = 0; i < card_set_num; i++) {
                if (set.card[index] == (ui_sys_param.card_select[i] + 1)) {
                    tmp_index = i;
                    break;
                }
            }
            for (i = tmp_index; i < (sizeof(ui_sys_param.card_select) - 1); i++) {
                ui_sys_param.card_select[i] = ui_sys_param.card_select[i + 1];
            }
            card_set_num--;

            ui_pic_show_image_by_id(pic->elm.id, 0);
        } else {

            set.card[index] |= BIT(7);
            ui_sys_param.card_select[card_set_num++] = (set.card[index] & 0x7f) - 1;

            ui_pic_show_image_by_id(pic->elm.id, 1);
        }
        set_ui_sys_param(CardSetNum, card_set_num);
        log_info("sel_index %d, %d, %d\n", index, set.card[index] & 0x7f, card_set_num);

        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        touch_action = 2;
        break;
    case ELM_EVENT_TOUCH_DOWN:
        touch_action = 1;
        return true;
    case ELM_EVENT_TOUCH_U_MOVE:
        break;
    case ELM_EVENT_TOUCH_D_MOVE:
        break;
    }

    return false;
}
REGISTER_UI_EVENT_HANDLER(PIC_CARD_SELECT_R0)//????????????-????????????-????????????(sel)
.onchange = list_pic_card_select_onchange,
 .onkey = NULL,
  .ontouch = list_pic_card_select_ontouch,
};
REGISTER_UI_EVENT_HANDLER(PIC_CARD_SELECT_R1)//????????????-????????????-????????????(sel)
.onchange = list_pic_card_select_onchange,
 .onkey = NULL,
  .ontouch = list_pic_card_select_ontouch,
};
REGISTER_UI_EVENT_HANDLER(PIC_CARD_SELECT_R2)//????????????-????????????-????????????(sel)
.onchange = list_pic_card_select_onchange,
 .onkey = NULL,
  .ontouch = list_pic_card_select_ontouch,
};
REGISTER_UI_EVENT_HANDLER(PIC_CARD_SELECT_R3)//????????????-????????????-????????????(sel)
.onchange = list_pic_card_select_onchange,
 .onkey = NULL,
  .ontouch = list_pic_card_select_ontouch,
};

static int layout_card_select_ontouch(void *ctr, struct element_touch_event *e)
{
    struct layout *layout = (struct layout *)ctr;

    switch (e->event) {
    case ELM_EVENT_TOUCH_R_MOVE:
        if (get_ui_sys_param(CardSetNum) > 0) {
            set.show_layout = LAYOUT_CARD_SET;
            ui_hide(LAYOUT_SELECT_CARD);
            ui_show(set.show_layout);
        } else {
            set.show_layout = LAYOUT_CARD_SET_NULL;
            ui_hide(LAYOUT_SELECT_CARD);
            ui_show(set.show_layout);
        }
        break;
    case ELM_EVENT_TOUCH_UP:
        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        break;
    case ELM_EVENT_TOUCH_DOWN:
        break;
    case ELM_EVENT_TOUCH_U_MOVE:
        break;
    case ELM_EVENT_TOUCH_D_MOVE:
        break;
    }

    return false;
}
REGISTER_UI_EVENT_HANDLER(LAYOUT_SELECT_CARD)//????????????
.onchange = NULL,
 .onkey = NULL,
  .ontouch = layout_card_select_ontouch,
};

static int pic_card_select_add_ontouch(void *ctr, struct element_touch_event *e)
{
    /* struct ui_pic *pic = (struct ui_pic *)ctr; */

    switch (e->event) {
    case ELM_EVENT_TOUCH_UP:
        set.show_layout = LAYOUT_SELECT_CARD;
        ui_hide(LAYOUT_CARD_SET_NULL);
        ui_show(set.show_layout);
        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        break;
    case ELM_EVENT_TOUCH_DOWN:
        return true;
    case ELM_EVENT_TOUCH_U_MOVE:
        break;
    case ELM_EVENT_TOUCH_D_MOVE:
        break;
    }

    return false;
}
REGISTER_UI_EVENT_HANDLER(PIC_CARD_ADD)//??????add-????????????
.onchange = NULL,
 .onkey = NULL,
  .ontouch = pic_card_select_add_ontouch,
};

static int layout_card_select_add_ontouch(void *ctr, struct element_touch_event *e)
{
    /* struct layout *layout = (struct layout *)ctr; */

    switch (e->event) {
    case ELM_EVENT_TOUCH_R_MOVE:
        set.show_layout = LAYOUT_SCREEN_DISP;
        ui_hide(LAYOUT_CARD_SET_NULL);
        ui_show(set.show_layout);
        break;
    case ELM_EVENT_TOUCH_UP:
        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        break;
    case ELM_EVENT_TOUCH_DOWN:
        break;
    case ELM_EVENT_TOUCH_U_MOVE:
        break;
    case ELM_EVENT_TOUCH_D_MOVE:
        break;
    }

    return false;
}
REGISTER_UI_EVENT_HANDLER(LAYOUT_CARD_SET_NULL)//??????add
.onchange = NULL,
 .onkey = NULL,
  .ontouch = layout_card_select_add_ontouch,
};

static int layout_card_set_ontouch(void *ctr, struct element_touch_event *e)
{
    /* struct layout *layout = (struct layout *)ctr; */

    switch (e->event) {
    case ELM_EVENT_TOUCH_R_MOVE:
        set.show_layout = LAYOUT_SCREEN_DISP;
        ui_hide(LAYOUT_CARD_SET);
        ui_show(set.show_layout);
        break;
    case ELM_EVENT_TOUCH_UP:
        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        break;
    case ELM_EVENT_TOUCH_DOWN:
        break;
    case ELM_EVENT_TOUCH_U_MOVE:
        break;
    case ELM_EVENT_TOUCH_D_MOVE:
        break;
    }

    return false;
}
REGISTER_UI_EVENT_HANDLER(LAYOUT_CARD_SET)//??????add
.onchange = NULL,
 .onkey = NULL,
  .ontouch = layout_card_set_ontouch,
};

//---------------------------????????????-------------------------------//

u8 get_ui_backlight_level()
{
    u8 backlight_level = get_ui_sys_param(LightLevel);
    if (backlight_level == 0) {
        backlight_level = 1;
    }
    return backlight_level;
}

static int light_ajust_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_grid *grid = (struct ui_grid *)_ctrl;

    switch (event) {
    case ON_CHANGE_INIT:
        ui_grid_set_slide_direction(grid, SCROLL_DIRECTION_UD);
        ui_grid_set_hi_index(grid, get_ui_sys_param(LightLevel));
        break;
    }
    return 0;
}

static int light_ajust_ontouch(void *ctr, struct element_touch_event *e)
{
    struct ui_grid *grid = (struct ui_grid *)ctr;
    int sel_item;

    switch (e->event) {
    case ELM_EVENT_TOUCH_UP:
        sel_item = ui_grid_get_hindex(grid);
        /* log_info("sel_item %d, %d\n", sel_item, grid->hi_index); */
        if (sel_item >= 0) {
            /* set.ui_vm_info.light_ajust_sel = sel_item; */
            /* ui_ajust_light(set.ui_vm_info.light_ajust_sel * 2); */
            ui_ajust_light((sel_item + 1) * 2);
        }
        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        break;
    case ELM_EVENT_TOUCH_DOWN:
        break;
    case ELM_EVENT_TOUCH_U_MOVE:
        break;
    case ELM_EVENT_TOUCH_D_MOVE:
        break;
    }

    return false;
}
REGISTER_UI_EVENT_HANDLER(VLIST_LIGHT_AJUST)//????????????-????????????
.onchange = light_ajust_onchange,
 .onkey = NULL,
  .ontouch = light_ajust_ontouch,
};

static int light_ajust_text_ok_ontouch(void *ctr, struct element_touch_event *e)
{
    struct ui_text *text = (struct ui_text *)ctr;

    switch (e->event) {
    case ELM_EVENT_TOUCH_UP:
        set_ui_sys_param(LightLevel, ui_grid_get_hindex(ui_grid_for_id(VLIST_LIGHT_AJUST)));

        set.show_layout = LAYOUT_SCREEN_DISP;
        ui_hide(LAYOUT_AJUST_BRIGHTNESS);
        ui_show(LAYOUT_SCREEN_DISP);
        if (UIInfo_w_vm_timer == 0) {
            UIInfo_w_vm_timer = sys_timer_add(NULL, write_UIInfo_to_vm, 1000);
        } else {
            sys_timer_re_run(UIInfo_w_vm_timer);
        }
        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        break;
    case ELM_EVENT_TOUCH_DOWN:
        return true;
    case ELM_EVENT_TOUCH_U_MOVE:
        break;
    case ELM_EVENT_TOUCH_D_MOVE:
        break;
    }

    return false;
}
REGISTER_UI_EVENT_HANDLER(TEXT_LIGHT_AJUST_SURE)//????????????-????????????(??????)
.onchange = NULL,
 .onkey = NULL,
  .ontouch = light_ajust_text_ok_ontouch,
};

static int light_ajust_text_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_text *text = (struct ui_text *)_ctrl;
    int yes = (u32)arg;

    switch (event) {
    case ON_CHANGE_INIT:
        break;
    case ON_CHANGE_HIGHLIGHT:
        ui_text_set_index(text, !!yes);
        break;
    default:
        return FALSE;
    }
    return FALSE;
}
REGISTER_UI_EVENT_HANDLER(TEXT_LIGHT_AJUST0)//????????????-????????????-????????????
.onchange = light_ajust_text_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_LIGHT_AJUST1)//????????????-????????????-????????????
.onchange = light_ajust_text_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_LIGHT_AJUST2)//????????????-????????????-????????????
.onchange = light_ajust_text_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_LIGHT_AJUST3)//????????????-????????????-????????????
.onchange = light_ajust_text_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_LIGHT_AJUST4)//????????????-????????????-????????????
.onchange = light_ajust_text_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_LIGHT_AJUST5)//????????????-????????????-????????????
.onchange = light_ajust_text_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};


static int layout_light_ajust_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    /* struct layout *layout = (struct layout *)_ctrl; */
    switch (event) {
    case ON_CHANGE_INIT:
        break;
    case ON_CHANGE_RELEASE:
        ui_ajust_light((get_ui_sys_param(LightLevel) + 1) * 2);
        log_info("%s %d", __FUNCTION__, __LINE__);
        break;
    }
    return 0;
}
static int layout_light_ajust_ontouch(void *ctr, struct element_touch_event *e)
{
    struct layout *layout = (struct layout *)ctr;

    switch (e->event) {
    case ELM_EVENT_TOUCH_R_MOVE:
        set.show_layout = LAYOUT_SCREEN_DISP;
        ui_hide(LAYOUT_AJUST_BRIGHTNESS);
        ui_show(LAYOUT_SCREEN_DISP);
        break;
    case ELM_EVENT_TOUCH_UP:
        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        break;
    case ELM_EVENT_TOUCH_DOWN:
        break;
    case ELM_EVENT_TOUCH_U_MOVE:
        break;
    case ELM_EVENT_TOUCH_D_MOVE:
        break;
    }

    return false;
}
REGISTER_UI_EVENT_HANDLER(LAYOUT_AJUST_BRIGHTNESS)//????????????
.onchange = layout_light_ajust_onchange,
 .onkey = NULL,
  .ontouch = layout_light_ajust_ontouch,
};


//------------------------------??????????????????-------------------------------//

static int layout_high_set_ontouch(void *ctr, struct element_touch_event *e)
{
    struct layout *layout = (struct layout *)ctr;

    switch (e->event) {
    case ELM_EVENT_TOUCH_R_MOVE:
        set.show_layout = LAYOUT_SCREEN_DISP;
        ui_hide(LAYOUT_HIGH_SET);
        ui_show(LAYOUT_SCREEN_DISP);
        break;
    case ELM_EVENT_TOUCH_UP:
        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        break;
    case ELM_EVENT_TOUCH_DOWN:
        break;
    case ELM_EVENT_TOUCH_U_MOVE:
        break;
    case ELM_EVENT_TOUCH_D_MOVE:
        break;
    }

    return false;
}

REGISTER_UI_EVENT_HANDLER(LAYOUT_HIGH_SET)//????????????
.onchange = NULL,
 .onkey = NULL,
  .ontouch = layout_high_set_ontouch,
};

static int list_high_set_ontouch(void *ctr, struct element_touch_event *e)
{
    struct ui_grid *grid = (struct ui_grid *)ctr;
    static u8 touch_action = 0;
    int sel_item;

    switch (e->event) {
    case ELM_EVENT_TOUCH_UP:
        if (touch_action != 1) {
            break;
        }

        sel_item = ui_grid_cur_item(grid);
        if (sel_item <= 0) {
            break;
        }
        switch (sel_item) {
        case 1://????????????
            set.show_layout = LAYOUT_SCREEN_DARK_TIME;
            ui_hide(LAYOUT_HIGH_SET);
            ui_show(LAYOUT_SCREEN_DARK_TIME);
            break;
        case 2://????????????
            set.show_layout = LAYOUT_LIGHT_TIME;
            ui_hide(LAYOUT_HIGH_SET);
            ui_show(LAYOUT_LIGHT_TIME);
            break;
        }

        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        touch_action = 2;
        break;
    case ELM_EVENT_TOUCH_DOWN:
        touch_action = 1;
        break;
    case ELM_EVENT_TOUCH_U_MOVE:
        break;
    case ELM_EVENT_TOUCH_D_MOVE:
        break;
    }

    return false;
}
REGISTER_UI_EVENT_HANDLER(VLIST_HIGH_SET)//????????????-????????????
.onchange = NULL,
 .onkey = NULL,
  .ontouch = list_high_set_ontouch,
};

static int list_text_high_set_dark_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_text *text = (struct ui_text *)_ctrl;
    /* u8 index_buf[1]; */
    /* static u16 store_buf[2]; */

    switch (event) {
    case ON_CHANGE_INIT:
        ui_text_set_index(text, get_ui_sys_param(DarkTime));
        /* index_buf[0] = set.ui_vm_info.screen_dark_time_sel; */
        /* ui_text_set_combine_index(text, store_buf, index_buf, 1); */
        break;
    case ON_CHANGE_HIGHLIGHT:
        /* ui_text_set_index(text, !!yes); */
        break;
    default:
        return FALSE;
    }
    return FALSE;
}
REGISTER_UI_EVENT_HANDLER(TEXT_SDT_SEL)//????????????-????????????-????????????(?????????????????????)
.onchange = list_text_high_set_dark_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};

static int list_text_high_set_light_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_text *text = (struct ui_text *)_ctrl;
    /* u8 index_buf[1]; */
    /* static u16 store_buf[2]; */

    switch (event) {
    case ON_CHANGE_INIT:
        ui_text_set_index(text, get_ui_sys_param(LightTime));
        /* index_buf[0] = set.light_time_sel; */
        /* ui_text_set_combine_index(text, store_buf, index_buf, 1); */
        break;
    case ON_CHANGE_HIGHLIGHT:
        /* ui_text_set_index(text, !!yes); */
        break;
    default:
        return FALSE;
    }
    return FALSE;
}
REGISTER_UI_EVENT_HANDLER(TEXT_LIGHT_TIME_SEL)//????????????-????????????-????????????(?????????????????????)
.onchange = list_text_high_set_light_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};

//--------------------------------??????????????????-------------------------------//

static int list_screen_dark_time_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_grid *grid = (struct ui_grid *)_ctrl;

    switch (event) {
    case ON_CHANGE_INIT:
        ui_grid_set_slide_direction(grid, SCROLL_DIRECTION_UD);
        ui_grid_set_hi_index(grid, get_ui_sys_param(DarkTime));
        break;
    }
    return 0;
}
REGISTER_UI_EVENT_HANDLER(VLIST_SDT)//????????????-????????????
.onchange = list_screen_dark_time_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};

static int text_screen_dark_time_ok_ontouch(void *ctr, struct element_touch_event *e)
{
    struct ui_text *text = (struct ui_text *)ctr;
    u8 screen_dark_time_sel;

    switch (e->event) {
    case ELM_EVENT_TOUCH_UP:
        screen_dark_time_sel = ui_grid_get_hindex(ui_grid_for_id(VLIST_SDT));
        ui_set_dark_time(screen_dark_time_sel);
        set_ui_sys_param(DarkTime, screen_dark_time_sel);

        set.show_layout = LAYOUT_HIGH_SET;
        ui_hide(LAYOUT_SCREEN_DARK_TIME);
        ui_show(set.show_layout);
        if (UIInfo_w_vm_timer == 0) {
            UIInfo_w_vm_timer = sys_timer_add(NULL, write_UIInfo_to_vm, 1000);
        } else {
            sys_timer_re_run(UIInfo_w_vm_timer);
        }
        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        break;
    case ELM_EVENT_TOUCH_DOWN:
        return true;
    case ELM_EVENT_TOUCH_U_MOVE:
        break;
    case ELM_EVENT_TOUCH_D_MOVE:
        break;
    }

    return false;
}
REGISTER_UI_EVENT_HANDLER(TEXT_SDT_SURE)//????????????-????????????-????????????(??????)
.onchange = NULL,
 .onkey = NULL,
  .ontouch = text_screen_dark_time_ok_ontouch,
};


static int list_text_screen_dark_time_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_text *text = (struct ui_text *)_ctrl;
    int yes = (u32)arg;

    switch (event) {
    case ON_CHANGE_INIT:
        break;
    case ON_CHANGE_HIGHLIGHT:
        ui_text_set_index(text, !!yes);
        break;
    default:
        return FALSE;
    }
    return FALSE;
}
REGISTER_UI_EVENT_HANDLER(TEXT_SDT0)//????????????-????????????-????????????
.onchange = list_text_screen_dark_time_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_SDT1)//????????????-????????????-????????????
.onchange = list_text_screen_dark_time_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_SDT2)//????????????-????????????-????????????
.onchange = list_text_screen_dark_time_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_SDT3)//????????????-????????????-????????????
.onchange = list_text_screen_dark_time_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};


static int layout_screen_dark_time_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    /* struct layout *layout = (struct layout *)_ctrl; */
    switch (event) {
    case ON_CHANGE_INIT:
        break;
    case ON_CHANGE_RELEASE:
        break;
    }
    return 0;
}
static int layout_screen_dark_time_ontouch(void *ctr, struct element_touch_event *e)
{
    struct layout *layout = (struct layout *)ctr;

    switch (e->event) {
    case ELM_EVENT_TOUCH_R_MOVE:
        set.show_layout = LAYOUT_HIGH_SET;
        ui_hide(LAYOUT_SCREEN_DARK_TIME);
        ui_show(set.show_layout);
        break;
    case ELM_EVENT_TOUCH_UP:
        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        break;
    case ELM_EVENT_TOUCH_DOWN:
        break;
    case ELM_EVENT_TOUCH_U_MOVE:
        break;
    case ELM_EVENT_TOUCH_D_MOVE:
        break;
    }

    return false;
}
REGISTER_UI_EVENT_HANDLER(LAYOUT_SCREEN_DARK_TIME)//????????????
.onchange = layout_screen_dark_time_onchange,
 .onkey = NULL,
  .ontouch = layout_screen_dark_time_ontouch,
};

//-----------------------------------????????????-------------------------------//

u8 get_light_time_sel()
{
    return get_ui_sys_param(LightTime);
}

static int list_light_time_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_grid *grid = (struct ui_grid *)_ctrl;

    switch (event) {
    case ON_CHANGE_INIT:
        ui_grid_set_slide_direction(grid, SCROLL_DIRECTION_UD);
        ui_grid_set_hi_index(grid, get_ui_sys_param(LightTime));
        break;
    }
    return 0;
}
REGISTER_UI_EVENT_HANDLER(VLIST_LIGHT_TIME)//????????????-????????????
.onchange = list_light_time_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};

static int text_light_time_ok_ontouch(void *ctr, struct element_touch_event *e)
{
    struct ui_text *text = (struct ui_text *)ctr;

    switch (e->event) {
    case ELM_EVENT_TOUCH_UP:
        set_ui_sys_param(LightTime, ui_grid_get_hindex(ui_grid_for_id(VLIST_LIGHT_TIME)));
        set.show_layout = LAYOUT_HIGH_SET;
        ui_hide(LAYOUT_LIGHT_TIME);
        ui_show(set.show_layout);
        if (UIInfo_w_vm_timer == 0) {
            UIInfo_w_vm_timer = sys_timer_add(NULL, write_UIInfo_to_vm, 1000);
        } else {
            sys_timer_re_run(UIInfo_w_vm_timer);
        }
        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        break;
    case ELM_EVENT_TOUCH_DOWN:
        return true;
    case ELM_EVENT_TOUCH_U_MOVE:
        break;
    case ELM_EVENT_TOUCH_D_MOVE:
        break;
    }

    return false;
}
REGISTER_UI_EVENT_HANDLER(VLIST_LIGHT_TIME_SURE)//????????????-????????????(??????)
.onchange = NULL,
 .onkey = NULL,
  .ontouch = text_light_time_ok_ontouch,
};

static int list_text_light_time_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_text *text = (struct ui_text *)_ctrl;
    int yes = (u32)arg;

    switch (event) {
    case ON_CHANGE_INIT:
        break;
    case ON_CHANGE_HIGHLIGHT:
        ui_text_set_index(text, !!yes);
        break;
    default:
        return FALSE;
    }
    return FALSE;
}
REGISTER_UI_EVENT_HANDLER(TEXT_LIGHT_TIME0)//????????????-????????????-????????????
.onchange = list_text_light_time_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_LIGHT_TIME1)//????????????-????????????-????????????
.onchange = list_text_light_time_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_LIGHT_TIME2)//????????????-????????????-????????????
.onchange = list_text_light_time_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_LIGHT_TIME3)//????????????-????????????-????????????
.onchange = list_text_light_time_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};

static int layout_light_time_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    /* struct layout *layout = (struct layout *)_ctrl; */
    switch (event) {
    case ON_CHANGE_INIT:
        break;
    case ON_CHANGE_RELEASE:
        break;
    }
    return 0;
}

static int layout_light_time_ontouch(void *ctr, struct element_touch_event *e)
{
    struct layout *layout = (struct layout *)ctr;

    switch (e->event) {
    case ELM_EVENT_TOUCH_R_MOVE:
        set.show_layout = LAYOUT_HIGH_SET;
        ui_hide(LAYOUT_LIGHT_TIME);
        ui_show(set.show_layout);
        break;
    case ELM_EVENT_TOUCH_UP:
        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        break;
    case ELM_EVENT_TOUCH_DOWN:
        break;
    case ELM_EVENT_TOUCH_U_MOVE:
        break;
    case ELM_EVENT_TOUCH_D_MOVE:
        break;
    }

    return false;
}
REGISTER_UI_EVENT_HANDLER(LAYOUT_LIGHT_TIME)//??????????????????
.onchange = layout_light_time_onchange,
 .onkey = NULL,
  .ontouch = layout_light_time_ontouch,
};

//--------------------------------??????????????????--------------------------------//

static int list_voice_set_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_grid *grid = (struct ui_grid *)_ctrl;

    switch (event) {
    case ON_CHANGE_INIT:
        ui_grid_set_slide_direction(grid, SCROLL_DIRECTION_UD);
        break;
    }
    return 0;
}

static int list_voice_set_ontouch(void *ctr, struct element_touch_event *e)
{
    struct ui_grid *grid = (struct ui_grid *)ctr;
    static u8 touch_action = 0;
    int sel_item;

    switch (e->event) {
    case ELM_EVENT_TOUCH_UP:
        if (touch_action != 1) {
            break;
        }

        sel_item = ui_grid_cur_item(grid);
        if (sel_item <= 0) {
            break;
        }
        if (sel_item == 1) {
            set.show_layout = LAYOUT_VOICE;
            ui_hide(LAYOUT_VOICE_SET);
            ui_show(set.show_layout);
        }
        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        touch_action = 2;
        break;
    case ELM_EVENT_TOUCH_DOWN:
        touch_action = 1;
        break;
    case ELM_EVENT_TOUCH_U_MOVE:
        break;
    case ELM_EVENT_TOUCH_D_MOVE:
        break;
    }

    return false;
}
REGISTER_UI_EVENT_HANDLER(VLIST_VOICE_SET)//????????????-????????????
.onchange = list_voice_set_onchange,
 .onkey = NULL,
  .ontouch = list_voice_set_ontouch,
};

static int layout_voice_set_ontouch(void *ctr, struct element_touch_event *e)
{
    struct layout *layout = (struct layout *)ctr;

    switch (e->event) {
    case ELM_EVENT_TOUCH_R_MOVE:
        set.show_layout = LAYOUT_VOICE_SET;
        ui_send_event(KEY_CHANGE_PAGE, BIT(31) | PAGE_SET);
        break;
    case ELM_EVENT_TOUCH_UP:
        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        break;
    case ELM_EVENT_TOUCH_DOWN:
        break;
    case ELM_EVENT_TOUCH_U_MOVE:
        break;
    case ELM_EVENT_TOUCH_D_MOVE:
        break;
    }

    return false;
}
REGISTER_UI_EVENT_HANDLER(LAYOUT_VOICE_SET)//????????????
.onchange = NULL,
 .onkey = NULL,
  .ontouch = layout_voice_set_ontouch,
};

static int pic_voice_mute_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_pic *pic = (struct ui_pic *)_ctrl;

    switch (event) {
    case ON_CHANGE_INIT:
        set.vslider_percent = ui_voice_to_percent(app_audio_get_volume(APP_AUDIO_STATE_MUSIC));
        if (set.vslider_percent == 0) {
            ui_pic_set_image_index(pic, 1);
            set_ui_sys_param(SysVoiceMute, 1);
        } else {
            ui_pic_set_image_index(pic, 0);
            set_ui_sys_param(SysVoiceMute, 0);
        }
        break;
    }
    return 0;
}

static int pic_voice_mute_ontouch(void *ctr, struct element_touch_event *e)
{
    /* struct ui_pic *pic = (struct ui_pic *)ctr; */
    static u8 touch_action = 0;
    u8 voice_mute_sel;

    switch (e->event) {
    case ELM_EVENT_TOUCH_UP:
        if (touch_action != 1) {
            break;
        }
        voice_mute_sel = get_ui_sys_param(SysVoiceMute);
        voice_mute_sel = !voice_mute_sel;
        ui_pic_show_image_by_id(PIC_VOICE_MUTE, !!voice_mute_sel);
        ui_voice_mute(voice_mute_sel);
        set_ui_sys_param(SysVoiceMute, voice_mute_sel);
        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        touch_action = 2;
        break;
    case ELM_EVENT_TOUCH_DOWN:
        touch_action = 1;
        return true;
    case ELM_EVENT_TOUCH_U_MOVE:
        break;
    case ELM_EVENT_TOUCH_D_MOVE:
        break;
    }

    return false;
}
REGISTER_UI_EVENT_HANDLER(PIC_VOICE_MUTE)//????????????-????????????-??????????????????????????????
.onchange = pic_voice_mute_onchange,
 .onkey = NULL,
  .ontouch = pic_voice_mute_ontouch,
};

static int vslider_voice_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_vslider *vslider = (struct ui_vslider *)_ctrl;
    u8 voice_mute_sel;

    switch (event) {
    case ON_CHANGE_INIT:
        voice_mute_sel = get_ui_sys_param(SysVoiceMute);
        if (voice_mute_sel == 1) {
            set.vslider_percent = 0;
            ui_set_voice(set.vslider_percent);
        } else {
            set.vslider_percent = ui_voice_to_percent(app_audio_get_volume(APP_AUDIO_STATE_MUSIC));
        }
        /* log_info("%s %d--vslider_percent:%d",__FUNCTION__,__LINE__,set.vslider_percent); */
        ui_vslider_set_persent(vslider, set.vslider_percent);
        break;
    case ON_CHANGE_RELEASE:
        set.vslider_percent = vslider_get_percent(vslider);

        if (set.vslider_percent == 0) {
            voice_mute_sel = 1;
        } else {
            voice_mute_sel = 0;
        }
        set_ui_sys_param(SysVoiceMute, voice_mute_sel);
        break;
    }
    return 0;
}

static int vslider_voice_ontouch(void *ctr, struct element_touch_event *e)
{
    struct ui_vslider *vslider = (struct ui_vslider *)ctr;

    switch (e->event) {
    case ELM_EVENT_TOUCH_UP:
        set.vslider_percent = vslider_get_percent(vslider);
        /* log_info("%s %d--vslider_percent:%d",__FUNCTION__,__LINE__,set.vslider_percent); */
        ui_set_voice(set.vslider_percent);

        if (set.vslider_percent == 0) {
            ui_pic_show_image_by_id(PIC_VOICE_MIN, 1);
        } else {
            ui_pic_show_image_by_id(PIC_VOICE_MIN, 0);
        }
        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        vslider_touch_slider_move(vslider, e);
        break;
    case ELM_EVENT_TOUCH_DOWN:
        /* break; */
        return true;
    case ELM_EVENT_TOUCH_U_MOVE:
        break;
    case ELM_EVENT_TOUCH_D_MOVE:
        break;
    }

    return false;
}
REGISTER_UI_EVENT_HANDLER(NEWLAYOUT_21)//??????-vslider
.onchange = vslider_voice_onchange,
 .onkey = NULL,
  .ontouch = vslider_voice_ontouch,
};

static int pic_voice_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_pic *pic = (struct ui_pic *)_ctrl;

    switch (event) {
    case ON_CHANGE_INIT:
        set.vslider_percent = ui_voice_to_percent(app_audio_get_volume(APP_AUDIO_STATE_MUSIC));
        if (set.vslider_percent == 0) {
            ui_pic_set_image_index(pic, 1);
            set_ui_sys_param(SysVoiceMute, 1);
        } else {
            ui_pic_set_image_index(pic, 0);
            set_ui_sys_param(SysVoiceMute, 0);
        }
        break;
    case ON_CHANGE_RELEASE:
        break;
    }
    return 0;
}

static int pic_voice_ontouch(void *ctr, struct element_touch_event *e)
{
    struct ui_pic *pic = (struct ui_pic *)ctr;
    static u8 touch_action = 0;
    u8 voice_mute_sel;

    switch (e->event) {
    case ELM_EVENT_TOUCH_UP:
        if (touch_action != 1) {
            break;
        }
        switch (pic->elm.id) {
        case PIC_VOICE_ADD:
            ui_volume_up(1);
            break;
        case PIC_VOICE_MIN:
            ui_volume_down(1);
            break;
        default:
            return false;
        }
        set.vslider_percent = ui_voice_to_percent(app_audio_get_volume(APP_AUDIO_STATE_MUSIC));
        ui_vslider_set_persent_by_id(NEWLAYOUT_21, set.vslider_percent);
        if (set.vslider_percent == 0) {
            voice_mute_sel = 1;
            ui_pic_show_image_by_id(PIC_VOICE_MIN, 1);
        } else {
            voice_mute_sel = get_ui_sys_param(SysVoiceMute);
            if (voice_mute_sel == 1) {
                ui_pic_show_image_by_id(PIC_VOICE_MIN, 0);
            }
            voice_mute_sel = 0;
        }
        set_ui_sys_param(SysVoiceMute, voice_mute_sel);
        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        touch_action = 2;
        break;
    case ELM_EVENT_TOUCH_DOWN:
        touch_action = 1;
        return true;
    case ELM_EVENT_TOUCH_U_MOVE:
        break;
    case ELM_EVENT_TOUCH_D_MOVE:
        break;
    }

    return false;
}
REGISTER_UI_EVENT_HANDLER(PIC_VOICE_ADD)//?????????
.onchange = NULL,
 .onkey = NULL,
  .ontouch = pic_voice_ontouch,
};
REGISTER_UI_EVENT_HANDLER(PIC_VOICE_MIN)//?????????
.onchange = pic_voice_onchange,
 .onkey = NULL,
  .ontouch = pic_voice_ontouch,
};

static int layout_voice_ontouch(void *ctr, struct element_touch_event *e)
{
    struct layout *layout = (struct layout *)ctr;

    switch (e->event) {
    case ELM_EVENT_TOUCH_R_MOVE:
        set.show_layout = LAYOUT_VOICE_SET;
        ui_hide(LAYOUT_VOICE);
        ui_show(set.show_layout);
        break;
    case ELM_EVENT_TOUCH_UP:
        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        break;
    case ELM_EVENT_TOUCH_DOWN:
        break;
    case ELM_EVENT_TOUCH_U_MOVE:
        break;
    case ELM_EVENT_TOUCH_D_MOVE:
        break;
    }

    return false;
}
REGISTER_UI_EVENT_HANDLER(LAYOUT_VOICE)//??????
.onchange = NULL,
 .onkey = NULL,
  .ontouch = layout_voice_ontouch,
};

//--------------------------??????????????????-------------------------------//

void set_all_day_undisturb_sel(u8 sel)
{
    set_ui_sys_param(AllDayUndisturbEn, sel);
}

u8 get_all_day_undisturb_sel()
{
    return get_ui_sys_param(AllDayUndisturbEn);
}

static int list_undisturb_mode_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_grid *grid = (struct ui_grid *)_ctrl;

    switch (event) {
    case ON_CHANGE_INIT_PROBE:
        if (get_ui_sys_param(TimeUndisturbEn) == 1) {
            ui_grid_set_item_num(grid, 5);
        } else {
            ui_grid_set_item_num(grid, 3);
        }
        break;
    case ON_CHANGE_INIT:
        ui_grid_set_slide_direction(grid, SCROLL_DIRECTION_UD);
        /* ui_grid_set_pix_scroll(grid, true); */
        break;
    }
    return 0;
}

static int list_undisturb_mode_ontouch(void *ctr, struct element_touch_event *e)
{
    struct ui_grid *grid = (struct ui_grid *)ctr;
    static u8 touch_action = 0;
    int sel_item;

    switch (e->event) {
    case ELM_EVENT_TOUCH_UP:
        if (touch_action != 1) {
            break;
        }

        sel_item = ui_grid_cur_item(grid);
        if (sel_item <= 0) {
            break;
        }
        switch (sel_item) {
        case 1://????????????
            break;
        case 2://????????????
            break;
        case 3://????????????
            set.show_layout = LAYOUT_UNDISTURB_MODE_STIME;
            ui_hide(LAYOUT_UNDISTURB_MODE);
            ui_show(set.show_layout);
            break;
        case 4://????????????
            set.show_layout = LAYOUT_UNDISTURB_MODE_ETIME;
            ui_hide(LAYOUT_UNDISTURB_MODE);
            ui_show(set.show_layout);
            break;
        }

        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        touch_action = 2;
        break;
    case ELM_EVENT_TOUCH_DOWN:
        touch_action = 1;
        break;
    case ELM_EVENT_TOUCH_U_MOVE:
        break;
    case ELM_EVENT_TOUCH_D_MOVE:
        break;
    }

    return false;
}
REGISTER_UI_EVENT_HANDLER(VLIST_UNDISTURB)//????????????-????????????
.onchange = list_undisturb_mode_onchange,
 .onkey = NULL,
  .ontouch = list_undisturb_mode_ontouch,
};

static int pic_undisturb_mode_all_day_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_pic *pic = (struct ui_pic *)_ctrl;

    switch (event) {
    case ON_CHANGE_INIT:
        ui_pic_set_image_index(pic, !!get_ui_sys_param(AllDayUndisturbEn));
        break;
    }
    return 0;
}

static int pic_undisturb_mode_all_day_ontouch(void *ctr, struct element_touch_event *e)
{
    struct ui_pic *pic = (struct ui_pic *)ctr;
    static u8 touch_action = 0;
    u8 all_day_undisturb_sel;

    switch (e->event) {
    case ELM_EVENT_TOUCH_UP:
        if (touch_action != 1) {
            break;
        }
        all_day_undisturb_ontouch = 1;
        all_day_undisturb_sel = get_ui_sys_param(AllDayUndisturbEn);
        all_day_undisturb_sel = !all_day_undisturb_sel;
        if (all_day_undisturb_sel) {
            ui_pic_set_image_index(pic, !!all_day_undisturb_sel);
            ui_moto_run(3);//??????
        } else {
            ui_pic_show_image_by_id(PIC_UNDISTURB_ALL_DAY, !!all_day_undisturb_sel);
            ui_moto_run(4);//????????????
        }

        if (all_day_undisturb_sel == 1) {
            set.show_layout = LAYOUT_TIPS_UNDISTURB_MODE;
            ui_hide(LAYOUT_UNDISTURB_MODE);
            ui_show(set.show_layout);
        }
        set_ui_sys_param(AllDayUndisturbEn, all_day_undisturb_sel);
        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        touch_action = 2;
        break;
    case ELM_EVENT_TOUCH_DOWN:
        touch_action = 1;
        return true;
    case ELM_EVENT_TOUCH_U_MOVE:
        break;
    case ELM_EVENT_TOUCH_D_MOVE:
        break;
    }

    return false;
}
REGISTER_UI_EVENT_HANDLER(PIC_UNDISTURB_ALL_DAY)//????????????-??????????????????
.onchange = pic_undisturb_mode_all_day_onchange,
 .onkey = NULL,
  .ontouch = pic_undisturb_mode_all_day_ontouch,
};

static int pic_undisturb_mode_time_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_pic *pic = (struct ui_pic *)_ctrl;

    switch (event) {
    case ON_CHANGE_INIT:
        ui_pic_set_image_index(pic, !!get_ui_sys_param(TimeUndisturbEn));
        break;
    }
    return 0;
}

static int pic_undisturb_mode_time_ontouch(void *ctr, struct element_touch_event *e)
{
    struct ui_pic *pic = (struct ui_pic *)ctr;
    static u8 touch_action = 0;
    u8 time_undisturb_sel;

    switch (e->event) {
    case ELM_EVENT_TOUCH_UP:
        if (touch_action != 1) {
            break;
        }
        time_undisturb_ontouch = 1;
        time_undisturb_sel = get_ui_sys_param(TimeUndisturbEn);
        time_undisturb_sel = !time_undisturb_sel;
        /* ui_pic_show_image_by_id(PIC_UNDISTURB_TIME, !!time_undisturb_sel); */
        ui_pic_set_image_index(pic, !!time_undisturb_sel);

        if (time_undisturb_sel == 1) {
            set.show_layout = LAYOUT_TIPS_UNDISTURB_MODE;
            ui_hide(LAYOUT_UNDISTURB_MODE);
            ui_show(set.show_layout);
            ui_moto_run(5);
        } else {
            ui_hide(LAYOUT_UNDISTURB_MODE);
            ui_show(LAYOUT_UNDISTURB_MODE);
            ui_moto_run(6);
        }
        set_ui_sys_param(TimeUndisturbEn, time_undisturb_sel);
        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        touch_action = 2;
        break;
    case ELM_EVENT_TOUCH_DOWN:
        touch_action = 1;
        return true;
    case ELM_EVENT_TOUCH_U_MOVE:
        break;
    case ELM_EVENT_TOUCH_D_MOVE:
        break;
    }

    return false;
}
REGISTER_UI_EVENT_HANDLER(PIC_UNDISTURB_TIME)//????????????-??????????????????
.onchange = pic_undisturb_mode_time_onchange,
 .onkey = NULL,
  .ontouch = pic_undisturb_mode_time_ontouch,
};

static int time_undisturb_mode_stime_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_time *time = (struct ui_time *)_ctrl;
    struct utime t = {0};

    switch (event) {
    case ON_CHANGE_INIT:
        t.hour = get_ui_sys_param(UndisturbStimeH) % 24;
        t.min = get_ui_sys_param(UndisturbStimeM) % 60;
        ui_time_update(time, &t);
        break;
    }
    return 0;
}
REGISTER_UI_EVENT_HANDLER(TIME_UNDISTURB_STIME)//????????????-????????????-????????????
.onchange = time_undisturb_mode_stime_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};

static int time_undisturb_mode_etime_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_time *time = (struct ui_time *)_ctrl;
    struct utime t = {0};

    switch (event) {
    case ON_CHANGE_INIT:
        t.hour = get_ui_sys_param(UndisturbEtimeH) % 24;
        t.min = get_ui_sys_param(UndisturbEtimeM) % 60;
        ui_time_update(time, &t);
        break;
    }
    return 0;
}
REGISTER_UI_EVENT_HANDLER(TIME_UNDISTURB_ETIME)//????????????-????????????-????????????
.onchange = time_undisturb_mode_etime_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};

static int layout_tips_undisturb_mode_ontouch(void *ctr, struct element_touch_event *e)
{

    switch (e->event) {
    case ELM_EVENT_TOUCH_R_MOVE:
        if (all_day_undisturb_ontouch == 1) {
            all_day_undisturb_ontouch = 0;
            set_ui_sys_param(AllDayUndisturbEn, 0);
        } else if (time_undisturb_ontouch == 1) {
            time_undisturb_ontouch = 0;
            set_ui_sys_param(TimeUndisturbEn, 0);
        }
        set.show_layout = LAYOUT_UNDISTURB_MODE;
        ui_hide(LAYOUT_TIPS_UNDISTURB_MODE);
        ui_show(set.show_layout);
        break;
    }

    return false;
}
REGISTER_UI_EVENT_HANDLER(LAYOUT_TIPS_UNDISTURB_MODE)//????????????tips-????????????????????
.onchange = NULL,
 .onkey = NULL,
  .ontouch = layout_tips_undisturb_mode_ontouch,
};

static int pic_undisturb_mode_tips_no_ontouch(void *ctr, struct element_touch_event *e)
{
    /* struct ui_pic *pic = (struct ui_pic *)ctr; */

    switch (e->event) {
    case ELM_EVENT_TOUCH_UP:
        if (all_day_undisturb_ontouch == 1) {
            all_day_undisturb_ontouch = 0;
            set_ui_sys_param(AllDayUndisturbEn, 0);
        } else if (time_undisturb_ontouch == 1) {
            time_undisturb_ontouch = 0;
            set_ui_sys_param(TimeUndisturbEn, 0);
        }
        ui_moto_run(6);
        set.show_layout = LAYOUT_UNDISTURB_MODE;
        ui_hide(LAYOUT_TIPS_UNDISTURB_MODE);
        ui_show(set.show_layout);
        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        break;
    case ELM_EVENT_TOUCH_DOWN:
        return true;
    case ELM_EVENT_TOUCH_U_MOVE:
        break;
    case ELM_EVENT_TOUCH_D_MOVE:
        break;
    }

    return false;
}
REGISTER_UI_EVENT_HANDLER(PIC_UNDISTURB_TIPSN)//????????????tips-????????????????????
.onchange = NULL,
 .onkey = NULL,
  .ontouch = pic_undisturb_mode_tips_no_ontouch,
};

static int pic_undisturb_mode_tips_yes_ontouch(void *ctr, struct element_touch_event *e)
{
    /* struct ui_pic *pic = (struct ui_pic *)ctr; */

    switch (e->event) {
    case ELM_EVENT_TOUCH_UP:
        if (all_day_undisturb_ontouch) {
            set_ui_sys_param(TimeUndisturbEn, 0);
        }
        if (time_undisturb_ontouch) {
            set_ui_sys_param(AllDayUndisturbEn, 0);
        }
        all_day_undisturb_ontouch = 0;
        time_undisturb_ontouch = 0;
        set.show_layout = LAYOUT_UNDISTURB_MODE;
        ui_hide(LAYOUT_TIPS_UNDISTURB_MODE);
        ui_show(set.show_layout);
        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        break;
    case ELM_EVENT_TOUCH_DOWN:
        return true;
    case ELM_EVENT_TOUCH_U_MOVE:
        break;
    case ELM_EVENT_TOUCH_D_MOVE:
        break;
    }

    return false;
}

REGISTER_UI_EVENT_HANDLER(PIC_UNDISTURB_TIPSY)//????????????tips-????????????????????
.onchange = NULL,
 .onkey = NULL,
  .ontouch = pic_undisturb_mode_tips_yes_ontouch,
};

static int list_time_hour_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_grid *grid = (struct ui_grid *)_ctrl;
    int row, col;
    int base_index_once;
    int time_hour;

    switch (event) {
    case ON_CHANGE_INIT:
        row = 24;
        col = 1;
        ui_grid_init_dynamic(grid, &row, &col);
        log_info("dynamic_grid %d X %d\n", row, col);

        if (grid->elm.id == VLIST_DY_STIME_HOUR) {
            time_hour = get_ui_sys_param(UndisturbStimeH);
        } else {
            time_hour = get_ui_sys_param(UndisturbEtimeH);
        }
        time_hour = time_hour % 24;

        ui_grid_set_hindex_dynamic(grid, time_hour, true, 1);
        /*ui_grid_set_hindex_dynamic(grid, 2, true, 1);*/
        base_index_once = (time_hour >= 1) ? (time_hour - 1) : 0;

        if (!base_index_once) {
            ui_grid_set_base_dynamic(grid, 23);
        } else {
            ui_grid_set_base_dynamic(grid, base_index_once);
        }

        ui_grid_set_slide_direction(grid, SCROLL_DIRECTION_UD);
        break;
    case ON_CHANGE_RELEASE:
        if (set.make_sure) {
            if (grid->elm.id == VLIST_DY_STIME_HOUR) {
                set_ui_sys_param(UndisturbStimeH, ui_grid_get_hindex_dynamic(grid));
            } else {
                set_ui_sys_param(UndisturbEtimeH, ui_grid_get_hindex_dynamic(grid));
            }
        }
        break;
    }
    return 0;
}
REGISTER_UI_EVENT_HANDLER(VLIST_DY_STIME_HOUR)//????????????-????????????-??????????????????
.onchange = list_time_hour_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(VLIST_DY_ETIME_HOUR)//????????????-????????????-??????????????????
.onchange = list_time_hour_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};


static int list_time_min_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_grid *grid = (struct ui_grid *)_ctrl;
    int row, col;
    int base_index_once;
    int time_min;

    u8 start_hour;
    u8 start_min;
    u8 end_hour;
    u8 end_min;
    u32 start_all_time;
    u32 end_all_time;
    u8 new_hour;
    u8 new_min;

    switch (event) {
    case ON_CHANGE_INIT:
        row = 60;
        col = 1;
        ui_grid_init_dynamic(grid, &row, &col);
        log_info("dynamic_grid %d X %d\n", row, col);

        if (grid->elm.id == VLIST_DY_STIME_MIN) {
            time_min = get_ui_sys_param(UndisturbStimeM);
        } else {
            time_min = get_ui_sys_param(UndisturbEtimeM);
        }
        time_min = time_min % 60;
        ui_grid_set_hindex_dynamic(grid, time_min, true, 1);
        base_index_once = (time_min >= 1) ? (time_min - 1) : 0;
        if (!base_index_once) {
            ui_grid_set_base_dynamic(grid, 59);
        } else {
            ui_grid_set_base_dynamic(grid, base_index_once);
        }

        ui_grid_set_slide_direction(grid, SCROLL_DIRECTION_UD);
        break;
    case ON_CHANGE_RELEASE:
        if (set.make_sure) {
            if (grid->elm.id == VLIST_DY_STIME_MIN) {
                set_ui_sys_param(UndisturbStimeM, ui_grid_get_hindex_dynamic(grid));
            } else {
                set_ui_sys_param(UndisturbEtimeM, ui_grid_get_hindex_dynamic(grid));
            }
            start_hour = get_ui_sys_param(UndisturbStimeH);
            start_min  = get_ui_sys_param(UndisturbStimeM);
            end_hour   = get_ui_sys_param(UndisturbEtimeH);
            end_min    = get_ui_sys_param(UndisturbEtimeM);
            start_all_time = start_hour * 60 + start_min;
            end_all_time = end_hour * 60 + end_min;
            if (end_all_time - start_all_time < 5) {
                new_hour = (start_all_time + 5) / 60;
                new_min = (start_all_time + 5) % 60;
                set_ui_sys_param(UndisturbEtimeH, new_hour);
                set_ui_sys_param(UndisturbEtimeM, new_min);
            }
        }
        break;
    }
    return 0;
}
REGISTER_UI_EVENT_HANDLER(VLIST_DY_STIME_MIN)//????????????-????????????-??????????????????
.onchange = list_time_min_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(VLIST_DY_ETIME_MIN)//????????????-????????????-??????????????????
.onchange = list_time_min_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};


static int text_time_ok_ontouch(void *ctr, struct element_touch_event *e)
{
    struct ui_text *text = (struct ui_text *)ctr;

    u8 start_hour;
    u8 start_min;
    u8 end_hour;
    u8 end_min;
    u32 start_all_time;
    u32 end_all_time;
    u8 new_hour;
    u8 new_min;

    switch (e->event) {
    case ELM_EVENT_TOUCH_UP:
        set.make_sure = 1;
        set.show_layout = LAYOUT_UNDISTURB_MODE;
        if (text->elm.id == TEXT_STIME_SURE) {
            ui_hide(LAYOUT_UNDISTURB_MODE_STIME);
        } else {
            ui_hide(LAYOUT_UNDISTURB_MODE_ETIME);
        }
        ui_show(set.show_layout);
        start_hour = get_ui_sys_param(UndisturbStimeH);
        start_min  = get_ui_sys_param(UndisturbStimeM);
        end_hour   = get_ui_sys_param(UndisturbEtimeH);
        end_min    = get_ui_sys_param(UndisturbEtimeM);
        start_all_time = start_hour * 60 + start_min;
        end_all_time = end_hour * 60 + end_min;
        if (end_all_time - start_all_time < 5) {
            new_hour = (start_all_time + 5) / 60;
            new_min = (start_all_time + 5) % 60;
            set_ui_sys_param(UndisturbEtimeH, new_hour);
            set_ui_sys_param(UndisturbEtimeM, new_min);
        }
        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        break;
    case ELM_EVENT_TOUCH_DOWN:
        return true;
    case ELM_EVENT_TOUCH_U_MOVE:
        break;
    case ELM_EVENT_TOUCH_D_MOVE:
        break;
    }

    return false;
}
REGISTER_UI_EVENT_HANDLER(TEXT_STIME_SURE)//????????????-????????????(??????)
.onchange = NULL,
 .onkey = NULL,
  .ontouch = text_time_ok_ontouch,
};
REGISTER_UI_EVENT_HANDLER(TEXT_ETIME_SURE)//????????????-????????????(??????)
.onchange = NULL,
 .onkey = NULL,
  .ontouch = text_time_ok_ontouch,
};

static int text_time_h_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_text *text = (struct ui_text *)_ctrl;
    u8 index_buf;
    int index;

    switch (event) {
    case ON_CHANGE_INIT:
        break;
    case ON_CHANGE_HIGHLIGHT:
        break;
    case ON_CHANGE_UPDATE_ITEM:
        index = (u32)arg;
        index = index % 24;
        log_info("tid %d\n", index);
        if ((index < 0) || (index > 59)) {
            break;
        }
        switch (text->elm.id) {
        case TEXT_STIME_H00:
        case TEXT_STIME_H10:
        case TEXT_STIME_H20:
        case TEXT_STIME_H30:
        case TEXT_ETIME_H00:
        case TEXT_ETIME_H10:
        case TEXT_ETIME_H20:
        case TEXT_ETIME_H30:
            index_buf = index / 10;
            break;
        case TEXT_STIME_H01:
        case TEXT_STIME_H11:
        case TEXT_STIME_H21:
        case TEXT_STIME_H31:
        case TEXT_ETIME_H01:
        case TEXT_ETIME_H11:
        case TEXT_ETIME_H21:
        case TEXT_ETIME_H31:
            index_buf = index % 10;
            break;
        }
        ui_text_set_index(text, index_buf);
        break;
    }
    return FALSE;
}
REGISTER_UI_EVENT_HANDLER(TEXT_STIME_H00)//????????????-????????????-??????????????????-????????????
.onchange = text_time_h_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_STIME_H10)//????????????-????????????-??????????????????-????????????
.onchange = text_time_h_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_STIME_H20)//????????????-????????????-??????????????????-????????????
.onchange = text_time_h_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_STIME_H30)//????????????-????????????-??????????????????-????????????
.onchange = text_time_h_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_ETIME_H00)//????????????-????????????-??????????????????-????????????
.onchange = text_time_h_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_ETIME_H10)//????????????-????????????-??????????????????-????????????
.onchange = text_time_h_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_ETIME_H20)//????????????-????????????-??????????????????-????????????
.onchange = text_time_h_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_ETIME_H30)//????????????-????????????-??????????????????-????????????
.onchange = text_time_h_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_STIME_H01)//????????????-????????????-??????????????????-????????????
.onchange = text_time_h_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_STIME_H11)//????????????-????????????-??????????????????-????????????
.onchange = text_time_h_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_STIME_H21)//????????????-????????????-??????????????????-????????????
.onchange = text_time_h_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_STIME_H31)//????????????-????????????-??????????????????-????????????
.onchange = text_time_h_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_ETIME_H01)//????????????-????????????-??????????????????-????????????
.onchange = text_time_h_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_ETIME_H11)//????????????-????????????-??????????????????-????????????
.onchange = text_time_h_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_ETIME_H21)//????????????-????????????-??????????????????-????????????
.onchange = text_time_h_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_ETIME_H31)//????????????-????????????-??????????????????-????????????
.onchange = text_time_h_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};

static int text_time_m_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_text *text = (struct ui_text *)_ctrl;
    u8 index_buf;
    int index;

    switch (event) {
    case ON_CHANGE_INIT:
        break;
    case ON_CHANGE_HIGHLIGHT:
        break;
    case ON_CHANGE_UPDATE_ITEM:
        index = (u32)arg;
        index = index % 60;
        log_info("tid %d\n", index);
        if ((index < 0) || (index > 59)) {
            break;
        }
        switch (text->elm.id) {
        case TEXT_STIME_M00:
        case TEXT_STIME_M10:
        case TEXT_STIME_M20:
        case TEXT_STIME_M30:
        case TEXT_ETIME_M00:
        case TEXT_ETIME_M10:
        case TEXT_ETIME_M20:
        case TEXT_ETIME_M30:
            index_buf = index / 10;
            break;
        case TEXT_STIME_M01:
        case TEXT_STIME_M11:
        case TEXT_STIME_M21:
        case TEXT_STIME_M31:
        case TEXT_ETIME_M01:
        case TEXT_ETIME_M11:
        case TEXT_ETIME_M21:
        case TEXT_ETIME_M31:
            index_buf = index % 10;
            break;
        }
        ui_text_set_index(text, index_buf);
        break;
    }
    return FALSE;
}
REGISTER_UI_EVENT_HANDLER(TEXT_STIME_M00)//????????????-????????????-??????????????????-????????????
.onchange = text_time_m_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_STIME_M10)//????????????-????????????-??????????????????-????????????
.onchange = text_time_m_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_STIME_M20)//????????????-????????????-??????????????????-????????????
.onchange = text_time_m_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_STIME_M30)//????????????-????????????-??????????????????-????????????
.onchange = text_time_m_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_ETIME_M00)//????????????-????????????-??????????????????-????????????
.onchange = text_time_m_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_ETIME_M10)//????????????-????????????-??????????????????-????????????
.onchange = text_time_m_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_ETIME_M20)//????????????-????????????-??????????????????-????????????
.onchange = text_time_m_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_ETIME_M30)//????????????-????????????-??????????????????-????????????
.onchange = text_time_m_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_STIME_M01)//????????????-????????????-??????????????????-????????????
.onchange = text_time_m_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_STIME_M11)//????????????-????????????-??????????????????-????????????
.onchange = text_time_m_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_STIME_M21)//????????????-????????????-??????????????????-????????????
.onchange = text_time_m_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_STIME_M31)//????????????-????????????-??????????????????-????????????
.onchange = text_time_m_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_ETIME_M01)//????????????-????????????-??????????????????-????????????
.onchange = text_time_m_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_ETIME_M11)//????????????-????????????-??????????????????-????????????
.onchange = text_time_m_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_ETIME_M21)//????????????-????????????-??????????????????-????????????
.onchange = text_time_m_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_ETIME_M31)//????????????-????????????-??????????????????-????????????
.onchange = text_time_m_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};

static int layout_time_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    switch (event) {
    case ON_CHANGE_INIT:
        set.make_sure = 0;
        break;
    }
    return FALSE;
}

static int layout_time_ontouch(void *ctr, struct element_touch_event *e)
{
    struct layout *layout = (struct layout *)ctr;

    switch (e->event) {
    case ELM_EVENT_TOUCH_R_MOVE:
        set.show_layout = LAYOUT_UNDISTURB_MODE;
        if (layout->elm.id == LAYOUT_UNDISTURB_MODE_STIME) {
            ui_hide(LAYOUT_UNDISTURB_MODE_STIME);
        } else {
            ui_hide(LAYOUT_UNDISTURB_MODE_ETIME);
        }
        ui_show(set.show_layout);
        break;
    case ELM_EVENT_TOUCH_UP:
        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        break;
    case ELM_EVENT_TOUCH_DOWN:
        return true;
    case ELM_EVENT_TOUCH_U_MOVE:
        break;
    case ELM_EVENT_TOUCH_D_MOVE:
        break;
    }
    return false;
}
REGISTER_UI_EVENT_HANDLER(LAYOUT_UNDISTURB_MODE_STIME)//????????????-????????????
.onchange = layout_time_onchange,
 .onkey = NULL,
  .ontouch = layout_time_ontouch,
};
REGISTER_UI_EVENT_HANDLER(LAYOUT_UNDISTURB_MODE_ETIME)//????????????-????????????
.onchange = layout_time_onchange,
 .onkey = NULL,
  .ontouch = layout_time_ontouch,
};

static int layout_undisturb_mode_ontouch(void *ctr, struct element_touch_event *e)
{
    struct layout *layout = (struct layout *)ctr;

    switch (e->event) {
    case ELM_EVENT_TOUCH_R_MOVE:
        set.show_layout = LAYOUT_UNDISTURB_MODE;
        ui_send_event(KEY_CHANGE_PAGE, BIT(31) | PAGE_SET);
        break;
    case ELM_EVENT_TOUCH_UP:
        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        break;
    case ELM_EVENT_TOUCH_DOWN:
        return true;
    case ELM_EVENT_TOUCH_U_MOVE:
        break;
    case ELM_EVENT_TOUCH_D_MOVE:
        break;
    }
    return false;
}
REGISTER_UI_EVENT_HANDLER(LAYOUT_UNDISTURB_MODE)//????????????
.onchange = NULL,
 .onkey = NULL,
  .ontouch = layout_undisturb_mode_ontouch,
};

//-----------------------------???????????????????????????-------------------------------//

static int layout_return_set_ontouch(void *ctr, struct element_touch_event *e)
{
    /* struct layout *layout = (struct layout *)ctr; */

    switch (e->event) {
    case ELM_EVENT_TOUCH_R_MOVE:
        ui_send_event(KEY_CHANGE_PAGE, BIT(31) | PAGE_SET);
        break;
    case ELM_EVENT_TOUCH_UP:
        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        break;
    case ELM_EVENT_TOUCH_DOWN:
        return true;
    case ELM_EVENT_TOUCH_U_MOVE:
        break;
    case ELM_EVENT_TOUCH_D_MOVE:
        break;
    }
    return false;
}
REGISTER_UI_EVENT_HANDLER(LAYOUT_TRAIN_SET)//????????????
.onchange = NULL,
 .onkey = NULL,
  .ontouch = layout_return_set_ontouch,
};
/* REGISTER_UI_EVENT_HANDLER(LAYOUT_USER_GUIDE)//???????????? */
/* .onchange = NULL, */
/*  .onkey = NULL, */
/*   .ontouch = layout_return_set_ontouch, */
/* }; */
REGISTER_UI_EVENT_HANDLER(LAYOUT_SYS_MENU)//????????????
.onchange = NULL,
 .onkey = NULL,
  .ontouch = layout_return_set_ontouch,
};

//--------------------------------??????????????????-------------------------------//

static int list_train_set_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_grid *grid = (struct ui_grid *)_ctrl;

    switch (event) {
    case ON_CHANGE_INIT:
        ui_grid_set_slide_direction(grid, SCROLL_DIRECTION_UD);
        break;
    }
    return 0;
}
REGISTER_UI_EVENT_HANDLER(VLIST_TRAIN_SET)//????????????-????????????
.onchange = list_train_set_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};

static int pic_train_set_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_pic *pic = (struct ui_pic *)_ctrl;

    switch (event) {
    case ON_CHANGE_INIT:
        switch (pic->elm.id) {
        case PIC_TRAIN_SET_AUTO_SEL:
            ui_pic_set_image_index(pic, !!get_ui_sys_param(TrainAutoEn));
            break;
        default:
            return false;
        }

        break;
    case ON_CHANGE_HIGHLIGHT:
        break;
    }
    return FALSE;
}

static int pic_train_set_ontouch(void *ctr, struct element_touch_event *e)
{
    struct ui_pic *pic = (struct ui_pic *)ctr;
    static u8 touch_action = 0;
    u8 train_status_sel;

    switch (e->event) {
    case ELM_EVENT_TOUCH_UP:
        if (touch_action != 1) {
            break;
        }

        switch (pic->elm.id) {
        case PIC_TRAIN_SET_AUTO_SEL:
            train_status_sel = get_ui_sys_param(TrainAutoEn);
            train_status_sel = !train_status_sel;
            ui_pic_show_image_by_id(pic->elm.id, !!train_status_sel);
            set_ui_sys_param(TrainAutoEn, train_status_sel);
            break;
        default:
            return false;
        }

        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        touch_action = 2;
        break;
    case ELM_EVENT_TOUCH_DOWN:
        touch_action = 1;
        return true;
    case ELM_EVENT_TOUCH_U_MOVE:
        break;
    case ELM_EVENT_TOUCH_D_MOVE:
        break;
    }
    return false;
}
REGISTER_UI_EVENT_HANDLER(PIC_TRAIN_SET_AUTO_SEL)//????????????-????????????-????????????
.onchange = pic_train_set_onchange,
 .onkey = NULL,
  .ontouch = pic_train_set_ontouch,
};

//--------------------------------????????????????????????-------------------------------//

static u8 set_mub = 0;
static int language_yes_ontouch(void *ctr, struct element_touch_event *e)
{
    switch (e->event) {
    case ELM_EVENT_TOUCH_DOWN:
        struct element *elm = ui_core_get_element_by_id(TEXT_YES);
        ui_core_highlight_element(elm, 1);//????????????
        ui_core_redraw(elm->parent);
        log_info(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>2");
        os_time_dly(20);
        log_info("\n\n\n>>>>>>##############>>>>language sel %d\n\n\n", set_mub);
        switch (set_mub) {
        case 0:
            ui_language_set(Chinese_Simplified);
            break;
        case 1:
            ui_language_set(Chinese_Traditional);
            break;
        case 2:
            ui_language_set(English);
            break;
        case 3:
            ui_language_set(Japanese);
            break;
        }
        ui_send_event(KEY_CHANGE_PAGE, BIT(31) | PAGE_SET);
    }
    return false;
}
REGISTER_UI_EVENT_HANDLER(TEXT_YES)//????????????????????????-???
.onchange = NULL,
 .onkey = NULL,
  .ontouch = language_yes_ontouch,
};

static int language_no_ontouch(void *ctr, struct element_touch_event *e)
{
    switch (e->event) {
    case ELM_EVENT_TOUCH_DOWN:
        struct element *elm = ui_core_get_element_by_id(TEXT_NO);
        ui_core_highlight_element(elm, 1);//????????????
        ui_core_redraw(elm->parent);
        os_time_dly(20);
        ui_hide(LANGUAGE_SEL);//??????????????????
        ui_show(LAYOUT_LANGUAGE);//??????????????????
        break;
    }
    return false;
}
REGISTER_UI_EVENT_HANDLER(TEXT_NO)//????????????????????????-???
.onchange = NULL,
 .onkey = NULL,
  .ontouch = language_no_ontouch,
};

static int language_shoice_ontouch(void *ctr, struct element_touch_event *e)
{
    switch (e->event) {
    case ELM_EVENT_TOUCH_DOWN:
        struct element *elm = ui_core_get_element_by_id(LANGUAGE_SURE);
        ui_core_highlight_element(elm, 1);//????????????
        /*ui_grid_highlight_item_by_id*/
        ui_core_redraw(elm->parent);
        os_time_dly(20);
        ui_hide(LAYOUT_LANGUAGE);//????????????????????????
        ui_show(LANGUAGE_SEL);//????????????????????????
        break;
    }
    return false;
}
REGISTER_UI_EVENT_HANDLER(LANGUAGE_SURE)//????????????-????????????
.onchange = NULL,
 .onkey = NULL,
  .ontouch = language_shoice_ontouch,
};
static int language_sel_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_grid *grid = (struct ui_grid *)_ctrl;

    switch (event) {
    case ON_CHANGE_INIT:
        log_info("\n\n\n>>>>>>>>>>language sel %d\n\n\n", set_mub);
        ui_text_show_index_by_id(LANGUAGE_TIPS, set_mub);
        break;
    }
    return 0;
}
static int language_sel_ontouch(void *ctr, struct element_touch_event *e)
{
    switch (e->event) {
    case ELM_EVENT_TOUCH_R_MOVE:
        ui_send_event(KEY_CHANGE_PAGE, BIT(31) | PAGE_SET);
        break;
    }
    return false;
}
REGISTER_UI_EVENT_HANDLER(LANGUAGE_SEL)//??????????????????
.onchange = language_sel_onchange,
 .onkey = NULL,
  .ontouch = language_sel_ontouch,
};

static int vlist_language_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    switch (event) {
    case ON_CHANGE_INIT:
        set_mub = 0;
        break;
    }
    return 0;
}

static int vlist_language_ontouch(void *ctr, struct element_touch_event *e)
{
    struct ui_grid *grid = (struct ui_grid *)ctr;
    switch (e->event) {
    case ELM_EVENT_TOUCH_UP:
        set_mub = ui_grid_get_hindex(grid); //?????????????????????
        log_info("\n\n\nlanguage sel %d\n\n\n", set_mub);
        break;
    }

    return false;
}
REGISTER_UI_EVENT_HANDLER(VLIST_LANGUAGE)//????????????-????????????
.onchange = vlist_language_onchange,
 .onkey = NULL,
  .ontouch = vlist_language_ontouch,
};

static int layout_language_ontouch(void *ctr, struct element_touch_event *e)
{
    switch (e->event) {
    case ELM_EVENT_TOUCH_R_MOVE:
        ui_send_event(KEY_CHANGE_PAGE, BIT(31) | PAGE_SET);
        break;
    case ELM_EVENT_TOUCH_UP:
        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        break;
    case ELM_EVENT_TOUCH_DOWN:
        return true;
    case ELM_EVENT_TOUCH_U_MOVE:
        break;
    case ELM_EVENT_TOUCH_D_MOVE:
        break;
    }
    return false;
}
REGISTER_UI_EVENT_HANDLER(LAYOUT_LANGUAGE)//??????????????????
.onchange = NULL,
 .onkey = NULL,
  .ontouch = layout_language_ontouch,
};

//--------------------------------??????????????????-------------------------------//

static int text_shake_level_sure_ontouch(void *ctr, struct element_touch_event *e)
{
    switch (e->event) {
    case ELM_EVENT_TOUCH_DOWN:
        struct element *elm = ui_core_get_element_by_id(TEXT_SHAKE_LEVEL_SURE);
        ui_core_highlight_element(elm, 1);//????????????
        ui_core_redraw(elm->parent);
        os_time_dly(20);
        if (set_mub == 0) {
            ui_moto_set_H_L(TCFG_MOTO_PWM_H);
        }
        if (set_mub == 1) {
            ui_moto_set_H_L(TCFG_MOTO_PWM_L);
        }
        if (set_mub == 2) {
            ui_moto_set_H_L(TCFG_MOTO_PWM_NULL);
        }
        ui_send_event(KEY_CHANGE_PAGE, BIT(31) | PAGE_SET);
        return true;
    }
    return false;
}

REGISTER_UI_EVENT_HANDLER(TEXT_SHAKE_LEVEL_SURE)//?????? ????????????
.onchange = NULL,
 .onkey = NULL,
  .ontouch = text_shake_level_sure_ontouch,
};

static int vlist_shake_level_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    switch (event) {
    case ON_CHANGE_INIT:
        set_mub = 0;
        break;
    }
    return 0;
}
static int vlist_shake_level_ontouch(void *ctr, struct element_touch_event *e)
{
    struct ui_grid *grid = (struct ui_grid *)ctr;
    switch (e->event) {
    case ELM_EVENT_TOUCH_R_MOVE:
        ui_send_event(KEY_CHANGE_PAGE, BIT(31) | PAGE_SET);
        break;
    case ELM_EVENT_TOUCH_UP:
        set_mub = ui_grid_get_hindex(grid); //?????????????????????
        log_info("\n\n\nmtoto sel %d\n\n\n", set_mub);
        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        break;
    case ELM_EVENT_TOUCH_DOWN:
        return true;
    case ELM_EVENT_TOUCH_U_MOVE:
        break;
    case ELM_EVENT_TOUCH_D_MOVE:
        break;
    }
    return false;
}

REGISTER_UI_EVENT_HANDLER(VLIST_SHAKE_LEVEL)//?????? ????????????
.onchange = vlist_shake_level_onchange,
 .onkey = NULL,
  .ontouch = vlist_shake_level_ontouch,
};

static int layer_shake_ontouch(void *ctr, struct element_touch_event *e)
{
    switch (e->event) {
    case ELM_EVENT_TOUCH_R_MOVE:
        ui_send_event(KEY_CHANGE_PAGE, BIT(31) | PAGE_SET);
        break;
    case ELM_EVENT_TOUCH_UP:
        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        break;
    case ELM_EVENT_TOUCH_DOWN:
        return true;
    case ELM_EVENT_TOUCH_U_MOVE:
        break;
    case ELM_EVENT_TOUCH_D_MOVE:
        break;
    }
    return false;
}

REGISTER_UI_EVENT_HANDLER(LAYER_SHAKE)//??????????????????
.onchange = NULL,
 .onkey = NULL,
  .ontouch = layer_shake_ontouch,
};


//-------------------------------????????????????????????-------------------------------//

static int layout_return_sys_menu_ontouch(void *ctr, struct element_touch_event *e)
{
    struct layout *layout = (struct layout *)ctr;

    switch (e->event) {
    case ELM_EVENT_TOUCH_R_MOVE:

        if (layout->elm.id == LAYOUT_TIPS_RESET) {
            if (get_ui_sys_param(ConnNewPhone)) {
                set.show_layout = LAYOUT_PAIR;
                ui_hide(layout->elm.id);
                ui_show(set.show_layout);
                break;
            }
        }

        set.show_layout = LAYOUT_SYS_MENU;
        ui_hide(layout->elm.id);
        ui_show(set.show_layout);
        break;
    case ELM_EVENT_TOUCH_UP:
        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        break;
    case ELM_EVENT_TOUCH_DOWN:
        return true;
    case ELM_EVENT_TOUCH_U_MOVE:
        break;
    case ELM_EVENT_TOUCH_D_MOVE:
        break;
    }
    return false;
}
REGISTER_UI_EVENT_HANDLER(LAYOUT_REBOOT)//??????
.onchange = NULL,
 .onkey = NULL,
  .ontouch = layout_return_sys_menu_ontouch,
};
REGISTER_UI_EVENT_HANDLER(LAYOUT_SHUT_DOWN)//??????
.onchange = NULL,
 .onkey = NULL,
  .ontouch = layout_return_sys_menu_ontouch,
};
REGISTER_UI_EVENT_HANDLER(LAYOUT_TIPS_DISCON)//??????????????????tips???
.onchange = NULL,
 .onkey = NULL,
  .ontouch = layout_return_sys_menu_ontouch,
};
REGISTER_UI_EVENT_HANDLER(LAYOUT_TIPS_RESET)//???????????????tips???
.onchange = NULL,
 .onkey = NULL,
  .ontouch = layout_return_sys_menu_ontouch,
};

//--------------------------------????????????????????????-------------------------------//

static int sys_menu_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_grid *grid = (struct ui_grid *)_ctrl;

    switch (event) {
    case ON_CHANGE_INIT:
        ui_grid_set_slide_direction(grid, SCROLL_DIRECTION_UD);
        set_ui_sys_param(ConnNewPhone, 0);
        break;
    }
    return 0;
}

static int sys_menu_ontouch(void *ctr, struct element_touch_event *e)
{
    struct ui_grid *grid = (struct ui_grid *)ctr;
    static u8 touch_action = 0;
    int sel_item;

    switch (e->event) {
    case ELM_EVENT_TOUCH_UP:
        if (touch_action != 1) {
            break;
        }

        sel_item = ui_grid_cur_item(grid);
        if (sel_item <= 0) {
            break;
        }
        switch (sel_item) {
        case 1://??????
            set.show_layout = LAYOUT_REBOOT;
            ui_hide(LAYOUT_SYS_MENU);
            ui_show(set.show_layout);
            break;
        case 2://??????
            set.show_layout = LAYOUT_SHUT_DOWN;
            ui_hide(LAYOUT_SYS_MENU);
            ui_show(set.show_layout);
            break;
        case 3://???????????????
            set.show_layout = LAYOUT_TIPS_DISCON;
            ui_hide(LAYOUT_SYS_MENU);
            ui_show(set.show_layout);
            break;
        case 4://????????????
            set.show_layout = LAYOUT_TIPS_RESET;
            ui_hide(LAYOUT_SYS_MENU);
            ui_show(set.show_layout);
            break;
        default:
            return false;
        }

        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        touch_action = 2;
        break;
    case ELM_EVENT_TOUCH_DOWN:
        touch_action = 1;
        break;
    case ELM_EVENT_TOUCH_U_MOVE:
        break;
    case ELM_EVENT_TOUCH_D_MOVE:
        break;
    }

    return false;
}
REGISTER_UI_EVENT_HANDLER(VLIST_SYS_MENU)//????????????-????????????
.onchange = sys_menu_onchange,
 .onkey = NULL,
  .ontouch = sys_menu_ontouch,
};

static int pic_reboot_and_shutdown_ontouch(void *ctr, struct element_touch_event *e)
{
    struct ui_pic *pic = (struct ui_pic *)ctr;
    static u8 touch_action = 0;

    switch (e->event) {
    case ELM_EVENT_TOUCH_UP:
        if (touch_action != 1) {
            break;
        }

        switch (pic->elm.id) {
        case PIC_REBOOT:
            watch_reboot_or_shutdown(1, 0);
            break;
        case PIC_SHUTDOWN:
            watch_reboot_or_shutdown(0, 0);
            break;
        }
        break;
    case ELM_EVENT_TOUCH_MOVE:
        touch_action = 2;
        break;
    case ELM_EVENT_TOUCH_DOWN:
        touch_action = 1;
        return true;
    }
    return false;
}
REGISTER_UI_EVENT_HANDLER(PIC_REBOOT)
.onchange = NULL,
 .onkey = NULL,
  .ontouch = pic_reboot_and_shutdown_ontouch,
};
REGISTER_UI_EVENT_HANDLER(PIC_SHUTDOWN)
.onchange = NULL,
 .onkey = NULL,
  .ontouch = pic_reboot_and_shutdown_ontouch,
};

static int pic_tips_ontouch(void *ctr, struct element_touch_event *e)
{
    struct ui_pic *pic = (struct ui_pic *)ctr;
    static u8 touch_action = 0;

    switch (e->event) {
    case ELM_EVENT_TOUCH_UP:
        if (touch_action != 1) {
            break;
        }

        switch (pic->elm.id) {
        case PIC_TIPS_DISCONNECT_PHONE_NO://???????????????
            set.show_layout = LAYOUT_SYS_MENU;
            ui_hide(LAYOUT_TIPS_DISCON);
            ui_show(set.show_layout);
            break;
        case PIC_TIPS_DISCONNECT_PHONE_YES://????????????
            set_ui_sys_param(ConnNewPhone, 1);

            set.show_layout = LAYOUT_PAIR;
            ui_hide(LAYOUT_TIPS_DISCON);
            ui_show(set.show_layout);
            break;
        case PIC_TIPS_RESET_NO://???????????????
            if (get_ui_sys_param(ConnNewPhone)) {
                set.show_layout = LAYOUT_PAIR;
                ui_hide(LAYOUT_TIPS_RESET);
                ui_show(set.show_layout);
            } else {
                set.show_layout = LAYOUT_SYS_MENU;
                ui_hide(LAYOUT_TIPS_RESET);
                ui_show(set.show_layout);
            }
            break;
        case PIC_TIPS_RESET_YES://????????????
            restore_sys_settings();
            break;
        default:
            return false;
        }
        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        touch_action = 2;
        break;
    case ELM_EVENT_TOUCH_DOWN:
        touch_action = 1;
        return true;
    case ELM_EVENT_TOUCH_U_MOVE:
        break;
    case ELM_EVENT_TOUCH_D_MOVE:
        break;
    }
    return false;
}
REGISTER_UI_EVENT_HANDLER(PIC_TIPS_DISCONNECT_PHONE_NO)//TIPS-????????????
.onchange = NULL,
 .onkey = NULL,
  .ontouch = pic_tips_ontouch,
};
REGISTER_UI_EVENT_HANDLER(PIC_TIPS_DISCONNECT_PHONE_YES)//TIPS-????????????
.onchange = NULL,
 .onkey = NULL,
  .ontouch = pic_tips_ontouch,
};
REGISTER_UI_EVENT_HANDLER(PIC_TIPS_RESET_NO)//TIPS-????????????
.onchange = NULL,
 .onkey = NULL,
  .ontouch = pic_tips_ontouch,
};
REGISTER_UI_EVENT_HANDLER(PIC_TIPS_RESET_YES)//TIPS-????????????
.onchange = NULL,
 .onkey = NULL,
  .ontouch = pic_tips_ontouch,
};

static u8 my_sel = 0;
static int list_pair_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_grid *grid = (struct ui_grid *)_ctrl;

    switch (event) {
    case ON_CHANGE_INIT:
        ui_grid_set_slide_direction(grid, SCROLL_DIRECTION_UD);
        my_sel = 0;
        break;
    }
    return 0;
}

static int list_pair_ontouch(void *ctr, struct element_touch_event *e)
{
    struct ui_grid *grid = (struct ui_grid *)ctr;
    static u8 touch_action = 0;
    int sel_item;

    switch (e->event) {
    case ELM_EVENT_TOUCH_UP:
        if (touch_action != 1) {
            break;
        }
        sel_item = ui_grid_cur_item(grid);
        if (sel_item != 2) {
            break;
        }
        log_info("my_sel %d\n", my_sel);
        switch (my_sel) {
        case 1://????????????
            /*set.show_layout = LAYOUT_LANGUAGE;*/
            /*ui_hide(LAYOUT_PAIR);*/
            /*ui_show(set.show_layout);*/
            break;
        case 2://????????????
            set.show_layout = LAYOUT_TIPS_RESET;
            ui_hide(LAYOUT_PAIR);
            ui_show(set.show_layout);
            break;
        case 3://??????
            set.show_layout = LAYOUT_PAIR;
            ui_send_event(KEY_CHANGE_PAGE, BIT(31) | PAGE_ABOUT);
            break;
        default:
            break;
        }

        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        touch_action = 2;
        break;
    case ELM_EVENT_TOUCH_DOWN:
        touch_action = 1;
        break;
    case ELM_EVENT_TOUCH_R_MOVE:
        set.show_layout = LAYOUT_SYS_MENU;
        ui_hide(LAYOUT_PAIR);
        ui_show(set.show_layout);
        break;
    case ELM_EVENT_TOUCH_U_MOVE:
        break;
    case ELM_EVENT_TOUCH_D_MOVE:
        break;
    }

    return false;
}
REGISTER_UI_EVENT_HANDLER(VLIST_PAIR)//??????-????????????
.onchange = list_pair_onchange,
 .onkey = NULL,
  .ontouch = list_pair_ontouch,
};

static int layout_pair_list_ontouch(void *ctr, struct element_touch_event *e)
{
    struct layout *layout = (struct layout *)ctr;

    switch (e->event) {
    case ELM_EVENT_TOUCH_UP:
        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        break;
    case ELM_EVENT_TOUCH_DOWN:
        switch (layout->elm.id) {
        case LAYOUT_LANGUAGE_SEL://????????????
            my_sel = 1;
            break;
        case LAYOUT_RESET_SEL://????????????
            my_sel = 2;
            break;
        case LAYOUT_ABOUT_SEL://??????
            my_sel = 3;
            break;
        default:
            break;
        }
        break;
    case ELM_EVENT_TOUCH_U_MOVE:
        break;
    case ELM_EVENT_TOUCH_D_MOVE:
        break;
    }

    return false;
}
REGISTER_UI_EVENT_HANDLER(LAYOUT_LANGUAGE_SEL)//??????-????????????-????????????
.onchange = NULL,
 .onkey = NULL,
  .ontouch = layout_pair_list_ontouch,
};
REGISTER_UI_EVENT_HANDLER(LAYOUT_RESET_SEL)//??????-????????????-????????????
.onchange = NULL,
 .onkey = NULL,
  .ontouch = layout_pair_list_ontouch,
};
REGISTER_UI_EVENT_HANDLER(LAYOUT_ABOUT_SEL)//??????-????????????-????????????
.onchange = NULL,
 .onkey = NULL,
  .ontouch = layout_pair_list_ontouch,
};

#if 0//
static int list_language_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_grid *grid = (struct ui_grid *)_ctrl;
    int row, col;

    switch (event) {
    case ON_CHANGE_INIT:
        row = LANGUAGE_NUM;
        col = 1;
        ui_grid_init_dynamic(grid, &row, &col);
        log_info("dynamic_grid %d X %d\n", row, col);

        ui_grid_set_slide_direction(grid, SCROLL_DIRECTION_UD);
        break;
    case ON_CHANGE_RELEASE:
        break;
    }
    return 0;
}

static int list_language_ontouch(void *ctr, struct element_touch_event *e)
{
    struct ui_grid *grid = (struct ui_grid *)ctr;
    static u8 touch_action = 0;
    int index;

    switch (e->event) {
    case ELM_EVENT_TOUCH_UP:

        if (touch_action != 1) {
            break;
        }

        index = ui_grid_cur_item_dynamic(grid);
        log_info("\n\n\nlanguage sel %d\n\n\n", index);
        if (index >= LANGUAGE_NUM) {
            break;
        }

        set.show_layout = LAYOUT_PAIR;
        ui_hide(LAYOUT_LANGUAGE);
        ui_show(set.show_layout);
        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        touch_action = 2;
        break;
    case ELM_EVENT_TOUCH_DOWN:
        touch_action = 1;
        return true;
    case ELM_EVENT_TOUCH_U_MOVE:
        break;
    case ELM_EVENT_TOUCH_D_MOVE:
        break;
    case ELM_EVENT_TOUCH_R_MOVE:
        set.show_layout = LAYOUT_PAIR;
        ui_hide(LAYOUT_LANGUAGE);
        ui_show(set.show_layout);
        break;
    }

    return false;
}
REGISTER_UI_EVENT_HANDLER(VLIST_DY_LANGUAGE)//??????-????????????-????????????-????????????-????????????
.onchange = list_language_onchange,
 .onkey = NULL,
  .ontouch = list_language_ontouch,
};

static int list_text_language_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_text *text = (struct ui_text *)_ctrl;
    /* u8 index_buf[1]; */
    /* static u16  store_buf[4][2]; */
    /* u8 *store_buf_text; */
    int index;

    switch (event) {
    case ON_CHANGE_INIT:
        break;
    case ON_CHANGE_HIGHLIGHT:
        break;
    case ON_CHANGE_UPDATE_ITEM:

        index = (u32)arg;
        if (index >= LANGUAGE_NUM) {
            break;
        }
        switch (text->elm.id) {
        case TEXT_LANGUAGE0:
            /* store_buf_text = store_buf[0]; */
            break;
        case TEXT_LANGUAGE1:
            /* store_buf_text = store_buf[1]; */
            break;
        case TEXT_LANGUAGE2:
            /* store_buf_text = store_buf[2]; */
            break;
        case TEXT_LANGUAGE3:
            /* store_buf_text = store_buf[3]; */
            break;
        default:
            return FALSE;
        }
        /* log_info("language_index %d\n", index); */

        ui_text_set_index(text, index);
        /* index_buf[0] = index; */
        /* ui_text_set_combine_index(text, store_buf_text, index_buf, 1); */
        break;
    }
    return FALSE;
}
REGISTER_UI_EVENT_HANDLER(TEXT_LANGUAGE0)//????????????-????????????-????????????
.onchange = list_text_language_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_LANGUAGE1)//????????????-????????????-????????????
.onchange = list_text_language_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_LANGUAGE2)//????????????-????????????-????????????
.onchange = list_text_language_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_LANGUAGE3)//????????????-????????????-????????????
.onchange = list_text_language_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
#endif


//--------------------------------????????????-------------------------------//

struct SYS_ABOUT_MESSAGE {
    char *dev_name;
    char *dev_num;
    char mac_addr[18];
    char *version;
    char *serial_num;
};

struct SYS_ABOUT_MESSAGE sys_about_message = {
    .dev_num = "AC695N",
    .version = "master",
    .serial_num = "ABC1234567890",
};
extern void bt_addr2string(u8 *addr, u8 *buf);
static int text_about_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_text *text = (struct ui_text *)_ctrl;

    switch (event) {
    case ON_CHANGE_INIT:
        switch (text->elm.id) {
        case TEXT_DEV_NAME:
            sys_about_message.dev_name = bt_get_local_name();
            ui_text_set_text_attrs(text, sys_about_message.dev_name, strlen(sys_about_message.dev_name), FONT_ENCODE_ANSI, 0, FONT_DEFAULT);
            break;
        case TEXT_DEV_NUM:
            ui_text_set_text_attrs(text, sys_about_message.dev_num, strlen(sys_about_message.dev_num), FONT_ENCODE_ANSI, 0, FONT_DEFAULT);
            break;
        case TEXT_MAC_ADDR:
            bt_addr2string(bt_get_mac_addr(), sys_about_message.mac_addr);
            ui_text_set_text_attrs(text, sys_about_message.mac_addr, strlen(sys_about_message.mac_addr), FONT_ENCODE_ANSI, 0, FONT_DEFAULT);
            break;
        case TEXT_VERSION_NUM:
            ui_text_set_text_attrs(text, sys_about_message.version, strlen(sys_about_message.version), FONT_ENCODE_ANSI, 0, FONT_DEFAULT);
            break;
        case TEXT_SERIAL_NUM:
            ui_text_set_text_attrs(text, sys_about_message.serial_num, strlen(sys_about_message.serial_num), FONT_ENCODE_ANSI, 0, FONT_DEFAULT);
            break;
        }
        break;
    case ON_CHANGE_RELEASE:
        break;
    }
    return 0;
}
REGISTER_UI_EVENT_HANDLER(TEXT_DEV_NAME)
.onchange = text_about_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_DEV_NUM)
.onchange = text_about_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_MAC_ADDR)
.onchange = text_about_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_VERSION_NUM)
.onchange = text_about_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
REGISTER_UI_EVENT_HANDLER(TEXT_SERIAL_NUM)
.onchange = text_about_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};

static int list_about_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct ui_grid *grid = (struct ui_grid *)_ctrl;

    switch (event) {
    case ON_CHANGE_INIT:
        ui_grid_set_slide_direction(grid, SCROLL_DIRECTION_UD);
        break;
    case ON_CHANGE_RELEASE:
        break;
    }
    return 0;
}
REGISTER_UI_EVENT_HANDLER(VLIST_ABOUT)//??????-????????????
.onchange = list_about_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};
static int layout_about_ontouch(void *ctr, struct element_touch_event *e)
{
    /* struct layout *layout = (struct layout *)ctr; */

    switch (e->event) {
    case ELM_EVENT_TOUCH_R_MOVE:
        if (get_ui_sys_param(ConnNewPhone)) {
            set.show_layout = LAYOUT_PAIR;
            ui_send_event(KEY_CHANGE_PAGE, BIT(31) | PAGE_SYS_MENU);
        } else {
            ui_send_event(KEY_CHANGE_PAGE, BIT(31) | PAGE_SET);
        }

        break;
    case ELM_EVENT_TOUCH_UP:
        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        break;
    case ELM_EVENT_TOUCH_DOWN:
        break;
    case ELM_EVENT_TOUCH_U_MOVE:
        break;
    case ELM_EVENT_TOUCH_D_MOVE:
        break;
    }

    return false;
}
REGISTER_UI_EVENT_HANDLER(LAYOUT_ABOUT)//??????
.onchange = NULL,
 .onkey = NULL,
  .ontouch = layout_about_ontouch,
};

static int layout_menu_style_ontouch(void *ctr, struct element_touch_event *e)
{
    switch (e->event) {
    case ELM_EVENT_TOUCH_R_MOVE:
        ui_send_event(KEY_CHANGE_PAGE, BIT(31) | PAGE_SET);
        break;
    }
    return false;
}
REGISTER_UI_EVENT_HANDLER(LAYOUT_MENU_STYLE)
.onchange = NULL,
 .onkey = NULL,
  .ontouch = layout_menu_style_ontouch,
};

static int text_menu_style_ontouch(void *ctr, struct element_touch_event *e)
{
    struct ui_text *text = (struct ui_text *)ctr;
    static u8 touch_action = 0;

    switch (e->event) {
    case ELM_EVENT_TOUCH_UP:
        if (touch_action != 1) {
            break;
        }

        switch (text->elm.id) {
        case TEXT_MENU_STYLE1:
            set_ui_sys_param(MenuStyle, 0);
            break;
        case TEXT_MENU_STYLE2:
            set_ui_sys_param(MenuStyle, 1);
            break;
        case TEXT_MENU_STYLE3:
            set_ui_sys_param(MenuStyle, 2);
            break;
        }
        ui_send_event(KEY_CHANGE_PAGE, BIT(31) | PAGE_SET);
        break;
    case ELM_EVENT_TOUCH_DOWN:
        touch_action = 1;
        return true;
    case ELM_EVENT_TOUCH_MOVE:
        touch_action = 2;
        break;
    }

    return false;
}
REGISTER_UI_EVENT_HANDLER(TEXT_MENU_STYLE1)
.onchange = NULL,
 .onkey = NULL,
  .ontouch = text_menu_style_ontouch,
};
REGISTER_UI_EVENT_HANDLER(TEXT_MENU_STYLE2)
.onchange = NULL,
 .onkey = NULL,
  .ontouch = text_menu_style_ontouch,
};
REGISTER_UI_EVENT_HANDLER(TEXT_MENU_STYLE3)
.onchange = NULL,
 .onkey = NULL,
  .ontouch = text_menu_style_ontouch,
};

#endif /* #if (!TCFG_LUA_ENABLE) */
#else
void ui_sysinfo_init()
{

}
u8 get_all_day_undisturb_sel()
{
    return 0;
}
u8 get_light_time_sel()
{
    return 0;
}
void set_all_day_undisturb_sel(u8 sel)
{

}
#endif /* #if TCFG_UI_ENABLE_SYS_SET */
#endif /* #ifdef CONFIG_UI_STYLE_JL_ENABLE */




