#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include "audio_type.h"

/*
 *  板级配置选择
 */
#define CONFIG_BOARD_AC701N_DEMO
// #define CONFIG_BOARD_AC7012A_DEMO
// #define CONFIG_BOARD_AC701N_LVGL_DEMO

#include "board_ac701n_demo/board_ac701n_demo_cfg.h"
#include "board_ac7012a_demo/board_ac7012a_demo_cfg.h"
#include "board_ac701n_lvgl_demo/board_ac701n_lvgl_demo_cfg.h"

//#define  DUT_AUDIO_DAC_LDO_VOLT   				DACVDD_LDO_2_80V

#endif
