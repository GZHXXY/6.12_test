#include "app_config.h"
#include "system/includes.h"
#include "asm/iic_hw.h"
#include "asm/iic_soft.h"
#include "asm/gpio.h"
#include "asm/mcpwm.h"
#include "bl6133.h"

#if TCFG_TOUCH_PANEL_ENABLE


#if TCFG_TP_BL6133_ENABLE
#define     SLAVE_DEV_ADDR		(0x2C<<1) //固有，不可配置，用于 chipID 读取，固件烧录操作

#ifndef abs
#define abs(x)  ((x)>0?(x):-(x) )
#endif

/*************Betterlife ic debug***********/
/* #define LOG_TAG_CONST       BL6133 */
#define LOG_TAG             "[bl6133]"
#include "debug.h"
#if defined(BL_DEBUG)
/* #define bl_log_trace        log_info */
#endif
void delay(volatile u32 t);
void udelay(u32 us);
#define   MDELAY(n)	    mdelay(n)

#define log_info(fmt, ...)

#include "ui/ui.h"
#include "ui/ui_api.h"

/*************Betterlife ic iic select***********/
#if TCFG_BL6133_USER_IIC_TYPE
#define iic_init(iic)                       hw_iic_init(iic)
#define iic_uninit(iic)                     hw_iic_uninit(iic)
#define iic_start(iic)                      hw_iic_start(iic)
#define iic_stop(iic)                       hw_iic_stop(iic)
#define iic_tx_byte(iic, byte)              hw_iic_tx_byte(iic, byte)
#define iic_rx_byte(iic, ack)               hw_iic_rx_byte(iic, ack)
#define iic_read_buf(iic, buf, len)         hw_iic_read_buf(iic, buf, len)
#define iic_write_buf(iic, buf, len)        hw_iic_write_buf(iic, buf, len)
#define iic_suspend(iic)                    hw_iic_suspend(iic)
#define iic_resume(iic)                     hw_iic_resume(iic)
#else
#define iic_init(iic)                       soft_iic_init(iic)
#define iic_uninit(iic)                     soft_iic_uninit(iic)
#define iic_start(iic)                      soft_iic_start(iic)
#define iic_stop(iic)                       soft_iic_stop(iic)
#define iic_tx_byte(iic, byte)              soft_iic_tx_byte(iic, byte)
#define iic_rx_byte(iic, ack)               soft_iic_rx_byte(iic, ack)
#define iic_read_buf(iic, buf, len)         soft_iic_read_buf(iic, buf, len)
#define iic_write_buf(iic, buf, len)        soft_iic_write_buf(iic, buf, len)
#define iic_suspend(iic)                    soft_iic_suspend(iic)
#define iic_resume(iic)                     soft_iic_resume(iic)
#endif

static bl6133_param *bl6133_iic_info;
//return =len:ok; other:fail
u16 CTP_FLASH_I2C_WRITE(u8 slave_addr, u8 *buf, u16 len)
{
    u16 i = 0;
    iic_start(bl6133_iic_info->iic_hdl);
    if (0 == iic_tx_byte(bl6133_iic_info->iic_hdl, slave_addr)) {
        iic_stop(bl6133_iic_info->iic_hdl);
        return 0;
    }

    for (i = 0; i < len; i++) {
        delay(bl6133_iic_info->iic_delay);
        if (0 == iic_tx_byte(bl6133_iic_info->iic_hdl, buf[i])) {
            iic_stop(bl6133_iic_info->iic_hdl);
            return i;
        }
    }
    iic_stop(bl6133_iic_info->iic_hdl);
    return i;
}

//return =len:ok; other:fail
u16 CTP_FLASH_I2C_READ(u8 slave_addr, u8 *buf, u16 len)
{
    u16 i = 0;
    iic_start(bl6133_iic_info->iic_hdl);
    if (0 == iic_tx_byte(bl6133_iic_info->iic_hdl, slave_addr + 1)) {
        iic_stop(bl6133_iic_info->iic_hdl);
        return 0;
    }
    for (i = 0; i < len - 1; i++) {
        delay(bl6133_iic_info->iic_delay);
        *buf++ = iic_rx_byte(bl6133_iic_info->iic_hdl, 1);
    }
    i++;
    *buf = iic_rx_byte(bl6133_iic_info->iic_hdl, 0);
    iic_stop(bl6133_iic_info->iic_hdl);
    return i;
}

