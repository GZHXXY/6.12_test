#include "app_config.h"
#include "ui/ui_style.h"
#include "ui/ui.h"
#include "ui/ui_api.h"
#include "app_task.h"
#include "tone_player.h"
#include "system/timer.h"
#include "device/device.h"
#include "key_event_deal.h"
#include "res/resfile.h"
#include "ui/res_config.h"


#if TCFG_UI_ENABLE && (!TCFG_LUA_ENABLE)
#if TCFG_UI_ENABLE_TIMER_ACTION

#define STYLE_NAME  JL


#define LOG_TAG_CONST       UI
#define LOG_TAG     		"[UI-ACTION]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"


/* REGISTER_UI_STYLE(STYLE_NAME) */
extern void ui_send_event(u16 event, u32 val);


#define CALCULAGRAPH_SEL        PAGE_34//
#define CALCULAGRAPH_FRONT      PAGE_35//
#define CALCULAGRAPH_ADD        PAGE_36//

struct ui_timer {
    u8 countdown_hour;          //用来显示小时
    u8 countdown_min;
    u8 countdown_sec ;
    u8 countdown_flag;
    u8 countdown_customize_start;     //自定义设置好了时间，直接开始倒计时
    u16 contdown_show_num_id;         //倒计时时间刷新定时器id
    u16 countdown_count_num_id;       //计数定时器刷新id
    u16 countdown_time_id;            //页面上面刷新显示时间的定时器id
    u32 timer_countdown_num;          // 倒计时计数
    u8 countdown_dis_buf[20];         //储存显示数据的buf
};

#define     MALLOC_SIZE    (sizeof(struct ui_timer))//64
struct ui_timer *__timer_countdown = NULL;

/***************************************************************************************
                                LAQ PAGE20 页面处理部分
****************************************************************************************/
static int TIMER_CALCULAGRAPH_onchange(void *ctrl,  enum element_change_event e, void *arg)
{
    struct window *win = (struct window *)ctrl;
    switch (e) {
    case ON_CHANGE_INIT:
        if (__timer_countdown == NULL) {
            log_info("malloc addr to __timer_countdown and size is %d", sizeof(struct ui_timer));
            __timer_countdown = (struct ui_timer *) malloc(MALLOC_SIZE);
        }
        memset(__timer_countdown, 0, MALLOC_SIZE);
        ui_auto_shut_down_disable();
        break;
    case ON_CHANGE_RELEASE:
        ui_auto_shut_down_enable();
        if (__timer_countdown->countdown_count_num_id) {
            usr_timer_del(__timer_countdown->countdown_count_num_id);
            __timer_countdown->countdown_count_num_id = 0;
        }
        break;
    default:
        break;
    }
    return false;
}

static int TIMER_CALCULAGRAPH_ontouch(void *ctrl, struct element_touch_event *e)
{
    struct window *win = (struct window *) ctrl;
    switch (e->event) {
    case ELM_EVENT_TOUCH_L_MOVE:
    case ELM_EVENT_TOUCH_R_MOVE:
        ui_send_event(KEY_CHANGE_PAGE, BIT(31) | PAGE_7);
        break;

    default:
        break;
    }
    return false;
}

REGISTER_UI_EVENT_HANDLER(CALCULAGRAPH_SEL)
.ontouch = TIMER_CALCULAGRAPH_ontouch,
 .onkey = NULL,
  .onchange = TIMER_CALCULAGRAPH_onchange,
};

