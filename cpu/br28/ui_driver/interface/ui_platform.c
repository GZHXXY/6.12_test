#include "ui/includes.h"
#include "timer.h"
#include "asm/crc16.h"
#include "ui/lcd_spi/lcd_drive.h"
#include "ascii.h"
#include "font/font_textout.h"
#include "res/new_rle.h"
#include "res/resfile.h"
#include "res/mem_var.h"
#include "ui/res_config.h"
#include "app_config.h"
#include "dev_manager.h"
#include "app_task.h"
#include "smartbox/smartbox_task.h"
#include "fs/fs.h"
#include "asm/imb.h"
#include "ui/ui_measure.h"
#include "ui/watch_syscfg_manage.h"
#include "font/font_all.h"
#include "font/language_list.h"
#if TCFG_SPI_LCD_ENABLE


/***************************************************************/
//此表为语言偏移表,设定指定语言对应指定id，读取指定文件偏移量
static const LANG_TABLE lange_offset_table[16] = {
    [CP874] = {
        CP874,  //指向语言id
        0,//  指向pixel文件的偏移
        0xf400,  //指向tabfile的偏移
    },
    [CP937] = {
        CP937,
        0,
        0,//
    },
    [CPKSC] = {
        CPKSC,
        0xbc050,
        0x13560,//
    },
    [CP1252] = {
        CP1252,
        0,
        0x45864,
    },
    [CP1258] = {
        CP1258,
        0,
        0x2cac0,//
    },
    [CP1256] = {
        CP1256,
        0,
        0x34f4c,//
    },
    [CP1251] = {
        CP1251,
        0,
        0x39192,//
    },
    [CP1254] = {
        CP1254,
        0,
        0x3d3d8,//
    },
    [CP1250] = {
        CP1250,
        0,
        0x4161e,//
    },
    [CP1255] = {
        CP1255,
        0,
        0x49aaa,//
    },
};
/***************************************************************/
#define UI_DEBUG 0
/* #define UI_BUF_CALC */

#if (UI_DEBUG == 1)

#define UI_PUTS puts
#define UI_PRINTF printf

#else

#define UI_PUTS(...)
#define UI_PRINTF(...)

#endif

#define _RGB565(r,g,b)  (u16)((((r)>>3)<<11)|(((g)>>2)<<5)|((b)>>3))
#define UI_RGB565(c)  \
        _RGB565((c>>16)&0xff,(c>>8)&0xff,c&0xff)

#define TEXT_MONO_CLR 0x555aaa
#define TEXT_MONO_INV 0xaaa555
#define RECT_MONO_CLR 0x555aaa
#define BGC_MONO_SET  0x555aaa


struct fb_map_user {
    u16 xoffset;
    u16 yoffset;
    u16 width;
    u16 height;
    u8  *baddr;
    u8  *yaddr;
    u8  *uaddr;
    u8  *vaddr;
    u8 transp;
    u8 format;
};

struct fb_var_screeninfo {
    u16 s_xoffset;            //显示区域x坐标
    u16 s_yoffset;            //显示区域y坐标
    u16 s_xres;               //显示区域宽度
    u16 s_yres;               //显示区域高度
    u16 v_xoffset;      //屏幕的虚拟x坐标
    u16 v_yoffset;      //屏幕的虚拟y坐标
    u16 v_xres;         //屏幕的虚拟宽度
    u16 v_yres;         //屏幕的虚拟高度
    u16 rotate;
};

struct window_head {
    u32 offset;
    u32 len;
    u32 ptr_table_offset;
    u16 ptr_table_len;
    u16 crc_data;
    u16 crc_table;
    u16 crc_head;
};

struct ui_file_head {
    u8  res[16];
    u8 type;
    u8 window_num;
    u16 prop_len;
    u8 rotate;
    u8 rev[3];
};



struct ui_load_info ui_load_info_table[] = {
#if UI_WATCH_RES_ENABLE
    {0, RES_PATH"JL/JL.sty", NULL},
    {1, RES_PATH"watch/watch.sty", NULL},
    {2, RES_PATH"sidebar/sidebar.sty", NULL},
    {3, NULL, NULL},
    {4, UPGRADE_PATH"upgrade.sty", NULL},
#endif
    {-1, NULL, NULL},
};



static u32 ui_rotate = false;
static u32 ui_hori_mirror = false;
static u32 ui_vert_mirror = false;
static int malloc_cnt = 0;
static RESFILE *ui_file = NULL;
static RESFILE *ui_file1 = NULL;
static RESFILE *ui_file2 = NULL;
static u32 ui_file_len = 0;
static u32 last_elm_id = 0xffffffff;
/* static u8 elm_index = 0; */

static int open_resource_file();

extern __attribute__((noinline)) u32 check_ram1_size(void);
extern void *malloc_vlt(size_t size);
extern void free_vlt(void *pv);

static const struct ui_platform_api br28_platform_api;

struct ui_priv {
    struct ui_platform_api *api;
    struct lcd_interface *lcd;
    int window_offset;
    struct lcd_info info;
};
static struct ui_priv priv ALIGNED(4);
#define __this (&priv)

#ifdef UI_BUF_CALC
struct buffer {
    struct list_head list;
    u8 *buf;
    int size;
};
struct buffer buffer_used = {
    .list = {
        .next = &buffer_used.list,
        .prev = &buffer_used.list,
    },
};
#endif



char *file_name[] = {
    "JL",
    "WATCH",
    "WATCH1",
    "WATCH2",
    "WATCH3",
    "WATCH4",
    "WATCH5",
    "FONT",
};

void __attribute__((weak)) virfat_flash_get_dirinfo(void *file_buf, u32 *file_num)
{
    int i;
    for (i = 0; i < sizeof(file_name) / sizeof(file_name[0]); i++) {
        if (file_buf) {
            memcpy(file_buf + 12 * i, file_name[i], strlen(file_name[i]));
        }
    }
    *file_num = sizeof(file_name) / sizeof(file_name[0]);
}

void __attribute__((weak)) virfat_flash_erase_watch(int cmd, u32 arg)
{

}

u32 __attribute__((weak)) virfat_flash_write_watch(void *buf, u32 addr_sec, u32 len)
{
    return 0;
}

u32 __attribute__((weak)) virfat_flash_get_real_capacity() //获取实际flash容量
{
    return 0;
}

u32 __attribute__((weak)) virfat_flash_capacity() //fat上容量
{
    return 0;
}


void *br28_malloc(int size)
{
    void *buf;
    malloc_cnt++;

    if (check_ram1_size() == 0) {
        buf = (void *)malloc(size);
    } else {
        buf = (void *)malloc_vlt(size);
    }

    /* printf("platform_malloc : 0x%x, %d\n", buf, size); */
#ifdef UI_BUF_CALC
    struct buffer *new = (struct buffer *)malloc(sizeof(struct buffer));
    new->buf = buf;
    new->size = size;
    list_add_tail(new, &buffer_used);
    printf("platform_malloc : 0x%x, %d\n", buf, size);

    struct buffer *p;
    int buffer_used_total = 0;
    list_for_each_entry(p, &buffer_used.list, list) {
        buffer_used_total += p->size;
    }
    printf("used buffer size:%d\n\n", buffer_used_total);
#endif

    return buf;
}

void *br28_zalloc(int size)
{
    void *p = br28_malloc(size);
    if (p) {
        memset(p, 0x00, size);
    }
    return p;
}

void br28_free(void *buf)
{
    /* printf("platform_free : 0x%x\n",buf); */
    if (check_ram1_size() == 0) {
        free(buf);
    } else {
        free_vlt(buf);
    }
    malloc_cnt--;

#ifdef UI_BUF_CALC
    struct buffer *p, *n;
    list_for_each_entry_safe(p, n, &buffer_used.list, list) {
        if (p->buf == buf) {
            printf("platform_free : 0x%x, %d\n", p->buf, p->size);
            __list_del_entry(p);
            free(p);
        }
    }

    int buffer_used_total = 0;
    list_for_each_entry(p, &buffer_used.list, list) {
        buffer_used_total += p->size;
    }
    printf("used buffer size:%d\n\n", buffer_used_total);
#endif
}

static void *watch_malloc(int size)
{
    void *buf = (void *)malloc(size);
    return buf;
}
static void *watch_zalloc(int size)
{
    void *p = watch_malloc(size);
    if (p) {
        memset(p, 0x00, size);
    }
    return p;
}
static void watch_free(void *buf)
{
    free(buf);
}

int ui_platform_ok()
{
    return (malloc_cnt == 0);
}

static void draw_rect_range_check(struct rect *r, struct fb_map_user *map)
{
    if (r->left < map->xoffset) {
        r->left = map->xoffset;
    }
    if (r->left > (map->xoffset + map->width)) {
        r->left = map->xoffset + map->width;
    }
    if ((r->left + r->width) > (map->xoffset + map->width)) {
        r->width = map->xoffset + map->width - r->left;
    }
    if (r->top < map->yoffset) {
        r->top = map->yoffset;
    }
    if (r->top > (map->yoffset + map->height)) {
        r->top = map->yoffset + map->height;
    }
    if ((r->top + r->height) > (map->yoffset + map->height)) {
        r->height = map->yoffset + map->height - r->top;
    }

    ASSERT(r->left >= map->xoffset);
    ASSERT(r->top  >= map->yoffset);
    ASSERT((r->left + r->width) <= (map->xoffset + map->width));
    ASSERT((r->top + r->height) <= (map->yoffset + map->height));
}



/* 透明色: 16bits 0x55aa      0101 0xxx 1011 01xx 0101 0xxx
 *         24bits 0x50b450    0101 0000 1011 0100 0101 0000 , 80 180 80
 * */
__attribute__((always_inline))
void __font_pix_copy(struct font_info *info, u8 *pix, int x, int y, int height, int width)
{

    int i, j, h;

    for (j = 0; j < (height + 7) / 8; j++) { /* 纵向8像素为1字节 */
        for (i = 0; i < width; i++) {
            u8 pixel = pix[j * width + i];
            int hh = height - (j * 8);
            if (hh > 8) {
                hh = 8;
            }
            for (h = 0; h < hh; h++) {
                u16 clr = pixel & BIT(h) ? 1 : 0;
                if (clr) {
                    if (info->text_temp_buf) {
                        u8 *pdisp = info->text_temp_buf;
                        int offset = (y / 8 * info->text_show_width + x) + (j * info->text_show_width + i);
                        if (offset < info->text_temp_buf_size) {
                            pdisp[offset] |= BIT(h);
                        }
                    }
                }
            } /* endof for h */
        }/* endof for i */
    }/* endof for j */
}

void l1_data_transformation(u8 *pix, u8 *pix_buf, int x, int y, int height, int width)
{

    int i, j, k = -1, h;

    for (j = 0; j < (height + 7) / 8; j++) { /* 纵向8像素为1字节 */
        for (i = 0; i < width; i++) {
            if ((i % 8) == 0) {
                k++;
            }
            u8 pixel = pix[j * width + i];
            int hh = height - (j * 8);
            if (hh > 8) {
                hh = 8;
            }
            for (h = 0; h < hh; h++) {
                u16 clr = pixel & BIT(h) ? 1 : 0;
                if (clr) {
                    /* pix_buf[(width + 7) / 8 * (j * 8 + h) + k] |= (1 << (i % 8)); */
                    pix_buf[(width + 7) / 8 * (j * 8 + h) + k] |= (1 << (7 - (i % 8)));
                }
            } /* endof for h */
        }/* endof for i */
        k = -1;
    }/* endof for j */
}

void __font_pix_copy2(struct draw_context *dc, int format, struct fb_map_user *map, u8 *pix, struct rect *draw, int x, int y,
                      int height, int width, int color)
{

    int w, h, z;
    u16 osd_color;
    u32 size;

    osd_color = (format == DC_DATA_FORMAT_OSD8) || (format == DC_DATA_FORMAT_OSD8A) ? color & 0xff : color & 0xffff ;

    for (h = 0; h < height; h++) { /* 纵向8像素为1字节 */
        if (((y + h) < draw->top) || ((y + h) > (draw->top + draw->height - 1))) { /* x在绘制区域，要绘制 */
            continue;
        }
        for (w = 0; w < ((width + 7) / 8 * 8); w += 8) {
            u8 pixel = pix[h * ((width + 7) / 8) + w / 8];
            int ww = width - w;
            if (ww > 8) {
                ww = 8;
            }

            for (z = 0; z < ww; z++) {
                if (((x + w + z) < draw->left) || ((x + w + z) > (draw->left + draw->width - 1))) { /* y在绘制区域，要绘制 */
                    continue;
                }
                u16 clr = pixel & BIT(7 - z) ? osd_color : 0;
                if (clr) {
                    if (platform_api->draw_point) {
                        platform_api->draw_point(dc, x + w + z, y + h, clr);
                    }
                }
            } /* endof for h */
        }/* endof for w */
    }/* endof for h */
}

static int image_str_size_check(struct draw_context *dc, int page_num, const char *txt, int *width, int *height)
{

    u16 id = ((u8)txt[1] << 8) | (u8)txt[0];
    u16 cnt = 0;
    struct image_file file = {0};
    int w = 0, h = 0;

    while (id != 0x00ff) {
        struct mem_var *list;
        if ((list = mem_var_search(0, 0, id, page_num, dc->prj)) != NULL) {
            mem_var_get(list, (u8 *)&file, sizeof(struct image_file));
        } else {
            if (open_image_by_id(dc->prj, NULL, &file, id, page_num) != 0) {
                return -EFAULT;
            }
            mem_var_add(0, 0, id, page_num, dc->prj, (u8 *)&file, sizeof(struct image_file));
        }
        w += file.width;
        cnt += 2;
        id = ((u8)txt[cnt + 1] << 8) | (u8)txt[cnt];
    }
    h = file.height;
    *width = w;
    *height = h;
    return 0;
}

#if 1
void platform_putchar(struct font_info *info, u8 *pixel, u16 width, u16 height, u16 x, u16 y)
{
    __font_pix_copy(info,
                    pixel,
                    (s16)x,
                    (s16)y,
                    height,
                    width);
}
#else
void platform_putchar(struct font_info *info, u8 *pixel, u16 width, u16 height, u16 x, u16 y)
{
    int i;
    int offset;
    if (info->text_temp_buf) {
        /* y_printf("width:%d height:%d x:%d y:%d",width,height,x,y); */
        for (i = 0; i < (height + 7) / 8; i++) {
            offset = y / 8 * info->text_show_width + info->text_show_width * i + x;
            if ((offset + width) > info->text_temp_buf_size) {
                width = offset + width - info->text_temp_buf_size;
            }
            memcpy(&info->text_temp_buf[offset], pixel + width * i, width);
        }
    }
}
#endif

struct file_dev {
    const char *logo;
    const char *root_path;
};


struct file_browser {
    int show_mode;
    struct rect r;
    struct vfscan *fscan;
    /* struct server *server; */
    struct ui_file_browser bro;
    struct file_dev dev;//支持三个设备
};


static int check_file_ext(const char *ext_table, const char *ext)
{
    const char *str;

    for (str = ext_table; *str != '\0'; str += 3) {
        if (0 == ASCII_StrCmpNoCase(ext, str, 3)) {
            return true;
        }
    }
    return false;
}

static const u8 MUSIC_SCAN_PARAM[] = "-t"
                                     "MP1MP2MP3"
                                     " -sn -d"
                                     ;




static int platform_file_get_dev_total()
{
    return dev_manager_get_total(1);
}



static void platform_file_browser_get_dev_info(struct ui_file_browser *_bro, u8 index)
{
    struct file_browser *bro = container_of(_bro, struct file_browser, bro);
    struct file_dev *file_dev;//支持三个设备


    if (!bro) {
        return;
    }
    struct __dev *dev = dev_manager_find_by_index(index, 0);//storage_dev_find_by_index(index);
    if (dev) {
        file_dev = &bro->dev;
        file_dev->logo = dev_manager_get_logo(dev);//获取设备logo
        file_dev->root_path = dev_manager_get_root_path_by_logo((char *)file_dev->logo);//获取设备路径
    }
}



static struct ui_file_browser *platform_file_browser_open(struct rect *r,
        const char *path, const char *ftype, int show_mode)
{
    int err;
    struct file_browser *bro;
    struct __dev *dev = 0;
    bro = (struct file_browser *)br28_malloc(sizeof(*bro));
    if (!bro) {
        return NULL;
    }
    bro->bro.file_number = 0;
    bro->show_mode = show_mode;

    if (!path) {
        dev = dev_manager_find_active(0);///storage_dev_last();//获取最后一个设备的路径
        if (!dev) {
            br28_free(bro);
            return NULL;
        }
        path = dev_manager_get_root_path_by_logo(dev_manager_get_logo(dev));//dev->root_path;
    }

    if (!ftype) {
        ftype = MUSIC_SCAN_PARAM;
    }

    bro->fscan = fscan(path, ftype, 9);
    bro->bro.dev_num =  dev_manager_get_total(1);//获取在线设备总数

    if (bro->fscan) {
        bro->bro.file_number = bro->fscan->file_number;
        if (bro->bro.file_number == 0) {
            return &bro->bro;
        }
    }

    if (r) {
        memcpy(&bro->r, r, sizeof(struct rect));
    }

    return &bro->bro;

__err:
    fscan_release(bro->fscan);
    br28_free(bro);
    return NULL;
}



static void platform_file_browser_close(struct ui_file_browser *_bro)
{
    struct file_browser *bro = container_of(_bro, struct file_browser, bro);

    if (!bro) {
        return;
    }
    if (bro->fscan) {
        fscan_release(bro->fscan);
    }
    br28_free(bro);
}

static int platform_get_file_attrs(struct ui_file_browser *_bro,
                                   struct ui_file_attrs *attrs)
{
    int i, j;
    struct vfs_attr attr;
    struct file_browser *bro = container_of(_bro, struct file_browser, bro);

    if (!bro->fscan) {
        return -ENOENT;
    }

    FILE *file = fselect(bro->fscan, FSEL_BY_NUMBER, attrs->file_num + 1);
    if (!file) {
        return -ENOENT;
    }

    attrs->format = "ascii";

    fget_attrs(file, &attrs->attr);
    /* printf(" attr = %x, fsize =  %x,sclust = %x\n",attrs->attr.attr,attrs->attr.fsize,attrs->attr.sclust); */

    struct sys_time *time;
    time =  &(attrs->attr.crt_time);
    /* printf("y =%d  m =%d d = %d,h = %d ,m = %d ,s =%d\n",time->year,time->month,time->day,time->hour,time->min,time->sec);  */

