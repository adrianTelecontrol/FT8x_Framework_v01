/**
 * graphics_engine.c
 *
 * Scatter-Gather Version - Integrated
 * Implements "Fire and Forget" Parallel Rendering using External SDRAM.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// TivaWare & Driver Includes
#include <driverlib/sysctl.h>
#include <driverlib/rom_map.h>
#include <driverlib/udma.h>
#include <inc/hw_memmap.h>
#include <inc/hw_ssi.h>
#include <inc/hw_types.h>

#include "EVE.h"
#include "helpers.h"
#include "tiva_spi.h"
#include "graphics_engine.h"

// --- CONFIGURATION ---
#define GFX_WIDTH               800
#define GFX_HEIGHT              480
#define GFX_PIXELS_PER_FRAME    (GFX_WIDTH * GFX_HEIGHT)
#define GFX_BUFFER_SIZE_BYTES   (GFX_PIXELS_PER_FRAME * 2) 

// DMA Configuration
#define TRANSFER_SIZE           1024
// Calculate how many 1KB tasks we need for a full frame (Approx 750 tasks)
#define NUM_DMA_TASKS           ((GFX_BUFFER_SIZE_BYTES + TRANSFER_SIZE - 1) / TRANSFER_SIZE)

// --- GLOBALS ---
// Double Buffers in External SDRAM
pixel16_t *g_pBufferA;
pixel16_t *g_pBufferB;

// Pointers to track state (Ping-Pong)
pixel16_t *g_pDrawingBuffer; 
pixel16_t *g_pSendingBuffer; 

// DMA Control Tables (Must be aligned)
// We need two lists: One for sending Buffer A, one for Buffer B
#if defined(__ICCARM__)
#pragma data_alignment=1024
static tDMAControlTable g_sTxTaskList[NUM_DMA_TASKS];
static tDMAControlTable g_sRxTaskList[NUM_DMA_TASKS];
#elif defined(__TI_COMPILER_VERSION__)
#pragma DATA_ALIGN(g_sTxTaskList, 1024)
static tDMAControlTable g_sTxTaskList[NUM_DMA_TASKS];
#pragma DATA_ALIGN(g_sRxTaskList, 1024)
static tDMAControlTable g_sRxTaskList[NUM_DMA_TASKS];
#else
static tDMAControlTable g_sTxTaskList[NUM_DMA_TASKS] __attribute__ ((aligned(1024)));
static tDMAControlTable g_sRxTaskList[NUM_DMA_TASKS] __attribute__ ((aligned(1024)));
#endif

static uint32_t g_ulDummyRx; // Sink for RX data

// --- HELPER FUNCTIONS ---

// Builds the DMA Task List for a specific buffer (A or B)
// This chops the massive buffer into 1KB chunks the uDMA can handle.
void Gfx_BuildSG_For_Buffer(uint8_t *pBuffer) {
    uint32_t i;
    uint32_t remainingBytes = GFX_BUFFER_SIZE_BYTES;
    uint8_t *pCurrentSrc = pBuffer;

    for (i = 0; i < NUM_DMA_TASKS; i++) {
        // Calculate size for this chunk (usually 1024, but last one might be smaller)
        uint32_t chunkSize = (remainingBytes > TRANSFER_SIZE) ? TRANSFER_SIZE : remainingBytes;
        remainingBytes -= chunkSize;

        // Determine Mode: Last task is BASIC, others are SCATTER_GATHER
        uint32_t mode = (i == NUM_DMA_TASKS - 1) ? UDMA_MODE_BASIC : UDMA_MODE_PER_SCATTER_GATHER;

        // Configure TX Task (Send data to SPI)
        g_sTxTaskList[i] = (tDMAControlTable)uDMATaskStructEntry(
            chunkSize, UDMA_SIZE_8, UDMA_SRC_INC_8, 
            pCurrentSrc, 
            UDMA_DST_INC_NONE, (void *)(SSI3_BASE + SSI_O_DR), 
            UDMA_ARB_4, mode
        );

        // Configure RX Task (Drain FIFO to Dummy)
        g_sRxTaskList[i] = (tDMAControlTable)uDMATaskStructEntry(
            chunkSize, UDMA_SIZE_8, UDMA_SRC_INC_NONE, 
            (void *)(SSI3_BASE + SSI_O_DR), 
            UDMA_DST_INC_NONE, &g_ulDummyRx, 
            UDMA_ARB_4, mode
        );

        pCurrentSrc += chunkSize;
    }
}

// Executes the transfer (Blocking Wait Version for now)
void Gfx_Start_SG_Transfer(void) {
    // 1. Clear Errors
    MAP_uDMAErrorStatusClear();

    // 2. Clean Hardware
    MAP_SSIIntDisable(SSI3_BASE, SSI_DMATX | SSI_DMARX | SSI_RXFF | SSI_RXOR | SSI_RXTO);
    MAP_SSIIntClear(SSI3_BASE, SSI_DMATX | SSI_DMARX | SSI_RXFF | SSI_RXOR | SSI_RXTO);

    // 3. Drain FIFO (Critical)
    uint32_t trash;
    while (MAP_SSIDataGetNonBlocking(SSI3_BASE, &trash));
    MAP_SSIIntClear(SSI3_BASE, SSI_RXOR | SSI_RXTO);

    // 4. Configure DMA
    MAP_uDMAChannelScatterGatherSet(UDMA_CH15_SSI3TX, NUM_DMA_TASKS, g_sTxTaskList, 1);
    MAP_uDMAChannelScatterGatherSet(UDMA_CH14_SSI3RX, NUM_DMA_TASKS, g_sRxTaskList, 1);

    // 5. Enable
    MAP_uDMAChannelEnable(UDMA_CH14_SSI3RX);
    MAP_uDMAChannelEnable(UDMA_CH15_SSI3TX);
    MAP_SSIDMAEnable(SSI3_BASE, SSI_DMA_TX | SSI_DMA_RX);

    // 6. Kickstart
    MAP_uDMAChannelRequest(UDMA_CH15_SSI3TX);

    // 7. Wait (Blocking)
    //    TODO: Later, move this wait to the START of the NEXT frame (Step 1 of Render)
    //    to achieve true parallel processing.
    while (MAP_uDMAChannelIsEnabled(UDMA_CH15_SSI3TX)) {
         if(HWREG(SSI3_BASE + SSI_O_RIS) & SSI_RIS_RORRIS) {
              HWREG(SSI3_BASE + SSI_O_ICR) = SSI_ICR_RORIC;
         }
    }
    while (MAP_SSIBusy(SSI3_BASE));

    // 8. Cleanup
    MAP_SSIDMADisable(SSI3_BASE, SSI_DMA_TX | SSI_DMA_RX);
    
    // NOTE: CS High is handled by the caller (Gfx_render)
}

// --- ENGINE FUNCTIONS ---

bool Gfx_initEngine(const uint16_t ui16ResWidth, const uint16_t ui16ResHeight) {
    // 1. Allocate Buffers (Ensure Heap is > 2MB)
    g_pBufferA = (pixel16_t *)malloc(GFX_BUFFER_SIZE_BYTES);
    g_pBufferB = (pixel16_t *)malloc(GFX_BUFFER_SIZE_BYTES);

    if (!g_pBufferA || !g_pBufferB) return false;

    // 2. Clear Buffers
    memset(g_pBufferA, 0xFF, GFX_BUFFER_SIZE_BYTES); // White
    memset(g_pBufferB, 0xFF, GFX_BUFFER_SIZE_BYTES); // White

    // 3. Set Initial Pointers
    g_pDrawingBuffer = g_pBufferA; 
    g_pSendingBuffer = g_pBufferB; 
    
    return true;
}

void Gfx_render(void) 
{
    // --- STEP 1: RENDER TO SDRAM ---
    // Fill the drawing buffer with some data (e.g., solid blue)
    // In a real app, you would draw widgets/images here.
    memset(g_pDrawingBuffer, 0x1F, GFX_BUFFER_SIZE_BYTES); // Solid Blue (RGB565)

    // --- STEP 2: UPDATE DISPLAY LIST ---
    // Tell EVE to display the image currently in RAM_G (which we sent LAST frame)
    API_LIB_BeginCoProList();
    API_CMD_DLSTART();
    API_CLEAR_COLOR_RGB(0, 0, 0);
    API_CLEAR(1, 1, 1);

    API_BITMAP_HANDLE(0);
    API_BITMAP_SOURCE(0); // Display image at RAM_G offset 0
    
    API_BITMAP_LAYOUT(RGB565, GFX_WIDTH * 2, GFX_HEIGHT);
    API_BITMAP_LAYOUT_H((GFX_WIDTH * 2) >> 10, GFX_HEIGHT >> 9);

    API_BITMAP_SIZE(NEAREST, BORDER, BORDER, GFX_WIDTH, GFX_HEIGHT);
    API_BITMAP_SIZE_H(GFX_WIDTH >> 9, GFX_HEIGHT >> 9);

    API_BEGIN(BITMAPS);
    API_VERTEX2II(0, 0, 0, 0);
    API_END();

    API_DISPLAY();
    API_CMD_SWAP();
    API_LIB_EndCoProList();
    API_LIB_AwaitCoProEmpty();

    // --- STEP 3: SEND NEW FRAME (Scatter-Gather) ---
    
    // A. Prepare the Task List for the buffer we just finished drawing
    Gfx_BuildSG_For_Buffer((uint8_t *)g_pDrawingBuffer);

    // B. Start Transaction
    EVE_CS_LOW();
    EVE_AddrForWr(RAM_G); // Always write to RAM_G start (Address 0)

    // C. Fire DMA
    Gfx_Start_SG_Transfer();

    // D. End Transaction (CRITICAL)
    EVE_CS_HIGH();

    // --- STEP 4: SWAP POINTERS ---
    pixel16_t *temp = g_pDrawingBuffer;
    g_pDrawingBuffer = g_pSendingBuffer;
    g_pSendingBuffer = temp;
}