static void child_id_to_change(struct element *elm)
{
    static u8 last_return_vaule = 0;
    switch (elm->id) {
    case TIMER_PIC_1_MIN_NUM:
    case TIMER_PIC_1_MIN_TEXT:
    case TIMER_PIC_1_MIN:
        __timer_countdown->timer_countdown_num = 60;
        ui_send_event(KEY_CHANGE_PAGE, BIT(31) | CALCULAGRAPH_FRONT);
        break;
    case TIMER_PIC_3_MIN_NUM:
    case TIMER_PIC_3_MIN_TEXT:
    case TIMER_PIC_3_MIN:
        __timer_countdown->timer_countdown_num = 180;
        ui_send_event(KEY_CHANGE_PAGE, BIT(31) | CALCULAGRAPH_FRONT);
        break;
    case TIMER_PIC_5_MIN_NUM:
    case TIMER_PIC_5_MIN_TEXT:
    case TIMER_PIC_5_MIN:
        __timer_countdown->timer_countdown_num = 300;
        ui_send_event(KEY_CHANGE_PAGE, BIT(31) | CALCULAGRAPH_FRONT);
        break;
    case TIMER_PIC_10_MIN_NUM:
    case TIMER_PIC_10_MIN_TEXT:
    case TIMER_PIC_10_MIN:
        __timer_countdown->timer_countdown_num = 600;
        ui_send_event(KEY_CHANGE_PAGE, BIT(31) | CALCULAGRAPH_FRONT);
        break;
    case TIMER_PIC_15_MIN_NUM:
    case TIMER_PIC_15_MIN_TEXT:
    case TIMER_PIC_15_MIN:
        __timer_countdown->timer_countdown_num = 900;
        ui_send_event(KEY_CHANGE_PAGE, BIT(31) | CALCULAGRAPH_FRONT);
        break;
    case TIMER_PIC_30_MIN_NUM:
    case TIMER_PIC_30_MIN_TEXT:
    case TIMER_PIC_30_MIN:
        __timer_countdown->timer_countdown_num = 1800;
        ui_send_event(KEY_CHANGE_PAGE, BIT(31) | CALCULAGRAPH_FRONT);
        break;
    case TIMER_PIC_1_HOUR_NUM:
    case TIMER_PIC_1_HOUR_TEXT:
    case TIMER_PIC_1_HOUR:
        __timer_countdown->timer_countdown_num = 3600;
        ui_send_event(KEY_CHANGE_PAGE, BIT(31) | CALCULAGRAPH_FRONT);
        break;
    case TIMER_PIC_2_HOUR_NUM:
    case TIMER_PIC_2_HOUR_TEXT:
    case TIMER_PIC_2_HOUR:
        __timer_countdown->timer_countdown_num = 7200;
        ui_send_event(KEY_CHANGE_PAGE, BIT(31) | CALCULAGRAPH_FRONT);
        break;
    case TIMER_ADD_BUTTN_TEXT:
    /* case TIMER_ADD_BUTTN: */
    case BASEFORM_750:
        ui_send_event(KEY_CHANGE_PAGE, BIT(31) | CALCULAGRAPH_ADD);
        break;
    default:
        break;
    }
}

static int vlist_child_ontouch(void *ctrl, struct  element_touch_event *e)
{
    struct element *grid = (struct element *) ctrl;
    static change_page_action = 0;
    log_info("LAQtext the id is 0x%x", grid->id);
    log_info("LAQ event is %d", e->event);
    switch (e->event) {
    case ELM_EVENT_TOUCH_UP:
        if (change_page_action == 1) {
            child_id_to_change(grid);
            change_page_action = 0;
        }
        break;
    case ELM_EVENT_TOUCH_MOVE:
    case ELM_EVENT_TOUCH_R_MOVE:
    case ELM_EVENT_TOUCH_L_MOVE:
    case ELM_EVENT_TOUCH_U_MOVE:
    case ELM_EVENT_TOUCH_D_MOVE:
    case ELM_EVENT_TOUCH_HOLD:
        change_page_action = 2;
        break;
    case ELM_EVENT_TOUCH_DOWN:
        change_page_action = 1;
        break;
    default:
        break;
    }
    return false;
}

static int VLIST_onchange(void *ctrl, enum element_change_event e, void *arg)
{
    struct ui_grid *grid = (struct ui_grid *) ctrl;
    switch (e) {
    case ON_CHANGE_INIT:
        ui_grid_set_slide_direction(grid, SCROLL_DIRECTION_UD);
        ui_set_default_handler(vlist_child_ontouch, NULL, NULL);
        break;
    case ON_CHANGE_RELEASE:
        break;
    default:
        break;
    }
    return false;
}

