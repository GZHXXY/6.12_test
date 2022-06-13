
/*************************************************************
   此文件函数主要是蓝牙模式各种状态处理

	void app_bt_task()
   bt模式主函数

	static int bt_sys_event_handler(struct sys_event *event)
   bt模式系统事件所有处理入口

	static void bt_task_close(void)
	bt模式退出


	void bt_function_select_init()
    蓝牙功能配置

	void bredr_handle_register()
 	蓝牙协议栈回调函数

	static int bt_connction_status_event_handler(struct bt_event *bt)
	蓝牙状态事件处理函数

	static int bt_hci_event_handler(struct bt_event *bt)
	蓝牙协议栈事件回调，app处理函数

	static int bt_ai_event_handler(struct bt_event *bt)
	蓝牙ai事件处理函数

	int bt_background_event_handler_filter(struct sys_event *event)
	蓝牙后台事件过滤处理
	int bt_background_event_handler(struct sys_event *event)
	蓝牙后台事件处理函数

	int bt_key_event_handler(struct sys_event *event)
	蓝牙按键处理函数

	avctp_user.h 有蓝牙支持的命令介绍

**************************************************************/



#include "system/includes.h"
#include "media/includes.h"

#include "app_config.h"
#include "app_task.h"

#include "btstack/avctp_user.h"
#include "btstack/btstack_task.h"
#include "btstack/bluetooth.h"
#include "btstack/btstack_error.h"
#include "btctrler/btctrler_task.h"
#include "classic/hci_lmp.h"

#include "bt/bt_tws.h"
#include "bt/bt_ble.h"
#include "bt/bt.h"
#include "bt/vol_sync.h"
#include "bt/bt_emitter.h"
#include "bt_common.h"
#include "aec_user.h"

#include "math.h"
#include "spp_user.h"


#include "app_chargestore.h"
#include "app_charge.h"
#include "app_main.h"
#include "app_power_manage.h"
#include "user_cfg.h"

#include "asm/pwm_led.h"
#include "asm/timer.h"
#include "asm/hwi.h"
#include "cpu.h"

#include "ui/ui_api.h"
#include "ui_manage.h"
#include "ui/ui_style.h"

#include "key_event_deal.h"
#include "clock_cfg.h"
#include "gSensor/gSensor_manage.h"
/* #include "soundcard/soundcard.h" */

#include "audio_config.h"
#include "audio_dec.h"
/* #include "audio_reverb.h" */
#include "tone_player.h"
#include "dac.h"

#include "message_vm_cfg.h"
#include "watch_common.h"

#if TCFG_UI_ENABLE_MOTO
#include "ui/ui_sys_param.h"
#endif /* #if TCFG_UI_ENABLE_MOTO */

#define LOG_TAG_CONST        BT
#define LOG_TAG             "[BT]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"


#if TCFG_APP_BT_EN

struct app_bt_opr app_bt_hdl = {
    .exit_flag = 1,
    .replay_tone_flag = 1,
    .esco_dump_packet = ESCO_DUMP_PACKET_CALL,
    .hid_mode = 0,
    .a2dp_en = 0,
    .music_dev = SET_MUSIC_IN_WATCH,
};

#define __this 	(&app_bt_hdl)

BT_USER_PRIV_VAR bt_user_priv_var;


void set_call_log_number(u8 *number);
void set_call_log_date(u8 *date);
void update_call_log_message();

extern void __set_support_3M_size(u8 en);

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式变量初始化
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_var_init()
{
    memset((u8 *)&bt_user_priv_var, 0, sizeof(BT_USER_PRIV_VAR));
}

