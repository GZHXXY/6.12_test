#include "common/app_common.h"
#include "app_task.h"
#include "app_main.h"
#include "key_event_deal.h"
#include "music/music.h"
#include "pc/pc.h"
/* #include "record/record.h" */
/* #include "linein/linein.h" */
/* #include "fm/fm.h" */
#include "btstack/avctp_user.h"
#include "app_power_manage.h"
#include "app_chargestore.h"
#include "usb/otg.h"
#include "usb/host/usb_host.h"
#include <stdlib.h>
#include "bt/bt_tws.h"
#include "bt/bt.h"
#include "bt/bt_emitter.h"
#include "audio_config.h"
#include "common/power_off.h"
#include "common/user_msg.h"
#include "audio_config.h"
#include "audio_enc.h"
#include "ui/ui_api.h"
/* #include "fm_emitter/fm_emitter_manage.h" */
/* #include "common/fm_emitter_led7_ui.h" */
/* #include "audio_reverb.h" */
#if TCFG_CHARGE_ENABLE
#include "app_charge.h"
#endif
#include "dev_multiplex_api.h"
/* #include "charge_box/charge_ctrl.h" */
#include "device/chargebox.h"
#include "app_online_cfg.h"
/* #include "soundcard/soundcard.h" */
/* #include "smartbox_bt_manage.h" */
#include "dev_manager.h"
#include "common/dev_status.h"
#include "tone_player.h"
#include "ui_manage.h"
#include "soundbox.h"
#include "watch_common.h"
#include "sport/sport_api.h"
#include "sport_info_sync.h"
#define LOG_TAG_CONST       APP_ACTION
#define LOG_TAG             "[APP_ACTION]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"


int JL_rcsp_event_handler(struct rcsp_event *rcsp);
int bt_background_event_handler(struct sys_event *event);
extern u32 timer_get_ms(void);
extern int alarm_sys_event_handler(struct sys_event *event);
extern void bt_tws_sync_volume();

#if TCFG_UI_ENABLE

u32 ui_show_page_list[9] = {DIAL_PAGE_0, PAGE_7, PAGE_13};

__attribute__((weak))u8 ui_show_menu_page()
{
    UI_HIDE_CURR_WINDOW();
    UI_SHOW_WINDOW(ID_WINDOW_VMENU);
    return true;
}

static void lcd_ui_key_change_page(int key_value)
{
    static int curr_page = 0;
    int window_id = ui_get_current_window_id();
    int i;

    if (!lcd_backlight_status()) {
        return;
    }
    ui_hide_curr_main();

    for (i = 0; i < sizeof(ui_show_page_list) / sizeof(ui_show_page_list[0]); i++) {
        if (window_id == ui_show_page_list[i]) {
            curr_page = i;
            break;
        }
    }

__retry:
    if (key_value & BIT(31)) {
        u32 page = key_value & 0x7fffffff;
        int page_in_list = false;
        for (i = 0; i < sizeof(ui_show_page_list) / sizeof(ui_show_page_list[0]); i++) {
            if (ui_show_page_list[i] == page) {
                curr_page = i;
                page_in_list = true;
                break;
            }
        }
        if (!page_in_list) {
            ui_show_main(page);
            return;
        }
    } else {
        if (!key_value) {
            do {
                curr_page++;
                if (curr_page > (sizeof(ui_show_page_list) / sizeof(ui_show_page_list[0]) - 1)) {
                    curr_page = 0;
                }
            } while (ui_show_page_list[curr_page] == 0);
        } else {
            do {
                curr_page--;
                if (curr_page < 0) {
                    curr_page = sizeof(ui_show_page_list) / sizeof(ui_show_page_list[0]) - 1;
                }
            } while (ui_show_page_list[curr_page] == 0);
        }
    }

    /* if (ui_page_list[curr_page] == PAGE_10 && (!dev_manager_get_total(1))) { */
    /*     goto __retry; */
    /* }//判断音乐播放页面 判断是否有音乐设备 判断是否蓝牙已经连接 */
    /*  */

    /* if (ui_page_list[curr_page] == PAGE_10 && dev_manager_get_total(1) && app_get_curr_task() != APP_MUSIC_TASK) { */
    /*     app_task_switch_to(APP_MUSIC_TASK); */
    /*     //判断音乐播放页面 判断是否音乐模式 */
    /*     return; */
    /* } */
    /*  */

    ui_show_main(ui_show_page_list[curr_page]);

}
#else /* #if TCFG_UI_ENABLE */

