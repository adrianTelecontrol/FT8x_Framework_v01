/**
 */
#include <stdbool.h>
#include <stdint.h>

#include <driverlib/sysctl.h>

#include "EVE.h"

#include "FT8xx_params.h"
#include "helpers.h"
#include "tiva_log.h"

#include "image_loader.h"

#define MEASURE_PERF_ENABLE

static const char TASK_NAME[] = "IMAGE_LOADER";

void EVE_LoadPNG(const uint8_t *pui8PNGSrc, const uint32_t ui32PNGSize) {
	TIVA_LOGI(TASK_NAME, "Starting to load PNG");
	API_LIB_BeginCoProList();
	API_CMD_LOADIMAGE(0, 0);
	API_LIB_EndCoProList();
#ifdef MEASURE_PERF_ENABLE
	MEASURE_EXECUTION(API_LIB_WriteDataToCMD(pui8PNGSrc, ui32PNGSize));
#else
	API_LIB_WriteDataToCMD(TELEXIS_LOGO, TELEXIS_LOGO_SIZE);
#endif // MEASURE_PERF_ENABLE
	// API_LIB_WriteDataToCMD(TELECONTROL_LOGO, TELECONTROL_LOGO_SIZE);
	//  API_LIB_WriteDataRAMG(TELECONTROL_LOGO, TELECONTROL_LOGO_SIZE, 0);
	API_LIB_AwaitCoProEmpty();
	TIVA_LOGI(TASK_NAME, "PNG loaded to RAM_G");

	// SysCtlDelay(MS_2_CLK(50));

	API_LIB_BeginCoProList();
	API_CMD_GETPROPS(0, 0, 0);
	API_LIB_EndCoProList();
	API_LIB_AwaitCoProEmpty();

	uint16_t REG_CMD_WRITE_OFFSET = EVE_MemRead16(REG_CMD_WRITE);
	uint16_t ParameterAddr = ((REG_CMD_WRITE_OFFSET - 12) & 4095);
	uint16_t End_Address = EVE_MemRead16((RAM_CMD + ParameterAddr));

	ParameterAddr = ((REG_CMD_WRITE_OFFSET - 8) & 4095);
	uint16_t width = EVE_MemRead16((RAM_CMD + ParameterAddr));

	ParameterAddr = ((REG_CMD_WRITE_OFFSET - 4) & 4095);
	uint16_t height = EVE_MemRead16((RAM_CMD + ParameterAddr));

	API_LIB_BeginCoProList();
	API_CMD_DLSTART();
	API_CLEAR_COLOR_RGB(11, 19, 30);
	API_CLEAR(1, 1, 1);
	API_COLOR_RGB(255, 255, 255);

	API_BITMAP_HANDLE(0);
	API_BITMAP_SOURCE(RAM_G);
	API_BITMAP_LAYOUT(RGB565, (uint16_t)(width * 2), height);
	API_BITMAP_SIZE(NEAREST, BORDER, BORDER, width, height);
	API_BITMAP_LAYOUT_H((width * 2) >> 10, height >> 9);
	API_BITMAP_SIZE_H(width >> 9, height >> 9);

	// 2. SIZE: Update this to the NEW scaled size on screen
	//    We want it 2x bigger, so we multiply width/height by 2
	// float scale = 1.3;
	// uint16_t new_w = width * scale;
	// uint16_t new_h = height * scale;

	// 3. MATRIX: Apply scaling
	//    EVE Logic: To make image 2x BIGGER, we scale texture space by 0.5x
	//    Fixed Point 16.16 Math: 0.5 * 65536 = 32768
	// API_CMD_LOADIDENTITY();
	// API_CMD_SCALE(65536*scale, 65536*scale);
	// API_CMD_SETMATRIX();

	API_BEGIN(BITMAPS);
	API_VERTEX2II(100, 100, 0, 0);
	API_END();

	TIVA_LOGI(TASK_NAME, "PNG displayed in screen");
	API_DISPLAY();
	API_CMD_SWAP();
	API_LIB_EndCoProList();
	API_LIB_AwaitCoProEmpty();
}

