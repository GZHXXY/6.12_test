
#include "system/includes.h"
#include "app_config.h"
#include "sys_time.h"
#include "message_vm_cfg.h"
#include "sport_data/watch_time_common.h"


/*******************公共函数***********************/

/***************************************************
  					日期接口
***************************************************/
u8 monthmax1[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};//
u8 monthmax2[] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};//闰年
// 暂时模拟时间
static int get_elapse_time(struct sys_time *time)
{
    static struct sys_time cur_time = {0};
    static u32 jiffies_offset = 0;
    static u32 jiffies_base = 0;
    int current_msec = jiffies_msec();

    int msec = current_msec + jiffies_offset - jiffies_base;

    if (msec < 0) {  //越界，清0处理
        jiffies_base = 0;
        cur_time.hour = 0;
        cur_time.min = 0;
        cur_time.sec = 0;
        msec = current_msec + jiffies_offset - jiffies_base;
    }

    if (msec >= 1000) {
        do {
            msec -= 1000;
            if (++cur_time.sec >= 60) {
                cur_time.sec = 0;
                if (++cur_time.min >= 60) {
                    cur_time.min = 0;
                    if (++cur_time.hour >= 99) {
                        cur_time.hour = 0;
                    }
                }
            }
        } while (msec >= 1000);

        jiffies_base = current_msec + jiffies_offset - msec;
    }

    memcpy(time, &cur_time, sizeof(struct sys_time));
    /* sprintf(time, "[%02d:%02d:%02d.%03d]", cur_time.hour, cur_time.min, */
    /* cur_time.sec, msec); */

    return 14;
}
void watch_file_get_sys_time(struct sys_time *time)
{
    void *fd = dev_open("rtc", NULL);
    if (!fd) {
        get_elapse_time(time);
        time->year = 2021;
        time->month = 11;
        time->day = 18;
        /* memset(time, 0, sizeof(*time)); */
        return;
    }
    dev_ioctl(fd, IOCTL_GET_SYS_TIME, (u32)time);
    dev_close(fd);
}
void watch_file_prev_day(struct sys_time *ptime, struct sys_time *ntime)
{
    if (ntime->day == 1) {
        if (ntime->month == 1) {
            ptime->day = 31;
            ptime->month = 12;
            ptime->year = ntime->year - 1;
        } else {
            ptime->year = ntime->year;
            ptime->month = ntime->month - 1;
            if (ptime->year % 4 == 0) {
                ptime->day = monthmax2[ptime->month - 1];
            } else {
                ptime->day = monthmax1[ptime->month - 1];
            }
        }
    } else {
        ptime->day = ntime->day - 1;
        ptime->month = ntime->month;
        ptime->year = ntime->year;
    }
}
void watch_file_prev_x_day(struct sys_time *ptime, struct sys_time *ntime, int x)
{
    if (ntime->day <= x) {
        if (ntime->month == 1) {
            ptime->day = (31 - x + ntime->day);
            ptime->month = 12;
            ptime->year = ntime->year - 1;
        } else {
            ptime->year = ntime->year;
            ptime->month = ntime->month - 1;
            if (ptime->year % 4 == 0) {
                ptime->day = (monthmax2[ptime->month - 1] - x + ntime->day);
            } else {
                ptime->day = (monthmax1[ptime->month - 1] - x + ntime->day);
            }
        }
    } else {
        ptime->day = ntime->day - x;
        ptime->month = ntime->month;
        ptime->year = ntime->year;
    }
}
void watch_file_next_day(struct sys_time *ptime, struct sys_time *ntime)
{
    int daymax = (ntime->year % 4 == 0) ? monthmax2[ntime->month - 1] : monthmax1[ntime->month - 1];
    if (ntime->day == daymax) {
        if (ntime->month == 12) {
            ptime->year = ntime->year + 1;
            ptime->month = 1;
            ptime->day = 1;
        } else {
            ptime->year = ntime->year;
            ptime->month = ntime->month + 1;
            ptime->day = 1;
        }
    } else {
        ptime->year = ntime->year;
        ptime->month = ntime->month;
        ptime->day = ntime->day + 1;
    }
}
int watch_file_get_day_num(struct sys_time *time)
{
    int leap = ((time->year / 4) + 1);
    int month_day_num = 0;
    if (time->year % 4 == 0) {
        for (int i = 0; i < time->month - 1; i++) {
            month_day_num += monthmax2[i];
        }
    } else {
        for (int i = 0; i < time->month - 1; i++) {
            month_day_num += monthmax1[i];
        }
    }
    return (leap * 366 + (time->year - leap) * 365 + month_day_num + time->day);
}
void watch_time_add_min(struct sys_time *ptime, u8 min)
{
    if ((ptime->min + min) >= 60) {
        if (ptime->hour != 23) {
            ptime->hour++;
            ptime->min = ptime->min + min - 60;
            return;
        } else {
            ptime->hour = 0;
            int daymax = (ptime->year % 4 == 0) ? monthmax2[ptime->month - 1] : monthmax1[ptime->month - 1];
            if (ptime->day != daymax) {
                ptime->day++;
                ptime->hour = 0;
                ptime->min = ptime->min + min - 60;

            } else {
                if (ptime->month != 12) {
                    ptime->month++;
                    ptime->day = 1;
                    ptime->hour = 0;
                    ptime->min = ptime->min + min - 60;
                } else {
                    ptime->year++;
                    ptime->month = 1;
                    ptime->day = 1;
                    ptime->hour = 0;
                    ptime->min = ptime->min + min - 60;

                }
            }

        }
    } else {
        ptime->min += min;
    }
}
void watch_time_sub_min(struct sys_time *ptime, u8 min)
{
    if (ptime->min - min >= 0) {
        ptime->min = ptime->min - min;
    } else {
        if (ptime->hour == 0) {
            if (ptime->day == 1) {
                if (ptime->month == 1) {
                    ptime->year--;
                    ptime->month = 12;
                    ptime->day = 31;
                    ptime->hour = 23;
                    ptime->min = ptime->min + 60 - min;
                } else {
                    ptime->month--;
                    ptime->day = (ptime->year % 4 == 0) ? monthmax2[ptime->month - 1] : monthmax1[ptime->month - 1];
                    ptime->hour = 23;
                    ptime->min = ptime->min + 60 - min;
                }
            } else {
                ptime->day--;
                ptime->hour = 23;
                ptime->min = ptime->min + 60 - min;
            }
        } else {
            ptime->hour--;
            ptime->min = ptime->min - min + 60;
        }

    }
}
int watch_time_len_min(struct sys_time *ptime, struct sys_time *ntime)
{
    //基准日期2000/01/01
    int prun = (int)((ptime->year - 2000) / 4);
    int pday = prun * 366 + ((ptime->year - 2000 - prun) * 365);
    if (ptime->year % 4 == 0) {
        for (int i = 0; i < (ptime->month - 1); i++) {
            pday += monthmax2[i];
        }
    } else {
        for (int i = 0; i < (ptime->month - 1); i++) {
            pday += monthmax1[i];
        }
    }
    pday += ptime->day;

    int nrun = (int)((ntime->year - 2000) / 4);
    int nday = nrun * 366 + ((ntime->year - 2000 - nrun) * 365);
    if (ntime->year % 4 == 0) {
        for (int i = 0; i < (ntime->month - 1); i++) {
            nday += monthmax2[i];
        }
    } else {
        for (int i = 0; i < (ntime->month - 1); i++) {
            nday += monthmax1[i];
        }
    }
    nday += ntime->day;
    int min;
    min = (nday * 24 * 60 + ntime->hour * 60 + ntime->min) - (pday * 24 * 60 + ptime->hour * 60 + ptime->min);
    printf("min=%d", min);
    return min;

}
u8 watch_time_age(u16 year, u8 month, u8 day)
{
    struct sys_time ntime;
    watch_file_get_sys_time(&ntime);

    int xx, yy;
    if ((ntime.day - day) < 0) {
        xx = 1;
    } else {
        xx = 0;
    }
    if ((ntime.month - month - xx) < 0) {
        yy = 1;
    } else {
        yy = 0;
    }

    return ntime.year - year - yy;
}
struct file_offset {
    u8 type;
    u8 offset;
    u8 len;
};

