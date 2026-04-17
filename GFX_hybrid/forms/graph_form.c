
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

#include "graph_form.h"

// #define NUM_POINTS	800 - 40
#define NUM_POINTS	500


static gfx_Canvas g_sGraphFormCanvas;
int16_t g_i16GraphFormID = 0;

gfx_GenericWidget graphWidgetContainer;
gfx_GenericWidget graphTitleContainer;
gfx_GenericWidget graphSawtoothContainer;
gfx_GenericWidget multigraphContainer;

gfx_Graph graphWidget;
gfx_Graph graphSawtoothWidget;
gfx_Label graphTitle;
gfx_MultiGraph multigraphWidget;

int16_t graphData[NUM_POINTS]; 
int16_t sawtoothData[NUM_POINTS];
int16_t multigraphData1[NUM_POINTS];
int16_t multigraphData2[NUM_POINTS];


static void onNewGraphValue(uint32_t arg) {
	gfx_GraphAddPoint(&graphWidget, (int16_t)arg);
}

static void onSawtoothGraphValue(uint32_t arg) {
	gfx_GraphAddPoint(&graphSawtoothWidget, (int16_t)arg);
}

static void onMultigraphSine(uint32_t arg) {
	gfx_MultiGraphAddData(&multigraphWidget, 0, (int16_t)arg);
}

static void onMultigraphSawtoothValue(uint32_t arg) {
	gfx_MultiGraphAddData(&multigraphWidget, 1, (int16_t)arg);
}


void initGraphForm(void) {
	
	g_sGraphFormCanvas.ui16BackgroundColor = g_pCurrentTheme->palette.background;
	g_sGraphFormCanvas.psWidgets = NULL;

	graphTitle = (gfx_Label) {
        .text = "Graph",
		.name = "titulo",
        .pos.x = LCD_WIDTH / 2,
        .pos.y = 25,
        .oldPos.x = LCD_WIDTH / 2,
        .oldPos.y = 50,
        .alignment = ALIGN_CENTER,
        .typo = TYPO_H1,           
        .style = STYLE_DANGER,     
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
		.gridLinesY = 4,
		.size.height = 140,
		.size.width = 750,
		.pos.x = 20,
		.pos.y = 20,
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

	graphSawtoothWidget = (gfx_Graph){
		.name = "sawtooth_graph",
		.data = sawtoothData,
		.maxPoints = NUM_POINTS,
		.head = 0,
		.minY = 10,
		.maxY = 90,
		.gridLinesX = 4,
		.gridLinesY = 4,
		.size.height = 140,
		.size.width = 750,
		.pos.x = 20,
		.pos.y = 175,
		.bgColor = g_pCurrentTheme->palette.surface,
		.gridColor = g_pCurrentTheme->palette.textMuted,
		.lineColor = g_pCurrentTheme->palette.danger,
		.lineWidth = 2,
		.typo = TYPO_CAPTION,
		.textColor = g_pCurrentTheme->palette.textMuted,
		.bShowLabels = true,
	};
	memset(sawtoothData, 0, NUM_POINTS * 2);
	
	//gfx_GraphAddPoint(&graphWidget, 50);
	graphSawtoothContainer.eWidgetType = WD_TYPE_GRAPH;
	graphSawtoothContainer.pvWidget = (void *)&graphSawtoothWidget;

	multigraphWidget = (gfx_MultiGraph){
		.name = "sawtooth_graph",
		.maxPoints = NUM_POINTS,
		.activeTraces = 2,
		.minY = 10,
		.maxY = 90,
		.gridLinesX = 4,
		.gridLinesY = 4,
		.size.height = 140,
		.size.width = 750,
		.pos.x = 20,
		.pos.y = 330,
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
	
	//gfx_GraphAddPoint(&graphWidget, 50);
	multigraphContainer.eWidgetType = WD_TYPE_MULTIGRAPH;
	multigraphContainer.pvWidget = (void *)&multigraphWidget;

	Event_Subscribe(EVT_SYS_NEW_GRAPH_VALUE, ( EventHandler_fn )onNewGraphValue );
	Event_Subscribe(EVT_SYS_NEW_SAWTOOTH_VALUE, (EventHandler_fn)onSawtoothGraphValue);
	Event_Subscribe(EVT_SYS_NEW_GRAPH_VALUE, ( EventHandler_fn )onMultigraphSine );
	Event_Subscribe(EVT_SYS_NEW_SAWTOOTH_VALUE, (EventHandler_fn)onMultigraphSawtoothValue);

	canvasInsertAtTop(&g_sGraphFormCanvas.psWidgets, &graphWidgetContainer);
	canvasInsertAtTop(&g_sGraphFormCanvas.psWidgets, &graphSawtoothContainer);
	canvasInsertAtTop(&g_sGraphFormCanvas.psWidgets, &multigraphContainer);
	//canvasInsertAtTop(&g_sGraphFormCanvas.psWidgets, &graphTitleContainer);
	
	useNavigationButtons(&g_sGraphFormCanvas);

	g_i16GraphFormID = formManagerAddForm(&g_sGraphFormCanvas);
}



