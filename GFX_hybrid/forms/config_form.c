
#include <stdlib.h>
#include "gfx.h"
#include "common_widgets.h"
#include "gfx_canvas.h"
#include "gfx_theme.h"
#include "forms_manager.h"

#include "config_form.h"

int16_t g_i16ConfigFormID;

static gfx_Canvas g_sConfigCanvas;

void initConfigForm(void) {
	g_sConfigCanvas.ui16BackgroundColor = g_pCurrentTheme->palette.background;
	g_sConfigCanvas.psWidgets = NULL;

	useFullHeader(&g_sConfigCanvas);
	useNavigationButtons(&g_sConfigCanvas);
	
	g_i16ConfigFormID = formManagerAddForm(&g_sConfigCanvas);
}