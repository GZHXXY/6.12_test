#include "app_config.h"


#include "includes.h"
#include "ui/ui_api.h"
#include "system/includes.h"
#include "system/timer.h"
#include "asm/spi.h"
#include "clock_cfg.h"
#include "asm/mcpwm.h"
#include "ui/res_config.h"
#include "asm/imd.h"
#include "asm/imb.h"
#include "ui/ui_sys_param.h"


#define lcd_debug printf


static u8  backlight_status = 0;
static u8  lcd_sleep_in     = 0;
static volatile u8 is_lcd_busy = 0;
static struct lcd_spi_platform_data *spi_dat = NULL;
struct pwm_platform_data lcd_pwm_p_data;


#define __lcd ((struct _lcd_drive *)&lcd_drive)
#define __this ((struct imd_param *)lcd_drive.param)

/* extern u32 RAM1_SIZE ; */
extern __attribute__((weak)) unsigned RAM1_SIZE ;
extern void *malloc_vlt(size_t size);
extern void free_vlt(void *pv);

__attribute__((noinline)) u32 check_ram1_size(void)
{
    /* printf("check_ram1_size \n"); */
    return &RAM1_SIZE;
    /* return 1; */
}

// EN 控制
void lcd_en_ctrl(u8 val)
{
    if (spi_dat->pin_en == NO_CONFIG_PORT) {
        return;
    }
    gpio_direction_output(spi_dat->pin_en, val);
}

// BL 控制
void lcd_bl_ctrl(u8 val)
{
    if (spi_dat->pin_bl == NO_CONFIG_PORT) {
        return;
    }
    gpio_direction_output(spi_dat->pin_bl, val);
}



// CS 控制
static void spi_cs_ctrl(u8 val)
{
    if (spi_dat->pin_cs == NO_CONFIG_PORT) {
        return;
    }
    gpio_direction_output(spi_dat->pin_cs, val);
}

// DC 控制
static void spi_dc_ctrl(u8 val)
{
    if (spi_dat->pin_dc == NO_CONFIG_PORT) {
        return;
    }
    gpio_direction_output(spi_dat->pin_dc, val);
}

// TE 控制
static int spi_te_stat()
{
    if (spi_dat->pin_te == NO_CONFIG_PORT) {
        return -1;
    }
    gpio_set_pull_up(spi_dat->pin_te, 1);
    gpio_set_pull_down(spi_dat->pin_te, 0);
    gpio_direction_input(spi_dat->pin_te);
    gpio_set_die(spi_dat->pin_te, 1);

    return gpio_read(spi_dat->pin_te);
}

/*$PAGE*/
/*
 *********************************************************************************************************
 *                                       LCD DEVICE RESET
 *
 * Description: LCD 设备复位
 *
 * Arguments  : none
 *
 * Returns    : none
 *
 * Notes      : 1、判断是否在屏驱有重新定义LCD复位函数，
 * 					是使用屏驱上定义的LCD复位函数，
 * 					否使用GPIO控制板级配置的LCD复位IO
 *********************************************************************************************************
 */

static void lcd_reset()
{
    if (__lcd->reset) {
        __lcd->reset();
    } else {
        gpio_direction_output(spi_dat->pin_reset, 1);
        os_time_dly(10);
        gpio_direction_output(spi_dat->pin_reset, 0);
        os_time_dly(10);
        gpio_direction_output(spi_dat->pin_reset, 1);
        os_time_dly(10);
    }
}



/*$PAGE*/
/*
 *********************************************************************************************************
 *                                       LCD BACKLIGHT CONTROL
 *
 * Description: LCD 背光控制
 *
 * Arguments  : on LCD 背光开关标志，0为关，其它值为开
 *
 * Returns    : none
 *
 * Notes      : 1、判断是否在屏驱有重新定义背光控制函数，
 * 					是使用屏驱上定义的背光控制函数，
 * 					否使用GPIO控制板级配置的背光IO
 *
 *              2、配置背光状态标志，打开为true，关闭为false
 *********************************************************************************************************
 */

static void lcd_mcpwm_init()
{
#if (TCFG_BACKLIGHT_PWM_MODE == 2)
    extern void mcpwm_init(struct pwm_platform_data * arg);
    lcd_pwm_p_data.pwm_aligned_mode = pwm_edge_aligned;         //边沿对齐
    lcd_pwm_p_data.pwm_ch_num = pwm_ch0;                        //通道
    lcd_pwm_p_data.frequency = 10000;                           //Hz
    lcd_pwm_p_data.duty = 10000;                                //占空比
    lcd_pwm_p_data.h_pin = spi_dat->pin_bl;                     //任意引脚
    lcd_pwm_p_data.l_pin = -1;                                  //任意引脚,不需要就填-1
    lcd_pwm_p_data.complementary_en = 1;                        //两个引脚的波形, 0: 同步,  1: 互补，互补波形的占空比体现在H引脚上
    mcpwm_init(&lcd_pwm_p_data);
#endif
}

