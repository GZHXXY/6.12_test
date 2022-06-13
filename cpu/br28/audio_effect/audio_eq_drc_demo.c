
#include "system/includes.h"
#include "media/includes.h"
#include "app_config.h"
#include "clock_cfg.h"
#include "application/audio_eq_drc_apply.h"
#include "audio_eq_drc_demo.h"


/*----------------------------------------------------------------------------*/
/**@brief    数据流方式eq drc 打开 demo
   @param    sample_rate:采样率
   @param    ch_num:通道个数
   @param    drc_en:是否使用drc(限幅器)1:是  0:否
   @parm     mode:播歌song_eq_mode,播歌RL_RR通道用rl_eq_mode,
   @return   句柄
   @note     支持在线调试，支持使用eq_cfg_hw.bin效果文件的
*/
/*----------------------------------------------------------------------------*/
void *stream_eq_drc_open_demo(u32 sample_rate, u8 ch_num, u8 drc_en, u8 mode)
{
#if TCFG_EQ_ENABLE
    struct audio_eq_drc *eq_drc = NULL;
    struct audio_eq_drc_parm effect_parm = {0};

#if TCFG_DRC_ENABLE
    if (drc_en) {
        effect_parm.drc_en = 1;                  //drc使能
        effect_parm.drc_cb = drc_get_filter_info;//drc默认系数回调
    }
#endif//TCFG_DRC_ENABLE

    effect_parm.eq_en = 1;                 //eq使能
    effect_parm.eq_cb = eq_get_filter_info;//eq默认系数回调
    if (effect_parm.eq_en) {
        effect_parm.async_en = 1;      //异步eq
        effect_parm.online_en = 1;     //支持在线调试
        effect_parm.mode_en = 1;       //没效果eq_cfg_hw.bin时，支持使用默认系数表,宏TCFG_USE_EQ_FILE配0
    }
    if (effect_parm.drc_en && effect_parm.eq_en) {
        effect_parm.out_32bit = 1;     //使能32biteq输出，32bitdrc处理
    }
    effect_parm.eq_name = mode;        //非通话相关eq使用song_eq_mode标识
    effect_parm.ch_num = ch_num;       //通道数1或者2
    effect_parm.sr = sample_rate;      //输入音频采样率

#if defined(TCFG_EQ_DIVIDE_ENABLE) && TCFG_EQ_DIVIDE_ENABLE
    effect_parm.divide_en = 1;         //drc的左右声道是否支持独立效果，通常配0，使用统一效果
#endif//TCFG_EQ_DIVIDE_ENABLE

    eq_drc = audio_eq_drc_open(&effect_parm);//打开eq drc节点

    clock_add(EQ_CLK);
    if (effect_parm.drc_en) {
        clock_add(EQ_DRC_CLK);
    }
    return eq_drc;
#endif//TCFG_EQ_ENABLE

    return NULL;
}

