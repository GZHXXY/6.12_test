#ifndef LAYOUT_H
#define LAYOUT_H


#include "ui/ui_core.h"
#include "ui/control.h"





struct layout {
    struct element elm; 	//must be first
    u8 hide: 1;
    u8 inited: 1;
    u8 release: 6;
    // u8 css_num:5;
    // u32 css[2];
    u8 page;

    u8 movable;	// 移动标志
    u8 star_menu;	// 满天星标志
    struct list_head *icon_root;
    int lcd_w;
    int lcd_h;

    struct element_luascript_t *lua;
    struct layout *layout;
    const struct layout_info *info;
    const struct element_event_handler *handler;
};



#define layout_for_id(id) \
		(struct layout *)ui_core_get_element_by_id(id);


struct layout *layout_new(struct layout_info *, int, struct element *);

void layout_delete_probe(struct layout *layout, int num);

void layout_delete(struct layout *layout, int num);

int layout_show(int id);

int layout_hide(int id);

int layout_toggle(int id);

void layout_on_focus(struct layout *layout);
void layout_lose_focus(struct layout *layout);

int custom_draw(void *_layout);

/*int layout_current_highlight(int id);*/

/*int layout_onkey(struct layout *layout, struct element_key_event *e);*/



#endif


