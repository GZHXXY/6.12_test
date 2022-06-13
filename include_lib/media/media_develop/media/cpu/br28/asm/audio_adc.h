#ifndef AUDIO_ADC_H
#define AUDIO_ADC_H

#include "generic/typedef.h"
#include "generic/list.h"
#include "generic/atomic.h"

/*无电容电路*/
#define SUPPORT_MIC_CAPLESS     1

#define LADC_STATE_INIT			1
#define LADC_STATE_OPEN      	2
#define LADC_STATE_START     	3
#define LADC_STATE_STOP      	4

#define FPGA_BOARD          	0

#define LADC_MIC                0
#define LADC_LINEIN0            1
#define LADC_LINEIN1            2
#define LADC_LINEIN             3

/* 通道选择 */
#define AUDIO_ADC_MIC_0                     BIT(0)  // PA1
#define AUDIO_ADC_MIC_1                     BIT(1)  // PA3
#define AUDIO_ADC_MIC_2                     BIT(2)  // PG6
#define AUDIO_ADC_MIC_3                     BIT(3)  // PG8
#define AUDIO_ADC_LINE0                     BIT(0)  // PA1
#define AUDIO_ADC_LINE1                     BIT(1)  // PA3
#define AUDIO_ADC_LINE2                     BIT(2)  // PG6
#define AUDIO_ADC_LINE3                     BIT(3)  // PG8

/*mic_mode 工作模式定义*/
#define AUDIO_MIC_CAP_MODE          0   //单端隔直电容模式
#define AUDIO_MIC_CAP_DIFF_MODE     1   //差分隔直电容模式
#define AUDIO_MIC_CAPLESS_MODE      2   //单端省电容模式

/* 通道选择 */
#define AUDIO_ADC_MIC_L		    BIT(0)
#define AUDIO_ADC_MIC_R		    BIT(1)
#define AUDIO_ADC_LINE0_L		BIT(2)
#define AUDIO_ADC_LINE0_R		BIT(3)
#define AUDIO_ADC_LINE1_L		BIT(4)
#define AUDIO_ADC_LINE1_R		BIT(5)
#define AUDIO_ADC_MIC_CH		BIT(0)

#define AUDIO_ADC_LINE0_LR		(AUDIO_ADC_LINE0_L | AUDIO_ADC_LINE0_R)
#define AUDIO_ADC_LINE1_LR		(AUDIO_ADC_LINE1_L | AUDIO_ADC_LINE1_R)

#define LADC_CH_MIC_L		    BIT(0)
#define LADC_CH_MIC_R		    BIT(1)
#define LADC_CH_LINE0_L			BIT(2)
#define LADC_CH_LINE0_R			BIT(3)
#define LADC_CH_LINE1_L			BIT(4)
#define LADC_CH_LINE1_R			BIT(5)
#define PLNK_MIC				BIT(6)

#define LADC_MIC_MASK			(BIT(0) | BIT(1))
#define LADC_LINE0_MASK			(BIT(2) | BIT(3))
#define LADC_LINE1_MASK			(BIT(4) | BIT(5))

struct ladc_port {
    u8 channel;
};

struct adc_platform_data {
    u8 mic_channel;
    u8 ladc_num;
    /*
     *MIC0内部上拉电阻档位
     *21:1.18K	20:1.42K 	19:1.55K 	18:1.99K 	17:2.2K 	16:2.4K 	15:2.6K		14:2.91K	13:3.05K 	12:3.5K 	11:3.73K
     *10:3.91K  9:4.41K 	8:5.0K  	7:5.6K		6:6K		5:6.5K		4:7K		3:7.6K		2:8.0K		1:8.5K
     */
    u8 mic_bias_res;
    u8 mic1_bias_res;
    u8 mic2_bias_res;
    u8 mic3_bias_res;

