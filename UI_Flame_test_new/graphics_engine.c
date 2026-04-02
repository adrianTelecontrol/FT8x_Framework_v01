/**
 * graphics_engine.c
 *
 * Hybrid Rendering Engine:
 * 1. Scatter-Gather Linked Lists for full-screen (768KB) transitions.
 * 2. Asynchronous Dirty Rectangle Queue for high-speed UI widget updates.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <driverlib/interrupt.h>
#include <driverlib/rom_map.h>
#include <driverlib/sysctl.h>
#include <driverlib/udma.h>
#include <inc/hw_ints.h>
#include <inc/hw_memmap.h>
#include <inc/hw_ssi.h>
#include <inc/hw_types.h>

#include "EVE.h"
#include "FT8xx_params.h"
#include "event_engine.h"
#include "font_engine.h"
#include "forms_manager.h"
#include "gfx.h"
#include "graphics_engine.h"
#include "hal_spi.h"
#include "helpers.h"


// --- CONFIGURATION ---
#define GFX_WIDTH 800
#define GFX_HEIGHT 480
#define GFX_PIXELS_PER_FRAME (GFX_WIDTH * GFX_HEIGHT)
#define GFX_BUFFER_SIZE_BYTES (GFX_PIXELS_PER_FRAME * 2) // 768,000 Bytes

#define TRANSFER_SIZE 1024

#define GFX_ENABLE_INT
// #define MEASURE_PERF_ENABLE
//  #define MEASURE_PERF_ENABLE_FPS

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

static bool g_bRequestFullRepaint = false;

volatile DMAJobQueue_t g_DMAQueue = {0};

volatile RenderEngine_t g_RenderEngine = {RENDER_IDLE};

static const char *TASK_NAME = "gfx";

// --- HELPER FUNCTIONS ---

void Helper_IntToFPSString(char *buffer, uint32_t whole, uint32_t frac) {
  char *ptr = buffer;
  Helper_FloatToString(buffer, whole, frac, false);
  *ptr++ = '.';
  *ptr++ = (frac % 10) + '0';
  *ptr++ = ' ';
  *ptr++ = 'F';
  *ptr++ = 'P';
  *ptr++ = 'S';
  *ptr++ = '\0';
}

void Gfx_setProcessDuration(uint32_t ui32ProcDur) {
  g_ui32ProcDur = ui32ProcDur;
}

// --- BUILDER FUNCTIONS ---
void Gfx_BuildDynamicSG(uint8_t *pSrc, uint32_t totalBytes) {
  if (totalBytes == 0)
    return;

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
    if (bytesRem == 0)
      break;

    chunk = (bytesRem > TRANSFER_SIZE) ? TRANSFER_SIZE : bytesRem;
    mode = (bytesRem <= TRANSFER_SIZE) ? UDMA_MODE_BASIC
                                       : UDMA_MODE_PER_SCATTER_GATHER;

    g_txList0[i] = (tDMAControlTable)uDMATaskStructEntry(
        chunk, UDMA_SIZE_8, UDMA_SRC_INC_8, pSrc, UDMA_DST_INC_NONE,
        (void *)(SSI3_BASE + SSI_O_DR), UDMA_ARB_4, mode);
    g_rxList0[i] = (tDMAControlTable)uDMATaskStructEntry(
        chunk, UDMA_SIZE_8, UDMA_SRC_INC_NONE, (void *)(SSI3_BASE + SSI_O_DR),
        UDMA_DST_INC_NONE, &g_ulDummyRx, UDMA_ARB_4, mode);

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
      4, UDMA_SIZE_32, UDMA_SRC_INC_32, &txLink1, UDMA_DST_INC_32,
      &PRIMARY_CTRL_TABLE[15], UDMA_ARB_4, UDMA_MODE_PER_SCATTER_GATHER);
  g_rxList0[tCount] = (tDMAControlTable)uDMATaskStructEntry(
      4, UDMA_SIZE_32, UDMA_SRC_INC_32, &rxLink1, UDMA_DST_INC_32,
      &PRIMARY_CTRL_TABLE[14], UDMA_ARB_4, UDMA_MODE_PER_SCATTER_GATHER);
  tCount++;

  // --- POPULATE LIST 1 ---
  tCount = 0;
  for (i = 0; i < TASKS_LIST1 - 1; i++) {
    if (bytesRem == 0)
      break;

    chunk = (bytesRem > TRANSFER_SIZE) ? TRANSFER_SIZE : bytesRem;
    mode = (bytesRem <= TRANSFER_SIZE) ? UDMA_MODE_BASIC
                                       : UDMA_MODE_PER_SCATTER_GATHER;

    g_txList1[i] = (tDMAControlTable)uDMATaskStructEntry(
        chunk, UDMA_SIZE_8, UDMA_SRC_INC_8, pSrc, UDMA_DST_INC_NONE,
        (void *)(SSI3_BASE + SSI_O_DR), UDMA_ARB_4, mode);
    g_rxList1[i] = (tDMAControlTable)uDMATaskStructEntry(
        chunk, UDMA_SIZE_8, UDMA_SRC_INC_NONE, (void *)(SSI3_BASE + SSI_O_DR),
        UDMA_DST_INC_NONE, &g_ulDummyRx, UDMA_ARB_4, mode);

    pSrc += chunk;
    bytesRem -= chunk;
    tCount++;

    if (mode == UDMA_MODE_BASIC) {
      MAP_uDMAChannelScatterGatherSet(UDMA_CH15_SSI3TX, tCount, g_txList1, 1);
      MAP_uDMAChannelScatterGatherSet(UDMA_CH14_SSI3RX, tCount, g_rxList1, 1);
      // Arm List 0 so it starts the chain!
      MAP_uDMAChannelScatterGatherSet(UDMA_CH15_SSI3TX, TASKS_LIST0, g_txList0,
                                      1);
      MAP_uDMAChannelScatterGatherSet(UDMA_CH14_SSI3RX, TASKS_LIST0, g_rxList0,
                                      1);
      return;
    }
  }

  // Add Chain Link to List 2
  g_txList1[tCount] = (tDMAControlTable)uDMATaskStructEntry(
      4, UDMA_SIZE_32, UDMA_SRC_INC_32, &txLink2, UDMA_DST_INC_32,
      &PRIMARY_CTRL_TABLE[15], UDMA_ARB_4, UDMA_MODE_PER_SCATTER_GATHER);
  g_rxList1[tCount] = (tDMAControlTable)uDMATaskStructEntry(
      4, UDMA_SIZE_32, UDMA_SRC_INC_32, &rxLink2, UDMA_DST_INC_32,
      &PRIMARY_CTRL_TABLE[14], UDMA_ARB_4, UDMA_MODE_PER_SCATTER_GATHER);

  // --- POPULATE LIST 2 ---
  tCount = 0;
  for (i = 0; i < TASKS_LIST2; i++) {
    if (bytesRem == 0)
      break;

    chunk = (bytesRem > TRANSFER_SIZE) ? TRANSFER_SIZE : bytesRem;
    mode = (bytesRem <= TRANSFER_SIZE) ? UDMA_MODE_BASIC
                                       : UDMA_MODE_PER_SCATTER_GATHER;

    g_txList2[i] = (tDMAControlTable)uDMATaskStructEntry(
        chunk, UDMA_SIZE_8, UDMA_SRC_INC_8, pSrc, UDMA_DST_INC_NONE,
        (void *)(SSI3_BASE + SSI_O_DR), UDMA_ARB_4, mode);
    g_rxList2[i] = (tDMAControlTable)uDMATaskStructEntry(
        chunk, UDMA_SIZE_8, UDMA_SRC_INC_NONE, (void *)(SSI3_BASE + SSI_O_DR),
        UDMA_DST_INC_NONE, &g_ulDummyRx, UDMA_ARB_4, mode);

    pSrc += chunk;
    bytesRem -= chunk;
    tCount++;

    if (mode == UDMA_MODE_BASIC) {
      MAP_uDMAChannelScatterGatherSet(UDMA_CH15_SSI3TX, tCount, g_txList2, 1);
      MAP_uDMAChannelScatterGatherSet(UDMA_CH14_SSI3RX, tCount, g_rxList2, 1);
      // Arm List 0 to start the massive chain!
      MAP_uDMAChannelScatterGatherSet(UDMA_CH15_SSI3TX, TASKS_LIST0, g_txList0,
                                      1);
      MAP_uDMAChannelScatterGatherSet(UDMA_CH14_SSI3RX, TASKS_LIST0, g_rxList0,
                                      1);
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
        (void *)(SSI3_BASE + SSI_O_DR), UDMA_ARB_4,
        UDMA_MODE_PER_SCATTER_GATHER);
    g_rxList0[i] = (tDMAControlTable)uDMATaskStructEntry(
        TRANSFER_SIZE, UDMA_SIZE_8, UDMA_SRC_INC_NONE,
        (void *)(SSI3_BASE + SSI_O_DR), UDMA_DST_INC_NONE, &g_ulDummyRx,
        UDMA_ARB_4, UDMA_MODE_PER_SCATTER_GATHER);
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
        (void *)(SSI3_BASE + SSI_O_DR), UDMA_ARB_4,
        UDMA_MODE_PER_SCATTER_GATHER);
    g_rxList1[i] = (tDMAControlTable)uDMATaskStructEntry(
        TRANSFER_SIZE, UDMA_SIZE_8, UDMA_SRC_INC_NONE,
        (void *)(SSI3_BASE + SSI_O_DR), UDMA_DST_INC_NONE, &g_ulDummyRx,
        UDMA_ARB_4, UDMA_MODE_PER_SCATTER_GATHER);
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
    uint32_t mode =
        (i == TASKS_LIST2 - 1) ? UDMA_MODE_BASIC : UDMA_MODE_PER_SCATTER_GATHER;
    g_txList2[i] = (tDMAControlTable)uDMATaskStructEntry(
        TRANSFER_SIZE, UDMA_SIZE_8, UDMA_SRC_INC_8, pSrc, UDMA_DST_INC_NONE,
        (void *)(SSI3_BASE + SSI_O_DR), UDMA_ARB_4, mode);
    g_rxList2[i] = (tDMAControlTable)uDMATaskStructEntry(
        TRANSFER_SIZE, UDMA_SIZE_8, UDMA_SRC_INC_NONE,
        (void *)(SSI3_BASE + SSI_O_DR), UDMA_DST_INC_NONE, &g_ulDummyRx,
        UDMA_ARB_4, mode);
    pSrc += TRANSFER_SIZE;
  }

  // STEP 5: ARM LIST 0
  MAP_uDMAChannelScatterGatherSet(UDMA_CH15_SSI3TX, TASKS_LIST0, g_txList0, 1);
  MAP_uDMAChannelScatterGatherSet(UDMA_CH14_SSI3RX, TASKS_LIST0, g_rxList0, 1);
}

bool Gfx_BuildSg_For_Segments(pixel16_t *pActiveBuffer, int16_t x, int16_t y,
                              int16_t w, int16_t h) {
  if (x < 0) {
    w += x;
    x = 0;
  }
  if (y < 0) {
    h += y;
    y = 0;
  }
  if (x + w > 800)
    w = 800 - x;
  if (y + h > 480)
    h = 480 - y;
  if (w <= 0 || h <= 0)
    return true;

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
      if (g_DMAQueue.count >= MAX_DMA_JOBS)
        return false; // Queue full!

      uint16_t chunkLength = (bytesRemaining > MAX_DMA_TRANSFER)
                                 ? MAX_DMA_TRANSFER
                                 : bytesRemaining;

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

// bool Gfx_BuildSg_For_Segments(pixel16_t *pActiveBuffer, int16_t x, int16_t y,
// int16_t w, int16_t h) {
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

inline void Gfx_RestoreBackground_Fast(pixel16_t *pCleanBackground,
                                       pixel16_t *pDrawBuffer, int16_t x,
                                       int16_t y, int16_t width,
                                       int16_t height) {
  if (x < 0) {
    width += x;
    x = 0;
  }
  if (y < 0) {
    height += y;
    y = 0;
  }
  if (x + width > 800)
    width = 800 - x;
  if (y + height > 480)
    height = 480 - y;
  if (width <= 0 || height <= 0)
    return;

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

    if ((val1 & 0x0000FFFF) == 0)
      outPixelPair |= (val0 & 0x0000FFFF);
    else
      outPixelPair |= (val1 & 0x0000FFFF);

    if ((val1 & 0xFFFF0000) == 0)
      outPixelPair |= (val0 & 0xFFFF0000);
    else
      outPixelPair |= (val1 & 0xFFFF0000);

    *pDest++ = outPixelPair;
  }
}

// --- HARDWARE INTERFACES ---
void DisplayBitmap(void) {
  uint16_t ui16Width = 800;
  uint16_t ui16Height = 480;

  API_LIB_BeginCoProList();
  API_CMD_DLSTART();
  API_CLEAR_COLOR_RGB(255, 19, 30);
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
  uint32_t duration =
      (g_ui32EndTxCycles - g_ui32StartTxCycles) / (g_ui32SysClock / 1E6) / 1000;
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
  uint32_t ui32MsElapsed =
      (g_ui32EndTxCycles - g_ui32StartTxCycles) / (g_ui32SysClock / 1E6) / 1000;
  API_COLOR_RGB(255, 0, 0);
  API_CMD_NUMBER(u16DrawnWidth - 100, u16DrawnHeight - 50, 30, 0,
                 ui32MsElapsed);
  API_CMD_SETBASE(10);
#endif
#endif
  /*
API_COLOR_RGB(255, 0, 0);
API_CMD_NUMBER(20, u16DrawnHeight - 50, 30, 0, g_ui32ProcDur);
API_CMD_SETBASE(10);*/

  API_DISPLAY();
  API_CMD_SWAP();
  API_LIB_EndCoProList();
  API_LIB_AwaitCoProEmpty();
}

