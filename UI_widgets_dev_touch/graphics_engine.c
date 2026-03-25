/**
 * graphics_engine.c
 *
 * Hybrid Rendering Engine:
 * 1. Scatter-Gather Linked Lists for full-screen (768KB) transitions.
 * 2. Asynchronous Dirty Rectangle Queue for high-speed UI widget updates.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <driverlib/rom_map.h>
#include <driverlib/sysctl.h>
#include <driverlib/udma.h>
#include <driverlib/interrupt.h>
#include <inc/hw_memmap.h>
#include <inc/hw_ssi.h>
#include <inc/hw_types.h>
#include <inc/hw_ints.h>

#include "EVE.h"
#include "graphics_engine.h"
#include "hal_spi.h"
#include "FT8xx_params.h"
#include "helpers.h"

// --- CONFIGURATION ---
#define GFX_WIDTH 800
#define GFX_HEIGHT 480
#define GFX_PIXELS_PER_FRAME (GFX_WIDTH * GFX_HEIGHT)
#define GFX_BUFFER_SIZE_BYTES (GFX_PIXELS_PER_FRAME * 2) // 768,000 Bytes

#define TRANSFER_SIZE 1024

#define GFX_ENABLE_INT
#define MEASURE_PERF_ENABLE
// #define MEASURE_PERF_ENABLE_FPS

// 768,000 / 1024 = 750 Total Tasks.
#define TASKS_LIST0 256 // 255 Data + 1 Link
#define TASKS_LIST1 256 // 255 Data + 1 Link
#define TASKS_LIST2 240 // 240 Data (Stops)

// --- GLOBALS ---
pixel16_t *g_pBufferA;
pixel16_t *g_pBufferB;

pixel16_t *g_pDrawingBuffer;
pixel16_t *g_pSendingBuffer;

extern uint8_t g_HAL_uDMA_ControlTable[1024];
#define PRIMARY_CTRL_TABLE ((tDMAControlTable *)g_HAL_uDMA_ControlTable)

static uint32_t g_ui32ProcDur = 0;

// --- FULL SCREEN SCATTER GATHER STRUCTURES ---
#if defined(__ICCARM__)
#pragma data_alignment = 1024
#elif defined(__TI_COMPILER_VERSION__)
#pragma DATA_ALIGN(g_txList0, 1024)
#pragma DATA_ALIGN(g_rxList0, 1024)
#pragma DATA_ALIGN(g_txList1, 1024)
#pragma DATA_ALIGN(g_rxList1, 1024)
#pragma DATA_ALIGN(g_txList2, 1024)
#pragma DATA_ALIGN(g_rxList2, 1024)
#endif
static tDMAControlTable g_txList0[TASKS_LIST0] __attribute__((aligned(1024)));
static tDMAControlTable g_rxList0[TASKS_LIST0] __attribute__((aligned(1024)));
static tDMAControlTable g_txList1[TASKS_LIST1] __attribute__((aligned(1024)));
static tDMAControlTable g_rxList1[TASKS_LIST1] __attribute__((aligned(1024)));
static tDMAControlTable g_txList2[TASKS_LIST2] __attribute__((aligned(1024)));
static tDMAControlTable g_rxList2[TASKS_LIST2] __attribute__((aligned(1024)));

static tDMAControlTable txLink1, txLink2;
static tDMAControlTable rxLink1, rxLink2;
static uint32_t g_ulDummyRx;

uint32_t g_ui32StartTxCycles;
uint32_t g_ui32EndTxCycles;


volatile DMAJobQueue_t g_DMAQueue = {0};

volatile RenderEngine_t g_RenderEngine = {RENDER_IDLE};

// --- HELPER FUNCTIONS ---
void Helper_FloatToString(char *buffer, uint32_t whole, uint32_t frac, bool bAddEnd) {
    char *ptr = buffer;
    uint32_t temp = whole;

    if (temp == 0) {
        *ptr++ = '0';
    } else {
        char *start = ptr;
        while (temp > 0) {
            *ptr++ = (temp % 10) + '0';
            temp /= 10;
        }
        char *end = ptr - 1;
        while (start < end) {
            char t = *start;
            *start++ = *end;
            *end-- = t;
        }
    }
    if(bAddEnd) *ptr++ = '\0';
}

void Helper_IntToFPSString(char *buffer, uint32_t whole, uint32_t frac) {
    char *ptr = buffer;
    Helper_FloatToString(buffer, whole, frac, false);
    *ptr++ = '.';
    *ptr++ = (frac % 10) + '0';
    *ptr++ = ' '; *ptr++ = 'F'; *ptr++ = 'P'; *ptr++ = 'S'; *ptr++ = '\0';
}

void Gfx_setProcessDuration(uint32_t ui32ProcDur) {
    g_ui32ProcDur = ui32ProcDur;
}

// --- BUILDER FUNCTIONS ---
void Gfx_BuildDynamicSG(uint8_t *pSrc, uint32_t totalBytes) {
    if (totalBytes == 0) return;

    uint32_t bytesRem = totalBytes;
    uint32_t chunk;
    uint32_t mode;
    int tCount = 0;

    // Pre-calculate links just in case we need to chain lists
    MAP_uDMAChannelScatterGatherSet(UDMA_CH15_SSI3TX, TASKS_LIST2, g_txList2, 1);
    txLink2 = PRIMARY_CTRL_TABLE[15 & 0x1F];
    MAP_uDMAChannelScatterGatherSet(UDMA_CH14_SSI3RX, TASKS_LIST2, g_rxList2, 1);
    rxLink2 = PRIMARY_CTRL_TABLE[14 & 0x1F];

    MAP_uDMAChannelScatterGatherSet(UDMA_CH15_SSI3TX, TASKS_LIST1, g_txList1, 1);
    txLink1 = PRIMARY_CTRL_TABLE[15 & 0x1F];
    MAP_uDMAChannelScatterGatherSet(UDMA_CH14_SSI3RX, TASKS_LIST1, g_rxList1, 1);
    rxLink1 = PRIMARY_CTRL_TABLE[14 & 0x1F];

    // --- POPULATE LIST 0 ---
	int i = 0;
    for (; i < TASKS_LIST0 - 1; i++) {
        if (bytesRem == 0) break;
        
        chunk = (bytesRem > TRANSFER_SIZE) ? TRANSFER_SIZE : bytesRem;
        mode = (bytesRem <= TRANSFER_SIZE) ? UDMA_MODE_BASIC : UDMA_MODE_PER_SCATTER_GATHER;

        g_txList0[i] = (tDMAControlTable)uDMATaskStructEntry(
            chunk, UDMA_SIZE_8, UDMA_SRC_INC_8, pSrc, UDMA_DST_INC_NONE, (void *)(SSI3_BASE + SSI_O_DR), UDMA_ARB_4, mode);
        g_rxList0[i] = (tDMAControlTable)uDMATaskStructEntry(
            chunk, UDMA_SIZE_8, UDMA_SRC_INC_NONE, (void *)(SSI3_BASE + SSI_O_DR), UDMA_DST_INC_NONE, &g_ulDummyRx, UDMA_ARB_4, mode);

        pSrc += chunk;
        bytesRem -= chunk;
        tCount++;

        if (mode == UDMA_MODE_BASIC) {
            MAP_uDMAChannelScatterGatherSet(UDMA_CH15_SSI3TX, tCount, g_txList0, 1);
            MAP_uDMAChannelScatterGatherSet(UDMA_CH14_SSI3RX, tCount, g_rxList0, 1);
            return; // We finished within List 0!
        }
    }

    // Add Chain Link to List 1
    g_txList0[tCount] = (tDMAControlTable)uDMATaskStructEntry(
        4, UDMA_SIZE_32, UDMA_SRC_INC_32, &txLink1, UDMA_DST_INC_32, &PRIMARY_CTRL_TABLE[15], UDMA_ARB_4, UDMA_MODE_PER_SCATTER_GATHER);
    g_rxList0[tCount] = (tDMAControlTable)uDMATaskStructEntry(
        4, UDMA_SIZE_32, UDMA_SRC_INC_32, &rxLink1, UDMA_DST_INC_32, &PRIMARY_CTRL_TABLE[14], UDMA_ARB_4, UDMA_MODE_PER_SCATTER_GATHER);
    tCount++;

    // --- POPULATE LIST 1 ---
    tCount = 0;
    for (i = 0; i < TASKS_LIST1 - 1; i++) {
        if (bytesRem == 0) break;
        
        chunk = (bytesRem > TRANSFER_SIZE) ? TRANSFER_SIZE : bytesRem;
        mode = (bytesRem <= TRANSFER_SIZE) ? UDMA_MODE_BASIC : UDMA_MODE_PER_SCATTER_GATHER;

        g_txList1[i] = (tDMAControlTable)uDMATaskStructEntry(
            chunk, UDMA_SIZE_8, UDMA_SRC_INC_8, pSrc, UDMA_DST_INC_NONE, (void *)(SSI3_BASE + SSI_O_DR), UDMA_ARB_4, mode);
        g_rxList1[i] = (tDMAControlTable)uDMATaskStructEntry(
            chunk, UDMA_SIZE_8, UDMA_SRC_INC_NONE, (void *)(SSI3_BASE + SSI_O_DR), UDMA_DST_INC_NONE, &g_ulDummyRx, UDMA_ARB_4, mode);

        pSrc += chunk;
        bytesRem -= chunk;
        tCount++;

        if (mode == UDMA_MODE_BASIC) {
            MAP_uDMAChannelScatterGatherSet(UDMA_CH15_SSI3TX, tCount, g_txList1, 1);
            MAP_uDMAChannelScatterGatherSet(UDMA_CH14_SSI3RX, tCount, g_rxList1, 1);
            // Arm List 0 so it starts the chain!
            MAP_uDMAChannelScatterGatherSet(UDMA_CH15_SSI3TX, TASKS_LIST0, g_txList0, 1);
            MAP_uDMAChannelScatterGatherSet(UDMA_CH14_SSI3RX, TASKS_LIST0, g_rxList0, 1);
            return;
        }
    }

    // Add Chain Link to List 2
    g_txList1[tCount] = (tDMAControlTable)uDMATaskStructEntry(
        4, UDMA_SIZE_32, UDMA_SRC_INC_32, &txLink2, UDMA_DST_INC_32, &PRIMARY_CTRL_TABLE[15], UDMA_ARB_4, UDMA_MODE_PER_SCATTER_GATHER);
    g_rxList1[tCount] = (tDMAControlTable)uDMATaskStructEntry(
        4, UDMA_SIZE_32, UDMA_SRC_INC_32, &rxLink2, UDMA_DST_INC_32, &PRIMARY_CTRL_TABLE[14], UDMA_ARB_4, UDMA_MODE_PER_SCATTER_GATHER);
    
    // --- POPULATE LIST 2 ---
    tCount = 0;
    for (i = 0; i < TASKS_LIST2; i++) {
        if (bytesRem == 0) break;
        
        chunk = (bytesRem > TRANSFER_SIZE) ? TRANSFER_SIZE : bytesRem;
        mode = (bytesRem <= TRANSFER_SIZE) ? UDMA_MODE_BASIC : UDMA_MODE_PER_SCATTER_GATHER;

        g_txList2[i] = (tDMAControlTable)uDMATaskStructEntry(
            chunk, UDMA_SIZE_8, UDMA_SRC_INC_8, pSrc, UDMA_DST_INC_NONE, (void *)(SSI3_BASE + SSI_O_DR), UDMA_ARB_4, mode);
        g_rxList2[i] = (tDMAControlTable)uDMATaskStructEntry(
            chunk, UDMA_SIZE_8, UDMA_SRC_INC_NONE, (void *)(SSI3_BASE + SSI_O_DR), UDMA_DST_INC_NONE, &g_ulDummyRx, UDMA_ARB_4, mode);

        pSrc += chunk;
        bytesRem -= chunk;
        tCount++;

        if (mode == UDMA_MODE_BASIC) {
            MAP_uDMAChannelScatterGatherSet(UDMA_CH15_SSI3TX, tCount, g_txList2, 1);
            MAP_uDMAChannelScatterGatherSet(UDMA_CH14_SSI3RX, tCount, g_rxList2, 1);
            // Arm List 0 to start the massive chain!
            MAP_uDMAChannelScatterGatherSet(UDMA_CH15_SSI3TX, TASKS_LIST0, g_txList0, 1);
            MAP_uDMAChannelScatterGatherSet(UDMA_CH14_SSI3RX, TASKS_LIST0, g_rxList0, 1);
            return;
        }
    }
}

void Gfx_BuildSG_For_Buffer(uint8_t *pBuffer) {
    uint8_t *pSrc = pBuffer;
    int i;

    // STEP 1: CALCULATE LINK STRUCTURES
    MAP_uDMAChannelScatterGatherSet(UDMA_CH15_SSI3TX, TASKS_LIST2, g_txList2, 1);
    txLink2 = PRIMARY_CTRL_TABLE[15 & 0x1F]; 
    MAP_uDMAChannelScatterGatherSet(UDMA_CH14_SSI3RX, TASKS_LIST2, g_rxList2, 1);
    rxLink2 = PRIMARY_CTRL_TABLE[14 & 0x1F]; 

    MAP_uDMAChannelScatterGatherSet(UDMA_CH15_SSI3TX, TASKS_LIST1, g_txList1, 1);
    txLink1 = PRIMARY_CTRL_TABLE[15 & 0x1F];
    MAP_uDMAChannelScatterGatherSet(UDMA_CH14_SSI3RX, TASKS_LIST1, g_rxList1, 1);
    rxLink1 = PRIMARY_CTRL_TABLE[14 & 0x1F];

    // STEP 2: POPULATE LIST 0
    for (i = 0; i < TASKS_LIST0 - 1; i++) {
        g_txList0[i] = (tDMAControlTable)uDMATaskStructEntry(
            TRANSFER_SIZE, UDMA_SIZE_8, UDMA_SRC_INC_8, pSrc, UDMA_DST_INC_NONE,
            (void *)(SSI3_BASE + SSI_O_DR), UDMA_ARB_4, UDMA_MODE_PER_SCATTER_GATHER);
        g_rxList0[i] = (tDMAControlTable)uDMATaskStructEntry(
            TRANSFER_SIZE, UDMA_SIZE_8, UDMA_SRC_INC_NONE, (void *)(SSI3_BASE + SSI_O_DR), 
            UDMA_DST_INC_NONE, &g_ulDummyRx, UDMA_ARB_4, UDMA_MODE_PER_SCATTER_GATHER);
        pSrc += TRANSFER_SIZE;
    }
    g_txList0[TASKS_LIST0 - 1] = (tDMAControlTable)uDMATaskStructEntry(
        4, UDMA_SIZE_32, UDMA_SRC_INC_32, &txLink1, UDMA_DST_INC_32,
        &PRIMARY_CTRL_TABLE[15], UDMA_ARB_4, UDMA_MODE_PER_SCATTER_GATHER);
    g_rxList0[TASKS_LIST0 - 1] = (tDMAControlTable)uDMATaskStructEntry(
        4, UDMA_SIZE_32, UDMA_SRC_INC_32, &rxLink1, UDMA_DST_INC_32,
        &PRIMARY_CTRL_TABLE[14], UDMA_ARB_4, UDMA_MODE_PER_SCATTER_GATHER);

    // STEP 3: POPULATE LIST 1
    for (i = 0; i < TASKS_LIST1 - 1; i++) {
        g_txList1[i] = (tDMAControlTable)uDMATaskStructEntry(
            TRANSFER_SIZE, UDMA_SIZE_8, UDMA_SRC_INC_8, pSrc, UDMA_DST_INC_NONE,
            (void *)(SSI3_BASE + SSI_O_DR), UDMA_ARB_4, UDMA_MODE_PER_SCATTER_GATHER);
        g_rxList1[i] = (tDMAControlTable)uDMATaskStructEntry(
            TRANSFER_SIZE, UDMA_SIZE_8, UDMA_SRC_INC_NONE, (void *)(SSI3_BASE + SSI_O_DR), 
            UDMA_DST_INC_NONE, &g_ulDummyRx, UDMA_ARB_4, UDMA_MODE_PER_SCATTER_GATHER);
        pSrc += TRANSFER_SIZE;
    }
    g_txList1[TASKS_LIST1 - 1] = (tDMAControlTable)uDMATaskStructEntry(
        4, UDMA_SIZE_32, UDMA_SRC_INC_32, &txLink2, UDMA_DST_INC_32,
        &PRIMARY_CTRL_TABLE[15], UDMA_ARB_4, UDMA_MODE_PER_SCATTER_GATHER);
    g_rxList1[TASKS_LIST1 - 1] = (tDMAControlTable)uDMATaskStructEntry(
        4, UDMA_SIZE_32, UDMA_SRC_INC_32, &rxLink2, UDMA_DST_INC_32,
        &PRIMARY_CTRL_TABLE[14], UDMA_ARB_4, UDMA_MODE_PER_SCATTER_GATHER);

    // STEP 4: POPULATE LIST 2
    for (i = 0; i < TASKS_LIST2; i++) {
        uint32_t mode = (i == TASKS_LIST2 - 1) ? UDMA_MODE_BASIC : UDMA_MODE_PER_SCATTER_GATHER;
        g_txList2[i] = (tDMAControlTable)uDMATaskStructEntry(
            TRANSFER_SIZE, UDMA_SIZE_8, UDMA_SRC_INC_8, pSrc, UDMA_DST_INC_NONE,
            (void *)(SSI3_BASE + SSI_O_DR), UDMA_ARB_4, mode);
        g_rxList2[i] = (tDMAControlTable)uDMATaskStructEntry(
            TRANSFER_SIZE, UDMA_SIZE_8, UDMA_SRC_INC_NONE, (void *)(SSI3_BASE + SSI_O_DR), 
            UDMA_DST_INC_NONE, &g_ulDummyRx, UDMA_ARB_4, mode);
        pSrc += TRANSFER_SIZE;
    }

    // STEP 5: ARM LIST 0
    MAP_uDMAChannelScatterGatherSet(UDMA_CH15_SSI3TX, TASKS_LIST0, g_txList0, 1);
    MAP_uDMAChannelScatterGatherSet(UDMA_CH14_SSI3RX, TASKS_LIST0, g_rxList0, 1);
}

bool Gfx_BuildSg_For_Segments(pixel16_t *pActiveBuffer, int16_t x, int16_t y, int16_t w, int16_t h) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > 800) w = 800 - x;
    if (y + h > 480) h = 480 - y;
    if (w <= 0 || h <= 0) return true;

    uint16_t totalBytesPerRow = w * 2;
    // TM4C Hardware Limit: Maximum 1024 items per basic uDMA transfer
    const uint16_t MAX_DMA_TRANSFER = 1024; 

    int row = 0;
    for (; row < h; row++) {
        uint32_t screenOffset = ((y + row) * 800) + x;
        
        uint32_t currentDestRAMG = screenOffset * 2;
        uint8_t *pCurrentSDRAM = (uint8_t *)&pActiveBuffer[screenOffset];
        uint16_t bytesRemaining = totalBytesPerRow;

        // Slice wide rows into uDMA-safe chunks (<= 1024 bytes)
        while (bytesRemaining > 0) {
            if (g_DMAQueue.count >= MAX_DMA_JOBS) return false; // Queue full!

            uint16_t chunkLength = (bytesRemaining > MAX_DMA_TRANSFER) ? MAX_DMA_TRANSFER : bytesRemaining;

            DMARenderJob_t newJob;
            newJob.pSrcSDRAM = pCurrentSDRAM;
            newJob.destRAMG = currentDestRAMG;
            newJob.length = chunkLength;

            // Push to ring buffer safely
            MAP_IntDisable(INT_SSI3);
            g_DMAQueue.jobs[g_DMAQueue.head] = newJob;
            g_DMAQueue.head = (g_DMAQueue.head + 1) % MAX_DMA_JOBS;
            g_DMAQueue.count++;
            MAP_IntEnable(INT_SSI3);

            // Advance pointers for the next chunk of this row
            pCurrentSDRAM += chunkLength;
            currentDestRAMG += chunkLength;
            bytesRemaining -= chunkLength;
        }
    }
    return true;
}

// bool Gfx_BuildSg_For_Segments(pixel16_t *pActiveBuffer, int16_t x, int16_t y, int16_t w, int16_t h) {
//     if (x < 0) { w += x; x = 0; }
//     if (y < 0) { h += y; y = 0; }
//     if (x + w > LCD_WIDTH) w = LCD_WIDTH - x;
//     if (y + h > LCD_HEIGHT) h = LCD_HEIGHT - y;
//     if (w <= 0 || h <= 0) return true;
// 
//     uint16_t bytesPerRow = w * 2;
//     
// 	int row = 0;
//     for (; row < h; row++) {
//         if (g_DMAQueue.count >= MAX_DMA_JOBS) return false;
// 
//         uint32_t screenOffset = ((y + row) * LCD_WIDTH) + x;
//         
//         DMARenderJob_t newJob;
//         newJob.pSrcSDRAM = (uint8_t *)&pActiveBuffer[screenOffset];
//         newJob.destRAMG = screenOffset * 2; 
//         newJob.length = bytesPerRow;
// 
//         // Protect queue modification
//         g_DMAQueue.jobs[g_DMAQueue.head] = newJob;
//         g_DMAQueue.head = (g_DMAQueue.head + 1) % MAX_DMA_JOBS;
//         g_DMAQueue.count++;
//     }
//     return true;
// }

// --- ENGINE MEMORY FUNCTIONS ---
void Gfx_loadIntoBuffer(uint32_t ui32Index, uint16_t ui16Pixel) {
    g_pDrawingBuffer[ui32Index].u16 = ui16Pixel;
}

inline void Gfx_RestoreBackground_Fast(pixel16_t *pCleanBackground, pixel16_t *pDrawBuffer, 
                                              int16_t x, int16_t y, int16_t width, int16_t height) {
    if (x < 0) { width += x; x = 0; }
    if (y < 0) { height += y; y = 0; }
    if (x + width > 800) width = 800 - x;
    if (y + height > 480) height = 480 - y;
    if (width <= 0 || height <= 0) return;

	int16_t row = 0;
    for (; row < height; row++) {
        uint32_t offset = ((y + row) * 800) + x;
        uint32_t *pSrc32 = (uint32_t *)&pCleanBackground[offset];
        uint32_t *pDst32 = (uint32_t *)&pDrawBuffer[offset];
        int16_t colCount = width / 2;

        while (colCount--) {
            *pDst32++ = *pSrc32++;
        }
        if (width % 2 != 0) {
            pDrawBuffer[offset + width - 1] = pCleanBackground[offset + width - 1];
        }
    }
}

void Gfx_composite(pixel16_t *pLayer0, pixel16_t *pLayer1) {
    uint32_t *pSrc0 = (uint32_t *)pLayer0;
    uint32_t *pSrc1 = (uint32_t *)pLayer1;
    uint32_t *pDest = (uint32_t *)g_pDrawingBuffer;

    uint32_t ui32LoopCount = (800 * 480) / 2; 

    while (ui32LoopCount--) {
        uint32_t val0 = *pSrc0++;
        uint32_t val1 = *pSrc1++;
        uint32_t outPixelPair = 0;

        if ((val1 & 0x0000FFFF) == 0) outPixelPair |= (val0 & 0x0000FFFF);
        else outPixelPair |= (val1 & 0x0000FFFF);

        if ((val1 & 0xFFFF0000) == 0) outPixelPair |= (val0 & 0xFFFF0000);
        else outPixelPair |= (val1 & 0xFFFF0000);

        *pDest++ = outPixelPair;
    }
}

// --- HARDWARE INTERFACES ---
void DisplayBitmap(void) {
    uint16_t ui16Width = 800;
    uint16_t ui16Height = 480;

    API_LIB_BeginCoProList();
    API_CMD_DLSTART();
    API_CLEAR_COLOR_RGB(11, 19, 30);
    API_CLEAR(1, 1, 1);
    API_COLOR_RGB(255, 255, 255);

    API_BITMAP_HANDLE(0);
    API_BITMAP_SOURCE(RAM_G);
    uint16_t BytesPerPixel = 2; 
    uint16_t ui16Stride = ui16Width * BytesPerPixel;

    uint8_t u8Scale = 1;
    uint16_t u16DrawnWidth = ui16Width * u8Scale;
    uint16_t u16DrawnHeight = ui16Height * u8Scale;

    API_BITMAP_LAYOUT(RGB565, ui16Stride, ui16Height);
    API_BITMAP_LAYOUT_H(ui16Stride >> 10, ui16Height >> 9);
    API_BITMAP_SIZE(NEAREST, BORDER, BORDER, u16DrawnWidth, u16DrawnHeight);
    API_BITMAP_SIZE_H(u16DrawnWidth >> 9, u16DrawnHeight >> 9);

    int32_t s32ScaleFactor = 65536 * u8Scale;
    API_CMD_LOADIDENTITY();
    API_CMD_SCALE(s32ScaleFactor, s32ScaleFactor);
    API_CMD_SETMATRIX();

    API_BEGIN(BITMAPS);
    API_VERTEX2II(0, 0, 0, 0);
    API_END();

#ifdef MEASURE_PERF_ENABLE
#ifdef MEASURE_PERF_ENABLE_FPS
    uint32_t duration = (g_ui32EndTxCycles - g_ui32StartTxCycles) / (g_ui32SysClock / 1E6) / 1000;
    const uint32_t CLOCK_PERIOD = g_ui32SysClock / 1E6;
    uint32_t us_whole = duration / CLOCK_PERIOD;
    g_ui32ExecDurMs = us_whole / 1000;

    if (g_ui32ExecDurMs > 0) {
        uint32_t ui32FPS_x10 = 10000 / g_ui32ExecDurMs;
        uint32_t ui32Whole = ui32FPS_x10 / 10;
        uint32_t ui32Frac = ui32FPS_x10 % 10;

        char cFPSBuffer[16];
        Helper_IntToFPSString(cFPSBuffer, ui32Whole, ui32Frac);

        API_COLOR_RGB(255, 0, 0);
        API_CMD_LOADIDENTITY();
        API_CMD_SETMATRIX();
        API_CMD_TEXT(u16DrawnWidth - 120, u16DrawnHeight - 50, 30, 0, cFPSBuffer);
    }
    API_CMD_SETBASE(10);
#else
    uint32_t ui32MsElapsed = (g_ui32EndTxCycles - g_ui32StartTxCycles) / (g_ui32SysClock / 1E6) / 1000;
    API_COLOR_RGB(255, 0, 0);
    API_CMD_NUMBER(u16DrawnWidth - 100, u16DrawnHeight - 50, 30, 0, ui32MsElapsed);
    API_CMD_SETBASE(10);
#endif 
#endif 

    API_COLOR_RGB(255, 0, 0); 
    API_CMD_NUMBER(20, u16DrawnHeight - 50, 30, 0, g_ui32ProcDur);
    API_CMD_SETBASE(10);

    API_DISPLAY();
    API_CMD_SWAP();
    API_LIB_EndCoProList();
    API_LIB_AwaitCoProEmpty();
}

void Gfx_Start_SG_Transfer(void) {
    g_bSPI_TransferActive = true;

    MAP_uDMAErrorStatusClear();
    MAP_SSIIntDisable(SSI3_BASE, SSI_DMATX | SSI_DMARX | SSI_RXFF | SSI_RXOR | SSI_RXTO);
    MAP_SSIIntClear(SSI3_BASE, SSI_DMATX | SSI_DMARX | SSI_RXFF | SSI_RXOR | SSI_RXTO);

    uint32_t trash;
    while (MAP_SSIDataGetNonBlocking(SSI3_BASE, &trash));
    MAP_SSIIntClear(SSI3_BASE, SSI_RXOR | SSI_RXTO);

    MAP_uDMAChannelEnable(UDMA_CH14_SSI3RX);
    MAP_uDMAChannelEnable(UDMA_CH15_SSI3TX);
    MAP_SSIDMAEnable(SSI3_BASE, SSI_DMA_TX | SSI_DMA_RX);
#ifdef GFX_ENABLE_INT
    MAP_SSIIntEnable(SSI3_BASE, SSI_DMATX);
#endif
    MAP_uDMAChannelRequest(UDMA_CH15_SSI3TX);

#ifndef GFX_ENABLE_INT
    while (MAP_uDMAChannelIsEnabled(UDMA_CH15_SSI3TX)) {
        if (HWREG(SSI3_BASE + SSI_O_RIS) & SSI_RIS_RORRIS) {
            HWREG(SSI3_BASE + SSI_O_ICR) = SSI_ICR_RORIC;
        }
    }
    while (MAP_SSIBusy(SSI3_BASE));
    MAP_SSIDMADisable(SSI3_BASE, SSI_DMA_TX | SSI_DMA_RX);
	g_bSPI_TransferActive = false;
#endif
}

// --- INITIALIZATION ---
bool Gfx_initEngine(const uint16_t ui16ResWidth, const uint16_t ui16ResHeight) {
    g_pBufferA = (pixel16_t *)malloc(GFX_BUFFER_SIZE_BYTES);
    g_pBufferB = (pixel16_t *)malloc(GFX_BUFFER_SIZE_BYTES);

    if (!g_pBufferA || !g_pBufferB)
        return false;

    memset(g_pBufferA, 0x00, GFX_BUFFER_SIZE_BYTES);
    memset(g_pBufferB, 0x00, GFX_BUFFER_SIZE_BYTES);

    g_pDrawingBuffer = g_pBufferA;
    g_pSendingBuffer = g_pBufferB;

    return true;
}

// --- CORE EXECUTION PATHS ---

// 1. FULL SCREEN REFRESH (Call this when loading a brand new UI page)
void Gfx_render(void) {
	
    while (g_bSPI_TransferActive);

    DisplayBitmap();

    Gfx_BuildSG_For_Buffer((uint8_t *)g_pDrawingBuffer);

    HAL_SPI_CS_Enable();
    EVE_AddrForWr(RAM_G);

    Gfx_Start_SG_Transfer();
    g_ui32StartTxCycles = DWTGetCycleCounter();

#ifndef GFX_ENABLE_INT
    HAL_SPI_CS_Disable();
#endif

    /*
    pixel16_t *temp = g_pDrawingBuffer;
    g_pDrawingBuffer = g_pSendingBuffer;
    g_pSendingBuffer = temp;

    memset(g_pDrawingBuffer, 0, GFX_BUFFER_SIZE_BYTES);*/
}

