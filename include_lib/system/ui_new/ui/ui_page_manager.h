#ifndef __UI_PAGE_H__
#define __UI_PAGE_H__

#include "generic/list.h"
#include "system/spinlock.h"

struct ui_page {
    struct list_head head;
    u32 id;
    u8 num;
};

struct ui_page_head {
    struct list_head head;
    u8 ui_page_list_total_num;
    spinlock_t ui_page_list_lock;
};

void ui_page_init();
int ui_page_add(u32 id);
int ui_page_del(u32 id);
int ui_page_del_by_num(u8 num);
int ui_page_free();
u32 ui_page_next(u32 id);
u32 ui_page_prev(u32 id);
int ui_page_list_all();
struct ui_page *ui_page_search(u32 id);

int ui_page_has_been_init();
struct ui_page *ui_page_get_first();
int ui_page_list_id_modify(u8 num, u32 id);

#endif

