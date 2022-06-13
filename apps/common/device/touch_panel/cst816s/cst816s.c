#include "app_config.h"
#include "system/includes.h"
#include "asm/iic_hw.h"
#include "asm/iic_soft.h"
#include "asm/gpio.h"
#include "asm/mcpwm.h"
#include "ui/ui.h"
#include "ui/ui_api.h"

#if TCFG_TOUCH_PANEL_ENABLE

#if TCFG_TP_CST816S_ENABLE//CST816S_UPGRADE==1

#include "capacitive_hynitron_cst816s_update.h"

#if TCFG_TOUCH_USER_IIC_TYPE
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

typedef struct {
    u8 init;
    hw_iic_dev iic_hdl;
} ft_param;

static ft_param module_param = {0};
#define __this   (&module_param)

#define CST816S_IIC_DELAY   1
#define CST816S_IIC_ADDR  0x46

#if TCFG_LCD_RM69330_QSPI_ENABLE
#define CST816S_RESET_H() gpio_direction_output(IO_PORTA_01,1)
#define CST816S_RESET_L() gpio_direction_output(IO_PORTA_01,0)
#else
#define CST816S_RESET_H() gpio_direction_output(IO_PORTA_02,1)
#define CST816S_RESET_L() gpio_direction_output(IO_PORTA_02,0)
#endif
#define CST816S_INT_IO    (IO_PORTC_01)
#define CST816S_INT_IN()  gpio_direction_input(CST816S_INT_IO);gpio_set_pull_down(CST816S_INT_IO,0);gpio_set_pull_up(CST816S_INT_IO,1);
#define CST816S_INT_R()   gpio_read(CST816S_INT_IO)


#define PER_LEN	512
#define CST816S_UPGRADE_ADD (0x6A) /* 7-bit i2c addr */
#define HYN_I2C_ADDR 0x15 /* 7-bit i2c addr */ //0x38

#define I2C_MASTER_CI2C0 0
#define i2c_t uint8_t
#define mdelay(ms) delay_ms(ms)
#define msleep(ms) delay_ms(ms)

static void delay_ms(int cnt)
{
    int delay = (cnt + 9) / 10;
    os_time_dly(delay);
}

int i2c_write_regs(i2c_t dev, uint16_t address, uint16_t reg,
                   void *data, size_t length, uint8_t flags)
{
    int ret;
    /* printf("i2c_read_regs_ex.\n"); */

    iic_start(__this->iic_hdl);
    if (0 == iic_tx_byte(__this->iic_hdl, address << 1)) {
        iic_stop(__this->iic_hdl);
        return -1;
    }
    delay(CST816S_IIC_DELAY);
    /* ret = iic_write_buf(__this->iic_hdl, reg & 0xff, 1); */
    iic_tx_byte(__this->iic_hdl, reg);
    delay(CST816S_IIC_DELAY);

    delay(CST816S_IIC_DELAY);
    ret = iic_write_buf(__this->iic_hdl, data, length);
    delay(CST816S_IIC_DELAY);
    iic_stop(__this->iic_hdl);
    delay(CST816S_IIC_DELAY);

    return 0;
}

int i2c_read_regs(i2c_t dev, uint16_t address, uint16_t reg,
                  void *data, size_t length, uint8_t flags)
{
    int ret;
    /* printf("i2c_read_regs_ex.\n"); */

    iic_start(__this->iic_hdl);
    if (0 == iic_tx_byte(__this->iic_hdl, address << 1)) {
        iic_stop(__this->iic_hdl);
        return -1;
    }
    delay(CST816S_IIC_DELAY);
    /* ret = iic_write_buf(__this->iic_hdl, reg & 0xff, 1); */
    iic_tx_byte(__this->iic_hdl, reg);
    delay(CST816S_IIC_DELAY);

    iic_start(__this->iic_hdl);
    if (0 == iic_tx_byte(__this->iic_hdl, (address << 1) | 0x01)) {
        iic_stop(__this->iic_hdl);
        return -1;
    }

    delay(CST816S_IIC_DELAY);
    ret = iic_read_buf(__this->iic_hdl, data, length);
    delay(CST816S_IIC_DELAY);
    iic_stop(__this->iic_hdl);
    delay(CST816S_IIC_DELAY);

    return 0;
}

