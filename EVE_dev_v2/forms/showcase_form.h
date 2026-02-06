#ifndef SHOWCASE_FORM_H
#define SHOWCASE_FORM_H

#include <stdint.h>
#include <stdbool.h>

#include "touchscreen_task.h"

void initShowcaseForm(void);

void renderShowcaseForm(void);

bool handleShowcaseFormTouch(TouchStatus, gesture_type_e);

#endif //SHOWCASE_FORM_H


