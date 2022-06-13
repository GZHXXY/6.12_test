#include "event.h"
#include "system/includes.h"
#include "app_config.h"
#include "sport_data/watch_common.h"
#include "sport_data/watch_time_common.h"
#include "sport_data/sport.h"
#include "gSensor/SC7A20.h"
#include "gSensor/gSensor_manage.h"
#include "math.h"
#include "ui/ui.h"
#include "ui/ui_style.h"
#include "dev_manager.h"
#include "message_vm_cfg.h"

struct watch_run_info {
    int run_status;//0 无记录 1进行 2暂停 3继续 4结束
    struct motion_info start_info;
    struct motion_info result_info;
    struct motion_info tmp_info;
    struct sys_time timestart;
    struct sys_time timestop;
} run_info ;

struct watch_execise *execise_hd = NULL;
static struct motion_info __execise_info;
static u8 sport_mode;//0非运动 1户外跑 2室内跑
int sport_info_init_status = 0;

extern void sport_start_algo_deal(void);
extern void sport_end_algo_deal(void);

/**************************************************

***************************************************/
int  set_sport_mode(u8 mode)
{
    printf("%s,%d", __func__, mode);
    if (mode >= 0) {
        sport_mode = mode;
    } else {
        sport_mode = 0;
    }
    return 0;
}
u8 execise_mode_get(void)
{
    u8 mode = sport_mode;
    if (mode > 2) { //类型上限，室外跑、室内跑步以外的其他运动用室内跑步代替
        mode = 2;
    }
    printf("%s,%d", __func__, mode);
    return mode;
}

u8 execise_ctrl_status_get(void)
{
    printf("%s %d", __func__, run_info.run_status);
    return run_info.run_status;
}
int execise_ctrl_status_clr(void)
{
    printf("%s %d", __func__, run_info.run_status);
    if (run_info.run_status == 4) {
        run_info.run_status = 0;
        /* set_sport_mode(0); */
        return SUCCESS;
    } else {
        return EXECISE_STATUS_NOT_STOP;
    }
}
/**************************************************
                  运动控制接口
		用于记录运动开始和结束的时间\步数
***************************************************/
static u8 get_sport_info(struct motion_info *info)
{
    if (execise_hd == NULL) {
        execise_hd = zalloc(sizeof(struct watch_execise));
        watch_execise_handle_get(execise_hd);
    }
    return execise_hd->daily_motion_info_get(info);
}

