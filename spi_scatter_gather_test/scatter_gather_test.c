/* Scatter_gather_test.c */
#include <stdbool.h>
#include <stdint.h>
#include <string.h> // For memset

#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/ssi.h"
#include "driverlib/sysctl.h"
#include "driverlib/udma.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ssi.h"
#include "inc/hw_types.h"

#include "tiva_spi.h"
#include "EVE.h"
#include "scatter_gather_test.h"

#define TRANSFER_SIZE 1024
#define NUM_BUFFERS 10

// --- BUFFERS (10 Separate Arrays) ---
// We make them global to ensure they stay in RAM
uint8_t g_pBuf0[TRANSFER_SIZE]; // Red Stripe
uint8_t g_pBuf1[TRANSFER_SIZE]; // Green Stripe
uint8_t g_pBuf2[TRANSFER_SIZE]; // Blue Stripe
uint8_t g_pBuf3[TRANSFER_SIZE]; // Yellow
uint8_t g_pBuf4[TRANSFER_SIZE]; // Magenta
uint8_t g_pBuf5[TRANSFER_SIZE]; // Cyan
uint8_t g_pBuf6[TRANSFER_SIZE]; // White
uint8_t g_pBuf7[TRANSFER_SIZE]; // Light Gray
uint8_t g_pBuf8[TRANSFER_SIZE]; // Dark Gray
uint8_t g_pBuf9[TRANSFER_SIZE]; // Black

// Array of pointers to make loop configuration easier
uint8_t *g_pBufferList[NUM_BUFFERS] = {
    g_pBuf0, g_pBuf1, g_pBuf2, g_pBuf3, g_pBuf4, 
    g_pBuf5, g_pBuf6, g_pBuf7, g_pBuf8, g_pBuf9
};

// --- DMA TASK LISTS ---
// The uDMA controller requires these to be aligned to 1024 bytes if they are the Control Table,
// but for Scatter-Gather Task Lists loaded from memory, standard alignment is usually fine.
// However, enforcing alignment never hurts.
#if defined(__ICCARM__)
#pragma data_alignment=1024
static tDMAControlTable g_sTxTaskList[NUM_BUFFERS];
static tDMAControlTable g_sRxTaskList[NUM_BUFFERS];
#elif defined(__TI_COMPILER_VERSION__)
#pragma DATA_ALIGN(g_sTxTaskList, 1024)
static tDMAControlTable g_sTxTaskList[NUM_BUFFERS];
#pragma DATA_ALIGN(g_sRxTaskList, 1024)
static tDMAControlTable g_sRxTaskList[NUM_BUFFERS];
#else
static tDMAControlTable g_sTxTaskList[NUM_BUFFERS] __attribute__ ((aligned(1024)));
static tDMAControlTable g_sRxTaskList[NUM_BUFFERS] __attribute__ ((aligned(1024)));
#endif

static uint32_t g_ulDummyRx; // Sink for RX data

void Init_Buffers(void) {
  // Fill buffers with RGB332 colors
  memset(g_pBuf0, 0xE0, TRANSFER_SIZE); // Red (111 00 000)
  memset(g_pBuf1, 0x1C, TRANSFER_SIZE); // Green (000 111 00)
  memset(g_pBuf2, 0x03, TRANSFER_SIZE); // Blue (000 00 011)
  memset(g_pBuf3, 0xFC, TRANSFER_SIZE); // Yellow
  memset(g_pBuf4, 0xE3, TRANSFER_SIZE); // Magenta
  memset(g_pBuf5, 0x1F, TRANSFER_SIZE); // Cyan
  memset(g_pBuf6, 0xFF, TRANSFER_SIZE); // White
  memset(g_pBuf7, 0xB6, TRANSFER_SIZE); // Light Gray
  memset(g_pBuf8, 0x49, TRANSFER_SIZE); // Dark Gray
  memset(g_pBuf9, 0x00, TRANSFER_SIZE); // Black
}

