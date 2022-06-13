#ifndef __IMD_H__
#define __IMD_H__


//~~~~~~~~~~~~~~~~~~~~~~~spi接口配置~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// #define LCD_DRIVE_CONFIG	SPI_4WIRE_RGB565_1T8B
// #define LCD_DRIVE_CONFIG	SPI_3WIRE_RGB666_1T8B
// #define LCD_DRIVE_CONFIG    DSPI_3WIRE_RGB666_1T9B

// #define LCD_DRIVE_CONFIG    QSPI_RGB565_SUBMODE0_1T8B
// #define LCD_DRIVE_CONFIG    QSPI_RGB565_SUBMODE1_1T2B
// #define LCD_DRIVE_CONFIG    QSPI_RGB565_SUBMODE2_1T2B

// #define LCD_DRIVE_CONFIG    QSPI_RGB666_SUBMODE0_1T8B
// #define LCD_DRIVE_CONFIG    QSPI_RGB666_SUBMODE1_1T2B
// #define LCD_DRIVE_CONFIG    QSPI_RGB666_SUBMODE2_1T2B

// #define LCD_DRIVE_CONFIG    QSPI_RGB888_SUBMODE0_1T8B
// #define LCD_DRIVE_CONFIG    QSPI_RGB888_SUBMODE1_1T2B
// #define LCD_DRIVE_CONFIG    QSPI_RGB888_SUBMODE2_1T2B

//~~~~~~~~~~~~~~~~~~~~~~~rgb接口配置~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// #define LCD_DRIVE_CONFIG	RGB_3WIRE_RGB565

//~~~~~~~~~~~~~~~~~~~~~~~pap接口配置~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// #define LCD_DRIVE_CONFIG	PAP_RGB888



//[RGB接口IO使能]
#define IMD_RGB_SYNC0_EN    BIT(0) // hsync/vsync/de
#define IMD_RGB_SYNC1_EN    BIT(1) // hsync/vsync/de
#define IMD_RGB_SYNC2_EN    BIT(2) // hsync/vsync/de
#define IMD_RGB_DCLK_EN     BIT(3)
#define IMD_RGB_DATA0_EN    BIT(4)
#define IMD_RGB_DATA1_EN    BIT(5)
#define IMD_RGB_DATA2_EN    BIT(6)
#define IMD_RGB_DATA3_EN    BIT(7)
#define IMD_RGB_DATA4_EN    BIT(8)
#define IMD_RGB_DATA5_EN    BIT(9)
#define IMD_RGB_DATA6_EN    BIT(10)
#define IMD_RGB_DATA7_EN    BIT(11)


//[PAP接口IO使能]
#define IMD_PAP_SYNC1_EN    BIT(1) // wr/rd
#define IMD_PAP_SYNC2_EN    BIT(2) // wr/rd
#define IMD_PAP_DATA0_EN    BIT(4)
#define IMD_PAP_DATA1_EN    BIT(5)
#define IMD_PAP_DATA2_EN    BIT(6)
#define IMD_PAP_DATA3_EN    BIT(7)
#define IMD_PAP_DATA4_EN    BIT(8)
#define IMD_PAP_DATA5_EN    BIT(9)
#define IMD_PAP_DATA6_EN    BIT(10)
#define IMD_PAP_DATA7_EN    BIT(11)


//[SPI接口IO使能]
#define IMD_SPI_CLK_EN      BIT(12)
#define IMD_SPI_DATA0_EN    BIT(13)
#define IMD_SPI_DATA1_EN    BIT(14)
#define IMD_SPI_DATA2_EN    BIT(15)
#define IMD_SPI_DATA3_EN    BIT(16)


//<<<[lcd接口配置]>>>
#define SPI_MODE  (0<<4)
#define DSPI_MODE (1<<4)
#define QSPI_MODE (2<<4)

#define SPI_WIRE3  0
#define SPI_WIRE4  1

#define QSPI_SUBMODE0 0//0x02
#define QSPI_SUBMODE1 1//0x32
#define QSPI_SUBMODE2 2//0x12

#define PIXEL_1P1T (0<<5)
#define PIXEL_1P2T (1<<5)
#define PIXEL_1P3T (2<<5)
#define PIXEL_2P3T (3<<5)