static void lcd_ui_key_change_page(int key_value)
{
}
#endif /* #if TCFG_UI_ENABLE */



int app_common_key_msg_deal(struct sys_event *event)
{
    int ret = false;
    struct key_event *key = &event->u.key;
    int key_event = event->u.key.event;
    int key_value = event->u.key.value;

    if (key_event == KEY_NULL) {
        return false;
    }

#if TCFG_UI_ENABLE
    if (key_is_ui_takeover()) {
        switch (key_event) {
        case KEY_UI_HOME:
        case KEY_UI_SHORTCUT:
            ui_key_msg_post(key_event);
            return false;
            break;

        default:
            if (key_event < 0x80) {
                ui_key_msg_post(key_event);
                return false;
            }
            break;
        }
    }
#endif

#if (TCFG_UI_ENABLE && TCFG_APP_FM_EMITTER_EN)
    if (!ui_fm_emitter_common_key_msg(key_event)) {
        return false;
    }
#endif

    log_info("common_key_event:%d\n", key_event);

    if ((key_event != KEY_POWEROFF) && (key_event != KEY_POWEROFF_HOLD)) {
        extern u8 goto_poweroff_first_flag;
        goto_poweroff_first_flag = 0;
    }

#if (SMART_BOX_EN)
    extern bool smartbox_key_event_filter_before(int key_event);
    if (smartbox_key_event_filter_before(key_event)) {
        return true;
    }
#endif
    struct watch_execise __execise_hd;
    watch_execise_handle_get(&__execise_hd);
    u8 sport_status = __execise_hd.execise_ctrl_status_get();

    switch (key_event) {
    case KEY_CHANGE_PAGE:
        log_info("   KEY_CHANGE_PAGE  \n");
        if (app_get_curr_task() == APP_WATCH_UPDATE_TASK ||
            app_get_curr_task() == APP_SMARTBOX_ACTION_TASK) {
            printf(" curr is upate, can not opt \n");
            break;
        }

        if (get_call_status() == BT_CALL_HANGUP) {
            lcd_ui_key_change_page(key_value);
            extern void smartbox_extra_flash_opt_dial_nodify(void);
            smartbox_extra_flash_opt_dial_nodify();
        }
        break;
#if TCFG_UI_ENABLE
    case KEY_UI_HOME:
        log_info("   KEY_UI_MENU  \n");
        if ((sport_status != 4) & (sport_status != 0)) {
            if (ID_WINDOW_SPORT_CTRL == ui_get_current_window_id()) {
                UI_HIDE_CURR_WINDOW();
                UI_SHOW_WINDOW(ID_WINDOW_SPORT_INFO);
            } else {
                __execise_hd.execise_ctrl_status_set(-1, SPORT_STATUS_PAUSE);
                sport_info_sync_pause_exercise_resp();
                watch_sport_pause();

                UI_HIDE_CURR_WINDOW();
                UI_SHOW_WINDOW(ID_WINDOW_SPORT_CTRL);
            }
            break;
        }
        if (ID_WINDOW_BT == ui_get_current_window_id()) {
            ui_show_menu_page();
        } else {
            UI_HIDE_CURR_WINDOW();
            UI_SHOW_WINDOW(ID_WINDOW_BT);
        }
        break;
    case KEY_UI_SHORTCUT:
        log_info("  KEY_UI_SHORTCUT  \n");
        if ((sport_status != 4) & (sport_status != 0)) {
            break;
        } else {
            if (ID_WINDOW_SPORT_SHOW == ui_get_current_window_id()) {
                ui_key_msg_post(key_event);
                break;
            }
            extern u8 ui_show_shortcut_key();
            ui_show_shortcut_key();
        }
        break;
    case KEY_UI_POWEROFF:
        log_info("  KEY_UI_POWEROFF  \n");
        UI_HIDE_CURR_WINDOW();
        UI_SHOW_WINDOW(ID_WINDOW_SHUTDOWN_OR_RESET);
        break;
#endif /* #if TCFG_UI_ENABLE */

#if TCFG_APP_BT_EN

#if TCFG_USER_TWS_ENABLE
    case KEY_TWS_CONN:
        log_info("    KEY_TWS_CONN \n");
        bt_open_tws_conn(0);
        break;
    case KEY_TWS_DISCONN:
        log_info("    KEY_TWS_DISCONN \n");
        bt_disconnect_tws_conn();
        break;
    case KEY_TWS_REMOVE_PAIR:
        log_info("    KEY_TWS_REMOVE_PAIR \n");
        bt_tws_remove_tws_pair();
        break;
    case KEY_TWS_SEARCH_PAIR:
        log_info("    KEY_TWS_SEARCH_PAIR \n");
        bt_tws_start_search_and_pair();
        break;
    case KEY_TWS_SEARCH_REMOVE_PAIR:
        log_info("    KEY_TWS_SEARCH_REMOVE_PAIR \n");
        bt_tws_search_or_remove_pair();
        break;
#endif

    case KEY_BT_DIRECT_INIT:
        bt_direct_init();
        break;
    case KEY_BT_DIRECT_CLOSE:
        bt_direct_close();
        break;
#endif

    case  KEY_POWEROFF:
    case  KEY_POWEROFF_HOLD:
        power_off_deal(event, key_event - KEY_POWEROFF);
        break;

    case KEY_IR_NUM_0:
    case KEY_IR_NUM_1:
    case KEY_IR_NUM_2:
    case KEY_IR_NUM_3:
    case KEY_IR_NUM_4:
    case KEY_IR_NUM_5:
    case KEY_IR_NUM_6:
    case KEY_IR_NUM_7:
    case KEY_IR_NUM_8:
    case KEY_IR_NUM_9:
        break;

    case KEY_CHANGE_MODE:
#if (TCFG_DEC2TWS_ENABLE)
        if (!key->init) {
            break;
        }
#endif
#if TWFG_APP_POWERON_IGNORE_DEV
        if ((timer_get_ms() - app_var.start_time) > TWFG_APP_POWERON_IGNORE_DEV)
#endif//TWFG_APP_POWERON_IGNORE_DEV

        {
            printf("KEY_CHANGE_MODE\n");
            app_task_switch_next();
        }
        break;

    case KEY_VOL_UP:
        log_info("COMMON KEY_VOL_UP\n");
        if (!tone_get_status()) {
            app_audio_volume_up(1);
            printf("common vol+: %d", app_audio_get_volume(APP_AUDIO_CURRENT_STATE));
        }
        if (app_audio_get_volume(APP_AUDIO_CURRENT_STATE) == app_audio_get_max_volume()) {
            if (tone_get_status() == 0) {
#if TCFG_MAX_VOL_PROMPT
                tone_play_by_path(tone_table[IDEX_TONE_MAX_VOL], 0);
#endif
            }
        }

#if (TCFG_DEC2TWS_ENABLE)
        bt_tws_sync_volume();
#endif
        UI_SHOW_MENU(MENU_MAIN_VOL, 1000, app_audio_get_volume(APP_AUDIO_CURRENT_STATE), NULL);
        break;

    case KEY_VOL_DOWN:
        log_info("COMMON KEY_VOL_DOWN\n");
        app_audio_volume_down(1);
        printf("common vol-: %d", app_audio_get_volume(APP_AUDIO_CURRENT_STATE));
#if (TCFG_DEC2TWS_ENABLE)
        bt_tws_sync_volume();
#endif
        UI_SHOW_MENU(MENU_MAIN_VOL, 1000, app_audio_get_volume(APP_AUDIO_CURRENT_STATE), NULL);
        break;

    case  KEY_EQ_MODE:
#if(TCFG_EQ_ENABLE == 1)
        eq_mode_sw();
#endif
        break;

#if AUDIO_OUTPUT_INCLUDE_BT

    case KEY_BT_EMITTER_RECEIVER_SW:
        printf(" KEY_BT_EMITTER_RECEIVER_SW\n");
        bt_emitter_receiver_sw();
        break;

    case KEY_BT_EMITTER_PAUSE:
        r_printf(" KEY_BT_EMITTER_PAUSE\n");
        UI_MSG_POST("bt_emitter:status=%4", 0);
        bt_emitter_pp(0);
        break;

    case KEY_BT_EMITTER_PLAY:
        r_printf(" KEY_BT_EMITTER_PLAY\n");
        UI_MSG_POST("bt_emitter:status=%4", 1);
        bt_emitter_pp(1);
        break;

    case KEY_BT_EMITTER_SW:
        r_printf("KEY_BT_EMITTER_SW\n");
        {
            extern u8 bt_emitter_stu_sw(void);

            if (bt_emitter_stu_sw()) {
                printf("bt emitter start \n");
            } else {
                printf("bt emitter stop \n");
            }
        }
        break;
#endif


#if(TCFG_CHARGE_BOX_ENABLE)
    case  KEY_BOX_POWER_CLICK:
    case  KEY_BOX_POWER_LONG:
    case  KEY_BOX_POWER_HOLD:
    case  KEY_BOX_POWER_UP:
        charge_box_key_event_handler(key_event);
        break;
#endif
#if (TCFG_MIC_EFFECT_ENABLE)
    case KEY_REVERB_OPEN:
#if TCFG_USER_TWS_ENABLE
        if (!key->init) {
            break;
        }
#endif

        if (mic_effect_get_status()) {
            mic_effect_stop();
        } else {
            mic_effect_start();
        }
        break;
#endif
    case KEY_ENC_START:
#if (RECORDER_MIX_EN)
        if (recorder_mix_get_status()) {
            printf("recorder_encode_stop\n");
            recorder_mix_stop();
        } else {
            printf("recorder_encode_start\n");
            recorder_mix_start();
        }
#endif/*RECORDER_MIX_EN*/
        break;
    default:
        ui_key_msg_post(key_event);
#ifdef CONFIG_BOARD_AC695X_SOUNDCARD
        soundcard_key_event_deal(key_event);
#endif
        break;

    }
#if (SMART_BOX_EN)
    extern int smartbox_common_key_event_deal(u8 key_event, int ret);
    ret = smartbox_common_key_event_deal(key_event, ret);
#endif
    return ret;
}

