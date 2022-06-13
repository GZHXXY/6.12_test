#include "app_config.h"

#ifdef CONFIG_BOARD_AC7012A_DEMO

#include "system/includes.h"
#include "media/includes.h"
#include "asm/sdmmc.h"
#include "asm/chargestore.h"
#include "asm/charge.h"
#include "asm/pwm_led.h"
#include "tone_player.h"
#include "audio_config.h"
#include "gSensor/gSensor_manage.h"
#include "hrSensor_manage.h"
#include "key_event_deal.h"
#include "user_cfg.h"
#include "norflash_sfc.h"
#include "ui/ui_api.h"
#include "dev_manager.h"
#include "asm/spi.h"
#include "ui_manage.h"
#include "spi/nor_fs.h"
#include "fs/virfat_flash.h"
#include "norflash_sfc.h"
#include "asm/power/power_port.h"
#include "rtc/alarm.h"
#include "data_export.h"
#include "data_export_to_file.h"


#define LOG_TAG_CONST       BOARD
#define LOG_TAG             "[BOARD]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

void board_power_init(void);

/*各个状态下默认的闪灯方式和提示音设置，如果USER_CFG中设置了USE_CONFIG_STATUS_SETTING为1，则会从配置文件读取对应的配置来填充改结构体*/
STATUS_CONFIG status_config = {
    //灯状态设置
    .led = {
        .charge_start  = PWM_LED1_ON,
        .charge_full   = PWM_LED0_ON,
        .power_on      = PWM_LED0_ON,
        .power_off     = PWM_LED1_FLASH_THREE,
        .lowpower      = PWM_LED1_SLOW_FLASH,
        .max_vol       = PWM_LED_NULL,
        .phone_in      = PWM_LED_NULL,
        .phone_out     = PWM_LED_NULL,
        .phone_activ   = PWM_LED_NULL,
        .bt_init_ok    = PWM_LED0_LED1_SLOW_FLASH,
        .bt_connect_ok = PWM_LED0_ONE_FLASH_5S,
        .bt_disconnect = PWM_LED0_LED1_FAST_FLASH,
        .tws_connect_ok = PWM_LED0_LED1_FAST_FLASH,
        .tws_disconnect = PWM_LED0_LED1_SLOW_FLASH,
    },
    //提示音设置
    .tone = {
        .charge_start  = IDEX_TONE_NONE,
        .charge_full   = IDEX_TONE_NONE,
        .power_on      = IDEX_TONE_POWER_ON,
        .power_off     = IDEX_TONE_POWER_OFF,
        .lowpower      = IDEX_TONE_LOW_POWER,
        .max_vol       = IDEX_TONE_MAX_VOL,
        .phone_in      = IDEX_TONE_NONE,
        .phone_out     = IDEX_TONE_NONE,
        .phone_activ   = IDEX_TONE_NONE,
        .bt_init_ok    = IDEX_TONE_BT_MODE,
        .bt_connect_ok = IDEX_TONE_BT_CONN,
        .bt_disconnect = IDEX_TONE_BT_DISCONN,
        .tws_connect_ok   = IDEX_TONE_TWS_CONN,
        .tws_disconnect   = IDEX_TONE_TWS_DISCONN,
    }
};

#define __this (&status_config)


// *INDENT-OFF*
/************************** UART config****************************/
#if TCFG_UART0_ENABLE
UART0_PLATFORM_DATA_BEGIN(uart0_data)
    .tx_pin = TCFG_UART0_TX_PORT,                             //串口打印TX引脚选择
    .rx_pin = TCFG_UART0_RX_PORT,                             //串口打印RX引脚选择
    .baudrate = TCFG_UART0_BAUDRATE,                          //串口波特率

    .flags = UART_DEBUG,                                      //串口用来打印需要把改参数设置为UART_DEBUG
UART0_PLATFORM_DATA_END()
#endif //TCFG_UART0_ENABLE


#define SDX_POWER_ALONE		1 //单独电源

#if SDX_POWER_ALONE
static u8 sdx_power_enable = 0xff;
void sd_set_power(u8 enable)
{
	/* enable = !!enable; */
	// y_printf("sd_set_power:%d, %d \n", sdx_power_enable, enable);
	if(sdx_power_enable == enable){
		return;
	}
	sdpg_config(enable);
	sdx_power_enable = enable;
}
#endif /* #if SDX_POWER_ALONE */

/************************** CHARGE config****************************/
#if TCFG_CHARGE_ENABLE
CHARGE_PLATFORM_DATA_BEGIN(charge_data)
    .charge_en              = TCFG_CHARGE_ENABLE,              //内置充电使能
    .charge_poweron_en      = TCFG_CHARGE_POWERON_ENABLE,      //是否支持充电开机
    .charge_full_V          = TCFG_CHARGE_FULL_V,              //充电截止电压
    .charge_full_mA			= TCFG_CHARGE_FULL_MA,             //充电截止电流
    .charge_mA				= TCFG_CHARGE_MA,                  //恒流充电电流
    .charge_trickle_mA		= TCFG_CHARGE_TRICKLE_MA,          //涓流充电电流
/*ldo5v拔出过滤值，过滤时间 = (filter*2 + 20)ms,ldoin<0.6V且时间大于过滤时间才认为拔出
 对于充满直接从5V掉到0V的充电仓，该值必须设置成0，对于充满由5V先掉到0V之后再升压到xV的
 充电仓，需要根据实际情况设置该值大小*/
	.ldo5v_off_filter		= 100,
    .ldo5v_on_filter        = 50,
    .ldo5v_keep_filter      = 220,
    .ldo5v_pulldown_lvl     = CHARGE_PULLDOWN_200K,            //下拉电阻档位选择
    .ldo5v_pulldown_keep    = 1,
//1、对于自动升压充电舱,若充电舱需要更大的负载才能检测到插入时，请将该变量置1,并且根据需求配置下拉电阻档位
//2、对于按键升压,并且是通过上拉电阻去提供维持电压的舱,请将该变量设置1,并且根据舱的上拉配置下拉需要的电阻挡位
//3、对于常5V的舱,可将改变量设为0,省功耗
//4、为LDOIN防止被误触发唤醒,可设置为200k下拉
	.ldo5v_pulldown_en		= 1,
CHARGE_PLATFORM_DATA_END()
#endif//TCFG_CHARGE_ENABLE

/************************** chargestore config****************************/
#if TCFG_CHARGESTORE_ENABLE || TCFG_TEST_BOX_ENABLE || TCFG_ANC_BOX_ENABLE
CHARGESTORE_PLATFORM_DATA_BEGIN(chargestore_data)
    .io_port                = TCFG_CHARGESTORE_PORT,
CHARGESTORE_PLATFORM_DATA_END()
#endif

/************************** ADC ****************************/
#if TCFG_AUDIO_ADC_ENABLE

#ifndef TCFG_AUDIO_MIC0_BIAS_EN
#define TCFG_AUDIO_MIC0_BIAS_EN             0
#endif/*TCFG_AUDIO_MIC0_BIAS_EN*/
#ifndef TCFG_AUDIO_MIC1_BIAS_EN
#define TCFG_AUDIO_MIC1_BIAS_EN             0
#endif/*TCFG_AUDIO_MIC1_BIAS_EN*/
#ifndef TCFG_AUDIO_MIC2_BIAS_EN
#define TCFG_AUDIO_MIC2_BIAS_EN             0
#endif/*TCFG_AUDIO_MIC2_BIAS_EN*/
#ifndef TCFG_AUDIO_MIC3_BIAS_EN
#define TCFG_AUDIO_MIC3_BIAS_EN             0
#endif/*TCFG_AUDIO_MIC3_BIAS_EN*/
#ifndef TCFG_AUDIO_MIC_LDO_EN
#define TCFG_AUDIO_MIC_LDO_EN               0
#endif/*TCFG_AUDIO_MIC_LDO_EN*/