static int VLIST_ontouch(void *ctrl, struct  element_touch_event *e)
{
    struct ui_grid *grid = (struct ui_grid *) ctrl;
    struct rect *rect;
    int i;
    int sel_item = ui_grid_cur_item(grid);
    static u8 move_flag = 0;
    switch (e->event) {
    case ELM_EVENT_TOUCH_DOWN:
        move_flag = 0;
        break;
    case ELM_EVENT_TOUCH_MOVE:
        move_flag = 1;
        break;
    case ELM_EVENT_TOUCH_UP:
        if (move_flag) {
            move_flag = 0;
            break;
        }
        struct rect r;
        struct element *p, *n;
        struct element *elm = &grid->item[sel_item].elm;
        list_for_each_child_element_reverse(p, n, elm) {
            if (!p->css.invisible) {
                ui_core_get_element_abs_rect(p, &r);
                if (in_rect(&r, &e->pos)) {
                    p->handler->ontouch(p, e);
                    break;
                }
            }
        }
    }
    return false;
}

REGISTER_UI_EVENT_HANDLER(VLIST)
.ontouch = VLIST_ontouch,
 .onkey = NULL,
  .onchange = VLIST_onchange,
};

/***************************************************************************************
                                LAQ PAGE21 页面处理部分
****************************************************************************************/
static int __timer_countdown_tone_play(int param)
{
    if (!__timer_countdown || !__timer_countdown->countdown_count_num_id) {
        return 0;
    }
    tone_play_by_path(tone_table[IDEX_TONE_NORMAL], 0);
    return 0;
}
static void timer_countdown_func(void *priv)
{
    if (!__timer_countdown || !__timer_countdown->countdown_count_num_id) {
        return ;
    }
    log_info("timer_countdown_num = %d", __timer_countdown->timer_countdown_num);
    if (__timer_countdown->timer_countdown_num > 0) {
        __timer_countdown->timer_countdown_num--;
    } else if (__timer_countdown->timer_countdown_num <= 0) {
        __timer_countdown->timer_countdown_num = 0;
        {
            int argv[3];
            argv[0] = (int)__timer_countdown_tone_play;
            argv[1] = 1;
            argv[2] = 0;
            os_taskq_post_type("app_core", Q_CALLBACK, 3, argv);
        }
    }
}

