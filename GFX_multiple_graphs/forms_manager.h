#ifndef FORM_MANAGER_H
#define FORM_MANAGER_H

#include "gfx.h"
#include "gesture_engine.h"

#define MAX_FORM_NUMBER		5

typedef struct {
	gfx_Canvas *canvas;
	char name[10];
} Form_Entry_t;

extern gfx_Canvas *g_psCurrentForm;

static void formManagerOnFormChange(uint32_t arg);

int16_t formManagerAddForm(gfx_Canvas *form);

bool formManagerHandleGesture(TouchStatus touchStatus, gesture_type_e gesture);

void formManagerDirtyRender();

void formManagerComposite(pixel16_t *psPixelBuffer);

void formManagerInit(void);

#endif


