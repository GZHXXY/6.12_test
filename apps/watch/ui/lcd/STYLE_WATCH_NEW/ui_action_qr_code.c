#include "ui/ui_api.h"
#include "ui/ui.h"
#include "ui/ui_style.h"
#include "ui/qr_code.h"
#include "key_event_deal.h"

#define LOG_TAG_CONST       UI
#define LOG_TAG     		"[UI-ACTION]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"



/* #if (!TCFG_LUA_ENABLE) */
#ifdef CONFIG_UI_STYLE_JL_ENABLE
#if TCFG_UI_ENABLE_QR_CODE

#define STYLE_NAME  JL

#define LCD_IMG_WIDTH   300
#define QR_CODE_MESSAGE "{\"bleAddr\":\"0E:50:09:BC:8B:83\",\"connectWay\":1,\"edrAddr\":\"0E:50:09:1B:D0:E5\",\"name\":\"JL_WATCH_1\",\"pid\":49,\"vid\":2}"

static u8 *row_img = 0;
static u8 *copy_row_img = 0;
static u8 qr_code_mess[180];

struct _QR_CODE {
    int code128_mode;
    u8 qr_version;
    u8 qr_max_version;
    u8 qr_ecc_level;
    int qr_code_max_input_len;
    int qr_buf_size;
    int out_size;
    int line_size;
};

struct _QR_CODE qr_code = {
    .code128_mode = 60,
    .qr_version = 3,
    .qr_max_version = 8,
    .qr_ecc_level = 2,
    .qr_code_max_input_len = 384,
    .qr_buf_size = 4096,
};

jl_code_param_t jl_code_param;

void bt_addr2string(u8 *addr, u8 *buf);

void qr_code_message_init(u8 *ble_addr, u8 *edr_addr, u8 *name)
{
    u8 ble_addr_ptr[18] = {0};
    u8 edr_addr_ptr[18] = {0};
    bt_addr2string(ble_addr, ble_addr_ptr);
    bt_addr2string(edr_addr, edr_addr_ptr);
    snprintf(qr_code_mess, sizeof(qr_code_mess),
             "{\"bleAddr\":\"%s\",\"connectWay\":1,\"edrAddr\":\"%s\",\"name\":\"%s\",\"pid\":49,\"vid\":2}",
             ble_addr_ptr, edr_addr_ptr, name);
    log_info("%s", qr_code_mess);
}

void jl_qr_code_process()
{
    int img_w = LCD_IMG_WIDTH;
    int ret;
    u8 num = 0;
    log_info("before init:");
    mem_stats();
    //qr_code需要消耗一定的内存：qr_buf_size+4*(21+(qr_max_version-1)*4)*(21+(qr_max_version-1)*4)
    jl_code_init(qr_code.code128_mode, qr_code.qr_version, qr_code.qr_max_version, qr_code.qr_ecc_level,  qr_code.qr_code_max_input_len, qr_code.qr_buf_size, img_w);
    /* ret = jl_code_process(3, "https://cn.bing.com/", 20, &qr_code.out_size, &qr_code.line_size);  //返回值为 1，说明编码成功。 */
    /* ret = jl_code_process(3, QR_CODE_MESSAGE, 128, &qr_code.out_size, &qr_code.line_size);  //返回值为 1，说明编码成功。 */
    ret = jl_code_process(3, qr_code_mess, strlen(qr_code_mess), &qr_code.out_size, &qr_code.line_size);  //返回值为 1，说明编码成功。
    log_info("out_size:%d line_size:%d", qr_code.out_size, qr_code.line_size);
    if (ret == 1) {                                   //out_size为基础数据大小，也就是原始数据大小
        jl_code_param.l_size = 1;
        if (jl_code_param.l_size > qr_code.line_size) {
            jl_code_param.l_size = qr_code.line_size;
        }
        jl_code_set_info(&jl_code_param);  //设置一个输出数据占用几个像素点，这个地方同比放大了数据
        row_img = zalloc(qr_code.out_size * jl_code_param.l_size);
        if (row_img == 0) {
            jl_code_deinit();
            log_info("after deinit:");
            mem_stats();
            return;
        }
        copy_row_img = zalloc(qr_code.out_size * jl_code_param.l_size * qr_code.out_size * jl_code_param.l_size);
        if (copy_row_img == 0) {
            jl_code_deinit();
            log_info("after deinit:");
            mem_stats();
            return;
        }
        log_info("remain heap size:");
        mem_stats();
        for (unsigned char j = 0; j < qr_code.out_size; j++) {
            jl_code_get_data(qr_code.out_size, j, row_img);    //读取输出数据，这里实际上得到的是 out_size* jl_code_param.l_size个数据，是将原数据同比放大jl_code_param.l_size倍
            /* put_buf(row_img, qr_code.out_size * jl_code_param.l_size); */
            for (int k = 0; k < jl_code_param.l_size; k++) {             //jl_code_param.l_size和out_size共同影响row_img中可用数据的大小，例如：jl_code_param.l_size=x，out_size=y，那么
                memcpy(copy_row_img + (j * jl_code_param.l_size + k) * qr_code.out_size * jl_code_param.l_size, row_img, qr_code.out_size * jl_code_param.l_size);
            }
        }
    }
    jl_code_deinit();
    log_info("after deinit:");
    mem_stats();
}


