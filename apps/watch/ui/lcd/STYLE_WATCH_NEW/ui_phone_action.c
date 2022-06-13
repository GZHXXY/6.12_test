#include "app_config.h"
#include "ui/ui.h"
#include "ui/ui_api.h"
#include "ui/ui_style.h"
#include "app_task.h"
#include "system/timer.h"
#include "btstack/avctp_user.h"
#include "app_main.h"


#if TCFG_UI_ENABLE && (!TCFG_LUA_ENABLE)
#ifdef CONFIG_UI_STYLE_JL_ENABLE
#if TCFG_UI_ENABLE_PHONE_ACTION

#define STYLE_NAME  JL


static u16 phone_timer = 0;

extern void set_call_log_type(u8 type);
extern BT_USER_PRIV_VAR bt_user_priv_var;

static int phone_ui_switch_init(int id)
{

    printf("%s STATUS = %d\n", __FUNCTION__, get_call_status());
    if ((get_call_status() == BT_CALL_OUTGOING) ||
        (get_call_status() == BT_CALL_ALERT)) {
        ui_show(PHONE_OUT_LAYOUT);
    } else if (get_call_status() == BT_CALL_INCOMING) {
        ui_show(PHONE_INCOMING_LAYOUT);
    } else if (get_call_status() == BT_CALL_ACTIVE) {
        ui_show(PHONE_ACTIVE_LAYOUT);
    } else {

    }
    return 0;
}

static void phone_status_check(void *p)
{
    if (!phone_timer) {
        return ;
    }
}

static int bt_phone_handler(const char *type, u32 arg)
{
    if (arg) {
    }
    return 0;
}



/************************************************************
    窗口app与ui的消息交互注册 app可以发生消息到ui进行显示
 ************************************************************/

static const struct uimsg_handl ui_msg_handler[] = {
    { "phone_num",        bt_phone_handler     },
    { NULL, NULL},      /* 必须以此结尾！ */
};


/************************************************************
                         蓝牙电话主页窗口控件
              可以在这个窗口注册各个布局需要用的资源
 ************************************************************/


static int bt_phone_mode_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct window *window = (struct window *)ctr;
    switch (e) {
    case ON_CHANGE_INIT:
        puts("\n***bt_phone_mode_onchange***\n");
        /*
        * 注册APP消息响应
        */
        ui_register_msg_handler(ID_WINDOW_BT, ui_msg_handler);//注册消息交互的回调
        ui_set_call(phone_ui_switch_init, 0);
        if (!phone_timer) {
            phone_timer = sys_timer_add(NULL, phone_status_check, 500);
        }
#if TCFG_UI_ENABLE
        ui_auto_shut_down_disable();
        key_ui_takeover(1);
#endif
        break;
    case ON_CHANGE_RELEASE:
        if (phone_timer) {
            sys_timeout_del(phone_timer);
        }
        phone_timer = 0;
#if TCFG_UI_ENABLE
        ui_auto_shut_down_enable();
        key_ui_takeover(0);
#endif
        break;
    default:
        return false;
    }
    return false;
}



REGISTER_UI_EVENT_HANDLER(PAGE_6)
.onchange = bt_phone_mode_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};






static int phone_coming_layout_onkey(void *ctr, struct element_key_event *e)
{
    printf("%s %d\n", __FUNCTION__, e->value);
    switch (e->value) {
    case KEY_OK:
        if (get_call_status() == BT_CALL_INCOMING) {
            user_send_cmd_prepare(USER_CTRL_HFP_CALL_ANSWER, 0, NULL);
        } else {
            ui_hide(PAGE_6);
        }
        break;
    default:
        if (get_call_status() == BT_CALL_INCOMING) {
            user_send_cmd_prepare(USER_CTRL_HFP_CALL_HANGUP, 0, NULL);
        }
        /* ui_hide(PAGE_6); */
        break;
    }
    return false;
}



REGISTER_UI_EVENT_HANDLER(PHONE_INCOMING_LAYOUT)
.onchange = NULL,
 .onkey = phone_coming_layout_onkey,
  .ontouch = NULL,
};


static int phone_active_layout_onkey(void *ctr, struct element_key_event *e)
{
    printf("%s %d\n", __FUNCTION__, e->value);
    switch (e->value) {
    case KEY_OK:
        if (get_call_status() == BT_CALL_ACTIVE) {
            user_send_cmd_prepare(USER_CTRL_HFP_CALL_HANGUP, 0, NULL);
        }
        /* ui_hide(PAGE_6); */
        break;
    default:
        if (get_call_status() == BT_CALL_ACTIVE) {
            user_send_cmd_prepare(USER_CTRL_HFP_CALL_HANGUP, 0, NULL);
        }
        /* ui_hide(PAGE_6); */
        break;
    }
    return false;
}



REGISTER_UI_EVENT_HANDLER(PHONE_ACTIVE_LAYOUT)
.onchange = NULL,
 .onkey = phone_active_layout_onkey,
  .ontouch = NULL,
};


static int phone_out_layout_onkey(void *ctr, struct element_key_event *e)
{
    printf("%s %d\n", __FUNCTION__, e->value);
    switch (e->value) {
    case KEY_OK:
        if (get_call_status() == BT_CALL_OUTGOING) {
            user_send_cmd_prepare(USER_CTRL_HFP_CALL_HANGUP, 0, NULL);
        }
        /* ui_hide(PAGE_6); */
        break;
    default:
        if (get_call_status() == BT_CALL_OUTGOING) {
            user_send_cmd_prepare(USER_CTRL_HFP_CALL_HANGUP, 0, NULL);
        }
        /* ui_hide(PAGE_6); */

        break;
    }
    return false;
}