#include "ui_vm/ui_vm.h"
struct file_offset file_offset_table[] = {
    {F_TYPE_SPORTRECORD, 	15,		4},
    {F_TYPE_HEART,       	1, 		4},
    {F_TYPE_BLOOD_OXYGEN, 	1,	 	4},
    {F_TYPE_SLEEP,       	1,   	4},
    {F_TYPE_STEP,			1,	 	4},
};
#define FILE_OFFSET_TABLE 5
int watch_file_time_to_id(int type, struct sys_time *file_time)
{
    if (file_time == NULL) {
        return 0;
    }
    int table_len = flash_common_get_total(get_flash_vm_hd(type));
    if (table_len = 0) {
        return 0;
    }
    //get_id_table
    u16 id_table[7 * 2];
    flash_common_get_id_table(get_flash_vm_hd(type), table_len * 4, (u8 *)id_table);
    printf("table_len=%d", table_len);
    //get offset
    int offset = 0;
    int len = 0;
    for (int i = 0; i < FILE_OFFSET_TABLE; i++) {
        if (file_offset_table[i].type == type) {
            offset = file_offset_table[i].offset;
            len = file_offset_table[i].len;
        }
    }
    //get file time
    u8 rbuf[4];
    int t;
    struct sys_time time;
    for (int i = 0; i < 7; i++) {
        printf("table,id=%d,size=%d", id_table[i * 2], id_table[i * 2 + 1]);
        //read_file
        flash_common_read_by_id(get_flash_vm_hd(type), id_table[i * 2], offset, len, rbuf);
        //get_file_day
        if (type == F_TYPE_SPORTRECORD) {
            t = ((rbuf[0] << 24) | (rbuf[1] << 16) | (rbuf[2] << 8) | rbuf[3]);
            time.day = (t >> 17) & 0x1f;
            time.month = (t >> 22) & 0xf;
            time.year = ((t >> 26) & 0x3f) + 2010;
        } else {
            time.year = ((rbuf[0] << 8) | (rbuf[1]));
            time.month = rbuf[2];
            time.day = rbuf[3];
        }
        if ((time.year == file_time->year) & (time.month == file_time->month) & (time.day == file_time->day)) {
            return id_table[i * 2];
        }
    }
    return 0;
}