void Gfx_Start_SG_Transfer(void) {
  g_bSPI_TransferActive = true;

  MAP_uDMAErrorStatusClear();
  MAP_SSIIntDisable(SSI3_BASE,
                    SSI_DMATX | SSI_DMARX | SSI_RXFF | SSI_RXOR | SSI_RXTO);
  MAP_SSIIntClear(SSI3_BASE,
                  SSI_DMATX | SSI_DMARX | SSI_RXFF | SSI_RXOR | SSI_RXTO);

  uint32_t trash;
  while (MAP_SSIDataGetNonBlocking(SSI3_BASE, &trash))
    ;
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
  while (MAP_SSIBusy(SSI3_BASE))
    ;
  MAP_SSIDMADisable(SSI3_BASE, SSI_DMA_TX | SSI_DMA_RX);
  g_bSPI_TransferActive = false;
#endif
}

static void onFullRepaintEvent(uint32_t args) { g_bRequestFullRepaint = true; }

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

  Event_Subscribe(EVT_CMD_FULL_REPAINT, onFullRepaintEvent);

  return true;
}

// --- CORE EXECUTION PATHS ---

// 1. FULL SCREEN REFRESH (Call this when loading a brand new UI page)
void Gfx_render(void) {

  while (g_bSPI_TransferActive)
    ;

  DisplayBitmap();

  Gfx_BuildSG_For_Buffer((uint8_t *)g_pDrawingBuffer);

  HAL_SPI_CS_Enable();
  EVE_AddrForWr(RAM_G);

  Gfx_Start_SG_Transfer();
  // g_ui32StartTxCycles = DWTGetCycleCounter();

#ifndef GFX_ENABLE_INT
  HAL_SPI_CS_Disable();
#endif

  /*
  pixel16_t *temp = g_pDrawingBuffer;
  g_pDrawingBuffer = g_pSendingBuffer;
  g_pSendingBuffer = temp;

  memset(g_pDrawingBuffer, 0, GFX_BUFFER_SIZE_BYTES);*/
}

