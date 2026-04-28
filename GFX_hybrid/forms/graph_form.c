
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "event_engine.h"
#include "helpers.h"
#include "FT8xx_params.h"
#include "gfx.h"
#include "gfx_canvas.h"
#include "gfx_colors.h"
#include "font_engine.h"
#include "gfx_theme.h"
#include "forms_manager.h"
#include "common_widgets.h"
#include "gfx_colors.h"

#include "graph_form.h"

// #define NUM_POINTS	800 - 40
#define NUM_POINTS	500


static gfx_Canvas g_sGraphFormCanvas;
int16_t g_i16GraphFormID = 0;

gfx_GenericWidget graphWidgetContainer;
gfx_GenericWidget graphTitleContainer;
gfx_GenericWidget graphSawtoothContainer;
gfx_GenericWidget multigraphContainer;
gfx_GenericWidget graphOverlayContainer;
gfx_GenericWidget multigraphOverlayContainer;

gfx_Graph graphWidget;
gfx_Graph graphSawtoothWidget;
gfx_Label graphTitle;
gfx_MultiGraph multigraphWidget;
gfx_GraphOverlay graphOverlayWidget;
gfx_GraphOverlay multigraphOverlayWidget;

int16_t graphData[NUM_POINTS]; 
int16_t sawtoothData[NUM_POINTS];
int16_t multigraphData1[NUM_POINTS];
int16_t multigraphData2[NUM_POINTS];

static char graphOverlayData[] = "TC1: 62.49 [C]";


static void onNewGraphValue(EventParam_t arg) {
	gfx_GraphAddPoint(&graphWidget, (int16_t)arg.f32);
	if(GetExecTimeMs() % 500 == 0) {
		sprintf(graphOverlayWidget.traces[0].valueText, "TC1: %.2f [C]", arg.f32);
		graphOverlayWidget.bIsDirty = true;
	}
}

static void onSawtoothGraphValue(EventParam_t arg) {
	gfx_GraphAddPoint(&graphSawtoothWidget, (int16_t)arg.f32);
}

static void onMultigraphSine(EventParam_t arg) {
	gfx_MultiGraphAddData(&multigraphWidget, 0, (int16_t)arg.f32);
	if(GetExecTimeMs() % 500 == 0) {
		sprintf(multigraphOverlayWidget.traces[0].valueText, "TC1: %.2f [C]", arg.f32);
		multigraphOverlayWidget.bIsDirty = true;
	}
}

static void onMultigraphSawtoothValue(EventParam_t arg) {
	gfx_MultiGraphAddData(&multigraphWidget, 1, (int16_t)arg.f32);

	if(GetExecTimeMs() % 500 == 0) {
		sprintf(multigraphOverlayWidget.traces[1].valueText, "TC2: %.2f [C]", arg.f32);
		multigraphOverlayWidget.bIsDirty = true;
	}
}

