#include "system/includes.h"
#include "app_config.h"
#include "asm/pwm_led.h"
#include "tone_player.h"
#include "ui_manage.h"
#include "app_main.h"
#include "app_task.h"
#include "asm/charge.h"
#include "app_power_manage.h"
#include "app_charge.h"
#include "user_cfg.h"
#include "power_on.h"
#include "bt.h"
#include "vm.h"


#define LOG_TAG_CONST       APP
#define LOG_TAG             "[APP]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"



APP_VAR app_var;


void app_entry_idle()
{
    app_task_switch_to(APP_IDLE_TASK);
}



void app_task_loop()
{
    while (1) {
        switch (app_curr_task) {
        case APP_POWERON_TASK:
            log_info("APP_POWERON_TASK \n");
            app_poweron_task();
            break;
        case APP_POWEROFF_TASK:
            log_info("APP_POWEROFF_TASK \n");
            app_poweroff_task();
            break;
        case APP_BT_TASK:
            log_info("APP_BT_TASK \n");
            app_bt_task();
            break;
        case APP_MUSIC_TASK:
            log_info("APP_MUSIC_TASK \n");
            app_music_task();
            break;
        case APP_RTC_TASK:
            log_info("APP_RTC_TASK \n");
            app_rtc_task();
            break;
        case APP_PC_TASK:
            log_info("APP_PC_TASK \n");
            app_pc_task();
            break;
        case APP_WATCH_UPDATE_TASK:
            log_info(" APP_WATCH_UPDATE_TASK \n");
            app_watch_ui_updata_task();
            break;
        case APP_SMARTBOX_ACTION_TASK:
            log_info("APP_SMARTBOX_ACTION_TASK \n");
#ifdef CONFIG_APP_BT_ENABLE
            app_smartbox_task();
#endif
            break;
        case APP_IDLE_TASK:
            log_info("APP_IDLE_TASK \n");
            app_idle_task();
            break;
        }
        app_task_clear_key_msg();//清理按键消息
        //检查整理VM
        vm_check_all(0);
    }
}

void mem_printf(void *priv)
{
    mem_stats();
}

void app_main()
{
    log_info("app_main \n");

#if TCFG_CHARGE_ENABLE
    set_charge_event_flag(1);
#endif

    app_var.start_time = timer_get_ms();

    if (get_charge_online_flag()) {

        app_var.poweron_charge = 1;

#if (TCFG_SYS_LVD_EN == 1)
        vbat_check_init();
#endif
        app_curr_task = APP_IDLE_TASK;
    } else {

#if SOUNDCARD_ENABLE
        soundcard_peripheral_init();
#endif

#if TCFG_NORFLASH_DEV_ENABLE
        extern void ex_flash_file_download_init(void);
        ex_flash_file_download_init();
#endif
        /* endless_loop_debug_int(); */
        ui_manage_init();
        ui_update_status(STATUS_POWERON);

        if (0) {
            /* app_curr_task = APP_WATCH_UPDATE_TASK; */
            app_curr_task = APP_SMARTBOX_ACTION_TASK;
        } else {
            app_curr_task = APP_POWERON_TASK;
        }
    }
    /* sys_timer_add(NULL, mem_printf, 1000); */
    app_task_loop();
}