REGISTER_UI_EVENT_HANDLER(PHONE_OUT_LAYOUT)
.onchange = NULL,
 .onkey = phone_out_layout_onkey,
  .ontouch = NULL,
};



static u16 phone_num_timer = 0;
static u8 phone_num[16] = {"               "};
static u8 phone_name[20] = {0};

static void PHONE_TEXT_test(void *ptr)
{
    int len = 0;
    if (bt_user_priv_var.phone_num_flag) {
        snprintf((char *)phone_num, sizeof(phone_num), "%s", bt_user_priv_var.income_phone_num);
        extern int phonebook_get_name_by_number(u8 * number, u8 * name);
        len = phonebook_get_name_by_number(phone_num, phone_name);
        if (len > 0) {
            ui_text_set_textu_by_id(PHONE_NAME_TEXT, (const char *)phone_name, len, FONT_DEFAULT);
        } else {
            ui_text_set_textu_by_id(PHONE_NAME_TEXT, "UNKNOW", strlen("UNKNOW"), FONT_DEFAULT);
        }
        ui_text_set_str_by_id(PHONE_NUM_TEXT, "ascii", (const char *)phone_num);
        sys_timeout_del(phone_num_timer);
        phone_num_timer = 0;
    }
}



static int phone_num_ascii_onchange(void *ctr, enum element_change_event e, void *arg)
{
    int index;
    struct ui_text *text = (struct ui_text *)ctr;
    struct ui_text_attrs *text_attrs = &(text->attrs);
    switch (e) {
    case ON_CHANGE_INIT:
        memset(phone_num, ' ', sizeof(phone_num));
        phone_num[sizeof(phone_num) - 1] = 0;

        text_attrs->str = (const char *)phone_num;
        if (!phone_num_timer) {
            sys_timeout_add(NULL, PHONE_TEXT_test, 1000);
        }
        break;
    case ON_CHANGE_RELEASE:
        if (phone_num_timer) {
            sys_timeout_del(phone_num_timer);
        }
        phone_num_timer = 0;
        break;
    default:
        break;
    }
    return false;
}

REGISTER_UI_EVENT_HANDLER(PHONE_NUM_TEXT)
.onchange = phone_num_ascii_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};


static int button_phone_incoming_hangup(void *ctr, struct element_touch_event *e)
{
    printf("__FUNCTION__ = %s\n", __FUNCTION__);
    switch (e->event) {
    case ELM_EVENT_TOUCH_DOWN:
        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:

        break;
    case ELM_EVENT_TOUCH_UP:
        if (get_call_status() == BT_CALL_INCOMING) {
            set_call_log_type(3);
            user_send_cmd_prepare(USER_CTRL_HFP_CALL_HANGUP, 0, NULL);
        }
        ui_hide(PAGE_6);
        break;
    }
    return false;
}
REGISTER_UI_EVENT_HANDLER(PHONE_INCOMING_HANGUP)
.onchange = NULL,
 .onkey = NULL,
  .ontouch = button_phone_incoming_hangup,
};


static int button_phone_incoming_answer(void *ctr, struct element_touch_event *e)
{
    printf("__FUNCTION__ = %s\n", __FUNCTION__);
    switch (e->event) {
    case ELM_EVENT_TOUCH_DOWN:
        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:

        break;
    case ELM_EVENT_TOUCH_UP:
        if (get_call_status() == BT_CALL_INCOMING) {
            set_call_log_type(2);
            user_send_cmd_prepare(USER_CTRL_HFP_CALL_ANSWER, 0, NULL);
        } else {
            ui_hide(PAGE_6);
        }
        break;
    }
    return false;
}
REGISTER_UI_EVENT_HANDLER(PHONE_INCOMING_ANSWER)
.onchange = NULL,
 .onkey = NULL,
  .ontouch = button_phone_incoming_answer,
};


static int button_phone_active_hangup(void *ctr, struct element_touch_event *e)
{
    printf("__FUNCTION__ = %s\n", __FUNCTION__);
    switch (e->event) {
    case ELM_EVENT_TOUCH_DOWN:
        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:

        break;
    case ELM_EVENT_TOUCH_UP:
        if (get_call_status() == BT_CALL_ACTIVE) {
            user_send_cmd_prepare(USER_CTRL_HFP_CALL_HANGUP, 0, NULL);
        }
        ui_hide(PAGE_6);
        break;
    }
    return false;
}
REGISTER_UI_EVENT_HANDLER(PHONE_ACTIVE_HANGUP)
.onchange = NULL,
 .onkey = NULL,
  .ontouch = button_phone_active_hangup,
};


static int button_phone_out_hangup(void *ctr, struct element_touch_event *e)
{
    printf("__FUNCTION__ = %s\n", __FUNCTION__);
    switch (e->event) {
    case ELM_EVENT_TOUCH_DOWN:
        break;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:

        break;
    case ELM_EVENT_TOUCH_UP:
        if (get_call_status() == BT_CALL_OUTGOING) {
            user_send_cmd_prepare(USER_CTRL_HFP_CALL_HANGUP, 0, NULL);
        }
        //ui_hide(PAGE_6); //修复个别双卡机型选卡时ui提前挂断的异常问题
        break;
    }
    return false;
}

REGISTER_UI_EVENT_HANDLER(PHONE_OUT_HANGUP)
.onchange = NULL,
 .onkey = NULL,
  .ontouch = button_phone_out_hangup,
};



#endif /* #if TCFG_UI_ENABLE_PHONE_ACTION */
#endif
#endif /* #if (!TCFG_LUA_ENABLE) */