    time =  &(attrs->attr.wrt_time);
    /* printf("y =%d  m =%d d = %d,h = %d ,m = %d ,s =%d\n",time->year,time->month,time->day,time->hour,time->min,time->sec);  */


    int len = fget_name(file, (u8 *)attrs->fname, 16);//长文件获取有问题
    if (len < 0) {
        fclose(file);
        return -EINVAL;
    }

    for (i = 0; i < len; i++) {
        if ((u8)attrs->fname[i] >= 0x80) {
            attrs->format = "uft16";
            goto _next;
        }
    }


    /* ASCII_ToUpper(attrs->fname, strlen(attrs->fname)); */

_next:

#if 0//文件系统接口不完善，临时解决
    for (i = 0; i < len; i++) {
        if (attrs->fname[i] == '.') {
            break;
        }
    }

    if (i == len) {
        attrs->ftype = UI_FTYPE_DIR;
    } else {
        char *ext = attrs->fname + i + 1;

        if (check_file_ext("JPG", ext)) {
            attrs->ftype = UI_FTYPE_IMAGE;
        } else if (check_file_ext("MOVAVI", ext)) {
            attrs->ftype = UI_FTYPE_VIDEO;
        } else if (check_file_ext("MP3WMAWAV", ext)) {
            attrs->ftype = UI_FTYPE_AUDIO;
        } else {
            attrs->ftype = UI_FTYPE_UNKNOW;
        }
    }
#else
    /* printf("name = %d %d \n",strlen(attrs->fname),len); */
    /* put_buf(attrs->fname,len); */

    if (attrs->attr.attr & F_ATTR_DIR) {
        attrs->ftype = UI_FTYPE_DIR;
    } else {
        char *ext = attrs->fname + strlen(attrs->fname) - 3;
        if (check_file_ext("JPG", ext)) {
            attrs->ftype = UI_FTYPE_IMAGE;
        } else if (check_file_ext("MOVAVI", ext)) {
            attrs->ftype = UI_FTYPE_VIDEO;
        } else if (check_file_ext("MP3WMAWAV", ext)) {
            attrs->ftype = UI_FTYPE_AUDIO;
        } else {
            attrs->ftype = UI_FTYPE_UNKNOW;
        }

    }

#endif

    fclose(file);

    return 0;
}

static int platform_set_file_attrs(struct ui_file_browser *_bro,
                                   struct ui_file_attrs *attrs)
{
    int attr = 0;
    struct file_browser *bro = container_of(_bro, struct file_browser, bro);

    if (!bro->fscan) {
        return -ENOENT;
    }

    FILE *file = fselect(bro->fscan, FSEL_BY_NUMBER, attrs->file_num + 1);
    if (!file) {
        return -EINVAL;
    }

    fget_attr(file, &attr);
    if (attrs->attr.attr & F_ATTR_RO) {
        attr |= F_ATTR_RO;
    } else {
        attr &= ~F_ATTR_RO;
    }
    fset_attr(file, attr);

    fclose(file);

    return 0;
}

static void *platform_open_file(struct ui_file_browser *_bro,
                                struct ui_file_attrs *attrs)
{
    struct file_browser *bro = container_of(_bro, struct file_browser, bro);

    if (!bro->fscan) {
        return NULL;
    }

    return fselect(bro->fscan, FSEL_BY_NUMBER, attrs->file_num + 1);
}

static int platform_delete_file(struct ui_file_browser *_bro,
                                struct ui_file_attrs *attrs)
{
    struct file_browser *bro = container_of(_bro, struct file_browser, bro);

    if (!bro->fscan) {
        return -EINVAL;
    }

    FILE *file = fselect(bro->fscan, FSEL_BY_NUMBER, attrs->file_num + 1);
    if (!file) {
        return -EFAULT;
    }
    fdelete(file);

    return 0;
}

void test_browser()
{
    struct ui_file_browser *browser = NULL;
    static struct ui_file_attrs attrs = {0};
    if (!browser) {
        browser = platform_file_browser_open(NULL, NULL, MUSIC_SCAN_PARAM, 0);
    }

    if (browser) {
        printf("file num = %d \n", browser->file_number);
        platform_get_file_attrs(browser, &attrs);
        printf("format =%s name =%s type = %x \n", attrs.format, attrs.fname, attrs.ftype);
        platform_delete_file(browser, &attrs);
        attrs.file_num ++;
        if (attrs.file_num >= browser->file_number) {
            attrs.file_num = 0;
        }
    }
}

static void *br28_set_timer(void *priv, void (*callback)(void *), u32 msec)
{
    return (void *)sys_timer_add(priv, callback, msec);
}

static int br28_del_timer(void *fd)
{
    if (fd) {
        sys_timer_del((int)fd);
    }

    return 0;
}

u32 __attribute__((weak)) set_retry_cnt()
{
    return 10;
}

const char *WATCH_VERSION_LIST = {
    "W001"

};

/* static const char *WATCH_STY_CHECK_LIST[] = { */
/* RES_PATH"JL/JL.sty", */
/* RES_PATH"watch/watch.sty", */
/* RES_PATH"watch1/watch1.sty", */
/* RES_PATH"watch2/watch2.sty", */
/* RES_PATH"watch3/watch3.sty", */
/* RES_PATH"watch4/watch4.sty", */
/* RES_PATH"watch5/watch5.sty", */
/* }; */

#define WATCH_ITEMS_LIMIT       40
#define BGP_ITEMS_LIMIT         (WATCH_ITEMS_LIMIT + 10)//(确保 >= WATCH_ITEMS_LIMIT就可以)

struct __WATCH_STYLE {
    u8 watch_style;
    s8 standby_watch_style;
};

static struct __WATCH_STYLE dial_sty;
static u8 watch_items;
static volatile u8 watch_update_over = 0;

static char *watch_res[WATCH_ITEMS_LIMIT] = {0};
static char *watch_bgp_related[WATCH_ITEMS_LIMIT] = {0};
static char *watch_bgp[BGP_ITEMS_LIMIT] = {0};
static u8 watch_bgp_items;

static char *watch_bgpic_path = NULL;

extern u8 smartbox_eflash_flag_get(void);
extern u8 smartbox_eflash_update_flag_get(void);
extern void smartbox_eflash_flag_set(u8 eflash_state_type);
extern int smartbox_eflash_resume_opt(void);
static int watch_mem_bgp_related();

u32 watch_bgp_get_nums()
{
    return watch_bgp_items;
}

char *watch_bgp_get_item(u8 sel_item)
{
    if (sel_item >= watch_bgp_items) {
        printf("\n\n\nwatch_bgp items overflow %d\n\n\n\n", sel_item);
        return NULL;
    }

    return watch_bgp[sel_item];
}

char *watch_bgp_add(char *bgp)
{
    char *root_path = RES_PATH;
    char *new_bgp_item = NULL;
    u32 len;
    u32 i;

    if ((watch_bgp_items + 1) >= BGP_ITEMS_LIMIT) {
        printf("\n\n\nwatch_bgp items overflow %d\n\n\n\n", watch_bgp_items);
        return NULL;
    }

    len = strlen(bgp) + strlen(root_path) + 1;
    new_bgp_item = watch_malloc(len);
    if (new_bgp_item == NULL) {
        printf("\n\n\nwatch_bgp items malloc err %d\n\n\n\n", len);
        return NULL;
    }

    ASCII_ToLower(bgp, strlen(bgp));
    strcpy(new_bgp_item, root_path);
    /* strcpy(&new_bgp_item[strlen(root_path)], &bgp[1]); */
    strcpy(&new_bgp_item[strlen(root_path)], bgp);
    new_bgp_item[len - 1] = '\0';

    //如果已经存在这个背景图片路径，则直接返回对应的地址
    for (i = 0; i < watch_bgp_items; i++) {
        /* printf("num %s\n", watch_bgp[i]); */
        if (strncmp(new_bgp_item, watch_bgp[i], strlen(new_bgp_item)) == 0) {
            watch_free(new_bgp_item);
            new_bgp_item = watch_bgp[i];
            printf("already has %s\n", new_bgp_item);
            return new_bgp_item;
        }
    }

    watch_bgp[watch_bgp_items] = new_bgp_item;
    watch_bgp_items++;

    printf("add new_bgp_item succ %d, %s\n", watch_bgp_items, new_bgp_item);

    return new_bgp_item;
}

int watch_bgp_del(char *bgp)
{
    u32 i;
    char watch_bgp_item[64];
    u32 cur_items = watch_bgp_items;
    char *root_path = RES_PATH;
    char *bgp_item = NULL;

    ASSERT(((strlen(bgp) + strlen(root_path) + 1) < sizeof(watch_bgp_item)), "bgp err name0 %s\n", bgp);

    ASCII_ToLower(bgp, strlen(bgp));
    strcpy(watch_bgp_item, root_path);
    /* strcpy(&watch_bgp_item[strlen(root_path)], &bgp[1]); */
    strcpy(&watch_bgp_item[strlen(root_path)], bgp);
    watch_bgp_item[strlen(bgp) + strlen(root_path)] = '\0';
    printf("watch_bgp_item %s\n", watch_bgp_item);

    for (i = 0; i < cur_items; i++) {
        if (strncmp(watch_bgp_item, watch_bgp[i], strlen(watch_bgp_item)) == 0) {
            bgp_item = watch_bgp[i];
            watch_bgp[i] = NULL;
            watch_free(bgp_item);
            watch_bgp_items--;
            break;
        }
    }

    if (bgp_item == NULL) {
        printf("can not find bgp_item %s\n", watch_bgp_item);
        return -1;
    }

    for (; i < cur_items; i++) {
        if (watch_bgp[i + 1] != NULL) {
            watch_bgp[i] = watch_bgp[i + 1];
        } else {
            watch_bgp[i] = NULL;
            break;
        }
    }

    //del related item
    cur_items = sizeof(watch_bgp_related) / sizeof(watch_bgp_related[0]);
    for (i = 0; i < cur_items; i++) {
        if (bgp_item == watch_bgp_related[i]) {
            watch_bgp_related[i] = NULL;
        }
    }

    for (i = 0; i < cur_items; i++) {
        printf("cur related items %d, %s\n", i, watch_bgp_related[i]);
    }
    for (i = 0; i < watch_bgp_items; i++) {
        printf("cur bgp items %d, %s\n", i, watch_bgp[i]);
    }

    return 0;
}


//替换背景图片,(1)如果某个表盘没有背景图片，则增加，(2)如果有就替换
//如果被替换的这个背景图片已经不关联任何表盘，则将它从表watch_bgp删除
int watch_bgp_set_related(char *bgp, u8 cur_watch, u8 del)
{
    u32 i;
    u32 cur_items;
    char *root_path = RES_PATH;
    char *bgp_item = NULL;
    char *new_bgp_item = NULL;
    u32 total_relate_items;
    char old_bgp[16];
    u32 len;

    total_relate_items = sizeof(watch_bgp_related) / sizeof(watch_bgp_related[0]);
    if (cur_watch >= total_relate_items) {
        printf("\n\n\nwatch_bgp_related items overflow %d\n\n\n\n", cur_watch);
        return -1;
    }

    //提取旧的背景图片名字
    if (watch_bgp_related[cur_watch]) {
        bgp_item = watch_bgp_related[cur_watch];
        len = strlen(bgp_item) - strlen(root_path) + 1;
        if (len >= sizeof(old_bgp)) {
            return -1;
        }
        memcpy(old_bgp, &bgp_item[strlen(root_path)], len);
        printf("old bpg %s\n", old_bgp);
    }

    if (bgp != NULL) {
        new_bgp_item = watch_bgp_add(bgp);
        if (new_bgp_item == NULL) {
            printf("add bgp item err %s\n", bgp);
            return -1;
        }
    }

    //如果没有背景图片
    if (watch_bgp_related[cur_watch] == NULL) {
        watch_bgp_related[cur_watch] = new_bgp_item;
        /* printf("a1\n"); */
    } else {
        bgp_item = watch_bgp_related[cur_watch];
        watch_bgp_related[cur_watch] = new_bgp_item;
        cur_items = sizeof(watch_bgp_related) / sizeof(watch_bgp_related[0]);
        /* printf("a2 %s\n", bgp_item); */
        for (i = 0; i < cur_items; i++) {
            if (bgp_item == watch_bgp_related[i]) {
                /* printf("a3 %d\n", i); */
                break;
            }
        }
        if ((i == cur_items) && del) { //被替换的这个背景图片已经不关联任何表盘
            /* printf("a4\n"); */
            watch_bgp_del(old_bgp);
        }
    }

    watch_mem_bgp_related();

    return 0;
}

//根据某个表盘获取对应的背景图片
char *watch_bgp_get_related(u8 cur_watch)
{
    u32 total_relate_items;

    total_relate_items = sizeof(watch_bgp_related) / sizeof(watch_bgp_related[0]);
    if (cur_watch >= total_relate_items) {
        printf("\n\n\nwatch_bgp_related items overflow %d\n\n\n\n", cur_watch);
        return NULL;
    }

    return watch_bgp_related[cur_watch];
}

int watch_bgp_related_del_all(char *bgp)
{
    watch_bgp_del(bgp);
    watch_mem_bgp_related();
    return 0;
}

int watch_get_style_by_name(char *name)
{
    u32 i;
    u32 ret;
    printf("watch find %s\n", name);

    for (i = 0; i < watch_items; i++) {
        /* printf("watch finding %s\n", watch_res[i]); */
        if (strncmp(name, watch_res[i], strlen(name)) == 0) {
            printf("find watch style %d, %s\n\n\n", i, watch_res[i]);
            return i;
        }
    }

    printf("watch find faile\n");
    return -1;
}

#define WATCH_MEM_BGP       0x55aa66bb
static u32 wmem_last = 0;
static u32 wmem_area_num = 0;
static void *wmem_file = NULL;

int watch_mem_new(u32 area)
{
    wmem_last = area;
    wmem_area_num++;
    return 0;
}

void *watch_mem_open()
{
    if (wmem_file == NULL) {
        /* wmem_file = fopen(RES_PATH"wmem.bin", "w+"); */
        wmem_file = fopen("storage/sd1/C/download/wmem.bin", "w+");
    }
    return wmem_file;
}

void watch_mem_close()
{
    if (wmem_file) {
        fclose(wmem_file);
        wmem_file = NULL;
    }
}

int watch_mem_write(u32 offset, u32 len, u8 *buf, u32 area)
{
    u32 ret;
    u8 tmp_buf[8];
    u32 area_offset = 0;
    u32 area_len = 0;
    u32 find_tag = 0;

    if (wmem_file == NULL) {
        return -1;
    }

    if ((flen(wmem_file) == 0) || (wmem_area_num <= 1)) {
        area_offset = 0;
    } else {
        do {
            fseek_fast(wmem_file, area_offset, SEEK_SET);
            ret = fread_fast(wmem_file, tmp_buf, 8);
            if (ret != 8) {
                printf("wmem find tag err end\n");
                return -1;
            }
            memcpy(&ret, tmp_buf + 4, 4); //flag
            if (ret == area) {
                memcpy(&area_len, tmp_buf, 4);//len
                find_tag = 1;
                break;
            }
            memcpy(&ret, tmp_buf, 4);//len
            area_offset += ret;
        } while (find_tag == 0);
    }

    fseek_fast(wmem_file, area_offset + offset, SEEK_SET);
    if (area == wmem_last) {
        ret = fwrite(wmem_file, buf, len);
        if (ret != len) {
            return -1;
        }
    } else {
        if ((offset + len) <= area_len) {
            ret = fwrite(wmem_file, buf, len);
            if (ret != len) {
                return -1;
            }
        } else {
            //要将这个区域的内容先搬迁到文件最后，使这个区域成为最后的区域
            //再写数据

        }
    }

    return 0;
}

/* static u32 wmem_test_flag = 0; */
int watch_mem_read(u32 offset, u32 len, u8 *buf, u32 area)
{
    u32 area_offset = 0;
    u32 area_len = 0;
    u32 find_tag = 0;
    u32 ret;
    u8 tmp_buf[8];

    if (wmem_file == NULL) {
        return -1;
    }

    if (flen(wmem_file) == 0) {
        /* wmem_test_flag = 1; */
        return 0;
    }


    if (wmem_area_num <= 1) {
        area_offset = 0;
    } else {
        do {
            fseek_fast(wmem_file, area_offset, SEEK_SET);
            ret = fread_fast(wmem_file, tmp_buf, 8);
            if (ret != 8) {
                printf("wmem find tag err end\n");
                return -1;
            }
            memcpy(&ret, tmp_buf + 4, 4); //flag
            if (ret == area) {
                memcpy(&area_len, tmp_buf, 4);//len
                find_tag = 1;
                break;
            }
            memcpy(&ret, tmp_buf, 4);//len
            area_offset += ret;
        } while (find_tag == 0);

        if ((offset + len) > area_len) {
            return -1;
        }
    }

    fseek_fast(wmem_file, area_offset + offset, SEEK_SET);
    ret = fread_fast(wmem_file, buf, len);
    if (ret != len) {
        printf("wmem read err %d\n", ret);
        return -1;
    }

    return 0;
}


static int watch_mem_bgp_related()
{
    int ret = 0;
    u8 *related_buf;
    u32 total_relate_items = sizeof(watch_bgp_related) / sizeof(watch_bgp_related[0]);
    u32 len;
    char *related_item;
    u32 area = WATCH_MEM_BGP;
    u32 i;

    if (watch_bgp_items == 0) {
        return -1;
    }

    len = 64 * (total_relate_items + 1);
    related_buf = br28_malloc(len);
    if (related_buf == NULL) {
        return -1;
    }

    if (watch_mem_open() == NULL) {
        br28_free(related_buf);
        return -1;
    }

    memcpy(related_buf, &len, 4);
    memcpy(related_buf + 4, &area, 4);
    for (i = 0; i < total_relate_items; i++) {
        related_item = watch_bgp_related[i];
        if (related_item) {
            printf("related item : %s, %d, %d\n", related_item, strlen(related_item) + 1, i);
            memcpy(&related_buf[64 + 64 * i], related_item, strlen(related_item) + 1);
        }
    }
    ret = watch_mem_write(0, len, related_buf, area);
    if (ret != 0) {
        printf("watch mem werr %x\n", ret);
    } else {
        printf("watch mem succ\n");
    }

    watch_mem_close();

    br28_free(related_buf);

    return ret;
}

