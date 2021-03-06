#include "system/includes.h"
#include "media/includes.h"
#include "asm/iic_hw.h"
#include "asm/iic_soft.h"
#include "nfc_fm11nt081d.h"
#include "system/generic/gpio.h"

#if TCFG_FM11NT081D_DEV_ENABLE

/* #define LOG_TAG_CONST       NORM */
#define LOG_TAG             "[fm11nt08]"
#include "debug.h"


/********************************************************/
#if(TCFG_FM11NT081D_USER_IIC_TYPE)
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

struct fm11nt081d_platform_data *fm11nt081d_info;
/* extern void delay(unsigned int cnt); */
void delay_12us(u32 us)
{
    u32 mips = (clk_get("sys") / 1000000);
    /* log_info("sys:%d000000",mips); */
    while (us--) {
        for (int i = 0; i < mips; i++) {
            asm("nop");
        }
    }
}

static bool fm11nt08_read_buf(u16 reg, u8 *rbuf, u8 len)
{
    iic_start(fm11nt081d_info->hdl);
    if (0 == iic_tx_byte(fm11nt081d_info->hdl, FM11NT081D_WADDR)) {
        iic_stop(fm11nt081d_info->hdl);
        log_error("fm11nt081d read fail1!\n");
        return false;
    }
    /* os_time_dly(fm11nt081d_info->iic_delay); */
    /* delay(fm11nt081d_info->iic_delay); */
    delay_2ms(fm11nt081d_info->iic_delay);
    if (0 == iic_tx_byte(fm11nt081d_info->hdl, (u8)(reg >> 8))) {
        iic_stop(fm11nt081d_info->hdl);
        log_error("fm11nt081d read fail21!\n");
        return false;
    }
    delay_2ms(fm11nt081d_info->iic_delay);
    if (0 == iic_tx_byte(fm11nt081d_info->hdl, (u8)reg)) {
        iic_stop(fm11nt081d_info->hdl);
        log_error("fm11nt081d read fail22!\n");
        return false;
    }
    delay_2ms(fm11nt081d_info->iic_delay);
    iic_start(fm11nt081d_info->hdl);
    if (0 == iic_tx_byte(fm11nt081d_info->hdl, FM11NT081D_RADDR)) {
        iic_stop(fm11nt081d_info->hdl);
        log_error("fm11nt081d read fail3!\n");
        return false;
    }
    /* for (u8 i = 0; i < len - 1; i++) {	 */
    /* 	rbuf[i] = iic_rx_byte(fm11nt081d_info->hdl, 1); */
    /* 	delay(fm11nt081d_info->iic_delay); */
    /* } */
    /* rbuf[len - 1] = iic_rx_byte(fm11nt081d_info->hdl, 0); */
    iic_read_buf(fm11nt081d_info->hdl, rbuf, len);
    iic_stop(fm11nt081d_info->hdl);
    return true;
}

static bool fm11nt08_write_buf(u16 reg, u8 *wbuf, u8 len)
{
    u8 ret;
    iic_start(fm11nt081d_info->hdl);
    if (0 == iic_tx_byte(fm11nt081d_info->hdl, FM11NT081D_WADDR)) {
        iic_stop(fm11nt081d_info->hdl);
        log_error("fm11nt081d write fail1!\n");
        return false;
    }
    delay_2ms(fm11nt081d_info->iic_delay);
    if (0 == iic_tx_byte(fm11nt081d_info->hdl, (u8)(reg >> 8))) {
        iic_stop(fm11nt081d_info->hdl);
        log_error("fm11nt081d write fail21!\n");
        return false;
    }
    delay_2ms(fm11nt081d_info->iic_delay);
    if (0 == iic_tx_byte(fm11nt081d_info->hdl, (u8)reg)) {
        iic_stop(fm11nt081d_info->hdl);
        log_error("fm11nt081d write fail22!\n");
        return false;
    }
    delay_2ms(fm11nt081d_info->iic_delay);
    /* for (u8 i = 0; i < len - 1; i++) {	 */
    /* 	if(0==iic_tx_byte(fm11nt081d_info->hdl, wbuf[i])){ */
    /* 		iic_stop(fm11nt081d_info->hdl); */
    /* 		log_error("fm11nt081d write fail2!\n"); */
    /* 		return false; */
    /* 	} */
    /* 	delay(fm11nt081d_info->iic_delay); */
    /* } */
    /* if (0 == iic_tx_byte(fm11nt081d_info->hdl, wbuf[len-1])) {	 */
    /* 	iic_stop(fm11nt081d_info->hdl); */
    /* 	log_error("fm11nt081d write fail3!\n"); */
    /* 	return false; */
    /* } */
    ret = iic_write_buf(fm11nt081d_info->hdl, wbuf, len);
    if (ret < len) {
        log_error("fm11nt081d write fail3!\n");
        iic_stop(fm11nt081d_info->hdl);
        return false;
    }
    iic_stop(fm11nt081d_info->hdl);
    return true;
}