void ctp_delay_us(unsigned int time)
{
    udelay(time);
}
void ctp_delay_ms(unsigned short time)
{
    mdelay(time);
}


void bl_ts_set_intmode(char mode)
{
    if (0 == mode) {
        //GPIO mode
        CTP_SET_I2C_EINT_OUTPUT;
    } else if (1 == mode) {
        //INT mode
        CTP_SET_I2C_EINT_INPUT;
    }
}

void bl_ts_set_intup(char level)
{
    if (level == 1) {
        CTP_SET_I2C_EINT_HIGH;
    } else if (level == 0) {
        CTP_SET_I2C_EINT_LOW;
    }
}

#ifdef INT_PIN_WAKEUP
void bl_ts_int_wakeup(void)
{
    bl_ts_set_intmode(0);
    bl_ts_set_intup(1);
    ctp_delay_ms(20);
    bl_ts_set_intup(0);
    ctp_delay_ms(1);
    bl_ts_set_intup(1);
    ctp_delay_ms(20);
    bl_ts_set_intmode(1);
}
#endif

#ifdef  RESET_PIN_WAKEUP
void bl_ts_reset_wakeup(void)
{
    //    CTP_SET_RESET_PIN_OUTPUT;
    CTP_SET_RESET_PIN_LOW;
    ctp_delay_ms(1);
    CTP_SET_RESET_PIN_HIGH;
    ctp_delay_ms(20);
    CTP_SET_RESET_PIN_LOW;
    ctp_delay_ms(20);
    CTP_SET_RESET_PIN_HIGH;
    //	ctp_delay_ms(20);
}
void bl_ts_reset(void)
{
    //    CTP_SET_RESET_PIN_OUTPUT;
    CTP_SET_RESET_PIN_LOW;
    ctp_delay_ms(10);
    CTP_SET_RESET_PIN_HIGH;
    ctp_delay_ms(100);
}
#endif

int Check_bl6133_id(unsigned char *buf)
{
    int ret = 0x00;
    for (uint8_t t = 0; t < 3; t++) {
        bl_ts_reset();
        bl_ts_set_intup(0);
        ctp_delay_ms(50);
        uint8_t sendata[] = {0x0a, 0xf5, 0xe7, 0xe8};
        ret = CTP_FLASH_I2C_WRITE(SLAVE_DEV_ADDR, sendata, 4);
        if (ret != 4) {
            log_error("bl_get_chip_id:i2c write flash error___\n");
            ret = -1;
        }
        ret = CTP_FLASH_I2C_READ(SLAVE_DEV_ADDR, buf, 1);
        if (ret != 1) {
            log_error("bl_get_chip_id:i2c read flash error___\n");
            ret = -1;
        }
        if (*buf == 0x22) {
            bl_ts_reset_wakeup();
            log_info("TP_BL6133_CHIP_ID = 0x%02x, check: Pass\r\n", *buf);
            return 0;
        }
        ctp_delay_ms(20);
    }
    return ret;
}

unsigned char ctp_bl_ts_init(void)
{
    uint32_t s;
#ifdef  BTL_CHECK_CHIPID
    unsigned char chipID = 0x00;
#endif
    char  ret = 0;
#ifdef  RESET_PIN_WAKEUP
#ifndef BTL_CHECK_CHIPID // 读id里面有reset
    //bl_ts_reset_wakeup();
    bl_ts_reset();
#endif /* #ifndef BTL_CHECK_CHIPID */
#endif
    /* #ifdef INT_PIN_WAKEUP */
    /*     bl_ts_int_wakeup(); */
    /* #endif */
    MDELAY(20);

#ifdef  BTL_CHECK_CHIPID
    bl_ts_set_intmode(0);
    bl_ts_set_intup(0);
    //ret = bl_get_chip_id(&chipID);
    ret = Check_bl6133_id(&chipID);
    bl_ts_set_intup(1);
    bl_ts_set_intmode(1);
    if (ret < 0) {
        log_error("I2C communication ERROR!");
        return false;
    } else if (chipID != BTL_FLASH_ID) {
        log_error("Please specify the IC model:0x%x!", chipID);
        return false;
    } else {
        log_info("I2C communication success:chipID = %x!", chipID);
    }
    MDELAY(10);
#endif
#ifdef  BL_AUTO_UPDATE_FARMWARE
    extern int bl_auto_update_fw(void);
    //    s = os_lock();////////
    wdt_disable();
    bl_ts_set_intmode(0);
    bl_ts_set_intup(0);
    bl_auto_update_fw();
    ctp_delay_ms(5);
    bl_ts_set_intup(1);
    bl_ts_set_intmode(1);
    wdt_enable();
    //    os_unlock(s);////////
#endif
    return true;
}