// 2. DIRTY RECTANGLE ENGINE (Call this constantly in your main while(1) loop)
void Gfx_RenderTask(void) {
    switch (g_RenderEngine.state) {
        
        case RENDER_IDLE:
            if (g_DMAQueue.count > 0) {
                // Ensure FT81x draws from the freshly updated RAM_G
                //API_DISPLAY(); 
				DisplayBitmap();
                g_RenderEngine.state = RENDER_SEND_ROW;
            }
            break;

        case RENDER_SEND_ROW:
        {
            // Pop the next dirty row safely
            DMARenderJob_t job = g_DMAQueue.jobs[g_DMAQueue.tail];
            g_DMAQueue.tail = (g_DMAQueue.tail + 1) % MAX_DMA_JOBS;
            g_DMAQueue.count--;

            // Assert CS LOW
            HAL_SPI_CS_Enable();

            // Manually send the 3-byte EVE Memory Write Command
            uint32_t addr = job.destRAMG;
			EVE_AddrForWr(addr);
            
            // Fire the DMA payload!
            // uDMA_Start_SPI_Transfer(job.pSrcSDRAM, job.length);
			HAL_SPI_uDMATransfer(job.pSrcSDRAM, NULL, job.length, true);
            
            g_RenderEngine.state = RENDER_WAIT_DMA;
            break;
        }

        case RENDER_WAIT_DMA:
            // Poll the uDMA to see if it is still transferring
            if (g_bSPI_TransferActive) {
                return; // DMA active. Go do physics/comms.
            }

			HAL_SPI_CS_Disable();
            if (g_DMAQueue.count > 0) {
                g_RenderEngine.state = RENDER_SEND_ROW;
            } else {
                g_RenderEngine.state = RENDER_IDLE;
            }
            break;

		case RENDER_WAIT_SG_ISR:
            // 1. The hardware is actively blasting the 250KB block.
            // 2. When it finishes, your SSI3IntHandler will pull CS high 
            //    and set g_bSPI_TransferActive to false.
            
            if (!g_bSPI_TransferActive) {
                // The ISR caught the completion! The bus is free.
                g_RenderEngine.state = RENDER_IDLE;
            } else {
                // Transfer still running. Return immediately so CPU can do system tasks.
                return;
            }
            break;
		
    }
}