void fm11nt08_cs_on()
{
    gpio_write(fm11nt081d_info->csn_port_io, 0);
    delay_12us(10);//abort 100us
}
void fm11nt08_cs_off()
{
    gpio_write(fm11nt081d_info->csn_port_io, 1);
}
/*********************************************************************************************************
 * ** ????????????:	FM11NT08_WriteReg
 * ** ????????????:	???11NC08?????????
 * ** ????????????:    reg:???????????????
 * ** 				val:???????????????
 * ** ?????????:      ???
 * *********************************************************************************************************/
void fm11nt08_writereg(u8 reg, u8 val)
{
    u16 reg_temp;
    u8 val_temp = val;
    fm11nt08_cs_on();
    reg_temp = (0xff << 8) | (reg | 0xf0);
    fm11nt08_write_buf(reg_temp, &val_temp, 1);
    fm11nt08_cs_off();
}
/*********************************************************************************************************
 * ** ????????????:	FM11NT08_ReadReg
 * ** ????????????:	???????????????
 * ** ????????????:    reg:???????????????
 * ** ?????????:      val,?????????????????????
 * *********************************************************************************************************/
u8 fm11nt08_readreg(u8 reg)
{
    u16 reg_temp;
    u8 val;
    fm11nt08_cs_on();
    reg_temp = (0xff << 8) | (reg | 0xf0);
    fm11nt08_read_buf(reg_temp, &val, 1);
    fm11nt08_cs_off();
    return val;
}
/*********************************************************************************************************
 * ** ????????????:	FM11NT08_WriteFIFO
 * ** ????????????:	???FIFO
 * ** ????????????:    ilen:???????????????(<=32 fifo??????)
 * ** 					*ibuf:????????????
 * ** ?????????:      ???
 * *********************************************************************************************************/
void fm11nt08_writefifo(u8 *wbuf, u32 len)
{
    u16 reg_temp;
    fm11nt08_cs_on();
    reg_temp = 0xfff0;
    fm11nt08_write_buf(reg_temp, wbuf, len);
    fm11nt08_cs_off();
}
/*********************************************************************************************************
 * ** ????????????:	ReadFIFO
 * ** ????????????:	??????NAK???
 * ** ????????????:    *ilen:????????????????????????
 * ** ????????????:    *rbuf:???????????????
 * ** ?????????:      ?????????????????????
 * *********************************************************************************************************/
void fm11nt08_readfifo(u8 *rbuf, u32 len)
{
    u16 reg_temp;
    fm11nt08_cs_on();
    reg_temp = 0xfff0;
    fm11nt08_read_buf(reg_temp, rbuf, len);
    fm11nt08_cs_off();
}
/*********************************************************************************************************
 * ** ????????????:	FM11NT08_WriteE2
 * ** ????????????:	???E2??????
 * ** ????????????:    adr:E2??????
 * ** 				len:?????????????????????(<=16)
 * ** 				*ibuf:???????????????
 * ** ?????????:      ???
 * *********************************************************************************************************/
void fm11nt08_writeE2(u8 *addr, u8 *wbuf, u32 len)
{
    u16 reg_temp;
    fm11nt08_cs_on();
    reg_temp = ((addr[0] & 0x03) << 8) | addr[1];
    fm11nt08_write_buf(reg_temp, wbuf, len);
    fm11nt08_cs_off();
}
/*********************************************************************************************************
 * ** ????????????:	FM11NT08_ReadE2
 * ** ????????????:	?????????E2??????
 * ** ????????????:    adr:E2??????
 * ** 				len:?????????????????????
 * ** 				*rbuf:???????????????
 * ** ?????????:      ???
 * *********************************************************************************************************/
void fm11nt08_readE2(u8 *addr, u8 *rbuf, u32 len)
{
    u16 reg_temp;
    fm11nt08_cs_on();
    reg_temp = ((addr[0] & 0x03) << 8) | addr[1];
    fm11nt08_read_buf(reg_temp, rbuf, len);
    fm11nt08_cs_off();
}