const struct ladc_port ladc_list[] = {
	{// 0
		.channel = TCFG_AUDIO_ADC_LINE_CHA,
	},
	// total must < 4
};
struct adc_platform_data adc_data = {
	.mic_channel    = TCFG_AUDIO_ADC_MIC_CHA,                   //MIC通道选择，对于693x，MIC只有一个通道，固定选择右声道
/*MIC LDO电流档位设置：
    0:0.625ua    1:1.25ua    2:1.875ua    3:2.5ua*/
	.mic_ldo_isel   = TCFG_AUDIO_ADC_LD0_SEL,
/*mic_mode 工作模式定义
    #define AUDIO_MIC_CAP_MODE          0   //单端隔直电容模式
    #define AUDIO_MIC_CAP_DIFF_MODE     1   //差分隔直电容模式
    #define AUDIO_MIC_CAPLESS_MODE      2   //单端省电容模式
*/
    .mic_mode = TCFG_AUDIO_MIC_MODE,
    .mic1_mode = TCFG_AUDIO_MIC1_MODE,
    .mic2_mode = TCFG_AUDIO_MIC2_MODE,
    .mic3_mode = TCFG_AUDIO_MIC3_MODE,

    .mic_bias_inside = TCFG_AUDIO_MIC0_BIAS_EN,
    .mic1_bias_inside = TCFG_AUDIO_MIC1_BIAS_EN,
    .mic2_bias_inside = TCFG_AUDIO_MIC2_BIAS_EN,
    .mic3_bias_inside = TCFG_AUDIO_MIC3_BIAS_EN,


//MIC免电容方案需要设置，影响MIC的偏置电压
/*MIC免电容方案需要设置，影响MIC的偏置电压
    0b0001~0b1001 : 0.5k ~ 4.5k step = 0.5k
    0b1010~0b1111 : 5k ~ 10k step = 1k */
    .mic_bias_res   = 4,
    .mic1_bias_res   = 4,
    .mic2_bias_res   = 4,
    .mic3_bias_res   = 4,
/*MIC LDO电压档位设置,也会影响MIC的偏置电压
    0:2.3v  1:2.5v  2:2.7v  3:3.0v */
	.mic_ldo_vsel  = 2,
    .mic_ldo_pwr   = TCFG_AUDIO_MIC_LDO_EN,

	// ladc 通道
    .ladc_num = ARRAY_SIZE(ladc_list),
    .ladc = ladc_list,
};
#endif

/* struct audio_pf_data audio_pf_d = { */
/* #if TCFG_AUDIO_DAC_ENABLE */
    /* .adc_pf_data = &adc_data, */
/* #endif */

/* #if TCFG_AUDIO_ADC_ENABLE */
    /* .dac_pf_data = &dac_data, */
/* #endif */
/* }; */

/* struct audio_platform_data audio_data = { */
    /* .private_data = (void *) &audio_pf_d, */
/* }; */


/************************** IO KEY ****************************/
#if TCFG_IOKEY_ENABLE
const struct iokey_port iokey_list[] = {
    {
        .connect_way = TCFG_IOKEY0_WAY,          //IO按键的连接方式
        .key_type.one_io.port = TCFG_IOKEY0,    //IO按键对应的引脚
        .key_value = 0,                                       //按键值
    },

    {
        .connect_way = TCFG_IOKEY1_WAY,
        .key_type.one_io.port = TCFG_IOKEY1,
        .key_value = 1,
    },

    {
        .connect_way = TCFG_IOKEY2_WAY,
        .key_type.one_io.port = TCFG_IOKEY2,
        .key_value = 2,
    },

#if (TCFG_IOKEY3 != TCFG_RVDD2PVDD_DCDC_EN_IO)
    {
        .connect_way = TCFG_IOKEY3_WAY,
        .key_type.one_io.port = TCFG_IOKEY3,
        .key_value = 3,
    },
#endif
};
const struct iokey_platform_data iokey_data = {
    .enable = TCFG_IOKEY_ENABLE,                              //是否使能IO按键
    .num = ARRAY_SIZE(iokey_list),                            //IO按键的个数
    .port = iokey_list,                                       //IO按键参数表
};

#if MULT_KEY_ENABLE
//组合按键消息映射表
//配置注意事项:单个按键按键值需要按照顺序编号,如power:0, prev:1, next:2
//bit_value = BIT(0) | BIT(1) 指按键值为0和按键值为1的两个按键被同时按下,
//remap_value = 3指当这两个按键被同时按下后重新映射的按键值;
const struct key_remap iokey_remap_table[] = {
	{.bit_value = BIT(0) | BIT(1), .remap_value = 3},
	{.bit_value = BIT(0) | BIT(2), .remap_value = 4},
	{.bit_value = BIT(1) | BIT(2), .remap_value = 5},
};

const struct key_remap_data iokey_remap_data = {
	.remap_num = ARRAY_SIZE(iokey_remap_table),
	.table = iokey_remap_table,
};
#endif

#endif

/************************** TOUCH_KEY ****************************/
#if TCFG_TOUCH_KEY_ENABLE
const const struct touch_key_port touch_key_list[] = {
    {
	    .press_delta    = TCFG_TOUCH_KEY0_PRESS_DELTA,
        .port           = TCFG_TOUCH_KEY0_PORT,
        .key_value      = TCFG_TOUCH_KEY0_VALUE,
    },
    {
	    .press_delta    = TCFG_TOUCH_KEY1_PRESS_DELTA,
	    .port           = TCFG_TOUCH_KEY1_PORT,
        .key_value      = TCFG_TOUCH_KEY1_VALUE,
    },
};

const struct touch_key_platform_data touch_key_data = {
    .num = ARRAY_SIZE(touch_key_list),
    .port_list = touch_key_list,
};
#endif  /* #if TCFG_TOUCH_KEY_ENABLE */

/************************** AD KEY ****************************/
#if TCFG_ADKEY_ENABLE
const struct adkey_platform_data adkey_data = {
    .enable = TCFG_ADKEY_ENABLE,                              //AD按键使能
    .adkey_pin = TCFG_ADKEY_PORT,                             //AD按键对应引脚
    .ad_channel = TCFG_ADKEY_AD_CHANNEL,                      //AD通道值
    .extern_up_en = TCFG_ADKEY_EXTERN_UP_ENABLE,              //是否使用外接上拉电阻
    .ad_value = {                                             //根据电阻算出来的电压值
        TCFG_ADKEY_VOLTAGE0,
        TCFG_ADKEY_VOLTAGE1,
        TCFG_ADKEY_VOLTAGE2,
        TCFG_ADKEY_VOLTAGE3,
        TCFG_ADKEY_VOLTAGE4,
        TCFG_ADKEY_VOLTAGE5,
        TCFG_ADKEY_VOLTAGE6,
        TCFG_ADKEY_VOLTAGE7,
        TCFG_ADKEY_VOLTAGE8,
        TCFG_ADKEY_VOLTAGE9,
    },
    .key_value = {                                             //AD按键各个按键的键值
        TCFG_ADKEY_VALUE0,
        TCFG_ADKEY_VALUE1,
        TCFG_ADKEY_VALUE2,
        TCFG_ADKEY_VALUE3,
        TCFG_ADKEY_VALUE4,
        TCFG_ADKEY_VALUE5,
        TCFG_ADKEY_VALUE6,
        TCFG_ADKEY_VALUE7,
        TCFG_ADKEY_VALUE8,
        TCFG_ADKEY_VALUE9,
    },
};
#endif

/************************** RDEC_KEY ****************************/
#if TCFG_RDEC_KEY_ENABLE
const struct rdec_device rdeckey_list[] = {
    {
        .index = RDEC0 ,
        .sin_port0 = TCFG_RDEC0_ECODE1_PORT,
        .sin_port1 = TCFG_RDEC0_ECODE2_PORT,
        .key_value0 = TCFG_RDEC0_KEY0_VALUE | BIT(7),
        .key_value1 = TCFG_RDEC0_KEY1_VALUE | BIT(7),
    },

    {
        .index = RDEC1 ,
        .sin_port0 = TCFG_RDEC1_ECODE1_PORT,
        .sin_port1 = TCFG_RDEC1_ECODE2_PORT,
        .key_value0 = TCFG_RDEC1_KEY0_VALUE | BIT(7),
        .key_value1 = TCFG_RDEC1_KEY1_VALUE | BIT(7),
    },

    {
        .index = RDEC2 ,
        .sin_port0 = TCFG_RDEC2_ECODE1_PORT,
        .sin_port1 = TCFG_RDEC2_ECODE2_PORT,
        .key_value0 = TCFG_RDEC2_KEY0_VALUE | BIT(7),
        .key_value1 = TCFG_RDEC2_KEY1_VALUE | BIT(7),
    },


};
const struct rdec_platform_data  rdec_key_data = {
    .enable = 1, //TCFG_RDEC_KEY_ENABLE,                              //是否使能RDEC按键
    .num = ARRAY_SIZE(rdeckey_list),                            //RDEC按键的个数
    .rdec = rdeckey_list,                                       //RDEC按键参数表
};
#endif