void Gfx_SendContinuousBlock_SG(pixel16_t *pActiveBuffer, int16_t x, int16_t y, int16_t w, int16_t h) {
    // 1. Bounds checking
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > 800) w = 800 - x;
    if (y + h > 480) h = 480 - y;
    if (w <= 0 || h <= 0) return;

    // 2. Memory Math
    uint32_t startOffset = (y * 800) + x;
    uint32_t endOffset = ((y + h - 1) * 800) + (x + w);
    uint32_t totalBytesToSend = (endOffset - startOffset) * 2;
    
    uint8_t *pSrcSDRAM = (uint8_t *)&pActiveBuffer[startOffset];
    uint32_t startingRAMG = startOffset * 2;

    // 3. Build the SG Links dynamically
    Gfx_BuildDynamicSG(pSrcSDRAM, totalBytesToSend);

    // 4. Assert CS and manually send the single address
    HAL_SPI_CS_Enable();
    HWREG(SSI3_BASE + SSI_O_DR) = (startingRAMG >> 16) | 0x80;
    HWREG(SSI3_BASE + SSI_O_DR) = (startingRAMG >> 8) & 0xFF;
    HWREG(SSI3_BASE + SSI_O_DR) = startingRAMG & 0xFF;
    while(MAP_SSIBusy(SSI3_BASE));

    // 5. Fire the Scatter Gather DMA and poll until complete
    Gfx_Start_SG_Transfer();

	g_RenderEngine.state = RENDER_WAIT_SG_ISR;

    // 6. Release CS to complete the FT81x transaction
    // HAL_SPI_CS_Disable();
}

