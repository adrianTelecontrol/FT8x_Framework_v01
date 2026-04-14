
#include <stdlib.h>
#include <string.h>

#include "event_engine.h"
#include "helpers.h"
#include "FT8xx_params.h"
#include "gfx.h"
#include "gfx_canvas.h"
#include "gfx_colors.h"
#include "font_engine.h"
#include "gfx_theme.h"
#include "forms_manager.h"

#include "navigation_widgets.h"

gfx_GenericWidget btnPrevContainer;
gfx_GenericWidget btnNextContainer;

gfx_Button btnPrevWidget;
gfx_Button btnNextWidget;

void onPushButtonPressed(gfx_Button *btn)
{
    btn->state = BTN_STATE_PRESSED;
    btn->bIsDirty = true;
}

void onPushButtonRelease(gfx_Button *btn)
{
    btn->state = BTN_STATE_NORMAL;
    btn->bIsDirty = true;
}

void onPrevButtonRelease(gfx_Button *btn) {
	onPushButtonRelease(btn);

	Event_Post(EVT_SYS_PREV_FORM, 0);
}

void onNextButtonRelease(gfx_Button *btn) {
	onPushButtonRelease(btn);

	Event_Post(EVT_SYS_NEXT_FORM, 0);
}

void initNavigationWidgets(void) {
	
	btnPrevWidget = (gfx_Button) {
		.label = "<",
		.size.width = 70,
		.size.height = 50,
		.pos.x = 20,
		.pos.y = 10,
		.radius = 10,
		.name = "prevBtn",
		.borderWidth = 0,
		.state = BTN_STATE_NORMAL,
		.style = STYLE_PRIMARY,
		.typo = TYPO_BODY,
		.onPressed = onPushButtonPressed,
		.onRelease = onPrevButtonRelease,
	};
	btnPrevContainer.eWidgetType = WD_TYPE_BUTTON;
	btnPrevContainer.pvWidget = (void *)&btnPrevWidget;
	gfx_initRegTouch(btnPrevContainer.pvWidget, WD_TYPE_BUTTON);

	btnNextWidget = (gfx_Button) {
		.label = ">",
		.size.width = 70,
		.size.height = 50,
		.pos.x = 100,
		.pos.y = 10,
		.radius = 10,
		.name = "nextBtn",
		.borderWidth = 0,
		.state = BTN_STATE_NORMAL,
		.style = STYLE_PRIMARY,
		.typo = TYPO_BODY,
		.onPressed = onPushButtonPressed,
		.onRelease = onNextButtonRelease,
	};
	btnNextContainer.eWidgetType = WD_TYPE_BUTTON;
	btnNextContainer.pvWidget = (void *)&btnNextWidget;
	gfx_initRegTouch(btnNextContainer.pvWidget, WD_TYPE_BUTTON);
}

void useNavigationButtons(gfx_Canvas* canvas) {
	if(canvas == NULL) return;

	canvasInsertAtTop(&canvas->psWidgets, &btnPrevContainer);
	canvasInsertAtTop(&canvas->psWidgets, &btnNextContainer);
}



