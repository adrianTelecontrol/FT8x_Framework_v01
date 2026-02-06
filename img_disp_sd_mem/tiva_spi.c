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

uint8_t pui8TxBuff[SPI_TRX_SIZE]; // Max transaction size
uint8_t pui8RxBuff[SPI_TRX_SIZE];

extern uint32_t g_ui32SysClock;

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

uint8_t display_SPI_ReadWrite(uint16_t txData)
{
    uint32_t ui32RxData;
    SSIDataPut(SSI3_BASE, txData);

    while(SSIBusy(SSI3_BASE))
    {   
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

  // [STEP 1] DISABLE CHANNELS FIRST (CRITICAL)
  // <--- MISSING IN YOUR CODE
  // If you skip this, the DMA logic might not reset for the new size (1 vs 2
  // bytes)
  MAP_uDMAChannelDisable(UDMA_CH14_SSI3RX);
  MAP_uDMAChannelDisable(UDMA_CH15_SSI3TX);

  // [STEP 2] CLEAR FLAGS
  MAP_SSIIntClear(SSI3_BASE, SSI_DMATX | SSI_DMARX | SSI_RXTO | SSI_RXOR);

  // [STEP 3] DRAIN THE FIFO (CRITICAL)
  // <--- MISSING IN YOUR CODE
  // This removes any "Ghost Bytes" left over from EVE_Read8 or AddrForRd.
  // Without this, the DMA reads old data immediately and finishes early (or
  // hangs).
  uint32_t garbage;
  while (MAP_SSIDataGetNonBlocking(SSI3_BASE, &garbage)) {
    // Keep reading until FIFO is empty
  }

  // [STEP 4] Setup Pointers
  void *pDest =
      (pRxBuffer != NULL) ? (void *)pRxBuffer : (void *)&g_dummyRxByte;
  uint32_t dstInc = (pRxBuffer != NULL) ? UDMA_DST_INC_8 : UDMA_DST_INC_NONE;

  void *pSrc = (pTxBuffer != NULL) ? (void *)pTxBuffer : (void *)&g_dummyTxZero;
  uint32_t srcInc = (pTxBuffer != NULL) ? UDMA_SRC_INC_8 : UDMA_SRC_INC_NONE;

  // [STEP 5] Configure RX
  MAP_uDMAChannelControlSet(UDMA_CH14_SSI3RX | UDMA_PRI_SELECT,
                            UDMA_SIZE_8 | UDMA_SRC_INC_NONE | dstInc |
                                UDMA_ARB_1);

  MAP_uDMAChannelTransferSet(UDMA_CH14_SSI3RX | UDMA_PRI_SELECT,
                             UDMA_MODE_BASIC, (void *)(SSI3_BASE + SSI_O_DR),
                             pDest, count);

  // [STEP 6] Configure TX
  MAP_uDMAChannelControlSet(UDMA_CH15_SSI3TX | UDMA_PRI_SELECT,
                            UDMA_SIZE_8 | srcInc | UDMA_DST_INC_NONE |
                                UDMA_ARB_1);

  MAP_uDMAChannelTransferSet(UDMA_CH15_SSI3TX | UDMA_PRI_SELECT,
                             UDMA_MODE_BASIC, pSrc,
                             (void *)(SSI3_BASE + SSI_O_DR), count);

  // [STEP 7] Enable
  MAP_uDMAChannelEnable(UDMA_CH14_SSI3RX);
  MAP_uDMAChannelEnable(UDMA_CH15_SSI3TX);

  // [STEP 8] Blocking Wait
  while ((MAP_uDMAChannelModeGet(UDMA_CH14_SSI3RX | UDMA_PRI_SELECT) !=
          UDMA_MODE_STOP) ||
         (MAP_uDMAChannelModeGet(UDMA_CH15_SSI3TX | UDMA_PRI_SELECT) !=
          UDMA_MODE_STOP)) {
    // Busy Wait
  }

  return true;
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
  // Initialize TRX vectors
  //
  for (ui16Idx = 0; ui16Idx < SPI_TRX_SIZE; ui16Idx++) {
    pui8TxBuff[ui16Idx] = 0;
    pui8RxBuff[ui16Idx] = 0;
  }

  //
  // Configure CH14 SSI3 RX. First put the channel in a known state
  //
  MAP_uDMAChannelAttributeDisable(
      UDMA_CH14_SSI3RX, UDMA_ATTR_USEBURST | UDMA_PRI_SELECT |
                            (UDMA_ATTR_HIGH_PRIORITY | UDMA_ATTR_REQMASK));
  MAP_uDMAChannelControlSet(UDMA_CH14_SSI3RX | UDMA_PRI_SELECT,
                            UDMA_SIZE_8 | UDMA_SRC_INC_NONE | UDMA_DST_INC_8 |
                                UDMA_ARB_1);

  //
  // Configure CH15 SSI3 TX. First put the channel in a known state
  //
  MAP_uDMAChannelAttributeDisable(
      UDMA_CH15_SSI3TX, UDMA_ATTR_USEBURST | UDMA_PRI_SELECT |
                            (UDMA_ATTR_HIGH_PRIORITY | UDMA_ATTR_REQMASK));
  MAP_uDMAChannelControlSet(UDMA_CH15_SSI3TX | UDMA_PRI_SELECT,
                            UDMA_SIZE_8 | UDMA_SRC_INC_8 | UDMA_DST_INC_NONE |
                                UDMA_ARB_1);
}

void SPI3_Init(void) {
  SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI3);
  while (!SysCtlPeripheralReady(SYSCTL_PERIPH_SSI3)) {
  }

  GPIOPinConfigure(GPIO_PQ0_SSI3CLK);
  GPIOPinConfigure(GPIO_PQ2_SSI3XDAT0);
  GPIOPinConfigure(GPIO_PQ3_SSI3XDAT1);
  GPIOPinTypeSSI(SPI3_PORT, SPI3_CLK | SPI3_SDI | SPI3_SDO);
  SSIConfigSetExpClk(SSI3_BASE, g_ui32SysClock, SSI_FRF_MOTO_MODE_0,
                     SSI_MODE_MASTER, 9000000, 8);
  // SSIDMAEnable(SSI3_BASE,
  //              SSI_DMA_TX | SSI_DMA_RX); // Enable both read and write

  //
  // Enable interruptions at system level
  //
  // IntDisable(INT_SSI3);

  SSIEnable(SSI3_BASE);
}

void init_SPI_screen(void) {
  GPIOPinTypeGPIOOutput(GPIO_PORTM_BASE, SPI_DISP_PD);
  GPIOPinTypeGPIOOutput(GPIO_PORTQ_BASE, SPI_DISP_CS);
  GPIOPinTypeGPIOOutput(GPIO_PORTK_BASE, SPI_DISP_TURN_OFF);
  GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, SPI_COMM_LED);

  //EVE_PD_LOW();
  //EVE_TURN_ON_LOW();
  //EVE_CS_LOW();

  //uDMA_Init();
  SPI3_Init();
}