void Gfx_SendContinuousBlock_SG(pixel16_t *pActiveBuffer, int16_t x, int16_t y,
                                int16_t w, int16_t h) {
  // 1. Bounds checking
  if (x < 0) {
    w += x;
    x = 0;
  }
  if (y < 0) {
    h += y;
    y = 0;
  }
  if (x + w > 800)
    w = 800 - x;
  if (y + h > 480)
    h = 480 - y;
  if (w <= 0 || h <= 0)
    return;

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
  while (MAP_SSIBusy(SSI3_BASE))
    ;

  // 5. Fire the Scatter Gather DMA and poll until complete
  Gfx_Start_SG_Transfer();

  g_RenderEngine.state = RENDER_WAIT_SG_ISR;

  // 6. Release CS to complete the FT81x transaction
  // HAL_SPI_CS_Disable();
}

bool gfx_getWidgetBounds(gfx_GenericWidget *widget, int16_t *x, int16_t *y,
                         int16_t *w, int16_t *h) {
  if (widget == NULL || widget->pvWidget == NULL)
    return false;

  switch (widget->eWidgetType) {
  case WD_TYPE_BUTTON: {
    gfx_Button *btn = (gfx_Button *)widget->pvWidget;

    // 1. Calcular el desbordamiento del texto (Alpha-Blending Trap Fix)
    int16_t textWidth = 0;
    if (btn->label != NULL) {
      // NOTA: Ajusta el '10' por el ancho promedio en píxeles de tu fuente,
      // o usa la función de tu librería para medir el string si la tienes.
      textWidth = strlen(btn->label) * 10;
    }

    int16_t overflowX = 0;
    if (textWidth > btn->size.width) {
      overflowX = (textWidth - btn->size.width) / 2;
      overflowX += 4; // Padding extra para el anti-aliasing de la fuente
    }

    // 2. Calcular la huella total (Footprint + Overflow + Cinematic Offset)
    *x = btn->pos.x - overflowX - btn->borderWidth;
    *y = btn->pos.y - btn->borderWidth;

    // Sumamos 2 píxeles extra a W y H para acomodar el desplazamiento
    // cuando el botón pasa a estado BTN_STATE_PRESSED.
    *w = btn->size.width + (overflowX * 2) + (btn->borderWidth * 2) + 2;
    *h = btn->size.height + (btn->borderWidth * 2) + 2;

    return true;
  }
  case WD_TYPE_RECT: {
    gfx_Rectangle *rect = (gfx_Rectangle *)widget->pvWidget;
    *x = rect->pos.x;
    *y = rect->pos.y;
    *w = rect->dim.width;
    *h = rect->dim.height;
    return true;
  }
  case WD_TYPE_LABEL: {
    gfx_Label *lbl = (gfx_Label *)widget->pvWidget;
    *x = lbl->pos.x;
    *y = lbl->pos.y;
    // Reemplaza con la métrica real de tu fuente
    *w = 100;
    *h = 20;
    return true;
  }
  default:
    return false;
  }
}

