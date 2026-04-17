#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "gfx.h"
#include "FT8xx_params.h"
#include "form_common.h"
#include "EVE_colors.h"
#include "gfx_canvas.h"
#include "forms_manager.h"

#include "helpers.h"
#include "gfx_theme.h"
#include "event_engine.h"
#include "navigation_widgets.h"

#include "home_form.h"

static gfx_Canvas g_sHomeCanvas;

int16_t g_i16HomeFormID = 0;

// 1. The Generic Nodes
gfx_GenericWidget panelWidget;
gfx_GenericWidget buttonWidget;
gfx_GenericWidget secondWidget;
gfx_GenericWidget thirdWidget;
gfx_GenericWidget changeThemeButton;
gfx_GenericWidget counterLabel;
gfx_GenericWidget counterLabel2; 
gfx_GenericWidget sliderWidget;
gfx_GenericWidget sliderValueWidget;
gfx_GenericWidget vertSliderWidget;
gfx_GenericWidget vertSliderValueWidget;

// 2. NEW: The Persistent Memory for the actual widget data
gfx_Rectangle panelData;
gfx_Button btn1Data;
gfx_Button btn2Data;
gfx_Button themeBtnData;
gfx_Label titleData;
gfx_Label counterData;
gfx_Label counterData2;
gfx_Slider sliderData;
gfx_Label sliderValueData;
gfx_Slider vertSliderData;
gfx_Label vertSliderValueData;

// 3. NEW: The static text buffer to replace malloc
static char counterTextBuffer[16];
static char counter2TextBuffer[16];
static char sliderValTextBuffer[16];
static char vertSliderValTextBuffer[16];

// --- Callbacks ---

static void onCounterUpdated(int32_t arg) // Using int32_t assuming your Event payload is int32_t
{
    gfx_Label *lb = (gfx_Label *)counterLabel.pvWidget;
    
    // Safely format the dynamic argument into the persistent static buffer
    sprintf(lb->text, "%u", arg); 
    
    lb->bIsDirty = true;
} 

static void onSliderValueUpdated(gfx_Slider *sld, int16_t arg) // Using int32_t assuming your Event payload is int32_t
{
    gfx_Label *lb = (gfx_Label *)sliderValueWidget.pvWidget;
    
    // Safely format the dynamic argument into the persistent static buffer
    sprintf(lb->text, "Slider: %u", arg); 
    
    lb->bIsDirty = true;
} 

static void onVertSliderValueUpdated(gfx_Slider *sld, int16_t arg) // Using int32_t assuming your Event payload is int32_t
{
    gfx_Label *lb = (gfx_Label *)vertSliderValueWidget.pvWidget;
    
    // Safely format the dynamic argument into the persistent static buffer
    sprintf(lb->text, "VertSlider: %u", arg); 
    
    lb->bIsDirty = true;
} 

static void onCounter2Updated(int32_t arg) // Using int32_t assuming your Event payload is int32_t
{
    gfx_Label *lb = (gfx_Label *)counterLabel2.pvWidget;
    
    // Safely format the dynamic argument into the persistent static buffer
    sprintf(lb->text, "%u", arg); 
    
    lb->bIsDirty = true;
} 

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

void changeThemeButtonReleased(gfx_Button *btn)
{
    pushButtonOnRelease(btn);
    Event_Post(EVT_CMD_CHANGE_THEME, 0);    
}

void pushButtonOnPosChanged(gfx_Button *btn, Position newPos)
{
    if(newPos.x >= 0 && newPos.x < LCD_WIDTH - btn->size.width && newPos.y >= 0 && newPos.y < LCD_HEIGHT - btn->size.height)
    {
        btn->oldPos = btn->pos;

        btn->pos.x = newPos.x;
        btn->pos.y = newPos.y;
        gfx_initRegTouch((void *)btn, WD_TYPE_BUTTON);
        btn->state = BTN_STATE_PRESSED;
        
        btn->bIsDirty = true;
    }
}


// --- Initialization ---