#define PIXEL_1T2B  1
#define PIXEL_1T6B  5
#define PIXEL_1T8B  7
#define PIXEL_1T9B  8
#define PIXEL_1T12B 11
#define PIXEL_1T16B 15
#define PIXEL_1T18B 17
#define PIXEL_1T24B 23

#define FORMAT_RGB565 1//0//1P2T
#define FORMAT_RGB666 2//1//1P3T
#define FORMAT_RGB888 0//2//1P3T

#define SPI_PORTA 0
#define SPI_PORTB 1



// ~~~~~~~~~~配置选择, spi的配置搭配方式固定
#define SPI_3WIRE_RGB888_1T8B		31//0
#define SPI_3WIRE_RGB888_1T24B		1
#define SPI_3WIRE_RGB666_1T8B		2
#define SPI_3WIRE_RGB666_1T18B		3
#define SPI_3WIRE_RGB565_1T8B		4
#define SPI_3WIRE_RGB565_1T16B		5

#define SPI_4WIRE_RGB888_1T8B		6
#define SPI_4WIRE_RGB888_1T24B		7
#define SPI_4WIRE_RGB666_1T8B		8
#define SPI_4WIRE_RGB666_1T18B		9
#define SPI_4WIRE_RGB565_1T8B		10
#define SPI_4WIRE_RGB565_1T16B		11

#define DSPI_3WIRE_RGB565_1T8B		12
#define DSPI_3WIRE_RGB666_1T9B		13
#define DSPI_3WIRE_RGB666_1T6B		14
#define DSPI_3WIRE_RGB888_1T12B		15
#define DSPI_3WIRE_RGB888_1T8B		16

#define DSPI_4WIRE_RGB565_1T8B		17
#define DSPI_4WIRE_RGB666_1T9B		18
#define DSPI_4WIRE_RGB666_1T6B		19
#define DSPI_4WIRE_RGB888_1T12B		20
#define DSPI_4WIRE_RGB888_1T8B		21

#define QSPI_RGB565_SUBMODE0_1T8B	22
#define QSPI_RGB666_SUBMODE0_1T8B	23
#define QSPI_RGB888_SUBMODE0_1T8B	24

#define QSPI_RGB565_SUBMODE1_1T2B	25
#define QSPI_RGB666_SUBMODE1_1T2B	26
#define QSPI_RGB888_SUBMODE1_1T2B	27

#define QSPI_RGB565_SUBMODE2_1T2B	28
#define QSPI_RGB666_SUBMODE2_1T2B	29
#define QSPI_RGB888_SUBMODE2_1T2B	30

// mcu屏和rgb屏
#define RGB_3WIRE_RGB565			0xf0
#define PAP_RGB888					0xf1


#if 0
// ~~~~~~~~~~配置选择结束，以下为具体配置
// 注意：以下配置组合为固定搭配，不可随意更改
// 切换配置使用上面的宏选择
////////////////////////////////spi 3wire///////////////////////////
#if (LCD_DRIVE_CONFIG == SPI_3WIRE_RGB888_1T8B)
#define SPI_IF_MODE     (SPI_MODE|SPI_WIRE3)
#define OUT_FORMAT      FORMAT_RGB888
#define PIXEL_TYPE      (PIXEL_1P3T|PIXEL_1T8B)

#elif (LCD_DRIVE_CONFIG == SPI_3WIRE_RGB888_1T24B)
#define SPI_IF_MODE     (SPI_MODE|SPI_WIRE3)
#define OUT_FORMAT      FORMAT_RGB888
#define PIXEL_TYPE      (PIXEL_1P1T|PIXEL_1T24B)

#elif (LCD_DRIVE_CONFIG == SPI_3WIRE_RGB666_1T8B)
#define SPI_IF_MODE     (SPI_MODE|SPI_WIRE3)
#define OUT_FORMAT      FORMAT_RGB666
#define PIXEL_TYPE      (PIXEL_1P3T|PIXEL_1T8B)

#elif (LCD_DRIVE_CONFIG == SPI_3WIRE_RGB666_1T18B)
#define SPI_IF_MODE     (SPI_MODE|SPI_WIRE3)
#define OUT_FORMAT      FORMAT_RGB666
#define PIXEL_TYPE      (PIXEL_1P1T|PIXEL_1T18B)