/*----------------------------------------------------------------------------*/
/**@brief    数据流方式eq drc 关闭 demo
   @param    句柄(eq_drc_open_demo的返回值)
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void stream_eq_drc_close_demo(struct audio_eq_drc *eq_drc)
{
#if TCFG_EQ_ENABLE
    if (eq_drc) {
        audio_eq_drc_close(eq_drc);
        eq_drc = NULL;
        clock_remove(EQ_CLK);
#if TCFG_DRC_ENABLE
        clock_remove(EQ_DRC_CLK);
#endif//TCFG_DRC_ENABLE

    }
#endif//TCFG_EQ_ENABLE

}

#if 0
struct audio_eq_drc *eq_drc_demo;
//eq drc接入数据流处理
void  access_data_stream_demo()
{
    u32 sample_rate = 44100;
    u8 ch_num = 2;
    u8 drc_en = 1;
    eq_drc_demo = eq_drc_open_demo(sample_rate, ch_num, drc_en, song_eq_mode);//播歌eqdrc

    u8 entry_cnt = 0;
    entries[entry_cnt++] = ...;
    entries[entry_cnt++] = &eq_drc_demo->entry;;
    entries[entry_cnt++] = ...;
    ...;
}

//解码结束，关闭eq drc
void  dec_over_demo()
{
    if (eq_drc_demo) {
        eq_drc_close_demo(eq_drc_demo);
        eq_drc_demo = NULL;
    }
}

//宏TCFG_USE_EQ_FILE配0
//系数切换
void eq_sw_demo()
{
    eq_mode_sw();//7种默认系数切换
}

//获取当前eq系数表类型
void eq_mode_get_demo()
{
    u8 mode ;
    mode = eq_mode_get_cur();
}
//宏TCFG_USE_EQ_FILE配0
//自定义系数表动态更新
//本demo 示意更新中心截止频率，增益，总增益，如需设置更多参数，请查看eq_config.h头文件的demo
void eq_update_demo()
{
    eq_mode_set_custom_info(0, 200, 2);//第0段,200Hz中心截止频率，2db
    eq_mode_set_custom_info(5, 2000, 2);//第5段,2000Hz中心截止频率，2db

    set_global_gain(get_eq_cfg_hdl(), song_eq_mode, -2);//设置普通音乐eq 总增益 -2db（可避免最大增益大于0db，导致失真）

    eq_mode_set(EQ_MODE_CUSTOM);//设置系数、总增益更新
}

#endif

//非数据流方式eq,drc处理
#if  1
static void eq_32bit_out(struct dec_eq_drc *eff)
{
    int wlen = 0;
    if (eff->priv && eff->out_cb) {
        wlen = eff->out_cb(eff->priv, &eff->eq_out_buf[eff->eq_out_points], (eff->eq_out_total - eff->eq_out_points) * 2);
    }
    eff->eq_out_points += wlen / 2;
}

static int eq_output(void *priv, void *buf, u32 len)
{
    int wlen = 0;
    int rlen = len;
    s16 *data = (s16 *)buf;
    struct dec_eq_drc *eff = priv;
    if (!eff->async) {
        return rlen;
    }

    if (eff->drc && eff->async) {
        if (eff->eq_out_buf && (eff->eq_out_points < eff->eq_out_total)) {
            eq_32bit_out(eff);
            if (eff->eq_out_points < eff->eq_out_total) {
                return 0;
            }
        }

        audio_drc_run(eff->drc, data, len);

        if ((!eff->eq_out_buf) || (eff->eq_out_buf_len < len / 2)) {
            if (eff->eq_out_buf) {
                free(eff->eq_out_buf);
            }
            eff->eq_out_buf_len = len / 2;
            eff->eq_out_buf = malloc(eff->eq_out_buf_len);
            ASSERT(eff->eq_out_buf);
        }
        user_sat16((s32 *)data, (s16 *)eff->eq_out_buf, len / 4);
        eff->eq_out_points = 0;
        eff->eq_out_total = len / 4;

        eq_32bit_out(eff);
        return len;
    }

    int out_len = 0;
    if (eff->priv && eff->out_cb) {
        out_len = eff->out_cb(eff->priv, data, len);
    }
    return out_len;
}

/*----------------------------------------------------------------------------*/
/**@brief    非数据流方式eq drc 打开
   @param    priv:输出回调私有指针
   @param    eq_output_cb:输出回调（eq使用异步时，数据从该回调函数输出）
   @param    sample_rate:采样率
   @param    channel:输入数据通道数
   @param    async:eq是否采样异步方式处理 1：是， 0：否
   @param    drc_en:是否使能drc 1:是， 0：否
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void *dec_eq_drc_open_demo(void *priv, int (*eq_output_cb)(void *, void *, int), u32 sample_rate, u8 channel, u8 async, u8 drc_en)
{
#if TCFG_EQ_ENABLE
    struct dec_eq_drc *eff = zalloc(sizeof(struct dec_eq_drc));
    struct audio_eq_param eq_param = {0};
    eff->priv = priv;
    eff->out_cb = eq_output_cb;

    eq_param.channels = channel;
    eq_param.online_en = 1;
    eq_param.mode_en = 1;
    eq_param.remain_en = 1;
    eq_param.no_wait = async;
    if (drc_en) {
        eq_param.out_32bit = 1;
    }
    eq_param.max_nsection = EQ_SECTION_MAX;
    eq_param.cb = eq_get_filter_info;//系数回调，用户可自定义该回调
    eq_param.eq_name = song_eq_mode;
    eq_param.sr = sample_rate;
    eq_param.priv = eff;
    eq_param.output = eq_output;
    eff->eq = audio_dec_eq_open(&eq_param);//eq打开

    eff->async = async;
#if TCFG_DRC_ENABLE
    if (drc_en) {
        struct audio_drc_param drc_param = {0};
        drc_param.sr = sample_rate;
        drc_param.channels = channel;
        drc_param.online_en = 1;
        drc_param.remain_en = 1;
        drc_param.out_32bit = 1;
        drc_param.cb = drc_get_filter_info;
        drc_param.drc_name = song_eq_mode;
        eff->drc = audio_dec_drc_open(&drc_param);//drc打开
    }
#endif//TCFG_DRC_ENABLE

    return eff;
#else
    return NULL;
#endif//TCFG_EQ_ENABLE

}

/*----------------------------------------------------------------------------*/
/**@brief    非数据流方式eq drc 关闭
   @param    eff句柄(dec_eq_drc_open_demo的返回值)
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void dec_eq_drc_close_demo(void *eff)
{
#if TCFG_EQ_ENABLE
    struct dec_eq_drc *eff_hdl = (struct dec_eq_drc *)eff;
    if (!eff_hdl) {
        return;
    }

    if (eff_hdl->eq) {
        audio_dec_eq_close(eff_hdl->eq);//eq关闭
        eff_hdl->eq = NULL;
    }

    if (eff_hdl->drc) {
        audio_dec_drc_close(eff_hdl->drc);//drc关闭
        eff_hdl->drc = NULL;
    }
    if (eff_hdl->eq_out_buf) {
        free(eff_hdl->eq_out_buf);
        eff_hdl->eq_out_buf = NULL;
    }

    free(eff_hdl);
#endif//TCFG_EQ_ENABLE

}

/*----------------------------------------------------------------------------*/
/**@brief    非数据流方式eq drc 处理
   @param    eff句柄(dec_eq_drc_open_demo的返回值)
   @param    data:输入数据地址
   @param    len:输入数据字节长度
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
int dec_eq_drc_run_demo(void *eff, void *data, u32 len)
{
#if TCFG_EQ_ENABLE
    struct dec_eq_drc *eff_hdl = (struct dec_eq_drc *)eff;
    if (!eff_hdl) {
        return 0;
    }

#if TCFG_DRC_ENABLE
    if (eff_hdl->drc && !eff_hdl->async) {//同步32bit eq drc 处理
        if ((!eff_hdl->eq_out_buf) || (eff_hdl->eq_out_buf_len < len * 2)) {
            if (eff_hdl->eq_out_buf) {
                free(eff_hdl->eq_out_buf);
            }
            eff_hdl->eq_out_buf_len = len * 2;
            eff_hdl->eq_out_buf = malloc(eff_hdl->eq_out_buf_len);
            ASSERT(eff_hdl->eq_out_buf);
        }
        audio_eq_set_output_buf(eff_hdl->eq, eff_hdl->eq_out_buf, len);
    }
#endif//TCFG_DRC_ENABLE

    int eqlen = audio_eq_run(eff_hdl->eq, data, len);

#if TCFG_DRC_ENABLE
    if (eff_hdl->drc && !eff_hdl->async) {//同步32bit eq drc 处理
        audio_drc_run(eff_hdl->drc, eff_hdl->eq_out_buf, len * 2);
        user_sat16((s32 *)eff_hdl->eq_out_buf, (s16 *)data, (len * 2) / 4);
    }
#endif//TCFG_DRC_ENABLE

    if (eff_hdl->drc && !eff_hdl->async) {
        return len;
    }
    return eqlen;
#else
    return len;
#endif//TCFG_EQ_ENABLE

}

#endif

//用户自定义eq  drc系数
#if 1

static const struct eq_seg_info your_audio_out_eq_tab[] = {//2段系数
#ifdef EQ_CORE_V1
    {0, EQ_IIR_TYPE_BAND_PASS, 125,   0, 0.7f},
    {1, EQ_IIR_TYPE_BAND_PASS, 12000, 0, 0.7f},

#else
    {0, EQ_IIR_TYPE_BAND_PASS, 125,   0 << 20, (int)(0.7f * (1 << 24))},
    {1, EQ_IIR_TYPE_BAND_PASS, 12000, 0 << 20, (int)(0.7f * (1 << 24))},
#endif
};
static int your_eq_coeff_tab[2][5];//2段eq系数表的长度
/*----------------------------------------------------------------------------*/
/**@brief    用户自定义eq的系数回调
   @param    eq:句柄
   @param    sr:采样率
   @param    info: 系数地址
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
int eq_get_filter_info_demo(void *_eq, int sr, struct audio_eq_filter_info *info)
{
    if (!sr) {
        sr = 44100;
    }
#if TCFG_EQ_ENABLE
    local_irq_disable();
    u8 nsection = ARRAY_SIZE(your_audio_out_eq_tab);
    for (int i = 0; i < nsection; i++) {
        eq_seg_design(&your_audio_out_eq_tab[i], sr, &your_eq_coeff_tab[i]);//根据采样率对eq系数表，重计算，得出适用的系数表
    }
    local_irq_enable();
    info->L_coeff = info->R_coeff = (void *)your_eq_coeff_tab;//系数指针赋值
    info->L_gain = info->R_gain = 0;//总增益填写，用户可修改（-20~20db）,注意大于0db存在失真风险
    info->nsection = nsection;//eq段数，根据提供给的系数表来填写，例子是2
#endif//TCFG_EQ_ENABLE

    return 0;
}

/*----------------------------------------------------------------------------*/
/**@brief    更新自定义eq系数后，需要使用本函数将系数更新到eq模块
   @param    *_drc: 句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void eq_filter_info_update_demo(void *_eq)
{
#if TCFG_EQ_ENABLE
    struct audio_eq *eq = (struct audio_eq *)_eq;
    local_irq_disable();
    if (eq) {
        eq->updata = 1;
    }
    local_irq_enable();
#endif//TCFG_EQ_ENABLE

}



static struct drc_ch drc_fliter = {0};
#define your_threshold  (0)
/*----------------------------------------------------------------------------*/
/**@brief    自定义限幅器系数回调
   @param    *drc: 句柄
   @param    *info: 系数结构地址
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
int drc_get_filter_info_demo(void *drc, struct audio_drc_filter_info *info)
{
#if TCFG_DRC_ENABLE
    float th = your_threshold;//-60 ~0db
    int threshold = round(pow(10.0, th / 20.0) * 32768); // 0db:32768, -60db:33

    drc_fliter.nband = 1;
    drc_fliter.type = 1;
    drc_fliter._p.limiter[0].attacktime = 5;
    drc_fliter._p.limiter[0].releasetime = 300;
    drc_fliter._p.limiter[0].threshold[0] = threshold;
    drc_fliter._p.limiter[0].threshold[1] = 32768;

    info->pch = info->R_pch = &drc_fliter;
#endif//TCFG_DRC_ENABLE

    return 0;
}
#endif


/*----------------------------------------------------------------------------*/
/**@brief    更新自定义限幅器系数后，需要使用本函数将系数更新到drc模块
   @param    *_drc: 句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void drc_filter_info_update_demo(void *_drc)
{
#if TCFG_DRC_ENABLE
    struct audio_drc *drc = (struct audio_drc *)_drc;
    local_irq_disable();
    if (drc) {
        drc->updata = 1;
    }
    local_irq_enable();
#endif//TCFG_DRC_ENABLE

}