//iic scl:<=200KHz
bool ctp_init_bl6133(void *priv)
{
    if (priv == NULL) {
        log_error("ctp bl6133 priv ERROR!");
        return false;
    }
    bl6133_iic_info = (bl6133_param *)priv;
    bool ret = false;

    log_info("the iic speed of ctp-bl6133 must be set below 200k(iic<=200k)!");
    bl_ts_set_intmode(1);
    CTP_SET_RESET_PIN_OUTPUT;
    /* iic_init(bl6133_iic_info->iic_hdl); */
    ret = ctp_bl_ts_init();
    return ret;
}


void ctp_sleep_bl6133(void)
{
    unsigned char data = 0;
    unsigned char aa = 0, bb = 0;

    unsigned char sleepCmd[] = {0xa5, 0x03};
    CTP_FLASH_I2C_WRITE(SLAVE_DEV_ADDR, sleepCmd, 1);
    CTP_FLASH_I2C_READ(SLAVE_DEV_ADDR, &aa, 1);

    MDELAY(10);
    bb = CTP_FLASH_I2C_WRITE(SLAVE_DEV_ADDR, sleepCmd, sizeof(sleepCmd));
    log_info("ctp_sleep_bl6133 aa=%d,bb=%d", aa, bb);
    // MDELAY(100); //和显示屏共用delay耗时

    /* #ifdef INT_PIN_WAKEUP */
    /*     bl_ts_set_intmode(0); */
    /*     bl_ts_set_intup(1); */
    /* #endif */

    log_info("ctp_sleep_bl6133");
}

void ctp_wakeup_bl6133(void)
{
#ifdef  RESET_PIN_WAKEUP
    bl_ts_reset_wakeup();
#endif
    /* #ifdef INT_PIN_WAKEUP */
    /*     bl_ts_int_wakeup(); */
    /* #endif */
    bl_ts_set_intmode(1);//开io中断
    log_info("ctp_wakeup_bl6133");
}

//void ctp_pad_enter_dlps_bl6133(void){
//	Pad_Config(ABH_DRV_PIN_NUM_TP_INT, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
//	Pad_Config(ABH_DRV_PIN_NUM_TP_RST, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
//}

//void ctp_pad_exit_dlps_bl6133(void){
//	Pad_Config(ABH_DRV_PIN_NUM_TP_INT, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_DISABLE, PAD_OUT_LOW);
//  Pad_Config(ABH_DRV_PIN_NUM_TP_RST, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
//}


void ctp_enter_low_power(void)
{
    unsigned char data = 0;
    unsigned char aa = 0, bb = 0;

    unsigned char sleepCmd[] = {0xa5, 0x01};
    CTP_FLASH_I2C_WRITE(CTP_SLAVE_ADDR, sleepCmd, 1);
    CTP_FLASH_I2C_READ(CTP_SLAVE_ADDR, &aa, 1);

    MDELAY(10);
    bb = CTP_FLASH_I2C_WRITE(CTP_SLAVE_ADDR, sleepCmd, sizeof(sleepCmd));
    log_info("ctp_enter_low_power aa=%d,bb=%d", aa, bb);
    // MDELAY(100); //和显示屏共用delay耗时

    log_info("ctp_enter_low_power");
}

void ctp_exit_low_power(void)
{
#ifdef  RESET_PIN_WAKEUP
    bl_ts_reset_wakeup();
#endif
    /* #ifdef INT_PIN_WAKEUP */
    /*     bl_ts_int_wakeup(); */
    /* #endif */
    log_info("ctp_exit_low_power");
}

void ctp_enter_sleep(void)
{
    ctp_sleep_bl6133();
}
void ctp_exit_sleep(void)
{
    ctp_wakeup_bl6133();
}

#define ENERGY_XDISTANCE	15
#define ENERGY_YDISTANCE	15
#define ENERGY_TIME			20 //ms
struct touch_kinetic_energy {
    int t1;
    int x1;
    int y1;

    int t2;
    int x2;
    int y2;
};
static struct touch_kinetic_energy tke = {0};
static void tke_start(int x, int y)
{
    memcpy(&tke.t2, &tke.t1, sizeof(tke) / 2);

    if ((tke.x1 == x) && (tke.y1 == y)) {
        tke.t1 = tke.t1;
    } else {
        tke.t1 = jiffies_msec();
    }
    tke.x1 = x;
    tke.y1 = y;
    /* printf("tke start %d, %d, %d\n", tke.t1, tke.x1, tke.y1); */
}