#elif (LCD_DRIVE_CONFIG == SPI_3WIRE_RGB565_1T8B)
#define SPI_IF_MODE     (SPI_MODE|SPI_WIRE3)
#define OUT_FORMAT      FORMAT_RGB565
#define PIXEL_TYPE      (PIXEL_1P2T|PIXEL_1T8B)

#elif (LCD_DRIVE_CONFIG == SPI_3WIRE_RGB565_1T16B)
#define SPI_IF_MODE     (SPI_MODE|SPI_WIRE3)
#define OUT_FORMAT      FORMAT_RGB565
#define PIXEL_TYPE      (PIXEL_1P1T|PIXEL_1T16B)


////////////////////////////////spi 4wire///////////////////////////
#elif (LCD_DRIVE_CONFIG == SPI_4WIRE_RGB888_1T8B)
#define SPI_IF_MODE     (SPI_MODE|SPI_WIRE4)
#define OUT_FORMAT      FORMAT_RGB888
#define PIXEL_TYPE      (PIXEL_1P3T|PIXEL_1T8B)

#elif (LCD_DRIVE_CONFIG == SPI_4WIRE_RGB888_1T24B)
#define SPI_IF_MODE     (SPI_MODE|SPI_WIRE4)
#define OUT_FORMAT      FORMAT_RGB888
#define PIXEL_TYPE      (PIXEL_1P1T|PIXEL_1T24B)

#elif (LCD_DRIVE_CONFIG == SPI_4WIRE_RGB666_1T8B)
#define SPI_IF_MODE     (SPI_MODE|SPI_WIRE4)
#define OUT_FORMAT      FORMAT_RGB666
#define PIXEL_TYPE      (PIXEL_1P3T|PIXEL_1T8B)

#elif (LCD_DRIVE_CONFIG == SPI_4WIRE_RGB666_1T18B)
#define SPI_IF_MODE     (SPI_MODE|SPI_WIRE4)
#define OUT_FORMAT      FORMAT_RGB666
#define PIXEL_TYPE      (PIXEL_1P1T|PIXEL_1T18B)

#elif (LCD_DRIVE_CONFIG == SPI_4WIRE_RGB565_1T8B)
#define SPI_IF_MODE     (SPI_MODE|SPI_WIRE4)
#define OUT_FORMAT      FORMAT_RGB565
#define PIXEL_TYPE      (PIXEL_1P2T|PIXEL_1T8B)

#elif (LCD_DRIVE_CONFIG == SPI_4WIRE_RGB565_1T16B)
#define SPI_IF_MODE     (SPI_MODE|SPI_WIRE4)
#define OUT_FORMAT      FORMAT_RGB565
#define PIXEL_TYPE      (PIXEL_1P1T|PIXEL_1T16B)

////////////////////////////////dspi 3wire///////////////////////////
#elif (LCD_DRIVE_CONFIG == 	DSPI_3WIRE_RGB565_1T8B)
#define SPI_IF_MODE     (DSPI_MODE|SPI_WIRE3)
#define OUT_FORMAT      FORMAT_RGB565
#define PIXEL_TYPE      (PIXEL_1P1T|PIXEL_1T8B)

#elif (LCD_DRIVE_CONFIG == 	DSPI_3WIRE_RGB666_1T9B)
#define SPI_IF_MODE     (DSPI_MODE|SPI_WIRE3)
#define OUT_FORMAT      FORMAT_RGB666
#define PIXEL_TYPE      (PIXEL_1P1T|PIXEL_1T9B)

#elif (LCD_DRIVE_CONFIG == 	DSPI_3WIRE_RGB666_1T6B)
#define SPI_IF_MODE     (DSPI_MODE|SPI_WIRE3)
#define OUT_FORMAT      FORMAT_RGB666
#define PIXEL_TYPE      (PIXEL_2P3T|PIXEL_1T6B)

#elif (LCD_DRIVE_CONFIG == 	DSPI_3WIRE_RGB888_1T12B)
#define SPI_IF_MODE     (DSPI_MODE|SPI_WIRE3)
#define OUT_FORMAT      FORMAT_RGB888
#define PIXEL_TYPE      (PIXEL_1P1T|PIXEL_1T12B)

