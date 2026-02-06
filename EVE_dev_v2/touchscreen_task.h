#ifndef TOUCHSCREEN_TASK_H
#define TOUCHSCREEN_TASK_H

#include <xdc/std.h>

typedef enum
{
    GESTURE_EMPTY,
    GESTURE_CLICK,
    GESTURE_DRAG,
    GESTURE_LOCK_OBJ,
    GESTURE_RELEASE,
} gesture_type_e;

void touchScreenTaskStart(void);
//Void touchScreenTask(UArg arg0, UArg arg1);

#endif // TOUCHSCREEN_TASK_H