static void tke_stop(int x, int y)
{
    struct touch_event t;
    u8 xdir, ydir;

    if (tke.t1 == 0) {
        return;
    }
    tke.t1 = tke.t1 - tke.t2;
    tke.x1 = tke.x1 - tke.x2;
    tke.y1 = tke.y1 - tke.y2;

    /* printf("tke end %d, %d, %d\n", tke.t1, tke.x1, tke.y1); */

    if ((tke.t1 < ENERGY_TIME) &&
        ((abs(tke.x1) > ENERGY_XDISTANCE) || abs(tke.y1) > ENERGY_YDISTANCE)) {

        if (abs(tke.x1) <= ENERGY_XDISTANCE) {
            tke.x1 = 0;
        }
        if (abs(tke.y1) <= ENERGY_YDISTANCE) {
            tke.y1 = 0;
        }
        xdir = (tke.x1 < 0) ? 1 : 2;
        ydir = (tke.y1 < 0) ? 1 : 2;
        /* printf("xdir %d, ydir %d\n", xdir, ydir); */

        t.event = ELM_EVENT_TOUCH_ENERGY;
        t.x = (tke.x1 << 16) | (tke.t1 & 0xffff);
        t.y = (tke.y1 << 16) | (ydir << 8) | (xdir & 0xff);
        ui_touch_msg_post(&t);

        /* printf("tke out %d, %d, %d, %d, %d\n", (t.x & 0xffff), (t.x >> 16), (t.y >> 16), (t.y & 0xff), (t.y >> 8) & 0xff); */

    }

    memset(&tke, 0, sizeof(tke));

}


struct touch_info {
    int y;
    int x;
    int p;
    int id;
    int count;
};

#define Y_MIRROR  0
#define X_MIRROR  0
#define VK_Y      		454
#define VK_X      		454
#define VK_Y_MOVE   	100//227
#define VK_X_MOVE      	100//227
#define VK_X_Y_DIFF    	47

/* struct touch_event { */
/* 	int event; */
/* 	int x; */
/* 	int y; */
/* }; */


static u8 tp_last_staus = ELM_EVENT_TOUCH_UP;
static int tp_down_cnt = 0;

#define abs(x)  ((x)>0?(x):-(x) )

static void tpd_down(int raw_x, int raw_y, int x, int y, int p)
{
    struct touch_event t;
    static int first_x = 0;
    static int first_y = 0;
    static u8 move_flag = 0;

    if (x < 0) {
        x = 0;
    }
    if (x > (VK_X - 1)) {
        x = VK_X - 1;
    }
    if (y < 0) {
        y = 0;
    }
    if (y > (VK_Y - 1)) {
        y = VK_Y - 1;
    }
#if Y_MIRROR
    x = VK_X - x - 1;
#endif

#if X_MIRROR
    y = VK_Y - y - 1;
#endif

    if ((tp_last_staus == ELM_EVENT_TOUCH_DOWN) && (x == first_x) && (y == first_y)) {
        tp_down_cnt++;
        if (tp_down_cnt < 30) {
            return;
        }
        tp_last_staus = ELM_EVENT_TOUCH_HOLD;
        tp_down_cnt = 0;

        t.event = tp_last_staus;
        t.x = x;
        t.y = y;
        log_info("----tpd_hold----");
        ui_touch_msg_post(&t);
        return;
    }

    /* log_info("D[%4d %4d %4d]\n", x, y, p); */
    if (tp_last_staus != ELM_EVENT_TOUCH_UP) {
        int x_move = abs(x - first_x);
        int y_move = abs(y - first_y);

        if (!move_flag && (x_move >= VK_X_MOVE || y_move >= VK_Y_MOVE) && (abs(x_move - y_move) >= VK_X_Y_DIFF)) {
            if (x_move > y_move) {
                if (x > first_x) {
                    tp_last_staus = ELM_EVENT_TOUCH_R_MOVE;
                    log_info("----tpd_rigth_move----");
                } else {
                    tp_last_staus = ELM_EVENT_TOUCH_L_MOVE;
                    log_info("----tpd_left_move----");
                }

            } else {

                if (y > first_y) {
                    tp_last_staus = ELM_EVENT_TOUCH_D_MOVE;
                    log_info("----tpd_down_move----");
                } else {
                    tp_last_staus = ELM_EVENT_TOUCH_U_MOVE;
                    log_info("----tpd_up_move----");
                }
            }
            move_flag = 1;
        } else {
            if ((x == first_x) && (y == first_y)) {
                return;
            }
            tp_last_staus = ELM_EVENT_TOUCH_MOVE;
            log_info("----tpd_move----");
        }
        /* tp_last_staus = ELM_EVENT_TOUCH_HOLD; */
        tke_start(x, y);
    } else {
        tp_last_staus = ELM_EVENT_TOUCH_DOWN;
        first_x = x;
        first_y = y;
        move_flag = 0;
        log_info("----tpd_down----");
    }

    t.event = tp_last_staus;
    t.x = x;
    t.y = y;
    ui_touch_msg_post(&t);
}