int i2c_read_regs_ex(i2c_t dev, uint16_t address, void *reg,
                     void *data, size_t length, uint8_t flags)
{
    int ret;
    /* printf("i2c_read_regs_ex.\n"); */

    iic_start(__this->iic_hdl);
    if (0 == iic_tx_byte(__this->iic_hdl, address << 1)) {
        iic_stop(__this->iic_hdl);
        return -1;
    }
    delay(CST816S_IIC_DELAY);
    ret = iic_write_buf(__this->iic_hdl, reg, 2);
    delay(CST816S_IIC_DELAY);

    iic_start(__this->iic_hdl);
    if (0 == iic_tx_byte(__this->iic_hdl, (address << 1) | 0x01)) {
        iic_stop(__this->iic_hdl);
        return -1;
    }

    delay(CST816S_IIC_DELAY);
    ret = iic_read_buf(__this->iic_hdl, data, length);
    delay(CST816S_IIC_DELAY);
    iic_stop(__this->iic_hdl);
    delay(CST816S_IIC_DELAY);

    return 0;
}

int i2c_write_regs_ex(i2c_t dev, uint16_t address, void *reg,
                      const void *data, size_t length, uint8_t flags)
{
    int ret;
    int i;
    u8 *_reg = (u8 *)reg;
    u8 *_data = (u8 *)data;

    /* printf("i2c_write_regs_ex. reg:0x%02x%02x\n",_reg[0],_reg[1]); */
    /* printf("data:\n"); */
    /* for(i=0;i<length;i++) { */
    /* printf("%02x ",_data[i]); */
    /* } */
    /* printf("\n"); */
    /* printf("address = 0x%x\n",address<<1); */

    iic_start(__this->iic_hdl);
    if (0 == iic_tx_byte(__this->iic_hdl, address << 1)) {
        iic_stop(__this->iic_hdl);
        printf("iic_tx_byte fail!\n");
        return -1;
    }
    /* ret = iic_write_buf(__this->iic_hdl, reg, 2); */
    iic_tx_byte(__this->iic_hdl, _reg[0]);
    iic_tx_byte(__this->iic_hdl, _reg[1]);
    /* printf("ret = %d\n",ret); */
    delay(CST816S_IIC_DELAY);
    ret = iic_write_buf(__this->iic_hdl, data, length);
    /* printf("ret = %d\n",ret); */
    delay(CST816S_IIC_DELAY);
    iic_stop(__this->iic_hdl);
    delay(CST816S_IIC_DELAY);

    return 0;
}

/*
static uint8_t CST816S_I2C_ReadOneByte(uint8_t reg)
{
    uint8_t retVal = 0;

    i2c_read_reg(I2C_MASTER_CI2C0, CST816S_UPGRADE_ADD, reg, &retVal, 0);

    return retVal;
}

static void CST816S_I2C_Write_buf(uint8_t *buf, uint8_t len)
{
    i2c_write_regs(I2C_MASTER_CI2C0, CST816S_UPGRADE_ADD, buf[0], &buf[1], len - 1, 0);
}

static void CST816S_I2C_Direct_Write_buf(uint8_t *buf, uint8_t len)
{
    i2c_write_bytes(I2C_MASTER_CI2C0, CST816S_UPGRADE_ADD, buf, len, 0);
}

static void CST816S_I2C_Read_buf(uint8_t reg, uint8_t len, uint8_t *buf)
{
    i2c_read_regs(I2C_MASTER_CI2C0, CST816S_UPGRADE_ADD, reg, buf, len, 0);
}

static uint8_t CST816S_I2C_Read_32(uint32_t reg, uint8_t *buf, uint8_t len)
{
    uint8_t reg_addr[4] = {0};

	reg_addr[0] = ( reg >> 24 )& 0xFF;
	reg_addr[1] = ( reg >> 16 )& 0xFF;
	reg_addr[2] = ( reg >> 8 )& 0xFF;
	reg_addr[3] = reg&0xFF;

	i2c_write_bytes(I2C_MASTER_CI2C0, CST816S_UPGRADE_ADD, reg_addr, 4, 0);
	i2c_read_bytes(I2C_MASTER_CI2C0, CST816S_UPGRADE_ADD, buf, len, 0);

    return 0;
}*/

