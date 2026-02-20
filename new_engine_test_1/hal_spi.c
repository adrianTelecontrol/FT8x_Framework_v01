/*
 * hal_spi.c
 *
 *  Created on: 19/02/26
 *      Author: Adrian
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
#include <inc/hw_memmap.h>
#include <inc/hw_ssi.h>
#include <inc/hw_types.h>

#include "graphics_engine.h"
#include "hal_spi.h"

// --- MEMORY ALIGNMENT ---
// Matches the official example lines 138-146
#if defined(ewarm)
#pragma data_alignment = 1024
uint8_t g_HAL_uDMA_ControlTable[1024];
#elif defined(ccs)
#pragma DATA_ALIGN(g_HAL_uDMA_ControlTable, 1024)
uint8_t g_HAL_uDMA_ControlTable[1024];
#else
uint8_t g_HAL_uDMA_ControlTable[1024] __attribute__((aligned(1024)));
#endif

extern const uint32_t g_ui32SysClock;  // Get system clock from PLL

volatile bool g_bSPI_TransferActive = false;
static const volatile uint8_t g_dummyTxZero = 0;

// SSI Interrupt handler
void SSI3IntHandler(void) {
  // 1. Read and clear the interrupt status
  uint32_t ui32Status = MAP_SSIIntStatus(SSI3_BASE, 1);
  MAP_SSIIntClear(SSI3_BASE, ui32Status);

  // 2. Check if this interrupt is because the DMA transfer finished
  if (g_bSPI_TransferActive && !MAP_uDMAChannelIsEnabled(UDMA_CH15_SSI3TX)) {

    // Wait for the final few bits to shift out of the SPI hardware
    while (MAP_SSIBusy(SSI3_BASE))
      ;

    // CRITICAL: Close the EVE SPI transaction!
    HAL_SPI_CS_Disable();

    // Clean up DMA and disable this interrupt until the next frame
    MAP_SSIDMADisable(SSI3_BASE, SSI_DMA_TX | SSI_DMA_RX);
    MAP_SSIIntDisable(SSI3_BASE, SSI_TXFF);

    // Signal to the CPU that the DMA is free!
    g_bSPI_TransferActive = false;
  }

  // Safety catch for RX Overruns
  if (ui32Status & SSI_RXOR) {
    MAP_SSIIntClear(SSI3_BASE, SSI_RXOR);
  }
}

bool HAL_SPI_IsBusy(void)
{
	return g_bSPI_TransferActive;
}

uint8_t HAL_SPI_ReadWrite8(uint16_t txData) {
  uint32_t ui32RxData;
  SSIDataPut(SSI3_BASE, txData);

  while (SSIBusy(SSI3_BASE)) {
  }

  SSIDataGet(SSI3_BASE, &ui32RxData);

  return (uint8_t)(ui32RxData & 0xFF);
}

// --- INIT ---
bool HAL_SPI_uDMATransfer(const uint8_t *pTxBuffer, uint8_t *pRxBuffer,
                          uint32_t count, bool bIsBlocking) {
  if (count == 0)
    return true;

  uint8_t g_dummyRxByte;

  // Clear flag
  g_bSPI_TransferActive = true;

  MAP_SSIIntDisable(SSI3_BASE, SSI_DMATX | SSI_DMARX);
  MAP_SSIIntClear(SSI3_BASE, SSI_DMATX | SSI_DMARX | SSI_RXTO | SSI_RXOR);

  MAP_uDMAChannelDisable(UDMA_CH14_SSI3RX);
  MAP_uDMAChannelDisable(UDMA_CH15_SSI3TX);

  MAP_SSIIntDisable(SSI3_BASE, SSI_RXFF | SSI_RXOR | SSI_RXTO | SSI_DMARX);
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

  if (!bIsBlocking) {
    //    We enable "DMA Transmit Complete" (DMATX) or "End of Transmission"
    //    (EOT) Note: On some Tiva chips, DMATX is the correct flag for DMA
    //    completion.
    MAP_SSIIntEnable(SSI3_BASE, SSI_DMATX);
  } else {

    while ((MAP_uDMAChannelModeGet(UDMA_CH14_SSI3RX | UDMA_PRI_SELECT) !=
            UDMA_MODE_STOP) ||
           (MAP_uDMAChannelModeGet(UDMA_CH15_SSI3TX | UDMA_PRI_SELECT) !=
            UDMA_MODE_STOP)) {
      // Busy Wait
      SysCtlDelay(100);
    }

    while (MAP_SSIBusy(SSI3_BASE))
      ;
  	
	g_bSPI_TransferActive = true;

    MAP_uDMAChannelDisable(UDMA_CH14_SSI3RX);
    MAP_uDMAChannelDisable(UDMA_CH15_SSI3TX);
  }

  return true;
}

void uDMA_Init(void) {
  MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
  while (!MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_UDMA))
    ;

  MAP_uDMAEnable();
  MAP_uDMAControlBaseSet(g_HAL_uDMA_ControlTable);

  MAP_uDMAChannelAssign(UDMA_CH14_SSI3RX);
  MAP_uDMAChannelAssign(UDMA_CH15_SSI3TX);

}

void SPI3_Init(void) {
  MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI3);
  while (!MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_SSI3))
    ;

  MAP_GPIOPinConfigure(GPIO_PQ0_SSI3CLK);
  MAP_GPIOPinConfigure(GPIO_PQ2_SSI3XDAT0);
  MAP_GPIOPinConfigure(GPIO_PQ3_SSI3XDAT1);
  MAP_GPIOPadConfigSet(GPIO_PORTQ_BASE, GPIO_PIN_0 | GPIO_PIN_2 | GPIO_PIN_3,
                       GPIO_STRENGTH_12MA, GPIO_PIN_TYPE_STD);
  MAP_GPIOPinTypeSSI(GPIO_PORTQ_BASE, GPIO_PIN_0 | GPIO_PIN_2 | GPIO_PIN_3);

  MAP_SSIConfigSetExpClk(SSI3_BASE, g_ui32SysClock, SSI_FRF_MOTO_MODE_0,
                         SSI_MODE_MASTER, HAL_SPI_LOW_BITRATE, 8);

  MAP_SSIDMAEnable(SSI3_BASE, SSI_DMA_TX | SSI_DMA_RX);

  // Enable Interrupts (Required for uDMA even in blocking mode)
  MAP_IntEnable(INT_SSI3);
  MAP_SSIEnable(SSI3_BASE);
}

void HAL_SPI_SetHighSpeed(void) {
  SSIDisable(SSI3_BASE);
  SSIConfigSetExpClk(SSI3_BASE, g_ui32SysClock, SSI_FRF_MOTO_MODE_0,
                     SSI_MODE_MASTER, HAL_SPI_HIGH_BITRATE, 8);
  SSIEnable(SSI3_BASE);
}

void HAL_SPI_Init(void) {
  GPIOPinTypeGPIOOutput(GPIO_PORTM_BASE, HAL_SPI_PD);
  GPIOPinTypeGPIOOutput(GPIO_PORTQ_BASE, HAL_SPI_CS);
  GPIOPinTypeGPIOOutput(GPIO_PORTK_BASE, HAL_SPI_POWD);
  GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, HAL_SPI_LED);
  GPIOPadConfigSet(GPIO_PORTQ_BASE, HAL_SPI_CS, GPIO_STRENGTH_12MA,
                   GPIO_PIN_TYPE_STD);

  uDMA_Init();
  SPI3_Init();
}
