
#include "system/includes.h"
#include "app_config.h"
#include "sport_data/watch_common.h"
#include "sport_data/sleep.h"
#include "sport_data/watch_time_common.h"
#include "ui_vm/ui_vm.h"
#include "message_vm_cfg.h"

#define SL_PATH "storage/sd1/C/sport/sleeptime/SL"
struct sleep_data sleepdata;

#define FILE_WRITE_FLASH TCFG_NOR_VM
/************************************************
			记录短睡眠状态
************************************************/
u16 record_short_sleep(u8 flag)
{
    printf("%s", __func__);
    if (sleepdata.status == 0) {
        //开始记录睡眠状态
        sleepdata.sleeprecord_head = zalloc(sizeof(struct sleep_record)); //创建一个节点
        if (sleepdata.sleeprecord_head == NULL) {
            log_e("sleeprecord_zalloc_error");
            return 0;
        }
        //记录该段睡眠开始时间
        watch_file_get_sys_time(&sleepdata.sleeprecord_head->flag_start);
        sleepdata.sleeprecord_head->flag = flag;
        sleepdata.sleepcnt = 1;
        sleepdata.status = 1; //短睡眠标志
    } else if (sleepdata.status == 1) {
        //记录该段睡眠结束时间
        watch_file_get_sys_time(&sleepdata.sleeprecord_head->flag_start);
    }
    return sleepdata.sleepcnt;
}
/************************************************
			记录长睡眠状态
************************************************/
u16 record_long_sleep(u8 flag)
{
    printf("%s", __func__);

    if (sleepdata.status == 0) {
        log_e("no_short_sleep_flag\n");
        return 0;
    } else if (sleepdata.status == 1) {
        log_e("no_2hours_sleep_flag\n");
        return 0;
    }
    //记录长睡眠状态
    struct sleep_record *tmp = sleepdata.sleeprecord_head;
    printf("tmp=%x", tmp);
    struct sleep_record *psleep;
    for (int i = 1; i < sleepdata.sleepcnt; i++) {
        tmp = tmp->next; //指针移到末尾
    }
    printf("flag=%d,tmp->flag=%d", flag, tmp->flag);
    if (flag == tmp->flag) {
        return 0;
    }
    psleep = zalloc(sizeof(struct sleep_record));
    if (psleep == NULL) {
        log_e("%s psleep zalloc error", __func__);
        return 0;
    }
    watch_file_get_sys_time(&tmp->flag_end);
    watch_time_sub_min(&tmp->flag_end, 1);
    psleep->flag = flag;
    watch_file_get_sys_time(&psleep->flag_start);
    tmp->next = psleep;
    sleepdata.sleepcnt++;
    printf("cnt=%d", sleepdata.sleepcnt);

    return sleepdata.sleepcnt;
}
/************************************************
		进入长睡眠时刷新更换短睡眠状态
************************************************/

u16 record_2hour_sleep(u8 *buf)
{
    printf("%s", __func__);
    if (sleepdata.status == 0) {
        log_e("no_short_sleep_flag\n");
        return 0;
    }
    if (sleepdata.status == 2) {
        log_e("repetition_2hour_sleep\n");
        return 0;
    }
    u8 flag_status = buf[0];
    u8 flag_count = 0;

    struct sleep_record *psleep;
    struct sleep_record *tmp;
    tmp = sleepdata.sleeprecord_head;

    tmp->flag = buf[0];
    printf("buf[0]=%d", buf[0]);
    sleepdata.status = 2;
    for (int i = 1; i < 120; i++) {

        if (flag_status != buf[i])	{ //flag change
            //记录上一段睡眠
            psleep = zalloc(sizeof(struct sleep_record));
            tmp->next = psleep;
            struct sys_time ptime;
            ptime = tmp->flag_start;
            printf("ptime1 %d-%d", ptime.min, ptime.sec);
            printf("cnt=%d", flag_count);
            watch_time_add_min(&ptime, flag_count);
            printf("ptime2 %d-%d", ptime.min, ptime.sec);
            tmp->flag_end = ptime;
            printf("ptime3 %d-%d", ptime.min, ptime.sec);
            printf("timeend %d-%d", tmp->flag_end.min, tmp->flag_end.sec);
            tmp = tmp->next;
            tmp->flag = buf[i];
            sleepdata.sleepcnt++;

            watch_time_add_min(&ptime, 1); //获得下一段睡眠开始时间
            tmp->flag_start = ptime;

            flag_status = buf[i]; //更新新的睡眠标志
            printf("status%d count%d sleepcnt=%d", flag_status, flag_count, sleepdata.sleepcnt);
            printf("time %d-%d", tmp->flag_start.min, tmp->flag_start.sec);

            flag_count = 0; //计算新一段睡眠的长度
        }
        flag_count++;
    }


    return sleepdata.sleepcnt;
}
/************************************************
			获取最近一次睡眠状态
************************************************/

