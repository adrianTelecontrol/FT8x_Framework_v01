
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

gfx_GenericWidget btnInicioWidget;
gfx_GenericWidget btnConfigWidget;
gfx_GenericWidget btnGraphWidget;

gfx_Button btnInicioData;
gfx_Button btnConfigData;
gfx_Button btnGraphData;

// ==========================================
// Callbacks (Botones)
// ==========================================
static void onGenericBtnPressed(gfx_Button *btn) {
    btn->state = BTN_STATE_PRESSED;
    btn->bIsDirty = true;
}

static void onInicioBtnReleased(gfx_Button *btn) {
	btnConfigData.style = STYLE_SECONDARY;
	btnConfigData.state = BTN_STATE_NORMAL;
	btnConfigData.bIsDirty = true;
	btnGraphData.style = STYLE_SECONDARY;
	btnGraphData.state = BTN_STATE_NORMAL;
	btnGraphData.bIsDirty = true;

    btn->state = BTN_STATE_NORMAL;
	btn->style = STYLE_DANGER;
    btn->bIsDirty = true;
    //Event_Post(EVT_CMD_NAV_HOME, 0); // Ejemplo de evento de navegación
}

static void onConfigBtnReleased(gfx_Button *btn) {
	btnInicioData.style = STYLE_SECONDARY;
	btnInicioData.state = BTN_STATE_NORMAL;
	btnInicioData.bIsDirty = true;
	btnGraphData.style = STYLE_SECONDARY;
	btnGraphData.state = BTN_STATE_NORMAL;
	btnGraphData.bIsDirty = true;
	
    btn->state = BTN_STATE_NORMAL;
	btn->style = STYLE_DANGER;
    btn->bIsDirty = true;
    //Event_Post(EVT_CMD_NAV_CONFIG, 0);
}

static void onGraphBtnReleased(gfx_Button *btn) {
	btnConfigData.style = STYLE_SECONDARY;
	btnConfigData.state = BTN_STATE_NORMAL;
	btnConfigData.bIsDirty = true;
	btnInicioData.style = STYLE_SECONDARY;
	btnInicioData.state = BTN_STATE_NORMAL;
	btnInicioData.bIsDirty = true;

    btn->state = BTN_STATE_NORMAL;
	btn->style = STYLE_DANGER;
    btn->bIsDirty = true;
    //Event_Post(EVT_CMD_NAV_GRAPH, 0);
}

void initNavigationWidgets(void) {
    uint16_t btnWidth = 260;
    uint16_t btnHeight = 60;
    uint16_t btnY = 425;

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
        .typo = TYPO_H2, .style = STYLE_SECONDARY, .borderWidth = 0, .radius = 5,
        .state = BTN_STATE_NORMAL, .onPressed = onGenericBtnPressed, .onRelease = onConfigBtnReleased
    };
    btnConfigWidget.eWidgetType = WD_TYPE_BUTTON; btnConfigWidget.pvWidget = (void *)&btnConfigData;
    gfx_initRegTouch(btnConfigWidget.pvWidget, WD_TYPE_BUTTON);

    btnGraphData = (gfx_Button){
        .name = "btnGraph", .label = "Graph", .size.width = btnWidth, .size.height = btnHeight,
        .pos.x = btnWidth * 2 + 15, .pos.y = btnY, .oldPos.x = 555, .oldPos.y = btnY,
        .typo = TYPO_H2, .style = STYLE_SECONDARY, .borderWidth = 0, .radius = 5,
        .state = BTN_STATE_NORMAL, .onPressed = onGenericBtnPressed, .onRelease = onGraphBtnReleased
    };
    btnGraphWidget.eWidgetType = WD_TYPE_BUTTON; btnGraphWidget.pvWidget = (void *)&btnGraphData;
    gfx_initRegTouch(btnGraphWidget.pvWidget, WD_TYPE_BUTTON);
}

void useNavigationButtons(gfx_Canvas* canvas) {
	if(canvas == NULL) return;

    canvasInsertAtTop(&canvas->psWidgets, &btnInicioWidget);
    canvasInsertAtTop(&canvas->psWidgets, &btnConfigWidget);
    canvasInsertAtTop(&canvas->psWidgets, &btnGraphWidget);
}