/************************** PWM_LED ****************************/
#if TCFG_PWMLED_ENABLE
LED_PLATFORM_DATA_BEGIN(pwm_led_data)
	.io_mode = TCFG_PWMLED_IOMODE,              //推灯模式设置:支持单个IO推两个灯和两个IO推两个灯
	.io_cfg.one_io.pin = TCFG_PWMLED_PIN,       //单个IO推两个灯的IO口配置
LED_PLATFORM_DATA_END()
#endif

#if TCFG_UI_ENABLE

#if TCFG_UI_LED7_ENABLE
LED7_PLATFORM_DATA_BEGIN(led7_data)
	.pin_type = LED7_PIN7,
    .pin_cfg.pin7.pin[0] = IO_PORTC_01,
    .pin_cfg.pin7.pin[1] = IO_PORTC_02,
    .pin_cfg.pin7.pin[2] = IO_PORTC_03,
    .pin_cfg.pin7.pin[3] = IO_PORTC_04,
    .pin_cfg.pin7.pin[4] = IO_PORTC_05,
    .pin_cfg.pin7.pin[5] = IO_PORTB_06,
    .pin_cfg.pin7.pin[6] = IO_PORTB_07,
LED7_PLATFORM_DATA_END()

const struct ui_devices_cfg ui_cfg_data = {
	.type = LED_7,
	.private_data = (void *)&led7_data,
};
#endif /* #if TCFG_UI_LED7_ENABLE */

#if TCFG_UI_LED1888_ENABLE
LED7_PLATFORM_DATA_BEGIN(led7_data)
	.pin_type = LED7_PIN7,
	.pin_cfg.pin7.pin[0] = IO_PORTB_00,
	.pin_cfg.pin7.pin[1] = IO_PORTB_01,
	.pin_cfg.pin7.pin[2] = IO_PORTB_02,
	.pin_cfg.pin7.pin[3] = IO_PORTB_03,
	.pin_cfg.pin7.pin[4] = IO_PORTB_04,
	.pin_cfg.pin7.pin[5] = IO_PORTB_05,
	.pin_cfg.pin7.pin[6] = (u8)-1,//IO_PORTB_06,
LED7_PLATFORM_DATA_END()

const struct ui_devices_cfg ui_cfg_data = {
	.type = LED_7,
	.private_data = (void *)&led7_data,
};
#endif /* #if TCFG_UI_LED7_ENABLE */


#if TCFG_UI_LCD_SEG3X9_ENABLE
LCD_SEG3X9_PLATFORM_DATA_BEGIN(lcd_seg3x9_data)
	.vlcd = LCD_SEG3X9_VOLTAGE_3_3V,
	.bias = LCD_SEG3X9_BIAS_1_3,
	.pin_cfg.pin_com[0] = IO_PORTC_05,
	.pin_cfg.pin_com[1] = IO_PORTC_04,
	.pin_cfg.pin_com[2] = IO_PORTC_03,

	.pin_cfg.pin_seg[0] = IO_PORTA_00,
	.pin_cfg.pin_seg[1] = IO_PORTA_01,
	.pin_cfg.pin_seg[2] = IO_PORTA_02,
	.pin_cfg.pin_seg[3] = IO_PORTA_03,
	.pin_cfg.pin_seg[4] = IO_PORTA_04,
	.pin_cfg.pin_seg[5] = IO_PORTA_07,
	.pin_cfg.pin_seg[6] = IO_PORTA_12,
	.pin_cfg.pin_seg[7] = IO_PORTA_10,
	.pin_cfg.pin_seg[8] = IO_PORTA_09,
LCD_SEG3X9_PLATFORM_DATA_END()

const struct ui_devices_cfg ui_cfg_data = {
	.type = LCD_SEG3X9,
	.private_data = (void *)&lcd_seg3x9_data,
};

#endif /* #if TCFG_UI_LCD_SEG3X9_ENABLE */

#if TCFG_SPI_LCD_ENABLE
LCD_SPI_PLATFORM_DATA_BEGIN(lcd_spi_data)
    .pin_reset= IO_PORTA_02,
    .pin_cs	= IO_PORTA_06,
    .pin_rs	= NO_CONFIG_PORT,
    .pin_bl = NO_CONFIG_PORT,
    .pin_dc	= NO_CONFIG_PORT,
    .pin_en	= IO_PORTB_09,
    .pin_te = IO_PORTA_03,

#if (TCFG_TFT_LCD_DEV_SPI_HW_NUM == 1)
    .spi_cfg	= SPI1,
    .spi_pdata  = &spi1_p_data,
#elif (TCFG_TFT_LCD_DEV_SPI_HW_NUM == 2)
    .spi_cfg	= SPI2,
    .spi_pdata  = &spi2_p_data,
#endif

LCD_SPI__PLATFORM_DATA_END()

const struct ui_devices_cfg ui_cfg_data = {
	.type = TFT_LCD,
	.private_data = (void *)&lcd_spi_data,
};
#endif

#endif /*TCFG_UI_ENABLE*/


#if 1
const struct soft_iic_config soft_iic_cfg[] = {
    //iic0 data
    {
        .scl = TCFG_SW_I2C0_CLK_PORT,                   //IIC CLK脚
        .sda = TCFG_SW_I2C0_DAT_PORT,                   //IIC DAT脚
        .delay = TCFG_SW_I2C0_DELAY_CNT,                //软件IIC延时参数，影响通讯时钟频率
        .io_pu = 1,                                     //是否打开上拉电阻，如果外部电路没有焊接上拉电阻需要置1
    },
#if 0
    //iic1 data
    {
        .scl = IO_PORTA_05,
        .sda = IO_PORTA_06,
        .delay = 50,
        .io_pu = 1,
    },
#endif
};


const struct hw_iic_config hw_iic_cfg[] = {
    //iic0 data
    {
        /*硬件IIC端口下选择
 			    SCL         SDA
		  	{IO_PORT_DP,  IO_PORT_DM},    //group a
        	{IO_PORTC_04, IO_PORTC_05},  //group b
        	{IO_PORTC_02, IO_PORTC_03},  //group c
        	{IO_PORTA_05, IO_PORTA_06},  //group d
         */
        .port = TCFG_HW_I2C0_PORTS,
        .baudrate = TCFG_HW_I2C0_CLK,      //IIC通讯波特率
        .hdrive = 0,                       //是否打开IO口强驱
        .io_filter = 1,                    //是否打开滤波器（去纹波）
        .io_pu = 1,                        //是否打开上拉电阻，如果外部电路没有焊接上拉电阻需要置1
    },
};
#endif

#if	TCFG_HW_SPI1_ENABLE
const struct spi_platform_data spi1_p_data = {
    .port = {
        TCFG_HW_SPI1_PORT_CLK,
        TCFG_HW_SPI1_PORT_DO,
        TCFG_HW_SPI1_PORT_DI,
    },
	.mode = TCFG_HW_SPI1_MODE,
	.clk  = TCFG_HW_SPI1_BAUD,
	.role = TCFG_HW_SPI1_ROLE,
};
#endif

#if	TCFG_HW_SPI2_ENABLE
const struct spi_platform_data spi2_p_data = {
    .port = {
        TCFG_HW_SPI2_PORT_CLK,
        TCFG_HW_SPI2_PORT_DO,
        TCFG_HW_SPI2_PORT_DI,
    },
	.mode = TCFG_HW_SPI2_MODE,
	.clk  = TCFG_HW_SPI2_BAUD,
	.role = TCFG_HW_SPI2_ROLE,
};
#endif

#if TCFG_NORFLASH_DEV_ENABLE
#if TCFG_NOR_FAT
NORFLASH_DEV_PLATFORM_DATA_BEGIN(norflash_fat_dev_data)
    .spi_hw_num     = TCFG_FLASH_DEV_SPI_HW_NUM,
    .spi_cs_port    = TCFG_FLASH_DEV_SPI_CS_PORT,
