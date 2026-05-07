
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
gfx_GenericWidget graphCursorContainer;
gfx_GenericWidget graphCursor2Container;
gfx_GenericWidget cursorOverlayContainer;
gfx_GenericWidget statsPanelContainer;
gfx_GenericWidget statsTitleContainer;
gfx_GenericWidget statsAvgContainer;
gfx_GenericWidget statsMaxContainer;
gfx_GenericWidget statsMinContainer;

gfx_Graph graphWidget;
gfx_Graph graphSawtoothWidget;
gfx_Label graphTitle;
gfx_MultiGraph multigraphWidget;
gfx_GraphOverlay graphOverlayWidget;
gfx_GraphOverlay multigraphOverlayWidget;
gfx_GraphCursor graphCursorWidget;
gfx_GraphCursor graphCursor2Widget;
gfx_GraphOverlay graphCursorOverlayWidget;
gfx_Rectangle statsPanelWidget;
gfx_Label statsTitleWidget;
gfx_Label statsMaxWidget;
gfx_Label statsAvgWidget;
gfx_Label statsMinWidget;

float graphData[NUM_POINTS]; 
float sawtoothData[NUM_POINTS];
float multigraphData1[NUM_POINTS];
float multigraphData2[NUM_POINTS];

char avgBuffer[] = "Avg  100.23 [C]";
char maxBuffer[] = "Max  100.23 [C]";
char minBuffer[] = "Max  100.23 [C]";