int sport_run_start(void)
{
    printf("%s", __func__);
    if (run_info.run_status == 1) {
        log_e("run_record_now\n");
        return 1;
    }
    run_info.run_status = 0;
    watch_file_get_sys_time(&run_info.timestart);
    memset(&run_info.start_info, 0, sizeof(struct motion_info));
    memset(&run_info.tmp_info, 0, sizeof(struct motion_info));
    memset(&run_info.result_info, 0, sizeof(struct motion_info));
    memset(&__execise_info, 0, sizeof(struct motion_info));
    get_sport_info(&run_info.start_info);
    algo_deal_sport_start();
    run_info.run_status = 1;
    return run_info.run_status;
}
int sport_run_pause(void)
{
    printf("%s", __func__);
    if ((run_info.run_status == 0) | (run_info.run_status == 2)) {
        log_e("run_record_not_start\n");
        return 0;
    }
    get_sport_info(&run_info.tmp_info);
    run_info.result_info.step += (run_info.tmp_info.step - run_info.start_info.step);
    run_info.result_info.step_distance += (run_info.tmp_info.step_distance - run_info.start_info.step_distance);
    run_info.result_info.caloric += (run_info.tmp_info.caloric - run_info.start_info.caloric);
    run_info.result_info.motion_time += (run_info.tmp_info.motion_time - run_info.start_info.motion_time);
    if (run_info.result_info.step_distance != 0) {
        run_info.result_info.motion_pace = (run_info.result_info.motion_time * 1000 / run_info.result_info.step_distance) ; //平均配速 单位秒/公里
    } else {
        run_info.result_info.motion_pace = 0;
    }
    run_info.run_status = 2;
    return run_info.run_status;
}
int sport_run_continue(void)
{
    printf("%s", __func__);
    if (run_info.run_status == 1) {
        log_e("run_record_now\n");
        return 1;
    }
    memset(&run_info.start_info, 0, sizeof(struct motion_info));
    memset(&run_info.tmp_info, 0, sizeof(struct motion_info));
    get_sport_info(&run_info.start_info);
    run_info.run_status = 3;
    return run_info.run_status;
}
int sport_run_stop(void)
{
    printf("%s", __func__);
    if (run_info.run_status == 0) {
        log_e("run_record_not_start\n");
        return 0;
    }
    if (run_info.run_status == 1) {
        watch_file_get_sys_time(&run_info.timestop);
        run_info.run_status = 4;
        return run_info.run_status;
    }
    algo_deal_sport_end();//关闭运动算法,关闭连续心率，刷新一下运动恢复时间和运动强度区间时间
    get_sport_info(&run_info.tmp_info);
    /* run_info.result_info.step += (run_info.tmp_info.step - run_info.start_info.step); */
    /* run_info.result_info.step_distance += (run_info.tmp_info.step_distance - run_info.start_info.step_distance); */
    /* run_info.result_info.caloric += (run_info.tmp_info.caloric - run_info.start_info.caloric); */
    /* run_info.result_info.motion_time += (run_info.tmp_info.motion_time - run_info.start_info.motion_time); */
    /* if (run_info.result_info.step_distance != 0) { */
    /* run_info.result_info.motion_pace = (run_info.result_info.motion_time * 1000 / run_info.result_info.step_distance) ; //平均配速 单位秒/公里 */
    /* } else { */
    /* run_info.result_info.motion_pace = 0; */
    /* } */
    run_info.result_info.recoverytime_hour = run_info.tmp_info.recoverytime_hour;
    run_info.result_info.recoverytime_min = run_info.tmp_info.recoverytime_min;
    run_info.result_info.intensity = run_info.tmp_info.intensity;
    run_info.result_info.intensity_time[0] = run_info.tmp_info.intensity_time[0];
    run_info.result_info.intensity_time[1] = run_info.tmp_info.intensity_time[1];
    run_info.result_info.intensity_time[2] = run_info.tmp_info.intensity_time[2];
    run_info.result_info.intensity_time[3] = run_info.tmp_info.intensity_time[3];
    run_info.result_info.intensity_time[4] = run_info.tmp_info.intensity_time[45];

    watch_file_get_sys_time(&run_info.timestop);

    run_info.run_status = 4;
    return run_info.run_status;
}

int execise_info_get(struct motion_info *info)
{

    if (run_info.run_status == 0) { //无运动记录
        /* printf("%s %d", __func__, __LINE__); */
        memset(info, 0, sizeof(struct motion_info));
        return NOT_SPORT ;
    } else if (run_info.run_status == 1) { //开始，无暂停记录
        /* printf("%s %d", __func__, __LINE__); */
        get_sport_info(&__execise_info);//增量
        __execise_info.step 			-=	run_info.start_info.step;
        __execise_info.step_distance	-=	run_info.start_info.step_distance;
        __execise_info.caloric			-= 	run_info.start_info.caloric;
        __execise_info.motion_time		-=  run_info.start_info.motion_time;
        memcpy(info, &__execise_info, sizeof(struct motion_info));
    } else if (run_info.run_status == 2) { //暂停
        /* printf("%s %d", __func__, __LINE__); */
        memcpy(info, &run_info.result_info, sizeof(struct motion_info));
    } else if (run_info.run_status == 3) { //继续，暂停过
        /* printf("%s %d", __func__, __LINE__); */
        get_sport_info(&__execise_info);//已记录值+未记录值
        __execise_info.step				+= (run_info.result_info.step	-	run_info.start_info.step);
        __execise_info.step_distance	+= (run_info.result_info.step_distance	-	run_info.start_info.step_distance);
        __execise_info.caloric			+= (run_info.result_info.caloric		-	run_info.start_info.caloric);
        __execise_info.motion_time		+= (run_info.result_info.motion_time	-	run_info.start_info.motion_time);
        memcpy(info, &__execise_info, sizeof(struct motion_info));
    } else if (run_info.run_status == 4) { //已结束
        /* printf("%s %d", __func__, __LINE__); */
        memcpy(info, &run_info.result_info, sizeof(struct motion_info));
    }
    return SUCCESS;
}
void execise_info_clr(void)
{
    printf("%s %d", __func__, __LINE__);
    memset(&__execise_info, 0, sizeof(struct motion_info));
}