u8 get_sleep_record(struct sleep_record *p)
{
    if (sleepdata.sleeprecord_head != NULL) {
        memcpy(p, sleepdata.sleeprecord_head, sizeof(struct sleep_record));
        return sleepdata.sleepcnt;
    } else {
        return 0;
    }
}

/************************************************
		清空最近一次睡眠数据
*************************************************/

void sleep_flag_free(void)
{
    if (sleepdata.sleepcnt != 0) {
        int cnt = sleepdata.sleepcnt ;
        for (int i = 0; i < cnt; i++) {
            struct sleep_record *tmp = sleepdata.sleeprecord_head;
            for (int n = 1; n < sleepdata.sleepcnt; n++) {
                tmp = tmp->next; //指针移到末尾
            }
            printf("flag=%d", tmp->flag);
            free(tmp);
            sleepdata.sleepcnt--;
            printf("sleepdata.sleepcnt=%d", sleepdata.sleepcnt);

        }
    }
}
/************************************************
  		记录转buf绘图
buf大小通过get_sleep_time_all获取总分钟数申请
************************************************/
int sleep_info_trans_buf(u8 *buf)
{
    if (sleepdata.sleeprecord_head == NULL) {
        log_e("sleepdata.sleeprecord_head==NULL");
        return 0;
    }
    int buf_pos = 0;
    struct sleep_record	*tmp = sleepdata.sleeprecord_head;
    for (int i = 0; i < sleepdata.sleepcnt; i++) {
        int len = watch_time_len_min(&tmp->flag_start, &tmp->flag_end);
        for (int n = 0; n < len; n++) {
            buf[buf_pos] = tmp->flag;
            buf_pos++;
        }
        tmp = tmp->next;
    }
    return buf_pos;
}
int sleep_info_trans_buf_len(void)
{
    if (sleepdata.sleeprecord_head == NULL) {
        log_e("sleepdata.sleeprecord_head==NULL");
        return 0;
    }
    int buf_pos = 0;
    struct sleep_record	*tmp = sleepdata.sleeprecord_head;
    for (int i = 0; i < sleepdata.sleepcnt; i++) {
        int len = watch_time_len_min(&tmp->flag_start, &tmp->flag_end);
        buf_pos += len;
        tmp = tmp->next;
    }
    return buf_pos;
}
//最近一次睡眠的开始时间
int get_sleep_start_time(struct sys_time *sleeptime)
{
    if (sleepdata.sleeprecord_head != NULL) {
        struct sys_time ntime;
        ntime = sleepdata.sleeprecord_head->flag_start;
        printf("yyyy=%d,mm=%d,dd=%d,hh=%d,mm=%d", ntime.year, ntime.month, ntime.day, ntime.hour, ntime.min);
        memcpy(sleeptime, &sleepdata.sleeprecord_head->flag_start, sizeof(struct sys_time));
        return 1;
    }
    return 0;
}
//睡觉结束时间
int get_sleep_stop_time(struct sys_time *sleeptime)
{
    if (sleepdata.sleepcnt != 0) {
        struct sleep_record *tmp = sleepdata.sleeprecord_head;
        for (int n = 1; n < sleepdata.sleepcnt; n++) {
            tmp = tmp->next; //指针移到末尾
        }
        printf("%s month=%d day=%d hour=%d min=%d", __func__, tmp->flag_end.month, tmp->flag_end.day, tmp->flag_end.hour, tmp->flag_end.min);
        memcpy(sleeptime, &tmp->flag_end, sizeof(struct sys_time));
        return 1;
    }
    return 0;
}
int sleep_time_start_stop_time(int type, struct sys_time *sleeptime)
{
    if (type == SLEEP_START_TIME) {
        return get_sleep_start_time(sleeptime);
    } else if (type == SLEEP_STOP_TIME) {
        return get_sleep_stop_time(sleeptime);
    }
    return 0;
}
int get_watch_sleep_target(void)
{
    return 8;
}