void fill_rect(void *_dc, struct rect *rectangle, u16 color);
static void qr_fill_rect(void *_dc, int x, int y, int width, int height, int color)
{
    struct rect rectangle;
    rectangle.left = x;
    rectangle.top = y;
    rectangle.width = width;
    rectangle.height = height;

    fill_rect(_dc, &rectangle, color);
}

static void ui_draw_user_guide(int id, u8 *dst_buf, struct rect *dst_r, struct rect *src_r, u8 bytes_per_pixel, void *priv)
{
    int w, h;
    struct rect r;
    struct rect rect = {0};
    int dst_stride = (dst_r->width * bytes_per_pixel + 3) / 4 * 4;
    int src_stride = (src_r->width * bytes_per_pixel + 3) / 4 * 4;
    u8 step_size;
    u8 remain_size;


    struct draw_context dc = {0};
    dc.buf = dst_buf;
    memcpy(&dc.disp, dst_r, sizeof(struct rect));
    memcpy(&dc.draw, dst_r, sizeof(struct rect));
    memcpy(&rect, src_r, sizeof(struct rect));

    if (id == 1) {
        //添加白底画图区域
        qr_fill_rect(&dc, rect.left + 70, rect.top + 80, LCD_IMG_WIDTH + 10, LCD_IMG_WIDTH + 10, 0xffff);
        /*
         * 以下绘图函数只能在该控件的范围内显示，超出控件区域不显示
         * */
        if ((qr_code.out_size == 0) || (jl_code_param.l_size == 0)) {
            step_size = 0;
            remain_size = 0;
        } else {
            step_size = LCD_IMG_WIDTH / (qr_code.out_size * jl_code_param.l_size);
            remain_size = (LCD_IMG_WIDTH % (qr_code.out_size * jl_code_param.l_size)) / 2;
        }
        /* log_info("step_size:%d %d %d",step_size,remain_size,qr_code.out_size * jl_code_param.l_size); */
        for (int i = 0; i < qr_code.out_size * jl_code_param.l_size; i++) {
            for (int j = 0; j < qr_code.out_size * jl_code_param.l_size; j++) {
                /* if (img[i][j] == 0xff) { */
                if (copy_row_img[i * qr_code.out_size * jl_code_param.l_size + j] == 0xff) {
                    qr_fill_rect(&dc, rect.left + 75 + remain_size + step_size * j, rect.top + 85 + remain_size + step_size * i, step_size, step_size, 0xffff);
                }
                /* if (img[i][j] == 0x00) { */
                if (copy_row_img[i * qr_code.out_size * jl_code_param.l_size + j] == 0x00) {
                    qr_fill_rect(&dc, rect.left + 70 + remain_size + step_size * j, rect.top + 85 + remain_size + step_size * i, step_size, step_size, 0x0000);
                }
            }
        }
    } else if (id == 2) {
        //添加白底画图区域
        qr_fill_rect(&dc, rect.left + 70, rect.top + 70, LCD_IMG_WIDTH + 10, LCD_IMG_WIDTH + 10, 0xffff);
        /*
         * 以下绘图函数只能在该控件的范围内显示，超出控件区域不显示
         * */
        if ((qr_code.out_size == 0) || (jl_code_param.l_size == 0)) {
            step_size = 0;
            remain_size = 0;
        } else {
            step_size = LCD_IMG_WIDTH / (qr_code.out_size * jl_code_param.l_size);
            remain_size = (LCD_IMG_WIDTH % (qr_code.out_size * jl_code_param.l_size)) / 2;
        }
        /* log_info("step_size:%d %d %d",step_size,remain_size,qr_code.out_size * jl_code_param.l_size); */
        for (int i = 0; i < qr_code.out_size * jl_code_param.l_size; i++) {
            for (int j = 0; j < qr_code.out_size * jl_code_param.l_size; j++) {
                /* if (img[i][j] == 0xff) { */
                if (copy_row_img[i * qr_code.out_size * jl_code_param.l_size + j] == 0xff) {
                    qr_fill_rect(&dc, rect.left + 75 + remain_size + step_size * j, rect.top + 75 + remain_size + step_size * i, step_size, step_size, 0xffff);
                }
                /* if (img[i][j] == 0x00) { */
                if (copy_row_img[i * qr_code.out_size * jl_code_param.l_size + j] == 0x00) {
                    qr_fill_rect(&dc, rect.left + 75 + remain_size + step_size * j, rect.top + 75 + remain_size + step_size * i, step_size, step_size, 0x0000);
                }
            }
        }
    }
}