static int TIMER_CALCULAGRAPH_FRONT_onchange(void *ctrl,  enum element_change_event e, void *arg)
{
    struct window *win = (struct window *)ctrl;
    switch (e) {
    case ON_CHANGE_INIT:
        if (__timer_countdown->countdown_customize_start) {
            if (__timer_countdown->countdown_flag == 0) {
                if (!(__timer_countdown->countdown_count_num_id)) {
                    __timer_countdown->countdown_count_num_id = usr_timer_add(NULL, timer_countdown_func, 1000, 1);
                    __timer_countdown->countdown_flag = 1;
                }
            }
        }
        ui_auto_shut_down_disable();
        break;
    case ON_CHANGE_RELEASE:
        ui_auto_shut_down_enable();
        if (__timer_countdown->countdown_count_num_id) {
            usr_timer_del(__timer_countdown->countdown_count_num_id);
            __timer_countdown->countdown_count_num_id = 0;
            __timer_countdown->countdown_flag = 0;
        }
        if (__timer_countdown->countdown_customize_start) {
            __timer_countdown->countdown_customize_start = 0;
            __timer_countdown->countdown_flag = 0;
        }
        break;
    default:
        break;
    }
    return false;
}
REGISTER_UI_EVENT_HANDLER(CALCULAGRAPH_FRONT)
.ontouch = NULL,
 .onkey = NULL,
  .onchange = TIMER_CALCULAGRAPH_FRONT_onchange,
};
static void timer_countdown_display_string(void *priv)
{
    if (__timer_countdown->contdown_show_num_id == 0) {
        return ;
    }
    struct unumber countdown_buf;
    if (__timer_countdown->timer_countdown_num >= 3600) {
        __timer_countdown->countdown_hour = __timer_countdown->timer_countdown_num / 3600;
        __timer_countdown->countdown_min = (__timer_countdown->timer_countdown_num % 3600) / 60;
        __timer_countdown->countdown_sec = ((__timer_countdown->timer_countdown_num % 3600) % 60);
        /* countdown_buf.type = TYPE_STRING; */
        countdown_buf.type = TYPE_NUM;
        countdown_buf.numbs = 1;
        countdown_buf.number[0] = __timer_countdown->countdown_sec;
        ui_number_update_by_id(BIG_S, &countdown_buf);
        countdown_buf.number[0] = __timer_countdown->countdown_min;
        ui_number_update_by_id(BIG_M, &countdown_buf);
        countdown_buf.number[0] = __timer_countdown->countdown_hour;
        ui_number_update_by_id(BIG_H, &countdown_buf);
    } else {
        ui_hide(BIG_ONE);
        ui_show(SMALL_ONE);
        __timer_countdown->countdown_min = __timer_countdown->timer_countdown_num / 60;
        __timer_countdown->countdown_sec = __timer_countdown->timer_countdown_num % 60;
        countdown_buf.type = TYPE_NUM;
        countdown_buf.numbs = 1;
        countdown_buf.number[0] = __timer_countdown->countdown_sec;
        ui_number_update_by_id(SMALL_S, &countdown_buf);
        countdown_buf.number[0] = __timer_countdown->countdown_min;
        ui_number_update_by_id(SMALL_M, &countdown_buf);
    }
    /* countdown_buf.num_str = &(__timer_countdown->countdown_dis_buf); */
    /* ui_number_update_by_id(TIMER_COUNTDOWM, &countdown_buf); */
}

#define ui_number_for_id(id) \
    ({ \
        struct element *elm = ui_core_get_element_by_id(id); \
        elm ? (struct ui_number *)elm: NULL; \
     })
static int TIMER_COUNTDOWM_onchange(void *ctrl, enum element_change_event e, void *arge)
{
    struct unumber countdown;
    struct ui_number *num = (struct ui_number *) ctrl;
    switch (e) {
    case ON_CHANGE_INIT:
        if (__timer_countdown->timer_countdown_num >= 3600) {
            __timer_countdown->countdown_hour = __timer_countdown->timer_countdown_num / 3600;
            __timer_countdown->countdown_min = (__timer_countdown->timer_countdown_num % 3600) / 60;
            __timer_countdown->countdown_sec = ((__timer_countdown->timer_countdown_num % 3600) % 60);
            countdown.type = TYPE_NUM;
            countdown.numbs = 1;
            countdown.number[0] = __timer_countdown->countdown_sec;
            ui_number_update(ui_number_for_id(BIG_S), &countdown);
            countdown.number[0] = __timer_countdown->countdown_min;
            ui_number_update(ui_number_for_id(BIG_M), &countdown);
            countdown.number[0] = __timer_countdown->countdown_hour;
            ui_number_update(ui_number_for_id(BIG_H), &countdown);
        } else {
            struct layout *layout = layout_for_id(BIG_ONE);  //默认隐藏
            layout->elm.css.invisible = 1;
            __timer_countdown->countdown_min = __timer_countdown->timer_countdown_num / 60;
            __timer_countdown->countdown_sec = __timer_countdown->timer_countdown_num % 60;
            countdown.type = TYPE_NUM;
            countdown.numbs = 1;
            countdown.number[0] = __timer_countdown->countdown_sec;
            ui_number_update(ui_number_for_id(SMALL_S), &countdown);
            countdown.number[0] = __timer_countdown->countdown_min;
            ui_number_update(ui_number_for_id(SMALL_M), &countdown);
        }
        if (__timer_countdown->contdown_show_num_id == 0) {
            log_info("creat the sys to display countdown");
            log_info("__timer_countdown->timer_countdown_num is %d", __timer_countdown->timer_countdown_num);
            /* if (!strcmp(num->source, "dis_str")) { */
            __timer_countdown->contdown_show_num_id = sys_timer_add(NULL, timer_countdown_display_string, 80);
            /* } */

        }
        ui_auto_shut_down_disable();
        break;
    case ON_CHANGE_RELEASE:
        if (__timer_countdown->contdown_show_num_id) {
            sys_timer_del(__timer_countdown->contdown_show_num_id);
            __timer_countdown->contdown_show_num_id = 0;
        }
        ui_auto_shut_down_enable();
        break;
    default:
        break;
    }
    return false;
}

