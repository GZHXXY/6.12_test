/*--------------------------------------------------------------------------*/
/**@file    UI_common.c
   @brief   UI 显示公共函数
   @details
   @author  bingquan Cai
   @date    2012-8-30
   @note    AC319N
*/
/*----------------------------------------------------------------------------*/
#include "app_config.h"
#include "includes.h"
/* #include "ui/ui_api.h" */
/* #include "ui/ui.h" */
#include "typedef.h"
#include "clock_cfg.h"
#include "app_task.h"
#include "key_event_deal.h"

#include "lvgl.h"
#include "lv_port_disp.h"

#include "lv_demo_conf.h"

#if LVGL_TEST_ENABLE

#define UI_TASK_NAME 	"ui"

void lv_demo_widgets(void);
void lv_demo_stress(void);
void lv_demo_music(void);

#if LV_USE_LOG && LV_LOG_PRINTF
static void lv_rt_log(const char *buf)
{
    printf(buf);
}
#endif

static void lvgl_test_tick(void *param)
{
    lv_tick_inc(2);
}

static void lvgl_task(void *p)
{
    int msg[32];
    int ret;

#if LV_USE_LOG && LV_LOG_PRINTF
    lv_log_register_print_cb(lv_rt_log);
#endif

    lv_init();

    lv_port_disp_init(p);

    sys_s_hi_timer_add(NULL, lvgl_test_tick, 2);

#if LV_USE_DEMO_WIDGETS
    lv_demo_widgets();
#endif /* #if LV_USE_DEMO_WIDGETS */

#if LV_USE_DEMO_STRESS
    lv_demo_stress();
#endif /* #if LV_USE_DEMO_STRESS */

#if LV_USE_DEMO_MUSIC
    lv_demo_music();
#endif /* #if LV_USE_DEMO_MUSIC */

    while (1) {
        ret = os_taskq_accept(ARRAY_SIZE(msg), msg);
        if (ret == OS_TASKQ) {
            // msg deal
        }
        lv_task_handler();
    }
}

int lvgl_test_init(void *param)
{
    int err = 0;
    clock_add_set(DEC_UI_CLK);
    err = task_create(lvgl_task, param, UI_TASK_NAME);
    if (err) {
        r_printf("ui task create err:%d \n", err);
    }
    return err;
}

#endif /* #if LVGL_TEST_ENABLE */