/************************************************
				写文件
************************************************/
struct sleep_file {
    u16 file_id;
    u16 w_file_offset;
    struct sys_time file_time;
} sleepfiledata;
int sleep_file_write(void)
{
    printf("%s", __func__);
#if TCFG_NOR_VM
    struct sys_time ntime;
    if (get_sleep_stop_time(&ntime) != 1) {
        log_e("sleep_end_time_error\n");
        return 0;
    }
    if (ntime.day != sleepfiledata.file_time.day) { //当前日期与上一次保存日期不一致
        sleepfiledata.file_time = ntime;
        u8 fbuf[11];//写文件头
        fbuf[0] = 0x05;
        fbuf[1] = (ntime.year >> 8) & 0xff;
        fbuf[2] = (ntime.year & 0xff);
        fbuf[3] = ntime.month;
        fbuf[4] = ntime.day;
        fbuf[5] = 0xff; //CRC
        fbuf[6] = 0xff; //CRC
        fbuf[7] = 0x00; //版本号
        fbuf[8] = 0xff; //存储时间间隔
        fbuf[9] = 0xff; //保留位1
        fbuf[10] = 0xff; //保留位2
        /*printf("filetype=%d,year=%d,month=%d,day=%d",fbuf[0],ntime.year,ntime.month,ntime.day);*/
        if (flash_common_get_total(get_flash_vm_hd(F_TYPE_SLEEP)) == get_flash_vm_number_max(F_TYPE_SLEEP)) {
            printf("total==%d", flash_common_get_total(get_flash_vm_hd(F_TYPE_SLEEP)));
            flash_common_delete_by_index(get_flash_vm_hd(F_TYPE_SLEEP), 0);
        }//清理旧文件
        sleepfiledata.file_id = flash_common_open_id(get_flash_vm_hd(F_TYPE_SLEEP), 0, 512); // 创建新文件
        if (sleepfiledata.file_id == 0) {
            log_e("file_id_error\n");
            return 0;
        }
        printf("file_head id=%d", sleepfiledata.file_id);
        flash_common_write_packet(get_flash_vm_hd(F_TYPE_SLEEP), sleepfiledata.file_id, 11, fbuf); //写入文件头
        sleepfiledata.w_file_offset = 11;
        u8 data_head[4];//数据段首信息
        get_sleep_start_time(&ntime);
        data_head[0] = ntime.hour;
        data_head[1] = ntime.min;
        data_head[2] = ((sleepdata.sleepcnt * 2) >> 8) & 0xff; //len_h
        data_head[3] = (sleepdata.sleepcnt * 2) & 0xff; //len_l
        printf("hour=%d,min=%d", data_head[0], data_head[1]);
        flash_common_write_packet(get_flash_vm_hd(F_TYPE_SLEEP), sleepfiledata.file_id, 4, data_head);
        sleepfiledata.w_file_offset += 4;
        printf("file_data0_head");
        //依次写入本次所有睡眠数据
        struct sleep_record	*tmp = sleepdata.sleeprecord_head;
        for (int i = 0; i < sleepdata.sleepcnt; i++) {
            u8 sleep_point[2];
            sleep_point[0] = tmp->flag;
            sleep_point[1] = (u8)watch_time_len_min(&tmp->flag_start, &tmp->flag_end);
            flash_common_write_packet(get_flash_vm_hd(F_TYPE_SLEEP), sleepfiledata.file_id, 2, sleep_point);
            tmp = tmp->next;
        }

        sleepfiledata.w_file_offset += sleepdata.sleepcnt * 2;
        //写入睡眠评估情况
        data_head[0] = 0xff;
        data_head[1] = 0xff;
        data_head[2] = 0x00; //len_h
        data_head[3] = 0x07; //len_l
        flash_common_write_packet(get_flash_vm_hd(F_TYPE_SLEEP), sleepfiledata.file_id, 4, data_head);
        sleepfiledata.w_file_offset += 4;
        extern void sleep_quality_evaluation_get(void);
        sleep_quality_evaluation_get();//评估
        printf("sleep_quality_evaluation___end_");
        flash_common_write_push(get_flash_vm_hd(F_TYPE_SLEEP), sleepfiledata.file_id);
        printf("offset=%d", sleepfiledata.w_file_offset);

        sleep_flag_free();//清除睡眠数据
    } else {//多段睡眠
        u8 *tbuf = zalloc(sleepfiledata.w_file_offset + 1);
        flash_common_read_by_id(get_flash_vm_hd(F_TYPE_SLEEP), sleepfiledata.file_id, 0, sleepfiledata.w_file_offset, tbuf);
        flash_common_delete_by_id(get_flash_vm_hd(F_TYPE_SLEEP), sleepfiledata.file_id);
        sleepfiledata.file_id = flash_common_open_id(get_flash_vm_hd(F_TYPE_SLEEP), 0, 512);
        flash_common_write_packet(get_flash_vm_hd(F_TYPE_SLEEP), sleepfiledata.file_id, sleepfiledata.w_file_offset, tbuf);
        free(tbuf);
        tbuf = NULL;
        get_sleep_start_time(&ntime);

        u8 data_head[4];//数据段首信息
        data_head[0] = ntime.hour;
        data_head[1] = ntime.min;
        data_head[2] = ((sleepdata.sleepcnt * 2) >> 8) & 0xff; //len_h
        data_head[3] = (sleepdata.sleepcnt * 2) & 0xff; //len_l
        printf("hour=%d,min=%d", data_head[0], data_head[1]);
        flash_common_write_packet(get_flash_vm_hd(F_TYPE_SLEEP), sleepfiledata.file_id, 4, data_head);
        sleepfiledata.w_file_offset += 4;

        printf("offset=%d", sleepfiledata.w_file_offset);

        printf("file_data1_head");

        //依次写入本次所有睡眠数据
        struct sleep_record	*tmp = sleepdata.sleeprecord_head;
        for (int i = 0; i < sleepdata.sleepcnt; i++) {
            u8 sleep_point[2];
            sleep_point[0] = tmp->flag;
            sleep_point[1] = (u8)watch_time_len_min(&tmp->flag_start, &tmp->flag_end);
            flash_common_write_packet(get_flash_vm_hd(F_TYPE_SLEEP), sleepfiledata.file_id, 2, sleep_point);
            tmp = tmp->next;
        }
        sleepfiledata.w_file_offset += sleepdata.sleepcnt * 2;
        //写入睡眠评估情况
        data_head[0] = 0xff;
        data_head[1] = 0xff;
        data_head[2] = 0x00; //len_h
        data_head[3] = 0x07; //len_l
        flash_common_write_packet(get_flash_vm_hd(F_TYPE_SLEEP), sleepfiledata.file_id, 4, data_head);
        sleepfiledata.w_file_offset += 4;
        extern void sleep_quality_evaluation_get(void);
        sleep_quality_evaluation_get();//评估测试
        printf("sleep_quality_evaluation___end_");
        flash_common_write_push(get_flash_vm_hd(F_TYPE_SLEEP), sleepfiledata.file_id);


        printf("offset=%d", sleepfiledata.w_file_offset);
        sleep_flag_free();//清除睡眠数据
    }
//校验文件，这里不适用CRC加密校验，采用按次改位加密的方式,理论上单日睡眠次数不会超过16,超过16次的数据将无法记录
//如需记录16段以上记录可使用协议保留位fbuf[10]fbuf[11]，最高支持32段
    /* u8 CRC[2];	 */
    /* flash_common_read_by_id(get_flash_vm_hd(F_TYPE_SLEEP),sleepfiledata.file_id,5,2,CRC); */
    /* if(CRC!=0){ */
    /* for(int i=0;i<16;i++){ */
    /* if(i<8){ */
    /* if(CRC[0]&BIT(i)==1){ */
    /* CRC[0] &= ~BIT(i); */
    /* break; */
    /* } */
    /* }else{ */
    /* if(CRC[1]&BIT(i-8)==1){ */
    /* CRC[1] &= ~BIT(i-8); */
    /* break; */
    /* } */
    /* } */
    /* } */


    /* }else{ */
    /* flash_common_read_by_id(get_flash_vm_hd(F_TYPE_SLEEP),sleepfiledata.file_id,10,2,CRC);//利用保留位拓展睡眠记录demo */
    /* for(int i=0;i<16;i++){ */
    /* if(i<8){ */
    /* if(CRC[0]&BIT(i)==1){ */
    /* CRC[0] &= ~BIT(i); */
    /* break; */
    /* } */
    /* }else{ */
    /* if(CRC[1]&BIT(i-8)==1){ */
    /* CRC[1] &= ~BIT(i-8); */
    /* break; */
    /* } */
    /* } */
    /* }	 */
    /* } */
#endif
    return 0;
}
/************************************************
  //睡眠模拟

 ************************************************/