#if (TCFG_FLASH_DEV_SPI_HW_NUM == 1)
    .spi_pdata      = &spi1_p_data,
#elif (TCFG_FLASH_DEV_SPI_HW_NUM == 2)
    .spi_pdata      = &spi2_p_data,
#endif
    .start_addr     = 0,
    .size           = 1*1024*1024,
NORFLASH_DEV_PLATFORM_DATA_END()
#endif

#if TCFG_NOR_FS
NORFLASH_DEV_PLATFORM_DATA_BEGIN(norflash_norfs_dev_data)
    .spi_hw_num     = TCFG_FLASH_DEV_SPI_HW_NUM,
    .spi_cs_port    = TCFG_FLASH_DEV_SPI_CS_PORT,
#if (TCFG_FLASH_DEV_SPI_HW_NUM == 1)
    .spi_pdata      = &spi1_p_data,
#elif (TCFG_FLASH_DEV_SPI_HW_NUM == 2)
    .spi_pdata      = &spi2_p_data,
#endif
    /* .start_addr     = 1*1024*1024, */
    .start_addr     = 0,
    .size           = 4*1024*1024,
NORFLASH_DEV_PLATFORM_DATA_END()
#endif

#if TCFG_NOR_REC
NORFLASH_DEV_PLATFORM_DATA_BEGIN(norflash_norfs_rec_dev_data)
    .spi_hw_num     = TCFG_FLASH_DEV_SPI_HW_NUM,
    .spi_cs_port    = TCFG_FLASH_DEV_SPI_CS_PORT,
#if (TCFG_FLASH_DEV_SPI_HW_NUM == 1)
    .spi_pdata      = &spi1_p_data,
#elif (TCFG_FLASH_DEV_SPI_HW_NUM == 2)
    .spi_pdata      = &spi2_p_data,
#endif
    .start_addr     = 2*1024*1024,
    .size           = 2*1024*1024,
NORFLASH_DEV_PLATFORM_DATA_END()
#endif

#endif /*TCFG_NORFLASH_DEV_ENABLE*/



#if TCFG_NORFLASH_SFC_DEV_ENABLE
SFC_SPI_PLATFORM_DATA_BEGIN(sfc_spi_data)
    .spi_hw_index    = 1,
    .sfc_data_width  = SFC_DATA_WIDTH_4,
    .sfc_read_mode   = SFC_RD_OUTPUT,
    .sfc_encry       = 1, //是否加密
    .sfc_clk_div     = 0, //时钟分频: sfc_fre = sys_clk / div;
    .unencry_start_addr = CONFIG_EXTERN_FLASH_SIZE - CONFIG_EXTERN_USER_VM_FLASH_SIZE,//不加密区域
    .unencry_size  = CONFIG_EXTERN_USER_VM_FLASH_SIZE,
SFC_SPI_PLATFORM_DATA_END()

NORFLASH_SFC_DEV_PLATFORM_DATA_BEGIN(norflash_sfc_dev_data)
    .sfc_spi_pdata     = &sfc_spi_data,
    .start_addr     = 0,
    .size           = CONFIG_EXTERN_FLASH_SIZE - CONFIG_EXTERN_USER_VM_FLASH_SIZE,
NORFLASH_SFC_DEV_PLATFORM_DATA_END()
#endif /* #if TCFG_NORFLASH_SFC_DEV_ENABLE */

#if TCFG_NOR_VM
NORFLASH_SFC_DEV_PLATFORM_DATA_BEGIN(norflash_norfs_vm_dev_data)
    .sfc_spi_pdata     = &sfc_spi_data,
    .start_addr     = CONFIG_EXTERN_FLASH_SIZE - CONFIG_EXTERN_USER_VM_FLASH_SIZE,
    .size           = CONFIG_EXTERN_USER_VM_FLASH_SIZE,
NORFLASH_SFC_DEV_PLATFORM_DATA_END()
#endif



#if TCFG_SD0_ENABLE
#if TCFG_SD_ALWAY_ONLINE_ENABLE
int sdmmc_0_io_detect(const struct sdmmc_platform_data *data)
{
	return 1;
}
#endif /* #if TCFG_SD_ALWAY_ONLINE_ENABLE */

SD0_PLATFORM_DATA_BEGIN(sd0_data)
	.port = {
        TCFG_SD0_PORT_CMD,
        TCFG_SD0_PORT_CLK,
        TCFG_SD0_PORT_DA0,
        TCFG_SD0_PORT_DA1,
        TCFG_SD0_PORT_DA2,
        TCFG_SD0_PORT_DA3,
    },
	.data_width             = TCFG_SD0_DAT_MODE,
	.speed                  = TCFG_SD0_CLK,
	.detect_mode            = TCFG_SD0_DET_MODE,
	.priority				= 3,

#if (TCFG_SD0_DET_MODE == SD_IO_DECT)
    .detect_io              = TCFG_SD0_DET_IO,
    .detect_io_level        = TCFG_SD0_DET_IO_LEVEL,
    .detect_func            = sdmmc_0_io_detect,
	.power                  = sd_set_power,
	/* .power                  = NULL, */
#elif (TCFG_SD0_DET_MODE == SD_CLK_DECT)
    .detect_io_level        = TCFG_SD0_DET_IO_LEVEL,
    .detect_func            = sdmmc_0_clk_detect,
	.power                  = sd_set_power,
	/* .power                  = NULL, */
#else
    .detect_func            = sdmmc_cmd_detect,
    .power                  = NULL,
#endif

SD0_PLATFORM_DATA_END()
#endif /* #if TCFG_SD0_ENABLE */



 /************************** hrsensor ****************************/
#if (TCFG_HR_SENSOR_ENABLE||TCFG_SPO2_SENSOR_ENABLE)

HRSENSOR_PLATFORM_DATA_BEGIN(hrSensor_data)
    .iic = 0,//TCFG_HR_SENOR_USER_IIC_INDEX ,
    .hrSensor_name = TCFG_HR_SENOR_NAME,
HRSENSOR_PLATFORM_DATA_END()

#endif


/************************** gsensor ****************************/
#if TCFG_GSENSOR_ENABLE
GSENSOR_PLATFORM_DATA_BEGIN(gSensor_data)
    .iic = TCFG_GSENOR_USER_IIC_INDEX,
    .gSensor_name = TCFG_GSENSOR_NAME,
    .gSensor_int_io = TCFG_GSENSOR_DETECT_IO,
GSENSOR_PLATFORM_DATA_END();
#endif      //end if CONFIG_GSENSOR_ENABLE

/************************** FM ****************************/
#if TCFG_FM_ENABLE
FM_DEV_PLATFORM_DATA_BEGIN(fm_dev_data)
	.iic_hdl = 0,
	.iic_delay = 50,
FM_DEV_PLATFORM_DATA_END();
#endif


/************************** linein KEY ****************************/
#if TCFG_LINEIN_ENABLE
struct linein_dev_data linein_data = {
	.enable = TCFG_LINEIN_ENABLE,
	.port = TCFG_LINEIN_CHECK_PORT,
	.up = TCFG_LINEIN_PORT_UP_ENABLE,
	.down = TCFG_LINEIN_PORT_DOWN_ENABLE,
	.ad_channel = TCFG_LINEIN_AD_CHANNEL,
	.ad_vol = TCFG_LINEIN_VOLTAGE,
};
#endif

/************************** rtc ****************************/
#if TCFG_RTC_ENABLE
const struct sys_time def_sys_time = {  //初始一下当前时间
    .year = 2020,
    .month = 1,
    .day = 1,
    .hour = 0,
    .min = 0,
    .sec = 0,
};
const struct sys_time def_alarm = {     //初始一下目标时间，即闹钟时间
    .year = 2050,
    .month = 1,
    .day = 1,
    .hour = 0,
    .min = 0,
    .sec = 0,
};

/* extern void alarm_isr_user_cbfun(u8 index); */
RTC_DEV_PLATFORM_DATA_BEGIN(rtc_data)
    .default_sys_time = &def_sys_time,
    .default_alarm = &def_alarm,
#if TCFG_RTC_USE_LRC
	.clk_sel = CLK_SEL_LRC,
#else
	.clk_sel = CLK_SEL_32K,
#endif
	.cbfun = NULL,                      //闹钟中断的回调函数,用户自行定义
    /* .cbfun = alarm_isr_user_cbfun, */
