
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
#include "file_manager.h"

#include "common_widgets.h"

#ifndef EVE_FREE_RAMG_START
#define EVE_FREE_RAMG_START		768000 + 200
#endif

gfx_GenericWidget titleWidget;
gfx_GenericWidget btnInicioWidget;
gfx_GenericWidget btnConfigWidget;
gfx_GenericWidget btnGraphWidget;
gfx_GenericWidget headerPanelWidget;
gfx_GenericWidget tcLogoImgWidget;
gfx_GenericWidget dateWidget;
gfx_GenericWidget timeWidget;
gfx_GenericWidget emergencyStopWidget;

gfx_Button btnInicioData;
gfx_Button btnConfigData;
gfx_Button btnGraphData;

gfx_Label titleData;
gfx_Image tcLogoImgData;
gfx_Rectangle headerPanelData;
gfx_Label dateData;
gfx_Label timeData;
gfx_Button emergencyStopData;

static char dateBuffer[12] = "21/04/2026";
static char timeBuffer[12] = "15:35:12";

static const char TAG[] = "commonWidgets";
// ==========================================
// Callbacks (Botones)
// ==========================================
static void onInicioBtnReleased(gfx_Button *btn) {
	btnConfigData.style = STYLE_DEFAULT;
	btnConfigData.state = BTN_STATE_NORMAL;
	btnConfigData.bIsDirty = true;
	btnGraphData.style = STYLE_DEFAULT;
	btnGraphData.state = BTN_STATE_NORMAL;
	btnGraphData.bIsDirty = true;

    btn->state = BTN_STATE_NORMAL;
	btn->style = STYLE_DANGER;
    btn->bIsDirty = true;
    Event_Post(EVT_SYS_SHOW_HOME_FORM, (EventParam_t){.ptr = NULL}); // Ejemplo de evento de navegación
}

static void onConfigBtnReleased(gfx_Button *btn) {
	btnInicioData.style = STYLE_DEFAULT;
	btnInicioData.state = BTN_STATE_NORMAL;
	btnInicioData.bIsDirty = true;
	btnGraphData.style = STYLE_DEFAULT;
	btnGraphData.state = BTN_STATE_NORMAL;
	btnGraphData.bIsDirty = true;
	
    btn->state = BTN_STATE_NORMAL;
	btn->style = STYLE_DANGER;
    btn->bIsDirty = true;
	Event_Post(EVT_SYS_SHOW_CONFIG_FORM, (EventParam_t){.ptr = NULL});
}

static void onGraphBtnReleased(gfx_Button *btn) {
	btnConfigData.style = STYLE_DEFAULT;
	btnConfigData.state = BTN_STATE_NORMAL;
	btnConfigData.bIsDirty = true;
	btnInicioData.style = STYLE_DEFAULT;
	btnInicioData.state = BTN_STATE_NORMAL;
	btnInicioData.bIsDirty = true;

    btn->state = BTN_STATE_NORMAL;
	btn->style = STYLE_DANGER;
    btn->bIsDirty = true;
    Event_Post(EVT_CMD_SHOW_GRAPH_FORM, (EventParam_t){.ptr = NULL});
    //Event_Post(EVT_CMD_NAV_GRAPH, 0);
}

static void onEmergencyStopBtnRelease(gfx_Button *btn) {
	onGenericBtnRelease(btn);	
}

static void onDateChanged(EventParam_t arg) {
	if(arg.str == NULL) return;

	strcpy(dateBuffer, arg.str);
	dateData.bIsDirty = true;
}
static void onRTCTimeChanged(EventParam_t arg) {
	if(arg.str == NULL) return;

	strcpy(timeBuffer, arg.str);
	timeData.bIsDirty = true;
}