int execise_ctrl_status_set(unsigned char sport_mode, unsigned char status)
{
    printf("sport_mode=%dstatus= %d", sport_mode, status);
    if (sport_mode != 0xff) {
        set_sport_mode(sport_mode);
    }
    switch (status) {
    case SPORT_STATUS_START:
        sport_run_start();
        break;
    case SPORT_STATUS_PAUSE:
        sport_run_pause();
        break;
    case SPORT_STATUS_CONTINNUE:
        sport_run_continue();
        break;
    case SPORT_STATUS_STOP:
        sport_run_stop();
        break;
    }
    return SUCCESS;
}

int get_sport_strength_time(void)
{
    static u8 strength = 0;
    strength ++ ;
    if (strength > 30) {
        strength = 0;
    }
    return strength;
}

int get_sport_active_time(void)
{
    static u8 time = 0;
    time ++ ;
    if (time > 12) {
        time = 0;
    }
    return time;
}


static const u8 time_table[12] = {1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1};
int get_sport_active_time_table()//活动时间表
{
    int table = 0;
    for (int i = 0; i < sizeof(time_table); i++) {
        if (time_table[i]) {
            table |= BIT(i);
        }
    }
    return table;
}


u32 get_sport_start_time(struct sys_time *t)
{

    u32 time;
    time = ((run_info.timestart.sec	& 0x3F) | \
            ((run_info.timestart.min	& 0x3F) << 6) | \
            ((run_info.timestart.hour	& 0x1F) << 12) | \
            ((run_info.timestart.day	& 0x1F) << 17) | \
            ((run_info.timestart.month	& 0x0F) << 22) | \
            (((run_info.timestart.year - 2010)	& 0x3F) << 26));
    printf("%s=%l", __func__, time);
    if (t) {
        memcpy(t, &run_info.timestart, sizeof(struct sys_time));
    }
    return time;
}

static u16 pace_sec = 0;
int get_sport_pace_sec(void) //每公里调用一次
{
    struct motion_info info;
    execise_info_get(&info);
    u16 pace_now_sec = info.motion_time;
    u16 pace_prev_sec = pace_sec;
    pace_sec = pace_now_sec; //计入当前时长
    printf("pace_sec=%d", pace_now_sec - pace_prev_sec);
    return pace_now_sec - pace_prev_sec; //当前一公里的时长
}

u32 get_sport_end_time(struct sys_time *t)
{

    u32 time;
    time = ((run_info.timestop.sec	& 0x3F) | \
            ((run_info.timestop.min	& 0x3F) << 6) | \
            ((run_info.timestop.hour	& 0x1F) << 12) | \
            ((run_info.timestop.day	& 0x1F) << 17) | \
            ((run_info.timestop.month & 0x0F) << 22) | \
            (((run_info.timestop.year - 2010)	& 0x3F) << 26));
    printf("%s=%l", __func__, time);
    if (t) {
        memcpy(t, &run_info.timestop, sizeof(struct sys_time));
    }
    return time;
}