static void onNewGraphValue(EventParam_t arg) {
	gfx_GraphAddPoint(&graphWidget, arg.f32);
	if(GetExecTimeMs() % 500 == 0) {
		sprintf(graphOverlayWidget.traces[0].valueText, "TC1: %.2f [C]", arg.f32);
		graphOverlayWidget.bIsDirty = true;

		float val = 0;
		if(graphCursorWidget.isVisible && graphWidget.isTraceVisible) {
			val = gfx_GraphCursorGetValue(&graphCursorWidget);
			sprintf(graphCursorOverlayWidget.traces[0].valueText, "C1: %.2f [C]", val);
			graphCursorOverlayWidget.bIsDirty = true;
		}
	
		if(graphCursor2Widget.isVisible && graphWidget.isTraceVisible) {
			val = gfx_GraphCursorGetValue(&graphCursor2Widget);
			sprintf(graphCursorOverlayWidget.traces[1].valueText, "C2: %.2f [C]", val);
			graphCursorOverlayWidget.bIsDirty = true;
		}

		if(graphCursor2Widget.isVisible && graphWidget.isTraceVisible && graphCursorWidget.isVisible) {
			float avg = gfx_GraphGetAverageBetweenCursors(&graphCursorWidget, &graphCursor2Widget);
			snprintf(avgBuffer, sizeof(avgBuffer), "Avg   %.2f [C]", avg);
			statsAvgWidget.bIsDirty = true;

			float max = gfx_GraphGetMaxBetweenCursors(&graphCursorWidget, &graphCursor2Widget);
			snprintf(maxBuffer, sizeof(maxBuffer), "Max   %.2f [C]", max);
			statsMaxWidget.bIsDirty = true;

			float min = gfx_GraphGetMinBetweenCursors(&graphCursorWidget, &graphCursor2Widget);
			snprintf(minBuffer, sizeof(minBuffer), "Min   %.2f [C]", min);
			statsMinWidget.bIsDirty = true;
		}
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

static void onOverlayToggle(gfx_GraphOverlay *over, int trace) {
	if(trace >= over->numTraces) return;
	
	if(over->traces[trace].isVisible) {
		over->traces[trace].isVisible = false;
		graphWidget.isTraceVisible = false;
	} else {
		over->traces[trace].isVisible = true;
		graphWidget.isTraceVisible = true;
	}

	over->bIsDirty = true;
}

static void onCursorOverlayToggle(gfx_GraphOverlay *over, int trace) {
	if(trace >= over->numTraces) return;
	
	if(over->traces[trace].isVisible) {
		over->traces[trace].isVisible = false;
		if(trace == 0) {
			graphCursorWidget.isVisible = false;
			strcpy(graphCursorOverlayWidget.traces[0].valueText, "C1: --");
			strcpy(avgBuffer, "Avg  --");
			strcpy(maxBuffer, "Max  --");
			strcpy(minBuffer, "Min  --");
			statsAvgWidget.bIsDirty = true;
			statsMaxWidget.bIsDirty = true;
			statsMinWidget.bIsDirty = true;

		}
		else if(trace == 1) {
			graphCursor2Widget.isVisible = false;
			strcpy(graphCursorOverlayWidget.traces[1].valueText, "C2: --");
			strcpy(avgBuffer, "Avg  --");
			strcpy(maxBuffer, "Max  --");
			strcpy(minBuffer, "Min  --");
			statsAvgWidget.bIsDirty = true;
			statsMaxWidget.bIsDirty = true;
			statsMinWidget.bIsDirty = true;
		}
		
	} else {
		over->traces[trace].isVisible = true;
		if(trace == 0)
			graphCursorWidget.isVisible = true;
		else if(trace == 1)
			graphCursor2Widget.isVisible = true;
	}

	over->bIsDirty = true;
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
		//.size.height = 162,
		.size.height = 162 * 2,
		// .size.width = 750,
		.size.width = 575,
		.pos.x = 20,
		.pos.y = 75,
		.bgColor = g_pCurrentTheme->palette.surface,
		.gridColor = g_pCurrentTheme->palette.textMuted,
		.lineColor = g_pCurrentTheme->palette.primary,
		.lineWidth = 2,
		.typo = TYPO_CAPTION,
		.textColor = g_pCurrentTheme->palette.textMuted,
		.bShowLabels = true,
		.totalPointsAdded = 0,
		.isTraceVisible = true,
	};
	memset(graphData, 0, NUM_POINTS * 2);
	
	//gfx_GraphAddPoint(&graphWidget, 50);
	graphWidgetContainer.eWidgetType = WD_TYPE_GRAPH;
	graphWidgetContainer.pvWidget = (void *)&graphWidget;

	graphCursorWidget = (gfx_GraphCursor){
        .name = "cursor1",      // <- IMPORTANTE
        .bIsDirty = false,      // <- IMPORTANTE: Evita el spam del SPI
        // .pos y .size se inicializarán en 0 por defecto, lo cual está bien
        .parent = (const gfx_Graph *)&graphWidget,
        .relX = 200,
        .relY = 10,
        .type = CURSOR_VERTICAL,
        .lineWidth = 3,
        .color = COLOR_BLUE_PRIMARY,
        .isVisible = true,
        .alpha = 155,
    };
	gfx_initRegTouch((void *)&graphCursorWidget, WD_TYPE_GRAPH_CURSOR);
	graphCursorContainer.eWidgetType = WD_TYPE_GRAPH_CURSOR;
	graphCursorContainer.pvWidget = (void *)&graphCursorWidget;

	graphCursor2Widget = (gfx_GraphCursor){
        .name = "cursor2",      // <- IMPORTANTE
        .bIsDirty = false,      // <- IMPORTANTE: Evita el spam del SPI
        // .pos y .size se inicializarán en 0 por defecto, lo cual está bien
        .parent = (const gfx_Graph *)&graphWidget,
        .relX = 300,
        .relY = 10,
        .type = CURSOR_VERTICAL,
        .lineWidth = 3,
        .color = COLOR_SOFT_RED,
        .isVisible = true,
        .alpha = 155,
    };
	gfx_initRegTouch((void *)&graphCursor2Widget, WD_TYPE_GRAPH_CURSOR);
	graphCursor2Container.eWidgetType = WD_TYPE_GRAPH_CURSOR;
	graphCursor2Container.pvWidget = (void *)&graphCursor2Widget;

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
	            .isVisible = true,
				.enableTrace = true,
	        }
	    },
	    .bIsDirty = true,
		.onTraceToggle = onOverlayToggle,
	};

	gfx_initRegTouch((void *)&graphOverlayWidget, WD_TYPE_GRAPH_OVERLAY);

	graphOverlayContainer.eWidgetType = WD_TYPE_GRAPH_OVERLAY;
	graphOverlayContainer.pvWidget = (void *)&graphOverlayWidget;

	gfx_GetStringDimensions("CX: 62.49 [C]", Theme_ResolveFontId(TYPO_MONO), &strWidth, &strHeight, 1);

	uint16_t cursorOverlayWidth = strWidth * 1.3f;
	uint16_t cursorOverlayHeight = strHeight * 1.6f;
	graphCursorOverlayWidget = (gfx_GraphOverlay){
	    .name = "cursorOverlay",
	    .pos.x = graphWidget.pos.x + graphWidget.size.width + 10, 
	    .pos.y = graphWidget.pos.y,        
	    .size.width = cursorOverlayWidth,
	    .size.height = cursorOverlayHeight * 2, 
	    .bgColor = g_pCurrentTheme->palette.surface, 
	    .textColor = g_pCurrentTheme->palette.textMain, // Make it pop more than textMuted
	    .typo = TYPO_CAPTION,
	    // --- DATA ---
	    .numTraces = 2,
	    .traces = {
	        [0] = {
	            // Must strictly match the graph's lineColor to make visual sense
	            .color = graphCursorWidget.color, 
	            .valueText = "C1: 62.49 [C]", // Default state before first event
	            .isVisible = true,
				.enableTrace = true,
	        }, 
	        [1] = {
	            // Must strictly match the graph's lineColor to make visual sense
	            .color = graphCursor2Widget.color, 
	            .valueText = "C2: 62.49 [C]", // Default state before first event
	            .isVisible = true,
				.enableTrace = true,
	        }
	    },
	    .bIsDirty = true,
		.onTraceToggle = onCursorOverlayToggle,
	};

	gfx_initRegTouch((void *)&graphCursorOverlayWidget, WD_TYPE_GRAPH_OVERLAY);

	cursorOverlayContainer.eWidgetType = WD_TYPE_GRAPH_OVERLAY;
	cursorOverlayContainer.pvWidget = (void *)&graphCursorOverlayWidget;

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
		.totalPointsAdded = {0},
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

	int16_t posY = graphCursorOverlayWidget.pos.y + graphCursorOverlayWidget.size.height + 15;
	statsPanelWidget = (gfx_Rectangle){
		.pos.x = graphWidget.pos.x + graphWidget.size.width + 15,
		.pos.y = posY, 
		.dim.width = cursorOverlayWidth,
		.dim.height = graphWidget.pos.y + graphWidget.size.height - posY,
		.color = g_pCurrentTheme->palette.surface,
		.borderWidth = 1,
		.round = 5,
		.name = "statsPanel",
	};

	statsPanelContainer.eWidgetType = WD_TYPE_RECT;
	statsPanelContainer.pvWidget = (void *)&statsPanelWidget;

	statsTitleWidget = (gfx_Label){
		.text = "Stats",
		.alignment = ALIGN_CENTER,
		.isVisible = true,
		.pos.x = statsPanelWidget.pos.x + statsPanelWidget.dim.width / 2,
		.pos.y = statsPanelWidget.pos.y + 10,
		.style = STYLE_TEXT_MAIN,
		.typo = TYPO_MONO_BOLD,
	};
	statsTitleContainer.eWidgetType = WD_TYPE_LABEL;
	statsTitleContainer.pvWidget = (void *)&statsTitleWidget;

	statsAvgWidget = (gfx_Label){
		.text = avgBuffer,
		.name = "avgLb",
		.alignment = ALIGN_CENTER,
		.pos.x = statsPanelWidget.pos.x + statsPanelWidget.dim.width / 2,
		.pos.y = statsTitleWidget.pos.y + 35,
		.style = STYLE_SECONDARY,
		.typo = TYPO_MONO,
		.isVisible = true,
	};
	statsAvgContainer.eWidgetType = WD_TYPE_LABEL;
	statsAvgContainer.pvWidget = (void *)&statsAvgWidget;

	statsMaxWidget = (gfx_Label){
		.text = maxBuffer,
		.name = "avgLb",
		.alignment = ALIGN_CENTER,
		.pos.x = statsPanelWidget.pos.x + statsPanelWidget.dim.width / 2,
		.pos.y = statsAvgWidget.pos.y + 35,
		.style = STYLE_SECONDARY,
		.typo = TYPO_MONO,
		.isVisible = true,
	};
	statsMaxContainer.eWidgetType = WD_TYPE_LABEL;
	statsMaxContainer.pvWidget = (void *)&statsMaxWidget;

	statsMinWidget = (gfx_Label){
		.text = minBuffer,
		.name = "avgLb",
		.alignment = ALIGN_CENTER,
		.pos.x = statsPanelWidget.pos.x + statsPanelWidget.dim.width / 2,
		.pos.y = statsMaxWidget.pos.y + 35,
		.style = STYLE_SECONDARY,
		.typo = TYPO_MONO,
		.isVisible = true,
	};
	statsMinContainer.eWidgetType = WD_TYPE_LABEL;
	statsMinContainer.pvWidget = (void *)&statsMinWidget;
	
	Event_Subscribe(EVT_SYS_NEW_GRAPH_VALUE, ( EventHandler_fn )onNewGraphValue );
	Event_Subscribe(EVT_SYS_NEW_SAWTOOTH_VALUE, (EventHandler_fn)onSawtoothGraphValue);
	Event_Subscribe(EVT_SYS_NEW_GRAPH_VALUE, ( EventHandler_fn )onMultigraphSine );
	Event_Subscribe(EVT_SYS_NEW_SAWTOOTH_VALUE, (EventHandler_fn)onMultigraphSawtoothValue);

	useFullHeader(&g_sGraphFormCanvas);
	canvasInsertAtTop(&g_sGraphFormCanvas.psWidgets, &graphTitleContainer);
	canvasInsertAtTop(&g_sGraphFormCanvas.psWidgets, &graphWidgetContainer);
 	// canvasInsertAtTop(&g_sGraphFormCanvas.psWidgets, &multigraphContainer);
	canvasInsertAtTop(&g_sGraphFormCanvas.psWidgets, &graphOverlayContainer);
	// canvasInsertAtTop(&g_sGraphFormCanvas.psWidgets, &multigraphOverlayContainer);
	canvasInsertAtTop(&g_sGraphFormCanvas.psWidgets, &graphCursorContainer);
	canvasInsertAtTop(&g_sGraphFormCanvas.psWidgets, &graphCursor2Container);
	canvasInsertAtTop(&g_sGraphFormCanvas.psWidgets, &cursorOverlayContainer);
	canvasInsertAtTop(&g_sGraphFormCanvas.psWidgets, &statsPanelContainer);
	canvasInsertAtTop(&g_sGraphFormCanvas.psWidgets, &statsTitleContainer);
	canvasInsertAtTop(&g_sGraphFormCanvas.psWidgets, &statsAvgContainer);
	canvasInsertAtTop(&g_sGraphFormCanvas.psWidgets, &statsMaxContainer);
	canvasInsertAtTop(&g_sGraphFormCanvas.psWidgets, &statsMinContainer);

	
	useNavigationButtons(&g_sGraphFormCanvas);

	g_i16GraphFormID = formManagerAddForm(&g_sGraphFormCanvas);
}



