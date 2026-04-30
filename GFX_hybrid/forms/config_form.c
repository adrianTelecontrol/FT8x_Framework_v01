
#include <stdlib.h>
#include <stdio.h>

#include "gfx.h"
#include "gfx_canvas.h"
#include "gfx_theme.h"
#include "forms_manager.h"
#include "common_widgets.h"
#include "FT8xx_params.h"
#include "file_manager.h"
#include "rtc_module.h"

#include "config_form.h"

gfx_GenericWidget writeLogButtonContainer;

gfx_Button writeLogWidget;

int16_t g_i16ConfigFormID;

static gfx_Canvas g_sConfigCanvas;

static void onWriteLogButtonReleased(gfx_Button *btn) {
	onGenericBtnRelease(btn);
	
	char pcData[120];
    char dateStr[12];
	char timeStr[12];
	// char psFilePath[50];
	RTC_getFormattedDate(dateStr, sizeof(dateStr));
	RTC_getFormattedTime(timeStr, sizeof(timeStr));
	snprintf(pcData, sizeof(pcData), "%s %s - %s", dateStr, timeStr, "This is just to test if the usb and log engine is working! Please ignore me :)");
	
	FM_WriteFile(DRIVE_USB, "test2.txt", (uint8_t *)pcData, sizeof(pcData));
}

void initConfigForm(void) {
	g_sConfigCanvas.ui16BackgroundColor = g_pCurrentTheme->palette.background;
	g_sConfigCanvas.psWidgets = NULL;

	writeLogWidget = (gfx_Button) {
		.label = "Write Log",
		.pos.x = LCD_WIDTH / 2 - 150,
		.pos.y = LCD_HEIGHT / 2 - 60,
		.size.width = 300,
		.size.height = 120,
		.borderWidth = 1,
		.radius = 6,
		.style = STYLE_PRIMARY,
		.typo = TYPO_H1,
		.onPosChanged = NULL,
		.onPressed = onGenericBtnPressed,
		.onRelease = onWriteLogButtonReleased,
	};
	gfx_initRegTouch((void *)&writeLogWidget, WD_TYPE_BUTTON);
	writeLogButtonContainer.eWidgetType = WD_TYPE_BUTTON;
	writeLogButtonContainer.pvWidget = (void *)&writeLogWidget;

	useFullHeader(&g_sConfigCanvas);
	useNavigationButtons(&g_sConfigCanvas);
	canvasInsertAtTop(&g_sConfigCanvas.psWidgets, &writeLogButtonContainer);
	
	g_i16ConfigFormID = formManagerAddForm(&g_sConfigCanvas);
}