int app_power_user_event_handler(struct device_event *dev)
{
#if(TCFG_SYS_LVD_EN == 1)
    switch (dev->event) {
    case POWER_EVENT_POWER_WARNING:
        ui_update_status(STATUS_LOWPOWER);
        tone_play_by_path(tone_table[IDEX_TONE_LOW_POWER], 1);
        /* return 0; */
    }
#endif
    return app_power_event_handler(dev);
}

static void app_common_device_event_handler(struct sys_event *event)
{
    int ret = 0;
    const char *logo = NULL;
    const char *usb_msg = NULL;
    u8 app  = 0xff ;
    u8 alarm_flag = 0;
    switch ((u32)event->arg) {
#if TCFG_CHARGE_ENABLE
    case DEVICE_EVENT_FROM_CHARGE:
        app_charge_event_handler(&event->u.dev);
        break;
#endif//TCFG_CHARGE_ENABLE

#if TCFG_ONLINE_ENABLE
    case DEVICE_EVENT_FROM_CI_UART:
        ci_data_rx_handler(CI_UART);
        break;

#if TCFG_USER_TWS_ENABLE
    case DEVICE_EVENT_FROM_CI_TWS:
        ci_data_rx_handler(CI_TWS);
        break;
#endif//TCFG_USER_TWS_ENABLE
#endif//TCFG_ONLINE_ENABLE

    case DEVICE_EVENT_FROM_POWER:
        app_power_user_event_handler(&event->u.dev);
        break;

#if TCFG_CHARGESTORE_ENABLE || TCFG_TEST_BOX_ENABLE
    case DEVICE_EVENT_CHARGE_STORE:
        app_chargestore_event_handler(&event->u.chargestore);
        break;
#endif//TCFG_CHARGESTORE_ENABLE || TCFG_TEST_BOX_ENABLE

#if(TCFG_CHARGE_BOX_ENABLE)
    case DEVICE_EVENT_FROM_CHARGEBOX:
        charge_box_ctrl_event_handler(&event->u.chargebox);
        break;
#endif


    case DEVICE_EVENT_FROM_OTG:
        ///先分析OTG设备类型
        usb_msg = (const char *)event->u.dev.value;
        if (usb_msg[0] == 's') {
            ///是从机
#if TCFG_PC_ENABLE
            ret = pc_device_event_handler(event);
            if (ret == true) {
                app = APP_PC_TASK;
            }
#endif
            break;
        } else if (usb_msg[0] == 'h') {
            ///是主机, 统一于SD卡等响应主机处理，这里不break
        } else {
            log_e("unknow otg devcie !!!\n");
            break;
        }
    case DRIVER_EVENT_FROM_SD0:
    case DRIVER_EVENT_FROM_SD1:
    case DRIVER_EVENT_FROM_SD2:
#if TCFG_APP_MUSIC_EN
        ret = dev_status_event_filter(event);///解码设备上下线， 设备挂载等处理
        if (ret == true) {
            if (event->u.dev.event == DEVICE_EVENT_IN) {
                ///设备上线， 非解码模式切换到解码模式播放
                if (app_get_curr_task() != APP_MUSIC_TASK) {
                    app = APP_MUSIC_TASK;
                }
            }
        }
#if (TCFG_SD_ALWAY_ONLINE_ENABLE && (TCFG_SD0_ENABLE || TCFG_SD1_ENABLE))
        extern void sdx_dev_detect_timer_del();
        sdx_dev_detect_timer_del();
#endif
#endif
        break;

#if TCFG_APP_LINEIN_EN
    case DEVICE_EVENT_FROM_LINEIN:
        ret = linein_device_event_handler(event);
        if (ret == true) {
            app = APP_LINEIN_TASK;
        }
        break;
#endif//TCFG_APP_LINEIN_EN

#if TCFG_APP_RTC_EN
    case DEVICE_EVENT_FROM_ALM:
        ret = alarm_sys_event_handler(event);
        if (ret == true) {
            alarm_flag = 1;
            app = APP_RTC_TASK;
        }
        break;
#endif//TCFG_APP_RTC_EN

    default:
        /* printf("unknow SYS_DEVICE_EVENT!!, %x\n", (u32)event->arg); */
        break;
    }

#if (SMART_BOX_EN)
    extern void smartbox_update_dev_state(u32 event, void *param);
    if (ret == true) {
        smartbox_update_dev_state((u32)event->arg, &app);
    }
#endif

    if (app != 0xff) {
        if ((true != app_check_curr_task(APP_PC_TASK)) || alarm_flag) {

            //PC 不响应因为设备上线引发的模式切换
#if TWFG_APP_POWERON_IGNORE_DEV
            if ((timer_get_ms() - app_var.start_time) > TWFG_APP_POWERON_IGNORE_DEV)
#endif//TWFG_APP_POWERON_IGNORE_DEV
            {
#if (TCFG_CHARGE_ENABLE && (!TCFG_CHARGE_POWERON_ENABLE))
                extern u8 get_charge_online_flag(void);
                if (get_charge_online_flag()) {

                } else
#endif
                {

                    printf(">>>>>>>>>>>>>%s %d \n", __FUNCTION__, __LINE__);
                    app_task_switch_to(app);
                }
            }
        }
    }
}


///公共事件处理， 各自模式没有处理的事件， 会统一在这里处理
void app_default_event_deal(struct sys_event *event)
{
    int ret;
    SYS_EVENT_HANDLER_SPECIFIC(event);
    switch (event->type) {
    case SYS_DEVICE_EVENT:
        /*默认公共设备事件处理*/
        /* printf(">>>>>>>>>>>>>%s %d \n", __FUNCTION__, __LINE__); */
        app_common_device_event_handler(event);
        break;
#if TCFG_APP_BT_EN
    case SYS_BT_EVENT:
        if (true != app_check_curr_task(APP_BT_TASK)) {
            /*默认公共BT事件处理*/
            bt_background_event_handler(event);
        }
        break;
#endif
    case SYS_KEY_EVENT:
        app_common_key_msg_deal(event);
        break;
    default:
        printf("unknow event\n");
        break;
    }
}