void initGraphForm(void) {
	
	g_sGraphFormCanvas.ui16BackgroundColor = g_pCurrentTheme->palette.background;
	g_sGraphFormCanvas.psWidgets = NULL;

	graphTitle = (gfx_Label) {
        .text = "GRAPH",
        .name = "sysTitle",
        .pos.x = 110,
        .pos.y = 50,
        .alignment = ALIGN_LEFT,
        .typo = TYPO_H2,           
        .style = STYLE_TEXT_MAIN,
        .isVisible = true,
	};
	graphTitleContainer.eWidgetType = WD_TYPE_LABEL;
	graphTitleContainer.pvWidget = (void *)&graphTitle;

	graphWidget = (gfx_Graph){
		.name = "graph",
		.data = graphData,
		.maxPoints = NUM_POINTS,
		.head = 0,
		.minY = 15,
		.maxY = 90,
		.gridLinesX = 4,
		.gridLinesY = 3,
		.size.height = 162,
		.size.width = 750,
		.pos.x = 20,
		.pos.y = 75,
		.bgColor = g_pCurrentTheme->palette.surface,
		.gridColor = g_pCurrentTheme->palette.textMuted,
		.lineColor = g_pCurrentTheme->palette.primary,
		.lineWidth = 2,
		.typo = TYPO_CAPTION,
		.textColor = g_pCurrentTheme->palette.textMuted,
		.bShowLabels = true,
	};
	memset(graphData, 0, NUM_POINTS * 2);
	
	//gfx_GraphAddPoint(&graphWidget, 50);
	graphWidgetContainer.eWidgetType = WD_TYPE_GRAPH;
	graphWidgetContainer.pvWidget = (void *)&graphWidget;

	uint16_t strWidth, strHeight;
	gfx_GetStringDimensions("TC1: 62.49 [C]", Theme_ResolveFontId(TYPO_MONO), &strWidth, &strHeight, 1);
	
	uint16_t overlayWidth = strWidth * 1.3f;
	uint16_t overlayHeight = strHeight * 1.6f;
	graphOverlayWidget = (gfx_GraphOverlay){
	    .name = "graphOvl",
	    .pos.x = graphWidget.pos.x + graphWidget.size.width - overlayWidth - 15, 
	    .pos.y = graphWidget.pos.y + 10,        
	    .size.width = overlayWidth,
	    .size.height = overlayHeight, 
	    .bgColor = g_pCurrentTheme->palette.background, 
	    .textColor = g_pCurrentTheme->palette.textMain, // Make it pop more than textMuted
	    .typo = TYPO_CAPTION,
	    // --- DATA ---
	    .numTraces = 1,
	    .traces = {
	        [0] = {
	            // Must strictly match the graph's lineColor to make visual sense
	            .color = g_pCurrentTheme->palette.primary, 
	            .valueText = "--", // Default state before first event
	            .isVisible = true
	        }
	    },
	    .bIsDirty = true
	};

	graphOverlayContainer.eWidgetType = WD_TYPE_GRAPH_OVERLAY;
	graphOverlayContainer.pvWidget = (void *)&graphOverlayWidget;

	multigraphWidget = (gfx_MultiGraph){
		.name = "sawtooth_graph",
		.maxPoints = NUM_POINTS,
		.activeTraces = 2,
		.minY = 10,
		.maxY = 90,
		.gridLinesX = 4,
		.gridLinesY = 3,
		.size.height = 162,
		.size.width = 750,
		.pos.x = 20,
		.pos.y = 247,
		.bgColor = g_pCurrentTheme->palette.surface,
		.gridColor = g_pCurrentTheme->palette.textMuted,
		.lineWidth = 2,
		.typo = TYPO_CAPTION,
		.textColor = g_pCurrentTheme->palette.textMuted,
		.bShowLabels = true,
	};
	multigraphWidget.dataSets[0] = multigraphData1;
	multigraphWidget.dataSets[1] = multigraphData2;
	multigraphWidget.lineColors[0] = g_pCurrentTheme->palette.danger;
	multigraphWidget.lineColors[1] = g_pCurrentTheme->palette.success;
	memset(multigraphWidget.heads, 0, MAX_GRAPH_DATA_SETS);
	memset(multigraphData1, 0, NUM_POINTS * 2);
	memset(multigraphData2, 0, NUM_POINTS * 2);
	
	multigraphOverlayWidget = (gfx_GraphOverlay){
	    .name = "graphOvl",
	    .pos.x = multigraphWidget.pos.x + multigraphWidget.size.width - overlayWidth - 15, 
	    .pos.y = multigraphWidget.pos.y + 10,        
	    .size.width = overlayWidth,
	    .size.height = overlayHeight * 1.8, 
	    .bgColor = g_pCurrentTheme->palette.background, 
	    .textColor = g_pCurrentTheme->palette.textMain, // Make it pop more than textMuted
	    .typo = TYPO_CAPTION,
	    // --- DATA ---
	    .numTraces = 2,
	    .traces = {
	        [0] = {
	            // Must strictly match the graph's lineColor to make visual sense
	            .color = g_pCurrentTheme->palette.danger, 
	            .valueText = "--", // Default state before first event
	            .isVisible = true
	        }, 
	        [1] = {
	            // Must strictly match the graph's lineColor to make visual sense
	            .color = g_pCurrentTheme->palette.success, 
	            .valueText = "--", // Default state before first event
	            .isVisible = true
	        }
	    },
	    .bIsDirty = true
	};

	multigraphOverlayContainer.eWidgetType = WD_TYPE_GRAPH_OVERLAY;
	multigraphOverlayContainer.pvWidget = (void *)&multigraphOverlayWidget;

	//gfx_GraphAddPoint(&graphWidget, 50);
	multigraphContainer.eWidgetType = WD_TYPE_MULTIGRAPH;
	multigraphContainer.pvWidget = (void *)&multigraphWidget;

	Event_Subscribe(EVT_SYS_NEW_GRAPH_VALUE, ( EventHandler_fn )onNewGraphValue );
	Event_Subscribe(EVT_SYS_NEW_SAWTOOTH_VALUE, (EventHandler_fn)onSawtoothGraphValue);
	Event_Subscribe(EVT_SYS_NEW_GRAPH_VALUE, ( EventHandler_fn )onMultigraphSine );
	Event_Subscribe(EVT_SYS_NEW_SAWTOOTH_VALUE, (EventHandler_fn)onMultigraphSawtoothValue);

	useFullHeader(&g_sGraphFormCanvas);
	canvasInsertAtTop(&g_sGraphFormCanvas.psWidgets, &graphTitleContainer);
	canvasInsertAtTop(&g_sGraphFormCanvas.psWidgets, &graphWidgetContainer);
	canvasInsertAtTop(&g_sGraphFormCanvas.psWidgets, &multigraphContainer);
	canvasInsertAtTop(&g_sGraphFormCanvas.psWidgets, &graphOverlayContainer);
	canvasInsertAtTop(&g_sGraphFormCanvas.psWidgets, &multigraphOverlayContainer);

	
	useNavigationButtons(&g_sGraphFormCanvas);

	g_i16GraphFormID = formManagerAddForm(&g_sGraphFormCanvas);
}