void cst816s_reset();
static int cst816s_enter_bootmode(void)
{
    uint8_t retryCnt = 10;

#if 0
    gpio_output_set(GPIO15, 0);
    uos_sleep(4);/* 20 ms */
    gpio_output_set(GPIO15, 1);
    uos_sleep(1);/* 5ms */
#else
    cst816s_reset();
#endif

    printf("%s %d \r\n", __FUNCTION__, __LINE__);

    while (retryCnt--) {
        uint8_t cmd[3];
        cmd[0] = 0xAB;

        char reg[2] = {0xA0, 0x01};
        if (-1 == i2c_write_regs_ex(I2C_MASTER_CI2C0, CST816S_UPGRADE_ADD, &reg, cmd, 1, 0)) { // enter program mode
            mdelay(2); // 4ms
            continue;
        }

        char reg1[2] = {0xA0, 0x03};
        if (-1 == i2c_read_regs_ex(I2C_MASTER_CI2C0, CST816S_UPGRADE_ADD, &reg1, cmd, 1, 0)) { // read flag
            mdelay(2); // 4ms
            continue;
        } else {
            printf("cmd[0] = 0x%02x\n", cmd[0]);
            if (cmd[0] != 0x55) {
                msleep(2); // 4ms
                continue;
            } else {
                /* printf("%s %d \r\n", __FUNCTION__, __LINE__); */
                printf("cst816s_enter_bootmode succ!\n");
                return 0;
            }
        }
    }
    printf("%s %d retryCnt %d\r\n", __FUNCTION__, __LINE__, retryCnt);
    return -1;
}

static int cst816s_update(uint16_t startAddr, uint16_t len, uint8_t *src)
{
    uint16_t sum_len;
    uint8_t cmd[10];

    if (cst816s_enter_bootmode() == -1) {
        //printf("%s %d \r\n", __FUNCTION__, __LINE__);
        return -1;
    }
    sum_len = 0;
    do {
        if (sum_len >= len) {
            return -1;
        }

        // send address
        cmd[0] = startAddr & 0xFF;
        cmd[1] = startAddr >> 8;

        char reg0[2] = {0xA0, 0x14};
        i2c_write_regs_ex(I2C_MASTER_CI2C0, CST816S_UPGRADE_ADD, &reg0, cmd, 2, 0);
        char reg1[2] = {0xA0, 0x18};
        i2c_write_regs_ex(I2C_MASTER_CI2C0, CST816S_UPGRADE_ADD, &reg1, src, PER_LEN, 0);

        cmd[0] = 0xEE;
        char reg2[2] = {0xA0, 0x04};
        i2c_write_regs_ex(I2C_MASTER_CI2C0, CST816S_UPGRADE_ADD, &reg2, cmd, 1, 0);
        msleep(100);
        //printf("%s %d \r\n", __FUNCTION__, __LINE__);
        uint8_t retrycnt = 50;
        while (retrycnt--) {
            cmd[0] = 0;
            char reg_read[2] = {0xA0, 0x05};
            i2c_read_regs_ex(I2C_MASTER_CI2C0, CST816S_UPGRADE_ADD, &reg_read, cmd, 1, 0);
            //printf("%s %d \r\n", __FUNCTION__, __LINE__);
            if (cmd[0] == 0x55) {
                // success
                break;
            }
            msleep(10);
        }

        startAddr += PER_LEN;
        src += PER_LEN;
        sum_len += PER_LEN;
        //printf("%s %d \r\n", __FUNCTION__, __LINE__);
    } while (len);

    // exit program mode
    cmd[0] = 0x00;
    char reg3[2] = {0xA0, 0x03};
    i2c_write_regs_ex(I2C_MASTER_CI2C0, CST816S_UPGRADE_ADD, &reg3, cmd, 1, 0);

    return 0;
}

static uint32_t cst816s_read_checksum(uint16_t startAddr, uint16_t len)
{
    union {
        uint32_t sum;
        uint8_t buf[4];
    } checksum;

    char cmd[3];
    char readback[4] = {0};

    if (cst816s_enter_bootmode() == -1) {
        return -1;
    }

    cmd[0] = 0;
    char reg[2] = {0xA0, 0x03};
    if (-1 == i2c_write_regs_ex(I2C_MASTER_CI2C0, CST816S_UPGRADE_ADD, &reg, cmd, 1, 0)) {
        return -1;
    }
    msleep(500);

    checksum.sum = 0;
    char reg_read[2] = {0xA0, 0x08};
    if (-1 == i2c_read_regs_ex(I2C_MASTER_CI2C0, CST816S_UPGRADE_ADD, &reg_read, checksum.buf, 2, 0)) {
        return -1;
    }
    printf("%s checksum.sum %d \r\n", __FUNCTION__, checksum.sum);
    return checksum.sum;
}