static int watch_bgp_related_init()
{
    static u8 flag = 0;
    int ret = 0;
    u8 *related_buf;
    u32 total_relate_items = sizeof(watch_bgp_related) / sizeof(watch_bgp_related[0]);
    u32 len;
    char *related_item;
    u32 area = WATCH_MEM_BGP;
    u32 i, j;

    if (flag == 0) {
        watch_mem_new(area);
        flag = 1;
    }

    if (watch_bgp_items == 0) {
        return -1;
    }

    len = 64 * (total_relate_items + 1);
    related_buf = br28_malloc(len);
    if (related_buf == NULL) {
        return -1;
    }

    if (watch_mem_open() == NULL) {
        br28_free(related_buf);
        return -1;
    }

    ret = watch_mem_read(0, len, related_buf, area);
    if (ret != 0) {
        br28_free(related_buf);
        return -1;
    }

    for (i = 0; i < total_relate_items; i++) {
        watch_bgp_related[i] = NULL;
        related_item = &related_buf[64 + 64 * i];
        if (related_item[0]) {
            printf("related item : %s, %d, %d\n", related_item, strlen(related_item), i);
            for (j = 0; j < watch_bgp_items; j++) {
                if (strncmp(related_item, watch_bgp[j], strlen(watch_bgp[j])) == 0) {
                    watch_bgp_related[i] =  watch_bgp[j];
                    printf("find bgp related %d\n", j);
                    break;
                }
            }
        }
    }

    watch_mem_close();

    br28_free(related_buf);

    return 0;
}



char *watch_get_background()
{
    if (watch_bgpic_path == NULL) {
        return NULL;
    }
    return watch_bgpic_path;
}

int watch_set_background(char *bg_pic)
{
    u32 bg_strlen = strlen(bg_pic);
    u32 root_path_strlen = strlen(RES_PATH);

    /* if ((bg_strlen + root_path_strlen) >= sizeof(watch_bgpic_path)) { */
    /* printf("set background err %d, %d\n", bg_strlen, root_path_strlen); */
    /* return -1; */
    /* } */

    if (bg_pic == NULL) {
        /* memset(watch_bgpic_path, 0, sizeof(watch_bgpic_path)); */
        if (watch_bgpic_path != NULL) {
            br28_free(watch_bgpic_path);
        }
        watch_bgpic_path = NULL;
    } else {

        if (watch_bgpic_path != NULL) {
            br28_free(watch_bgpic_path);
            watch_bgpic_path = NULL;
        }

        watch_bgpic_path = br28_malloc(bg_strlen + root_path_strlen + 1);
        if (watch_bgpic_path == NULL) {
            printf("set background err %d, %d\n", bg_strlen, root_path_strlen);
            return -1;
        }

        strcpy(watch_bgpic_path, RES_PATH);
        strcpy(watch_bgpic_path + root_path_strlen, bg_pic);
        watch_bgpic_path[root_path_strlen + bg_strlen] = '\0';
    }

    return 0;
}

void watch_update_finish()
{
    printf("\n\nwatch update finish\n\n");
    watch_update_over = 1;
}

int watch_get_update_status()
{
    return watch_update_over;
}

char *watch_get_item(int style)
{
    if (style >= watch_items) {
        return NULL;
    }

    return watch_res[style];
}
char *watch_get_root_path()
{
    return RES_PATH;
}

int watch_set_style(int style)
{
#if UI_WATCH_RES_ENABLE
    /* if (style == 0) { */
    /* style = 1; */
    /* } */
    /* if (style > sizeof(watch_res) / sizeof(watch_res[0])) { */
    if (style >= watch_items) {
        return false;
    }
    dial_sty.watch_style = style;
#endif
    return true;
}

static int watch_get_msg_by_path(char *key, char *data, u32 data_len, u32 offset)
{
    int ret = 0;
    RESFILE *file = res_fopen(data, "r");
    if (NULL == file) {
        return -1;
    }
    // 获取长度
    u32 tmp_strlen = res_flen(file);
    // 用来装路径名字
    u8 *tmp_data = br28_malloc(strlen(data) + 1);
    if (NULL == tmp_data) {
        return -1;
    }
    memcpy(tmp_data, data, strlen(data) + 1);
    memset(data, 0, data_len);
    // 读出数据
    res_fseek(file, offset, SEEK_SET);
    res_fread(file, data, data_len);

    // 关闭数据
    if (file) {
        res_fclose(file);
    }

    char *tver = NULL;
    for (u32 i = 0; i < data_len; i++) {
        // 换行
        if (0xd == data[i] && 0x0a == data[i + 1]) {
            offset += i + 1 + 1;
            break;
        }
        // 文件结束
        if (0 == data[i] || (offset + i >= tmp_strlen)) {
            offset += i;
            tmp_strlen = 0;
            break;
        }
        if ('"' == data[i]) {
            if (tver) {
                tver = data + i + 1;
                break;
            }
            if (0 == strncmp(key, data + i + 1, strlen(key))) {
                i += strlen(key) + 1 + 1;
                tver = data + i - 1;
                if ('"' != tver[0]) {
                    tver = NULL;
                }
            }
        }
    }

    if (tver) {
        tmp_strlen = index(tver, '"') - tver;
        memcpy(data, tver, tmp_strlen);
        data[tmp_strlen] = '\0';
    } else {
        memcpy(data, tmp_data, strlen(tmp_data) + 1);
        ret = -1;
    }

    if (tmp_data) {
        br28_free(tmp_data);
    }

    if (offset < tmp_strlen && NULL == tver) {
        ret = watch_get_msg_by_path(key, data, data_len, offset);
    }

    return ret;
}

static int watch_get_msg_from_json(char *watch_item, char *value, char *key)
{
    int ret = 0;
    char *sty_suffix = ".sty";
    char *json_suffix = ".json";
    u32 tmp_strlen;
    char tmp_name[64];
    u32 sty_strlen;

    if (watch_item == NULL) {
        return -1;
    }

    sty_strlen = strlen(sty_suffix);
    tmp_strlen = strlen(watch_item);
    strcpy(tmp_name, watch_item);
    strcpy(&tmp_name[tmp_strlen - sty_strlen], json_suffix);
    tmp_name[tmp_strlen - sty_strlen + strlen(json_suffix)] = '\0';
    printf("key %s, path %s\n", key, tmp_name);
    ret = watch_get_msg_by_path(key, tmp_name, sizeof(tmp_name), 0);
    if (0 == ret) {
        memcpy(value, tmp_name, strlen(tmp_name) + 1);
    }
    return ret;
}

int watch_get_uuid(char *watch_item, char *uuid)
{
    return watch_get_msg_from_json(watch_item, uuid, "prj_uuid");
}

int watch_get_version(char *watch_item, char *version)
{
#if 1
    return watch_get_msg_from_json(watch_item, version, "version_id");
#else
    char *sty_suffix = ".sty";
    char *json_suffix = ".json";
    u32 tmp_strlen;
    char tmp_name[64];
    u32 sty_strlen;
    RESFILE *file;
    /* char *version; */
    char *tver;
    u32 i;

    if (watch_item == NULL) {
        return -1;
    }

    sty_strlen = strlen(sty_suffix);
    tmp_strlen = strlen(watch_item);
    strcpy(tmp_name, watch_item);
    strcpy(&tmp_name[tmp_strlen - sty_strlen], json_suffix);
    tmp_name[tmp_strlen - sty_strlen + strlen(json_suffix)] = '\0';
    printf("version name %s\n", tmp_name);
    file = res_fopen(tmp_name, "r");
    if (!file) {
        printf("open_jsonfile fail %s\n", tmp_name);
        return -1;
    }
    memset(tmp_name, 0, sizeof(tmp_name));
    res_fread(file, tmp_name, sizeof(tmp_name));
    res_fclose(file);
    /* printf("json buf : %s\n", tmp_name); */
    tmp_name[26] = '\0';
    /* version = &tmp_name[22]; */
    strcpy(version, &tmp_name[22]);

    return 0;
#endif
}


int watch_version_juge(char *watch_item)
{
#if 1
    char *tver;
    char version[64] = {0};
    if (0 == watch_get_version(watch_item, version)) {
        for (u8 i = 0; i < strlen(WATCH_VERSION_LIST); i += 5) {
            tver = (char *)&WATCH_VERSION_LIST[i];
            if (0 == strncmp(version, tver, strlen(version))) {
                return 0;
            }
        }
    }
    printf("juge watch version err\n");

    return -1;
#else
    char *sty_suffix = ".sty";
    char *json_suffix = ".json";
    u32 tmp_strlen;
    char tmp_name[64];
    u32 sty_strlen;
    RESFILE *file;
    char *version;
    char *tver;
    u32 i;

    if (watch_item == NULL) {
        return -1;
    }

    sty_strlen = strlen(sty_suffix);
    tmp_strlen = strlen(watch_item);
    strcpy(tmp_name, watch_item);
    strcpy(&tmp_name[tmp_strlen - sty_strlen], json_suffix);
    tmp_name[tmp_strlen - sty_strlen + strlen(json_suffix)] = '\0';
    printf("version name %s\n", tmp_name);
    file = res_fopen(tmp_name, "r");
    if (!file) {
        printf("open_jsonfile fail %s\n", tmp_name);
        return -1;
    }
    memset(tmp_name, 0, sizeof(tmp_name));
    res_fread(file, tmp_name, sizeof(tmp_name));
    res_fclose(file);
    printf("json buf : %s\n", tmp_name);
    tmp_name[0x2f] = '\0';
    version = &tmp_name[0x2b];
    printf("ve %s\n", version);

    for (i = 0; i < strlen(WATCH_VERSION_LIST); i += 5) {
        tver = &WATCH_VERSION_LIST[i];
        /* printf("ver %s, %d\n", tver, strlen(version)); */
        printf("tver %s, %s\n", tver, version);
        if (strncmp(version, tver, strlen(version)) == 0) {
            return 0;
        }
    }

    printf("juge watch version err\n");

    return -1;
#endif
}


int watch_set_style_by_name(char *name)
{
    u32 i;
    u32 ret;
    printf("watch find %s\n", name);

    for (i = 0; i < watch_items; i++) {
        /* printf("watch finding %s\n", watch_res[i]); */
        if (strncmp(name, watch_res[i], strlen(name)) == 0) {
            printf("find watch style %d, %s\n\n\n", i, watch_res[i]);

            ret = watch_version_juge(watch_res[i]);
            if (ret != 0) {
                break;
            }
            watch_set_style(i);
            return 0;
        }
    }

    printf("watch find faile\n");
    return -1;
}

int standby_watch_set_style(int style)
{
    dial_sty.standby_watch_style = style;
    /* printf("standby_watch_style:%d",dial_sty.standby_watch_style); */
    return style;
}

int standby_watch_get_style()
{
    return dial_sty.standby_watch_style;
}

int watch_get_style()
{
    return dial_sty.watch_style;
}

int watch_get_items_num()
{
    return watch_items;
}

char *watch_get_cur_path()
{
    static char path[16];
    u8 i;
    char *tmp = &watch_res[dial_sty.watch_style][strlen(RES_PATH)];

    path[0] = '/';
    for (i = 0; i < 15; i++) {
        if (tmp[i] == '/') {
            break;
        }
        path[i + 1] = tmp[i];
        putchar(tmp[i]);
    }
    path[i + 1] = '\0';

    ASCII_ToUpper(path, strlen(path));
    /* printf("cur : %s, %s, %d\n", tmp, path, i); */

    /* return watch_res[watch_style]; */
    return path;
}

int watch_get_cur_path_len()
{
    return strlen(watch_get_cur_path()) + 1;

    /* char *path = watch_res[watch_style]; */
    /* u32 i; */

    /* for (i = strlen(path) - 1; i > 0; i--) { */
    /* if (path[i] == '/') { */
    /* break; */
    /* } */
    /* } */

    /* return i + 1; */
}

extern void virfat_flash_get_dirinfo(void *file_buf, u32 *file_num);
void watch_add_item_deal(void)
{
    u32 i, j, k;
    u32 index = -1;
    u32 file_num;
    u8 *fname_buf = NULL;
    u8 *fname = NULL;
    u8 fname_len;

    virfat_flash_get_dirinfo(NULL, &file_num);

    fname_buf = watch_malloc(file_num * 12);
    if (NULL == fname_buf) {
        goto __watch_add_item_deal_end;
    }

    virfat_flash_get_dirinfo(fname_buf, &file_num);

    for (i = 0, k = 0; i < file_num; i++) {
        fname = fname_buf + i * 12;
        for (j = 0; j < 12; j++) {
            if (' ' == fname[j]) {
                fname[j] = '\0';
                break;
            }
        }
        fname_len = strlen(fname);
        ASCII_ToLower(fname, fname_len);
        if (0 == strncmp(fname, "watch", strlen("watch"))) {
            fname_len = rindex(watch_res[k], '.') - rindex(watch_res[k], '/') - 1;
            if (0 != strncmp(fname, rindex(watch_res[k], '/') + 1, fname_len)) {
                // 假如名字不一样，则往下找一样的名字，并进行交换
                for (index = k + 1; index < sizeof(watch_res) / sizeof(watch_res[0]); index++) {
                    fname_len = rindex(watch_res[index], '.') - rindex(watch_res[index], '/') - 1;
                    if (0 == strncmp(fname, rindex(watch_res[index], '/') + 1, fname_len)) {
                        fname = watch_res[index];
                        for (; index > k; index --) {
                            watch_res[index] = watch_res[index - 1];
                            watch_bgp_related[index] = watch_bgp_related[index - 1];
                            if (dial_sty.watch_style == (index - 1)) {
                                dial_sty.watch_style = index;
                            }
                        }
                        watch_res[k] = fname;
                        break;
                    }
                }
            }
            k++;
        }
    }

__watch_add_item_deal_end:
    if (fname_buf) {
        watch_free(fname_buf);
    }
    watch_mem_bgp_related();
}

int watch_add_item(char *s)
{
    char *new_item = NULL;
    /* char watch_name[16]; */
    u32 len;
    char *suffix = ".sty";


    char *root_path = RES_PATH;
    char watch[64];

    ASSERT(((strlen(s) + strlen(root_path) + 1) < sizeof(watch)), "err name3 %s\n", s);
    /* if ((strlen(s) + strlen(root_path) + 1) >= sizeof(watch)) { */
    /* printf("err name3 %s\n", s); */
    /* while (1); */
    /* } */

    ASCII_ToLower(s, strlen(s));
    strcpy(watch, root_path);
    strcpy(&watch[strlen(root_path)], &s[1]);
    s = watch;

    printf("watch_path %s\n", s);

    ASSERT(((watch_items + 1) < WATCH_ITEMS_LIMIT), "watch items overflow %d\n\n\n\n", watch_items);
    /* if ((watch_items + 1) >= WATCH_ITEMS_LIMIT) { */
    /* printf("watch items overflow %d\n\n\n\n", watch_items); */
    /* while (1); */
    /* } */
    len = strlen(s) - strlen(RES_PATH);
    /* printf("test len %d, %d, %d, %s, %s\n", strlen(s), strlen(RES_PATH), len, RES_PATH, s, s[len]); */

    /* strcpy(watch_name, &s[strlen(s) - len]); */
    /* printf("end str %s\n", watch_name); */

    new_item = watch_malloc(strlen(s) + 1 + len + strlen(suffix) + 1);
    if (new_item == NULL) {
        printf("watch add item fail\n");
        return -1;
    }
    strcpy(new_item, s);
    new_item[strlen(s)] = '/';
    strcpy(&new_item[strlen(s) + 1], &s[strlen(s) - len]);
    strcpy(&new_item[strlen(s) + 1 + len], suffix);
    new_item[strlen(s) + 1 + len + strlen(suffix)] = '\0';

    for (u8 i = 0; i < watch_items; i++) {
        if (0 == strcmp(new_item, watch_res[i])) {
            printf("repeat : %s, %s\n", new_item, watch_res[i]);
            watch_free(new_item);
            return 0;
        }
    }

    watch_res[watch_items] = new_item;
    watch_items++;

    watch_add_item_deal();

    printf("watch add item succ %d, %s\n", watch_items, new_item);

    return 0;
}

int watch_del_item(char *s)
{
    u32 i;
    char *item;
    u32 cur_items = watch_items;

    char *root_path = RES_PATH;
    char watch[64];
    char *cur_watch = watch_res[dial_sty.watch_style];

    ASSERT(((strlen(s) + strlen(root_path) + 1 + 1) < sizeof(watch)), "err name5 %s\n", s);
    /* if ((strlen(s) + strlen(root_path) + 1) >= sizeof(watch)) { */
    /* printf("err name5 %s\n", s); */
    /* while (1); */
    /* } */
    ASCII_ToLower(s, strlen(s));
    strcpy(watch, root_path);
    strcpy(&watch[strlen(root_path)], &s[1]);

    if (watch[strlen(watch) - 1] != '/') {
        strcat(&watch[strlen(root_path)], "/");//解决下面表盘匹配，watch 和 watch1半匹配撞车
    }
    s = watch;


    printf("watch_path %s\n", s);

    if (watch_items <= 2) {
        return -1;
    }

    for (i = 0; i < cur_items; i++) {
        if (strncmp(s, watch_res[i], strlen(s)) == 0) {
            item = watch_res[i];
            watch_res[i] = NULL;
            watch_free(item);
            watch_items--;

            watch_bgp_related[i] = NULL;
            break;
        }
    }

    for (; i < cur_items; i++) {
        if (watch_res[i + 1] != NULL) {
            watch_res[i] = watch_res[i + 1];

            watch_bgp_related[i] = watch_bgp_related[i + 1];
        } else {
            watch_res[i] = NULL;

            watch_bgp_related[i] = NULL;
            break;
        }
    }

    for (i = 0; i < watch_items; i++) {
        if (cur_watch == watch_res[i]) {
            printf("del set style %d, %s\n", i, watch_res[i]);
            watch_set_style(i);
            break;
        }
    }
    if (i == watch_items) {
        printf("end style\n");
        watch_set_style(0);
    }

    watch_mem_bgp_related();

    for (i = 0; i < watch_items; i++) {
        printf("cur watch item %d, %s\n", watch_items, watch_res[i]);
    }

    return 0;
}

int watch_select_wtrite_vm(void *priv)
{
#if UI_WATCH_RES_ENABLE
    int ret = 0;
    if ((int)priv == (int)SYSCFG_WRITE_ERASE_STATUS) {
        dial_sty.watch_style = 0;
        dial_sty.standby_watch_style = -1;
    }
    ret = syscfg_write(VM_WATCH_SELECT, &dial_sty, sizeof(struct __WATCH_STYLE));
    if (ret != sizeof(struct __WATCH_STYLE)) {
        printf("watch_select_wtrite_vm err\n");
        return -1;
    }
    return ret;
#endif
}

