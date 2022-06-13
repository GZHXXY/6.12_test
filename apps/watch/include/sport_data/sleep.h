#ifndef __SLEEP_H__
#define __SLEEP_H__

struct sleep_record {
    u8 flag;//睡眠类型：RSM深睡浅睡
    struct sys_time flag_start;
    struct sys_time flag_end;
    struct sleep_record *next;
};

struct sleep_data {
    u8 sleepcnt;
    u8 status;//0为初始化 1短睡眠 2长睡眠段开始3长睡眠段结束
    struct sleep_record *sleeprecord_head;

};



//睡眠信息记录
u16 record_short_sleep(u8 flag);
u16 record_long_sleep(u8 flag);
// u16 record_2hour_sleep(short *buf);


//睡眠数据接口，用于绘图
// extern u8 get_sleep_record(struct sleep_record *p);//获取最近一次睡眠数据
// extern void sleep_flag_free(void);//清除最近一次睡眠数据，并释放空间

// extern int sleep_info_trans_buf(u8 *buf);
// extern int sleep_info_trans_buf_len(void);

static int get_sleep_start_time(struct sys_time *sleeptime);//睡眠开始时间
static int get_sleep_stop_time(struct sys_time *sleeptime);//睡眠结束时间
// extern int sleep_time_start_stop_time(int type,struct sys_time *sleeptime)

// extern int get_watch_sleep_target(void);

// extern int sleep_file_write(void);
#endif//__SLEEP_H__
