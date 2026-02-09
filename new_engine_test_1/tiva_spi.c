/*
 * tiva_spi.c
 *
 *  Created on: 12/01/2026
 *      Author: Adrian Pulido
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <driverlib/gpio.h>
#include <driverlib/interrupt.h>
#include <driverlib/pin_map.h>
#include <driverlib/rom_map.h>
#include <driverlib/ssi.h>
#include <driverlib/sysctl.h>
#include <driverlib/udma.h>
#include <inc/hw_ints.h>
#include <inc/hw_ssi.h>

#include "utils/uartstdio.h"

#include "tiva_spi.h"

extern uint8_t pui8ControlTable[1024];

extern const uint32_t g_ui32SysClock;

#define VERBOSE_LV1

void uDMAIntHandler(void) {
  if (MAP_uDMAChannelModeGet(UDMA_CH14_SSI3RX | UDMA_PRI_SELECT) ==
      UDMA_MODE_STOP) {
    // Clear any pending interrupt flags (good practice, though DMA is distinct)
    uint32_t ui32Status = MAP_SSIIntStatus(SSI3_BASE, 1);
    MAP_SSIIntClear(SSI3_BASE, ui32Status);

    // Mark transaction as complete

    // Optional: Call a callback function here if you need to notify the App
    // layer if(MyCallback) MyCallback();
  }
}

uint8_t display_SPI_ReadWrite(uint16_t txData) {
  uint32_t ui32RxData;
  SSIDataPut(SSI3_BASE, txData);

  while (SSIBusy(SSI3_BASE)) {
  }

  SSIDataGet(SSI3_BASE, &ui32RxData);

  return (uint8_t)(ui32RxData & 0xFF);
}

// Helper for unused RX data
static uint8_t g_dummyRxByte;

// Helper for unused TX data (Sending Zeros)
static const volatile uint8_t g_dummyTxZero = 0;

bool display_SPI_uDMA_transfer(const uint8_t *pTxBuffer, uint8_t *pRxBuffer,
                               uint32_t count) {
  if (count == 0)
    return true;

  MAP_uDMAChannelDisable(UDMA_CH14_SSI3RX);
  MAP_uDMAChannelDisable(UDMA_CH15_SSI3TX);

  MAP_SSIIntClear(SSI3_BASE, SSI_DMATX | SSI_DMARX | SSI_RXTO | SSI_RXOR);

  uint32_t garbage;
  while (MAP_SSIDataGetNonBlocking(SSI3_BASE, &garbage))
    ;

  void *pDest =
      (pRxBuffer != NULL) ? (void *)pRxBuffer : (void *)&g_dummyRxByte;
  uint32_t dstInc = (pRxBuffer != NULL) ? UDMA_DST_INC_8 : UDMA_DST_INC_NONE;

  void *pSrc = (pTxBuffer != NULL) ? (void *)pTxBuffer : (void *)&g_dummyTxZero;
  uint32_t srcInc = (pTxBuffer != NULL) ? UDMA_SRC_INC_8 : UDMA_SRC_INC_NONE;

  // Configure RX
  MAP_uDMAChannelControlSet(UDMA_CH14_SSI3RX | UDMA_PRI_SELECT,
                            UDMA_SIZE_8 | UDMA_SRC_INC_NONE | dstInc |
                                UDMA_ARB_1);

  MAP_uDMAChannelTransferSet(UDMA_CH14_SSI3RX | UDMA_PRI_SELECT,
                             UDMA_MODE_BASIC, (void *)(SSI3_BASE + SSI_O_DR),
                             pDest, count);

  // Configure TX
  MAP_uDMAChannelControlSet(UDMA_CH15_SSI3TX | UDMA_PRI_SELECT,
                            UDMA_SIZE_8 | srcInc | UDMA_DST_INC_NONE |
                                UDMA_ARB_1);

  MAP_uDMAChannelTransferSet(UDMA_CH15_SSI3TX | UDMA_PRI_SELECT,
                             UDMA_MODE_BASIC, pSrc,
                             (void *)(SSI3_BASE + SSI_O_DR), count);

  // Enable
  MAP_uDMAChannelEnable(UDMA_CH14_SSI3RX);
  MAP_uDMAChannelEnable(UDMA_CH15_SSI3TX);

  // Blocking Wait
  while ((MAP_uDMAChannelModeGet(UDMA_CH14_SSI3RX | UDMA_PRI_SELECT) !=
          UDMA_MODE_STOP) ||
         (MAP_uDMAChannelModeGet(UDMA_CH15_SSI3TX | UDMA_PRI_SELECT) !=
          UDMA_MODE_STOP)) {
    // Busy Wait
    SysCtlDelay(100);
  }

  while (MAP_SSIBusy(SSI3_BASE))
    ;

  MAP_uDMAChannelDisable(UDMA_CH14_SSI3RX);
  MAP_uDMAChannelDisable(UDMA_CH15_SSI3TX);

  return true;
}
// Helper for unused RX data
static uint32_t g_ulDummyRx;

void EVE_SPI_uDMA_BurstWrite(const uint8_t *pui8Src, uint32_t ui32TotalBytes) {
  uint32_t ui32BytesSent = 0;
  uint32_t ui32TransferSize;

  // Clear any residual SSI status
  MAP_SSIIntClear(SSI3_BASE, SSI_DMATX | SSI_DMARX | SSI_RXTO | SSI_RXOR);
  while (MAP_SSIDataGetNonBlocking(SSI3_BASE, &g_ulDummyRx))
    ;

  // Loop through data in 1024-byte chunks (Hardware uDMA Limit)
  while (ui32BytesSent < ui32TotalBytes) {

    // Calculate current chunk size
    uint32_t ui32Remaining = ui32TotalBytes - ui32BytesSent;
    ui32TransferSize = (ui32Remaining > 1024) ? 1024 : ui32Remaining;

    // --- CONFIGURE RX (Dummy Read) ---
    // Must read RX FIFO to prevent overflow, even if we discard data.
    MAP_uDMAChannelControlSet(UDMA_CH14_SSI3RX | UDMA_PRI_SELECT,
                              UDMA_SIZE_8 | UDMA_SRC_INC_NONE |
                                  UDMA_DST_INC_NONE | UDMA_ARB_1);

    MAP_uDMAChannelTransferSet(UDMA_CH14_SSI3RX | UDMA_PRI_SELECT,
                               UDMA_MODE_BASIC, (void *)(SSI3_BASE + SSI_O_DR),
                               &g_ulDummyRx, ui32TransferSize);

    // --- CONFIGURE TX (Stability Mode) ---
    // ARB_1: Sends 1 byte, releases bus, waits, requests bus again.
    // This prevents SDRAM burst noise.
    MAP_uDMAChannelControlSet(
        UDMA_CH15_SSI3TX | UDMA_PRI_SELECT,
        UDMA_SIZE_8 | UDMA_SRC_INC_8 |       // Increment Source (SDRAM)
            UDMA_DST_INC_NONE | UDMA_ARB_1); // Arbitration Size 1

    MAP_uDMAChannelTransferSet(UDMA_CH15_SSI3TX | UDMA_PRI_SELECT,
                               UDMA_MODE_BASIC, (void *)&pui8Src[ui32BytesSent],
                               (void *)(SSI3_BASE + SSI_O_DR),
                               ui32TransferSize);

    // Enable Channels
    MAP_uDMAChannelEnable(UDMA_CH14_SSI3RX);
    MAP_uDMAChannelEnable(UDMA_CH15_SSI3TX);

    // Blocking Wait for Chunk Completion
    while ((MAP_uDMAChannelModeGet(UDMA_CH14_SSI3RX | UDMA_PRI_SELECT) !=
            UDMA_MODE_STOP) ||
           (MAP_uDMAChannelModeGet(UDMA_CH15_SSI3TX | UDMA_PRI_SELECT) !=
            UDMA_MODE_STOP)) {
      // Optional: SysCtlDelay(1) if needed for extreme stability
    }

    ui32BytesSent += ui32TransferSize;
  }

  // Wait for final bits to leave the SSI Shift Register
  while (MAP_SSIBusy(SSI3_BASE))
    ;
}

void uDMA_Init(void) {
  //
  // Enable uDMA
  //
  MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
  while (!MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_UDMA)) {
  }

  MAP_uDMAEnable();

  //
  // Point to the control table
  //
  MAP_uDMAControlBaseSet(pui8ControlTable);

  MAP_uDMAChannelAssign(UDMA_CH14_SSI3RX);
  MAP_uDMAChannelAssign(UDMA_CH15_SSI3TX);

  uint_fast16_t ui16Idx;


  //
  // Configure CH14 SSI3 RX. First put the channel in a known state
  //
  MAP_uDMAChannelAttributeDisable(
      UDMA_CH14_SSI3RX, UDMA_ATTR_USEBURST | UDMA_PRI_SELECT |
                            (UDMA_ATTR_HIGH_PRIORITY | UDMA_ATTR_REQMASK));
  MAP_uDMAChannelControlSet(UDMA_CH14_SSI3RX | UDMA_PRI_SELECT,
                            UDMA_SIZE_8 | UDMA_SRC_INC_NONE | UDMA_DST_INC_8 |
                                UDMA_ARB_8);

  //
  // Configure CH15 SSI3 TX. First put the channel in a known state
  //
  MAP_uDMAChannelAttributeDisable(
      UDMA_CH15_SSI3TX, UDMA_ATTR_USEBURST | UDMA_PRI_SELECT |
                            (UDMA_ATTR_HIGH_PRIORITY | UDMA_ATTR_REQMASK));
  MAP_uDMAChannelControlSet(UDMA_CH15_SSI3TX | UDMA_PRI_SELECT,
                            UDMA_SIZE_8 | UDMA_SRC_INC_8 | UDMA_DST_INC_NONE |
                                UDMA_ARB_8);
}

void SPI3_Init(void) {
  SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI3);
  while (!SysCtlPeripheralReady(SYSCTL_PERIPH_SSI3)) {
  }

  GPIOPinConfigure(GPIO_PQ0_SSI3CLK);
  GPIOPinConfigure(GPIO_PQ2_SSI3XDAT0);
  GPIOPinConfigure(GPIO_PQ3_SSI3XDAT1);
  GPIOPadConfigSet(GPIO_PORTQ_BASE, GPIO_PIN_0 | GPIO_PIN_2 | GPIO_PIN_3,
                   GPIO_STRENGTH_12MA, GPIO_PIN_TYPE_STD);
  GPIOPinTypeSSI(SPI3_PORT, SPI3_CLK | SPI3_SDI | SPI3_SDO);

  // The FT81x requires < 11 MHz during boot. 9MHz
  SSIConfigSetExpClk(SSI3_BASE, g_ui32SysClock, SSI_FRF_MOTO_MODE_0,
                     SSI_MODE_MASTER, 8E6, 8);
  SSIDMAEnable(SSI3_BASE,
               SSI_DMA_TX | SSI_DMA_RX); // Enable both read and write

  //
  // Enable interruptions at system level
  //
  IntDisable(INT_SSI3);

  SSIEnable(SSI3_BASE);
}

void SPI3_FT81x_HighSpeed(void) {
  SSIDisable(SSI3_BASE);
  SSIConfigSetExpClk(SSI3_BASE, g_ui32SysClock, SSI_FRF_MOTO_MODE_0,
                     SSI_MODE_MASTER, 15E6, 8);
  SSIEnable(SSI3_BASE);
}

void init_SPI_screen(void) {
  GPIOPinTypeGPIOOutput(GPIO_PORTM_BASE, SPI_DISP_PD);
  GPIOPinTypeGPIOOutput(GPIO_PORTQ_BASE, SPI_DISP_CS);
  GPIOPinTypeGPIOOutput(GPIO_PORTK_BASE, SPI_DISP_TURN_OFF);
  GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, SPI_COMM_LED);
  GPIOPadConfigSet(GPIO_PORTQ_BASE, SPI_DISP_CS, GPIO_STRENGTH_12MA,
                   GPIO_PIN_TYPE_STD);
  // EVE_PD_LOW();
  // EVE_TURN_ON_LOW();
  // EVE_CS_LOW();

  uDMA_Init();
  SPI3_Init();
}