//******************************
//全天步数、距离、热量记录
//******************************
#include "ui_vm/ui_vm.h"
#include "asm/crc16.h"
#define ST_TIME_INTERVAL 60
enum {
    ST_FILE_NULL,
    ST_FILE_NORMAL,
    ST_FILE_BREAK
};
struct step_file {
    int timer_id1;
    int timer_id2;
    u16 w_file_offset;
    u16 w_data_head_offset;
    u8 file_status;
    u8 file_id;
    u8 daily_cnt;
} stfiledata;
struct motion_info daily_active_record;
static int step_distance_kcal_file_write(void)
{
    printf("%s %d", __func__, __LINE__);
    struct sys_time ntime;
    watch_file_get_sys_time(&ntime);
    printf("%s %d", __func__, __LINE__);
#if TCFG_NOR_VM
    printf("file_status=%d", stfiledata.file_status);
    if (stfiledata.file_status == ST_FILE_NULL) { //当日数据未记录
        printf("%s %d", __func__, __LINE__);
        u8 fbuf[11];//写文件头
        fbuf[0] = 0x09;
        fbuf[1] = (ntime.year >> 8) & 0xff;
        fbuf[2] = (ntime.year & 0xff);
        fbuf[3] = ntime.month;
        fbuf[4] = ntime.day;
        fbuf[5] = 0xff; //CRC
        fbuf[6] = 0xff; //CRC
        fbuf[7] = 0x00; //版本号
        fbuf[8] = ST_TIME_INTERVAL; //存储时间间隔
        fbuf[9] = 0xff; //保留位1
        fbuf[10] = 0xff; //保留位2
        /*printf("filetype=%d,year=%d,month=%d,day=%d,interval=%d",fbuf[0],ntime.year,ntime.month,ntime.day,ST_TIME_INTERVAL);*/
        if (flash_common_get_total(get_flash_vm_hd(F_TYPE_STEP)) == get_flash_vm_number_max(F_TYPE_STEP)) {
            printf("total==%d", flash_common_get_total(get_flash_vm_hd(F_TYPE_STEP)));
            flash_common_delete_by_index(get_flash_vm_hd(F_TYPE_STEP), 0);
        }//清理旧文件
        stfiledata.file_id = flash_common_open_id(get_flash_vm_hd(F_TYPE_STEP), 0, 512); // 创建新文件
        if (stfiledata.file_id == 0) {
            log_e("st_file_id_error\n");
            return 0;
        }
        printf("file_head id=%d", stfiledata.file_id);
        flash_common_write_packet(get_flash_vm_hd(F_TYPE_STEP), stfiledata.file_id, 11, fbuf); //写入文件头
        stfiledata.w_file_offset = 11;
        stfiledata.w_data_head_offset = stfiledata.w_file_offset;
        printf("%s %d", __func__, __LINE__);
        u8 data_head[4];//数据段首信息
        data_head[0] = ntime.hour;
        data_head[1] = ntime.min;
        data_head[2] = 0xff; //len_h
        data_head[3] = 0xff; //len_l
        printf("hour=%d,min=%d", data_head[0], data_head[1]);
        flash_common_write_packet(get_flash_vm_hd(F_TYPE_STEP), stfiledata.file_id, 4, data_head);
        stfiledata.w_file_offset += 4;
        printf("file_data0_head");
    } else if (stfiledata.file_status == ST_FILE_BREAK) { //数据被打断
        u8 *tmp_buf = zalloc(stfiledata.w_file_offset + 1);
        //读取旧文件数据
        flash_common_read_by_id(get_flash_vm_hd(F_TYPE_STEP), stfiledata.file_id, 0, stfiledata.w_file_offset, tmp_buf);
        //删除旧文件
        flash_common_delete_by_id(get_flash_vm_hd(F_TYPE_STEP), stfiledata.file_id);
        //创建新文件
        flash_common_open_id(get_flash_vm_hd(F_TYPE_STEP), stfiledata.file_id, 512); //创建文件
        flash_common_write_packet(get_flash_vm_hd(F_TYPE_STEP), stfiledata.file_id, stfiledata.w_file_offset, tmp_buf);
        free(tmp_buf);
        printf("%s file_packet_ok", __func__);
        u8 data_head[4];
        data_head[0] = ntime.min;
        data_head[1] = ntime.sec;
        data_head[2] = 0xff; //len_h
        data_head[3] = 0xff; //len_l
        flash_common_write_packet(get_flash_vm_hd(F_TYPE_STEP), stfiledata.file_id, 4, data_head);
        stfiledata.w_data_head_offset = stfiledata.w_file_offset;
        stfiledata.w_file_offset += 4;
    }
    //写入一小时数据
    printf("file  seek=%d len=%d ", stfiledata.w_file_offset, (60 / ST_TIME_INTERVAL));
    u8 databuf[6];
    struct motion_info  temp_info;
    get_sport_info(&temp_info);
    u16 step_v = temp_info.step - daily_active_record.step;
    u16 distance_v = temp_info.step_distance - daily_active_record.step_distance;
    u16 kcal_v = temp_info.caloric - daily_active_record.caloric;
    printf("%s %d", __func__, __LINE__);
    databuf[0] = (step_v >> 8);
    databuf[1] = step_v & 0xff;
    databuf[2] = (distance_v >> 8) & 0xff;
    databuf[3] = (distance_v & 0xff);
    databuf[4] = (kcal_v >> 8) & 0xff;
    databuf[5] = kcal_v & 0xff;
    flash_common_write_packet(get_flash_vm_hd(F_TYPE_STEP), stfiledata.file_id, 6	, databuf);
    stfiledata.w_file_offset += 6;
#endif
    //文件标记为正常
    stfiledata.file_status = ST_FILE_NORMAL;
    return 1;
}

