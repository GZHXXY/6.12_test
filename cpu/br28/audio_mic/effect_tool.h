#ifndef __EFFECT_TOOL_H__
#define __EFFECT_TOOL_H__

#include "generic/typedef.h"
#include "effect_cfg.h"

int effect_tool_open(struct __effect_mode_cfg *parm, struct __tool_callback *cb);
void effect_tool_close(void);

#define EFFECTS_SAVE_TO_FLASH  1   //混响在线效果保存到flash 1:使能， 0：关闭
#endif//__EFFECT_TOOL_H__