int lcd_backlight_ctrl(u8 percent)
{
    if (__lcd->backlight_ctrl) {
        __lcd->backlight_ctrl(percent);
    } else if (spi_dat->pin_bl != -1) {
#if (TCFG_BACKLIGHT_PWM_MODE == 0)
        if (percent > 0) {
            gpio_direction_output(spi_dat->pin_bl, 1);
        } else {
            gpio_direction_output(spi_dat->pin_bl, 0);
        }
#elif (TCFG_BACKLIGHT_PWM_MODE == 1)
        //注意：duty不能大于prd，并且prd和duty是非标准非线性的，建议用示波器看着来调
        extern int pwm_led_output_clk(u8 gpio, u8 prd, u8 duty);
        if (percent) {
            u32 light_level = (percent + 9) / 10;
            if (light_level > 10) {
                printf("lcd pwm full\n");
                light_level = 10;
            }
            pwm_led_output_clk(spi_dat->pin_bl, 10, light_level);
        } else {
            pwm_led_output_clk(spi_dat->pin_bl, 10, 0);
        }
#elif (TCFG_BACKLIGHT_PWM_MODE == 2)
        if (percent) {
            u32 pwm_duty = percent * 100;
            if (pwm_duty > 10000) {
                pwm_duty = 10000;
                printf("lcd pwm full\n");
            }
            mcpwm_set_duty(lcd_pwm_p_data.pwm_ch_num, pwm_duty);
        } else {
            mcpwm_set_duty(lcd_pwm_p_data.pwm_ch_num, 0);
        }
#endif
    } else {
        backlight_status = false;
        return -1;
    }

    if (percent) {
        backlight_status = true;
    } else {
        backlight_status = false;
    }

    return 0;
}


int lcd_power_ctrl(u8 on)
{
    if (__lcd->power_ctrl) {
        __lcd->power_ctrl(on);
    }

    return 0;
}


struct lcd_spi_platform_data *lcd_get_platform_data()
{
    return spi_dat;
}

/*$PAGE*/
/*
 *********************************************************************************************************
 *                                       LCD DEVICE INIT
 *
 * Description: LCD 设备初始化
 *
 * Arguments  : *p 板级配置的 LCD SPI 信息
 *
 * Returns    : 0 初始化成功
 * 				-1 初始化失败
 *
 * Notes      : 1、判断是否在板级文件配置SPI，是继续，否进入断言，
 *
 *              2、配置SPI可操作IO给IMD操作
 *
 *              3、LCD设备复位
 *
 *              4、SPI模块初始化，IMD模块初始化
 *********************************************************************************************************
 */

int lcd_init(void *p)
{
    lcd_debug("lcd_init ...\n");
    int err = 0;
    struct spi_api *imd_spi = NULL;
    struct ui_devices_cfg *cfg = (struct ui_devices_cfg *)p;
    spi_dat = (struct lcd_spi_platform_data *)cfg->private_data;


    ASSERT(spi_dat, "Error! spi io not config");
    lcd_debug("spi pin rest:%d, cs:%d, dc:%d, en:%d, spi:%d\n", \
              spi_dat->pin_reset, spi_dat->pin_cs, spi_dat->pin_dc, spi_dat->pin_en, spi_dat->spi_cfg);


    /* 把 CS、DC IO 的控制配置到IMD */
    imd_spi = get_spi_api_config_handler();
    imd_spi->cs_ctrl = spi_cs_ctrl;
    imd_spi->dc_ctrl = spi_dc_ctrl;
    imd_spi->te_stat = spi_te_stat;

    /* 如果有使能IO，设置使能IO输出高电平 */
    if (spi_dat->pin_en != -1) {
        gpio_direction_output(spi_dat->pin_en, 1);
    }

    lcd_reset(); /* lcd复位 */

    /* 初始化imd和硬件SPI等 */
    /* imd_lcd_init(NULL, 0); */
    imd_lcd_init(__this->in_format, NULL, 0);

    /* y_printf("RAM1_SIZE:%d \n", &RAM1_SIZE); */
    if (check_ram1_size()) {
        /* y_printf("%s,%d \n", __func__, __LINE__); */
        imb_set_memory_func(malloc_vlt, free_vlt);
    }

    lcd_mcpwm_init();
    /* 把屏幕刷成纯色 */
    /* lcd_clear_screen(0xff0000); */
    return 0;
}