bool cst816s_upgrade(void)
{
    uint8_t lvalue;

    if (cst816s_enter_bootmode() == 0) {
        printf("%s %d \r\n", __FUNCTION__, __LINE__);
        if (sizeof(app_bin) > 10) {
            printf("%s %d \r\n", __FUNCTION__, __LINE__);
            uint16_t startAddr = app_bin[1];
            uint16_t length = app_bin[3];
            uint16_t checksum = app_bin[5];
            startAddr <<= 8;
            startAddr |= app_bin[0];
            length <<= 8;
            length |= app_bin[2];
            checksum <<= 8;
            checksum |= app_bin[4];

            printf("checksum = 0x%x\n", checksum);
            if (cst816s_read_checksum(startAddr, length) != checksum) {
                cst816s_update(startAddr, length, app_bin + 6);
                cst816s_read_checksum(startAddr, length);
                printf("%s %d \r\n", __FUNCTION__, __LINE__);
            }
        }
        return true;
    }

    return false;
}

extern void timer1_run(u32 precesion);

void cst816s_reset()
{
    CST816S_RESET_H();
    delay_ms(10);
    CST816S_RESET_L();
    delay_ms(20);
    CST816S_RESET_H();

    delay(11000);//5ms
}

static bool touch_flag = 0;
static int touch_int_handler()
{
    touch_flag = 1;
    return 0;
}

struct point {
    int x;
    int y;
};

typedef struct {
    struct point point;
    int state;
} lv_indev_data_t;

#define LV_INDEV_STATE_PR  0
#define LV_INDEV_STATE_REL 1
#define VK_X 454
#define VK_Y 454

static int vk_state = LV_INDEV_STATE_REL;


#define Y_MIRROR  1
#define X_MIRROR  1

static u8 tp_last_staus = ELM_EVENT_TOUCH_UP;

#define abs(x)  ((x)>0?(x):-(x) )

static void tpd_down(int raw_x, int raw_y, int x, int y, int p)
{
    struct touch_event t;
    static int first_x = 0;
    static int first_y = 0;
    static u8 move_flag = 0;

#if Y_MIRROR
    x = VK_X - x;
#endif

#if X_MIRROR
    y = VK_Y - y;
#endif

    /* printf("D[%4d %4d %4d]\n", x, y, p); */
    if (tp_last_staus != ELM_EVENT_TOUCH_UP) {
        int x_move = abs(x - first_x);
        int y_move = abs(y - first_y);

        if (!move_flag && (x_move >= 50 || y_move >= 50) && (abs(x_move - y_move) >= 25)) {
            if (x_move > y_move) {
                if (x > first_x) {
                    tp_last_staus = ELM_EVENT_TOUCH_R_MOVE;
                } else {
                    tp_last_staus = ELM_EVENT_TOUCH_L_MOVE;
                }

            } else {

                if (y > first_y) {
                    tp_last_staus = ELM_EVENT_TOUCH_D_MOVE;
                } else {
                    tp_last_staus = ELM_EVENT_TOUCH_U_MOVE;
                }
            }
            move_flag = 1;
        } else {
            tp_last_staus = ELM_EVENT_TOUCH_MOVE;
            /* return; */
        }
        /* tp_last_staus = ELM_EVENT_TOUCH_HOLD; */
    } else {
        tp_last_staus = ELM_EVENT_TOUCH_DOWN;
        first_x = x;
        first_y = y;
        move_flag = 0;
    }

    t.event = tp_last_staus;
    t.x = x;
    t.y = y;
    ui_touch_msg_post(&t);
}

static void tpd_up(int raw_x, int raw_y, int x, int y, int p)
{
    struct touch_event t;
#if Y_MIRROR
    x = VK_X - x;
#endif

#if X_MIRROR
    y = VK_Y - y;
#endif

    /* printf("U[%4d %4d %4d]\n", x, y, 0); */
    tp_last_staus = ELM_EVENT_TOUCH_UP;
    t.event = tp_last_staus;
    t.x = x;
    t.y = y;
    ui_touch_msg_post(&t);


}