    u32 mic_capless : 1;  		//MIC0免电容方案
    u32 mic_diff : 1;  			//MIC0差分模式方案
    u32 mic_ldo_isel: 2; 		//MIC0通道电流档位选择
    u32 mic_ldo_vsel : 3;		//0:1.3v 1:1.4v 2:1.5v 3:2.0v 4:2.2v 5:2.4v 6:2.6v 7:2.8v
    u32 mic_bias_keep : 1;		//保持内部MIC0偏置输出
    u32 mic_in_sel : 1;			//MICIN选择[0:MIC0 1:MICEXT(ldo5v)]
    u32 mic1_capless : 1;		//MIC1免电容方案
    u32 mic1_diff : 1;			//MIC1差分模式方案
    u32 mic1_ldo_isel: 2; 		//MIC0通道电流档位选择
    u32 mic1_ldo_vsel : 3;		//MIC1_LDO 00:2.3v 01:2.5v 10:2.7v 11:3.0v
    u32 mic1_bias_keep : 1;		//保持内部MIC1偏置输出
    u32 mic1_in_sel : 1;		//MICIN1选择[0:MIC1 1:MICEXT(ldo5v)]
    u32 mic1_ldo_state : 1;		//当前micldo是否打开
    u32 reserved : 11;
    u8 mic_ldo_pwr;             //MIC LDO 输出到 PA0
    u8 mic_ldo_state;            //当前micldo状态
    u32 mic_mode : 3;            //MIC0工作模式
    u32 mic1_mode : 3;           //MIC1工作模式
    u32 mic2_mode : 3;           //MIC2工作模式
    u32 mic3_mode : 3;           //MIC3工作模式
    // ADC mic 通道选择 0: amic0   1: amic1    2: amic2   3: amic3    4: dmic0    5: dmic1
    //u32 adc0_sel  : 3;             // ADC0 mic 通道选择
    //u32 adc1_sel  : 3;             // ADC1 mic 通道选择
    //u32 adc2_sel  : 3;             // ADC2 mic 通道选择
    //u32 adc3_sel  : 3;             // ADC3 mic 通道选择
    u32 mic_bias_inside : 1;     //MIC0电容隔直模式使用内部mic偏置  PA2
    u32 mic1_bias_inside : 1;    //MIC1电容隔直模式使用内部mic偏置  PA4
    u32 mic2_bias_inside : 1;    //MIC2电容隔直模式使用内部mic偏置  PG5
    u32 mic3_bias_inside : 1;    //MIC3电容隔直模式使用内部mic偏置  PG7

    const struct ladc_port *ladc;
};

struct capless_low_pass {
    u16 bud; //快调边界
    u16 count;
    u16 pass_num;
    u16 tbidx;
    u32 bud_factor;
};

struct audio_adc_output_hdl {
    struct list_head entry;
    void *priv;
    void (*handler)(void *, s16 *, int);
};

struct audio_adc_hdl {
    u8 state;
    u8 channel;
    u8 channel_num;
    struct list_head head;
    const struct adc_platform_data *pd;
    //atomic_t ref;
#if SUPPORT_MIC_CAPLESS
    struct capless_low_pass lp0;
    struct capless_low_pass lp1;
    struct capless_low_pass *lp;
#endif/*SUPPORT_MIC_CAPLESS*/
};

struct adc_mic_ch {
    struct audio_adc_hdl *adc;
    u8 gain;
    u8 gain1;
    u8 gain2;
    u8 gain3;
    u8 buf_num;
    u16 buf_size;
    s16 *bufs;
    u16 sample_rate;
    void (*handler)(struct adc_mic_ch *, s16 *, u16);
};

struct audio_adc_ch {
    u8 gain;
    u8 buf_num;
    u8 ch;
    u16 buf_size;
    u16 sample_rate;
    s16 *bufs;
    struct audio_adc_hdl *hdl;
    void (*handler)(struct audio_adc_ch *, s16 *, u16);
};


/*
*********************************************************************
*                  Audio ADC Initialize
* Description: 初始化Audio_ADC模块的相关数据结构
* Arguments  : adc	ADC模块操作句柄
*			   pd	ADC模块硬件相关配置参数
* Note(s)    : None.
*********************************************************************
*/
void audio_adc_init(struct audio_adc_hdl *, const struct adc_platform_data *);