REGISTER_UI_EVENT_HANDLER(BASEFORM_376)
.ontouch = NULL,
 .onkey = NULL,
  .onchange = TIMER_COUNTDOWM_onchange,
};

static int TIMER_COUNTDOWM_SP_BUT_ontouch(void *ctrl, struct element_touch_event *e)
{
    struct button *bton = (struct button *) ctrl;
    switch (e->event) {
    case ELM_EVENT_TOUCH_DOWN:
        log_info("LAQ test __timer_countdown->countdown_flag = %d", __timer_countdown->countdown_flag);
        /* printf("flag=%d id=%d",__timer_countdown->countdown_flag,__timer_countdown->countdown_count_num_id); */
        if (bton->elm.id == TIMER_COUNTDOWM_SP_BUT) {
            if (__timer_countdown->countdown_flag == 0) {
                ui_pic_show_image_by_id(TIMER_COUNTDOWM_SP, 1);
                if (!(__timer_countdown->countdown_count_num_id)) {
                    __timer_countdown->countdown_count_num_id = usr_timer_add(NULL, timer_countdown_func, 1000, 1);
                    __timer_countdown->countdown_flag = 1;
                }
            } else if (__timer_countdown->countdown_flag == 1) {
                if (__timer_countdown->countdown_count_num_id) {
                    ui_pic_show_image_by_id(TIMER_COUNTDOWM_SP, 0);
                    usr_timer_del(__timer_countdown->countdown_count_num_id);
                    __timer_countdown->countdown_count_num_id = 0;
                    __timer_countdown->countdown_flag = 0;
                }
            }
        } else if (bton->elm.id == TIMER_COUNTDOWM_CANCLE) {
            if (__timer_countdown->countdown_count_num_id) {
                usr_timer_del(__timer_countdown->countdown_count_num_id);
                __timer_countdown->countdown_flag = 0;
                __timer_countdown->countdown_count_num_id = 0;
            }
            ui_send_event(KEY_CHANGE_PAGE, BIT(31) | CALCULAGRAPH_SEL);
        }
        return true;
    default:
        break;
    }
    return false;
}

REGISTER_UI_EVENT_HANDLER(TIMER_COUNTDOWM_SP_BUT)
.ontouch = TIMER_COUNTDOWM_SP_BUT_ontouch,
 .onkey = NULL,
  .onchange = NULL,
};

REGISTER_UI_EVENT_HANDLER(TIMER_COUNTDOWM_CANCLE)
.ontouch = TIMER_COUNTDOWM_SP_BUT_ontouch,
 .onkey = NULL,
  .onchange = NULL,
};


static int TIMER_COUNTDOWM_SP_onchange(void *ctrl, enum element_change_event e, void *arge)
{
    struct ui_pic *pic = (struct ui_pic *)ctrl;
    switch (e) {
    case ON_CHANGE_INIT:
        if (__timer_countdown->countdown_customize_start) {
            log_info("display pic 1");
            ui_pic_set_image_index(pic, 1);
        }
        break;
    case ON_CHANGE_RELEASE:
        break;
    default:
        break;
    }
    return false;
}
REGISTER_UI_EVENT_HANDLER(TIMER_COUNTDOWM_SP)
.ontouch = NULL,
 .onkey = NULL,
  .onchange = TIMER_COUNTDOWM_SP_onchange,
};

