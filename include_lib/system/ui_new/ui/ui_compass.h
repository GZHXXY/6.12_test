#ifndef UI_COMPASS_H
#define UI_COMPASS_H


#include "ui/control.h"
#include "ui/ui_core.h"


#define COMPASS_CHILD_NUM     (CTRL_COMPASS_CHILD_END - CTRL_COMPASS_CHILD_BEGIN)


struct compass_pic_info {
    struct ui_ctrl_info_head head;
    u16 cent_x;
    u16 cent_y;
    struct ui_image_list *img;
};

struct compass_css_info {
    int left: 16;
    int top: 16;
    int width: 16;
    int height: 16;
};

struct ui_compass {
    struct element elm;
    struct element child_elm[COMPASS_CHILD_NUM];
    struct compass_css_info child_css[COMPASS_CHILD_NUM];
    char source[8];
    int bk_angle : 16;
    int indicator_angle : 16;
    int last_bk_angle : 16;
    int last_indicator_angle : 16;
    u8 updata;
    u8 ctrl_num;
    void *timer;
    const struct layout_info *info;
    const struct compass_pic_info *pic_info[COMPASS_CHILD_NUM];
    const struct element_event_handler *handler;
    const struct element_luascript_t *lua;
};

void ui_compass_enable();
void ui_compass_update(struct ui_compass *compass, u8 refresh);
int ui_compass_set_angle_by_id(int id, int bk_angle, int indicator_angle);
int ui_compass_set_angle(struct ui_compass *compass, int bk_angle, int indicator_angle);

#endif