/*$PAGE*/
/*
 *********************************************************************************************************
 *                                       GET LCD DEVICE INFO
 *
 * Description: 获取 LCD 设备信息
 *
 * Arguments  : *info LCD 设备信息缓存结构体，根据结构体内容赋值即可
 *
 * Returns    : 0 获取成功
 * 				-1 获取失败
 *
 * Notes      : 1、根据参数结构体的内容，将LCD对应信息赋值给结构体元素
 *********************************************************************************************************
 */

static int lcd_get_screen_info(struct lcd_info *info)
{
    /* imb的宽高 */
    info->width = __this->in_width;
    info->height = __this->in_height;

    /* imb的输出格式 */
    info->color_format = __this->in_format;//OUTPUT_FORMAT_RGB565;
    if (info->color_format == OUTPUT_FORMAT_RGB565) {
        info->stride = (info->width * 2 + 3) / 4 * 4;
    } else if (info->color_format == OUTPUT_FORMAT_RGB888) {
        info->stride = (info->width * 3 + 3) / 4 * 4;
    }

    /* 屏幕类型 */
    info->interface = __this->lcd_type;

    /* 对齐 */
    info->col_align = __lcd->column_addr_align;
    info->row_align = __lcd->row_addr_align;

    /* 背光状态 */
    info->bl_status = backlight_status;
    info->buf_num = __this->buffer_num;

    ASSERT(info->col_align, " = 0, lcd driver column address align error, default value is 1");
    ASSERT(info->row_align, " = 0, lcd driver row address align error, default value is 1");

    return 0;
}


/*$PAGE*/
/*
 *********************************************************************************************************
 *                                       MALLOC DISPLAY BUFFER
 *
 * Description: 申请 LCD 显存 buffer
 *
 * Arguments  : **buf 保存显存buffer指针
 * 				*size 保存显存buffer大小
 *
 * Returns    : 0 成功
 * 				-1 失败
 *
 * Notes      : 1、根据LCD驱动中配置的显存大小和数量申请显存BUFFER
 *
 *				2、将显存buffer指针赋值给参数**buf，显存buffer大小赋值给参数*size
 *
 *				注意：buffer默认是lock状态，此时不能推屏，需由UI框架获取并写入数据后才能推屏
 *********************************************************************************************************
 */

static int lcd_buffer_malloc(u8 **buf, u32 *size)
{
    /* int buf_size = ((__this->lcd_width * __this->lcd_height * 2) + 3) / 4 * 4; */
    int buf_size = (__this->buffer_size + 3) / 4 * 4;	// 把buffer大小做四字节对齐

    if (check_ram1_size() == 0) {
        *buf = (u8 *)malloc(buf_size * __this->buffer_num);
    } else {
        *buf = (u8 *)malloc_vlt(buf_size * __this->buffer_num);
    }

    /* *buf = (u8 *)malloc(__this->buffer_size * __this->buffer_num); */

    if (!buf) {
        // 如果buffer申请失败
        *buf = NULL;
        *size = 0;
        return -1;
    }

    /* *size = __this->buffer_size; */
    *size = buf_size * __this->buffer_num;

    return 0;
}



/*$PAGE*/
/*
 *********************************************************************************************************
 *                                       FREE DISPLAY BUFFER
 *
 * Description: 释放 LCD 显存 buffer
 *
 * Arguments  : *buf 显存buffer指针
 *
 * Returns    : 0 成功
 * 				-1 失败
 *
 * Notes      : 1、使用memory API 释放显存buffer
 *********************************************************************************************************
 */

static int lcd_buffer_free(u8 *buf)
{
    if (buf) {
        if (check_ram1_size() == 0) {
            free(buf);
        } else {
            free_vlt(buf);
        }
        buf = NULL;
    }
    return 0;
}


/*$PAGE*/
/*
 *********************************************************************************************************
 *                                       LCD DRAW BUFFER
 *
 * Description: 把显存 buf 推送到屏幕
 *
 * Arguments  : *buf 显存buffer指针
 * 				len 显存buffer的数据量
 * 				wait 是否等待
 *
 * Returns    : 0 成功
 * 				-1 失败
 *
 * Notes      : 1、使用 IMD 模块将显存buffer推给屏幕
 *********************************************************************************************************
 */

static int lcd_draw(u8 *buf, u32 len, u8 wait)
{
    lcd_buffer_put(buf);
    return 0;
}


/*$PAGE*/
/*
 *********************************************************************************************************
 *                                       LCD DRAW PAGE
 *
 * Description: 将UI页面buffer推给屏幕
 *
 * Arguments  : *buf 页面缓存buffer指针
 *
 * Returns    : 0 成功
 * 				-1 失败
 *
 * Notes      :
 *********************************************************************************************************
 */