static int layout_qr_code_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct element *elm = (struct element *)_ctrl;
    struct draw_context *dc = (struct draw_context *)arg;
    struct rect rect = {0};
    u8 step_size;
    u8 remain_size;

    switch (event) {
    case ON_CHANGE_INIT:
        jl_qr_code_process();
        break;
    case ON_CHANGE_SHOW:
        /*根据需要调用以下三个接口:ui_remove_backcolor,ui_remove_backimage,ui_remove_border*/
        /* ui_remove_backcolor(elm);//移除控件背景颜色 */
        /* ui_remove_backimage(elm);//移除控件背景图像 */
        /* ui_remove_border(elm);//移除控件边界 */
        break;
    case ON_CHANGE_SHOW_POST:
        ui_core_get_element_abs_rect(elm, &rect); //跟随控件移动,注释掉这句则不跟随控件移动
        ui_draw(dc, NULL, rect.left, rect.top, rect.width, rect.height, ui_draw_user_guide, NULL, 0, 2);
        break;
    case ON_CHANGE_RELEASE:
        if (row_img) {
            free(row_img);
            row_img = 0;
        }
        if (copy_row_img) {
            free(copy_row_img);
            copy_row_img = 0;
        }
        log_info("qr_code release:");
        mem_stats();
        break;
    }
    return false;
}
/* REGISTER_UI_EVENT_HANDLER(LAYOUT_QR_CODE) */
REGISTER_UI_EVENT_HANDLER(QR_CODE_LAYOUT)
.onchange = layout_qr_code_onchange,
 .onkey = NULL,
  .ontouch = NULL,
};



static int layout_user_guide_onchange(void *_ctrl, enum element_change_event event, void *arg)
{
    struct element *elm = (struct element *)_ctrl;
    struct draw_context *dc = (struct draw_context *)arg;
    struct rect rect = {0};

    switch (event) {
    case ON_CHANGE_INIT:
        jl_qr_code_process();
        break;
    case ON_CHANGE_SHOW:
        /*根据需要调用以下三个接口:ui_remove_backcolor,ui_remove_backimage,ui_remove_border*/
        /* ui_remove_backcolor(elm);//移除控件背景颜色 */
        /* ui_remove_backimage(elm);//移除控件背景图像 */
        /* ui_remove_border(elm);//移除控件边界 */
        break;
    case ON_CHANGE_SHOW_POST:
        ui_core_get_element_abs_rect(elm, &rect); //跟随控件移动,注释掉这句则不跟随控件移动
        ui_draw(dc, NULL, rect.left, rect.top, rect.width, rect.height, ui_draw_user_guide, NULL, 0, 1);
        break;

    case ON_CHANGE_RELEASE:
        if (row_img) {
            free(row_img);
            row_img = 0;
        }
        if (copy_row_img) {
            free(copy_row_img);
            copy_row_img = 0;
        }
        log_info("qr_code release:");
        mem_stats();
        break;
    }
    return false;
}
static int layout_user_guide_ontouch(void *_ctrl, struct element_touch_event *e)
{
    switch (e->event) {
    case ELM_EVENT_TOUCH_R_MOVE:
        UI_HIDE_CURR_WINDOW();
        UI_SHOW_WINDOW(ID_WINDOW_SET);
        break;
    case ELM_EVENT_TOUCH_DOWN:
        return true;
    case ELM_EVENT_TOUCH_HOLD:
        break;
    case ELM_EVENT_TOUCH_MOVE:
        break;
    case ELM_EVENT_TOUCH_UP:
        break;
    }
    return false;
}
REGISTER_UI_EVENT_HANDLER(LAYOUT_USER_GUIDE)//使用指南
.onchange = layout_user_guide_onchange,
 .onkey = NULL,
  .ontouch = layout_user_guide_ontouch,
};

#endif
#endif
/* #endif #<{(| #if (!TCFG_LUA_ENABLE) |)}># */

void bt_addr2string(u8 *addr, u8 *buf)
{
    u8 len = 0;
    for (s8 i = 5; i >= 0; i--) {
        if ((addr[i] / 16) >= 10) {
            buf[len] = 'A' + addr[i] / 16 - 10;
        } else {
            buf[len] = '0' + addr[i] / 16;
        }
        if ((addr[i] % 16) >= 10) {
            buf[len + 1] = 'A' + addr[i] % 16 - 10;
        } else {
            buf[len + 1] = '0' + addr[i] % 16;
        }
        len += 2;
        buf[len] = ':';
        len += 1;
    }
    buf[len - 1] = '\0';
    log_info("%s", buf);
}