/***************************************************************************************
                                LAQ PAGE23 页面处理部分
****************************************************************************************/

static int TIMER_CALCULAGRAPH_ADD_onchange(void *ctrl,  enum element_change_event e, void *arg)
{
    struct window *win = (struct window *)ctrl;
    switch (e) {
    case ON_CHANGE_INIT:
        __timer_countdown->timer_countdown_num = 0;
        __timer_countdown->countdown_hour = 0;
        __timer_countdown->countdown_min = 0;
        __timer_countdown->countdown_sec = 0;
        ui_auto_shut_down_disable();
        break;
    case ON_CHANGE_RELEASE:
        // if(__timer_countdown->countdown_num_id){
        //     sys_timer_del(__timer_countdown->countdown_num_id);
        //     __timer_countdown->countdown_num_id =0;
        // }
        ui_auto_shut_down_enable();
        break;
    default:
        break;
    }
    return false;
}
REGISTER_UI_EVENT_HANDLER(CALCULAGRAPH_ADD)
.ontouch = NULL,
 .onkey = NULL,
  .onchange = TIMER_CALCULAGRAPH_ADD_onchange,
};

static void reflash_set_num(void *priv)
{
    if ((__timer_countdown->contdown_show_num_id) == 0) {
        return ;
    }
    struct unumber set_num;
    /* memset(__timer_countdown->countdown_dis_buf, 0, sizeof(__timer_countdown->countdown_dis_buf)); */
    /* set_num.type = TYPE_STRING; */
    /* sprintf(__timer_countdown->countdown_dis_buf, "%02d%c%02d%c%02d", __timer_countdown->countdown_hour, ':', __timer_countdown->countdown_min, ':', __timer_countdown->countdown_sec); */
    /* set_num.num_str = &(__timer_countdown->countdown_dis_buf); */
    /* ui_number_update_by_id(TIMER_SET_NUM_, &set_num); */
    log_info("reflash_set_num\n");
    set_num.type = TYPE_NUM;
    set_num.numbs = 1;
    set_num.number[0] = __timer_countdown->countdown_sec;
    ui_number_update_by_id(TIMER_S, &set_num);
    set_num.number[0] = __timer_countdown->countdown_min;
    ui_number_update_by_id(TIMER_M, &set_num);
    set_num.number[0] = __timer_countdown->countdown_hour;
    ui_number_update_by_id(TIMER_H, &set_num);
}

static int TIMER_SET_NUM_onchange(void *ctrl, enum element_change_event e, void *arge)
{
    struct ui_number *numbr = (struct ui_number *) ctrl;

    switch (e) {
    case ON_CHANGE_INIT:
        if ((__timer_countdown->contdown_show_num_id) == 0) {
            __timer_countdown->contdown_show_num_id = sys_timer_add(NULL, reflash_set_num, 100);
        }
        break;
    case ON_CHANGE_RELEASE:
        if (__timer_countdown->contdown_show_num_id) {
            sys_timer_del(__timer_countdown->contdown_show_num_id);
            __timer_countdown->contdown_show_num_id = 0;
        }
        break;
    default:
        break;
    }
    return false;
}

/*
 * 设置时间操作部分 *
*/