static void tpd_up(int raw_x, int raw_y, int x, int y, int p)
{
    struct touch_event t;

    if (x < 0) {
        x = 0;
    }
    if (x > (VK_X - 1)) {
        x = VK_X - 1;
    }
    if (y < 0) {
        y = 0;
    }
    if (y > (VK_Y - 1)) {
        y = VK_Y - 1;
    }

#if Y_MIRROR
    x = VK_X - x - 1;
#endif

#if X_MIRROR
    y = VK_Y - y - 1;
#endif

    /* log_info("U[%4d %4d %4d]\n", x, y, 0); */
    tp_last_staus = ELM_EVENT_TOUCH_UP;
    tp_down_cnt = 0;
    t.event = tp_last_staus;
    t.x = x;
    t.y = y;
    log_info("----tpd_up----");
    ui_touch_msg_post(&t);

    tke_stop(x, y);
}


u8 get_ctp_bl6133_info(struct touch_info *cinfo)
{
    static uint8_t keep_times = 0;
    uint8_t  lvalue[2 + MAX_POINT_NUM * 6];
    uint16_t pdwSampleX, pdwSampleY;
    unsigned char cmdPoint = 0x01;


    CTP_FLASH_I2C_WRITE(SLAVE_DEV_ADDR, &cmdPoint, sizeof(cmdPoint));
    CTP_FLASH_I2C_READ(SLAVE_DEV_ADDR,  lvalue, sizeof(lvalue));

    pdwSampleX = (((unsigned short)(lvalue[2] & 0x0f)) << 8) | lvalue[3];
    pdwSampleY = (((unsigned short)(lvalue[4] & 0x0f)) << 8) | lvalue[5];

    //BI6133_evt_handle(((lvalue[2]&0xc0)>>6),lvalue[0],pdwSampleX,pdwSampleY);
    uint8_t status;
    u8 touchAction = 0;
    status = ((lvalue[2] & 0xc0) >> 6);

    /* log_info("coordinate move:%d statu:%d , x=%d, y=%d",lvalue[0],status, pdwSampleX, pdwSampleY); */
    cinfo->p = status; //event flag
    cinfo->x = pdwSampleX;
    cinfo->y = pdwSampleY;
    //    if (status == 1) { // Touched
    /* if (status == 0 || status == 2) { // Touched */
    /* 	if (gTouched) */
    /* 		touchAction = CD_TOUCH_MOVE; */
    /* 	else */
    /* 		touchAction = CD_TOUCH_DOWN; */
    /* 	gTouched = true; */
    /* } else { // Not touched */
    /* 	if (gTouched) */
    /* 		touchAction = CD_TOUCH_UP; */
    /* 	else */
    /* 		touchAction = CD_TOUCH_NONE; */
    /* 	gTouched = false; */
    /* } */
    /* if (touchAction != CD_TOUCH_NONE) {	 */
    /* 	//log_info("touchAction=%d", touchAction); */
    /* } */
    return true;
}



static bool touch_flag = 0;
static bool touch_event_polling_flag = 0; //bl6133不支持
bool BL6133_ID_G_MODE = 1; //bl6133不支持
static int touch_event_handler();
#ifdef CONFIG_CPU_BR23
static int touch_int_handler()
{
    touch_flag = 1;
    if (BL6133_ID_G_MODE) {
        touch_event_handler();
    } else {
        touch_event_polling_flag = 1;
    }
    return 0;
}
#elif defined(CONFIG_CPU_BR28)||defined(CONFIG_CPU_BR27)

