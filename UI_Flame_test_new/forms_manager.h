#ifndef FORM_MANAGER_H
#define FORM_MANAGER_H

#include "gfx.h"
#include "gesture_engine.h"

#define MAX_FORM_NUMBER		5

extern gfx_Canvas *g_psCurrentForm;

void formManagerLoadForm(gfx_Canvas *form);

bool formManagerHandleGesture(TouchStatus touchStatus, gesture_type_e gesture);

void formManagerDirtyRender();

void formManagerComposite(pixel16_t *psPixelBuffer);

#endif