/**
 * graphics_engine.c
 *
 * Scatter-Gather Version - Chained Lists
 * Implements >256KB "Fire and Forget" Rendering using Task Linking.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <driverlib/rom_map.h>
#include <driverlib/sysctl.h>
#include <driverlib/udma.h>
#include <inc/hw_memmap.h>
#include <inc/hw_ssi.h>
#include <inc/hw_types.h>

#include "EVE.h"
#include "graphics_engine.h"
#include "helpers.h"
#include "hal_spi.h"

// --- CONFIGURATION ---
#define GFX_WIDTH 800
#define GFX_HEIGHT 480
#define GFX_PIXELS_PER_FRAME (GFX_WIDTH * GFX_HEIGHT)
#define GFX_BUFFER_SIZE_BYTES (GFX_PIXELS_PER_FRAME * 2) // 768,000 Bytes

#define TRANSFER_SIZE 1024

#define GFX_ENABLE_INT

// 768,000 / 1024 = 750 Total Tasks.
// Split across 3 lists to bypass the 256 Hardware Limit.
#define TASKS_LIST0 256 // 255 Data + 1 Link
#define TASKS_LIST1 256 // 255 Data + 1 Link
#define TASKS_LIST2 240 // 240 Data (Stops)

// --- GLOBALS ---
pixel16_t *g_pBufferA;
pixel16_t *g_pBufferB;

pixel16_t *g_pDrawingBuffer;
pixel16_t *g_pSendingBuffer;

// Access the global DMA Control Table (Declared in your main/DMA init)
extern uint8_t g_HAL_uDMA_ControlTable[1024];
#define PRIMARY_CTRL_TABLE ((tDMAControlTable *)g_HAL_uDMA_ControlTable)

// The 3 Chained Task Lists
#if defined(__ICCARM__)
#pragma data_alignment = 1024
#elif defined(__TI_COMPILER_VERSION__)
#pragma DATA_ALIGN(g_txList0, 1024)
#pragma DATA_ALIGN(g_rxList0, 1024)
// ... (Align the rest if TI compiler)
#endif
static tDMAControlTable g_txList0[TASKS_LIST0] __attribute__((aligned(1024)));
static tDMAControlTable g_rxList0[TASKS_LIST0] __attribute__((aligned(1024)));
static tDMAControlTable g_txList1[TASKS_LIST1] __attribute__((aligned(1024)));
static tDMAControlTable g_rxList1[TASKS_LIST1] __attribute__((aligned(1024)));
static tDMAControlTable g_txList2[TASKS_LIST2] __attribute__((aligned(1024)));
static tDMAControlTable g_rxList2[TASKS_LIST2] __attribute__((aligned(1024)));

// Link Structures (Used to reprogram the DMA on the fly)
static tDMAControlTable txLink1, txLink2;
static tDMAControlTable rxLink1, rxLink2;
static uint32_t g_ulDummyRx;

// --- INTERRUPT HANDLER
// void SSI3IntHandler(void) {
//   // 1. Read and clear the interrupt status
//   uint32_t ui32Status = MAP_SSIIntStatus(SSI3_BASE, 1);
//   MAP_SSIIntClear(SSI3_BASE, ui32Status);
// 
//   // 2. Check if this interrupt is because the DMA transfer finished
//   if (g_bGFX_TransferActive && !MAP_uDMAChannelIsEnabled(UDMA_CH15_SSI3TX)) {
// 
//     // Wait for the final few bits to shift out of the SPI hardware
//     while (MAP_SSIBusy(SSI3_BASE))
//       ;
// 
//     // CRITICAL: Close the EVE SPI transaction!
//     HAL_SPI_CS_Disable();
// 
//     // Clean up DMA and disable this interrupt until the next frame
//     MAP_SSIDMADisable(SSI3_BASE, SSI_DMA_TX | SSI_DMA_RX);
//     MAP_SSIIntDisable(SSI3_BASE, SSI_TXFF);
// 
//     // Signal to the CPU that the DMA is free!
//     g_bGFX_TransferActive = false;
//   }
// 
//   // Safety catch for RX Overruns
//   if (ui32Status & SSI_RXOR) {
//     MAP_SSIIntClear(SSI3_BASE, SSI_RXOR);
//   }
// }

// --- BUILDER FUNCTION ---

void Gfx_BuildSG_For_Buffer(uint8_t *pBuffer) {
  uint8_t *pSrc = pBuffer;
  int i;

  // ------------------------------------------------------------------------
  // STEP 1: CALCULATE LINK STRUCTURES
  // ------------------------------------------------------------------------
  // We temporarily assign List 1 and 2 to the channels just so the TI Driverlib
  // calculates the exact 'Primary Control Structure' bits for us.
  // We snapshot those configurations into our txLink/rxLink variables.

  // Calculate Link 2 (Points to List 2)
  MAP_uDMAChannelScatterGatherSet(UDMA_CH15_SSI3TX, TASKS_LIST2, g_txList2, 1);
  txLink2 = PRIMARY_CTRL_TABLE[15 & 0x1F]; // Grab TX Primary Struct
  MAP_uDMAChannelScatterGatherSet(UDMA_CH14_SSI3RX, TASKS_LIST2, g_rxList2, 1);
  rxLink2 = PRIMARY_CTRL_TABLE[14 & 0x1F]; // Grab RX Primary Struct

  // Calculate Link 1 (Points to List 1)
  MAP_uDMAChannelScatterGatherSet(UDMA_CH15_SSI3TX, TASKS_LIST1, g_txList1, 1);
  txLink1 = PRIMARY_CTRL_TABLE[15 & 0x1F];
  MAP_uDMAChannelScatterGatherSet(UDMA_CH14_SSI3RX, TASKS_LIST1, g_rxList1, 1);
  rxLink1 = PRIMARY_CTRL_TABLE[14 & 0x1F];

  // ------------------------------------------------------------------------
  // STEP 2: POPULATE LIST 0 (255 SPI Tasks + 1 Link Task)
  // ------------------------------------------------------------------------
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
  // Task 255 (The Link): 4-word copy into the Primary Control Structure
  // Mode is PER_SCATTER_GATHER to force the uDMA to fetch the newly loaded
  // list!
  g_txList0[TASKS_LIST0 - 1] = (tDMAControlTable)uDMATaskStructEntry(
      4, UDMA_SIZE_32, UDMA_SRC_INC_32, &txLink1, UDMA_DST_INC_32,
      &PRIMARY_CTRL_TABLE[15], UDMA_ARB_4, UDMA_MODE_PER_SCATTER_GATHER);
  g_rxList0[TASKS_LIST0 - 1] = (tDMAControlTable)uDMATaskStructEntry(
      4, UDMA_SIZE_32, UDMA_SRC_INC_32, &rxLink1, UDMA_DST_INC_32,
      &PRIMARY_CTRL_TABLE[14], UDMA_ARB_4, UDMA_MODE_PER_SCATTER_GATHER);

  // ------------------------------------------------------------------------
  // STEP 3: POPULATE LIST 1 (255 SPI Tasks + 1 Link Task)
  // ------------------------------------------------------------------------
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
  // Task 255 (The Link): 4-word copy into the Primary Control Structure
  g_txList1[TASKS_LIST1 - 1] = (tDMAControlTable)uDMATaskStructEntry(
      4, UDMA_SIZE_32, UDMA_SRC_INC_32, &txLink2, UDMA_DST_INC_32,
      &PRIMARY_CTRL_TABLE[15], UDMA_ARB_4, UDMA_MODE_PER_SCATTER_GATHER);
  g_rxList1[TASKS_LIST1 - 1] = (tDMAControlTable)uDMATaskStructEntry(
      4, UDMA_SIZE_32, UDMA_SRC_INC_32, &rxLink2, UDMA_DST_INC_32,
      &PRIMARY_CTRL_TABLE[14], UDMA_ARB_4, UDMA_MODE_PER_SCATTER_GATHER);

  // ------------------------------------------------------------------------
  // STEP 4: POPULATE LIST 2 (240 SPI Tasks -> END)
  // ------------------------------------------------------------------------
  for (i = 0; i < TASKS_LIST2; i++) {
    // Only the absolute final task in the entire chain gets UDMA_MODE_BASIC
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

  // ------------------------------------------------------------------------
  // STEP 5: ARM LIST 0
  // ------------------------------------------------------------------------
  // Now that everything is built and linked, set the hardware to start at List
  // 0!
  MAP_uDMAChannelScatterGatherSet(UDMA_CH15_SSI3TX, TASKS_LIST0, g_txList0, 1);
  MAP_uDMAChannelScatterGatherSet(UDMA_CH14_SSI3RX, TASKS_LIST0, g_rxList0, 1);
}

// --- ENGINE EXECUTION ---
// --- ENGINE FUNCTIONS ---
void Gfx_loadIntoBuffer(uint32_t ui32Index, uint16_t ui16Pixel) {

  // Unsafe fast write to External RAM

  // (We skip bounds check for max performance in the render loop)

  g_pDrawingBuffer[ui32Index].u16 = ui16Pixel;
}

void DisplayBitmap(void) {
  uint16_t ui16Width = 800;
  uint16_t ui16Height = 480;
  // TIVA_LOGI(TASK_NAME, "Bitmap finished to load into RAM_G");

  API_LIB_BeginCoProList();
  API_CMD_DLSTART();
  API_CLEAR_COLOR_RGB(11, 19, 30);
  API_CLEAR(1, 1, 1);
  API_COLOR_RGB(255, 255, 255);

  API_BITMAP_HANDLE(0);
  API_BITMAP_SOURCE(RAM_G);
  uint16_t BytesPerPixel = 2; // RGB565 is 2 bytes
  // uint16_t ui16Stride = ((ui16Width * BytesPerPixel) + 3) & ~3;
  uint16_t ui16Stride = ui16Width * BytesPerPixel;

  // 1. Calculate Target Dimensions
  uint8_t u8Scale = 1;
  uint16_t u16DrawnWidth = ui16Width * u8Scale;
  uint16_t u16DrawnHeight = ui16Height * u8Scale;

  // 2. BITMAP_LAYOUT (Remains based on the SOURCE image size)
  API_BITMAP_LAYOUT(RGB565, ui16Stride, ui16Height);
  API_BITMAP_LAYOUT_H(ui16Stride >> 10, ui16Height >> 9);

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

  // 5. Draw
  API_BEGIN(BITMAPS);
  API_VERTEX2II(0, 0, 0, 0);
  API_END();

#ifdef MEASURE_PERF_ENABLE
  API_COLOR_RGB(255, 0, 0);
  API_CMD_NUMBER(LCD_WIDTH - 100, LCD_HEIGHT - 50, 30, 0, g_ui32ExecDurMs);
  // API_CMD_NUMBER(LCD_WIDTH / 2, LCD_HEIGHT / 2, 29, 0, g_ui32ExecDurMs);
  API_CMD_SETBASE(10);
#endif
  API_DISPLAY();
  API_CMD_SWAP();
  API_LIB_EndCoProList();
  API_LIB_AwaitCoProEmpty();
}

void Gfx_Start_SG_Transfer(void) {

  // Mark transfer as active
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
  // Trigger List 0. It will automatically chain to List 1, then List 2!
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
#endif
}

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

void Gfx_render(void) {
  // Draw solid pink as a test
  // memset(g_pDrawingBuffer, 0xF81F, GFX_BUFFER_SIZE_BYTES);

  while (g_bSPI_TransferActive)
    ;
  // while(!g_bSPI_TransferDone);
  DisplayBitmap();

  // Generate the 3 chained lists
  Gfx_BuildSG_For_Buffer((uint8_t *)g_pDrawingBuffer);

  HAL_SPI_CS_Enable();
  EVE_AddrForWr(RAM_G);

  // This single call now executes all 768KB via hardware links
  Gfx_Start_SG_Transfer();

#ifndef GFX_ENABLE_INT
  HAL_SPI_CS_Disable();
#endif

  // Ping-pong swap
  pixel16_t *temp = g_pDrawingBuffer;
  g_pDrawingBuffer = g_pSendingBuffer;
  g_pSendingBuffer = temp;

  memset(g_pDrawingBuffer, 0, GFX_BUFFER_SIZE_BYTES);
}