static void timer_set_func(int id)
{
    switch (id) {
    case TIMER_HOUR_ADD:
        if (__timer_countdown->countdown_hour < 23) {
            __timer_countdown->countdown_hour ++ ;
        } else if (__timer_countdown->countdown_hour == 23) {
            __timer_countdown->countdown_hour = 0;
        }
        break;
    case TIMER_MIN_ADD:
        if (__timer_countdown->countdown_min < 59) {
            __timer_countdown->countdown_min ++ ;
        } else if (__timer_countdown->countdown_min == 59) {
            __timer_countdown->countdown_min = 0;
        }
        break;
    case TIMER_SEC_ADD:
        if (__timer_countdown->countdown_sec < 59) {
            __timer_countdown->countdown_sec  ++ ;
        } else if (__timer_countdown->countdown_sec  == 59) {
            __timer_countdown->countdown_sec  = 0;
        }
        break;
    case TIMER_HOUR_CUT:
        if (__timer_countdown->countdown_hour > 0) {
            __timer_countdown->countdown_hour -- ;
        } else if (__timer_countdown->countdown_hour == 0) {
            __timer_countdown->countdown_hour = 23;
        }
        break;
    case TIMER_MIN_CUT:
        if (__timer_countdown->countdown_min > 0) {
            __timer_countdown->countdown_min -- ;
        } else if (__timer_countdown->countdown_min == 0) {
            __timer_countdown->countdown_min = 59;
        }
        break;
    case TIMER_SEC_CUT:
        if (__timer_countdown->countdown_sec > 0) {
            __timer_countdown->countdown_sec  -- ;
        } else if (__timer_countdown->countdown_sec  == 0) {
            __timer_countdown->countdown_sec  = 59;
        }
        break;
    /* case TIMER_SET_BUTT: */
    case BASEFORM_751:
        __timer_countdown->timer_countdown_num = 0;
        __timer_countdown->timer_countdown_num = (__timer_countdown->countdown_hour * 3600 + __timer_countdown->countdown_min * 60 + __timer_countdown->countdown_sec);
        log_info("timer_countdown num is %d", __timer_countdown->countdown_hour);
        if (__timer_countdown->timer_countdown_num) {
            //全部为0的时候，按键无效
            __timer_countdown->countdown_customize_start = 0;
            ui_send_event(KEY_CHANGE_PAGE, BIT(31) | CALCULAGRAPH_FRONT);
        }
        break;
    default:
        break;
    }
}

static int TIMER_SET_BUT_ontouch(void *ctrl, struct element_touch_event *e)
{
    struct button *btn = (struct button *) ctrl;
    switch (e->event) {
    case ELM_EVENT_TOUCH_DOWN:
        log_info("btn->elm.id is 0x%x", btn->elm.id);
        timer_set_func(btn->elm.id);
        break;
    default:
        break;
    }
    return false;
}

REGISTER_UI_EVENT_HANDLER(TIMER_HOUR_ADD)
.ontouch = TIMER_SET_BUT_ontouch,
 .onkey = NULL,
  .onchange = NULL,
};

REGISTER_UI_EVENT_HANDLER(TIMER_MIN_ADD)
.ontouch = TIMER_SET_BUT_ontouch,
 .onkey = NULL,
  .onchange = NULL,
};

REGISTER_UI_EVENT_HANDLER(TIMER_SEC_ADD)
.ontouch = TIMER_SET_BUT_ontouch,
 .onkey = NULL,
  .onchange = NULL,
};

REGISTER_UI_EVENT_HANDLER(TIMER_HOUR_CUT)
.ontouch = TIMER_SET_BUT_ontouch,
 .onkey = NULL,
  .onchange = NULL,
};

REGISTER_UI_EVENT_HANDLER(TIMER_MIN_CUT)
.ontouch = TIMER_SET_BUT_ontouch,
 .onkey = NULL,
  .onchange = NULL,
};

REGISTER_UI_EVENT_HANDLER(TIMER_SEC_CUT)
.ontouch = TIMER_SET_BUT_ontouch,
 .onkey = NULL,
  .onchange = NULL,
};

REGISTER_UI_EVENT_HANDLER(BASEFORM_751)
.ontouch = TIMER_SET_BUT_ontouch,
 .onkey = NULL,
  .onchange = NULL,
};

REGISTER_UI_EVENT_HANDLER(BASEFORM_478)
.ontouch = NULL,
 .onkey = NULL,
  .onchange = TIMER_SET_NUM_onchange,
};

#endif /* #if TCFG_UI_ENABLE_TIMER_ACTION */
#endif /* #if (!TCFG_LUA_ENABLE) */