RTC_DEV_PLATFORM_DATA_END()

#if TCFG_RTC_USE_LRC
static void rtc_lrc_timer_deal(void *param)
{
	struct sys_time time;
    void *fd = dev_open("rtc", NULL);
    if (!fd) {
        return;
    }
    dev_ioctl(fd, IOCTL_GET_SYS_TIME, (u32)&time);
    dev_close(fd);
}
#endif /* #if TCFG_RTC_USE_LRC */

#endif


extern const struct device_operations mass_storage_ops;

REGISTER_DEVICES(device_table) = {
    /* { "audio", &audio_dev_ops, (void *) &audio_data }, */

#if TCFG_VIRFAT_FLASH_ENABLE
#if TCFG_VIRFAT_INSERT_FLASH_ENABLE
	{ "virfat_flash", 	&virfat_flash_dev_ops, 	NULL},
#else
	{ "virfat_flash", 	&virfat_flash_dev_ops, 	(void *)"res_nor"},
#endif

#if TCFG_SFC_VM
	{ "ui_sfc_vm", 	&private_sfc_dev_ops, 	NULL},
#endif
#endif

#if TCFG_SD0_ENABLE
	{ "sd0", 	&sd_dev_ops, 	(void *) &sd0_data},
#endif

#if TCFG_LINEIN_ENABLE
    { "linein",  &linein_dev_ops, (void *)&linein_data},
#endif
#if TCFG_OTG_MODE
    { "otg",     &usb_dev_ops, (void *) &otg_data},
#endif
#if TCFG_UDISK_ENABLE
    { "udisk0",   &mass_storage_ops , NULL},
#endif
#if TCFG_RTC_ENABLE
    { "rtc",   &rtc_dev_ops , (void *)&rtc_data},
#endif

/* #if TCFG_NORFLASH_DEV_ENABLE */
    /* { "norflash",   &norflash_dev_ops , (void *)&norflash_dev_data}, */
/* #endif */
/* #if TCFG_NOR_FS_ENABLE */
    /* { "nor_ui",   &norfs_dev_ops , (void *)&norfs_dev_data}, */
/* #endif */

#if TCFG_NORFLASH_DEV_ENABLE
#if TCFG_NOR_FAT
    { "fat_nor",   &norflash_dev_ops , (void *)&norflash_fat_dev_data},
#endif

#if TCFG_NOR_FS
    { "res_nor",   &norfs_dev_ops , (void *)&norflash_norfs_dev_data},
#endif

#if TCFG_NOR_REC
    {"rec_nor",   &norfs_dev_ops , (void *)&norflash_norfs_rec_dev_data},
#endif
#endif /*TCFG_NORFLASH_DEV_ENABLE*/

#if TCFG_NORFLASH_SFC_DEV_ENABLE
#if TCFG_NOR_FS
    { "res_nor",   &norflash_sfc_fs_dev_ops, (void *)&norflash_sfc_dev_data},
#endif
#endif /*TCFG_NORFLASH_SFC_DEV_ENABLE*/

#if TCFG_NANDFLASH_DEV_ENABLE
	{"nand_flash",   &nandflash_dev_ops , (void *)&nandflash_dev_data},
#endif

#if TCFG_NOR_VM
    {"ui_vm",   &norflash_sfc_fs_dev_ops , (void *)&norflash_norfs_vm_dev_data},
#endif

};

/************************** LOW POWER config ****************************/
const struct low_power_param power_param = {
    .config         = TCFG_LOWPOWER_LOWPOWER_SEL,          //0：sniff时芯片不进入低功耗  1：sniff时芯片进入powerdown
    .btosc_hz       = TCFG_CLOCK_OSC_HZ,                   //外接晶振频率
    .delay_us       = TCFG_CLOCK_SYS_HZ / 1000000L,        //提供给低功耗模块的延时(不需要需修改)
    .btosc_disable  = TCFG_LOWPOWER_BTOSC_DISABLE,         //进入低功耗时BTOSC是否保持
    .vddiom_lev     = TCFG_LOWPOWER_VDDIOM_LEVEL,          //强VDDIO等级,可选：2.0V  2.2V  2.4V  2.6V  2.8V  3.0V  3.2V  3.6V
    .osc_type       = TCFG_LOWPOWER_OSC_TYPE,
#if (TCFG_LOWPOWER_RAM_SIZE == 4)
	.mem_lowpower_con	= MEM_PWR_RAM0_RAM3_CON | MEM_PWR_RAM4_RAM5_CON | MEM_PWR_RAM6_RAM7_CON,
#elif (TCFG_LOWPOWER_RAM_SIZE == 3)
	.mem_lowpower_con	= MEM_PWR_RAM0_RAM3_CON | MEM_PWR_RAM4_RAM5_CON,
#elif (TCFG_LOWPOWER_RAM_SIZE == 2)
	.mem_lowpower_con	= MEM_PWR_RAM0_RAM3_CON,
#elif (TCFG_LOWPOWER_RAM_SIZE == 1)
#error "BR28 RAM0~RAM3 IS 256K"
#endif
	.rvdd2pvdd = TCFG_RVDD2PVDD_DCDC,
	.rtc_clk = TCFG_RTC_USE_LRC,
	.flash_pg = TCFG_EX_FLASH_POWER_IO,
};



/************************** PWR config ****************************/
struct port_wakeup port0 = {
	// 暂时先用作触摸板外部中断检测
    .pullup_down_enable = ENABLE,                            //配置I/O 内部上下拉是否使能
    .edge               = FALLING_EDGE,                      //唤醒方式选择,可选：上升沿\下降沿
    .both_edge          = 0,
    .filter             = PORT_FLT_8ms,
    .iomap              = TCFG_TP_INT_IO,                    //唤醒口选择
};

#if (TCFG_TEST_BOX_ENABLE || TCFG_CHARGESTORE_ENABLE || TCFG_ANC_BOX_ENABLE)
struct port_wakeup port1 = {
    .pullup_down_enable = DISABLE,                            //配置I/O 内部上下拉是否使能
    .edge               = FALLING_EDGE,                      //唤醒方式选择,可选：上升沿\下降沿
    .both_edge          = 1,
    .filter             = PORT_FLT_1ms,
    .iomap              = TCFG_CHARGESTORE_PORT,             //唤醒口选择
};
#endif

#if (TCFG_IOKEY_ENABLE || TCFG_ADKEY_ENABLE)
struct port_wakeup port2 = {
	// 暂时先用作触摸板外部中断检测
    .pullup_down_enable = ENABLE,                            //配置I/O 内部上下拉是否使能
    .edge               = FALLING_EDGE,                      //唤醒方式选择,可选：上升沿\下降沿
    .both_edge          = 0,
    .filter             = PORT_FLT_8ms,
    .iomap              = TCFG_IOKEY0,                    //唤醒口选择
};
#endif



#if (TCFG_GSENSOR_ENABLE)
struct port_wakeup port3 = {
	//gsensor唤醒
    .pullup_down_enable = ENABLE,                            //配置I/O 内部上下拉是否使能
    .edge               = FALLING_EDGE,                      //唤醒方式选择,可选：上升沿\下降沿
    .both_edge          = 0,
    .filter             = PORT_FLT_256us,
    .iomap              = IO_PORTB_03,                    //唤醒口选择
};
#endif

#if TCFG_CHARGE_ENABLE
struct port_wakeup charge_port = {
    .edge               = RISING_EDGE,                       //唤醒方式选择,可选：上升沿\下降沿\双边沿
    .both_edge          = 0,
    .filter             = PORT_FLT_16ms,
    .iomap              = IO_CHGFL_DET,                      //唤醒口选择
};

struct port_wakeup vbat_port = {
    .edge               = BOTH_EDGE,                      //唤醒方式选择,可选：上升沿\下降沿\双边沿
    .both_edge          = 1,
    .filter             = PORT_FLT_16ms,
    .iomap              = IO_VBTCH_DET,                      //唤醒口选择
};

struct port_wakeup ldoin_port = {
    .edge               = BOTH_EDGE,                      //唤醒方式选择,可选：上升沿\下降沿\双边沿
    .both_edge          = 1,
    .filter             = PORT_FLT_16ms,
    .iomap              = IO_LDOIN_DET,                      //唤醒口选择
};
#endif

