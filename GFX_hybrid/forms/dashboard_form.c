#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "gfx.h"
#include "FT8xx_params.h"
#include "form_common.h"
#include "EVE_colors.h"
#include "gfx_canvas.h"
#include "forms_manager.h"
#include "sdspi_hal.h"

#include "helpers.h"
#include "gfx_theme.h"
#include "event_engine.h"
#include "navigation_widgets.h"

#include "dashboard_form.h"

#ifndef EVE_FREE_RAMG_START
#define EVE_FREE_RAMG_START		768000 + 200
#endif

static const char *TAG = "dashboardForm";

static gfx_Canvas g_sSystemCanvas;

int16_t g_i16DashboardFormID = 0;

// ==========================================
// 1. The Generic Nodes (Widgets)
// ==========================================
// Header
gfx_GenericWidget titleWidget;
gfx_GenericWidget dateWidget;
gfx_GenericWidget timeWidget;
gfx_GenericWidget headerPanelWidget;
gfx_GenericWidget emergencyStopWidget;
gfx_GenericWidget tcLogoImgWidget;

// Panel: Temperaturas
gfx_GenericWidget tempPanelBgWidget;
gfx_GenericWidget t1PanelWidget;
gfx_GenericWidget t2PanelWidget;
gfx_GenericWidget t3PanelWidget;
gfx_GenericWidget tempPanelTitleWidget;
gfx_GenericWidget t1Widget;
gfx_GenericWidget t2Widget;
gfx_GenericWidget t3Widget;
gfx_GenericWidget t1ValueWidget;
gfx_GenericWidget t2ValueWidget;
gfx_GenericWidget t3ValueWidget;

// Panel: Status
gfx_GenericWidget statusPanelBgWidget;
gfx_GenericWidget vinPanelWidget;
gfx_GenericWidget voutPanelWidget;
gfx_GenericWidget statusPanelWidget;
gfx_GenericWidget statusPanelTitleWidget;
gfx_GenericWidget vinLabelWidget;
gfx_GenericWidget voutLabelWidget;
gfx_GenericWidget statusLabelWidget;
gfx_GenericWidget vinValueWidget;
gfx_GenericWidget voutValueWidget;
gfx_GenericWidget statusValueWidget;

// Footer Buttons
gfx_GenericWidget btnInicioWidget;
gfx_GenericWidget btnConfigWidget;
gfx_GenericWidget btnGraphWidget;

// ==========================================
// 2. The Persistent Memory (Data)
// ==========================================
// Header
gfx_Rectangle headerPanelData;
gfx_Image tcLogoImgData;
gfx_Label titleData;
gfx_Label dateData;
gfx_Label timeData;
gfx_Button emergencyStopData;

// Panel: Temperaturas
gfx_Rectangle tempPanelBgData;
gfx_Rectangle t1PanelData;
gfx_Rectangle t2PanelData;
gfx_Rectangle t3PanelData;
gfx_Label     tempPanelTitleData;
gfx_Label     t1LabelData;
gfx_Label     t2LabelData;
gfx_Label     t3LabelData;
gfx_Label     t1ValueData;
gfx_Label     t2ValueData;
gfx_Label     t3ValueData;

// Panel: Status
gfx_Rectangle statusPanelBgData;
gfx_Rectangle vinPanelData;
gfx_Rectangle voutPanelData;
gfx_Rectangle statusPanelData;
gfx_Label     statusPanelTitleData;
gfx_Label     vinLabelData;
gfx_Label     voutLabelData;
gfx_Label     statusLabelData;
gfx_Label     vinValueData;
gfx_Label     voutValueData;
gfx_Label     statusValueData;

// Footer Buttons
gfx_Button btnInicioData;
gfx_Button btnConfigData;
gfx_Button btnGraphData;

// ==========================================
// 3. Static Text Buffers
// ==========================================
static char dateBuffer[32] = "21/04/2026";
static char timeBuffer[32] = "15:35";
static char t1Buffer[16] = "24.5 C";
static char t2Buffer[16] = "25.1 C";
static char t3Buffer[16] = "23.8 C";
static char vinBuffer[16]  = "24.0 V";
static char voutBuffer[16] = "5.0 V";
static char statusBuffer[32] = "NOMINAL";


