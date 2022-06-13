/**@file  		power_api.h
* @brief        pmu wakeup
* @details
* @author
* @date     	2021-8-26
* @version  	V1.0
* @copyright    Copyright:(c)JIELI  2011-2020  @ , All Rights Reserved.
 */

#ifndef __LP_WAKEUP_H__
#define __LP_WAKEUP_H__

//=========================唤醒参数配置==================================
struct port_wakeup {
    u8 pullup_down_enable;    //上下拉是否使能
    u8 edge;       			  //唤醒边沿条件
    u8 filter;				  //滤波参数，数字io输入没有滤波可配制
    u8 iomap;      			  //唤醒io

    //delete
    u8 both_edge;
};

#define MAX_WAKEUP_PORT     12  //最大同时支持数字io输入个数
#define MAX_WAKEUP_ANA_PORT 3   //最大同时支持模拟io输入个数

struct wakeup_param {
    //数字io输入
    const struct port_wakeup *port[MAX_WAKEUP_PORT];
    //模拟io输入
    const struct port_wakeup *aport[MAX_WAKEUP_ANA_PORT];
};

//edge
#define RISING_EDGE         0
#define FALLING_EDGE        1
#define BOTH_EDGE           2

//filter
enum {
    PORT_FLT_NULL = 0,
    PORT_FLT_256us,
    PORT_FLT_512us,
    PORT_FLT_1ms,
    PORT_FLT_2ms,
    PORT_FLT_4ms,
    PORT_FLT_8ms,
    PORT_FLT_16ms,
};

//=========================唤醒接口==================================
void power_wakeup_index_enable(u8 index);

void power_wakeup_index_disable(u8 index);

void power_wakeup_disable_with_port(u8 port);

void power_wakeup_enable_with_port(u8 port);

void power_wakeup_set_edge(u8 port_io, u8 edge);

void power_wakeup_init(const struct wakeup_param *param);

u32 get_wakeup_source(void);

u32 is_ldo5v_wakeup(void);

void port_edge_wkup_set_callback(void (*wakeup_callback)(u8 index, u8 gpio));

void aport_edge_wkup_set_callback(void (*wakeup_callback)(u8 index, u8 gpio, u8 edge));





#endif