const struct wakeup_param wk_param = {
/* #if (!TCFG_LP_TOUCH_KEY_ENABLE) */
	.port[1] = &port0,
/* #endif */
#if (TCFG_TEST_BOX_ENABLE || TCFG_CHARGESTORE_ENABLE || TCFG_ANC_BOX_ENABLE)
	.port[2] = &port1,
#endif

#if (TCFG_IOKEY_ENABLE || TCFG_ADKEY_ENABLE)
    .port[3] = &port2,
#endif

#if (TCFG_GSENSOR_ENABLE)
    .port[4] = &port3,
#endif

#if TCFG_CHARGE_ENABLE
    .aport[0] = &charge_port,
    .aport[1] = &vbat_port,
    .aport[2] = &ldoin_port,
#endif
};

void gSensor_wkupup_disable(void)
{
    log_info("gSensor wkup disable\n");
    power_wakeup_index_disable(1);
}

void gSensor_wkupup_enable(void)
{
    log_info("gSensor wkup enable\n");
    power_wakeup_index_enable(1);
}

static void key_wakeup_disable()
{
    power_wakeup_index_disable(1);
}
static void key_wakeup_enable()
{
    power_wakeup_index_enable(1);
}

void debug_uart_init(const struct uart_platform_data *data)
{
#if TCFG_UART0_ENABLE
    if (data) {
        uart_init(data);
    } else {
        uart_init(&uart0_data);
    }
#endif
}


STATUS *get_led_config(void)
{
    return &(__this->led);
}

STATUS *get_tone_config(void)
{
    return &(__this->tone);
}

u8 get_sys_default_vol(void)
{
    return 21;
}

u8 get_power_on_status(void)
{
#if TCFG_IOKEY_ENABLE
    struct iokey_port *power_io_list = NULL;
    power_io_list = iokey_data.port;

    if (iokey_data.enable) {
        if (gpio_read(power_io_list->key_type.one_io.port) == power_io_list->connect_way){
            return 1;
        }
    }
#endif

#if TCFG_ADKEY_ENABLE
    if (adkey_data.enable) {
        if (adc_get_value(adkey_data.ad_channel) < 10) {
            return 1;
        }
    }
#endif

    return 0;
}

static void board_devices_init(void)
{
#if TCFG_PWMLED_ENABLE
    /* pwm_led_init(&pwm_led_data); */
    ui_pwm_led_init(&pwm_led_data);
#endif

#if (TCFG_IOKEY_ENABLE || TCFG_ADKEY_ENABLE || TCFG_IRKEY_ENABLE || TCFG_RDEC_KEY_ENABLE ||  TCFG_CTMU_TOUCH_KEY_ENABLE)
	key_driver_init();
#endif

#if TCFG_UART_KEY_ENABLE
	extern int uart_key_init(void);
	uart_key_init();
#endif /* #if TCFG_UART_KEY_ENABLE */

#if (TCFG_HR_SENSOR_ENABLE||TCFG_SPO2_SENSOR_ENABLE)
    hr_sensor_init(&hrSensor_data);
#endif

#if TCFG_GSENSOR_ENABLE
    gravity_sensor_init((void*)&gSensor_data);
#endif      //end if CONFIG_GSENSOR_ENABLE

#if TCFG_UI_ENABLE
	UI_INIT(&ui_cfg_data);
#endif /* #if TCFG_UI_ENABLE */

#if  TCFG_APP_FM_EMITTER_EN
 	fm_emitter_manage_init(930);
#endif

#if TCFG_CHARGESTORE_ENABLE || TCFG_TEST_BOX_ENABLE || TCFG_ANC_BOX_ENABLE
    chargestore_api_init(&chargestore_data);
#endif
}

extern void cfg_file_parse(u8 idx);

void board_init()
{
    board_power_init();
    //adc_vbg_init();
    adc_init();
    cfg_file_parse(0);
    /* devices_init(); */

#if TCFG_RTC_ENABLE
    extern void p11_sys_time_init();
    p11_sys_time_init();
#if TCFG_RTC_USE_LRC
	// pb7和pb8合绑，设置高阻
	gpio_set_pull_up(IO_PORTB_07, 0);
	gpio_set_pull_down(IO_PORTB_07, 0);
	gpio_set_direction(IO_PORTB_07, 1);
    gpio_set_die(IO_PORTB_07, 0);
    gpio_set_dieh(IO_PORTB_07, 0);
	// LRC做时钟源时，每过一段时间读一下rtc用于校准
	sys_timer_add(NULL, rtc_lrc_timer_deal, 1*60*60*1000);
#endif /* #if TCFG_RTC_USE_LRC */
#endif

#if TCFG_CHARGE_ENABLE
    extern int charge_init(const struct dev_node *node, void *arg);
    charge_init(NULL, (void *)&charge_data);
#else
	CHARGE_EN(0);
    CHGGO_EN(0);
    P3_PINR_CON1 = 0;
#endif

    /* if (!get_charge_online_flag()) { */
        /* check_power_on_voltage(); */
    /* } */

#if (TCFG_SD0_ENABLE || TCFG_SD1_ENABLE)
	sdpg_config(4);
#endif

#if (TCFG_LCD_RM69330_ENABLE || TCFG_LCD_RM69330_QSPI_ENABLE)
    gpio_direction_output(IO_PORTA_05, 1);
#endif

// 触摸屏 和 屏公用复位，先初始化触摸
#if TCFG_TOUCH_PANEL_ENABLE
#if TCFG_TP_IT7259E_ENABLE
    extern void it7259_test();
    it7259_test();
    extern void it7259_int_en();
    it7259_int_en();
#endif
#if TCFG_TP_CST816S_ENABLE
    extern void cst816s_init();
    cst816s_init();
#endif
#if TCFG_TP_FT6336G_ENABLE
    extern void ft6336g_test();
    ft6336g_test();
    extern void ft6336g_int_en();
    ft6336g_int_en();
#endif
#if TCFG_TP_BL6133_ENABLE
    extern void bl6133_test();
    bl6133_test();
    extern void bl6133_int_en();
    bl6133_int_en();
#endif
#endif

#if TCFG_FM_ENABLE
	fm_dev_init(&fm_dev_data);
#endif

#if TCFG_NOR_REC
    nor_fs_ops_init();
#endif

#if TCFG_NOR_FS
    init_norsdfile_hdl();
#endif

#if FLASH_INSIDE_REC_ENABLE
    sdfile_rec_ops_init();
#endif

	dev_manager_init();
    dev_manager_set_valid_by_logo("res_nor", 0);///将设备设置为无效设备

	board_devices_init();

#if TCFG_CHARGE_ENABLE
    if(get_charge_online_flag())
#else
    if (0)
#endif
	{
    	power_set_mode(PWR_LDO15);
	}else{
    	power_set_mode(TCFG_LOWPOWER_POWER_SEL);
	}

    //针对硅mic要输出1给mic供电
    if(!adc_data.mic_capless){
        gpio_set_pull_up(IO_PORTA_04, 0);
        gpio_set_pull_down(IO_PORTA_04, 0);
        gpio_set_direction(IO_PORTA_04, 0);
        gpio_set_output_value(IO_PORTA_04,1);
    }

 #if TCFG_UART0_ENABLE
    if (uart0_data.rx_pin < IO_MAX_NUM) {
        gpio_set_die(uart0_data.rx_pin, 1);
    }
#endif

#if TCFG_AUDIO_VAD_ENABLE
	u32 audio_vad_init(void);
	audio_vad_init();
#endif /* #if TCFG_AUDIO_VAD_ENABLE */


#ifdef AUDIO_PCM_DEBUG
	uartSendInit();
#endif

#if TCFG_RTC_ENABLE
    alarm_init();
#endif

#if TCFG_SENSOR_DEBUG_ENABLE
    data_export_init();
    extern void *sensor_de_file;
#if TCFG_SD0_ENABLE
    sensor_de_file = data_export_file_open("storage/sd0/C/defile.dat",3, 10 * 1024);
#elif TCFG_SD1_ENABLE
    sensor_de_file = data_export_file_open("storage/sd1/C/defile.dat",3, 10 * 1024);
#elif TCFG_UDISK_ENABLE
    sensor_de_file = data_export_file_open("storage/udisk0/C/defile.dat",3,10 * 1024);
#else
#error "no storage dev"
#endif

    if(sensor_de_file){
        data_export_file_set_ch_name(sensor_de_file,0,1);
        data_export_file_set_ch_name(sensor_de_file,1,2);
        data_export_file_set_ch_name(sensor_de_file,2,3);
    }

#else

#if (TCFG_HR_SENSOR_ENABLE||TCFG_GSENSOR_ENABLE||TCFG_SPO2_SENSOR_ENABLE||TCFG_BMP280_ENABLE)
        extern int watch_sensor_open(void);
    watch_sensor_open();
#endif //TCFG_xxSENSOR_ENABLE
#endif
}