unsigned char irq_rxdone = 0;
unsigned char irq_txdone = 0;
unsigned char irq_data_in = 0;

unsigned char rfLen = 0;
unsigned char rfBuf[255];
unsigned char FlagFirstFrame = OFF;
unsigned char FlagErrIrq = OFF;
unsigned char CID;
unsigned char FSDI;
unsigned char block_num;
/*********************************************
 * ** ????????????:	FM11NT08_RFDataTx
 * ** ????????????:	RF????????????
 * ** ????????????:    ilen:??????????????????
 * ** 				*ibuf:???????????????
 * ** ????????????:    ???
 * ** ?????????:      ???
 * *******************************************/
void fm11nt08_RFDataTx(u8 *ibuf, u32 ilen)
{
    u32 slen, cnt;
    u8 *sbuf;

    slen = ilen;
    sbuf = ibuf;
    cnt = 0;
    if (slen <= 32) {
        fm11nt08_writefifo(sbuf, slen);		//write fifo	??????????????????32???????????????
        slen = 0;
    } else {
        fm11nt08_writefifo(sbuf, 32);		//write fifo	????????????32????????????32??????

        slen -= 32;		//???????????????32
        sbuf += 32;		//??????????????????+32
    }

    fm11nt08_writereg(RF_TXEN, 0x55);	//???0x55?????????????????????????????????

    while (slen > 0) {
        if ((fm11nt08_readreg(FIFO_WORDCNT) & 0x3F) <= 8) {
            if (slen <= 24) {
                fm11nt08_writefifo(sbuf, slen);			//write fifo	??????24?????????fifo
                slen = 0;
            } else {
                fm11nt08_writefifo(sbuf, 24);			//write fifo	??????24?????????fifo

                slen -= 24; 	//???????????????24
                sbuf += 24; 	//??????????????????+24
            }
        }

        fm11nt08_writereg(RF_TXEN, 0x55);	//???0x55?????????????????????????????????
    }
    //while(fm11nt08_ChkIrqInfo(FIFO_IRQ_EMPTY,FIFO_IRQ)==OFF);
    //fm11nt08_readreg(MAIN_IRQ);

    //while((fm11nt08_readreg(FIFO_WORDCNT) & 0x3F )> 0);	//?????????????????????????????????
    irq_txdone = 0;
}

/*******************************************
 ** ????????????:	FM11NT08_RFDataRx
 ** ????????????:	???FIFO
 ** ????????????:    rbuf:????????????
 ** ????????????:    ???
 ** ?????????:      ?????????????????????
 ******************************************/
unsigned int fm11nt08_RFDataRx(unsigned char *rbuf)
{
    u8 rlen, irq_main;
    rlen = 0;
    bit_clr_ie(IRQ_PORT_IDX);
    while (1) {
        //		Delay_100us(10);
        rlen = fm11nt08_readreg(FIFO_WORDCNT) & 0x3F;
        if (rlen > 8) {
            fm11nt08_readfifo(rfBuf + rfLen, rlen);
            rfLen += rlen;
        }
        irq_main = fm11nt08_readreg(MAIN_IRQ);
        if (irq_main & MAIN_IRQ_RX_DONE) {
            break;
        }
        if (irq_main & MAIN_IRQ_AUX) {
            FlagErrIrq = ON;
            fm11nt08_readreg(AUX_IRQ);
        }
    }
    //	while(irq_rxdone == 0);
    irq_rxdone = 0;
    //	rlen=rfLen;
    //while((irq_rxdone == 0)&&(FlagErrIrq == OFF));
    //	irq_rxdone = 0;
    /*
    	if(FlagErrIrq == ON)
    	{
    	FlagErrIrq = OFF;
    	return 0;
    	}
    	*/
    rlen = fm11nt08_readreg(FIFO_WORDCNT) & 0x3F;	//????????????????????????fifo???????????????
    fm11nt08_readfifo(rfBuf + rfLen, rlen);		//??????????????????
    rfLen += rlen;
    rlen = rfLen;

    if (rlen <= 2)	{
        return 0;
    }
    rlen -= 2;	//2??????crc??????
    bit_set_ie(IRQ_PORT_IDX);
    return rlen;
}

void fm11nt08_Clearfifo(void)
{
    fm11nt08_writereg(FIFO_FLUSH, 0xFF);		//???fifo?????????
}


/*********************************************************************************************************
 * ** ????????????:	FM11NT08_GetStatus
 * ** ????????????:	???FM11NT08 status
 * ** ????????????:    ???
 * ** ????????????:    ???
 * ** ?????????:      sta ?????????
 * *********************************************************************************************************/