static int step_distance_kcal_file_write_break(void)//文件被打断
{
    printf("%s", __func__);
#if TCFG_NOR_VM
    u16 head_data_len = stfiledata.w_file_offset - stfiledata.w_data_head_offset - 4;
    u8 len_buf[2];
    len_buf[0] = (head_data_len >> 8) & 0xff;
    len_buf[1] = head_data_len & 0xff;
    flash_common_write_push(get_flash_vm_hd(F_TYPE_STEP), stfiledata.file_id);
    flash_common_update_by_id(get_flash_vm_hd(F_TYPE_STEP), stfiledata.file_id, stfiledata.w_data_head_offset + 2, 2, len_buf);
#endif
    //标记文件状态为打断
    stfiledata.file_status = ST_FILE_BREAK;
    return 0;
}

static int step_distance_kcal_file_write_stop(void) //跨天更换文件
{
    printf("%s", __func__);
    printf("offset=%d", stfiledata.w_file_offset);
#if TCFG_NOR_VM
    //写入最后一段数的len
    u16 head_data_len = stfiledata.w_file_offset - stfiledata.w_data_head_offset - 4;
    u8 len_buf[2];
    len_buf[0] = (head_data_len >> 8) & 0xff;
    len_buf[1] = head_data_len & 0xff;
    flash_common_write_push(get_flash_vm_hd(F_TYPE_STEP), stfiledata.file_id);
    flash_common_update_by_id(get_flash_vm_hd(F_TYPE_STEP), stfiledata.file_id, stfiledata.w_data_head_offset + 2, 2, len_buf);
    //写入CRC
    u16 temp_data_len = stfiledata.w_file_offset - 11;
    u8 *temp_data = zalloc(temp_data_len + 1);
    flash_common_read_by_id(get_flash_vm_hd(F_TYPE_STEP), stfiledata.file_id, 11, temp_data_len, temp_data);
    u16 crc_value = CRC16(temp_data, temp_data_len);
    u8 crc_buf[2];
    crc_buf[0] = crc_value >> 8 & 0xff;
    crc_buf[1] = crc_value & 0xff;
    flash_common_update_by_id(get_flash_vm_hd(F_TYPE_STEP), stfiledata.file_id, 5, 2, crc_buf);
    free(temp_data);
    temp_data = NULL;
#endif
    //切换文件标记

    stfiledata.file_status = ST_FILE_NULL;
    stfiledata.w_data_head_offset = 0;
    stfiledata.w_file_offset = 0;
    stfiledata.file_id = 0;
    return 0;
}
static void daily_active_file_update(void *priv)
{
    step_distance_kcal_file_write();
    if (stfiledata.timer_id1 != 0) {
        stfiledata.timer_id1 = 0;
    }
    stfiledata.daily_cnt--;
    if (stfiledata.daily_cnt == 0) {
        step_distance_kcal_file_write_stop();
        stfiledata.daily_cnt = 24;
    }
}

int	daily_active_record_start(void)
{
    struct sys_time time;
    watch_file_get_sys_time(&time);
    stfiledata.daily_cnt = 24 - time.hour;
    stfiledata.timer_id1 = sys_timeout_add(NULL, daily_active_file_update, (60 - time.min) * 60 * 1000);
    return 0;
}
int daily_active_record_stop(void)
{
    //关定时器
    if (stfiledata.timer_id1 != 0) {
        sys_timeout_del(stfiledata.timer_id1);
        stfiledata.timer_id1 = 0;
    }
    if (stfiledata.timer_id2 != 0) {
        sys_timer_del(stfiledata.timer_id2);
        stfiledata.timer_id2 = 0;
    }
    if (stfiledata.file_id) {
        step_distance_kcal_file_write_break();
    }
    return 0;
}