void Setup_SG_Transfer(void) {
  int i;
  for (i = 0; i < NUM_BUFFERS; i++) {
    // 1. Determine Mode
    // Tasks 0-8: PER_SCATTER_GATHER (Fetch next task)
    // Task 9:    BASIC (Stop)
    uint32_t ui32Mode = (i == (NUM_BUFFERS - 1)) ? UDMA_MODE_BASIC : UDMA_MODE_PER_SCATTER_GATHER;

    // 2. Configure TX Task using Macro (Handles ALT_SELECT flag automatically)
    //    We cast to (tDMAControlTable) to allow assignment in C
    g_sTxTaskList[i] = (tDMAControlTable)uDMATaskStructEntry(
        TRANSFER_SIZE,          // Count
        UDMA_SIZE_8,            // Item Size
        UDMA_SRC_INC_8,         // Src Increment (Walk buffer)
        g_pBufferList[i],       // Src Start Address (Macro calculates End)
        UDMA_DST_INC_NONE,      // Dst Increment (Fixed)
        (void *)(SSI3_BASE + SSI_O_DR), // Dst Address
        UDMA_ARB_4,             // Arbitration
        ui32Mode                // Mode
    );

    // 3. Configure RX Task (Trash Can)
    g_sRxTaskList[i] = (tDMAControlTable)uDMATaskStructEntry(
        TRANSFER_SIZE, 
        UDMA_SIZE_8, 
        UDMA_SRC_INC_NONE,      // Src Inc (Fixed at SSI Register)
        (void *)(SSI3_BASE + SSI_O_DR), 
        UDMA_DST_INC_NONE,      // Dst Inc (Fixed at Dummy)
        (void *)&g_ulDummyRx, 
        UDMA_ARB_4, 
        ui32Mode
    );
  }
}

// --- EXECUTE SCATTER-GATHER TRANSFER ---
void SPI_Start_SG_Transfer(void) {
  // 1. Clear Errors
  MAP_uDMAErrorStatusClear();

  // 2. Clean Hardware & Interrupts
  MAP_SSIIntDisable(SSI3_BASE, SSI_DMATX | SSI_DMARX | SSI_RXFF | SSI_RXOR | SSI_RXTO);
  MAP_SSIIntClear(SSI3_BASE, SSI_DMATX | SSI_DMARX | SSI_RXFF | SSI_RXOR | SSI_RXTO);

  // 3. Drain FIFO (CRITICAL STEP)
  //    This prevents "old" data from confusing the new RX DMA transfer
  uint32_t trash;
  while (MAP_SSIDataGetNonBlocking(SSI3_BASE, &trash));
  
  //    Explicitly clear any Overrun errors that might have occurred during the drain
  MAP_SSIIntClear(SSI3_BASE, SSI_RXOR | SSI_RXTO);

  // 4. Configure Primary Structures to point to our Task Lists
  MAP_uDMAChannelScatterGatherSet(UDMA_CH15_SSI3TX, NUM_BUFFERS, g_sTxTaskList, 1);
  MAP_uDMAChannelScatterGatherSet(UDMA_CH14_SSI3RX, NUM_BUFFERS, g_sRxTaskList, 1);

  // 5. Enable Channels
  MAP_uDMAChannelEnable(UDMA_CH14_SSI3RX);
  MAP_uDMAChannelEnable(UDMA_CH15_SSI3TX);

  // 6. Enable SSI DMA Triggers (Last step before kicking)
  MAP_SSIDMAEnable(SSI3_BASE, SSI_DMA_TX | SSI_DMA_RX);

  // 7. Trigger Transfer (Kickstart)
  //    Note: CS must be LOW before calling this!
  MAP_uDMAChannelRequest(UDMA_CH15_SSI3TX);

  // 8. Blocking Wait
  while (MAP_uDMAChannelIsEnabled(UDMA_CH15_SSI3TX)) {
      // Overrun Protection (Just in case)
      if(HWREG(SSI3_BASE + SSI_O_RIS) & SSI_RIS_RORRIS) {
          HWREG(SSI3_BASE + SSI_O_ICR) = SSI_ICR_RORIC;
      }
  }

  // 9. Wait for SPI Bus Idle
  while (MAP_SSIBusy(SSI3_BASE));

  // 10. Disable SSI DMA Triggers (Cleanup)
  MAP_SSIDMADisable(SSI3_BASE, SSI_DMA_TX | SSI_DMA_RX);
  
  // CS is raised in the calling function
  
  MAP_uDMAChannelDisable(UDMA_CH14_SSI3RX);
}