#elif (LCD_DRIVE_CONFIG == 	DSPI_3WIRE_RGB888_1T8B)
#define SPI_IF_MODE     (DSPI_MODE|SPI_WIRE3)
#define OUT_FORMAT      FORMAT_RGB888
#define PIXEL_TYPE      (PIXEL_2P3T|PIXEL_1T8B)

//////////////////////////////dspi 4wire///////////////////////////
#elif (LCD_DRIVE_CONFIG == 	DSPI_4WIRE_RGB565_1T8B)
#define SPI_IF_MODE     (DSPI_MODE|SPI_WIRE4)
#define OUT_FORMAT      FORMAT_RGB565
#define PIXEL_TYPE      (PIXEL_1P1T|PIXEL_1T8B)

#elif (LCD_DRIVE_CONFIG == DSPI_4WIRE_RGB666_1T9B)
#define SPI_IF_MODE     (DSPI_MODE|SPI_WIRE4)
#define OUT_FORMAT      FORMAT_RGB666
#define PIXEL_TYPE      (PIXEL_1P1T|PIXEL_1T9B)

#elif (LCD_DRIVE_CONFIG == DSPI_4WIRE_RGB666_1T6B)
#define SPI_IF_MODE     (DSPI_MODE|SPI_WIRE4)
#define OUT_FORMAT      FORMAT_RGB666
#define PIXEL_TYPE      (PIXEL_2P3T|PIXEL_1T6B)

#elif (LCD_DRIVE_CONFIG == DSPI_4WIRE_RGB888_1T12B)
#define SPI_IF_MODE     (DSPI_MODE|SPI_WIRE4)
#define OUT_FORMAT      FORMAT_RGB888
#define PIXEL_TYPE      (PIXEL_1P1T|PIXEL_1T12B)

#elif (LCD_DRIVE_CONFIG == DSPI_4WIRE_RGB888_1T8B)
#define SPI_IF_MODE     (DSPI_MODE|SPI_WIRE4)
#define OUT_FORMAT      FORMAT_RGB888
#define PIXEL_TYPE      (PIXEL_2P3T|PIXEL_1T8B)

//////////////////////////////qspi///////////////////////////
#elif (LCD_DRIVE_CONFIG == QSPI_RGB565_SUBMODE0_1T8B)
#define SPI_IF_MODE     (QSPI_MODE|QSPI_SUBMODE0)
#define OUT_FORMAT      FORMAT_RGB565
#define PIXEL_TYPE      (PIXEL_1P2T|PIXEL_1T8B)

#elif (LCD_DRIVE_CONFIG == QSPI_RGB666_SUBMODE0_1T8B)
#define SPI_IF_MODE     (QSPI_MODE|QSPI_SUBMODE0)
#define OUT_FORMAT      FORMAT_RGB666
#define PIXEL_TYPE      (PIXEL_1P3T|PIXEL_1T8B)

#elif (LCD_DRIVE_CONFIG == QSPI_RGB888_SUBMODE0_1T8B)
#define SPI_IF_MODE     (QSPI_MODE|QSPI_SUBMODE0)
#define OUT_FORMAT      FORMAT_RGB888
#define PIXEL_TYPE      (PIXEL_1P3T|PIXEL_1T8B)

#elif (LCD_DRIVE_CONFIG == QSPI_RGB565_SUBMODE1_1T2B)
#define SPI_IF_MODE     (QSPI_MODE|QSPI_SUBMODE1)
#define OUT_FORMAT      FORMAT_RGB565
#define PIXEL_TYPE      (PIXEL_1P2T|PIXEL_1T2B)

#elif (LCD_DRIVE_CONFIG == QSPI_RGB666_SUBMODE1_1T2B)
#define SPI_IF_MODE     (QSPI_MODE|QSPI_SUBMODE1)
#define OUT_FORMAT      FORMAT_RGB666
#define PIXEL_TYPE      (PIXEL_1P3T|PIXEL_1T2B)