/*
*********************************************************************
*                  Audio ADC Output Callback
* Description: 注册adc采样输出回调函数
* Arguments  : adc		adc模块操作句柄
*			   output  	采样输出回调
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_adc_add_output_handler(struct audio_adc_hdl *, struct audio_adc_output_hdl *);

/*
*********************************************************************
*                  Audio ADC Output Callback
* Description: 删除adc采样输出回调函数
* Arguments  : adc		adc模块操作句柄
*			   output  	采样输出回调
* Return	 : None.
* Note(s)    : 采样通道关闭的时候，对应的回调也要同步删除，防止内存释
*              放出现非法访问情况
*********************************************************************
*/
void audio_adc_del_output_handler(struct audio_adc_hdl *, struct audio_adc_output_hdl *);

/*
*********************************************************************
*                  Audio ADC IRQ Handler
* Description: Audio ADC中断回调函数
* Arguments  : adc  adc模块操作句柄
* Return	 : None.
* Note(s)    : 仅供Audio_ADC中断使用
*********************************************************************
*/
void audio_adc_irq_handler(struct audio_adc_hdl *adc);

/*
*********************************************************************
*                  Audio ADC Mic Open
* Description: 打开mic采样通道
* Arguments  : mic	mic操作句柄
*			   ch	mic通道索引
*			   adc  adc模块操作句柄
* Return	 : 0 成功	其他 失败
* Note(s)    : None.
*********************************************************************
*/
int audio_adc_mic_open(struct adc_mic_ch *mic, int ch, struct audio_adc_hdl *adc);
int audio_adc_mic1_open(struct adc_mic_ch *mic, int ch, struct audio_adc_hdl *adc);
int audio_adc_mic2_open(struct adc_mic_ch *mic, int ch, struct audio_adc_hdl *adc);
int audio_adc_mic3_open(struct adc_mic_ch *mic, int ch, struct audio_adc_hdl *adc);

/*
*********************************************************************
*                  Audio ADC Mic Sample Rate
* Description: 设置mic采样率
* Arguments  : mic			mic操作句柄
*			   sample_rate	采样率
* Return	 : 0 成功	其他 失败
* Note(s)    : None.
*********************************************************************
*/
int audio_adc_mic_set_sample_rate(struct adc_mic_ch *mic, int sample_rate);

/*
*********************************************************************
*                  Audio ADC Mic Gain
* Description: 设置mic增益
* Arguments  : mic	mic操作句柄
*			   gain	mic增益
* Return	 : 0 成功	其他 失败
* Note(s)    : MIC增益范围：0(-8dB)~19(30dB),step:2dB,level(4)=0dB
*********************************************************************
*/
int audio_adc_mic_set_gain(struct adc_mic_ch *mic, int gain);
int audio_adc_mic1_set_gain(struct adc_mic_ch *mic, int gain);
int audio_adc_mic2_set_gain(struct adc_mic_ch *mic, int gain);
int audio_adc_mic3_set_gain(struct adc_mic_ch *mic, int gain);

void audio_adc_mic_0dB_en(bool en);
void audio_adc_mic1_0dB_en(bool en);
void audio_adc_mic2_0dB_en(bool en);
void audio_adc_mic3_0dB_en(bool en);

/*
*********************************************************************
*                  Audio ADC Mic Buffer
* Description: 设置采样buf和采样长度
* Arguments  : mic		mic操作句柄
*			   bufs		采样buf地址
*			   buf_size	采样buf长度，即一次采样中断数据长度
*			   buf_num 	采样buf的数量
* Return	 : 0 成功	其他 失败
* Note(s)    : (1)需要的总buf大小 = buf_size * ch_num * buf_num
* 		       (2)buf_num = 2表示，第一次数据放在buf0，第二次数据放在
*			   buf1,第三次数据放在buf0，依此类推。如果buf_num = 0则表
*              示，每次数据都是放在buf0
*********************************************************************
*/
int audio_adc_mic_set_buffs(struct adc_mic_ch *mic, s16 *bufs, u16 buf_size, u8 buf_num);

