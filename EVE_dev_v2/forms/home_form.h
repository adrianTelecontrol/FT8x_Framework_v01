#ifndef HOME_FORM_H_
#define HOME_FORM_H_

#include <stdint.h>
#include <stdbool.h>

#include "touchscreen_task.h"

void initHomeForm(void);

void renderHomeForm(void);

bool handleHomeFormTouch(TouchStatus, gesture_type_e);

#endif // HOME_FORM_H


