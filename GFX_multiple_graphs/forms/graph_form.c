
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

#define NUM_POINTS	1000

static gfx_Canvas g_sGraphFormCanvas;
int16_t g_i16GraphFormID = 0;

gfx_GenericWidget graphWidgetContainer;
gfx_GenericWidget graphTitleContainer;

gfx_Graph graphWidget;
gfx_Label graphTitle;

int16_t graphData[NUM_POINTS]; 

static void onNewGraphValue(uint32_t arg) {
	gfx_GraphAddPoint(&graphWidget, (int16_t)arg);
}

void initGraphForm(void) {
	
	g_sGraphFormCanvas.ui16BackgroundColor = g_pCurrentTheme->palette.background;
	g_sGraphFormCanvas.psWidgets = NULL;

	graphTitle = (gfx_Label) {
        .text = "Graph",
		.name = "titulo",
        .pos.x = LCD_WIDTH / 2,
        .pos.y = 50,
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
		.minY = -10,
		.maxY = 110,
		.gridLinesX = 4,
		.gridLinesY = 4,
		.size.height = 250,
		.size.width = 600,
		.pos.x = 100,
		.pos.y = 100,
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

	Event_Subscribe(EVT_SYS_NEW_GRAPH_VALUE, ( EventHandler_fn )onNewGraphValue );

	canvasInsertAtTop(&g_sGraphFormCanvas.psWidgets, &graphWidgetContainer);
	canvasInsertAtTop(&g_sGraphFormCanvas.psWidgets, &graphTitleContainer);
	
	useNavigationButtons(&g_sGraphFormCanvas);

	g_i16GraphFormID = formManagerAddForm(&g_sGraphFormCanvas);
}



