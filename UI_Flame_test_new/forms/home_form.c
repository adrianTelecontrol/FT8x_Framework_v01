
#include <stdlib.h>


#include "gfx.h"
#include "FT8xx_params.h"
#include "form_common.h"
#include "EVE_colors.h"
#include "gfx_canvas.h"
#include "forms_manager.h"

#include "home_form.h"

static gfx_Canvas g_sHomeCanvas;

gfx_GenericWidget panelWidget;
gfx_GenericWidget buttonWidget;
gfx_GenericWidget secondWidget;
gfx_GenericWidget thirdWidget;

void pushButtonOnClicked(gfx_Button *btn)
{
    if(btn->state == BTN_STATE_PRESSED)
        btn->state = BTN_STATE_NORMAL;
    else
    {
        btn->state = BTN_STATE_PRESSED;
        // btn->pos.x = LCD_WIDTH / 2 - btn->dim.width / 2;
        // btn->pos.y = LCD_HEIGHT / 2 - btn->dim.height / 2;
        // gfx_initRegTouch((void *)btn, WD_TYPE_BUTTON);
    }
	btn->bIsDirty = true;

}

void pushButtonOnPosChanged(gfx_Button *btn, Position newPos)
{
    if(newPos.x >= 0 && newPos.x < LCD_WIDTH - btn->size.width && newPos.y >= 0 &&newPos.y < LCD_HEIGHT - btn->size.height)
    {
		btn->oldPos = btn->pos;

        btn->pos.x = newPos.x;
        btn->pos.y = newPos.y;
        gfx_initRegTouch((void *)btn, WD_TYPE_BUTTON);
	    
		btn->bIsDirty = true;
    }

    //System_printf("Button pos: (%d, %d)", btn->pos.x, btn->pos.y);
}

void initHomeForm(void)
{
    g_sHomeCanvas.ui16BackgroundColor = EVE_WHITE;
    g_sHomeCanvas.psWidgets = NULL;

    panelWidget.eWidgetType = WD_TYPE_RECT;
    panelWidget.pvWidget = (void *)&(gfx_Rectangle){
        .name = "bgPanel",
        .pos.x = HORIZONTAL_MARGIN,
        .pos.y = VERTICAL_MARGIN,
        .dim.width = LCD_WIDTH - 2 * HORIZONTAL_MARGIN,
        .dim.height = LCD_HEIGHT - 2 * VERTICAL_MARGIN,
        .round = 0,
        .color = EVE_BLUE,
    };

    buttonWidget.eWidgetType = WD_TYPE_BUTTON;
    buttonWidget.pvWidget = (void *)&(gfx_Button){
        .name = "btn1",
        .label = "PUSH ME",
		.fontScale = 1,
		.font = FONT_ROBOTO,
        .size.width = BUTTON_WIDTH,
        .size.height = BUTTON_HEIGHT,
        .pos.x = LCD_WIDTH / 2 - BUTTON_WIDTH / 2,
        .pos.y = LCD_HEIGHT / 2 - BUTTON_HEIGHT / 2,        
        .oldPos.x = LCD_WIDTH / 2 - BUTTON_WIDTH / 2,
        .oldPos.y = LCD_HEIGHT / 2 - BUTTON_HEIGHT / 2,        
        .backgroundColor = EVE_GREEN,
        .textColor = EVE_WHITE,
		.borderWidth = 1,
        .onPosChanged = pushButtonOnPosChanged,
        .onClicked = pushButtonOnClicked,
		.radius = 10,
    };  
    gfx_initRegTouch(buttonWidget.pvWidget, WD_TYPE_BUTTON);

    secondWidget.eWidgetType = WD_TYPE_BUTTON;
    secondWidget.pvWidget = (void *)&(gfx_Button){
        .name = "btn2",
        .size.height = 100,
        .size.width = 100,
        .pos.x = LCD_WIDTH - HORIZONTAL_MARGIN - BUTTON_WIDTH,
        .pos.y = LCD_HEIGHT / 2 - BUTTON_HEIGHT / 2,
        .oldPos.x = LCD_WIDTH - HORIZONTAL_MARGIN - BUTTON_WIDTH,
        .oldPos.y = LCD_HEIGHT / 2 - BUTTON_HEIGHT / 2,
        .label = "TOUCH",
		.fontScale = 1,
        .backgroundColor = EVE_PURPLE,
        .textColor = EVE_WHITE,                                               
		.borderWidth = 1,
        .onPosChanged = pushButtonOnPosChanged,
        .onClicked = pushButtonOnClicked,
		.radius = 10,
    };
    gfx_initRegTouch(secondWidget.pvWidget, WD_TYPE_BUTTON); 

	thirdWidget.eWidgetType = WD_TYPE_LABEL;
	thirdWidget.pvWidget = (void *)&(gfx_Label){
		.text = "Titulo",
		.pos.x = LCD_WIDTH / 2,
		.pos.y = 50,
		.scale = 2,
		.font = FONT_BEBAS,
		.alignment = ALIGN_CENTER,
		.textColor = EVE_GREEN_APPLE,
	};

    canvasInsertAtTop(&g_sHomeCanvas.psWidgets, &panelWidget);
    canvasInsertAtTop(&g_sHomeCanvas.psWidgets, &buttonWidget);
    canvasInsertAtTop(&g_sHomeCanvas.psWidgets, &secondWidget);
	canvasInsertAtTop(&g_sHomeCanvas.psWidgets, &thirdWidget);

    // Add reference to formManager
	formManagerLoadForm(&g_sHomeCanvas);
}