//maskrom 使用到的io
static void mask_io_cfg()
{
    struct boot_soft_flag_t boot_soft_flag = {0};
    boot_soft_flag.flag0.boot_ctrl.wdt_dis = 0;
    boot_soft_flag.flag0.boot_ctrl.poweroff = 0;
    boot_soft_flag.flag0.boot_ctrl.is_port_b = JL_IOMAP->CON0 & BIT(16) ? 1 : 0;

    boot_soft_flag.flag1.misc.usbdm = SOFTFLAG_HIGH_RESISTANCE;
    boot_soft_flag.flag1.misc.usbdp = SOFTFLAG_HIGH_RESISTANCE;

    boot_soft_flag.flag1.misc.uart_key_port = 0;
    boot_soft_flag.flag1.misc.ldoin = SOFTFLAG_HIGH_RESISTANCE;

    boot_soft_flag.flag2.pg2_pg3.pg2 = SOFTFLAG_HIGH_RESISTANCE;
    boot_soft_flag.flag2.pg2_pg3.pg3 = SOFTFLAG_HIGH_RESISTANCE;

    boot_soft_flag.flag3.pg4_res.pg4 = SOFTFLAG_HIGH_RESISTANCE;

    boot_soft_flag.flag4.fast_boot_ctrl.fast_boot = 1;
    boot_soft_flag.flag4.fast_boot_ctrl.flash_stable_delay_sel = 0;

    mask_softflag_config(&boot_soft_flag);

}
/*进软关机之前默认将IO口都设置成高阻状态，需要保留原来状态的请修改该函数*/
extern void dac_power_off(void);
void board_set_soft_poweroff(void)
{
	u16 port_group[] = {
        [PORTA_GROUP] = 0xffff,
        [PORTB_GROUP] = 0xffff,
        [PORTC_GROUP] = 0xffff,
       	[PORTD_GROUP] = 0xffff,
		[PORTE_GROUP] = 0xffff,
        [PORTG_GROUP] = 0xffff,
		[PORTP_GROUP] = 0x1,//
    };

#if (TCFG_SD0_ENABLE || TCFG_SD1_ENABLE)
	sdpg_config(0);
#endif

	//flash电源
	if(get_sfc_port()==0){
		port_protect(port_group, SPI0_PWR_A);
		port_protect(port_group, SPI0_CS_A);
		port_protect(port_group, SPI0_CLK_A);
		port_protect(port_group, SPI0_DO_D0_A);
		port_protect(port_group, SPI0_DI_D1_A);
		if(get_sfc_bit_mode()==4){
			port_protect(port_group, SPI0_WP_D2_A);
			port_protect(port_group, SPI0_HOLD_D3_A);
		}
	}else{
		port_protect(port_group, SPI0_PWR_B);
		port_protect(port_group, SPI0_CS_B);
		port_protect(port_group, SPI0_CLK_B);
		port_protect(port_group, SPI0_DO_D0_B);
		port_protect(port_group, SPI0_DI_D1_B);
		if(get_sfc_bit_mode()==4){
			port_protect(port_group, SPI0_WP_D2_B);
			port_protect(port_group, SPI0_HOLD_D3_B);
		}
	}

	// 外置flash
	port_protect(port_group, TCFG_EX_FLASH_POWER_IO);
	port_protect(port_group, SPI0_CS_C);
	port_protect(port_group, SPI0_CLK_C);
	port_protect(port_group, SPI0_DO_D0_C);
	port_protect(port_group, SPI0_DI_D1_C);
	port_protect(port_group, SPI0_WP_D2_C);
	port_protect(port_group, SPI0_HOLD_D3_C);

	//power按键
#if TCFG_IOKEY_ENABLE
    port_protect(port_group, TCFG_IOKEY0);
#endif

#if TCFG_UMIDIGI_BOX_ENABLE
	power_wakeup_index_disable(PORT_WKUP_F_INDEX);
	power_wakeup_index_disable(PORT_WKUP_R_INDEX);
#elif (TCFG_TEST_BOX_ENABLE || TCFG_CHARGESTORE_ENABLE || TCFG_ANC_BOX_ENABLE)
	power_wakeup_index_disable(2);
#endif

	mask_io_cfg();
    key_wakeup_disable();

#if  (TCFG_UI_ENABLE  && TCFG_SPI_LCD_ENABLE)
	if (lcd_spi_data.pin_reset != NO_CONFIG_PORT) {
		port_protect(port_group, lcd_spi_data.pin_reset);
	}
#endif /* #if TCFG_SPI_LCD_ENABLE */

	gpio_dir(GPIOA,    0, 16,  port_group[PORTA_GROUP], GPIO_OR);
    gpio_set_pu(GPIOA, 0, 16, ~port_group[PORTA_GROUP], GPIO_AND);
    gpio_set_pd(GPIOA, 0, 16, ~port_group[PORTA_GROUP], GPIO_AND);
    gpio_die(GPIOA,    0, 16, ~port_group[PORTA_GROUP], GPIO_AND);
    gpio_dieh(GPIOA,   0, 16, ~port_group[PORTA_GROUP], GPIO_AND);

	gpio_dir(GPIOB,    0, 16,  port_group[PORTB_GROUP], GPIO_OR);
    gpio_set_pu(GPIOB, 0, 16, ~port_group[PORTB_GROUP], GPIO_AND);
    gpio_set_pd(GPIOB, 0, 16, ~port_group[PORTB_GROUP], GPIO_AND);
    gpio_die(GPIOB,    0, 16, ~port_group[PORTB_GROUP], GPIO_AND);
    gpio_dieh(GPIOB,   0, 16, ~port_group[PORTB_GROUP], GPIO_AND);

	gpio_dir(GPIOC,    0, 16,  port_group[PORTC_GROUP], GPIO_OR);
	gpio_set_pu(GPIOC, 0, 16, ~port_group[PORTC_GROUP], GPIO_AND);
	gpio_set_pd(GPIOC, 0, 16, ~port_group[PORTC_GROUP], GPIO_AND);
	gpio_die(GPIOC,    0, 16, ~port_group[PORTC_GROUP], GPIO_AND);
	gpio_dieh(GPIOC,   0, 16, ~port_group[PORTC_GROUP], GPIO_AND);

	gpio_dir(GPIOG,    0, 16,  port_group[PORTG_GROUP], GPIO_OR);
    gpio_set_pu(GPIOG, 0, 16, ~port_group[PORTG_GROUP], GPIO_AND);
    gpio_set_pd(GPIOG, 0, 16, ~port_group[PORTG_GROUP], GPIO_AND);
    gpio_die(GPIOG,    0, 16, ~port_group[PORTG_GROUP], GPIO_AND);
    gpio_dieh(GPIOG,   0, 16, ~port_group[PORTG_GROUP], GPIO_AND);

	gpio_dir(GPIOD,    0, 16,  port_group[PORTD_GROUP], GPIO_OR);
    gpio_set_pu(GPIOD, 0, 16, ~port_group[PORTD_GROUP], GPIO_AND);
    gpio_set_pd(GPIOD, 0, 16, ~port_group[PORTD_GROUP], GPIO_AND);
    gpio_die(GPIOD,    0, 16, ~port_group[PORTD_GROUP], GPIO_AND);
    gpio_dieh(GPIOD,   0, 16, ~port_group[PORTD_GROUP], GPIO_AND);

	gpio_dir(GPIOE,    0, 16,  port_group[PORTE_GROUP], GPIO_OR);
    gpio_set_pu(GPIOE, 0, 16, ~port_group[PORTE_GROUP], GPIO_AND);
    gpio_set_pd(GPIOE, 0, 16, ~port_group[PORTE_GROUP], GPIO_AND);
    gpio_die(GPIOE,    0, 16, ~port_group[PORTE_GROUP], GPIO_AND);
    gpio_dieh(GPIOE,   0, 16, ~port_group[PORTE_GROUP], GPIO_AND);

	gpio_dir(GPIOP,    0, 1,  port_group[PORTP_GROUP],  GPIO_OR);
    gpio_set_pu(GPIOP, 0, 1,  ~port_group[PORTP_GROUP], GPIO_AND);
    gpio_set_pd(GPIOP, 0, 1,  ~port_group[PORTP_GROUP], GPIO_AND);
    gpio_die(GPIOP,    0, 1,  ~port_group[PORTP_GROUP], GPIO_AND);
    gpio_dieh(GPIOP,   0, 1,  ~port_group[PORTP_GROUP], GPIO_AND);

    gpio_set_pull_up(IO_PORT_DP, 0);
    gpio_set_pull_down(IO_PORT_DP, 0);
    gpio_set_direction(IO_PORT_DP, 1);
    gpio_set_die(IO_PORT_DP, 0);
    gpio_set_dieh(IO_PORT_DP, 0);

    gpio_set_pull_up(IO_PORT_DM, 0);
    gpio_set_pull_down(IO_PORT_DM, 0);
    gpio_set_direction(IO_PORT_DM, 1);
    gpio_set_die(IO_PORT_DM, 0);
    gpio_set_dieh(IO_PORT_DM, 0);

    dac_power_off();
}