bool cst816s_read(lv_indev_data_t *data)
{
    unsigned irq_state;
    unsigned char buf[5];
    int update = 0;
    static int x_last, y_last;

    /* if (update) { */
    i2c_read_regs(I2C_MASTER_CI2C0, HYN_I2C_ADDR, 0x2, &buf[0], 5, 0);
    data->point.x = buf[2] | ((buf[1] & 0xF) << 8);
    data->point.y = buf[4] | ((buf[3] & 0xF) << 8);
    x_last = data->point.x;
    y_last = data->point.y;
    data->state = (buf[0] != 0) ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
    printf("finger:%d, (%d, %d), %s\n", buf[0], data->point.x, data->point.y, (data->state == LV_INDEV_STATE_PR) ? "pressed" : "released");
    if ((data->point.x == VK_X) && (data->point.y == VK_Y)) {
        vk_state = data->state;
        printf("vk %s\n", (vk_state == LV_INDEV_STATE_PR) ? "press" : "release");
    }

    if (buf[0] != 0) {
        tpd_down(0, 0, data->point.x, data->point.y, 0);
    } else {
        tpd_up(0, 0, data->point.x, data->point.y, 0);
    }

    /* } else { */
    /* data->point.x = x_last; */
    /* data->point.y = y_last; */
    /* data->state = LV_INDEV_STATE_REL; */
    /* } */

    return false; /* no more data to read */
}

static u16 regs_table[] = {
    0x01,//GestureID
    0x02,//FingerNum
    0x03,//XposH
    0x04,//XposL
    0x05,//YposH
    0x06,//YposL
    0xB0,//BPC0H
    0xB1,//BPC0L
    0xB2,//BPC1H
    0xB3,//BPC1L
    0xA7,//ChipID
    0xA8,//ProjID
    0xA9,//FwVersion
    0xEC,//MotionMask
    0xED,//IrqPluseWidth
    0xEE,//NorScanPer
    0xEF,//MotionS1Angle
    0xF0,//LpScanRaw1H
    0xF1,//LpScanRaw1L
    0xF2,//LpScanRaw2H
    0xF3,//LpScanRaw2L
    0xF4,//LpAutoWakeTime
    0xF5,//LpScanTH
    0xF6,//LpScanWin
    0xF7,//LpScanFreq
    0xF8,//LpScanIdac
    0xF9,//AutoSleepTime
    0xFA,//IrqCtl
    0xFB,//AutoReset
    0xFC,//LongPressTime
    0xFD,//IOCtl
    0xFE//DisAutoSleep
};
static void touch_event_handler(void *priv)
{
    u8 buf[1];
    int i;

    lv_indev_data_t touch_data;
    if (!touch_flag) {
        return ;
    }
    touch_flag = 0;
    /* printf("touch_event_handler\n"); */
    cst816s_read(&touch_data);

#if 0
    buf[0] = 0x3;
    i2c_write_regs(I2C_MASTER_CI2C0, HYN_I2C_ADDR, 0xf8, &buf[0], 1, 0);

    for (i = 0; i < sizeof(regs_table) / sizeof(regs_table[0]); i++) {
        i2c_read_regs(I2C_MASTER_CI2C0, HYN_I2C_ADDR, regs_table[i], &buf[0], 1, 0);
        printf("addr:0x%x , val:0x%02x\n", regs_table[i], buf[0]);
    }
#endif
}


void ui_set_touch_event(void (*touch_event)(void *priv), int interval);
extern void io_ext_interrupt_init(u8 port, trigger_mode_type trigger_mode, IO_ISR_FUNC cbfun);
void cst816s_init()
{
    u8 buf[1];
    int i;
    printf("cst816s init...\n");

    iic_init(__this->iic_hdl);

    CST816S_INT_IN();

    cst816s_reset();

    if (!cst816s_upgrade()) {
        printf("cst816s_upgrade fail!\n");
    }
    cst816s_reset();

    io_ext_interrupt_init(CST816S_INT_IO, falling_edge_trigger, touch_int_handler);
#if TCFG_UI_ENABLE
    ui_set_touch_event(touch_event_handler, 2);
#endif
}

#endif  //CST816S_COB==1

#endif