/*
*********************************************************************
*                  Audio ADC Mic Start
* Description: 启动audio_adc采样
* Arguments  : mic	mic操作句柄
* Return	 : 0 成功	其他 失败
* Note(s)    : None.
*********************************************************************
*/
int audio_adc_mic_start(struct adc_mic_ch *mic);

/*
*********************************************************************
*                  Audio ADC Mic Close
* Description: 关闭mic采样
* Arguments  : mic	mic操作句柄
* Return	 : 0 成功	其他 失败
* Note(s)    : None.
*********************************************************************
*/
int audio_adc_mic_close(struct adc_mic_ch *mic);
int audio_adc_mic1_close(struct adc_mic_ch *mic);
int audio_adc_mic2_close(struct adc_mic_ch *mic);
int audio_adc_mic3_close(struct adc_mic_ch *mic);

/*
*********************************************************************
*                  Audio ADC Mic Pre_Gain
* Description: 设置mic第一级/前级增益
* Arguments  : en 前级增益使能(0:6dB 1:0dB)
* Return	 : None.
* Note(s)    : 前级增益只有0dB和6dB两个档位，使能即为0dB，否则为6dB
*********************************************************************
*/
void audio_mic_0dB_en(bool en);
void audio_mic1_0dB_en(bool en);
void audio_adc_mic2_0dB_en(bool en);
void audio_adc_mic3_0dB_en(bool en);

int audio_mic_ldo_en(u8 en, struct adc_platform_data *pd);
/*
*********************************************************************
*                  AUDIO MIC_LDO Control
* Description: mic电源mic_ldo控制接口
* Arguments  : index    ldo索引(MIC_LDO/MIC_LDO_BIAS0/MIC_LDO_BIAS1)
*              en       使能控制
*              pd       audio_adc模块配置
* Return     : 0 成功 其他 失败
* Note(s)    : (1)MIC_LDO输出不经过上拉电阻分压
*                 MIC_LDO_BIAS输出经过上拉电阻分压
*              (2)打开一个mic_ldo示例：
*               audio_mic_ldo_en(MIC_LDO,1,&adc_data);
*              (2)打开多个mic_ldo示例：
*               audio_mic_ldo_en(MIC_LDO | MIC_LDO_BIAS,1,&adc_data);
*********************************************************************
*/
/*MIC LDO index输出定义*/
#define MIC_LDO                 BIT(0)  //PA0输出原始MIC_LDO
#define MIC_LDO_BIAS0           BIT(1)  //PA2输出经过内部上拉电阻分压的偏置
#define MIC_LDO_BIAS1           BIT(2)  //PA4输出经过内部上拉电阻分压的偏置
#define MIC_LDO_BIAS2           BIT(3)  //PG7输出经过内部上拉电阻分压的偏置
#define MIC_LDO_BIAS3           BIT(4)  //PG5输出经过内部上拉电阻分压的偏置
int audio_adc_mic_ldo_en(u8 index, u8 en, struct adc_platform_data *pd);

/*
*********************************************************************
*                  Audio MIC Mute
* Description: mic静音使能控制
* Arguments  : mute 静音使能
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_set_mic_mute(bool mute);
void audio_set_mic1_mute(bool mute);


int audio_adc_linein_open(struct audio_adc_ch *adc, int ch, struct audio_adc_hdl *hdl);
int audio_adc_linein_set_sample_rate(struct audio_adc_ch *ch, int sample_rate);
int audio_adc_linein_set_gain(struct audio_adc_ch *ch, int gain);
int audio_adc_set_buffs(struct audio_adc_ch *ch, s16 *bufs, u16 buf_size, u8 buf_num);
int audio_adc_linein_start(struct audio_adc_ch *ch);
int audio_adc_linein_close(struct audio_adc_ch *ch);

int audio_adc_start(struct audio_adc_ch *linein_ch, struct adc_mic_ch *mic_ch);
int audio_adc_close(struct audio_adc_ch *linein_ch, struct adc_mic_ch *mic_ch);

#endif/*AUDIO_ADC_H*/