int watch_select_read_vm()
{
#if UI_WATCH_RES_ENABLE
    int ret = 0;
    ret = syscfg_read(VM_WATCH_SELECT, &dial_sty, sizeof(struct __WATCH_STYLE));
    if ((sizeof(struct __WATCH_STYLE) != ret) ||
        (dial_sty.watch_style >= watch_items) ||
        (dial_sty.standby_watch_style >= watch_items)) {
        printf("write watch_select_read_vm err\n");
        ret = -1;
    }
    return ret;
#endif
}

/* 说明 ：申请表盘以及表盘背景管理所需的内存, 内存不释放，内存的生命周期为整个ui的生命周期 */
int watch_set_init()
{
#if UI_WATCH_RES_ENABLE
    u32 i, j;
    u32 len;
    u32 file_num;
    char *fname_buf;
    char *suffix = ".sty";
    u8 root_path_len = strlen(RES_PATH);
    u8 suffix_len = strlen(suffix);
    u8 fname_len;
    char *fname;


    for (i = 0; i < WATCH_ITEMS_LIMIT; i++) {
        if (watch_res[i] != NULL) {
            watch_free(watch_res[i]);
            watch_res[i] = NULL;
        }
    }
    dial_sty.watch_style = 0;
    dial_sty.standby_watch_style = -1;
    watch_items = 0;


    for (i = 0; i < BGP_ITEMS_LIMIT; i++) {
        if (watch_bgp[i]) {
            watch_free(watch_bgp[i]);
            watch_bgp[i] = NULL;
        }
    }
    watch_bgp_items = 0;


    virfat_flash_get_dirinfo(NULL, &file_num);

    fname_buf = watch_malloc(file_num * 12);
    if (fname_buf == NULL) {
        printf("fname_buf br28_zalloc faile %d\n", file_num);
        return -1;
    }

    virfat_flash_get_dirinfo(fname_buf, &file_num);
    printf("file_num : %d\n", file_num);

    if (file_num >= BGP_ITEMS_LIMIT) {
        printf("file num overflow %d\n", file_num);
        watch_free(fname_buf);
        return -1;
    }

    for (i = 0; i < file_num; i++) {
        fname = &fname_buf[i * 12];
        for (j = 0; j < 12; j++) {
            if (fname[j] == ' ') {
                fname[j] = '\0';
                break;
            }
        }
        if (j == 12) {
            printf("\n\n\n\nfname overflow\n\n\n\n\n");
            watch_free(fname_buf);
            return -1;
        }
        fname_len = strlen(fname);
        ASCII_ToLower(fname, fname_len);

        if (strncmp(fname, "watch", strlen("watch")) == 0) {
            len = root_path_len + fname_len + 1 + fname_len + suffix_len;
            watch_res[watch_items] = watch_malloc(len + 1);
            if (watch_res[watch_items] == NULL) {
                printf("\n\nmalloc watch list err\n\n");
                watch_free(fname_buf);
                return -1;
            }
            strcpy(watch_res[watch_items], RES_PATH);
            strcpy(&watch_res[watch_items][root_path_len], fname);
            watch_res[watch_items][root_path_len + fname_len] = '/';
            strcpy(&watch_res[watch_items][root_path_len + fname_len + 1], fname);
            strcpy(&watch_res[watch_items][root_path_len + fname_len + 1 + fname_len], suffix);
            watch_res[watch_items][len] = '\0';

            printf("watch list : %s, %d, %d\n", watch_res[watch_items], watch_items, len);

            watch_items++;

        } else if (strncmp(fname, "bgp_w", strlen("bgp_w")) == 0) {
            watch_bgp_add(fname);
        }

    }


    watch_free(fname_buf);

    if (watch_bgp_related_init() != 0) {
        printf("bgp_related_init fail\n");
    } else {
        printf("bgp_related_init succ\n");
    }

    if (watch_select_read_vm() < 0) {
        dial_sty.watch_style = 0;
        dial_sty.standby_watch_style = -1;
    }

#endif

    return 0;
}


REGISTER_WATCH_SYSCFG(watch_select_ops) = {
    .name = "watch_select",
    .read = watch_select_read_vm,
    .write = watch_select_wtrite_vm,
};

static u8 watch_need_load = 0;

void watch_set_need_reload(u8 need)
{
    watch_need_load = !!need;
}




int switch_ui_page(u8 page)
{
    static u8 last_watch_style = 0xff;
    if (watch_need_load || last_watch_style != dial_sty.watch_style) {
        /* printf(">>>>>>>>>>>>>>>>>>%s %d %d %s\n",__FUNCTION__,__LINE__,dial_sty.watch_style,watch_res[dial_sty.watch_style]); */
        ui_set_sty_path_by_pj_id(1, NULL);
        ui_set_sty_path_by_pj_id(1, watch_res[dial_sty.watch_style]);
        if (!watch_res[dial_sty.watch_style] || !strlen(watch_res[dial_sty.watch_style])) {
            printf("watch res is null,watch style =%d\n", dial_sty.watch_style);
            /* ASSERT(0); */
        }
        last_watch_style = dial_sty.watch_style;
        watch_need_load = 0;
    }

#if UI_UPGRADE_RES_ENABLE//升级模式加载资源
    if (app_get_curr_task() == APP_WATCH_UPDATE_TASK ||
        app_get_curr_task() == APP_SMARTBOX_ACTION_TASK) {
        watch_set_need_reload(1);
    }
#endif

    return 0;
}

RESFILE *platform_get_file(int prj)
{
    return ui_load_sty_by_pj_id(prj);
}



static void *br28_load_window(int id)
{
    u8 *ui;
    int i;
    u32 *ptr;
    u16 *ptr_table;
    struct ui_file_head head ALIGNED(4);
    struct window_head window ALIGNED(4);
    int len = sizeof(struct ui_file_head);
    int retry;
    static const int rotate[] = {0, 90, 180, 270};


    if (!ui_file) {
        printf("ui_file : 0x%x\n", ui_file);
        return NULL;
    }
    ui_platform_ok();

    for (retry = 0; retry < set_retry_cnt(); retry++) {
        res_fseek(ui_file, 0, SEEK_SET);
        res_fread(ui_file, &head, len);

        if (id >= head.window_num) {
            return NULL;
        }

        res_fseek(ui_file, sizeof(struct window_head)*id, SEEK_CUR);
        res_fread(ui_file, &window, sizeof(struct window_head));

        u16 crc = CRC16(&window, (u32) & (((struct window_head *)0)->crc_data));
        if (crc == window.crc_head) {
            ui_rotate = rotate[head.rotate];
            ui_core_set_rotate(ui_rotate);
            switch (head.rotate) {
            case 1: /* 旋转90度 */
                ui_hori_mirror = true;
                ui_vert_mirror = false;
                break;
            case 3:/* 旋转270度 */
                ui_hori_mirror = false;
                ui_vert_mirror = true;
                break;
            default:
                ui_hori_mirror = false;
                ui_vert_mirror = false;
                break;
            }
            goto __read_data;
        }
    }

    return NULL;

__read_data:
    ui = (u8 *)br28_malloc(window.len);
    if (!ui) {
        return NULL;
    }
    for (retry = 0; retry < set_retry_cnt(); retry++) {
        res_fseek(ui_file, window.offset, SEEK_SET);
        res_fread(ui_file, ui, window.len);

        u16 crc = CRC16(ui, window.len);
        if (crc == window.crc_data) {
            goto __read_table;
        }
    }

    br28_free(ui);
    return NULL;

__read_table:
    ptr_table = (u16 *)br28_malloc(window.ptr_table_len);
    if (!ptr_table) {
        br28_free(ui);
        return NULL;
    }
    for (retry = 0; retry < set_retry_cnt(); retry++) {
        res_fseek(ui_file, window.ptr_table_offset, SEEK_SET);
        res_fread(ui_file, ptr_table, window.ptr_table_len);

        u16 crc = CRC16(ptr_table, window.ptr_table_len);
        if (crc == window.crc_table) {
            u16 *offset = ptr_table;
            for (i = 0; i < window.ptr_table_len; i += 2) {
                ptr = (u32 *)(ui + *offset++);
                if (*ptr != 0) {
                    *ptr += (u32)ui;
                }
            }
            br28_free(ptr_table);
            return ui;
        }
    }

    br28_free(ui);
    br28_free(ptr_table);

    return NULL;
}

static void br28_unload_window(void *ui)
{
    if (ui) {
        br28_free(ui);
    }
}

extern RESFILE *res_file;
extern RESFILE *str_file;

static int br28_load_style(struct ui_style *style)
{
    int err;
    int i, j;
    int len;
    struct vfscan *fs;
    char name[64];
    char style_name[16];
    static char cur_style = 0xff;


    if (!style->file && cur_style == 0) {
        return 0;
    }

    /* if (ui_file1) { */
    /* res_fclose(ui_file1); */
    /* ui_file1 = NULL; */
    /* } */

    if (style->file == NULL) {
        ASSERT(0);
        cur_style = 0;
        err = open_resource_file();
        if (err) {
            return -EINVAL;
        }
        ui_file1 = res_fopen(RES_PATH"JL/JL.sty", "r");
        if (!ui_file1) {
            return -ENOENT;
        }
        ui_file = ui_file1;
        ui_file_len = 0x7fffffff;//res_flen(ui_file1);
        len = 6;
        strcpy(style_name, "JL.sty");
        if (len) {
            style_name[len - 4] = 0;
            ui_core_set_style(style_name);
        }
    } else {
        cur_style = 1;
        ui_set_sty_path_by_pj_id(0, style->file);
        ui_file = ui_load_sty_by_pj_id(0);
        ASSERT(ui_file);

        ui_file_len = 0x7fffffff;//res_flen(ui_file1);
        for (i = 0; style->file[i] != '.'; i++) {
            name[i] = style->file[i];
        }
        printf("open resfile 0 :%s\n", name);
        name[i++] = '.';
        name[i++] = 'r';
        name[i++] = 'e';
        name[i++] = 's';
        name[i] = '\0';
        printf("open resfile %s\n", name);
        res_file = ui_load_res_by_pj_id(0);

        ASSERT(res_file);
        name[--i] = 'r';
        name[--i] = 't';
        name[--i] = 's';
        printf("open strfile %s\n", name);
        str_file = ui_load_str_by_pj_id(0);
        ASSERT(str_file);

        name[i++] = 'a';
        name[i++] = 's';
        name[i++] = 'i';
        printf("open asciifile %s\n", name);
        font_ascii_init(RES_PATH"font/ascii.res");

        for (i = strlen(style->file) - 5; i >= 0; i--) {
            if (style->file[i] == '/') {
                break;
            }
        }

        for (i++, j = 0; style->file[i] != '\0'; i++) {
            if (style->file[i] == '.') {
                name[j] = '\0';
                break;
            }
            name[j++] = style->file[i];
        }
        ASCII_ToUpper(name, j);
        if (!strncmp(name, "WATCH", strlen("WATCH"))) {
            strcpy(name, "JL");
        }
        if (!strncmp(name, "UPGRADE", strlen("UPGRADE"))) {
            strcpy(name, "JL");
        }

        err = ui_core_set_style(name);
        if (err) {
            printf("style_err: %s\n", name);
        }
    }

    return 0;

__err2:
    ASSERT(0);
    close_resfile();
__err1:
    res_fclose(ui_file1);
    ui_file1 = NULL;

    return err;
}


static int dc_index = 0;;
static u8 dc_flag = 0;
static int br28_open_draw_context(struct draw_context *dc)
{
    int i;
    for (i = 0; i < 2; i++) {
        if (!(dc_flag & BIT(i))) {
            dc->index = i;
            dc_flag |= BIT(i);
            break;
        }
    }
    ASSERT(i != 2);

    ASSERT(imb_task_head_check(imb_task_head_get(dc->index)));

    dc_index++;
    ASSERT(dc_index < 3);

    dc->buf_num = 1;
    struct imb_task_info imb_tsk_info = {0};
    /* printf("br28_open_draw_context 0x%08x %d:%d\n", dc->elm->parent->id, dc->elm->parent->id >> 29, (dc->elm->parent->id >> 22) & 0x7f); */

    if (__this->lcd->get_screen_info) {
        __this->lcd->get_screen_info(&__this->info);
    }

    // 申请缓存buffer并交给imd初始化管理
    if (__this->lcd->buffer_malloc) {
        u8 *buf = NULL;
        u32 len;

        __this->lcd->buffer_malloc(&buf, &len);
        dc->buf = buf;
        dc->len = len;
        __this->lcd->init_buffer(dc->index, buf, len / __this->info.buf_num);

        dc->buf0 = NULL;
        dc->buf1 = NULL;
    }

    dc->width = __this->info.width;
    dc->height = __this->info.height;
    dc->col_align = __this->info.col_align;
    dc->row_align = __this->info.row_align;
    dc->colums = dc->width;
    dc->lines = dc->len / __this->info.buf_num / __this->info.stride;

    imb_task_head_set_buf(imb_task_head_get(dc->index), dc->buf, dc->len, dc->width, dc->height, __this->info.stride, dc->lines, __this->info.buf_num);

    imb_tsk_info.data_src       = DATA_SRC_NONE;
    imb_tsk_info.zip_en         = 0;
    imb_tsk_info.in_format      = LAYER_FORMAT_SOLID;
    imb_tsk_info.out_format     = __this->info.color_format;
    imb_tsk_info.x_offset       = 0;
    imb_tsk_info.y_offset       = 0;
    imb_tsk_info.src_w          = dc->width;
    imb_tsk_info.src_h          = dc->height;
    imb_tsk_info.des_w          = dc->width;
    imb_tsk_info.des_h          = dc->height;
    imb_tsk_info.priv           = 0;
    /* printf("info.src_w : %d\n", info.src_w); */
    /* printf("info.src_h : %d\n", info.src_h); */
    /* printf("info.in_format : %d\n", info.in_format); */
    /* printf("dc->index : %d\n", dc->index); */
    imb_task_list_init(imb_task_head_get(dc->index), &imb_tsk_info);

    dc->draw_state = 0;

    return 0;
}








static int br28_get_draw_context(struct draw_context *dc)
{
//non-meld
#if 0
    // 获取buffer并设置给imb写数据
    u8 *draw_buf = NULL;
    while (!draw_buf) {
        // 如果所有buffer都在忙，则阻塞等待拿到可用buffer再继续
        draw_buf = __this->lcd->get_buffer(dc->buf);
    }
    dc->buf = draw_buf;
    set_imb_output_buffer(dc->buf);

    dc->disp.left  = dc->need_draw.left;
    dc->disp.width = dc->need_draw.width;
    dc->disp.top   = dc->need_draw.top;
    dc->disp.height = dc->need_draw.height;
#else
    dc->disp.left  = 0;//dc->need_draw.left;
    dc->disp.width = dc->width;//dc->need_draw.width;
    dc->disp.top   = 0;//dc->need_draw.top;
    dc->disp.height = dc->height; //dc->need_draw.height;
#endif

    return 0;
}


static int br28_put_draw_context(struct draw_context *dc)
{
    struct rect disp;

    if (!dc->refresh) {
        return -1;
    }

    // imb写buffer，并get出来送imd显示
    imb_task_head_config(imb_task_head_get(dc->index), &dc->rect_orig, &dc->rect, &dc->r2, dc->draw_state);
    imb_start(imb_task_head_get(dc->index), ui_core_get_screen_draw_rect(), disp, dc->colums, dc->lines);


    return 0;
}

void lcd_draw_test(int left, int top, int width, int height)
{
    struct draw_context dc;
    dc.disp.left = left;
    dc.disp.top  = top;
    dc.disp.width = width;
    dc.disp.height = height;

    dc.need_draw.left = left;
    dc.need_draw.top  = top;
    dc.need_draw.width = width;
    dc.need_draw.height = height;

    dc.data_format = DC_DATA_FORMAT_OSD16;
    /* br28_put_draw_context(&dc); */
    dc.buf = get_imb_output_buf(NULL);

    y_printf("dc.disp l:%d w:%d t:%d h:%d", dc.disp.left, dc.disp.width, dc.disp.top, dc.disp.height);
    if (__this->lcd->set_draw_area) {
        __this->lcd->set_draw_area(dc.disp.left, dc.disp.left + dc.disp.width - 1,
                                   dc.disp.top, dc.disp.top + dc.disp.height - 1);
    }

    u8 wait = ((dc.need_draw.top + dc.need_draw.height) == (dc.disp.top + dc.disp.height)) ? 1 : 0;
    if (__this->lcd->draw) {
        if (dc.data_format == DC_DATA_FORMAT_OSD16) {
            __this->lcd->draw(dc.buf, dc.disp.height * dc.disp.width * 2, wait);
            y_printf("dc.need_draw l:%d w:%d t:%d h:%d", dc.need_draw.left, dc.need_draw.width, dc.need_draw.top, dc.need_draw.height);
        } else if (dc.data_format == DC_DATA_FORMAT_MONO) {
            __this->lcd->draw(dc.buf, __this->info.width * __this->info.height / 8, wait);
        }
    }
}
static int br28_set_draw_context(struct draw_context *dc)
{
    return 0;
}

static int br28_close_draw_context(struct draw_context *dc)
{
    dc_index--;

    ASSERT(dc, ", dc : 0x%x\n", dc);
    struct rect *orig = &dc->rect_orig;
    /* printf("br28_close_draw_context 0x%08x %d:%d [%d, %d, %d, %d]\n", dc->elm->parent->id, dc->elm->parent->id >> 29, (dc->elm->parent->id >> 22) & 0x7f, orig->left, orig->top, orig->width, orig->height); */
    imb_task_all_destroy(imb_task_head_get(dc->index));

    if (__this->lcd->buffer_free) {
        extern bool is_imd_buf_using(u8 index, u8 * lcd_buf);
        while (is_imd_buf_using(dc->index, dc->buf)) {
            os_time_dly(1);
        }
        __this->lcd->release_buffer(dc->index);
        __this->lcd->buffer_free(dc->buf);
    }

    if (dc->fbuf) {
        br28_free(dc->fbuf);
        dc->fbuf = NULL;
        dc->fbuf_len = 0;
    }

    dc_flag &= ~BIT(dc->index);

    return 0;
}


static int br28_invert_rect(struct draw_context *dc, u32 acolor)
{
    int i;
    int len;
    int w, h;
    int color = acolor & 0xffff;

    if (dc->data_format == DC_DATA_FORMAT_MONO) {
        color |= BIT(31);
        for (h = 0; h < dc->draw.height; h++) {
            for (w = 0; w < dc->draw.width; w++) {
                if (platform_api->draw_point) {
                    platform_api->draw_point(dc, dc->draw.left + w, dc->draw.top + h, color);
                }
            }
        }
    }
    return 0;
}