bool gfx_compositePartialFrame(gfx_Canvas *srf, pixel16_t *psPixelBuffer,
                               int16_t dirtyX, int16_t dirtyY, int16_t dirtyW,
                               int16_t dirtyH) {
  if (srf == NULL) {
    UARTprintf("Cannot render canvas! Is empty.");
    return false;
  }

  // 1. Erase the old UI state by restoring the background ONLY inside the dirty
  // area (Assuming sBitmapHandler is globally accessible as in your previous
  // snippets) Gfx_RestoreBackground_Fast((pixel16_t *)sBitmapHandler.ui8Pixels,
  //                            psPixelBuffer, dirtyX, dirtyY, dirtyW, dirtyH);
  int16_t y = dirtyY;
  for (; y < dirtyY + dirtyH; y++) {
    int16_t x = dirtyX;
    for (; x < dirtyX + dirtyW; x++) {

      // Safety guard: Ensure we don't write outside the physical screen memory
      if (x >= 0 && x < LCD_WIDTH && y >= 0 && y < LCD_HEIGHT) {
        // Calculate the 1D linear index for the 2D coordinate
        uint32_t linearIndex = (y * LCD_WIDTH) + x;
        psPixelBuffer[linearIndex] =
            (pixel16_t)g_pCurrentTheme->palette.background;
        //psPixelBuffer[linearIndex] = (pixel16_t)(uint16_t)0xFF70;
      }
    }
  }

  gfx_GenericWidgetNode *iter = srf->psWidgets;

  // 2. Iterate through all widgets in the canvas
  while (iter != NULL) {
    int16_t objX = 0, objY = 0, objW = 0, objH = 0;

    // Fetch the bounding box for this specific widget
    if (gfx_getWidgetBounds(&iter->sWidget, &objX, &objY, &objW, &objH)) {

      // 3. Collision Detection Math
      // Returns TRUE if the widget overlaps the dirty rectangle at all
      bool isIntersecting = !(objX > dirtyX + dirtyW || objX + objW < dirtyX ||
                              objY > dirtyY + dirtyH || objY + objH < dirtyY);

      // 4. Only draw the widget if it touches the dirty zone!
      if (isIntersecting) {
        switch (iter->sWidget.eWidgetType) {
        case WD_TYPE_BUTTON:
          gfx_drawButton(psPixelBuffer, (gfx_Button *)iter->sWidget.pvWidget);
          break;
        case WD_TYPE_RECT:
          gfx_drawRectangle(psPixelBuffer,
                            (gfx_Rectangle *)iter->sWidget.pvWidget);
          break;
        case WD_TYPE_LABEL:
          gfx_drawLabel(psPixelBuffer, (gfx_Label *)iter->sWidget.pvWidget);
          break;
        default:
          break;
        }
      }
    }

    iter = iter->psNext;
  }

  return true;
}