#elif (LCD_DRIVE_CONFIG == QSPI_RGB888_SUBMODE1_1T2B)
#define SPI_IF_MODE     (QSPI_MODE|QSPI_SUBMODE1)
#define OUT_FORMAT      FORMAT_RGB888
#define PIXEL_TYPE      (PIXEL_1P3T|PIXEL_1T2B)

#elif (LCD_DRIVE_CONFIG == QSPI_RGB565_SUBMODE2_1T2B)
#define SPI_IF_MODE     (QSPI_MODE|QSPI_SUBMODE2)
#define OUT_FORMAT      FORMAT_RGB565
#define PIXEL_TYPE      (PIXEL_1P2T|PIXEL_1T2B)

#elif (LCD_DRIVE_CONFIG == QSPI_RGB666_SUBMODE2_1T2B)
#define SPI_IF_MODE     (QSPI_MODE|QSPI_SUBMODE2)
#define OUT_FORMAT      FORMAT_RGB666
#define PIXEL_TYPE      (PIXEL_1P3T|PIXEL_1T2B)

#elif (LCD_DRIVE_CONFIG == QSPI_RGB888_SUBMODE2_1T2B)
#define SPI_IF_MODE     (QSPI_MODE|QSPI_SUBMODE2)
#define OUT_FORMAT      FORMAT_RGB888
#define PIXEL_TYPE      (PIXEL_1P3T|PIXEL_1T2B)

//////////////////////////////rgb lcd///////////////////////////
#elif (LCD_DRIVE_CONFIG == RGB_3WIRE_RGB565)
#define SPI_IF_MODE     (SPI_MODE|SPI_WIRE3)
#define OUT_FORMAT      FORMAT_RGB565

//////////////////////////////pap lcd///////////////////////////
#elif (LCD_DRIVE_CONFIG == PAP_RGB888)
#define OUT_FORMAT      FORMAT_RGB888

#else
#error "error! undefined lcd drive!, see imd.h"
#endif
//////////////////////////////lcd mode end///////////////////////////
#endif


#define SPI_MODE_UNIDIR  0//半双工，d0分时发送接收
#define SPI_MODE_BIDIR   1//全双工，d0发送、d1接收



// 函数声明处用到了u8等重定义类型
#include "typedef.h"

// lcd buffer声明处用到了list_head
#include "list.h"


//~~~~~~~~~~~~~~~~~~~~~~~屏驱相关的参数和结构体~~~~~~~~~~~~~~~~~~~~~~~~~~//
// 屏幕初始化代码延时标志
#ifndef REGFLAG_DELAY
#define REGFLAG_DELAY      0xFF
#endif

// 区分屏幕初始化代码开始和结束的标志
#define BEGIN_FLAG          0x55555555
#define END_FLAG            0xaaaaaaaa
#define _BEGIN_             ((BEGIN_FLAG>>24)&0xff),((BEGIN_FLAG>>16)&0xff),((BEGIN_FLAG>>8)&0xff),(BEGIN_FLAG&0xff)
#define _END_               ((END_FLAG>>24)&0xff),((END_FLAG>>16)&0xff),((END_FLAG>>8)&0xff),(END_FLAG&0xff)


// 初始化代码结构体
struct lcd_cmd {
    u8 addr;		// 地址
    u8 param_cnt;	// 参数个数
    u8 param[64];	// 参数
};


// LCD屏幕类型
typedef enum lcd_type_cfg {
    LCD_TYPE_SPI,	// SPI屏
    LCD_TYPE_MCU,	// MCU屏
    LCD_TYPE_RGB,	// RGB屏
} LCD_TYPE;


// 缓存buf的状态
typedef enum lcd_buf_status {
    LCD_BUFFER_INIT,		// 初始状态，防止还没写数据就推屏
    LCD_BUFFER_IDLE,		// 空闲
    LCD_BUFFER_LOCK,		// 锁定
    LCD_BUFFER_PENDING,		// 等待
    LCD_BUFFER_INUSED,		// 使用
} BUF_STATUS;


// lcd buffer
struct lcd_buffer {
    struct list_head entry;
    BUF_STATUS status;
    u8 *baddr;
};