static int br28_fill_rect(struct draw_context *dc, u32 acolor)
{
    struct imb_task_info imb_tsk_info = {0};
    u16 id = CRC16(&acolor, 4);
    struct imb_task *p = NULL;
    u8 elm_index = 0;

    p = imb_task_search_by_id(imb_task_head_get(dc->index), dc->elm->id, elm_index);

    imb_tsk_info.rect.left = dc->rect.left - dc->rect_orig.left;
    imb_tsk_info.rect.top = dc->rect.top - dc->rect_orig.top;
    imb_tsk_info.rect.width = dc->rect.width;
    imb_tsk_info.rect.height = dc->rect.height;

    struct rect draw;
    draw.left = dc->draw.left - dc->rect_orig.left;
    draw.top = dc->draw.top - dc->rect_orig.top;
    draw.width = dc->draw.width;
    draw.height = dc->draw.height;

    if (!get_rect_cover(&imb_tsk_info.rect, &draw, &imb_tsk_info.draw)) {
        if (p) {
            imb_task_enable(p, false);
        }
        return -EFAULT;
    }
    memcpy(&imb_tsk_info.crop, &imb_tsk_info.draw, sizeof(struct rect));

    imb_tsk_info.elm_id         = dc->elm->id;
    imb_tsk_info.id             = (dc->page << 26) | (2 << 24) | (id << 8) | elm_index;
    imb_tsk_info.data_src       = DATA_SRC_NONE;
    imb_tsk_info.zip_en         = 0;
    imb_tsk_info.in_format      = LAYER_FORMAT_SOLID;
    imb_tsk_info.x_offset       = dc->rect.left - dc->rect_orig.left;
    imb_tsk_info.y_offset       = dc->rect.top - dc->rect_orig.top;
    imb_tsk_info.src_w          = dc->rect.width;
    imb_tsk_info.src_h          = dc->rect.height;
    imb_tsk_info.des_w          = dc->rect.width;
    imb_tsk_info.des_h          = dc->rect.height;
    imb_tsk_info.priv           = acolor;

    /* printf("[%s] %d, %d, %d, %d\n", __FUNCTION__, dc->rect_orig.left, dc->rect_orig.top, dc->rect_orig.width, dc->rect_orig.height); */
    /* printf("[%s] %d, %d, %d, %d, %d, %d\n", __FUNCTION__, info.x_offset, info.y_offset, info.src_w, info.src_h, info.des_w, info.des_h); */

    if (p) {
        imb_task_reset(imb_task_head_get(dc->index), p, &imb_tsk_info);
    } else {
        imb_task_list_add(imb_task_head_get(dc->index), &imb_tsk_info);
    }

    return 0;
}



__attribute__((always_inline))
static inline void __draw_vertical_line(struct draw_context *dc, int x, int y, int width, int height, int color, int format)
{
    int i, j;
    struct rect r = {0};
    struct rect disp = {0};

    disp.left  = x;
    disp.top   = y;
    disp.width = width;
    disp.height = height;
    if (!get_rect_cover(&dc->draw, &disp, &r)) {
        return;
    }

    switch (format) {
    case DC_DATA_FORMAT_OSD16:
        for (i = 0; i < r.width; i++) {
            for (j = 0; j < r.height; j++) {
                if (platform_api->draw_point) {
                    platform_api->draw_point(dc, r.left + i, r.top + j, color);
                }
            }
        }
        break;
    case DC_DATA_FORMAT_MONO:
        for (i = 0; i < r.width; i++) {
            for (j = 0; j < r.height; j++) {
                if (platform_api->draw_point) {
                    platform_api->draw_point(dc, r.left + i, r.top + j, color);
                }
            }
        }
        break;

    }
}


__attribute__((always_inline))
static inline void __draw_line(struct draw_context *dc, int x, int y, int width, int height, int color, int format)
{
    int i, j;
    struct rect r = {0};
    struct rect disp = {0};

    disp.left  = x;
    disp.top   = y;
    disp.width = width;
    disp.height = height;
    if (!get_rect_cover(&dc->draw, &disp, &r)) {
        return;
    }

    switch (format) {
    case DC_DATA_FORMAT_OSD16:
        for (i = 0; i < r.height; i++) {
            for (j = 0; j < r.width; j++) {
                if (platform_api->draw_point) {
                    platform_api->draw_point(dc, r.left + j, r.top + i, color);
                }
            }
        }
        break;
    case DC_DATA_FORMAT_MONO:
        for (i = 0; i < r.height; i++) {
            for (j = 0; j < r.width; j++) {
                if (platform_api->draw_point) {
                    platform_api->draw_point(dc, r.left + j, r.top + i, color);
                }
            }
        }
        break;
    }
}

static int br28_draw_rect(struct draw_context *dc, struct css_border *border)
{
    int err;
    int offset;
    int color = border->color & 0xffff;

    /* draw_rect_range_check(&dc->draw, map); */
    /* draw_rect_range_check(&dc->rect, map); */

    if (dc->data_format == DC_DATA_FORMAT_OSD16) {
        color = border->color & 0xffff;
    } else if (dc->data_format == DC_DATA_FORMAT_MONO) {
        color = (color != UI_RGB565(RECT_MONO_CLR)) ? (color ? color : 0xffff) : 0x55aa;
    }

    if (border->left) {
        if (dc->rect.left >= dc->draw.left &&
            dc->rect.left <= rect_right(&dc->draw)) {
            __draw_vertical_line(dc, dc->draw.left, dc->draw.top,
                                 border->left, dc->draw.height, color, dc->data_format);
        }
    }
    if (border->right) {
        if (rect_right(&dc->rect) >= dc->draw.left &&
            rect_right(&dc->rect) <= rect_right(&dc->draw)) {
            __draw_vertical_line(dc, dc->draw.left + dc->draw.width - border->right, dc->draw.top,
                                 border->right, dc->draw.height, color, dc->data_format);
        }
    }
    if (border->top) {
        if (dc->rect.top >= dc->draw.top &&
            dc->rect.top <= rect_bottom(&dc->draw)) {
            __draw_line(dc, dc->draw.left, dc->draw.top,
                        dc->draw.width, border->top, color, dc->data_format);
        }
    }
    if (border->bottom) {
        if (rect_bottom(&dc->rect) >= dc->draw.top &&
            rect_bottom(&dc->rect) <= rect_bottom(&dc->draw)) {
            __draw_line(dc, dc->draw.left, dc->draw.top + dc->draw.height - border->bottom,
                        dc->draw.width, border->bottom, color, dc->data_format);
        }
    }

    return 0;
}

__attribute__((always_inline_when_const_args))
AT_UI_RAM
static u16 get_mixed_pixel(u16 backcolor, u16 forecolor, u8 alpha)
{
    u16 mixed_color;
    u8 r0, g0, b0;
    u8 r1, g1, b1;
    u8 r2, g2, b2;

    if (alpha == 255) {
        return (forecolor >> 8) | (forecolor & 0xff) << 8;
    } else if (alpha == 0) {
        return (backcolor >> 8) | (backcolor & 0xff) << 8;
    }

    r0 = ((backcolor >> 11) & 0x1f) << 3;
    g0 = ((backcolor >> 5) & 0x3f) << 2;
    b0 = ((backcolor >> 0) & 0x1f) << 3;

    r1 = ((forecolor >> 11) & 0x1f) << 3;
    g1 = ((forecolor >> 5) & 0x3f) << 2;
    b1 = ((forecolor >> 0) & 0x1f) << 3;

    r2 = (alpha * r1 + (255 - alpha) * r0) / 255;
    g2 = (alpha * g1 + (255 - alpha) * g0) / 255;
    b2 = (alpha * b1 + (255 - alpha) * b0) / 255;

    mixed_color = ((r2 >> 3) << 11) | ((g2 >> 2) << 5) | (b2 >> 3);

    return (mixed_color >> 8) | (mixed_color & 0xff) << 8;
}

static int br28_read_image_info(struct draw_context *dc, u32 id, u8 page, struct ui_image_attrs *attrs)
{
    struct image_file file = {0};

    if (((u16) - 1 == id) || ((u32) - 1 == id)) {
        return -1;
    }

    int err = open_image_by_id(dc->prj, NULL, &file, id, dc->page);
    if (err) {
        return -EFAULT;
    }
    attrs->width = file.width;
    attrs->height = file.height;

    return 0;
}

AT_UI_RAM
int line_update(u8 *mask, u16 y, u16 width)
{
    int i;
    if (!mask) {
        return true;
    }
    for (i = 0; i < (width + 7) / 8; i++) {
        if (mask[y * ((width + 7) / 8) + i]) {
            return true;
        }
    }
    return false;
}

extern void imb_task_lock();
extern void imb_task_unlock();
extern void jpeg_dec_start(struct rect *root_draw, struct rect *draw, void *dec_out_buf);
static void *imb_jpeg_cb(void *priv)
{
    struct imb_task_out *out = (struct imb_task_out *)priv;
    imb_task_lock();
    jpeg_dec_start(&out->root_rect, &out->rect, out->outbuf);
    imb_task_unlock();
    return NULL;
}

static int br28_draw_image(struct draw_context *dc, u32 src, u8 quadrant, u8 *mask)
{
    struct imb_task_info imb_tsk_info = {0};
    struct image_file file = {0};
    RESFILE *fp;
    int id;
    int page;
    struct flash_file_info *file_info;

    if (dc->elm->css.invisible) {
        printf("image invisible\n");
        return -1;
    }

    if (dc->preview.file) {
        fp = dc->preview.file;
        id = dc->preview.id;
        page = dc->preview.page;
        file_info = dc->preview.file_info;
    } else {
        fp = NULL;
        id = src;
        page = dc->page;
        file_info = ui_get_image_file_info_by_pj_id(dc->prj);
    }

    if (((u16) - 1 == id) || ((u32) - 1 == id)) {
        return -1;
    }

    if (dc->draw_img.en) {
        id = dc->draw_img.id;
        page = dc->draw_img.page;
    }

    /* printf("fp:0x%x, id:%d, page:%d\n", fp, id, page); */
    int err = open_image_by_id(dc->prj, fp, &file, id, page);
    if (err) {
        printf("%s, read file err\n", __func__);
        return -EFAULT;
    }

    int x = dc->rect.left;
    int y = dc->rect.top;

    /* y_printf("x:%d y:%d", x, y); */
    if (dc->align == UI_ALIGN_CENTER) {
        x += (dc->rect.width / 2 - file.width / 2);
        y += (dc->rect.height / 2 - file.height / 2);
    } else if (dc->align == UI_ALIGN_RIGHT) {
        x += dc->rect.width - file.width;
    }

    if (dc->crop && !get_rect_cover(&dc->rect, &dc->draw, &imb_tsk_info.crop)) {
        memset(&imb_tsk_info.crop, 0x00, sizeof(struct rect));
    }


    u8 elm_index = 1;
    struct imb_task *p = NULL;
    p = imb_task_search_by_id(imb_task_head_get(dc->index), dc->elm->id, elm_index);

    if (dc->elm->css.rotate.en) {
        int max = dc->elm->css.rotate.cent_x > dc->elm->css.rotate.cent_y ? dc->elm->css.rotate.cent_x : dc->elm->css.rotate.cent_y;
        imb_tsk_info.rect.left = dc->elm->css.rotate.dx - max;
        imb_tsk_info.rect.top = dc->elm->css.rotate.dy - max;
        imb_tsk_info.rect.width = max * 2;
        imb_tsk_info.rect.height = max * 2;
    } else {
        imb_tsk_info.rect.left = x - dc->rect_orig.left;
        imb_tsk_info.rect.top = y - dc->rect_orig.top;
        imb_tsk_info.rect.width = file.width;
        imb_tsk_info.rect.height = file.height;
    }

    struct rect draw;
    draw.left = dc->draw.left - dc->rect_orig.left;
    draw.top = dc->draw.top - dc->rect_orig.top;
    draw.width = dc->draw.width;
    draw.height = dc->draw.height;

    if (!get_rect_cover(&imb_tsk_info.rect, &draw, &imb_tsk_info.draw)) {
        if (p) {
            imb_task_enable(p, false);
        }
        return -EFAULT;
    }

    ASSERT((dc->elm->css.rotate.en == 0) || (dc->elm->css.ratio.en == 0), "rotate and ratio cannot enable at the same time");

    imb_tsk_info.rotate_en      = dc->elm->css.rotate.en;
    if (imb_tsk_info.rotate_en) {
        imb_tsk_info.x_src_offset   = 0;
        imb_tsk_info.y_src_offset   = 0;
        imb_tsk_info.rotate_c_x     = dc->elm->css.rotate.cent_x;
        imb_tsk_info.rotate_c_y     = dc->elm->css.rotate.cent_y;
        imb_tsk_info.rotate_angle   = dc->elm->css.rotate.angle;
        imb_tsk_info.rotate_dx      = dc->elm->css.rotate.dx;
        imb_tsk_info.rotate_dy      = dc->elm->css.rotate.dy;
        /* imb_tsk_info.rotate_chunk   = dc->elm->css.rotate.chunk; */
        if (dc->elm->css.rotate.chunk) {
            imb_tsk_info.rotate_chunk_offset = 0;
            imb_tsk_info.rotate_chunk_num = 8;
            imb_tsk_info.rotate_chunk = (file.height / imb_tsk_info.rotate_chunk_num + 9) / 10 * 10;
        } else {
            imb_tsk_info.rotate_chunk = 0;
        }
        /* printf("info.rotate_angle : %d\n", imb_tsk_info.rotate_angle); */
        /* printf("%d, %d, %d, %d, %d\n", imb_tsk_info.rotate_c_x, imb_tsk_info.rotate_c_y, imb_tsk_info.rotate_angle, imb_tsk_info.rotate_dx, imb_tsk_info.rotate_dy); */
    }

    imb_tsk_info.scale_en       = dc->elm->css.ratio.en;
    if (imb_tsk_info.scale_en) {
        imb_tsk_info.ratio_w = dc->elm->css.ratio.ratio_w;
        imb_tsk_info.ratio_h = dc->elm->css.ratio.ratio_h;
        if (imb_tsk_info.ratio_w == 0) {
            imb_tsk_info.ratio_w = 1;
        }
        if (imb_tsk_info.ratio_h == 0) {
            imb_tsk_info.ratio_h = 1;
        }
        x = dc->rect.left;
        y = dc->rect.top;
        if (dc->align == UI_ALIGN_CENTER) {
            x += (dc->rect.width / 2 - (file.width * imb_tsk_info.ratio_w) / 2);
            y += (dc->rect.height / 2 - (file.height * imb_tsk_info.ratio_h) / 2);
        } else if (dc->align == UI_ALIGN_RIGHT) {
            x += dc->rect.width - file.width;
        }
    }



    imb_tsk_info.elm_id = dc->elm->id;
    imb_tsk_info.id = (dc->page << 26) | (0 << 24) | (id << 8) | elm_index;
    if (imb_tsk_info.rotate_en) {
        imb_tsk_info.data_src       = DATA_SRC_SRAM;
        imb_tsk_info.cur_in_flash   = 1;
        imb_tsk_info.zip_en         = 0;
    } else {
        imb_tsk_info.data_src       = DATA_SRC_FLASH;
        imb_tsk_info.cur_in_flash   = 1;
        if (file.format == PIXEL_FMT_L1) {
            imb_tsk_info.zip_en     = 0;
        } else {
            imb_tsk_info.zip_en     = 1;
        }
    }

    imb_tsk_info.in_format      = file.format;
    imb_tsk_info.x_offset       = x - dc->rect_orig.left;
    imb_tsk_info.y_offset       = y - dc->rect_orig.top;
    imb_tsk_info.src_w          = file.width;
    imb_tsk_info.src_h          = file.height;
    imb_tsk_info.des_w          = file.width;
    imb_tsk_info.des_h          = file.height;
    imb_tsk_info.ff_info        = file_info;

    if (imb_tsk_info.in_format == PIXEL_FMT_JPEG) {
        y_printf("jpeg....");
        imb_tsk_info.data_src       = DATA_SRC_SRAM;
        imb_tsk_info.cur_in_flash   = 1;
        imb_tsk_info.zip_en         = 0;
        imb_tsk_info.lcd_width      = dc->width;
        imb_tsk_info.lcd_height     = dc->height;
        imb_tsk_info.lcd_format     = __this->info.color_format;
        imb_tsk_info.cb             = imb_jpeg_cb;
        imb_tsk_info.quote          = 1;
        /* p = imb_task_search(imb_task_head_get(dc->index), "task00"); */
    } else if (imb_tsk_info.in_format == PIXEL_FMT_L1) {
        imb_tsk_info.l1_type        = L1_IMAGE;
    }

    if ((imb_tsk_info.data_src == DATA_SRC_SRAM) || (imb_tsk_info.data_src == DATA_SRC_PSRAM)) {
        /* g_printf("%s %d", __FUNCTION__, __LINE__); */
        if ((p) && (!strcmp(p->name, "task00"))) {
            if (p->id != imb_tsk_info.id) {
                imb_task_file_info_release(p);
                imb_task_lut_tab_release(p);
                imb_tsk_info.priv       = (u32)&file;
            }
        } else if ((p) && (p->id == imb_tsk_info.id)) {
            imb_tsk_info.priv = p->dat_src_adr;
            imb_tsk_info.cur_in_flash   = 0;
        } else {
            if (p) {
                imb_task_src_adr_release(p);
                imb_task_lut_tab_release(p);
            }
            imb_tsk_info.priv       = (u32)&file;
        }
    } else if (imb_tsk_info.data_src == DATA_SRC_FLASH) {
        /* g_printf("%s %d", __FUNCTION__, __LINE__); */
        if ((p) && (p->id != imb_tsk_info.id)) {
            imb_task_file_info_release(p);
            imb_task_lut_tab_release(p);
        }
        imb_tsk_info.priv       = (u32)&file;
    }

    if (p) {
        imb_task_reset(imb_task_head_get(dc->index), p, &imb_tsk_info);
    } else {
        imb_task_list_add(imb_task_head_get(dc->index), &imb_tsk_info);
    }

    return 0;
}

void ui_draw_cb(u8 *dst_buf, struct rect *dst_r, u8 *src_buf, struct rect *src_r, u8 bytes_per_pixel)
{
    int w, h;
    struct rect r;
    int dst_stride = (dst_r->width * bytes_per_pixel + 3) / 4 * 4;
    int src_stride = (src_r->width * bytes_per_pixel + 3) / 4 * 4;

    if (get_rect_cover(dst_r, src_r, &r)) {
        for (h = 0; h < r.height; h++) {
            memcpy(&dst_buf[(r.top + h - dst_r->top) * dst_stride + (r.left - dst_r->left) * bytes_per_pixel],
                   &src_buf[(r.top + h - src_r->top)*src_stride + (r.left - src_r->left) * bytes_per_pixel],
                   r.width * bytes_per_pixel);
        }
    }
}