unsigned char fm11nt08_GetStatus(void)
{
    unsigned char sta;
    sta = fm11nt08_readreg(RF_STATUS);
    sta = (sta & 0xE0) >> 5;		//???3bit
    return sta;
}
/*********************************************************************************************************
 * ** ????????????:	FM11NT08_ChkIrqInfo
 * ** ????????????:	???FM11NT08 ????????????
 * ** ????????????:    irq:????????????
 * **					reg:mian/fifo/aux
 * ** ????????????:    ???
 * ** ?????????:      sta:ON/OFF
 * *********************************************************************************************************/
unsigned char fm11nt08_ChkIrqInfo(u8 irq, u8 reg)
{
    unsigned char ret;
    ret = fm11nt08_readreg(reg);
    if (ret & irq) {
        return ON;		//???????????????ON
    } else {
        return OFF;
    }
}

/*********************************************************************************************************
 * ** ????????????:	FM11NT08_FIFO_func
 * ** ????????????:	??????FIFO???????????????
 * ** ????????????:    ibuf
 * ** ????????????:    ???
 * ** ?????????:      ????????????
 * *********************************************************************************************************/
unsigned char fm11nt08_fifo_func(unsigned char ibuf)
{
    if (ibuf == 0xE0) {		//Rats??????
        return FUNC_RATS;
    }
    if ((ibuf & 0xF0) == 0xD0) {	//pps??????
        return  FUNC_PPSS;
    }
    if ((ibuf & 0xE2) == 0x02) {	//I_BLOCK??????
        return  FUNC_I_BLOCK;
    }
    if ((ibuf & 0xE6) == 0xA2) {	//R_BLOCK??????
        return  FUNC_R_BLOCK;
    }
    if ((ibuf & 0xC7) == 0xC2) {	//S_BLOCK??????
        return  FUNC_S_BLOCK;
    }
    if (ibuf == 0x80) {		//??????0x80???????????????,PDTP??????
        return  FUNC_PDTP;
    }
    return 0xFF;
}

void fm11nt08_SetRatsCfg(unsigned char rats)
{
    u8 temp;
    CID = rats & 0x0F;
    temp = (rats >> 4) & 0x0F;
    if (temp < 5) {
        FSDI = 8 * (temp + 2);
    } else if ((temp >= 5) && (temp <= 7)) {
        FSDI = 32 * (temp - 3);
    } else {
        FSDI = 255;
    }

    FSDI -= 2;	//??????2??????EDC
    block_num = 0x01;	//?????????1??????????????????????????????????????????
}
void port_irq_fm11nt08(void)//FM175XX IRQ??????
{
    u8 irq_main;
    u8 irq_fifo;
    u8 rx_len = 0;
    log_info("nfc irq");
    irq_main = fm11nt08_readreg(MAIN_IRQ);

    if (irq_main & MAIN_IRQ_RF_PWON) {
        /* log_info("port  irq rf_pwon"); */
        fm11nt08_Clearfifo();
        return;
    }

    if (irq_main & MAIN_IRQ_RX_START) {
        rfLen = 0;
        irq_rxdone = 0;
        irq_data_in = 1;
        /* log_info("port  irq rx_start"); */
        return;
    }
    if (irq_main & MAIN_IRQ_TX_DONE) {
        irq_txdone = 1;
        /* log_info("port  irq tx_done"); */
        return;
    }
    /* if(reg_data & MAIN_IRQ_ACTIVE)FlagFirstFrame = ON; */
}
/*********************************************************************************************************
 ** ????????????:	FM11NT08_Init
 ** ????????????:	Fm11NT08???spi??????????????????
 ** ????????????:    ???
 ** ????????????:    ???
 ** ?????????:      ???
 *********************************************************************************************************/