void EVE_LoadBitmap(BitmapHandler_t *psBitmapHandler, const uint8_t ui8Flags)
{
	const uint32_t ui16Width = psBitmapHandler->sHeader.bitmap_width;
	const uint32_t ui16Height = psBitmapHandler->sHeader.bitmap_height;

	TIVA_LOGI(TASK_NAME, "Starting to load bitmap data to RAM_G");
	uint32_t ui32ImgRealSize = psBitmapHandler->sHeader.bitmap_width * 2 * psBitmapHandler->sHeader.bitmap_height;

	// Check operation mode
	if (ui8Flags & EVE_LOAD_IMG_POLLING) {
		TIVA_LOGI(TASK_NAME, "Bitmap loader: Polling load");
#ifdef MEASURE_PERF_ENABLE
		MEASURE_EXECUTION(
			API_LIB_WriteDataRAMG(psBitmapHandler->ui8Pixels, ui32ImgRealSize, RAM_G));
#else
		API_LIB_WriteDataRAMG(psBitmapHandler->ui8Pixels, ui32BmpSize, RAM_G);
#endif
	} else if (ui8Flags & EVE_LOAD_IMG_UDMA) {
		TIVA_LOGI(TASK_NAME, "Bitmap loader: uDMA load");
#ifdef MEASURE_PERF_ENABLE
		MEASURE_EXECUTION(
			API_LIB_WriteDataRAMG_uDMA(psBitmapHandler->ui8Pixels, ui32ImgRealSize, RAM_G));
#else
		API_LIB_WriteDataRAMG_FromSDRAM(psBitmapHandler->ui8Pixels, ui32BmpSize, RAM_G);
#endif
	}

	TIVA_LOGI(TASK_NAME, "Bitmap finished to load into RAM_G");

	API_LIB_BeginCoProList();
	API_CMD_DLSTART();
	API_CLEAR_COLOR_RGB(11, 19, 30);
	API_CLEAR(1, 1, 1);
	API_COLOR_RGB(255, 255, 255);

	API_BITMAP_HANDLE(0);
	API_BITMAP_SOURCE(RAM_G);
	uint16_t BytesPerPixel = 2; // RGB565 is 2 bytes
    //uint16_t ui16Stride = ((ui16Width * BytesPerPixel) + 3) & ~3;
    uint16_t ui16Stride = ui16Width * BytesPerPixel;

    // Use 'ui16Stride' instead of 'ui16Width * 2'
    API_BITMAP_LAYOUT(RGB565, ui16Stride, ui16Height);
	API_BITMAP_SIZE(BILINEAR, BORDER, BORDER, ui16Width, ui16Height);
	API_BITMAP_LAYOUT_H(ui16Stride >> 10, ui16Height >> 9);
	API_BITMAP_SIZE_H(ui16Width >> 9, ui16Height >> 9);

	// 2. SIZE: Update this to the NEW scaled size on screen
	//    We want it 2x bigger, so we multiply width/height by 2
	// float scale = 1.3;
	// uint16_t new_w = width * scale;
	// uint16_t new_h = height * scale;

	// 3. MATRIX: Apply scaling
	//    EVE Logic: To make image 2x BIGGER, we scale texture space by 0.5x
	//    Fixed Point 16.16 Math: 0.5 * 65536 = 32768
	// API_CMD_LOADIDENTITY();
	// API_CMD_SCALE(65536*scale, 65536*scale);
	// API_CMD_SETMATRIX();

	// Apply the Flip Matrix (If standard bottom-up BMP)
    // API_CMD_LOADIDENTITY();
    // API_CMD_SCALE(-1 * 65536, 1 * 65536);
    // //API_CMD_TRANSLATE(0, -ui16Height * 65536);
    // API_CMD_TRANSLATE(-ui16Width* 65536, 0);
    // API_CMD_SETMATRIX();

	API_BEGIN(BITMAPS);
	API_VERTEX2II(0, 0, 0, 0);
	API_END();

#ifdef MEASURE_PERF_ENABLE
	API_COLOR_RGB(255, 0, 0);
	API_CMD_NUMBER(LCD_WIDTH - 100, LCD_HEIGHT - 50, 30, 0, g_ui32ExecDurMs);
	API_CMD_SETBASE(10);
#endif
	API_DISPLAY();
	API_CMD_SWAP();
	API_LIB_EndCoProList();
	API_LIB_AwaitCoProEmpty();
	
	TIVA_LOGI(TASK_NAME, "Displaying bitmap!");
}