// SPI 自由配置接口
// 在 lcd_drive.c中实现，通过handler注册给imd.c使用
struct spi_api {
    // void (*reset_ctrl)(u8 val);
    void (*dc_ctrl)(u8 val);
    void (*cs_ctrl)(u8 val);
    int (*te_stat)();
    // void (*bl_ctrl)(u8 val);
};


// lcd配置结构体
struct imd_param {
    // 显示区域相关
    int scr_x;
    int scr_y;
    int scr_w;
    int scr_h;

    // lcd配置
    int lcd_width;
    int lcd_height;
    LCD_TYPE lcd_type;

    // 显存配置
    int buffer_num;
    int buffer_size;

    // imd模块的输入，与imb模块一起用时，也是imb模块的输出
    int in_width;
    int in_height;
    int in_format;
    int in_stride;

    // debug模式
    int debug_mode_en;
    int debug_mode_color;

    // te功能
    int te_en;		//TE信号使能
    int te_port;	// te的io

    // 帧率配置
    int fps;

    // 以下为三种屏相关配置，out_format为屏幕的像素格式类型
    struct spi_param {
        int spi_mode;
        int pixel_type;
        int out_format;
        int port;
        int spi_dat_mode;
    } spi;

    struct pap_param {
        int out_format;
        int wr_rd_swap_en;
        int right_shift_2bit;
        int dat_l2h_en;
    } pap;

    struct rgb_param {
        int dat_l2h;
        int dat_sf;
        int hv_mode;
        int xor_edge;
        int out_format;
        int continue_frames;

        int hsync_out;
        int vsync_out;
        int dsync_out;

        int hpw_prd;
        int hbw_prd;
        int hfw_prd;
        int vpw_prd;
        int vbw_prd;
        int vfw_prd;
    } rgb;
};


struct _lcd_drive {
    char *logo;

    u8 column_addr_align;
    u8 row_addr_align;

    // 初始化寄存器
    u8 *lcd_cmd;
    int cmd_cnt;

    // 配置参数
    void *param;

    // 应用层函数
    void (*reset)(void); /* 复位函数 */
    int (*backlight_ctrl)(u8);
    int (*power_ctrl)(u8);
    void (*entersleep)(void);
    void (*exitsleep)(void);
};

struct lcd_buffer_head {
    struct list_head head;
    u8 *pre_buf;
};


#define REGISTER_LCD_DEVICE() \
	struct _lcd_drive lcd_drive

extern struct _lcd_drive lcd_drive;



void imd_spi_tx_cmd(u32 cmd, u8 cmd_cnt);
void imd_spi_tx_dat(u32 para, u8 para_cnt);
void imd_dump();
int imd_lcd_init(int format, struct lcd_cmd *lcd_cmd, int item);
void lcd_clear_screen(u32 color);
int  lcd_init_with_draw(int width, int height, u8 *lcd_buf, int stride, int format, struct lcd_cmd *lcd_cmd, int item);
void lcd_fill_rect(int left, int top, int width, int height, u32 color);
int  lcd_draw_area(u8 index, u8 *lcd_buf, int left, int top, int width, int height, int wait);
int lcd_draw_area_direct(u8 *buf0, u8 *buf1, u8 *lcd_buf, int left, int top, int width, int height, int wait);
void lcd_set_area(u16 xs, u16 xe, u16 ys, u16 ye);
void lcd_buffer_put(u8 *buffer);
// te模式推屏接口，目前测试将接口放到外部调用。后续要在初始化时调用，不给外面用
void te_put_buffer_callback(u8 index, u8 gpio);
int  lcd_busy();

u8 *lcd_buffer_init(u8 index, u8 *baddr, u32 size);
void lcd_buffer_release(u8 index);
u8 *lcd_buffer_get(u8 index, u8 *pre_baddr);
u8 lcd_buffer_pending(u8 index, u8 *buffer);
void lcd_buffer_idle(u8 index);
u8 *lcd_buffer_next(u8 index);
u8 *lcd_buffer_check(u8 index, u8 *lcd_buf);
struct spi_api *get_spi_api_config_handler();

void lcd_write_cmd(u8 cmd, u8 *buf, u8 len);
void lcd_read_cmd(u8 cmd, u8 *buf, u8 len);


u16 get_lcd_width_from_imd(void);
u16 get_lcd_height_from_imd(void);


#endif