bool fm11nt08_init(void *priv)
{
    if (priv == NULL) {
        log_info("fm11nt081d init fail(no priv)\n");
        return false;
    }
    fm11nt081d_info = (struct fm11nt081d_platform_data *)priv;

    gpio_write(fm11nt081d_info->csn_port_io, 1);
    gpio_set_direction(fm11nt081d_info->csn_port_io, 0);
    gpio_set_die(fm11nt081d_info->csn_port_io, 1);
    port_wkup_interrupt_init(fm11nt081d_info->fd_port_io, 1, port_irq_fm11nt08);//????????????
    u8 reg_data;
    /* delay_2ms(100);//100ms */

    fm11nt08_writereg(MAIN_IRQ_MASK, 0x54);	//MAIN????????????;??????????????????
    reg_data = fm11nt08_readreg(MAIN_IRQ_MASK);
    log_info("-> MAIN_IRQ_MASK = %x", reg_data);

    fm11nt08_writereg(FIFO_IRQ_MASK, 0x07);	//FIFO????????????
    reg_data = fm11nt08_readreg(FIFO_IRQ_MASK);
    log_info("-> FIFO_IRQ_MASK = %x", reg_data);

    fm11nt08_writereg(NFC_CFG, 0x03);	//???NFC?????????(????????????-4)
    reg_data = fm11nt08_readreg(NFC_CFG);
    log_info("-> NFC_CFG = %x", reg_data);

    fm11nt08_Clearfifo();
    log_info("clear fm fifo");
    //	RFpdtpReady = OFF;
    //	RFapduReady = OFF;
    //	RFdataLen = 0;
    //	CTdataLen = 0;
    //	CTdataIndex = 0;
    //
    //	memset(RFdataBuf,0,BUF_LENTH);
    //	memset(CTdataBuf,0,BUF_LENTH);
    return true;
}





/********************************test*********************************/
#if 1
//iic??????:???????????????standard-mode(100kbps)???fast-mode(400kbps)???fast-mode plus(1mbps)
struct fm11nt081d_platform_data fm11nt081d_test_data = {
    .comms = 0,  //0:IIC,  1:SPI
    .hdl = 0,    //iic_hdl  or spi_hdl
    .csn_port_io = IO_PORTC_02,
    .fd_port_io = IO_PORTC_03,
    .iic_delay = 0 //????????????????????????iic????????????????????????2Byte?????????????????????
};
void fm11nt081d_test_main()
{
    u8 reg_data;
    log_info("----------FM11NT081D test---------\n");
#if(TCFG_FM11NT081D_USER_IIC_TYPE)
    log_info("-------hw_iic-------\n");
#else
    log_info("-------soft_iic-------\n");
#endif
    iic_init(fm11nt081d_test_data.hdl);
    fm11nt08_init(&fm11nt081d_test_data);
    u32 eeprom_addr = 0;
    u8 buff[16] = {0x05, 0x72, 0x01, 0x57, 0xf7, 0x60, 0x02, 0x00, 0x00, 0x00, 0xa2, 0xb2, 0x00, 0x00, 0x00, 0x00};
    u8 adr[2] = {0x03, 0x10};
    log_info("write fm ee");
    wdt_clear();
    for (u8 i = 0; i < 4; i++) {
        fm11nt08_writeE2(adr, buff + i * 4, 4);
        log_info("write fm ee:%d", i);
        delay_2ms(3);//110ms
        adr[1] += 4;
    }
    delay_2ms(110);//110ms
    wdt_clear();
    for (u16 i = 0; i < 0x7a; i++) {
        eeprom_addr = i * 8;
        adr[1] = eeprom_addr & 0xFF;	//LSB
        adr[0] = (eeprom_addr & 0xFF00) >> 8; //MSB
        fm11nt08_readE2(adr, buff, 8);
        log_info("read eeprom:%d", i * 2);
        log_info_hexdump(buff, 8);
        delay_2ms(2);//110ms
    }
    while (1) {
        if (irq_data_in) {
            //		   reg_data = fm11nt08_readreg(MAIN_IRQ);
            //			Uart_Send_Msg("-> MAIN_IRQ = ");
            //			Uart_Send_Hex(&reg_data,1);
            //			Uart_Send_Msg("\r\n");

            log_info("-> irq_data_in ");
            rfLen = fm11nt08_RFDataRx(rfBuf);		//??????rf??????(??????)
            if (rfLen > 0) {
                if (FlagFirstFrame == ON) {
                    fm11nt08_SetRatsCfg(fm11nt08_readreg(RF_RATS));	//???????????????rats??????
                    FlagFirstFrame = OFF;
                }

                reg_data = fm11nt08_readreg(RF_BAUD);
                log_info("-> RF_BAUD = %x", reg_data);
                log_info("-> FM11NT08_RFDataRx = ");
                log_info_hexdump(rfBuf, rfLen);

                fm11nt08_RFDataTx(rfBuf, rfLen);
            }
            irq_data_in = 0;
        }
        delay_2ms(2);//110ms
        wdt_clear();
    }

    iic_uninit(fm11nt081d_test_data.hdl);
}


#endif
#endif


