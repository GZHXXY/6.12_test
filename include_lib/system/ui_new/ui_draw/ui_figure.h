#ifndef UI_FIGURE_H
#define UI_FIGURE_H

#define DRAW_LINE           0x1
#define DRAW_LINE_BY_ANGLE  0x2
#define DRAW_RECT           0x3
#define FILL_RECT           0x4
#define DRAW_CIRCLE         0x5
#define DRAW_RING           0x6
#define DRAW_RING_CPU       0x7

struct _line {
    int x_begin;
    int y_begin;
    int x_end;
    int y_end;
    int length;
    int angle;
    int color;
};

struct _rect {
    struct rect rectangle;
    int color;
};

void draw_line(void *_dc, struct _line *line_info);
void draw_line_by_angle(void *_dc, struct _line *line_info);
void draw_triangle(void *_dc, struct _line *line_info1, struct _line *line_info2, struct _line *line_info3);
void draw_rect(void *_dc, struct rect *rectangle, u16 color);
void draw_circle_or_arc(void *_dc, struct circle_info *info);
void ui_draw_default(int id, u8 *dst_buf, struct rect *dst_r, struct rect *src_r, u8 bytes_per_pixel, void *priv);

#endif