extern void imb_task_calc_crc(struct imb_task *task);
extern int imb_task_check_crc(struct imb_task *task);
static void *imb_draw_cb(void *priv)
{
    struct imb_task_out *out = (struct imb_task_out *)priv;
    struct imb_task *task = out->task;

    ASSERT(imb_task_check_crc(task));

    if (out->format == OUTPUT_FORMAT_RGB888) {
        task->cur_format = LAYER_FORMAT_RGB888;
    } else if (out->format == OUTPUT_FORMAT_RGB565) {
        task->cur_format = LAYER_FORMAT_RGB565;
    } else {
        ASSERT(0, "curr format is not support!\n");
    }
    task->x_offset = 0;
    task->y_offset = 0;
    task->src_w = out->rect.width;
    task->src_h = out->rect.height;
    task->des_w = out->rect.width;
    task->des_h = out->rect.height;
    task->dat_src_adr = (u32)out->outbuf;

    imb_task_calc_crc(task);

    if (task->user_cb) {
        void (*cb)(int id, u8 * dst_buf, struct rect * dst_r, struct rect * src_r, u8 bytes_per_pixel, void *priv) = out->priv;
        cb(task->cb_id, out->outbuf, &out->rect, &task->rect, (out->format == OUTPUT_FORMAT_RGB888) ? 3 : 2, task->priv_buf);//不带alpha的显示
    } else {
        ui_draw_cb(out->outbuf, &out->rect, out->priv, &task->rect, (out->format == OUTPUT_FORMAT_RGB888) ? 3 : 2);//不带alpha的显示
    }

    return NULL;
}


int ui_draw(struct draw_context *dc, u8 *buf, int x, int y, int width, int height, void *cb, void *priv, int priv_len, int id)
{
    struct imb_task_info imb_tsk_info = {0};
    struct imb_task *p = NULL;
    struct element *elm = dc->elm;
    struct rect rect;
    u8 elm_index = dc->elm_index++;

    p = imb_task_search_by_id(imb_task_head_get(dc->index), elm->id, elm_index);

    imb_tsk_info.elm_id         = elm->id;
    imb_tsk_info.id             = (dc->page << 26) | (0 << 24) | (0 << 8) | elm_index;
    imb_tsk_info.data_src       = DATA_SRC_SRAM;
    imb_tsk_info.cur_in_flash   = 0;
    imb_tsk_info.zip_en         = 0;
    imb_tsk_info.in_format      = 0;
    imb_tsk_info.x_offset       = x;
    imb_tsk_info.y_offset       = y;
    imb_tsk_info.src_w          = width;
    imb_tsk_info.src_h          = height;
    imb_tsk_info.des_w          = width;
    imb_tsk_info.des_h          = height;
    imb_tsk_info.ff_info        = NULL;
    imb_tsk_info.cb             = imb_draw_cb;
    imb_tsk_info.cb_priv        = cb ? cb : buf;
    imb_tsk_info.quote          = 1;
    imb_tsk_info.user_cb        = cb ? 1 : 0;
    imb_tsk_info.cb_id          = id;

    if (p) {
        imb_task_reset(imb_task_head_get(dc->index), p, &imb_tsk_info);
        if (priv && priv_len) {
            ASSERT(imb_task_check_crc(p));
            memcpy(p->priv_buf, priv, priv_len);
            imb_task_calc_crc(p);
        }
    } else {
        p = imb_task_list_add(imb_task_head_get(dc->index), &imb_tsk_info);
        if (priv && priv_len) {
            ASSERT(imb_task_check_crc(p));
            memcpy(p->priv_buf, priv, priv_len);
            imb_task_calc_crc(p);
        }
    }
    return 0;
}

int get_multi_string_width(struct draw_context *dc, u8 *str, int *str_width, int *str_height)
{
    struct image_file file = {0};
    u16 *p = (u16 *)str;
    int width = 0;
    while (*p != 0) {
        if (open_string_pic(dc->prj, &file, *p)) {
            return -EINVAL;
        }
        width += file.width;
        p++;
    }
    *str_width = width;
    *str_height = file.height;

    return 0;
}

struct font_info *text_font_init(u8 init)
{
    static struct font_info *info = NULL;
    static int language = 0;

    if (init) {
        if (!info || (language != ui_language_get())) {
            language = ui_language_get();
            if (info) {
                font_close(info);
            }
            info = font_open(NULL, language);
            ASSERT(info, "font_open fail!");
        }
    } else {
        if (info) {
            font_close(info);
            info = NULL;
        }
    }

    return info;
}

u16 ui_get_text_width_and_height(u8 encode, u8 *str, u16 strlen, u16 elm_width, u16 elm_height, u8 flags, char *value_type)
{
    int ret;
    int width = 0;
    int height = 0;
    struct font_info *info = text_font_init(true);
    if (info) {
        info->text_width  = elm_width;
        info->text_height = elm_height;
        info->flags       = flags;
        if (encode == FONT_ENCODE_ANSI) {
            width = font_text_width(info, str, strlen);
        } else if (encode == FONT_ENCODE_UNICODE) {
            width = font_textw_width(info, str, strlen);
        } else {
            width = font_textu_width(info, str, strlen);
        }
        height = info->string_height;
        text_font_init(false);
    }

    if (!strcmp(value_type, "width")) {
        ret = width;
    } else if (!strcmp(value_type, "height")) {
        ret = height;
    } else {
        ret = 0;
    }

    return ret;
}


int br28_show_element(struct draw_context *dc)
{
    imb_task_enable_by_id(imb_task_head_get(dc->index), dc->elm->id, true);

    return 0;
}

int br28_hide_element(struct draw_context *dc)
{
    imb_task_enable_by_id(imb_task_head_get(dc->index), dc->elm->id, false);

    return 0;
}

int br28_delete_element(struct draw_context *dc)
{
    imb_task_delete_by_elm(imb_task_head_get(dc->index), dc->elm->id);

    return 0;
}