void initCommonWidgets(void) {

    dateData = (gfx_Label){
        .text = dateBuffer,
        .name = "dateWidget",
        .pos.x = LCD_WIDTH / 2.0 + 130, // Esquina superior derecha
        .pos.y = 25,
        .alignment = ALIGN_RIGHT,
        .typo = TYPO_CAPTION,           
        .style = STYLE_TEXT_MUTED,
        .isVisible = true,
    };
    dateWidget.eWidgetType = WD_TYPE_LABEL; dateWidget.pvWidget = (void *)&dateData;

    timeData = (gfx_Label){
        .text = timeBuffer,
        .name = "timeWidget",
        .pos.x = LCD_WIDTH / 2.0 + 130, // Esquina superior derecha
        .pos.y = 55,
        .alignment = ALIGN_RIGHT,
        .typo = TYPO_H2,           
        .style = STYLE_TEXT_MAIN_BOLD,
        .isVisible = true,
    };
    timeWidget.eWidgetType = WD_TYPE_LABEL; timeWidget.pvWidget = (void *)&timeData;

	emergencyStopData = (gfx_Button){
		.label = "EMERGENCY STOP",
		.size.height = 55,
		.size.width = 245,
		.pos.x = timeData.pos.x + 20,
		.pos.y = 5,
		.name = "emergencyStop",
		.onPosChanged = NULL,
		.onPressed = onGenericBtnPressed,
		.onRelease = onEmergencyStopBtnRelease,
		.radius = 5,
		.state = BTN_STATE_NORMAL,
		.style = STYLE_DANGER,
		.typo = TYPO_MONO_BOLD,
		.borderWidth = 1,
	};
	gfx_initRegTouch((void *)&emergencyStopData, WD_TYPE_BUTTON);
	emergencyStopWidget.eWidgetType = WD_TYPE_BUTTON; emergencyStopWidget.pvWidget = (void *)&emergencyStopData;

    uint16_t btnWidth = 260;
    uint16_t btnHeight = 60;
    uint16_t btnY = 420;

    btnInicioData = (gfx_Button){
        .name = "btnIni", .label = "Inicio", .size.width = btnWidth, .size.height = btnHeight,
        .pos.x = 5, .pos.y = btnY, .oldPos.x = 65, .oldPos.y = btnY,
        .typo = TYPO_H2, .style = STYLE_PRIMARY, .borderWidth = 0, .radius = 5,
        .state = BTN_STATE_NORMAL, .onPressed = onGenericBtnPressed, .onRelease = onInicioBtnReleased
    };
    btnInicioWidget.eWidgetType = WD_TYPE_BUTTON; btnInicioWidget.pvWidget = (void *)&btnInicioData;
    gfx_initRegTouch(btnInicioWidget.pvWidget, WD_TYPE_BUTTON);

    btnConfigData = (gfx_Button){
        .name = "btnCfg", .label = "Config", .size.width = btnWidth, .size.height = btnHeight,
        .pos.x = btnWidth + 10, .pos.y = btnY, .oldPos.x = 310, .oldPos.y = btnY,
        .typo = TYPO_H2, .style = STYLE_DEFAULT, .borderWidth = 0, .radius = 5,
        .state = BTN_STATE_NORMAL, .onPressed = onGenericBtnPressed, .onRelease = onConfigBtnReleased
    };
    btnConfigWidget.eWidgetType = WD_TYPE_BUTTON; btnConfigWidget.pvWidget = (void *)&btnConfigData;
    gfx_initRegTouch(btnConfigWidget.pvWidget, WD_TYPE_BUTTON);

    btnGraphData = (gfx_Button){
        .name = "btnGraph", .label = "Graph", .size.width = btnWidth, .size.height = btnHeight,
        .pos.x = btnWidth * 2 + 15, .pos.y = btnY, .oldPos.x = 555, .oldPos.y = btnY,
        .typo = TYPO_H2, .style = STYLE_DEFAULT, .borderWidth = 0, .radius = 5,
        .state = BTN_STATE_NORMAL, .onPressed = onGenericBtnPressed, .onRelease = onGraphBtnReleased
    };
    btnGraphWidget.eWidgetType = WD_TYPE_BUTTON; btnGraphWidget.pvWidget = (void *)&btnGraphData;
    gfx_initRegTouch(btnGraphWidget.pvWidget, WD_TYPE_BUTTON);

	tcLogoImgData = (gfx_Image) {
		.name = "mainLogo",
		.pos.x = 5,
		.pos.y = 15,
		.scale = 1,
	};

	if(!FM_LoadEVEImage(DRIVE_SD_ID, "logo_tc.png", &tcLogoImgData, EVE_FREE_RAMG_START)) {
		TIVA_LOGE(TAG, "Fallo al cargar logo_tc.png en EVE");
	}

	tcLogoImgWidget.eWidgetType = WD_TYPE_IMAGE;
	tcLogoImgWidget.pvWidget = (void *)&tcLogoImgData;

	headerPanelData = (gfx_Rectangle){
		.dim.width = LCD_WIDTH,
		.dim.height = 65,
		.pos.x = 0,
		.pos.y = 0,
		.color = g_pCurrentTheme->palette.surface,
		.round = 0,
		.borderWidth = 1,
	};
	headerPanelWidget.eWidgetType = WD_TYPE_RECT; headerPanelWidget.pvWidget = (void *)&headerPanelData;


	Event_Subscribe(EVT_SYS_DATE_CHANGED, (EventHandler_fn)onDateChanged);
	Event_Subscribe(EVT_SYS_TIME_CHANGED, (EventHandler_fn)onRTCTimeChanged);
}

void useNavigationButtons(gfx_Canvas* canvas) {
	if(canvas == NULL) return;

    canvasInsertAtTop(&canvas->psWidgets, &btnInicioWidget);
    canvasInsertAtTop(&canvas->psWidgets, &btnConfigWidget);
    canvasInsertAtTop(&canvas->psWidgets, &btnGraphWidget);
}

void useHeaderPanelWidget(gfx_Canvas* canvas) {
	if(canvas == NULL) return;

    canvasInsertAtTop(&canvas->psWidgets, &headerPanelWidget);
}

void useLogoWidget(gfx_Canvas* canvas) {
	if(canvas == NULL) return;

    canvasInsertAtTop(&canvas->psWidgets, &tcLogoImgWidget);

}

void useFullHeader(gfx_Canvas* canvas) {
	if(canvas == NULL) return;

	canvasInsertAtTop(&canvas->psWidgets, &headerPanelWidget);
	canvasInsertAtTop(&canvas->psWidgets, &tcLogoImgWidget);
	canvasInsertAtTop(&canvas->psWidgets, &dateWidget);
	canvasInsertAtTop(&canvas->psWidgets, &timeWidget);
	canvasInsertAtTop(&canvas->psWidgets, &emergencyStopWidget);
}