// ==========================================
// Callbacks 
// ==========================================
static void onEmergencyStopBtnRelease(gfx_Button *btn) {
	onGenericBtnRelease(btn);	
}

static void onT1ValueChanged(uint32_t val) {
	//float fval = *(float *)val;
	//float fval = (float )val;
	float fval = rand() % 20;
	sprintf(t1Buffer, "%.1f C", fval);
	t1ValueData.bIsDirty = true;
}

static void onT2ValueChanged(uint32_t val) {
	float fval = *(float *)val;
	snprintf(t2Buffer, 16, "%.1f C", fval);
	t2ValueData.bIsDirty = true;
}

static void onT3ValueChanged(uint32_t val) {
	float fval = *(float *)val;
	snprintf(t3Buffer, 16, "%.1f C", fval);
	t3ValueData.bIsDirty = true;
}

static void onVinValueChanged(uint32_t val) {
	float fval = *(float *)val;
	snprintf(vinBuffer, 16, "%.1f V", fval);
	vinValueData.bIsDirty = true;
}

static void onVoutValueChanged(uint32_t val) {
	float fval = *(float *)val;
	snprintf(voutBuffer, 16, "%.1f V", fval);
	voutValueData.bIsDirty = true;
}

// ==========================================
// 4. Initialization Function
// ==========================================
void initDashboardForm(void)
{
    g_sSystemCanvas.ui16BackgroundColor = g_pCurrentTheme->palette.background; 
    g_sSystemCanvas.psWidgets = NULL;

	tcLogoImgData = (gfx_Image) {
		.name = "mainLogo",
		.pos.x = 5,
		.pos.y = 10,
		.scale = 1,
	};

	if(!SDSPI_LoadEVEImage(&tcLogoImgData, "logo_tc.png", EVE_FREE_RAMG_START)) {
		TIVA_LOGE(TAG, "Fallo al cargar logo_tc.png en EVE");
	}

	tcLogoImgWidget.eWidgetType = WD_TYPE_IMAGE;
	tcLogoImgWidget.pvWidget = (void *)&tcLogoImgData;

    // --- HEADER ---
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
	
    titleData = (gfx_Label){
        .text = "SYSTEM MONITOR",
        .name = "sysTitle",
        .pos.x = 110,
        .pos.y = 50,
        .alignment = ALIGN_LEFT,
        .typo = TYPO_H2,           
        .style = STYLE_TEXT_MAIN,
        .isVisible = true,
    };
    titleWidget.eWidgetType = WD_TYPE_LABEL; titleWidget.pvWidget = (void *)&titleData;

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

    // --- PANEL 1: TEMPERATURAS (Izquierda) ---
	uint16_t panelWidth = 385;
	uint16_t panelHeight = 335;
	uint16_t indicatorWidth = panelWidth - 30;
	uint16_t indicatorHeight = (panelHeight - 40) / 4.0;
    tempPanelBgData = (gfx_Rectangle){
        .name = "tempBg",
        .pos.x = 10,
        .pos.y = 75,
        .dim.width = panelWidth,
        .dim.height = panelHeight,
        .round = 5,
        .color = g_pCurrentTheme->palette.surface,
		.borderWidth = 1,
    };
    tempPanelBgWidget.eWidgetType = WD_TYPE_RECT; tempPanelBgWidget.pvWidget = (void *)&tempPanelBgData;


    tempPanelTitleData = (gfx_Label){ .text = "TEMPERATURAS", .name = "tTemp", .pos.x = 80, .pos.y = tempPanelBgData.pos.y + 40, .alignment = ALIGN_LEFT, .typo = TYPO_H2, .style = STYLE_SECONDARY, .isVisible = true };
    tempPanelTitleWidget.eWidgetType = WD_TYPE_LABEL; tempPanelTitleWidget.pvWidget = (void *)&tempPanelTitleData;

	t1PanelData = (gfx_Rectangle){
        .name = "t1Bg",
        .pos.x = tempPanelBgData.pos.x + 20,
        .pos.y = tempPanelBgData.pos.y + indicatorHeight + 5,
        .dim.width = indicatorWidth,
        .dim.height = indicatorHeight,
        .round = 5,
        .color = g_pCurrentTheme->palette.background,
		.borderWidth = 1,
	};
	t1PanelWidget.eWidgetType = WD_TYPE_RECT; t1PanelWidget.pvWidget = (void *)&t1PanelData;

	t2PanelData = (gfx_Rectangle){
        .name = "t2Bg",
        .pos.x = tempPanelBgData.pos.x + 20,
        .pos.y = tempPanelBgData.pos.y + indicatorHeight * 2 + 15,
        .dim.width = indicatorWidth,
        .dim.height = indicatorHeight,
        .round = 5,
        .color = g_pCurrentTheme->palette.background,
		.borderWidth = 1,
	};
	t2PanelWidget.eWidgetType = WD_TYPE_RECT; t2PanelWidget.pvWidget = (void *)&t2PanelData;

	t3PanelData = (gfx_Rectangle){
        .name = "t3Bg",
        .pos.x = tempPanelBgData.pos.x + 20,
        .pos.y = tempPanelBgData.pos.y + indicatorHeight * 3 + 25,
        .dim.width = indicatorWidth,
        .dim.height = indicatorHeight,
        .round = 5,
        .color = g_pCurrentTheme->palette.background,
		.borderWidth = 1,
		.bIsDirty = true,
	};
	t3PanelWidget.eWidgetType = WD_TYPE_RECT; t3PanelWidget.pvWidget = (void *)&t3PanelData;

    t1LabelData = (gfx_Label){ .text = "T1", .name = "t1", .pos.x = 50, .pos.y = t1PanelData.pos.y + indicatorHeight / 2, .alignment = (gfx_Align_e)(ALIGN_LEFT | ALIGN_VCENTER), .typo = TYPO_BODY, .style = STYLE_TEXT_MAIN, .isVisible = true };
    t1Widget.eWidgetType = WD_TYPE_LABEL; t1Widget.pvWidget = (void *)&t1LabelData;

    t2LabelData = (gfx_Label){ .text = "T2", .name = "t2", .pos.x = 50, .pos.y = t2PanelData.pos.y + indicatorHeight / 2, .alignment = (gfx_Align_e)(ALIGN_LEFT | ALIGN_VCENTER), .typo = TYPO_BODY, .style = STYLE_TEXT_MAIN, .isVisible = true };
    t2Widget.eWidgetType = WD_TYPE_LABEL; t2Widget.pvWidget = (void *)&t2LabelData;

    t3LabelData = (gfx_Label){ .text = "T3", .name = "t3", .pos.x = 50, .pos.y = t3PanelData.pos.y + indicatorHeight / 2, .alignment = (gfx_Align_e)(ALIGN_LEFT | ALIGN_VCENTER), .typo = TYPO_BODY, .style = STYLE_TEXT_MAIN, .isVisible = true };
    t3Widget.eWidgetType = WD_TYPE_LABEL; t3Widget.pvWidget = (void *)&t3LabelData;

    t1ValueData = (gfx_Label){ .text = t1Buffer, .name = "tval1", .pos.x = t1PanelData.pos.x + t1PanelData.dim.width - 30, .pos.y = t1PanelData.pos.y + indicatorHeight / 2, .alignment = (gfx_Align_e)(ALIGN_RIGHT | ALIGN_VCENTER), .typo = TYPO_H1, .style = STYLE_SUCCESS, .isVisible = true };
    t1ValueWidget.eWidgetType = WD_TYPE_LABEL; t1ValueWidget.pvWidget = (void *)&t1ValueData;

    t2ValueData = (gfx_Label){ .text = t2Buffer, .name = "tval2", .pos.x = t1PanelData.pos.x + t1PanelData.dim.width - 30, .pos.y = t2PanelData.pos.y + indicatorHeight / 2, .alignment = (gfx_Align_e)(ALIGN_RIGHT | ALIGN_VCENTER), .typo = TYPO_H1, .style = STYLE_DANGER, .isVisible = true };
    t2ValueWidget.eWidgetType = WD_TYPE_LABEL; t2ValueWidget.pvWidget = (void *)&t2ValueData;

    t3ValueData = (gfx_Label){ .text = t3Buffer, .name = "tval3", .pos.x = t1PanelData.pos.x + t1PanelData.dim.width - 30, .pos.y = t3PanelData.pos.y + indicatorHeight / 2, .alignment = (gfx_Align_e)(ALIGN_RIGHT | ALIGN_VCENTER), .typo = TYPO_H1, .style = STYLE_SECONDARY, .isVisible = true };
    t3ValueWidget.eWidgetType = WD_TYPE_LABEL; t3ValueWidget.pvWidget = (void *)&t3ValueData;

    // --- PANEL 2: STATUS (Derecha) ---
    statusPanelBgData = (gfx_Rectangle){
        .name = "statBg",
        .pos.x = panelWidth + 20,
        .pos.y = 75,
        .dim.width = panelWidth,
        .dim.height = panelHeight,
        .round = 5,
        .color = g_pCurrentTheme->palette.surface, 
		.borderWidth = 1,
    };
    statusPanelBgWidget.eWidgetType = WD_TYPE_RECT; statusPanelBgWidget.pvWidget = (void *)&statusPanelBgData;

	vinPanelData= (gfx_Rectangle){
        .name = "vinBgPanel",
        .pos.x = statusPanelBgData.pos.x + 20,
        .pos.y = tempPanelBgData.pos.y + indicatorHeight + 5,
        .dim.width = indicatorWidth,
        .dim.height = indicatorHeight,
        .round = 5,
        .color = g_pCurrentTheme->palette.background,
		.borderWidth = 1,
	};
	vinPanelWidget.eWidgetType = WD_TYPE_RECT; vinPanelWidget.pvWidget = (void *)&vinPanelData;

	voutPanelData = (gfx_Rectangle){
        .name = "t2Bg",
        .pos.x = statusPanelBgData.pos.x + 20,
        .pos.y = tempPanelBgData.pos.y + indicatorHeight * 2 + 15,
        .dim.width = indicatorWidth,
        .dim.height = indicatorHeight,
        .round = 5,
        .color = g_pCurrentTheme->palette.background,
		.borderWidth = 1,
	};
	voutPanelWidget.eWidgetType = WD_TYPE_RECT; voutPanelWidget.pvWidget = (void *)&voutPanelData;

	statusPanelData = (gfx_Rectangle){
        .name = "t3Bg",
        .pos.x = statusPanelBgData.pos.x + 20,
        .pos.y = tempPanelBgData.pos.y + indicatorHeight * 3 + 25,
        .dim.width = indicatorWidth,
        .dim.height = indicatorHeight,
        .round = 5,
        .color = g_pCurrentTheme->palette.background,
		.borderWidth = 1,
		.bIsDirty = true,
	};
	statusPanelWidget.eWidgetType = WD_TYPE_RECT; statusPanelWidget.pvWidget = (void *)&statusPanelData;

    statusPanelTitleData = (gfx_Label){ .text = "SYSTEM STATUS", .name = "tStat", .pos.x = 470, .pos.y = statusPanelBgData.pos.y + 40, .alignment = ALIGN_LEFT, .typo = TYPO_H2, .style = STYLE_SECONDARY, .isVisible = true };
    statusPanelTitleWidget.eWidgetType = WD_TYPE_LABEL; statusPanelTitleWidget.pvWidget = (void *)&statusPanelTitleData;

    vinLabelData = (gfx_Label){ .text = "Vin", .name = "vin", .pos.x = 440, .pos.y = vinPanelData.pos.y + vinPanelData.dim.height / 2, .alignment = (gfx_Align_e)(ALIGN_LEFT | ALIGN_VCENTER), .typo = TYPO_BODY, .style = STYLE_TEXT_MAIN, .isVisible = true };
    vinLabelWidget.eWidgetType = WD_TYPE_LABEL; vinLabelWidget.pvWidget = (void *)&vinLabelData;

    voutLabelData = (gfx_Label){ .text = "Vout", .name = "vout", .pos.x = 440, .pos.y = voutPanelData.pos.y + voutPanelData.dim.height / 2, .alignment = ( gfx_Align_e )(ALIGN_LEFT | ALIGN_VCENTER), .typo = TYPO_BODY, .style = STYLE_TEXT_MAIN, .isVisible = true };
    voutLabelWidget.eWidgetType = WD_TYPE_LABEL; voutLabelWidget.pvWidget = (void *)&voutLabelData;

    statusLabelData = (gfx_Label){ .text = "Status", .name = "sysSt", .pos.x = 440, .pos.y = statusPanelData.pos.y + statusPanelData.dim.height / 2, .alignment = ( gfx_Align_e )(ALIGN_LEFT | ALIGN_VCENTER), .typo = TYPO_BODY, .style = STYLE_TEXT_MAIN, .isVisible = true };
    statusLabelWidget.eWidgetType = WD_TYPE_LABEL; statusLabelWidget.pvWidget = (void *)&statusLabelData;

    vinValueData = (gfx_Label){ .text = vinBuffer, .name = "vinVal", .pos.x = vinPanelData.pos.x + vinPanelData.dim.width - 30, .pos.y = vinPanelData.pos.y + indicatorHeight / 2, .alignment = (gfx_Align_e)(ALIGN_RIGHT | ALIGN_VCENTER), .typo = TYPO_H1, .style = STYLE_SUCCESS, .isVisible = true };
    vinValueWidget.eWidgetType = WD_TYPE_LABEL; vinValueWidget.pvWidget = (void *)&vinValueData;

    voutValueData = (gfx_Label){ .text = voutBuffer, .name = "voutVal", .pos.x = voutPanelData.pos.x + voutPanelData.dim.width - 30, .pos.y = voutPanelData.pos.y + indicatorHeight / 2, .alignment = (gfx_Align_e)(ALIGN_RIGHT | ALIGN_VCENTER), .typo = TYPO_H1, .style = STYLE_DANGER, .isVisible = true };
    voutValueWidget.eWidgetType = WD_TYPE_LABEL; voutValueWidget.pvWidget = (void *)&voutValueData;

    statusValueData = (gfx_Label){ .text = statusBuffer, .name = "tval3", .pos.x = statusPanelData.pos.x + statusPanelData.dim.width - 30, .pos.y = statusPanelData.pos.y + indicatorHeight / 2, .alignment = (gfx_Align_e)(ALIGN_RIGHT | ALIGN_VCENTER), .typo = TYPO_H1, .style = STYLE_SECONDARY, .isVisible = true };
    statusValueWidget.eWidgetType = WD_TYPE_LABEL; statusValueWidget.pvWidget = (void *)&statusValueData;



    // ==========================================
    // 5. Insertion into Canvas (Back-to-front)
    // ==========================================
    // Fondos de paneles
    canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &headerPanelWidget);
    canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &tempPanelBgWidget);
    canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &statusPanelBgWidget);
	canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &tcLogoImgWidget);
	canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &t1PanelWidget);
	canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &t2PanelWidget);
	canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &t3PanelWidget);
	canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &vinPanelWidget);
	canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &voutPanelWidget);
	canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &statusPanelWidget);

    // Header
    canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &titleWidget);
    canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &dateWidget);
    canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &timeWidget);
    canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &emergencyStopWidget);

    // Contenido Temperaturas
    canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &tempPanelTitleWidget);
    canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &t1Widget);
    canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &t2Widget);
    canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &t3Widget);

    // Contenido Status
    canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &statusPanelTitleWidget);
    canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &vinLabelWidget);
    canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &voutLabelWidget);
    canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &statusLabelWidget);
	   
    canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &t1ValueWidget);
    canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &t2ValueWidget);
    canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &t3ValueWidget);
    canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &vinValueWidget);
    canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &voutValueWidget);
    canvasInsertAtTop(&g_sSystemCanvas.psWidgets, &statusValueWidget);

	useNavigationButtons(&g_sSystemCanvas);

	// Subscribe to events
	Event_Subscribe(EVT_SYS_T1_VAL_CHANGED, ( EventHandler_fn ) onT1ValueChanged);
	Event_Subscribe(EVT_SYS_T2_VAL_CHANGED, ( EventHandler_fn ) onT2ValueChanged);
	Event_Subscribe(EVT_SYS_T3_VAL_CHANGED, ( EventHandler_fn ) onT3ValueChanged);
	Event_Subscribe(EVT_SYS_VIN_VAL_CHANGED, ( EventHandler_fn ) onVinValueChanged);
	Event_Subscribe(EVT_SYS_VOUT_VAL_CHANGED, ( EventHandler_fn ) onVoutValueChanged);

    // Register Form
    g_i16DashboardFormID = formManagerAddForm(&g_sSystemCanvas);
}