static void touch_int_handler(u8 index, u8 gpio)
{
    if (gpio != CTP_EINT_PIN) {
        return ;
    }

    touch_flag = 1;
    if (BL6133_ID_G_MODE) {
        touch_event_handler();
    } else {
        touch_event_polling_flag = 1;
    }
}
#endif


static int touch_event_handler()
{
    struct touch_info cinfo;
    int i = 0;
    if (!touch_flag) {
        return 0;
    }
    touch_flag = 0;

    if (get_ctp_bl6133_info(&cinfo)) {
        if (cinfo.p == 0 || cinfo.p == 2) {
            tpd_down(cinfo.x, cinfo.y, cinfo.x, cinfo.y, 0);
        } else {
            tpd_up(cinfo.x, cinfo.y, cinfo.x, cinfo.y, 0);
        }
//fun test
        log_info("coordinate move: statu: , x=%d, y=%d", cinfo.x, cinfo.y);
        /* if(cinfo.x>400){ */
        /* 	ctp_enter_low_power(); */
        /* }else if(cinfo.x<50){ */
        /* 	ctp_exit_low_power(); */
        /* } */
        /* if(cinfo.y>400){ */
        /* 	ctp_sleep_bl6133(); */
        /* }else if(cinfo.y<50){ */
        /* 	ctp_wakeup_bl6133(); */
        /* } */
    }
    return 0;
}
/* void touch_event_polling() */
/* { */
/* 	if(touch_event_polling_flag){ */
/* 		touch_flag = 1; */
/* 		touch_event_handler(); */
/* 		if(BL6133_INT_R()==1){ */
/* 			touch_event_polling_flag=0; */
/* 		} */
/* 	} */
/* } */
/* int BL6133_id_g_mode(u8 mode)//1:trigger, 0:polling */
/* { */
/* 	u8 data_w[2]={FTS_REG_ID_G_MODE,mode}; */
/* 	int res =0; */
/* 	res=tpd_i2c_write(data_w, 2); */
/* 	if(res!=2){ */
/* 		log_error("set id_g_mode fail!"); */
/* 	} */
/* 	return res; */
/* } */



void ui_set_touch_event(void (*touch_event)(void *priv), int interval);

extern void port_wkup_interrupt_init(u32 port, u8 trigger_edge, void (*cbfun)(void));
void bl6133_int_en()
{
    /* log_info("-------------------port wkup isr---------------------------"); */
#ifdef CONFIG_CPU_BR23
    io_ext_interrupt_init(CTP_EINT_PIN, 1, touch_int_handler);
#elif defined(CONFIG_CPU_BR28)||defined(CONFIG_CPU_BR27)
    // br28外部中断回调函数，按照现在的外部中断注册方式
    // io配置在板级，定义在板级头文件，这里只是注册回调函数
    port_edge_wkup_set_callback(touch_int_handler);
#endif
    /* if (BL6133_ID_G_MODE == 0) { */
    /*     //定时执行touch_event_polling(); */
    /*     sys_timer_add((void *)NULL, touch_event_polling, 20); //注册按键扫描定时器 */
    /* } */

    /* sys_timer_add((void *)NULL, touch_event_handler, 20); //注册按键扫描定时器 */
#if TCFG_UI_ENABLE
    ui_set_touch_event(NULL, 0);
#endif
}




/****************************************************/
bl6133_param bl6133_iic_info_test = {
    .iic_hdl = 0,
    .iic_delay = 1,
};
/*************************************
 *注意：
 *    bl6133触摸屏iic时钟频率必须小于200k
 * **********************************/
void bl6133_test()
{
    iic_init(bl6133_iic_info_test.iic_hdl);
    if (ctp_init_bl6133(&bl6133_iic_info_test) == true) {
        /* bl6133_int_en(); */
        log_info("bl6133 init ok!");
    } else {
        log_error("bl6133 init fail!");
    }
}

/**************************test**************************/
#if 0
void bl6133_test1()
{
    iic_init(bl6133_iic_info_test.iic_hdl);

    if (ctp_init_bl6133(&bl6133_iic_info_test) == true) {
        log_info("-------------------port wkup isr---------------------------");
        bl6133_int_en();
        /* port_wkup_enable(IO_PORTA_04, 1, touch_int_handler); //PA08-IO中断，1:下降沿触发，回调函数port_wkup_irq_cbfun_test */
        log_info("bl6133 init ok!");
    } else {
        log_error("bl6133 init fail!");
    }
}

#endif
#endif

#endif