static int br28_show_text(struct draw_context *dc, struct ui_text_attrs *text)
{
    int offset = 0;
    struct rect draw_r;
    int elm_task_num;
    /* 控件从绝对x,y 转成相对图层的x,y */
    int x = dc->rect.left;
    int y = dc->rect.top;
    struct image_file file;
    struct imb_task_info imb_tsk_info = {0};
    struct imb_task *p = NULL;
    u8 *pixbuf;


    if (dc->elm->css.invisible) {
        printf("text invisible\n");
        return -1;
    }
    /* struct element *elm = container_of(text,struct ui_text,attrs); */
    /*  */

    /* 绘制区域从绝对x,y 转成相对图层的x,y */
    draw_r.left   = dc->draw.left;
    draw_r.top    = dc->draw.top;
    draw_r.width  = dc->draw.width;
    draw_r.height = dc->draw.height;
    /* font_set_offset_table(lange_offset_table); */

    if (text->format && !strcmp(text->format, "text")) {
        if (!text->str || !text->strlen) {
            return -EFAULT;
        }

        struct font_info *info = text_font_init(true);
        if (info && (FT_ERROR_NONE == (info->sta & (~(FT_ERROR_NOTABFILE | FT_ERROR_NOPIXFILE))))) {
            draw_r.left -= dc->rect_orig.left;
            draw_r.top -= dc->rect_orig.top;
            info->disp.map    = 0;
            info->disp.rect   = &draw_r;
            info->disp.format = dc->data_format;
            info->disp.color  = text->color;

            info->dc = dc;

            info->text_width  = dc->rect.width;//draw_r.width;
            info->text_height = dc->rect.height;//draw_r.height;
            info->flags       = text->flags;
            /* info->offset      = text->offset; */
            int roll = 0;//需要滚动
            int multi_line = 0;
            /* FONT_SHOW_MULTI_LINE */
            if (text->encode == FONT_ENCODE_ANSI) {
                int width = font_text_width(info, (u8 *)text->str, text->strlen);
                int height = info->string_height;

                if (height == 0) {
                    if (info->ascpixel.size) {
                        height = info->ascpixel.size;
                    } else if (info->pixel.size) {
                        height = info->pixel.size;
                    } else {
                        ASSERT(0, "can't get the height of font.");
                    }
                }

                if (width > dc->rect.width) {
                    width = dc->rect.width;
                    roll = 1;
                    multi_line = 1;
                }


                if (text->flags & FONT_SHOW_MULTI_LINE) {
                    height += multi_line * height;
                }

                if (height > dc->rect.height) {
                    height = dc->rect.height;
                }

                y += (dc->rect.height / 2 - height / 2);

                if (dc->align == UI_ALIGN_CENTER) {
                    x += (dc->rect.width / 2 - width / 2);
                } else if (dc->align == UI_ALIGN_RIGHT) {
                    x += (dc->rect.width - width);
                }

                if (dc->draw_img.en) {//指定坐标刷新
                    x = dc->draw_img.x;
                    y = dc->draw_img.y;
                }

                info->x = x;
                info->y = y;

                u8 elm_index = 2;
                u16 id = CRC16(text->str, text->strlen);
                p = imb_task_search_by_id(imb_task_head_get(dc->index), dc->elm->id, elm_index);

                if ((!p) || (p->id != ((dc->page << 26) | (3 << 24) | (id << 8) | elm_index))) {
                    if (p) {
                        imb_task_src_adr_release(p);
                    }

                    if (info->text_temp_buf) {
                        ASSERT(0, "info->text_temp_buf is not NULL");
                        br28_free(info->text_temp_buf);
                        info->text_temp_buf = 0;
                    }
                    info->text_show_width = width;
                    info->text_temp_buf_size = (width + 7) / 8 * height;
                    info->text_temp_buf = br28_zalloc(info->text_temp_buf_size);
                    if (!info->text_temp_buf) {
                        printf("text->str : 0x%x, text->strlen : %d\n", text->str, text->strlen);
                        put_buf((u8 *)text->str, text->strlen);
                        ASSERT(0, "info->text_temp_buf is NULL:w-%d h-%d", width, height);
                        return 0;
                    }

                    int len = font_textout(info, (u8 *)(text->str + roll * text->offset * 2), text->strlen - roll * text->offset * 2, x, y);
                    ASSERT(len <= 255);
                    text->displen = len;

                    /* g_printf("==========info->text_temp_buf=========="); */
                    /* put_buf(info->text_temp_buf, width * height / 8); */
                    /* g_printf("==============================="); */

                    pixbuf = br28_zalloc((width + 31) / 32 * 32 * height / 8);
                    if (!pixbuf) {
                        return 0;
                    }

                    l1_data_transformation(info->text_temp_buf, pixbuf, x, y, height, width);

                    offset = 0;
                    memset(info->text_temp_buf, 0, (width + 7) / 8 * height);
                    memcpy(info->text_temp_buf, pixbuf, (width + 7) / 8 * height);
                    memset(pixbuf, 0, (width + 31) / 32 * 32 * height / 8);
                    for (int ptr = 0; ptr < (width + 7) / 8 * height;) {
                        for (int ptr2 = 0; ptr2 < (width + 7) / 8; ptr2++) {
                            pixbuf[offset + ptr2] = info->text_temp_buf[ptr];
                            ptr++;
                        }
                        offset += ((width + 31) / 32 * 32 / 8);
                    }

                    /* r_printf("==========pixbuf=========="); */
                    /* put_buf(pixbuf, (width + 31) / 32 * 32 * height / 8); */
                    /* r_printf("=========================="); */

                    if (info->text_temp_buf) {
                        br28_free(info->text_temp_buf);
                        info->text_temp_buf = 0;
                    }
                    info->text_show_width = 0;
                }

                imb_tsk_info.elm_id         = dc->elm->id;
                imb_tsk_info.id             = (dc->page << 26) | (3 << 24) | (id << 8) | elm_index;
                imb_tsk_info.cur_in_flash   = 0;
                imb_tsk_info.data_src       = DATA_SRC_SRAM;
                imb_tsk_info.zip_en         = 0;
                imb_tsk_info.in_format      = LAYER_FORMAT_L1;
                imb_tsk_info.x_offset       = x - dc->rect_orig.left;
                imb_tsk_info.y_offset       = y - dc->rect_orig.top;
                imb_tsk_info.src_w          = (width + 31) / 32 * 32;
                imb_tsk_info.src_h          = height;
                imb_tsk_info.des_w          = (width + 31) / 32 * 32;
                imb_tsk_info.des_h          = height;
                imb_tsk_info.text_color     = text->color;
                imb_tsk_info.ui_malloc = 1;
                if ((p) && (p->id == imb_tsk_info.id)) {
                    imb_tsk_info.priv           = p->dat_src_adr;
                } else {
                    if (p) {
                        imb_task_src_adr_release(p);
                        imb_task_lut_tab_release(p);
                    }
                    imb_tsk_info.priv           = (u32)pixbuf;
                }
                if (p) {
                    imb_task_reset(imb_task_head_get(dc->index), p, &imb_tsk_info);
                } else {
                    imb_task_list_add(imb_task_head_get(dc->index), &imb_tsk_info);
                }
            } else if (text->encode == FONT_ENCODE_UNICODE) {
                if (FT_ERROR_NONE == (info->sta & ~(FT_ERROR_NOTABFILE | FT_ERROR_NOPIXFILE))) {
                    if (text->endian == FONT_ENDIAN_BIG) {
                        info->bigendian = true;
                    } else {
                        info->bigendian = false;
                    }
                    int width = font_textw_width(info, (u8 *)text->str, text->strlen);
                    int height = info->string_height;

                    if (height == 0) {
                        if (info->ascpixel.size) {
                            height = info->ascpixel.size;
                        } else if (info->pixel.size) {
                            height = info->pixel.size;
                        } else {
                            ASSERT(0, "can't get the height of font.");
                        }
                    }

                    if (width > dc->rect.width) {
                        width = dc->rect.width;
                        roll = 1;
                        multi_line = 1;
                    }

                    if (text->flags & FONT_SHOW_MULTI_LINE) {
                        height += multi_line * height;
                    }


                    if (height > dc->rect.height) {
                        height = dc->rect.height;
                    }

                    y += (dc->rect.height / 2 - height / 2);
                    if (dc->align == UI_ALIGN_CENTER) {
                        x += (dc->rect.width / 2 - width / 2);
                    } else if (dc->align == UI_ALIGN_RIGHT) {
                        x += (dc->rect.width - width);
                    }

                    if (dc->draw_img.en) {//指定坐标刷新
                        x = dc->draw_img.x;
                        y = dc->draw_img.y;
                    }

                    info->x = x;
                    info->y = y;


                    u8 elm_index = 2;
                    u16 id = CRC16(text->str, text->strlen);
                    p = imb_task_search_by_id(imb_task_head_get(dc->index), dc->elm->id, elm_index);

                    if ((!p) || (p->id != ((dc->page << 26) | (3 << 24) | (id << 8) | elm_index))) {
                        if (p) {
                            imb_task_src_adr_release(p);
                        }

                        if (info->text_temp_buf) {
                            ASSERT(0, "info->text_temp_buf is not NULL");
                            br28_free(info->text_temp_buf);
                            info->text_temp_buf = 0;
                        }
                        info->text_show_width = width;
                        info->text_temp_buf_size = (width + 7) / 8 * height;
                        info->text_temp_buf = br28_zalloc(info->text_temp_buf_size);
                        if (!info->text_temp_buf) {
                            ASSERT(0, "info->text_temp_buf is NULL");
                            return 0;
                        }

                        int len = font_textout_unicode(info, (u8 *)(text->str + roll * text->offset * 2), text->strlen - roll * text->offset * 2, x, y);
                        ASSERT(len <= 255);
                        text->displen = len;

                        /* g_printf("==========info->text_temp_buf=========="); */
                        /* put_buf(info->text_temp_buf, width * height / 8); */
                        /* g_printf("==============================="); */

                        pixbuf = br28_zalloc((width + 31) / 32 * 32 * height / 8);
                        if (!pixbuf) {
                            return 0;
                        }

                        l1_data_transformation(info->text_temp_buf, pixbuf, x, y, height, width);

                        offset = 0;
                        memset(info->text_temp_buf, 0, (width + 7) / 8 * height);
                        memcpy(info->text_temp_buf, pixbuf, (width + 7) / 8 * height);
                        memset(pixbuf, 0, (width + 31) / 32 * 32 * height / 8);
                        for (int ptr = 0; ptr < (width + 7) / 8 * height;) {
                            for (int ptr2 = 0; ptr2 < (width + 7) / 8; ptr2++) {
                                pixbuf[offset + ptr2] = info->text_temp_buf[ptr];
                                ptr++;
                            }
                            offset += ((width + 31) / 32 * 32 / 8);
                        }

                        /* r_printf("==========pixbuf=========="); */
                        /* put_buf(pixbuf, (width + 31) / 32 * 32 * height / 8); */
                        /* r_printf("=========================="); */

                        if (info->text_temp_buf) {
                            br28_free(info->text_temp_buf);
                            info->text_temp_buf = 0;
                        }
                        info->text_show_width = 0;
                    }

                    imb_tsk_info.elm_id         = dc->elm->id;
                    imb_tsk_info.id             = (dc->page << 26) | (3 << 24) | (id << 8) | elm_index;
                    imb_tsk_info.cur_in_flash   = 0;
                    imb_tsk_info.data_src       = DATA_SRC_SRAM;
                    imb_tsk_info.zip_en         = 0;
                    imb_tsk_info.in_format      = LAYER_FORMAT_L1;
                    imb_tsk_info.x_offset       = x - dc->rect_orig.left;
                    imb_tsk_info.y_offset       = y - dc->rect_orig.top;
                    imb_tsk_info.src_w          = (width + 31) / 32 * 32;
                    imb_tsk_info.src_h          = height;
                    imb_tsk_info.des_w          = (width + 31) / 32 * 32;
                    imb_tsk_info.des_h          = height;
                    imb_tsk_info.text_color     = text->color;
                    imb_tsk_info.ui_malloc = 1;
                    if ((p) && (p->id == imb_tsk_info.id)) {
                        imb_tsk_info.priv           = p->dat_src_adr;
                    } else {
                        if (p) {
                            imb_task_src_adr_release(p);
                            imb_task_lut_tab_release(p);
                        }
                        imb_tsk_info.priv           = (u32)pixbuf;
                    }
                    if (p) {
                        imb_task_reset(imb_task_head_get(dc->index), p, &imb_tsk_info);
                    } else {
                        imb_task_list_add(imb_task_head_get(dc->index), &imb_tsk_info);
                    }
                }
            } else {
                int width = font_textu_width(info, (u8 *)text->str, text->strlen);
                int height = info->string_height;
                /* ASSERT(width == 186, ", width = %d\n", width); */
                if (height == 0) {
                    if (info->ascpixel.size) {
                        height = info->ascpixel.size;
                    } else if (info->pixel.size) {
                        height = info->pixel.size;
                    } else {
                        ASSERT(0, "can't get the height of font.");
                    }
                }
                if (width > dc->rect.width) {
                    width = dc->rect.width;
                }
                if (height > dc->rect.height) {
                    height = dc->rect.height;
                }
                y += (dc->rect.height / 2 - height / 2);
                if (dc->align == UI_ALIGN_CENTER) {
                    if (ui_language_get() == Arabic && width < dc->rect.width) {
                        x -= (dc->rect.width / 2 - width / 2);
                        width = dc->rect.width;
                    }
                    x += (dc->rect.width / 2 - width / 2);
                } else if (dc->align == UI_ALIGN_RIGHT) {
                    x += (dc->rect.width - width);
                }
                if (dc->draw_img.en) {//指定坐标刷新
                    x = dc->draw_img.x;
                    y = dc->draw_img.y;
                }
                info->x = x;
                info->y = y;
                u8 elm_index = 2;
                u16 id = CRC16(text->str, text->strlen);
                p = imb_task_search_by_id(imb_task_head_get(dc->index), dc->elm->id, elm_index);

                if ((!p) || (p->id != ((dc->page << 26) | (3 << 24) | (id << 8) | elm_index))) {
                    if (p) {
                        imb_task_src_adr_release(p);
                    }

                    if (info->text_temp_buf) {
                        ASSERT(0, "info->text_temp_buf is not NULL");
                        br28_free(info->text_temp_buf);
                        info->text_temp_buf = 0;
                    }
                    info->text_show_width = width;
                    info->text_temp_buf_size = (width + 7) / 8 * height;
                    info->text_temp_buf = br28_zalloc(info->text_temp_buf_size);
                    if (!info->text_temp_buf) {
                        ASSERT(0, "info->text_temp_buf is NULL");
                        return 0;
                    }

                    /* r_printf("%s %d--width:%d info->text_show_width:%d", __FUNCTION__, __LINE__, width, info->text_show_width); */
                    int len = font_textout_utf8(info, (u8 *)text->str, text->strlen, x - dc->rect_orig.left, y - dc->rect_orig.top);
                    /* ASSERT(len <= 255); */
                    text->displen = len;

                    /* g_printf("==========info->text_temp_buf=========="); */
                    /* put_buf(info->text_temp_buf, width * height / 8); */
                    /* g_printf("==============================="); */

                    pixbuf = br28_zalloc((width + 31) / 32 * 32 * height / 8);
                    if (!pixbuf) {
                        return 0;
                    }

                    l1_data_transformation(info->text_temp_buf, pixbuf, x - dc->rect_orig.left, y - dc->rect_orig.top, height, width);

                    /* y_printf("==========pixbuf=========="); */
                    /* put_buf(pixbuf, (width + 7) / 8 * height); */
                    /* y_printf("=========================="); */

                    offset = 0;
                    memset(info->text_temp_buf, 0, (width + 7) / 8 * height);
                    memcpy(info->text_temp_buf, pixbuf, (width + 7) / 8 * height);
                    memset(pixbuf, 0, (width + 31) / 32 * 32 * height / 8);
                    for (int ptr = 0; ptr < (width + 7) / 8 * height;) {
                        for (int ptr2 = 0; ptr2 < (width + 7) / 8; ptr2++) {
                            pixbuf[offset + ptr2] = info->text_temp_buf[ptr];
                            ptr++;
                        }
                        offset += ((width + 31) / 32 * 32 / 8);
                    }

                    /* r_printf("==========pixbuf=========="); */
                    /* put_buf(pixbuf, (width + 31) / 32 * 32 * height / 8); */
                    /* r_printf("=========================="); */

                    if (info->text_temp_buf) {
                        br28_free(info->text_temp_buf);
                        info->text_temp_buf = 0;
                    }
                    info->text_show_width = 0;
                }

                imb_tsk_info.elm_id         = dc->elm->id;
                imb_tsk_info.id             = (dc->page << 26) | (3 << 24) | (id << 8) | elm_index;
                imb_tsk_info.cur_in_flash   = 0;
                imb_tsk_info.data_src       = DATA_SRC_SRAM;
                imb_tsk_info.zip_en         = 0;
                imb_tsk_info.in_format      = LAYER_FORMAT_L1;
                imb_tsk_info.x_offset       = x - dc->rect_orig.left;
                imb_tsk_info.y_offset       = y - dc->rect_orig.top;
                imb_tsk_info.src_w          = (width + 31) / 32 * 32;
                imb_tsk_info.src_h          = height;
                imb_tsk_info.des_w          = (width + 31) / 32 * 32;
                imb_tsk_info.des_h          = height;
                imb_tsk_info.text_color     = text->color;
                imb_tsk_info.ui_malloc = 1;
                if ((p) && (p->id == imb_tsk_info.id)) {
                    ASSERT(p->dat_src_adr);
                    imb_tsk_info.priv           = p->dat_src_adr;
                } else {
                    if (p) {
                        imb_task_src_adr_release(p);
                        imb_task_lut_tab_release(p);
                    }
                    imb_tsk_info.priv           = (u32)pixbuf;
                }
                if (p) {
                    imb_task_reset(imb_task_head_get(dc->index), p, &imb_tsk_info);
                } else {
                    imb_task_list_add(imb_task_head_get(dc->index), &imb_tsk_info);
                }
            }
        }
    } else if (text->format && !strcmp(text->format, "ascii")) {
        char *str;
        u32 w_sum;
        if (!text->str) {
            return 0;
        }
        if ((u8)text->str[0] == 0xff) {
            return 0;
        }

        if (dc->align == UI_ALIGN_CENTER) {
            w_sum = font_ascii_width_check(text->str);
            x += (dc->rect.width / 2 - w_sum / 2);
        } else if (dc->align == UI_ALIGN_RIGHT) {
            w_sum = font_ascii_width_check(text->str);
            x += (dc->rect.width - w_sum);
        }

        if (dc->draw_img.en) {//指定坐标刷新
            x = dc->draw_img.x;
            y = dc->draw_img.y;
        }

        str = text->str;

        int width;
        int height;
        u32 realsize_w = 0;
        u32 realsize_h = 0;
        u8 *pixbuf_temp;
        u16 id = CRC16(str, strlen(str));

        u8 elm_index = 2;
        p = imb_task_search_by_id(imb_task_head_get(dc->index), dc->elm->id, elm_index);

        if ((!p) || (p->id != ((dc->page << 26) | (3 << 24) | (id << 8) | elm_index))) {
            /* r_printf("task have no exist or task param is modify"); */
            if (p) {
                /* r_printf("task have exist"); */
                imb_task_src_adr_release(p);
            }

            dc->fbuf_len = 128;
            if (!dc->fbuf) {
                dc->fbuf = br28_zalloc(dc->fbuf_len);
                if (!dc->fbuf) {
                    return 0;
                }
            }

            /* printf("str:%d begin\n", *str); */
            str = text->str;
            while (*str) {
                font_ascii_get_pix(*str, dc->fbuf, dc->fbuf_len, &height, &width);
                realsize_w += width;
                /* printf("str:%d, width: %d, height: %d, realsize_w:%d\n", *str, width, height, realsize_w); */
                str++;
            }
            realsize_h = height;
            r_printf("realsize_w:%d realsize_h:%d", realsize_w, realsize_h);

            pixbuf = br28_zalloc((realsize_w + 31) / 32 * 32 * realsize_h / 8);
            if (!pixbuf) {
                return 0;
            }

            pixbuf_temp = br28_zalloc((realsize_w + 7) / 8 * realsize_h);
            if (!pixbuf_temp) {
                return 0;
            }

            str = text->str;
            while (*str) {
                int i;
                int color;

                memset(dc->fbuf, 0, dc->fbuf_len);
                font_ascii_get_pix(*str, dc->fbuf, dc->fbuf_len, &height, &width);
                /* y_printf("height:%d width:%d", height, width); */
                for (i = 0; i < height / 8; i++) {
                    memcpy(pixbuf_temp + realsize_w * i + offset, dc->fbuf + width * i, width);
                }
                offset += width;
                /* x += width; */
                str++;
            }

            /* g_printf("==========pixbuf_temp=========="); */
            /* put_buf(pixbuf_temp, realsize_w * realsize_h / 8); */
            /* g_printf("==============================="); */

            l1_data_transformation(pixbuf_temp, pixbuf, x, y, realsize_h, realsize_w);

            /* y_printf("==========pixbuf_temp=========="); */
            /* put_buf(pixbuf, (realsize_w + 7) / 8 * realsize_h); */
            /* y_printf("==============================="); */

            offset = 0;
            memset(pixbuf_temp, 0, (realsize_w + 7) / 8 * realsize_h);
            memcpy(pixbuf_temp, pixbuf, (realsize_w + 7) / 8 * realsize_h);
            memset(pixbuf, 0, (realsize_w + 31) / 32 * 32 * realsize_h / 8);
            for (int ptr = 0; ptr < (realsize_w + 7) / 8 * realsize_h;) {
                for (int ptr2 = 0; ptr2 < (realsize_w + 7) / 8; ptr2++) {
                    pixbuf[offset + ptr2] = pixbuf_temp[ptr];
                    ptr++;
                }
                offset += ((realsize_w + 31) / 32 * 32 / 8);
            }

            /* r_printf("==========pixbuf=========="); */
            /* put_buf(pixbuf, (realsize_w + 31) / 32 * 32 * realsize_h / 8); */
            /* r_printf("=========================="); */

            if (pixbuf_temp) {
                br28_free(pixbuf_temp);
            }

            if (dc->fbuf) {
                br28_free(dc->fbuf);
                dc->fbuf = NULL;
                dc->fbuf_len = 0;
            }
        } else {
            dc->fbuf_len = 128;
            if (!dc->fbuf) {
                dc->fbuf = br28_zalloc(dc->fbuf_len);
                if (!dc->fbuf) {
                    return 0;
                }
            }

            str = text->str;
            while (*str) {
                font_ascii_get_pix(*str, dc->fbuf, dc->fbuf_len, &height, &width);
                realsize_w += width;
                /* printf("str:%d, width: %d, height: %d, realsize_w:%d\n", *str, width, height, realsize_w); */
                str++;
            }
            realsize_h = height;

            if (dc->fbuf) {
                br28_free(dc->fbuf);
                dc->fbuf = NULL;
                dc->fbuf_len = 0;
            }
        }

        imb_tsk_info.elm_id         = dc->elm->id;
        imb_tsk_info.id             = (dc->page << 26) | (3 << 24) | (id << 8) | elm_index;
        imb_tsk_info.cur_in_flash   = 0;
        imb_tsk_info.data_src       = DATA_SRC_SRAM;
        imb_tsk_info.zip_en         = 0;
        imb_tsk_info.in_format      = LAYER_FORMAT_L1;
        imb_tsk_info.x_offset       = x - dc->rect_orig.left;
        imb_tsk_info.y_offset       = y - dc->rect_orig.top;
        imb_tsk_info.src_w          = (realsize_w + 31) / 32 * 32;
        imb_tsk_info.src_h          = realsize_h;
        imb_tsk_info.des_w          = (realsize_w + 31) / 32 * 32;
        imb_tsk_info.des_h          = realsize_h;
        imb_tsk_info.text_color     = text->color;
        imb_tsk_info.ui_malloc = 1;
        if ((p) && (p->id == imb_tsk_info.id)) {
            imb_tsk_info.priv           = p->dat_src_adr;
        } else {
            if (p) {
                imb_task_src_adr_release(p);
                imb_task_lut_tab_release(p);
            }
            imb_tsk_info.priv           = (u32)pixbuf;
        }
        if (p) {
            imb_task_reset(imb_task_head_get(dc->index), p, &imb_tsk_info);
        } else {
            imb_task_list_add(imb_task_head_get(dc->index), &imb_tsk_info);
        }
    } else if (text->format && !strcmp(text->format, "strpic")) {
        u16 id = *((u16 *)text->str);
        if (open_string_pic(dc->prj, &file, id)) {
            return 0;
        }

        if (!file.width || !file.height) {
            return 0;
        }

        y += (dc->rect.height / 2 - file.height / 2);

        if (dc->align == UI_ALIGN_CENTER) {
            x += (dc->rect.width / 2 - file.width / 2);
        } else if (dc->align == UI_ALIGN_RIGHT) {
            x += (dc->rect.width - file.width);
        }

        if (dc->draw_img.en) {
            x = dc->draw_img.x;
            y = dc->draw_img.y;
        }

        u8 elm_index = 2;
        p = imb_task_search_by_id(imb_task_head_get(dc->index), dc->elm->id, elm_index);

        imb_tsk_info.rect.left = x - dc->rect_orig.left;
        imb_tsk_info.rect.top = y - dc->rect_orig.top;
        imb_tsk_info.rect.width = file.width;
        imb_tsk_info.rect.height = file.height;

        struct rect draw;
        draw.left = dc->draw.left - dc->rect_orig.left;
        draw.top = dc->draw.top - dc->rect_orig.top;
        draw.width = dc->draw.width;
        draw.height = dc->draw.height;

        if (!get_rect_cover(&imb_tsk_info.rect, &draw, &imb_tsk_info.draw)) {
            if (p) {
                imb_task_enable(p, false);
            }
            return -EFAULT;
        }

        imb_tsk_info.elm_id         = dc->elm->id;
        imb_tsk_info.id             = (dc->page << 26) | (1 << 24) | (id << 8) | elm_index;
        imb_tsk_info.data_src       = DATA_SRC_FLASH;
        imb_tsk_info.cur_in_flash   = 1;
        imb_tsk_info.zip_en         = 0;
        imb_tsk_info.in_format      = LAYER_FORMAT_L1;
        imb_tsk_info.x_offset       = x - dc->rect_orig.left;
        imb_tsk_info.y_offset       = y - dc->rect_orig.top;
        imb_tsk_info.src_w          = file.width;
        imb_tsk_info.src_h          = file.height;
        imb_tsk_info.des_w          = file.width;
        imb_tsk_info.des_h          = file.height;
        imb_tsk_info.text_color     = text->color;
        imb_tsk_info.ff_info        = ui_get_str_file_info_by_pj_id(dc->prj);
        imb_tsk_info.l1_type        = L1_TEXT;
        /* printf("%d %d\n",file.width,file.height); */

        if ((imb_tsk_info.data_src == DATA_SRC_SRAM) || (imb_tsk_info.data_src == DATA_SRC_PSRAM)) {
            g_printf("%s %d", __FUNCTION__, __LINE__);
            if ((p) && (p->id == imb_tsk_info.id)) {
                imb_tsk_info.priv = p->dat_src_adr;
                imb_tsk_info.cur_in_flash = 0;
            } else {
                if (p) {
                    imb_task_src_adr_release(p);
                    imb_task_lut_tab_release(p);
                }
                imb_tsk_info.priv       = (u32)&file;
            }
        } else if (imb_tsk_info.data_src == DATA_SRC_FLASH) {
            if ((p) && (p->id != imb_tsk_info.id)) {
                imb_task_file_info_release(p);
                imb_task_lut_tab_release(p);
            }
            imb_tsk_info.priv       = (u32)&file;
        }

        if (p) {
            imb_task_reset(imb_task_head_get(dc->index), p, &imb_tsk_info);
        } else {
            imb_task_list_add(imb_task_head_get(dc->index), &imb_tsk_info);
        }
    } else if (text->format && !strcmp(text->format, "mulstr")) {
        u16 id = (u8)text->str[0];
        int w;
        int h;
        int mulstr_task_cnt = 0;
        u16 *str_p = text->str;

        if (get_multi_string_width(dc, text->str, &w, &h)) {
            return -EINVAL;
        }

        y += (dc->rect.height / 2 - h / 2);
        if (dc->align == UI_ALIGN_CENTER) {
            x += (dc->rect.width / 2 - w / 2);
        } else if (dc->align == UI_ALIGN_RIGHT) {
            x += (dc->rect.width - w);
        }


        u8 elm_index = 2;
        str_p = text->str;
        while (*str_p != 0) {
            id = *str_p;

            if ((id == 0xffff) || (id == 0)) {
                return 0;
            }

            if (open_string_pic(dc->prj, &file, id)) {
                return 0;
            }

            p = imb_task_search_by_id(imb_task_head_get(dc->index), dc->elm->id, elm_index);

            imb_tsk_info.elm_id         = dc->elm->id;
            imb_tsk_info.id             = (dc->page << 26) | (1 << 24) | (id << 8) | elm_index;
            imb_tsk_info.data_src       = DATA_SRC_FLASH;
            imb_tsk_info.cur_in_flash   = 1;
            imb_tsk_info.zip_en         = 0;
            imb_tsk_info.in_format      = LAYER_FORMAT_L1;//file.format;
            imb_tsk_info.x_offset       = x - dc->rect_orig.left;
            imb_tsk_info.y_offset       = y - dc->rect_orig.top;
            imb_tsk_info.src_w          = file.width;
            imb_tsk_info.src_h          = file.height;
            imb_tsk_info.des_w          = file.width;
            imb_tsk_info.des_h          = file.height;
            imb_tsk_info.text_color     = text->color;
            imb_tsk_info.ff_info        = ui_get_str_file_info_by_pj_id(dc->prj);//&dc->str_file_info;
            imb_tsk_info.l1_type        = L1_TEXT;

            if ((imb_tsk_info.data_src == DATA_SRC_SRAM) || (imb_tsk_info.data_src == DATA_SRC_PSRAM)) {
                if ((p) && (p->id == imb_tsk_info.id)) {
                    imb_tsk_info.priv = p->dat_src_adr;
                    imb_tsk_info.cur_in_flash = 0;
                } else {
                    if (p) {
                        imb_task_src_adr_release(p);
                        imb_task_lut_tab_release(p);
                    }
                    imb_tsk_info.priv       = (u32)&file;
                }
            } else if (imb_tsk_info.data_src == DATA_SRC_FLASH) {
                /* g_printf("%s %d", __FUNCTION__, __LINE__); */
                if ((p) && (p->id != imb_tsk_info.id)) {
                    imb_task_file_info_release(p);
                    imb_task_lut_tab_release(p);
                }
                imb_tsk_info.priv       = (u32)&file;
            }

            if (p) {
                imb_task_reset(imb_task_head_get(dc->index), p, &imb_tsk_info);
            } else {
                imb_task_list_add(imb_task_head_get(dc->index), &imb_tsk_info);
            }
            x += file.width;
            str_p++;
            elm_index++;
        }
        imb_task_delete_invalid(imb_task_head_get(dc->index), dc->elm->id, elm_index);
    } else if (text->format && !strcmp(text->format, "image")) {
        /* y_printf("image...."); */
        /* y_printf("dc->elm->id:0x%x", dc->elm->id); */
        u16 id = ((u8)text->str[1] << 8) | (u8)text->str[0];
        u16 cnt = 0;
        int image_task_cnt = 0;
        u32 w, h;
        int ww, hh;

        if (image_str_size_check(dc, dc->page, text->str, &ww, &hh) != 0) {
            return -EFAULT;
        }
        if (dc->align == UI_ALIGN_CENTER) {
            x += (dc->rect.width / 2 - ww / 2);
        } else if (dc->align == UI_ALIGN_RIGHT) {
            x += (dc->rect.width - ww);
        }
        y += (dc->rect.height / 2 - hh / 2);

#if 0
        id = ((u8)text->str[1] << 8) | (u8)text->str[0];
        cnt = 0;
        while ((id != 0x00ff) && (id != 0xffff)) {
            cnt += 2;
            id = ((u8)text->str[cnt + 1] << 8) | (u8)text->str[cnt];
            image_task_cnt++;
        }
        elm_task_num = get_elm_task_num(dc->elm->id);
        printf("elm id : 0x%x\n", dc->elm->id);
        printf("elm_task_num : %d\n", elm_task_num);
        printf("image_task_cnt : %d\n", image_task_cnt);

        while (elm_task_num > image_task_cnt) {
            imb_task_delete_by_id(dc->elm->id, 0xffffffff);
            elm_task_num--;
        }
#endif

        id = ((u8)text->str[1] << 8) | (u8)text->str[0];
        cnt = 0;


        u8 elm_index = 2;
        while ((id != 0x00ff) && (id != 0xffff)) {
            if (open_image_by_id(dc->prj, NULL, &file, id, dc->page) != 0) {
                return -EFAULT;
            }

            p = imb_task_search_by_id(imb_task_head_get(dc->index), dc->elm->id, elm_index);

            imb_tsk_info.elm_id         = dc->elm->id;
            imb_tsk_info.id             = (dc->page << 26) | (0 << 24) | (id << 8) | elm_index;
            imb_tsk_info.data_src       = DATA_SRC_FLASH;
            imb_tsk_info.cur_in_flash   = 1;
            imb_tsk_info.zip_en         = 1;
            imb_tsk_info.in_format      = file.format;
            imb_tsk_info.x_offset       = x - dc->rect_orig.left;
            imb_tsk_info.y_offset       = y - dc->rect_orig.top;
            imb_tsk_info.src_w          = file.width;
            imb_tsk_info.src_h          = file.height;
            imb_tsk_info.des_w          = file.width;
            imb_tsk_info.des_h          = file.height;
            imb_tsk_info.ff_info        = ui_get_image_file_info_by_pj_id(dc->prj);//&dc->image_file_info;
            if ((imb_tsk_info.data_src == DATA_SRC_SRAM) || (imb_tsk_info.data_src == DATA_SRC_PSRAM)) {
                /* g_printf("%s %d", __FUNCTION__, __LINE__); */
                if ((p) && (p->id == imb_tsk_info.id)) {
                    imb_tsk_info.priv = p->dat_src_adr;
                    imb_tsk_info.cur_in_flash   = 0;
                } else {
                    if (p) {
                        imb_task_src_adr_release(p);
                        imb_task_lut_tab_release(p);
                    }
                    imb_tsk_info.priv       = (u32)&file;
                }
            } else if (imb_tsk_info.data_src == DATA_SRC_FLASH) {
                /* g_printf("%s %d", __FUNCTION__, __LINE__); */
                if ((p) && (p->id != imb_tsk_info.id)) {
                    imb_task_file_info_release(p);
                    imb_task_lut_tab_release(p);
                }
                imb_tsk_info.priv       = (u32)&file;
            }

            if (p) {
                imb_task_reset(imb_task_head_get(dc->index), p, &imb_tsk_info);
            } else {
                imb_task_list_add(imb_task_head_get(dc->index), &imb_tsk_info);
            }
            x += file.width;
            cnt += 2;
            id = ((u8)text->str[cnt + 1] << 8) | (u8)text->str[cnt];
            elm_index++;
        }

        imb_task_delete_invalid(imb_task_head_get(dc->index), dc->elm->id, elm_index);
    }
    return 0;
}