void Gfx_RenderTask(void) {
  // Bandera estática para recordar si nos falta enviar la mitad inferior
  static bool g_bPendingBottomHalf = false;

  switch (g_RenderEngine.state) {

  case RENDER_IDLE:
    // 1. If the queue is populated, start the pumping process!
    if (g_DMAQueue.count > 0) {
      // DisplayBitmap(); // Depending on your EVE config, call this if needed
      g_RenderEngine.state = RENDER_SEND_ROW;
      break;
    }

    // 2. ¿Nos quedó pendiente la mitad inferior del repintado masivo?
    // Entramos aquí solo porque sabemos que la cola (g_DMAQueue) ya está en 0.
    if (g_bPendingBottomHalf) {
      g_bPendingBottomHalf = false;

      // Enviar la MITAD INFERIOR (Desde Y = LCD_HEIGHT / 2, Altura = LCD_HEIGHT
      // / 2)
      Gfx_BuildSg_For_Segments(g_pDrawingBuffer, 0, LCD_HEIGHT / 2, LCD_WIDTH,
                               LCD_HEIGHT / 2);

      break;
    }

    // 3. ¿Hay una nueva solicitud de repintado masivo?
    if (g_bRequestFullRepaint) {
      g_bRequestFullRepaint = false;

      // A. Componer TODA la pantalla en la memoria RAM del TM4C
      formManagerComposite(g_pDrawingBuffer);

      // B. Enviar solo la MITAD SUPERIOR a la cola DMA
      // (Desde Y = 0, Altura = LCD_HEIGHT / 2)
      Gfx_BuildSg_For_Segments(g_pDrawingBuffer, 0, 0, LCD_WIDTH,
                               LCD_HEIGHT / 2);

      // C. Levantar la bandera para enviar la parte inferior en la siguiente
      // vuelta
      g_bPendingBottomHalf = true;

      break;
    }

    // 4. If the queue is empty, the SPI is totally free.
    // Let's sweep the Form Manager for dirty widgets!
    if (g_psCurrentForm != NULL) {
      gfx_GenericWidgetNode *iter = g_psCurrentForm->psWidgets;

      while (iter != NULL) {
        bool isDirty = false;
        int16_t bboxX, bboxY, bboxW, bboxH;

        // Check which widget type we are looking at
        if (iter->sWidget.eWidgetType == WD_TYPE_BUTTON) {
          gfx_Button *btn = (gfx_Button *)iter->sWidget.pvWidget;
          if (btn->bIsDirty) {
            btn->bIsDirty = false;

            // Always include the pressed offset in the bbox so the
            // cinematic shift is covered whether pressing or releasing.
            int16_t textWidth =
                (btn->label != NULL) ? strlen(btn->label) * 10 : 0;
            int16_t overflowX = 0;
            if (textWidth > btn->size.width) {
              overflowX = ((textWidth - btn->size.width) / 2) + 4;
            }

            // +2 on every side to absorb the cinematic press offset
            // so the bbox is always large enough regardless of state.
            int16_t w =
                btn->size.width + (overflowX * 2) + (btn->borderWidth * 2) + 4;
            int16_t h = btn->size.height + (btn->borderWidth * 2) + 4;

            int16_t oldX = btn->oldPos.x - overflowX - btn->borderWidth - 2;
            int16_t oldY = btn->oldPos.y - btn->borderWidth - 2;
            int16_t newX = btn->pos.x - overflowX - btn->borderWidth - 2;
            int16_t newY = btn->pos.y - btn->borderWidth - 2;

            bboxX = (oldX < newX) ? oldX : newX;
            bboxY = (oldY < newY) ? oldY : newY;
            int16_t rightEdge =
                ((oldX + w) > (newX + w)) ? (oldX + w) : (newX + w);
            int16_t bottomEdge =
                ((oldY + h) > (newY + h)) ? (oldY + h) : (newY + h);
            bboxW = rightEdge - bboxX;
            bboxH = bottomEdge - bboxY;

            btn->oldPos = btn->pos;
            isDirty = true;
          }
        } else if (iter->sWidget.eWidgetType == WD_TYPE_LABEL) {
          gfx_Label *lb = (gfx_Label *)iter->sWidget.pvWidget;

          if (lb->bIsDirty && lb->text != NULL) {

            int8_t fontId = -1;
            switch (lb->typo) {
            case TYPO_H1:
              fontId = g_pCurrentTheme->fonts.h1;
              break;
            case TYPO_H2:
              fontId = g_pCurrentTheme->fonts.h2;
              break;
            case TYPO_BODY:
              fontId = g_pCurrentTheme->fonts.body;
              break;
            case TYPO_CAPTION:
              fontId = g_pCurrentTheme->fonts.caption;
              break;
            case TYPO_MONO:
              fontId = g_pCurrentTheme->fonts.mono;
              break;
            }

            uint16_t textW = 0, textH = 0;
            if (fontId >= 0) {
              gfx_GetStringDimensions(lb->text, fontId, &textW, &textH, 1);
            }

            int16_t wipeX = lb->pos.x;
            int16_t wipeY = lb->pos.y;

            if (lb->alignment & ALIGN_HCENTER) {
              wipeX = lb->pos.x - (textW / 2);
            } else if (lb->alignment & ALIGN_RIGHT) {
              wipeX = lb->pos.x - textW;
            }

            if (lb->alignment & ALIGN_VCENTER) {
              wipeY = lb->pos.y - (textH / 2);
            } else if (lb->alignment & ALIGN_BOTTOM) {
              wipeY = lb->pos.y - textH;
            }

            // Margin of safety — absorbs font bearing overhang
			wipeX -= 4;
			wipeY -= 2;
			textW += 8;
			textH += 4;
			
			// Union bounding box (shrinking ghost fix)
			if (lb->oldSize.width > 0) {
			    bboxX = (wipeX < lb->oldPos.x) ? wipeX : lb->oldPos.x;
			    bboxY = (wipeY < lb->oldPos.y) ? wipeY : lb->oldPos.y;
			    int16_t rightEdge  = ((wipeX + textW) > (lb->oldPos.x + lb->oldSize.width))
			                          ? (wipeX + textW) : (lb->oldPos.x + lb->oldSize.width);
			    int16_t bottomEdge = ((wipeY + textH) > (lb->oldPos.y + lb->oldSize.height))
			                          ? (wipeY + textH) : (lb->oldPos.y + lb->oldSize.height);
			    bboxW = rightEdge  - bboxX;
			    bboxH = bottomEdge - bboxY;
			} else {
			    bboxX = wipeX;
			    bboxY = wipeY;
			    bboxW = textW;
			    bboxH = textH;
			}
			
			// Save expanded footprint for next frame
			lb->oldPos.x     = wipeX;
			lb->oldPos.y     = wipeY;
			lb->oldSize.width  = textW;
			lb->oldSize.height = textH;
			
			// No more bboxW += 50

            // bboxW += 4;

            // WARNING: "Shrinking Ghost" logic goes here (see note below)

            isDirty = true; // Trigger the DMA composite pipeline!
            lb->bIsDirty =
                false; // Clear the widget's flag so we don't loop forever
          }
        }
        // Add other cases (WD_TYPE_RECT, WD_TYPE_LABEL) here...

        // If we found a dirty widget, process it!
        if (isDirty) {

          if (bboxX % 2 != 0) {
            bboxX -= 1;
            bboxW += 1;
          }

          // If Width is still odd, grow it 1 pixel right
          if (bboxW % 2 != 0) {
            bboxW += 1;
          }
          // if (bboxW % 2 != 0) {
          //                    bboxW += 1;
          //                }
          // A. Composite exactly this rectangle in the TM4C SDRAM
          gfx_compositePartialFrame(g_psCurrentForm, g_pDrawingBuffer, bboxX,
                                    bboxY, bboxW, bboxH);

          // B. Break the 2D bounding box into 1D rows and populate the DMA
          // Queue!
          Gfx_BuildSg_For_Segments(g_pDrawingBuffer, bboxX, bboxY, bboxW,
                                   bboxH);

          // C. Break out of the while loop immediately.
          // The queue is now full. On the next super-loop cycle,
          // the engine will hit the (g_DMAQueue.count > 0) check above
          // and transition to RENDER_SEND_ROW.
          break;
        }

        iter = iter->psNext;
      }
    }
    break;

  case RENDER_SEND_ROW: {
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
    HAL_SPI_uDMATransfer(job.pSrcSDRAM, NULL, job.length, true);

    g_RenderEngine.state = RENDER_WAIT_DMA;
    break;
  }

  case RENDER_WAIT_DMA:
    // 1. Poll the uDMA to see if it is still feeding the SPI
    if (g_bSPI_TransferActive) {
      return; // DMA active. Go do physics/comms.
    }

    // 2. THE FIX: Wait for the SPI hardware to physically empty its FIFO!
    // Replace 'SSI3_BASE' with whatever SPI module you are actually using
    // (e.g., SSI0_BASE, SSI2_BASE) or use your HAL equivalent.
    while (SSIBusy(SSI3_BASE)) {
      // Spin here for a fraction of a microsecond until the wire is 100% idle
    }

    // 3. Now it is completely safe to terminate the transfer
    HAL_SPI_CS_Disable();

    if (g_DMAQueue.count > 0) {
      g_RenderEngine.state = RENDER_SEND_ROW;
    } else {
      g_RenderEngine.state = RENDER_IDLE;
    }
    break;

  case RENDER_WAIT_SG_ISR:
    // (Keep this if you still use it for full-screen blasts)
    if (!g_bSPI_TransferActive) {

#ifndef GFX_ENABLE_INT
      HAL_SPI_CS_Disable();
#endif
      g_RenderEngine.state = RENDER_IDLE;
    } else {
      return;
    }
    break;
  }
}