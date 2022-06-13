/**@file  		power_api.c
* @brief        电源应用流程
* @details		HW API
* @author		JL
* @date     	2021-11-30
* @version  	V1.0
* @copyright    Copyright:(c)JIELI  2011-2020  @ , All Rights Reserved.
 */
#include "asm/power_interface.h"
#include "app_config.h"




#if (TCFG_LOWPOWER_LOWPOWER_SEL == SLEEP_EN)


static enum LOW_POWER_LEVEL power_app_level(void)
{
    return LOW_POWER_MODE_SLEEP;
}

static u8 power_app_idle(void)
{
    return 1;
}

REGISTER_LP_TARGET(power_app_lp_target) = {
    .name = "power_app",
    .level = power_app_level,
    .is_idle = power_app_idle,
};

#endif