#include "ui_draw/ui_circle.h"
void ui_draw_circle(struct draw_context *dc, int center_x, int center_y,
                    int radius_small, int radius_big, int angle_begin,
                    int angle_end, int color, int percent)
{
    struct circle_info info;
    info.center_x = center_x;
    info.center_y = center_y;
    info.radius_big = radius_big;
    info.radius_small = radius_small;
    info.angle_begin = angle_begin;
    info.angle_end = angle_end;
    info.left = dc->draw.left;
    info.top = dc->draw.top;
    info.width = dc->draw.width;
    info.height = dc->draw.height;

    info.x = 0;
    info.y = 0;
    info.disp_x = dc->disp.left;
    info.disp_y = dc->disp.top;

    info.color = color;
    info.bitmap_width = dc->disp.width;
    info.bitmap_height = dc->disp.height;
    info.bitmap_pitch = dc->disp.width * 2;
    info.bitmap = dc->buf;
    info.bitmap_size = dc->len;
    info.bitmap_depth = 16;

    draw_circle_by_percent(&info, percent);
}

AT_UI_RAM
u32 br28_read_point(struct draw_context *dc, u16 x, u16 y)
{
    u32 pixel;
    u16 *pdisp = dc->buf;
    if (dc->data_format == DC_DATA_FORMAT_OSD16) {
        int offset = (y - dc->disp.top) * dc->disp.width + (x - dc->disp.left);
        ASSERT((offset * 2 + 1) < dc->len, "dc->len:%d", dc->len);
        if ((offset * 2 + 1) >= dc->len) {
            return -1;
        }

        pixel = pdisp[offset];//(dc->buf[offset * 2] << 8) | dc->buf[offset * 2 + 1];
    } else {
        ASSERT(0);
    }

    return pixel;
}


__attribute__((always_inline_when_const_args))
AT_UI_RAM
int br28_draw_point(struct draw_context *dc, u16 x, u16 y, u32 pixel)
{
    if (dc->data_format == DC_DATA_FORMAT_OSD16) {
        int offset = (y - dc->disp.top) * dc->disp.width + (x - dc->disp.left);

        /* ASSERT((offset * 2 + 1) < dc->len, "dc->len:%d", dc->len); */
        if ((offset * 2 + 1) >= dc->len) {
            return -1;
        }

        dc->buf[offset * 2    ] = pixel >> 8;
        dc->buf[offset * 2 + 1] = pixel;
    } else if (dc->data_format == DC_DATA_FORMAT_MONO) {
        /* ASSERT(x < __this->info.width); */
        /* ASSERT(y < __this->info.height); */
        if ((x >= __this->info.width) || (y >= __this->info.height)) {
            return -1;
        }

        if (pixel & BIT(31)) {
            dc->buf[y / 8 * __this->info.width + x] ^= BIT(y % 8);
        } else if (pixel == 0x55aa) {
            dc->buf[y / 8 * __this->info.width + x] &= ~BIT(y % 8);
        } else if (pixel) {
            dc->buf[y / 8 * __this->info.width + x] |= BIT(y % 8);
        } else {
            dc->buf[y / 8 * __this->info.width + x] &= ~BIT(y % 8);
        }
    }

    return 0;
}


int __ui_fill_rect(struct draw_context *dc, int left, int top, int width, int height, u32 acolor)
{
    int i;
    int w, h;
    u16 color = acolor & 0xffff;

    if (!dc->buf) {
        return 0;
    }

    struct rect rect;
    rect.left = left;
    rect.top = top;
    rect.width = width;
    rect.height = height;

    struct rect cover;
    if (!get_rect_cover(&dc->draw, &rect, &cover)) {
        return 0;
    }

#if 0
    for (h = 0; h < cover.height; h++) {
        for (w = 0; w < cover.width; w++) {
            if (platform_api->draw_point) {
                platform_api->draw_point(dc, cover.left + w, cover.top + h, color);
            }
        }
    }
#else
    u16 color16 = (color >> 8) | ((color & 0xff) << 8);
    u32 color32 = (color16 << 16) | color16;

    h = 0;
    u32 *p32 = (u32 *)&dc->buf[(cover.top + h - dc->disp.top) * dc->disp.width * 2 + (cover.left - dc->disp.left) * 2];
    u32 *_p32 = p32;
    u32 len = cover.width * 2;
    if ((u32)p32 % 4) {
        u16 *p16 = (u16 *)p32;
        *p16++ = color16;
        p32 = (u32 *)p16;
        len -= 2;
        ASSERT((u32)p32 % 4 == 0);
    }

    u32 count = len / 4;
    while (count--) {
        *p32++ = color32;
    }
    count = (len % 4) / 2;
    u16 *p16 = (u16 *)p32;
    while (count--) {
        *p16++ = color16;
    }

    for (h = 1; h < cover.height; h++) {
        u32 *__p32 = (u32 *)&dc->buf[(cover.top + h - dc->disp.top) * dc->disp.width * 2 + (cover.left - dc->disp.left) * 2];
        memcpy(__p32, _p32, cover.width * 2);
    }
#endif

    return 0;
}


/* int ui_fill_rect(struct draw_context *dc, int left, int top, int width, int height, u32 acolor) */
/* { */
/* } */

int ui_draw_image(struct draw_context *dc, int page, int id, int x, int y)
{
    dc->draw_img.en = true;
    dc->draw_img.x = x;
    dc->draw_img.y = y;
    dc->draw_img.id = id;
    dc->draw_img.page = page;
    return br28_draw_image(dc, id, 0, NULL);
}


int ui_draw_ascii(struct draw_context *dc, char *str, int strlen, int x, int y, int color)
{
    struct ui_text_attrs text = {0};
    text.str = str;
    text.format = "ascii";
    text.color = color;
    text.strlen = strlen;
    text.flags = FONT_DEFAULT;

    dc->draw_img.en = true;
    dc->draw_img.x = x;
    dc->draw_img.y = y;

    return br28_show_text(dc, &text);
}

int ui_draw_text(struct draw_context *dc, int encode, int endian, char *str, int strlen, int x, int y, int color)
{
    struct ui_text_attrs text = {0};
    text.str = str;
    text.format = "text";
    text.color = color;
    text.strlen = strlen;
    text.encode = encode;
    text.endian = endian;
    text.flags = FONT_DEFAULT;

    dc->draw_img.en = true;
    dc->draw_img.x = x;
    dc->draw_img.y = y;

    return br28_show_text(dc, &text);
}

int ui_draw_strpic(struct draw_context *dc, int id, int x, int y, int color)
{
    struct ui_text_attrs text = {0};
    u32 strbuf ALIGNED(32);

    strbuf = id;
    text.str = &strbuf;
    text.format = "strpic";
    text.color = color;
    text.strlen = 0;
    text.encode = 0;
    text.endian = 0;
    text.flags = 0;

    dc->draw_img.en = true;
    dc->draw_img.x = x;
    dc->draw_img.y = y;

    return br28_show_text(dc, &text);
}

u32 ui_draw_get_pixel(struct draw_context *dc, int x, int y)
{
    /* return platform_api->read_point(dc, x, y); */
    return 0;
}

u16 ui_draw_get_mixed_pixel(u16 backcolor, u16 forecolor, u8 alpha)
{
    /* return get_mixed_pixel(backcolor, forecolor, alpha); */
    return 0;
}

static const struct ui_platform_api br28_platform_api = {
    .malloc             = br28_malloc,
    .free               = br28_free,

    .load_style         = br28_load_style,
    .load_window        = br28_load_window,
    .unload_window      = br28_unload_window,

    .open_draw_context  = br28_open_draw_context,
    .get_draw_context   = br28_get_draw_context,
    .put_draw_context   = br28_put_draw_context,
    .close_draw_context = br28_close_draw_context,
    .set_draw_context   = br28_set_draw_context,

    .show_element       = br28_show_element,
    .hide_element       = br28_hide_element,
    .delete_element     = br28_delete_element,

    .fill_rect          = br28_fill_rect,
    .draw_image         = br28_draw_image,
    .show_text          = br28_show_text,

    .draw_rect          = br28_draw_rect,
    .read_point         = br28_read_point,
    .draw_point         = br28_draw_point,
    .invert_rect        = br28_invert_rect,

    .read_image_info    = br28_read_image_info,

    .set_timer          = br28_set_timer,
    .del_timer          = br28_del_timer,
};


static int open_resource_file()
{
    int ret;

    printf("open_resouece_file...\n");

    ret = open_resfile(RES_PATH"JL/JL.res");
    if (ret) {
        return -EINVAL;
    }
    ret = open_str_file(RES_PATH"JL/JL.str");
    if (ret) {
        return -EINVAL;
    }
    ret = font_ascii_init(FONT_PATH"ascii.res");
    if (ret) {
        return -EINVAL;
    }
    return 0;
}

int __attribute__((weak)) lcd_get_scrennifo(struct fb_var_screeninfo *info)
{
    info->s_xoffset = 0;
    info->s_yoffset = 0;
    info->s_xres = 240;
    info->s_yres = 240;

    return 0;
}

int ui_platform_init(void *lcd)
{
    struct rect rect;
    struct lcd_info info = {0};

    /* #ifdef UI_BUF_CALC */
    /* INIT_LIST_HEAD(&buffer_used.list); */
    /* #endif */

    __this->api = &br28_platform_api;
    ASSERT(__this->api->open_draw_context);
    ASSERT(__this->api->get_draw_context);
    ASSERT(__this->api->put_draw_context);
    ASSERT(__this->api->set_draw_context);
    ASSERT(__this->api->close_draw_context);


    __this->lcd = lcd_get_hdl();
    ASSERT(__this->lcd);
    ASSERT(__this->lcd->init);
    ASSERT(__this->lcd->get_screen_info);
    ASSERT(__this->lcd->buffer_malloc);
    ASSERT(__this->lcd->buffer_free);
    ASSERT(__this->lcd->draw);
    ASSERT(__this->lcd->set_draw_area);
    ASSERT(__this->lcd->backlight_ctrl);

    if (__this->lcd->power_ctrl) {
        __this->lcd->power_ctrl(true);
    }

    if (__this->lcd->init) {
        __this->lcd->init(lcd);
    }

    if (__this->lcd->backlight_ctrl) {
        __this->lcd->backlight_ctrl(100);
    }

    if (__this->lcd->get_screen_info) {
        __this->lcd->get_screen_info(&info);
    }

    /* if (__this->lcd->clear_screen) { */
    /* __this->lcd->clear_screen(0x000000); */
    /* } */

    rect.left   = 0;
    rect.top    = 0;
    rect.width  = info.width;
    rect.height = info.height;

    printf("ui_platform_init :: [%d,%d,%d,%d]\n", rect.left, rect.top, rect.width, rect.height);

    ui_core_init(__this->api, &rect);

    return 0;
}



int ui_style_file_version_compare(int version)
{
    int v;
    int len;
    struct ui_file_head head;
    static u8 checked = 0;

    if (checked == 0) {
        if (!ui_file) {
            puts("ui version_compare ui_file null!\n");
            ASSERT(0);
            return 0;
        }
        res_fseek(ui_file, 0, SEEK_SET);
        len = sizeof(struct ui_file_head);
        res_fread(ui_file, &head, len);
        printf("style file version is: 0x%x,UI_VERSION is: 0x%x\n", *(u32 *)(head.res), version);
        if (*(u32 *)head.res != version) {
            puts("style file version is not the same as UI_VERSION !!\n");
            ASSERT(0);
        }
        checked = 1;
    }
    return 0;
}





int ui_watch_poweron_update_check()
{
#if(CONFIG_UI_STYLE == STYLE_JL_WTACH)
    //如果上次表盘升级异常，直接进入表盘升级模式等待升级完成
    //加入新标志位的判断，用于升级过程中断电后开机进入升级模式的情况
    if (smartbox_eflash_flag_get() != 0 ||
        smartbox_eflash_update_flag_get()) {
        printf("\n\ngoto watch update mode\n\n");

        //上电后，发现上一次表盘升级异常,先初始化升级需要的蓝牙相关部分,
        //再跳转到升级模式

        watch_update_over = 2;
        app_smartbox_task_prepare(0, SMARTBOX_TASK_ACTION_WATCH_TRANSFER, 0);
        return -1;
    }
#endif
    return 0;
}


int ui_upgrade_file_check_valid()
{
#if UI_UPGRADE_RES_ENABLE
    //简单实现
    //假设升级界面必须存在，调用了该接口证明资源不完整
    //需要进行升级
    smartbox_eflash_flag_set(1);//这个标志的清理需要注意
    return 0;
#endif
    return -ENOENT;
}

int ui_file_check_valid()
{

#if UI_WATCH_RES_ENABLE
    int ret;
    printf("open_resouece_file...\n");
    int i = 0;
    RESFILE *file = NULL;
    char *sty_suffix = ".sty";
    char *res_suffix = ".res";
    char *str_suffix = ".str";
    char tmp_name[100];
    /* u32 list_len = sizeof(WATCH_STY_CHECK_LIST) / sizeof(WATCH_STY_CHECK_LIST[0]); */
    u32 list_len;
    u32 tmp_strlen;
    u32 suffix_len;

    //如果上次表盘升级异常，直接进入表盘升级模式等待升级完成
    /* if (smartbox_eflash_flag_get() != 0 || */
    /*     smartbox_eflash_update_flag_get()) { */
    if (smartbox_eflash_update_flag_get() != 0 ||
        smartbox_eflash_resume_opt()) {
        printf("\n\nneed goto watch update mode\n\n");
        return -EINVAL;
    } else {
        ret = watch_set_init();
        if (ret != 0) {
            return -EINVAL;
        }
    }

    list_len = watch_items;

    for (i = 0; i < list_len; i++) {
        if (file) {
            res_fclose(file);
            file = NULL;
        }
        /* file = res_fopen(WATCH_STY_CHECK_LIST[i], "r"); */
        file = res_fopen(watch_res[i], "r");
        if (!file) {
            return -ENOENT;
        }
    }
    printf("%s : %d", __FUNCTION__, __LINE__);

#endif

    return 0;
}

int get_cur_srreen_width_and_height(u16 *screen_width, u16 *screen_height)
{
    memcpy((u8 *)screen_width, (u8 *)&__this->info.width, sizeof(__this->info.width));
    memcpy((u8 *)screen_height, (u8 *)&__this->info.height, sizeof(__this->info.height));
    return 0;
}

#endif