#define SL_TEST 0
#define TEST_DAY 1

u16 record_long_sleep_test(u8 flag, u8 len, struct sys_time *ntime)
{
    printf("%s flag=%d len=%d", __func__, flag, len);
    if (flag == 0) {//睡醒
        struct sys_time testtime;
        struct sleep_record *tmp = sleepdata.sleeprecord_head;
        for (int i = 1; i < sleepdata.sleepcnt; i++) {
            tmp = tmp->next;
        }
        printf("flag=%d,tmp->flag=%d", flag, tmp->flag);
        testtime = tmp->flag_start;
        if (len > 60) {//结束时，把最后一段时长补上
            watch_time_add_min(&testtime, 60);
            watch_time_add_min(&testtime, len - 60);
        } else {
            watch_time_add_min(&testtime, len);
        }

        tmp->flag_end = testtime;
        return sleepdata.sleepcnt;
    }
    if (sleepdata.status == 0) {
        //开始记录睡眠状态
        sleepdata.sleeprecord_head = zalloc(sizeof(struct sleep_record)); //创建一个节点
        if (sleepdata.sleeprecord_head == NULL) {
            log_e("sleeprecord_zalloc_error");
            return 0;
        }

        memcpy(&sleepdata.sleeprecord_head->flag_start, ntime, sizeof(struct sys_time)); //赋予开始时间
        sleepdata.sleeprecord_head->flag = flag;
        sleepdata.sleepcnt = 1;
        sleepdata.status = 1;
    } else {
        struct sys_time testtime;
        struct sleep_record *tmp = sleepdata.sleeprecord_head;
        struct sleep_record *psleep;
        for (int i = 1; i < sleepdata.sleepcnt; i++) {
            tmp = tmp->next;
        }
        printf("flag=%d,tmp->flag=%d", flag, tmp->flag);
        psleep = zalloc(sizeof(struct sleep_record));
        if (psleep == NULL) {
            log_e("%s psleep zalloc error", __func__);
            return 0;
        }
        testtime = tmp->flag_start;
        if (len > 60) {
            watch_time_add_min(&testtime, 60);
            watch_time_add_min(&testtime, len - 60);
        } else {
            watch_time_add_min(&testtime, len);
        }
        tmp->flag_end = testtime;
        psleep->flag = flag;
        watch_time_add_min(&testtime, 1);
        psleep->flag_start = testtime;
        tmp->next = psleep;
        sleepdata.sleepcnt++;
        printf("cnt=%d", sleepdata.sleepcnt);

    }
    return sleepdata.sleepcnt;
}

