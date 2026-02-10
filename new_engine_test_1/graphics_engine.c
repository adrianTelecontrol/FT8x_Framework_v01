/**
 *
 * 
 */

#include <stdlib.h>
#include <string.h>

#include "EVE.h"
#include "helpers.h"

#include <graphics_engine.h>


#define WORKING_LAYERS	2

#define GFX_LAYER_0		0
#define GFX_LAYER_1		1

GfxLayer_t g_sWorkingLayers[WORKING_LAYERS];

static uint8_t g_ui8WorkingLayer = GFX_LAYER_0;
static const char TASK_NAME[] = "GFX_ENGINE";

bool Gfx_initEngine(const uint16_t ui16ResWidth, const uint16_t ui16ResHeight)
{
	// Initialize working buffers
	GfxLayer_t *gfx = &g_sWorkingLayers[GFX_LAYER_0];
	gfx->ui16Height = ui16ResHeight;
	gfx->ui16Width = ui16ResWidth;
	gfx->ui32BuffSize = ui16ResHeight * ui16ResWidth;
	gfx->psPixelBuffer = (pixel16_t *)malloc(sizeof(pixel16_t) * gfx->ui32BuffSize);

	memset(gfx->psPixelBuffer, 0, gfx->ui32BuffSize * 2);

	gfx = &g_sWorkingLayers[GFX_LAYER_1];
	gfx->ui16Height = ui16ResHeight;
	gfx->ui16Width = ui16ResWidth;
	gfx->ui32BuffSize = ui16ResHeight * ui16ResWidth;
	gfx->psPixelBuffer = (pixel16_t *)malloc(sizeof(pixel16_t) * gfx->ui32BuffSize);
	memset(gfx->psPixelBuffer, 0, gfx->ui32BuffSize * 2);

	return true;
}

void Gfx_loadIntoBuffer(uint32_t ui32Index, uint16_t ui16Pixel)
{
	if(ui32Index < g_sWorkingLayers[g_ui8WorkingLayer].ui32BuffSize)
		g_sWorkingLayers[g_ui8WorkingLayer].psPixelBuffer[ui32Index].u16 = ui16Pixel; 
}

void Gfx_render(void)
{
	uint8_t ui8PastLayer = GFX_LAYER_1;

	GfxLayer_t *gfxPast = &g_sWorkingLayers[ui8PastLayer];
	GfxLayer_t *gfxCurr = &g_sWorkingLayers[g_ui8WorkingLayer];

	uint32_t ui32Index = 0;
	for(ui32Index = 0; ui32Index < 1000; ui32Index++)
	{
		pixel16_t ui16CurrVal = gfxCurr->psPixelBuffer[ui32Index];
		pixel16_t ui16PastVal = gfxPast->psPixelBuffer[ui32Index];
		if(ui16PastVal.u16 != ui16CurrVal.u16)
		{
			gfxPast[ui32Index].psPixelBuffer->u16 = ui16CurrVal.u16;
			API_LIB_WriteDataRAMG(ui16CurrVal.u8, sizeof(uint16_t), RAM_G + ui32Index);
		}
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
    uint16_t ui16Stride = gfxPast->ui16Width * BytesPerPixel;

    // 1. Calculate Target Dimensions
    uint8_t u8Scale = 4;
    uint16_t u16DrawnWidth = gfxPast->ui16Width * u8Scale;
    uint16_t u16DrawnHeight = gfxPast->ui16Height * u8Scale;

    // 2. BITMAP_LAYOUT (Remains based on the SOURCE image size)
    API_BITMAP_LAYOUT(RGB565, ui16Stride, gfxPast->ui16Height);
    API_BITMAP_LAYOUT_H(ui16Stride >> 10, gfxPast->ui16Height >> 9);

    // 3. BITMAP_SIZE (Update this to the NEW scaled size on screen)
    //    We use NEAREST for clean integer scaling (pixel art look). 
    //    Use BILINEAR if you want it smoothed/blurry.
    API_BITMAP_SIZE(NEAREST, BORDER, BORDER, u16DrawnWidth, u16DrawnHeight);
    API_BITMAP_SIZE_H(u16DrawnWidth >> 9, u16DrawnHeight >> 9);

    // 4. TRANSFORM MATRIX
    //    We calculate the sampling step.
    //    To make it 4x BIGGER, we step 0.25x per pixel.
    //    16.16 Fixed Point: 65536 / 4 = 16384
    int32_t s32ScaleFactor = 65536 * u8Scale; 

    API_CMD_LOADIDENTITY();
    API_CMD_SCALE(s32ScaleFactor, s32ScaleFactor);
    API_CMD_SETMATRIX();

    API_BEGIN(BITMAPS);
    API_VERTEX2II(0, 0, 0, 0);
    API_END();

#ifdef MEASURE_PERF_ENABLE
	API_COLOR_RGB(255, 0, 0);
	API_CMD_NUMBER(LCD_WIDTH - 100, LCD_HEIGHT - 50, 30, 0, g_ui32ExecDurMs);
	//API_CMD_NUMBER(LCD_WIDTH / 2, LCD_HEIGHT / 2, 29, 0, g_ui32ExecDurMs);
	API_CMD_SETBASE(10);
#endif
	API_DISPLAY();
	API_CMD_SWAP();
	API_LIB_EndCoProList();
	API_LIB_AwaitCoProEmpty();
	
	TIVA_LOGI(TASK_NAME, "Displaying bitmap!");
}
