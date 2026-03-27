
#include <stdlib.h>


#include "gfx.h"
#include "FT8xx_params.h"
#include "form_common.h"
#include "EVE_colors.h"
#include "gfx_canvas.h"
#include "forms_manager.h"
#include "gfx_theme.h"

#include "home_form.h"

static gfx_Canvas g_sHomeCanvas;

gfx_GenericWidget panelWidget;
gfx_GenericWidget buttonWidget;
gfx_GenericWidget secondWidget;
gfx_GenericWidget thirdWidget;

void pushButtonOnPressed(gfx_Button *btn)
{
    btn->state = BTN_STATE_PRESSED;
	btn->bIsDirty = true;
}

void pushButtonOnRelease(gfx_Button *btn)
{
    btn->state = BTN_STATE_NORMAL;
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
	    btn->state = BTN_STATE_PRESSED;
		
		btn->bIsDirty = true;
    }

    //System_printf("Button pos: (%d, %d)", btn->pos.x, btn->pos.y);
}

void initHomeForm(void)
{
    // Ensure Theme_Init() was called in your main setup before reaching here!

    g_sHomeCanvas.ui16BackgroundColor = g_pCurrentTheme->palette.background;
    g_sHomeCanvas.psWidgets = NULL;

    panelWidget.eWidgetType = WD_TYPE_RECT;
    panelWidget.pvWidget = (void *)&(gfx_Rectangle){
        .name = "bgPanel",
        .pos.x = HORIZONTAL_MARGIN,
        .pos.y = VERTICAL_MARGIN,
        .dim.width = LCD_WIDTH - 2 * HORIZONTAL_MARGIN,
        .dim.height = LCD_HEIGHT - 2 * VERTICAL_MARGIN,
        .round = 0,
        .color = g_pCurrentTheme->palette.surface, // Semantic Panel Color
    };

    buttonWidget.eWidgetType = WD_TYPE_BUTTON;
    buttonWidget.pvWidget = (void *)&(gfx_Button){
        .name = "btn1",
        .label = "Push Me",
        .size.height = 100,
        .size.width = 100,
        
        // --- NEW THEME ARCHITECTURE ---
        .typo = TYPO_BODY,         // Connects to g_pCurrentTheme->fonts.body
        .style = STYLE_PRIMARY,    // Connects to g_pCurrentTheme->palette.primary
        // ------------------------------
        
        .pos.x = LCD_WIDTH / 2 - BUTTON_WIDTH / 2,
        .pos.y = LCD_HEIGHT / 2 - BUTTON_HEIGHT / 2,
        .oldPos.x = LCD_WIDTH - HORIZONTAL_MARGIN - BUTTON_WIDTH,
        .oldPos.y = LCD_HEIGHT / 2 - BUTTON_HEIGHT / 2,
        .borderWidth = 1,
        .state = BTN_STATE_NORMAL,
        .onPosChanged = pushButtonOnPosChanged,
        .onPressed = pushButtonOnPressed,
        .onRelease = pushButtonOnRelease,
        .radius = 10,
    };
    gfx_initRegTouch(buttonWidget.pvWidget, WD_TYPE_BUTTON);

    secondWidget.eWidgetType = WD_TYPE_BUTTON;
    secondWidget.pvWidget = (void *)&(gfx_Button){
        .name = "btn2",
        .label = "TOUCH",
        .size.height = 100,
        .size.width = 100,
        
        // --- NEW THEME ARCHITECTURE ---
        .typo = TYPO_BODY,         // Connects to g_pCurrentTheme->fonts.body
        .style = STYLE_SECONDARY,  // Connects to g_pCurrentTheme->palette.secondary
        // ------------------------------
        
        .pos.x = LCD_WIDTH * (2 / 3),
        .pos.y = LCD_HEIGHT / 2 - BUTTON_HEIGHT / 2,
        .oldPos.x = LCD_WIDTH * (2 / 3),
        .oldPos.y = LCD_HEIGHT / 2 - BUTTON_HEIGHT / 2,
        .borderWidth = 1,
        .state = BTN_STATE_NORMAL,
        .onPosChanged = pushButtonOnPosChanged,
        .onPressed = pushButtonOnPressed,
        .onRelease = pushButtonOnRelease,
        .radius = 10,
    };
    gfx_initRegTouch(secondWidget.pvWidget, WD_TYPE_BUTTON); 

    thirdWidget.eWidgetType = WD_TYPE_LABEL;
    thirdWidget.pvWidget = (void *)&(gfx_Label){
        .text = "Titulo de app",
        .pos.x = LCD_WIDTH / 2,
        .pos.y = 50,
        .alignment = ALIGN_CENTER,
        
        // --- NEW THEME ARCHITECTURE ---
        .typo = TYPO_H1,           // Connects to g_pCurrentTheme->fonts.h1
        .style = STYLE_DANGER,     // Connects to g_pCurrentTheme->palette.danger (Replaces EVE_RED)
        // ------------------------------
    };

    // canvasInsertAtTop(&g_sHomeCanvas.psWidgets, &panelWidget);
    canvasInsertAtTop(&g_sHomeCanvas.psWidgets, &buttonWidget);
    canvasInsertAtTop(&g_sHomeCanvas.psWidgets, &secondWidget);
    canvasInsertAtTop(&g_sHomeCanvas.psWidgets, &thirdWidget);

    // Add reference to formManager
    formManagerLoadForm(&g_sHomeCanvas);
}