int sleepdeeptime = 0;
int sleepshallowtime = 0;
int sleepremtime = 0;

struct sleep_quality_evaluation {
    char all_sleep_score;
    char deepsleep_scale;
    char shallowsleep_scale;
    char remsleep_scale;
    char sleeptime_evaluation;//4*2bits
    char deepsleep_continue_score;
    char wakeup_times;
};
struct sleep_quality_evaluation *sleep_evaluation;
void sleep_quality_evaluation_get(void)
{
    printf("%s", __func__);
    sleep_evaluation = zalloc(sizeof(struct sleep_quality_evaluation));
    struct sleep_quality_evaluation *__this;
    __this = sleep_evaluation;
#if SL_TEST
    int timeall = sleepdeeptime + sleepshallowtime + sleepremtime; //分钟
    int sleep_time_tmp = (timeall >= 9 * 60) ? (timeall - 9 * 60) : (9 * 60 - timeall);
    __this->all_sleep_score = (100 - (int)(sleep_time_tmp / 10));
    __this->deepsleep_scale = 70;
    __this->shallowsleep_scale = 85;
    __this->remsleep_scale = 60;
    __this->sleeptime_evaluation = 0xfe; //4*2bits
    __this->deepsleep_continue_score = 80;
    __this->wakeup_times = 0;
    sleepdeeptime = 0;
    sleepshallowtime = 0;
    sleepremtime = 0;
#else
    struct watch_sleep __sleep_hd;
    watch_sleep_handle_get(&__sleep_hd);
    int arg[3] = {0};
    __sleep_hd.sleep_time_analyse(arg, SLEEP_ALL);
    __this->all_sleep_score = (char)arg[1];
    __this->sleeptime_evaluation |= ((char)arg[2] << 6);

    __sleep_hd.sleep_time_analyse(arg, SLEEP_DEEP);
    __this->deepsleep_scale = (char)arg[1];
    __this->sleeptime_evaluation |= ((char)arg[2] << 4);

    __sleep_hd.sleep_time_analyse(arg, SLEEP_SHALLOW);
    __this->shallowsleep_scale = (char)arg[1];
    __this->sleeptime_evaluation |= ((char)arg[2] << 2);

    __sleep_hd.sleep_time_analyse(arg, SLEEP_SHALLOW);
    __this->remsleep_scale = (char)arg[1];
    __this->sleeptime_evaluation |= ((char)arg[2] << 0);

    __sleep_hd.sleep_time_analyse(arg, SLEEP_CONTINUE);
    __this->deepsleep_continue_score = (char)arg[1];
    __this->wakeup_times = (char)arg[0];

#endif

    flash_common_write_packet(get_flash_vm_hd(F_TYPE_SLEEP), sleepfiledata.file_id, 7, (u8 *)__this) ;
    sleepfiledata.w_file_offset += 7;
    printf("%s file_id=%d", __func__, sleepfiledata.file_id);
    free(__this);
}
void sleep_file_test(void)
{
#if SL_TEST

    struct sys_time ntime, ptime;
    watch_file_get_sys_time(&ptime);
    printf("%d %d %d ", ptime.year, ptime.month, ptime.day);
    watch_file_prev_x_day(ntime, ptime, TEST_DAY); //前一天
    printf("%d %d %d ", ntime.year, ntime.month, ptime.day);
//day1
    record_long_sleep_test(1, 0, &ntime); //创建一个睡眠记录节点
    record_long_sleep_test(1, 10, NULL); //写入前一天的数据
    u32	timecnt = 0;
    u8 flag_old = 1;
    while (1) {
        u8 flag = rand32() % 3 + 1;
        if (flag == flag_old) {
            continue;
        } else {
            flag_old = flag;
        }
        u8 time_len = rand32() % 110 + 10;
        timecnt += time_len;

        if (timecnt > (7 * 60)) {
            record_long_sleep_test(0, time_len, NULL); //结束记录
            break;
        }
        record_long_sleep_test(flag, time_len, NULL); //记录flag
        switch (flag) {
        case 1:
            sleepshallowtime += time_len;
            break;
        case 2:
            sleepdeeptime += time_len;
            break;
        case 3:
            sleepremtime += time_len;
            break;
        }
    }
    printf("%s moni ok", __func__);
    sleep_file_write();
    sleepdata.status = 0;
    printf("%s record ok!!!", __func__);


#endif
}
