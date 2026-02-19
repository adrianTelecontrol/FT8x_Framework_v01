/*
 * tiva_spi.c
 * Matches TI Example: udma_scatter_gather.c logic
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
#include "tiva_spi.h"

// --- MEMORY ALIGNMENT ---
// Matches the official example lines 138-146
#if defined(ewarm)
#pragma data_alignment = 1024
uint8_t pui8ControlTable[1024];
#elif defined(ccs)
#pragma DATA_ALIGN(pui8ControlTable, 1024)
uint8_t pui8ControlTable[1024];
#else
uint8_t pui8ControlTable[1024] __attribute__((aligned(1024)));
#endif

volatile bool g_bSPI_TransferDone = true;
extern const uint32_t g_ui32SysClock;
static const volatile uint8_t g_dummyTxZero = 0;

void SSI3IntHandler(void) {
  uint32_t ui32Status;

  // Read interrupt status
  ui32Status = MAP_SSIIntStatus(SSI3_BASE, 1);

  // Clear the flags
  MAP_SSIIntClear(SSI3_BASE, ui32Status);

  // ui32Status = MAP_SSIIntStatus(SSI3_BASE, 1);

  // Check if it was a DMA finish
  // (Note: Sometimes Tiva just fires the raw interrupt, checking mode is safer)
  // if ((MAP_uDMAChannelModeGet(UDMA_CH14_SSI3RX | UDMA_PRI_SELECT) ==
  //      UDMA_MODE_STOP) &&
  //     (MAP_uDMAChannelModeGet(UDMA_CH15_SSI3TX | UDMA_PRI_SELECT) ==
  //      UDMA_MODE_STOP)) 
	if(!MAP_uDMAChannelIsEnabled(UDMA_CH15_SSI3TX))
	   {
    // Signal your main loop that the SPI bus is free
    g_bSPI_TransferDone = true;

    while (MAP_SSIBusy(SSI3_BASE))
      ;

    // SSIIntDisable(SSI3_BASE, SSI_TXFF | SSI_RXFF | SSI_RXOR | SSI_RXTO);
    MAP_SSIIntDisable(SSI3_BASE, SSI_DMATX | SSI_DMARX);

    MAP_uDMAChannelDisable(UDMA_CH14_SSI3RX);
    MAP_uDMAChannelDisable(UDMA_CH15_SSI3TX);
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

// --- INIT ---
bool display_SPI_uDMA_transfer(const uint8_t *pTxBuffer, uint8_t *pRxBuffer,
                               uint32_t count) {
  if (count == 0)
    return true;

  uint8_t g_dummyRxByte;

  // Clear flag
  g_bSPI_TransferDone = false;

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

#ifdef SPI_uDMA_USE_INTER
  //    We enable "DMA Transmit Complete" (DMATX) or "End of Transmission"
  //    (EOT) Note: On some Tiva chips, DMATX is the correct flag for DMA
  //    completion.
  MAP_SSIIntEnable(SSI3_BASE, SSI_DMATX);

#else

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
#endif
  return true;
}

void uDMA_Init(void) {
  MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
  while (!MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_UDMA))
    ;

  MAP_uDMAEnable();
  MAP_uDMAControlBaseSet(pui8ControlTable);

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
                         SSI_MODE_MASTER, 9000000, 8);

  MAP_SSIDMAEnable(SSI3_BASE, SSI_DMA_TX | SSI_DMA_RX);

  // Enable Interrupts (Required for uDMA even in blocking mode)
  MAP_IntEnable(INT_SSI3);
  MAP_SSIEnable(SSI3_BASE);
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

  uDMA_Init();
  SPI3_Init();
}