void initHomeForm(void)
{
    // Ensure Theme_Init() was called in your main setup before reaching here!

    g_sHomeCanvas.ui16BackgroundColor = g_pCurrentTheme->palette.background;
    g_sHomeCanvas.psWidgets = NULL;

    // --- PANEL ---
    panelData = (gfx_Rectangle){
        .name = "bgPanel",
        .pos.x = HORIZONTAL_MARGIN,
        .pos.y = VERTICAL_MARGIN,
        .dim.width = LCD_WIDTH - 2 * HORIZONTAL_MARGIN,
        .dim.height = LCD_HEIGHT - 2 * VERTICAL_MARGIN,
        .round = 0,
        .color = g_pCurrentTheme->palette.surface, 
    };
    panelWidget.eWidgetType = WD_TYPE_RECT;
    panelWidget.pvWidget = (void *)&panelData; // Link to static memory

    // --- BUTTON 1 ---
    btn1Data = (gfx_Button){
        .name = "btn1",
        .label = "Push Me",
        .size.height = 100,
        .size.width = 100,
        .typo = TYPO_BODY,         
        .style = STYLE_PRIMARY,    
        .pos.x = 200,
        .pos.y = LCD_HEIGHT / 2 - BUTTON_HEIGHT / 2,
        .oldPos.x = LCD_WIDTH / 2 - BUTTON_WIDTH / 2,
        .oldPos.y = LCD_HEIGHT / 2 - BUTTON_HEIGHT / 2,
        .borderWidth = 1,
        .state = BTN_STATE_NORMAL,
        .onPosChanged = pushButtonOnPosChanged,
        .onPressed = pushButtonOnPressed,
        .onRelease = pushButtonOnRelease,
        .radius = 10,
    };
    buttonWidget.eWidgetType = WD_TYPE_BUTTON;
    buttonWidget.pvWidget = (void *)&btn1Data;
    gfx_initRegTouch(buttonWidget.pvWidget, WD_TYPE_BUTTON);

    // --- BUTTON 2 ---
    btn2Data = (gfx_Button){
        .name = "btn2",
        .label = "TOUCH",
        .size.height = 100,
        .size.width = 100,
        .typo = TYPO_BODY,         
        .style = STYLE_SECONDARY,  
        .pos.x = (LCD_WIDTH * 2) / 3, // FIX: Avoided integer division trap (2/3 = 0)
        .pos.y = LCD_HEIGHT / 2 - BUTTON_HEIGHT / 2,
        .oldPos.x = (LCD_WIDTH * 2) / 3,
        .oldPos.y = LCD_HEIGHT / 2 - BUTTON_HEIGHT / 2,
        .borderWidth = 1,
        .state = BTN_STATE_NORMAL,
        .onPosChanged = NULL,
        .onPressed = pushButtonOnPressed,
        .onRelease = pushButtonOnRelease,
        .radius = 10,
    };
    secondWidget.eWidgetType = WD_TYPE_BUTTON;
    secondWidget.pvWidget = (void *)&btn2Data;
    gfx_initRegTouch(secondWidget.pvWidget, WD_TYPE_BUTTON); 

    // --- THEME BUTTON ---
    themeBtnData = (gfx_Button){
        .name = "btnTheme",
        .label = "TEMA",
        .size.height = 106,
        .size.width = 125,
        .typo = TYPO_BODY,         
        .style = STYLE_SUCCESS,  
        .pos.x = LCD_WIDTH / 2 - 50,
        .pos.y = LCD_HEIGHT - 300,
        .oldPos.x = LCD_WIDTH / 2 - 50,
        .oldPos.y = LCD_HEIGHT - 150,
        .borderWidth = 1,
        .state = BTN_STATE_NORMAL,
        .onPosChanged = NULL,
        .onPressed = pushButtonOnPressed,
        .onRelease = changeThemeButtonReleased,
        .radius = 10,
    };
    changeThemeButton.eWidgetType = WD_TYPE_BUTTON;
    changeThemeButton.pvWidget = (void *)&themeBtnData;
    gfx_initRegTouch(changeThemeButton.pvWidget, WD_TYPE_BUTTON); 

    // --- TITLE LABEL ---
    titleData = (gfx_Label){
        .text = "Titulo de app",
		.name = "titulo",
        .pos.x = LCD_WIDTH / 2,
        .pos.y = 50,
        .oldPos.x = LCD_WIDTH / 2,
        .oldPos.y = 50,
        .alignment = ALIGN_CENTER,
        .typo = TYPO_H1,           
        .style = STYLE_DANGER,     
    };
    thirdWidget.eWidgetType = WD_TYPE_LABEL;
    thirdWidget.pvWidget = (void *)&titleData;

    // --- COUNTER LABEL ---
    counterData = (gfx_Label){
        .text = counterTextBuffer, // Point directly to our static array
		.name = "contador",
        .pos.x = LCD_WIDTH * 2.0 / 3.0,	
        .pos.y = 100,                   
        .oldPos.x = LCD_WIDTH / 2,       
        .oldPos.y = 90,
        .alignment = ALIGN_CENTER,
        .typo = TYPO_H2,           
        .style = STYLE_SUCCESS,    
    };
	
    counterLabel.eWidgetType = WD_TYPE_LABEL;
    counterLabel.pvWidget = (void *)&counterData;

    counterData2 = (gfx_Label){
        .text = counter2TextBuffer, // Point directly to our static array
		.name = "contador2",
        .pos.x = LCD_WIDTH * 1.0 / 3.0,
        .pos.y = 100,
        .oldPos.x = LCD_WIDTH / 2,
        .oldPos.y = 90,
        .alignment = ALIGN_CENTER,
        .typo = TYPO_H2,           
        .style = STYLE_SUCCESS,    
    };
	
    counterLabel2.eWidgetType = WD_TYPE_LABEL;
    counterLabel2.pvWidget = (void *)&counterData2;

	sliderData = (gfx_Slider){
		.knobRadius = 25,
		.maxValue = 100,
		.minValue = 0,
		.name = "slider1",
		.size.height = 30,
		.size.width = 500,
		.pos.x = LCD_WIDTH / 2 - 250,
		.pos.y = LCD_HEIGHT - 50,
		.trackHeight = 50,
		.style = STYLE_PRIMARY,
		.onValueChanged = onSliderValueUpdated,
		.bIsVertical = false,
		.bShowKnob = false,
		.currentValue = 0,
	};
	sliderWidget.eWidgetType = WD_TYPE_SLIDER;
	sliderWidget.pvWidget = (void *)&sliderData;
	gfx_initRegTouch(sliderWidget.pvWidget, WD_TYPE_SLIDER);

    sliderValueData = (gfx_Label) {
        .text = sliderValTextBuffer, // Point directly to our static array
		.name = "sliderValue",
		.pos.x = LCD_WIDTH / 2 - 250,
		.pos.y = LCD_HEIGHT - 100,
        .oldPos.x = LCD_WIDTH / 2,
        .oldPos.y = 90,
        .alignment = ALIGN_CENTER,
        .typo = TYPO_H2,           
        .style = STYLE_SECONDARY,    
    };
	sprintf(sliderValTextBuffer, "Slider: %u", 0);
    sliderValueWidget.eWidgetType = WD_TYPE_LABEL;
    sliderValueWidget.pvWidget = (void *)&sliderValueData;

	vertSliderData = (gfx_Slider){
		.knobRadius = 25,
		.maxValue = 100,
		.minValue = 0,
		.name = "slider1",
		.size.height = 300,
		.size.width = 30,
		.pos.x = LCD_WIDTH - 70,
		.pos.y = LCD_HEIGHT * 1.0 / 3.0,
		.trackHeight = 50,
		.style = STYLE_SECONDARY,
		.onValueChanged = onVertSliderValueUpdated,
		.bIsVertical = true,
		.bShowKnob = false,
		.currentValue = 0,
	};
	vertSliderWidget.eWidgetType = WD_TYPE_SLIDER;
	vertSliderWidget.pvWidget = (void *)&vertSliderData;
	gfx_initRegTouch(vertSliderWidget.pvWidget, WD_TYPE_SLIDER);

    vertSliderValueData = (gfx_Label) {
        .text = vertSliderValTextBuffer, // Point directly to our static array
		.name = "vertSliderValue",
		.pos.x = LCD_WIDTH - 70,
		.pos.y = 100,
        .oldPos.x = LCD_WIDTH / 2,
        .oldPos.y = 90,
        .alignment = ALIGN_CENTER,
        .typo = TYPO_CAPTION,           
        .style = STYLE_SECONDARY,    
    };
	sprintf(vertSliderValTextBuffer, "VertSlider: %u", 0);
    vertSliderValueWidget.eWidgetType = WD_TYPE_LABEL;
    vertSliderValueWidget.pvWidget = (void *)&vertSliderValueData;
	
    // Subscribe to events
    Event_Subscribe(EVT_SYS_COUNTER_CHANGED, (EventHandler_fn)onCounterUpdated);
    Event_Subscribe(EVT_SYS_COUNTER2_CHANGED, (EventHandler_fn)onCounter2Updated);

    // Initial render setup
    memset(counterTextBuffer, 0, sizeof(counterTextBuffer));
    sprintf(counterData.text, "%u", 0);

    // canvasInsertAtTop(&g_sHomeCanvas.psWidgets, &panelWidget);
    canvasInsertAtTop(&g_sHomeCanvas.psWidgets, &buttonWidget);
    canvasInsertAtTop(&g_sHomeCanvas.psWidgets, &secondWidget);
    canvasInsertAtTop(&g_sHomeCanvas.psWidgets, &counterLabel);
    canvasInsertAtTop(&g_sHomeCanvas.psWidgets, &counterLabel2);
    canvasInsertAtTop(&g_sHomeCanvas.psWidgets, &thirdWidget);
	canvasInsertAtTop(&g_sHomeCanvas.psWidgets, &sliderWidget);
	canvasInsertAtTop(&g_sHomeCanvas.psWidgets, &vertSliderWidget);
	canvasInsertAtTop(&g_sHomeCanvas.psWidgets, &sliderValueWidget);
	canvasInsertAtTop(&g_sHomeCanvas.psWidgets, &vertSliderValueWidget);
    canvasInsertAtTop(&g_sHomeCanvas.psWidgets, &changeThemeButton);
	useNavigationButtons(&g_sHomeCanvas);
	
	//canvasInsertAtTop(&g_sHomeCanvas.psWidgets, &btnPrevFormContainer);

    // Add reference to formManager
    g_i16HomeFormID = formManagerAddForm(&g_sHomeCanvas);
}