static int lcd_draw_page(u8 *buf, u8 page_star, u8 page_len)
{
    // 绘制页面功能暂时没有
    return 0;
}



/*$PAGE*/
/*
 *********************************************************************************************************
 *                                       GET LCD BACKLIGHT STATUS
 *
 * Description: 获取 LCD 背光状态
 *
 * Arguments  : none
 *
 * Returns    : 0 背光熄灭
 * 				1 背光点亮
 *
 * Notes      :
 *********************************************************************************************************
 */

int lcd_backlight_status()
{
    return backlight_status;
}

/*
 *********************************************************************************************************
 *                                       GET LCD BACKLIGHT STATUS
 *
 * Description: 获取 LCD sleep状态
 *
 * Arguments  : none
 *
 * Returns    : 0 sleep out
 * 				1 sleep in
 *
 * Notes      :
 *********************************************************************************************************
 */

int lcd_sleep_status()
{
    return lcd_sleep_in;
}

// 触摸屏sleep处理
__attribute__((weak)) void ctp_enter_sleep(void)
{
}
__attribute__((weak)) void ctp_exit_sleep(void)
{
}

/*$PAGE*/
/*
 *********************************************************************************************************
 *                                       LCD SLEEP CONTROL
 *
 * Description: LCD 休眠控制
 *
 * Arguments  : enter 是否进入休眠，true 进入休眠，false 退出休眠
 *
 * Returns    : 0 成功
 * 				-1 失败
 *
 * Notes      : 1、判断 LCD 是否正在使用，是等待使用结束，否进入下一步
 *
 * 				2、enter是否进入休眠，是使用LCD休眠函数进入休眠状态，否使用LCD退出休眠函数退出休眠状态
 *
 * 				3、lcd_sleep_in 记录LCD的休眠状态
 *********************************************************************************************************
 */

int lcd_sleep_ctrl(u8 enter)
{
    if ((!!enter) == lcd_sleep_in) {
        return -1;
    }
    while (is_lcd_busy);
    is_lcd_busy = 0x11;

    if (enter) {
#if TCFG_TP_SLEEP_EN
        ctp_enter_sleep();
#endif /* #if TCFG_TP_SLEEP_EN */
        if (__lcd->entersleep) {
            __lcd->entersleep();
            lcd_sleep_in = true;
        }
        clock_remove_set(DEC_UI_CLK);
    } else {
        clock_add_set(DEC_UI_CLK);
#if TCFG_TP_SLEEP_EN
        ctp_exit_sleep();
#endif /* #if TCFG_TP_SLEEP_EN */
        if (__lcd->exitsleep) {
            __lcd->exitsleep();
            lcd_sleep_in = false;
        }
    }

    is_lcd_busy = 0;
    return 0;
}


/*$PAGE*/
/*
 *********************************************************************************************************
 *                                       GET LCD DRIVE HANDLER
 *
 * Description: 获取LCD驱动句柄
 *
 * Arguments  : none
 *
 * Returns    : struct lcd_interface* LCD驱动接口句柄
 *
 * Notes      : 1、从LCD接口列表中找到LCD接口句柄并返回
 *********************************************************************************************************
 */

struct lcd_interface *lcd_get_hdl()
{
    struct lcd_interface *p;

    ASSERT(lcd_interface_begin != lcd_interface_end, "don't find lcd interface!");
    for (p = lcd_interface_begin; p < lcd_interface_end; p++) {
        return p;
    }
    return NULL;
}

REGISTER_LCD_INTERFACE(lcd) = {
    .init = lcd_init,
    .draw = lcd_draw,
    .get_screen_info	= lcd_get_screen_info,
    .buffer_malloc		= lcd_buffer_malloc,
    .buffer_free		= lcd_buffer_free,
    .backlight_ctrl		= lcd_backlight_ctrl,
    .power_ctrl		    = lcd_power_ctrl,
    .draw_page = NULL,

    // 下面部分直接调用imd的函数
    .set_draw_area	= lcd_set_area,
    .clear_screen	= lcd_clear_screen,

    // br28新加
    .init_buffer	= lcd_buffer_init,
    .get_buffer		= lcd_buffer_get,
    .release_buffer	= lcd_buffer_release,
    .set_buffer_pending = lcd_buffer_pending,
    .draw_area = lcd_draw_area,
};


static u8 lcd_idle_query(void)
{
    return !is_lcd_busy;
}

REGISTER_LP_TARGET(lcd_lp_target) = {
    .name = "lcd",
    .is_idle = lcd_idle_query,
};