void set_smartbox_watch_upgrade_flag(u8 flag)
{
    __this->smartbox_watch_upgrade_flag = flag;
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式协议栈回调函数
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
void bredr_handle_register()
{
#if TRANS_DATA_EN
    spp_data_deal_handle_register(user_spp_data_handler);
#else
    spp_data_deal_handle_register(spp_data_handler);
#endif

#if BT_SUPPORT_MUSIC_VOL_SYNC
    ///蓝牙音乐和通话音量同步
    music_vol_change_handle_register(bt_set_music_device_volume, phone_get_device_vol);
#endif

#if BT_SUPPORT_DISPLAY_BAT
    ///电量显示获取电量的接口
    get_battery_value_register(bt_get_battery_value);
#endif

    ///被测试盒链接上进入快速测试回调
    bt_fast_test_handle_register(bt_fast_test_api);

    ///样机进入dut被测试仪器链接上回调
    bt_dut_test_handle_register(bt_dut_api);

    ///获取远端设备蓝牙名字回调
    read_remote_name_handle_register(bt_read_remote_name);

    ////获取歌曲信息回调
    /* bt_music_info_handle_register(user_get_bt_music_info); */

}


/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式协议栈功能配置
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_function_select_init()
{
#if defined(CONFIG_CPU_BR28)
    // 3M包
    set_bt_data_rate_acl_3mbs_mode(1);
    __set_support_3M_size(1);
#endif /* #if defined(CONFIG_CPU_BR28) */
    set_idle_period_slot(1600);
    ////设置协议栈支持设备数
    __set_user_ctrl_conn_num(TCFG_BD_NUM);
    ////msbc功能使能
    __set_support_msbc_flag(1);
#if TCFG_BT_SUPPORT_AAC
    ////AAC功能使能
    __set_support_aac_flag(1);
#else
    __set_support_aac_flag(0);
#endif

#if BT_SUPPORT_DISPLAY_BAT
    ////设置更新电池电量的时间间隔
    __bt_set_update_battery_time(60);
#else
    __bt_set_update_battery_time(0);
#endif

    ////回连搜索时间长度设置,可使用该函数注册使用，ms单位,u16
    __set_page_timeout_value(8000);

    ////回连时超时参数设置。ms单位。做主机有效
    __set_super_timeout_value(8000);

#if (TCFG_BD_NUM == 2)
    ////设置开机回链的设备个数
    /* __set_auto_conn_device_num(2); */
#endif

#if (TCFG_SPI_LCD_ENABLE)
#if TCFG_USER_EMITTER_ENABLE//带有屏幕的方案根据UI选项连接
    ////设置开机回链的设备个数
    __set_auto_conn_device_num(10);//彩屏方案支持10个设备的连接
#endif
#endif

#if BT_SUPPORT_MUSIC_VOL_SYNC
    ////设置音乐音量同步的表
    vol_sys_tab_init();
#endif

    ////设置蓝牙是否跑后台
    __set_user_background_goback(BACKGROUND_GOBACK); // 后台链接是否跳回蓝牙 1:跳回

    ////设置蓝牙加密的level
    //io_capabilities ; /*0: Display only 1: Display YesNo 2: KeyboardOnly 3: NoInputNoOutput*/
    //authentication_requirements: 0:not protect  1 :protect
    __set_simple_pair_param(3, 0, 2);

#if AUDIO_OUTPUT_INCLUDE_FM
    bt_set_ldos(2);
#endif

#if (USER_SUPPORT_PROFILE_PBAP==1)
    ////设置蓝牙设备类型
    __change_hci_class_type(BD_CLASS_CAR_AUDIO);
#endif

#if (TCFG_BT_SNIFF_ENABLE == 0)
    void lmp_set_sniff_disable(void);
    lmp_set_sniff_disable();
#endif


    /*
                TX     RX
       AI800x   PA13   PA12
       AC692x   PA13   PA12
       AC693x   PA8    PA9
       AC695x   PA9    PA10
       AC696x   PA9    PA10
       AC694x   PB1    PB2
       AC697x   PC2    PC3
       AC631x   PA7    PA8

    */
    ////设置蓝牙接收状态io输出，可以外接pa
    /* bt_set_rxtx_status_enable(1); */

#if TCFG_USER_BLE_ENABLE
    {
        u8 tmp_ble_addr[6];
#if (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_ADV)
        /* bt_set_tx_power(9);//ble txpwer level:0~9 */
        memcpy(tmp_ble_addr, (void *)bt_get_mac_addr(), 6);
#else
        lib_make_ble_address(tmp_ble_addr, (void *)bt_get_mac_addr());
#endif //
        le_controller_set_mac((void *)tmp_ble_addr);
        printf("\n-----edr + ble 's address-----");
#if TCFG_UI_ENABLE_QR_CODE
        extern void qr_code_message_init(u8 * ble_addr, u8 * edr_addr, u8 * name);
        qr_code_message_init(tmp_ble_addr, bt_get_mac_addr(), (u8 *)bt_get_local_name());
#endif /* #if TCFG_UI_ENABLE_QR_CODE */
        printf_buf((void *)bt_get_mac_addr(), 6);
        printf_buf((void *)tmp_ble_addr, 6);
    }
#endif // TCFG_USER_BLE_ENABLE

#if (CONFIG_BT_MODE != BT_NORMAL)
    set_bt_enhanced_power_control(1);
#endif
}


/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式协议栈对应状态处理函数
   @param    bt:事件
   @return
   @note     蓝牙初始化完成、链接、通话播歌等状态
*/
/*----------------------------------------------------------------------------*/
static int bt_connction_status_event_handler(struct bt_event *bt)
{

    log_debug("-----------------------bt_connction_status_event_handler %d", bt->event);

    if (bt_status_event_filter(bt) == false) {
        return false;
    }
    //运动相关的
    static u8 sport_phone_memory = 0;
    struct watch_execise __execise_hd;
    watch_execise_handle_get(&__execise_hd);
    u8 sport_status = __execise_hd.execise_ctrl_status_get();
    switch (bt->event) {
    case BT_STATUS_EXIT_OK:
        log_info("BT_STATUS_EXIT_OK\n");
        break;
    case BT_STATUS_INIT_OK:
        log_info("BT_STATUS_INIT_OK\n");
        bt_status_init_ok(bt);
        break;
    case BT_STATUS_START_CONNECTED:
        log_info(" BT_STATUS_START_CONNECTED\n");
        break;
    case BT_STATUS_ENCRY_COMPLETE:
        log_info(" BT_STATUS_ENCRY_COMPLETE\n");
        break;
    case BT_STATUS_SECOND_CONNECTED:
        log_info(" BT_STATUS_SECOND_CONNECTED\n");
        clear_current_poweron_memory_search_index(0);
    case BT_STATUS_FIRST_CONNECTED:
        log_info("BT_STATUS_CONNECTED\n");
        bt_status_connect(bt);
        break;
    case BT_STATUS_FIRST_DISCONNECT:
    case BT_STATUS_SECOND_DISCONNECT:
        log_info(" BT_STATUS_SECOND_DISCONNECT\n");
        bt_status_disconnect(bt);
#if (defined(TCFG_BREDR_CTRL_EN) && (TCFG_BREDR_CTRL_EN))
        bt_close_bredr();
#endif
        break;
    case BT_STATUS_PHONE_INCOME:
        log_info("BT_STATUS_PHONE_INCOME\n");
        //当手机app与手表传输过程中不能响应通话事件
        if (__this->smartbox_watch_upgrade_flag) {
            break;
        }
        UI_HIDE_CURR_WINDOW();
        UI_SHOW_WINDOW(ID_WINDOW_PHONE);
        log_info(" ui_moto_run 1");
#if TCFG_UI_ENABLE_MOTO
        UI_MOTO_RUN(1);
#endif
        bt_status_phone_income(bt);
#if TCFG_UI_ENABLE_PHONEBOOK
        set_call_log_date(NULL);
        set_call_log_type(2);
#endif /* #if TCFG_UI_ENABLE_PHONEBOOK */
        break;
    case BT_STATUS_PHONE_OUT:
        log_info("BT_STATUS_PHONE_OUT\n");
        //当手机app与手表传输过程中不能响应通话事件
        if (__this->smartbox_watch_upgrade_flag) {
            break;
        }
        log_info(" ui_moto_run 0");
#if TCFG_UI_ENABLE_MOTO
        UI_MOTO_RUN(0);
#endif
        bt_status_phone_out(bt);
        UI_HIDE_CURR_WINDOW();
        UI_SHOW_WINDOW(ID_WINDOW_PHONE);
#if TCFG_UI_ENABLE_PHONEBOOK
        set_call_log_date(NULL);
#endif /* #if TCFG_UI_ENABLE_PHONEBOOK */
        break;
    case BT_STATUS_PHONE_ACTIVE:
        log_info("BT_STATUS_PHONE_ACTIVE\n");
        //当手机app与手表传输过程中不能响应通话事件
        if (__this->smartbox_watch_upgrade_flag) {
            break;
        }
#if TCFG_UI_ENABLE_MOTO
        UI_MOTO_RUN(0);
#endif
        bt_status_phone_active(bt);
        /* printf("%s sport_status%d", __func__,sport_status); */
        if ((sport_status != 4) & (sport_status != 0)) {//运动过程中不响应通话界面
            UI_HIDE_CURR_WINDOW();
            UI_SHOW_WINDOW(ID_WINDOW_SPORT_INFO);
            sport_phone_memory = 1;
            break;
        } else {
            UI_HIDE_CURR_WINDOW();
            UI_SHOW_WINDOW(ID_WINDOW_PHONE);
        }
        break;
    case BT_STATUS_PHONE_HANGUP:
        log_info(" BT_STATUS_PHONE_HANGUP\n");
        //当手机app与手表传输过程中不能响应通话事件
        if (__this->smartbox_watch_upgrade_flag) {
            __this->smartbox_watch_upgrade_flag = 0;
            break;
        }


#if TCFG_UI_ENABLE_MOTO
        UI_MOTO_RUN(0);
#endif
        bt_status_phone_hangup(bt);
#if TCFG_UI_ENABLE_PHONEBOOK
        update_call_log_message();
#endif /* #if TCFG_UI_ENABLE_PHONEBOOK */
        printf("%s", __func__);
        if ((sport_status != 4) & (sport_status != 0)) {//运动过程中不响应通话界面
            UI_HIDE_CURR_WINDOW();
            UI_SHOW_WINDOW(ID_WINDOW_SPORT_INFO);
        } else {
            UI_HIDE_CURR_WINDOW();
            if (sport_phone_memory) {
                UI_SHOW_WINDOW(ID_WINDOW_CLOCK);
            } else {
                UI_SHOW_WINDOW(-1);
            }
            sport_phone_memory = 0;
        }
#if (defined(TCFG_BREDR_CTRL_EN) && (TCFG_BREDR_CTRL_EN))
        bt_close_bredr();
#endif
        break;
    case BT_STATUS_PHONE_NUMBER:
        log_info("BT_STATUS_PHONE_NUMBER\n");
        bt_status_phone_number(bt);
#if TCFG_UI_ENABLE_MOTO
        if (get_call_status() == BT_CALL_INCOMING) {
            log_info(" ui_moto_run 1");
            UI_MOTO_RUN(1);
        }
#endif
#if TCFG_UI_ENABLE_PHONEBOOK
        u8 call_name[20] = {0};
        if (phonebook_get_name_by_number(bt->value, call_name)) {
            set_call_log_name(call_name);
        }
        set_call_log_number((u8 *)bt->value);
#endif /* #if TCFG_UI_ENABLE_PHONEBOOK */
        break;
    case BT_STATUS_INBAND_RINGTONE:
        log_info("BT_STATUS_INBAND_RINGTONE\n");
        bt_status_inband_ringtone(bt);
        break;
    case BT_STATUS_BEGIN_AUTO_CON:
        log_info("BT_STATUS_BEGIN_AUTO_CON\n");
        break;
    case BT_STATUS_A2DP_MEDIA_START:
        log_info(" BT_STATUS_A2DP_MEDIA_START\n");
        bt_status_a2dp_media_start(bt);
        break;
    case BT_STATUS_A2DP_MEDIA_STOP:
        log_info(" BT_STATUS_A2DP_MEDIA_STOP");
        bt_status_a2dp_media_stop(bt);
        break;
    case BT_STATUS_SCO_STATUS_CHANGE:
        log_info(" BT_STATUS_SCO_STATUS_CHANGE");
        log_info(" ui_moto_run 0");
#if TCFG_UI_ENABLE_MOTO
        UI_MOTO_RUN(0);
#endif
        bt_status_sco_change(bt);
        break;
    case BT_STATUS_CALL_VOL_CHANGE:
        log_info(" BT_STATUS_CALL_VOL_CHANGE ");
        bt_status_call_vol_change(bt);
        break;
    case BT_STATUS_SNIFF_STATE_UPDATE:
        log_info(" BT_STATUS_SNIFF_STATE_UPDATE \n");    //0退出SNIFF
        bt_status_sniff_state_update(bt);
        break;
    case BT_STATUS_LAST_CALL_TYPE_CHANGE:
        log_info("BT_STATUS_LAST_CALL_TYPE_CHANGE\n");
        bt_status_last_call_type_change(bt);
        break;
    case BT_STATUS_CONN_A2DP_CH:
        bt_status_conn_a2dp_ch(bt);
        break;
    case BT_STATUS_CONN_HFP_CH:
        bt_status_conn_hfp_ch(bt);
        break;
    case BT_STATUS_PHONE_MANUFACTURER:
        log_info("BT_STATUS_PHONE_MANUFACTURER\n");
        bt_status_phone_menufactuer(bt);
        break;
    case BT_STATUS_VOICE_RECOGNITION:
        log_info(" BT_STATUS_VOICE_RECOGNITION \n");
        bt_status_voice_recognition(bt);
        break;
    case BT_STATUS_AVRCP_INCOME_OPID:
        log_info("  BT_STATUS_AVRCP_INCOME_OPID \n");
        bt_status_avrcp_income_opid(bt);
        break;
    case  BT_STATUS_RECONN_OR_CONN:
        log_info("  BT_STATUS_RECONN_OR_CONN \n");
        break;
    default:
        log_info(" BT STATUS DEFAULT\n");
        break;
    }
    return 0;
}




static int bt_update_hci_event_handler(struct bt_event *bt)
{
    //对应原来的蓝牙连接上断开处理函数  ,bt->value=reason
    log_debug("------------bt_update_hci_event_handler reason %x %x", bt->event, bt->value);

    switch (bt->event) {
    case HCI_EVENT_INQUIRY_COMPLETE:
        log_info(" HCI_EVENT_INQUIRY_COMPLETE \n");
        break;
    case HCI_EVENT_IO_CAPABILITY_REQUEST:
        log_info(" HCI_EVENT_IO_CAPABILITY_REQUEST \n");
        clock_add_set(BT_CONN_CLK);
        break;
    case HCI_EVENT_USER_CONFIRMATION_REQUEST:
        log_info(" HCI_EVENT_USER_CONFIRMATION_REQUEST \n");
        ///<可通过按键来确认是否配对 1：配对   0：取消
        bt_send_pair(1);
        clock_remove_set(BT_CONN_CLK);
        break;
    case HCI_EVENT_USER_PASSKEY_REQUEST:
        log_info(" HCI_EVENT_USER_PASSKEY_REQUEST \n");
        ///<可以开始输入6位passkey
        break;
    case HCI_EVENT_REMOTE_KEYPRESS_NOTIFICATION:
        log_info(" HCI_EVENT_REMOTE_KEYPRESS_NOTIFICATION %x\n", bt->value);
        ///<可用于显示输入passkey位置 value 0:start  1:enrer  2:earse   3:clear  4:complete
        break;
    case HCI_EVENT_USER_PRESSKEY_NOTIFICATION:
        log_info(" HCI_EVENT_USER_PRESSKEY_NOTIFICATION %x\n", bt->value);
        break;
    case HCI_EVENT_PIN_CODE_REQUEST :
        log_info("HCI_EVENT_PIN_CODE_REQUEST  \n");
        bt_send_pair(1);
        break;
    case HCI_EVENT_VENDOR_NO_RECONN_ADDR :
        log_info("HCI_EVENT_VENDOR_NO_RECONN_ADDR \n");
        /* user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_ENABLE, 0, NULL); */
        user_send_cmd_prepare(USER_CTRL_WRITE_CONN_ENABLE, 0, NULL);
        break;
    case HCI_EVENT_DISCONNECTION_COMPLETE :
        log_info("HCI_EVENT_DISCONNECTION_COMPLETE \n");
        /* user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_ENABLE, 0, NULL); */
        user_send_cmd_prepare(USER_CTRL_WRITE_CONN_ENABLE, 0, NULL);
        clock_remove_set(BT_CONN_CLK);
        break;
    case BTSTACK_EVENT_HCI_CONNECTIONS_DELETE:
    case HCI_EVENT_CONNECTION_COMPLETE:
        log_info(" HCI_EVENT_CONNECTION_COMPLETE \n");
        switch (bt->value) {
        case ERROR_CODE_SUCCESS :
            log_info("ERROR_CODE_SUCCESS  \n");
            break;
        case ERROR_CODE_PIN_OR_KEY_MISSING:
            log_info(" ERROR_CODE_PIN_OR_KEY_MISSING \n");
            bt_hci_event_linkkey_missing(bt);
        case ERROR_CODE_SYNCHRONOUS_CONNECTION_LIMIT_TO_A_DEVICE_EXCEEDED :
        case ERROR_CODE_CONNECTION_REJECTED_DUE_TO_LIMITED_RESOURCES:
        case ERROR_CODE_CONNECTION_REJECTED_DUE_TO_UNACCEPTABLE_BD_ADDR:
        case ERROR_CODE_CONNECTION_ACCEPT_TIMEOUT_EXCEEDED  :
        case ERROR_CODE_REMOTE_USER_TERMINATED_CONNECTION   :
        case ERROR_CODE_CONNECTION_TERMINATED_BY_LOCAL_HOST :
        case ERROR_CODE_AUTHENTICATION_FAILURE :
        case CUSTOM_BB_AUTO_CANCEL_PAGE:
        case ERROR_CODE_PAGE_TIMEOUT:
        case ERROR_CODE_CONNECTION_TIMEOUT:
        case ERROR_CODE_ACL_CONNECTION_ALREADY_EXISTS  :
            /* user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_ENABLE, 0, NULL); */
            user_send_cmd_prepare(USER_CTRL_WRITE_CONN_ENABLE, 0, NULL);
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
    return 0;
}


static int bt_update_connction_status_event_handler(struct bt_event *bt)
{
    log_debug("------------bt_update_connction_status_event_handler %d", bt->event);

    switch (bt->event) {
    case BT_STATUS_INIT_OK:
        log_info("BT_STATUS_INIT_OK\n");

#if TCFG_USER_EMITTER_ENABLE
        ////发射器设置回调等
        void bt_emitter_init() ;
        bt_emitter_init();
        extern u8 emitter_search_result(char *name, u8 name_len, u8 * addr, u32 dev_class, char rssi);
        inquiry_result_handle_register(emitter_search_result);
        extern void emitter_or_receiver_switch(u8 flag);

        emitter_or_receiver_switch(BT_EMITTER_EN);
#endif
        /* user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_ENABLE, 0, NULL);
        user_send_cmd_prepare(USER_CTRL_WRITE_CONN_ENABLE, 0, NULL); */
        break;
    }
    return 0;
}

int bt_update_event_handler(struct sys_event *event)
{
    int ret = false;
    switch (event->type) {
    case SYS_BT_EVENT:
        if ((u32)event->arg == SYS_BT_EVENT_TYPE_CON_STATUS) {
            bt_update_connction_status_event_handler(&event->u.bt);
        } else if ((u32)event->arg == SYS_BT_EVENT_TYPE_HCI_STATUS) {
            bt_update_hci_event_handler(&event->u.bt);
        }
        return true;
    default:
        return false;
    }
    return ret;
}



/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式协议栈事件
   @param    bt:事件
   @return
   @note     蓝牙底层事件,通过app层处理
*/
/*----------------------------------------------------------------------------*/
static int bt_hci_event_handler(struct bt_event *bt)
{
    //对应原来的蓝牙连接上断开处理函数  ,bt->value=reason
    log_debug("------------------------bt_hci_event_handler reason %x %x", bt->event, bt->value);

    UI_MSG_POST("bt_status:hci_event=%4", bt->event);
    if (bt_hci_event_filter(bt) == 0) {
        return 0;
    }

    switch (bt->event) {
    case HCI_EVENT_INQUIRY_COMPLETE:
        log_info(" HCI_EVENT_INQUIRY_COMPLETE \n");
        bt_hci_event_inquiry(bt);
        break;
    case HCI_EVENT_IO_CAPABILITY_REQUEST:
        log_info(" HCI_EVENT_IO_CAPABILITY_REQUEST \n");
        clock_add_set(BT_CONN_CLK);
        break;
    case HCI_EVENT_USER_CONFIRMATION_REQUEST:
        log_info(" HCI_EVENT_USER_CONFIRMATION_REQUEST \n");
        ///<可通过按键来确认是否配对 1：配对   0：取消
        bt_send_pair(1);
        clock_remove_set(BT_CONN_CLK);
        break;
    case HCI_EVENT_USER_PASSKEY_REQUEST:
        log_info(" HCI_EVENT_USER_PASSKEY_REQUEST \n");
        ///<可以开始输入6位passkey
        break;
    case HCI_EVENT_USER_PRESSKEY_NOTIFICATION:
        log_info(" HCI_EVENT_USER_PRESSKEY_NOTIFICATION %x\n", bt->value);
        ///<可用于显示输入passkey位置 value 0:start  1:enrer  2:earse   3:clear  4:complete
        break;
    case HCI_EVENT_PIN_CODE_REQUEST :
        log_info("HCI_EVENT_PIN_CODE_REQUEST  \n");
        bt_send_pair(1);
        break;
    case HCI_EVENT_VENDOR_NO_RECONN_ADDR :
        log_info("HCI_EVENT_VENDOR_NO_RECONN_ADDR \n");
        bt_hci_event_disconnect(bt) ;
        break;
    case HCI_EVENT_DISCONNECTION_COMPLETE :
        log_info("HCI_EVENT_DISCONNECTION_COMPLETE \n");
        if (bt->value == ERROR_CODE_PAIRING_NOT_ALLOWED) { //用于判断华为手表的断开情况
            UI_MSG_POST("bt_status:hci_value=%4", bt->value);
        }
        bt_hci_event_disconnect(bt) ;
        clock_remove_set(BT_CONN_CLK);
        break;
    case BTSTACK_EVENT_HCI_CONNECTIONS_DELETE:
    case HCI_EVENT_CONNECTION_COMPLETE:
        log_info(" HCI_EVENT_CONNECTION_COMPLETE \n");
        UI_MSG_POST("bt_status:hci_value=%4", bt->value);
        switch (bt->value) {
        case ERROR_CODE_SUCCESS :
            log_info("ERROR_CODE_SUCCESS  \n");
            bt_hci_event_connection(bt);
            break;
        case ERROR_CODE_PIN_OR_KEY_MISSING:
            log_info(" ERROR_CODE_PIN_OR_KEY_MISSING \n");
            bt_hci_event_linkkey_missing(bt);
        case ERROR_CODE_SYNCHRONOUS_CONNECTION_LIMIT_TO_A_DEVICE_EXCEEDED :
        case ERROR_CODE_CONNECTION_REJECTED_DUE_TO_LIMITED_RESOURCES:
        case ERROR_CODE_CONNECTION_REJECTED_DUE_TO_UNACCEPTABLE_BD_ADDR:
        case ERROR_CODE_CONNECTION_ACCEPT_TIMEOUT_EXCEEDED  :
        case ERROR_CODE_REMOTE_USER_TERMINATED_CONNECTION   :
        case ERROR_CODE_CONNECTION_TERMINATED_BY_LOCAL_HOST :
        case ERROR_CODE_AUTHENTICATION_FAILURE :
            bt_hci_event_disconnect(bt) ;
            break;
        case CUSTOM_BB_AUTO_CANCEL_PAGE:
            bt_wait_phone_connect_control(1);
            break;
        case ERROR_CODE_PAGE_TIMEOUT:
            log_info(" ERROR_CODE_PAGE_TIMEOUT \n");
            bt_hci_event_page_timeout(bt);
            break;
        case ERROR_CODE_CONNECTION_TIMEOUT:
            log_info(" ERROR_CODE_CONNECTION_TIMEOUT \n");
            bt_hci_event_connection_timeout(bt);
            break;
        case ERROR_CODE_ACL_CONNECTION_ALREADY_EXISTS  :
            log_info("ERROR_CODE_ACL_CONNECTION_ALREADY_EXISTS   \n");
            bt_hci_event_connection_exist(bt);
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
    return 0;
}



/*----------------------------------------------------------------------------*/
/**@brief    蓝牙后台事件过滤处理
   @param    event: 事件
   @return   0:不需要切换模式， 1:需要切换模式，2:通话导致需要切换
   @note
*/
/*----------------------------------------------------------------------------*/
int bt_background_event_handler_filter(struct sys_event *event)
{
    u8 ret = 0;
    u8 send = 0;
#if TCFG_BLUETOOTH_BACK_MODE
    if ((u32)event->arg == SYS_BT_EVENT_TYPE_CON_STATUS) {
        log_info("bt con event: %d \n", event->u.bt.event);
        switch (event->u.bt.event) {
        // 需要切换蓝牙的命令
        /* case BT_STATUS_A2DP_MEDIA_START: */
        /*     if (__this->sbc_packet_step != 0) { */
        /*         #<{(| log_info("sbc_packet_step : %d \n", __this->sbc_packet_step); |)}># */
        /*         break; */
        /*     } */
        /*  */
        case BT_STATUS_FIRST_DISCONNECT:
        case BT_STATUS_SECOND_DISCONNECT:
            //关机导致的断开不可以回去蓝牙，否则后台关机会有问题
            if (app_var.goto_poweroff_flag) {
                break;
            }
#if BACKGROUND_GOBACK
            ret = 1;
#else
            //判断断开的是sink设备，默认切换蓝牙
            if (event->u.bt.value) {
                /* ret = 1; */
                /* bt_status_disconnect_background(&event->u.bt); */
            }
            if (ret == 0) {
                bt_status_disconnect_background(&event->u.bt);
            }
#endif
            break;

        case BT_STATUS_SECOND_CONNECTED:
        case BT_STATUS_FIRST_CONNECTED:
#if BACKGROUND_GOBACK
            ret = 1;
#else
            bt_status_connect_background(&event->u.bt);
#endif

            if (__this->smartbox_watch_upgrade_flag) {
                extern void lmp_esco_rejust_establish(u8 value);
                lmp_esco_rejust_establish(1);
            }
            break;

        case BT_STATUS_START_CONNECTED:
#if BACKGROUND_GOBACK
            ret = 1;
#endif
            break;

        case  BT_STATUS_ENCRY_COMPLETE:
            break;
        case BT_STATUS_SCO_STATUS_CHANGE:
            ret = 1;
            break;
        case BT_STATUS_LAST_CALL_TYPE_CHANGE:
            bt_status_last_call_type_change(&event->u.bt);
        case BT_STATUS_VOICE_RECOGNITION:
        case BT_STATUS_PHONE_INCOME:
        case BT_STATUS_PHONE_NUMBER:
        /* case BT_STATUS_PHONE_MANUFACTURER: */
        case BT_STATUS_PHONE_OUT:
        case BT_STATUS_PHONE_ACTIVE:
            /* case BT_STATUS_PHONE_HANGUP: */
            //当手机app与手表传输过程中不能响应通话事件
            if (__this->smartbox_watch_upgrade_flag) {
                break;
            }
            ret = 2;
            break;
        // 不需要处理的命令
        case BT_STATUS_INIT_OK:
            bt_status_init_ok_background(&event->u.bt);
            break;
        case BT_STATUS_A2DP_MEDIA_STOP:
            bt_drop_a2dp_frame_stop();
        case BT_STATUS_CALL_VOL_CHANGE:
            break;
        // 按原方式处理的命令
        default:
            send = 1;
            u8 temp_flag = (u8) - 1;
            if (app_get_curr_task() == APP_SMARTBOX_ACTION_TASK) {
                //后台不清升级变量
                temp_flag = __this->smartbox_watch_upgrade_flag;
            }
            bt_connction_status_event_handler(&event->u.bt);
            if (temp_flag != (u8) - 1) {
                __this->smartbox_watch_upgrade_flag = temp_flag;
            }
            break;

        }
        if (!send) {
            UI_MSG_POST("bt_status:event=%4", event->u.bt.event);
        }
    } else if ((u32)event->arg == SYS_BT_EVENT_TYPE_HCI_STATUS) {
        /* log_info("bt hci event: %d \n", event->u.bt.event); */
        switch (event->u.bt.event) {
        case HCI_EVENT_IO_CAPABILITY_REQUEST:
            clock_add_set(BT_CONN_CLK);
#if BACKGROUND_GOBACK
            ret = 1;
#endif
            break;
        default:
            bt_hci_event_handler(&event->u.bt);
            break;
        }
    }
#if TCFG_USER_TWS_ENABLE
    else if (((u32)event->arg == SYS_BT_EVENT_FROM_TWS)) {
        /* log_info("bt tws event: %d \n", event->u.bt.event); */
        switch (event->u.bt.event) {
        /* case TWS_EVENT_CONNECTED: */
        /* ret = 1; */
        /* break; */
        default:
            bt_tws_connction_status_event_handler(&event->u.bt);
#if (TCFG_DEC2TWS_ENABLE)
            localtws_bt_event_deal(&event->u.bt);
#endif
            break;
        }
    }
#endif
    else if ((u32)event->arg == SYS_EVENT_FROM_CTRLER) {
        switch (event->u.bt.event) {
        case  BTCTRLER_EVENT_RESUME_REQ:
            log_info("-------   BTCTRLER_EVENT_RESUME_REQ \n");

            if (__this->smartbox_watch_upgrade_flag) {
                break;
            }

            ret = 2;
            break;
        }
    }
#endif
    return ret;
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙后台事件处理
   @param    event: 事件
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
int bt_background_event_handler(struct sys_event *event)
{
    int ret = bt_background_event_handler_filter(event);
    if (ret) {
        if (false == app_check_curr_task(APP_BT_TASK)) {
            __this->cmd_flag = 1;
            if (ret == 2) {
                __this->call_flag = 1;
            } else {
                __this->call_flag = 0;
            }
            app_task_switch_to(APP_BT_TASK);
            sys_event_notify(event);
        }
    }
    return 0;
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式按键响应处理
   @param    event: 按键
   @return   1:事件处理  0:事件无处理
   @note
*/
/*----------------------------------------------------------------------------*/
int bt_key_event_handler(struct sys_event *event)
{
    int ret = true;
    struct key_event *key = &event->u.key;
    int key_event = event->u.key.event;
    int key_value = event->u.key.value;


    if (bt_key_event_filter_before() == false) {
        return false;
    }

    if (bt_ui_key_event_filter(key_event) == true) { //判断为ui 按键直接到ui
        return true;
    }

    log_debug("bt key_event:%d %d %d %d\n", key_event, key->value, key->event, key->init);

    if (bt_key_event_filter_after(key_event) == true) {
        return true;
    }

    static int curr_page = 0;

    switch (key_event) {
#if 0
    case KEY_CHANGE_MODE:
        log_info("   KEY_CHANGE_MODE  \n");
        /*
                static int test = 0;
                test++;
                if(test %2){
                UI_MSG_POST("test1:a=%4,test2:bcd=%4,test3:efgh=%4,test4:hijkl=%4", 1,2,3,4);
                //消息交互测试
                break;
                }

        */
#if TCFG_UI_ENABLE
        ui_hide_curr_main();
        const u32 ui_page_list[] = {PAGE_0, PAGE_1, PAGE_2, PAGE_3, PAGE_7, PAGE_9};
        if (!key_value) {
            curr_page++;
            if (curr_page > (sizeof(ui_page_list) / sizeof(ui_page_list[0]) - 1)) {
                curr_page = 0;
            }
        } else {
            curr_page--;
            if (curr_page < 0) {
                curr_page = sizeof(ui_page_list) / sizeof(ui_page_list[0]) - 1;
            }
        }
        ui_show_main(ui_page_list[curr_page]);
#endif
        break;
#endif

    case  KEY_MUSIC_PP:
        log_info("   KEY_MUSIC_PP  \n");
        bt_key_music_pp();
        break;
    case  KEY_MUSIC_PREV:
        log_info("    KEY_MUSIC_PREV \n");
        bt_key_music_prev();
        break;
    case  KEY_MUSIC_NEXT:
        log_info("    KEY_MUSIC_NEXT \n");
        bt_key_music_next();
        break;
    case  KEY_VOL_UP:
        log_info("    KEY_VOL_UP \n");
        bt_key_vol_up();
        break;
    case  KEY_VOL_DOWN:
        log_info("    KEY_VOL_DOWN \n");
        bt_key_vol_down();
        break;
    case  KEY_CALL_LAST_NO:
        log_info("    KEY_CALL_LAST_NO \n");
        bt_key_call_last_on();
        break;
    case  KEY_CALL_HANG_UP:
        log_info("    KEY_CALL_HANG_UP \n");
        bt_key_call_hand_up();
        break;
    case  KEY_CALL_ANSWER:
        log_info("    KEY_CALL_ANSWER \n");
        bt_key_call_answer();
        break;
    case  KEY_OPEN_SIRI:
        log_info("    KEY_OPEN_SIRI \n");
        bt_key_call_siri();
        break;
    case  KEY_HID_CONTROL:
        log_info("    KEY_HID_CONTROL \n");
        bt_key_hid_control();
        break;
    case KEY_THIRD_CLICK:
        log_info("    KEY_THIRD_CLICK \n");
        bt_key_third_click(event);
        break;
    case KEY_LOW_LANTECY:
        log_info("    KEY_LOW_LANTECY \n");
        bt_key_low_lantecy();
        break;
    case  KEY_NULL:
        log_info("    KEY_NULL \n");
        ret = false;
#if TCFG_USER_TWS_ENABLE
        if ((u32)event->arg == KEY_EVENT_FROM_TWS) {
            break;
        }
#endif
        break;

    case KEY_HID_MODE_SWITCH:
        __this->hid_mode = !__this->hid_mode;
        printf("KEY_HID_TAKE_PICTURE, %d\n", __this->hid_mode);
        user_change_profile_mode(__this->hid_mode);///HID设备名称， 默认蓝牙名称追加_HID
        break;
    case KEY_HID_TAKE_PICTURE:
        if (__this->hid_mode) {
            printf("KEY_HID_TAKE_PICTURE\n");
            user_send_cmd_prepare(USER_CTRL_HID_BOTH, 0, NULL);
        }
        break;

    default:
        ret = false;
        break;
    }
#if (SMART_BOX_EN)
    extern int smartbox_bt_key_event_deal(u8 key_event, int ret);
    ret = smartbox_bt_key_event_deal(key_event, ret);
#endif
    return ret;
}


/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式协议栈状态事件、hci事件、对箱事件
   @param    event: 事件
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
int bt_sys_event_office(struct sys_event *event)
{
    if ((u32)event->arg == SYS_BT_EVENT_TYPE_CON_STATUS) {
        bt_connction_status_event_handler(&event->u.bt);
    } else if ((u32)event->arg == SYS_BT_EVENT_TYPE_HCI_STATUS) {
        bt_hci_event_handler(&event->u.bt);
    } else if ((u32)event->arg == SYS_BT_EVENT_FORM_SELF) {
        bt_reverb_status_change(&event->u.bt);
    }
#if TCFG_USER_TWS_ENABLE
    else if (((u32)event->arg == SYS_BT_EVENT_FROM_TWS)) {
        bt_tws_connction_status_event_handler(&event->u.bt);
    }
#endif
    else if (((u32)event->arg == SYS_BT_EVENT_FROM_KEY)) {
        switch (event->u.key.event) {
        case KEY_CHANGE_MODE:
            app_task_switch_next();
            break;
        }
    }
    return false;
}


/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式事件消息处理
   @param    event: 事件
   @return   1:处理完成,消息不需要进入common处理  0:common defult处理
   @note
*/
/*----------------------------------------------------------------------------*/
static int bt_sys_event_handler(struct sys_event *event)
{
    int ret = false;
    switch (event->type) {
    case SYS_KEY_EVENT:
        ret = bt_key_event_handler(event);
        break;
    case SYS_BT_EVENT:
        ret = bt_sys_event_office(event);
        break;
    case SYS_DEVICE_EVENT:
        break;
    default:
        return false;
    }
    // SYS_EVENT_HANDLER_SPECIFIC(event);
    return ret;
}



/*----------------------------------------------------------------------------*/
/**@brief    蓝牙模式
   @param
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
void app_bt_task()
{
    int res;
    int msg[32];
    ui_update_status(STATUS_EXIT_LOWPOWER);

    if (!__this->cmd_flag) { //蓝牙后台拉回蓝牙模式不播放提示音
        tone_play_by_path(tone_table[IDEX_TONE_BT_MODE], 1);
    }

    bt_task_start();

    while (1) {
        app_task_get_msg(msg, ARRAY_SIZE(msg), 1);

        switch (msg[0]) {
        case APP_MSG_SYS_EVENT:
            if (bt_sys_event_handler((struct sys_event *)(msg + 1)) == false) {
                app_default_event_deal((struct sys_event *)(&msg[1]));
            }
            break;
        default:
            break;
        }

        if (app_task_exitting()) {
            bt_task_close();
            __this->wait_exit = 1;
        }

        if (__this->wait_exit) { //开始退出
            if (!__this->exiting) { ///等待蓝牙退出处理完成
                return;
            }
        }
    }
}

// 耳机/音箱按键处理
void forward_esco_key_handler(u8 cmd, u8 *packet, u16 size)
{
    static u32 esco_volume;
    u32 temp = 0;

    switch (cmd) {
    case ATCMD_CMD_CHUP:
        log_info("KEY_CALL_HANG_UP \n");
        bt_key_call_hand_up();
        break;
        /* case ATCMD_CMD_VGS: */
        /*     if(packet[size - 3] != 0x3d) { */
        /*         temp = packet[size - 3]*0x100 | packet[size - 2]; */
        /*     }else { */
        /*      */
        /*         temp = packet[size - 2]; */
        /*     } */
        /*     if(temp > esco_volume) { //调大音量 */
        /*         bt_key_vol_up(); */
        /*      */
        /*     } */
        /*     if(temp < esco_volume) { //调小音量 */
        /*         bt_key_vol_down(); */
        /*     } */
        /*     esco_volume = temp; */
        /*     break; */

    }

}

u8 bt_app_exit_check()
{
    return	bt_app_switch_exit_check();
}

#else

void app_bt_task()
{
}

u8 bt_app_exit_check()
{
    return 1;
}

int bt_background_event_handler_filter(struct sys_event *event)
{
    return 0;
}
#endif