#define     APP_IO_DEBUG_0(i,x)       //{JL_PORT##i->DIR &= ~BIT(x), JL_PORT##i->OUT &= ~BIT(x);}
#define     APP_IO_DEBUG_1(i,x)       //{JL_PORT##i->DIR &= ~BIT(x), JL_PORT##i->OUT |= BIT(x);}


#if TCFG_SD_ALWAY_ONLINE_ENABLE

extern int sdx_dev_entry_lowpower(const char *sdx_name);

static int sdx_entry_lowpower(int param)
{
	/* putchar('s'); */
#if TCFG_SD0_ENABLE
	sdx_dev_entry_lowpower("sd0");
#endif /* #if TCFG_SD1_ENABLE */
#if TCFG_SD1_ENABLE
	sdx_dev_entry_lowpower("sd1");
#endif /* #if TCFG_SD1_ENABLE */
	return 0;
}
static void sdx_sleep_callback(void)
{
    int argv[3];

    argv[0] = (int)sdx_entry_lowpower;
    argv[1] = 1;
    /* argv[2] = ; */

    int ret = os_taskq_post_type("app_core", Q_CALLBACK, 3, argv);
	if (ret) {
		printf("post ret:%d \n", ret);
	}
}

#else /*#if TCFG_SD_ALWAY_ONLINE_ENABLE*/

extern void sdx_dev_detect_modify(u32 modify_time);
void sniff_hook(u32 slot, u8 num, u8 first_conn, int t_sniff)
{
	if (num) {
		if (first_conn) {
			/* printf("..%d..\n", t_sniff); */
			sdx_dev_detect_modify(t_sniff - 12);
		}
	} else {
		/* printf("..%d..\n", t_sniff); */
		sdx_dev_detect_modify(t_sniff - 3);
	}
}
#endif /*#if TCFG_SD_ALWAY_ONLINE_ENABLE*/

void sleep_exit_callback(u32 usec)
{
    putchar('>');
    APP_IO_DEBUG_1(A, 5);

#if SDX_POWER_ALONE // sd使用单独的电源才能用
#if TCFG_SD_ALWAY_ONLINE_ENABLE
	if (sdx_power_enable && (sdx_power_enable != 0xff)) {
		sdx_sleep_callback();
	}
#endif /*#if TCFG_SD_ALWAY_ONLINE_ENABLE*/
#endif /* #if SDX_POWER_ALONE */
}
void sleep_enter_callback(u8  step)
{
    /* 此函数禁止添加打印 */
    if (step == 1) {
        putchar('<');
        APP_IO_DEBUG_0(A, 5);
        /* dac_sniff_power_off(); */
        /* dac_power_off(); */
#if 0
		extern u32 check_ram1_size(void);
		extern int mem_vlt_is_recyclable(void);
		if (check_ram1_size()) {
			if (mem_vlt_is_recyclable() <= 0) {
				printf("mem_vlt error \n");
				while(1);
			}
		}
#endif
    } else {
        usb_iomode(1);

        gpio_set_pull_up(IO_PORT_DP, 0);
        gpio_set_pull_down(IO_PORT_DP, 0);
        gpio_set_direction(IO_PORT_DP, 1);
        gpio_set_die(IO_PORT_DP, 0);

        gpio_set_pull_up(IO_PORT_DM, 0);
        gpio_set_pull_down(IO_PORT_DM, 0);
        gpio_set_direction(IO_PORT_DM, 1);
        gpio_set_die(IO_PORT_DM, 0);
    }
}

static void port_wakeup_callback(u8 index, u8 gpio)
{
    log_info("%s:%d,%d",__FUNCTION__,index,gpio);

    switch (index) {
#if (TCFG_TEST_BOX_ENABLE || TCFG_CHARGESTORE_ENABLE || TCFG_ANC_BOX_ENABLE)
        case 2:
            extern void chargestore_ldo5v_fall_deal(void);
            chargestore_ldo5v_fall_deal();
            break;
#endif
    }
}

static void aport_wakeup_callback(u8 index, u8 gpio, u8 edge)
{
    log_info("%s:%d,%d",__FUNCTION__,index,gpio);
#if TCFG_CHARGE_ENABLE
    switch (gpio) {
        case IO_CHGFL_DET://charge port
            charge_wakeup_isr();
            break;
        case IO_VBTCH_DET://vbat port
        case IO_LDOIN_DET://ldoin port
            ldoin_wakeup_isr();
            break;
    }
#endif
}

void board_power_init(void)
{
    log_info("Power init : %s", __FILE__);

#if (TCFG_EX_FLASH_POWER_IO != NO_CONFIG_PORT)
	// ext flash power
    log_info("ex flash Power on \n");
	gpio_direction_output(TCFG_EX_FLASH_POWER_IO, 1);
	gpio_set_hd(TCFG_EX_FLASH_POWER_IO, 1);
	gpio_set_hd0(TCFG_EX_FLASH_POWER_IO, 1);
	/* os_time_dly(5); */
#endif /* #if (TCFG_EX_FLASH_POWER_IO != NO_CONFIG_PORT) */

#if TCFG_RVDD2PVDD_DCDC
	gpio_direction_output(TCFG_RVDD2PVDD_DCDC_EN_IO, 1);
#endif /* #if TCFG_RVDD2PVDD_DCDC */

    power_init(&power_param);

    power_set_callback(TCFG_LOWPOWER_LOWPOWER_SEL, sleep_enter_callback, sleep_exit_callback, board_set_soft_poweroff);

    power_keep_dacvdd_en(0);

	power_wakeup_init(&wk_param);
	aport_edge_wkup_set_callback(aport_wakeup_callback);
	port_edge_wkup_set_callback(port_wakeup_callback);

#if USER_UART_UPDATE_ENABLE
        {
#include "uart_update.h"
            uart_update_cfg  update_cfg = {
                .rx = IO_PORTA_02,
                .tx = IO_PORTA_03,
                .output_channel = CH1_UT1_TX,
                .input_channel = INPUT_CH0
            };
            uart_update_init(&update_cfg);
        }
#endif
}

static int board_power_wakeup_init(void)
{
    power_wakeup_init(&wk_param);
#if TCFG_POWER_ON_NEED_KEY
    extern u8 power_reset_src;
    if (is_reset_source(P33_VDDIO_POR_RST) || is_reset_source(P33_VDDIO_LVD_RST)) {
#if TCFG_CHARGE_ENABLE
        log_info("is ldo5v wakeup:%d\n",is_ldo5v_wakeup());
        if (is_ldo5v_wakeup()) {
            return 0;
        }
        if (get_ldo5v_online_hw()) {
            return 0;
        }
#endif
        power_set_callback(TCFG_LOWPOWER_LOWPOWER_SEL, sleep_enter_callback, sleep_exit_callback, board_set_soft_poweroff);
        power_set_soft_poweroff();
    }
#endif
	return 0;
}
/* early_initcall(board_power_wakeup_init); */
#endif /* #ifdef CONFIG_BOARD_AC7012A_DEMO*/
