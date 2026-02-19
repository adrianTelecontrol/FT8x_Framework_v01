#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "inc/hw_memmap.h"
#include "inc/hw_ssi.h"
#include "inc/hw_types.h"

#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom_map.h"
#include "driverlib/ssi.h"
#include "driverlib/sysctl.h"
#include "driverlib/udma.h"
#include "utils/uartstdio.h" // Assuming you use this for logs

#define SYSCLK 120000000
#define MS_2_CLK(ms)                                                           \
  ((uint32_t)(((SYSCLK) / (3.0f)) / ((1.0f) / ((ms) / (1000.0f)))))

#define REG_ID 3153920UL
#define REG_CPURESET 3153952UL

#define MEM_READ 0x00  // FT800 Host Memory Read
#define MEM_WRITE 0x80 // FT800 Host Memory Write
// #define MEM_WRITE 0x02 // FT800 Host Memory Write

#define SPI3_PORT GPIO_PORTQ_BASE
#define SPI3_CLK GPIO_PIN_0
#define SPI3_SDO GPIO_PIN_2
#define SPI3_SDI GPIO_PIN_3

#define HIGH 0XFF // Estado alto para los pines
#define LOW 0X00  // Estado bajo para los pines

#define SPI_DISP_CS GPIO_PIN_1 // PQ1   Chip selector de la pantalla
#define SPI_DISP_INT                                                           \
  GPIO_PIN_3 // PP3   pin que recibe interrupciones de la pantalla
#define SPI_DISP_PD GPIO_PIN_6 // PM6   pin para despertar a la pantalla
#define SPI_DISP_TURN_OFF GPIO_PIN_7
#define SPI_COMM_LED GPIO_PIN_0
#define SPI_TRX_SIZE 1024

inline void EVE_CS_LOW() { GPIOPinWrite(GPIO_PORTQ_BASE, SPI_DISP_CS, LOW); }
inline void EVE_CS_HIGH() { GPIOPinWrite(GPIO_PORTQ_BASE, SPI_DISP_CS, HIGH); }
inline void EVE_PD_LOW() { GPIOPinWrite(GPIO_PORTM_BASE, SPI_DISP_PD, LOW); }
inline void EVE_PD_HIGH() { GPIOPinWrite(GPIO_PORTM_BASE, SPI_DISP_PD, HIGH); }

// --- PINS (Adjust to your board) ---
#define FT_CS_PORT GPIO_PORTQ_BASE
#define FT_CS_PIN GPIO_PIN_1
#define FT_PD_PORT GPIO_PORTM_BASE
#define FT_PD_PIN GPIO_PIN_6

#define TRANSFER_SIZE 1024
#define NUM_TASKS 3

// --- ALIGNMENT ---
#if defined(__ICCARM__)
#pragma data_alignment = 1024
uint8_t ControlTable[1024];
#elif defined(__TI_COMPILER_VERSION__)
#pragma DATA_ALIGN(ControlTable, 1024)
uint8_t ControlTable[1024];
#else
uint8_t ControlTable[1024] __attribute__((aligned(1024)));
#endif

// DMA Task structures
tDMAControlTable txTask[NUM_TASKS];
tDMAControlTable rxTask[NUM_TASKS];

// Source Buffers
uint8_t buf0[TRANSFER_SIZE];
uint8_t buf1[TRANSFER_SIZE];
uint8_t buf2[TRANSFER_SIZE];

// Verification Buffer (Big enough to hold everything we read back)
uint8_t readBackBuf[TRANSFER_SIZE * NUM_TASKS];

uint32_t dummyRx;

// ------------------------------------------------
// SPI LOW LEVEL
// ------------------------------------------------
uint8_t SPI_readWrite(uint16_t txData) {
  uint32_t ui32RxData;
  SSIDataPut(SSI3_BASE, txData);

  while (SSIBusy(SSI3_BASE)) {
  }

  SSIDataGet(SSI3_BASE, &ui32RxData);

  return (uint8_t)(ui32RxData & 0xFF);
}

// ------------------------------------------------
// EVE HELPER FUNCTIONS
// ------------------------------------------------
uint8_t FT_Read8(uint32_t ftAddress) {
  uint8_t id;

  EVE_CS_LOW();
  SPI_readWrite((uint8_t)((ftAddress >> 16) | MEM_READ));
  SPI_readWrite((uint8_t)(ftAddress >> 8));
  SPI_readWrite((uint8_t)(ftAddress));
  SPI_readWrite(0x00);

  id = SPI_readWrite(0x00);
  EVE_CS_HIGH();

  return id;
}

void EVE_Init(void) {
  // Wake up the device
  EVE_CS_LOW();
  SPI_readWrite(0x00);
  SPI_readWrite(0x00);
  SPI_readWrite(0x00);
  EVE_CS_LOW();
  SysCtlDelay(MS_2_CLK(40));

  // Setup the CLk to be external an run at default speed (60 MHz)
  EVE_CS_LOW();
  SPI_readWrite(0x40);
  SPI_readWrite(0x00);
  SPI_readWrite(0x00);
  EVE_CS_LOW();
  // EVE_CmdWrite(FT81x_HOST_CMD_CLKSEL, FT81x_HOST_PARAM_CLK_DEFAULT);

  // Cmd_Active start the self diagnosis process and may take up to 300ms.
  // But we can read REG_ID to verify this step
  while (FT_Read8(REG_ID) != 0x7C) {
    // TIVA_LOGE(TASK_NAME, "Error! FT81x ID mismatch! Retrying...");
    SysCtlDelay(MS_2_CLK(100));
  }
  // TIVA_LOGI(TASK_NAME, "REG_ID is correct!");

  // Ensure CPUreset register reads 0 and so FT8xx is ready
  while (FT_Read8(REG_CPURESET) != 0x00) {
    SysCtlDelay(MS_2_CLK(100));
  }
}

void API_WakeUpScreen(void) {
  EVE_PD_LOW();
  SysCtlDelay(MS_2_CLK(1000)); // 350

  EVE_PD_HIGH();
  SysCtlDelay(MS_2_CLK(500)); // Must be at least 20ms

  EVE_Init();
}

// Start a WRITE transaction (Leaves CS LOW for DMA)
void FT_StartWrite(uint32_t ftAddress) {
  SPI_readWrite((uint8_t)((ftAddress >> 16) | MEM_WRITE));
  SPI_readWrite((uint8_t)(ftAddress >> 8));
  SPI_readWrite((uint8_t)(ftAddress));
  // SPI_readWrite(0x00);
}

// Blocking Read for Verification
void FT_Read_RAM(uint32_t ftAddress, uint8_t *ui8Dest) {

  EVE_CS_LOW();
  SPI_readWrite((uint8_t)((ftAddress >> 16) | MEM_READ));
  SPI_readWrite((uint8_t)(ftAddress >> 8));
  SPI_readWrite((uint8_t)(ftAddress));
  SPI_readWrite(0x00);

  *ui8Dest = SPI_readWrite(0x00);

  EVE_CS_HIGH();
}

// ------------------------------------------------
// DMA INIT & BUILD
// ------------------------------------------------

void DMA_Init(void) {
  SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
  while (!SysCtlPeripheralReady(SYSCTL_PERIPH_UDMA))
    ;
  uDMAEnable();
  uDMAControlBaseSet(ControlTable);
  uDMAChannelAssign(UDMA_CH15_SSI3TX);
  uDMAChannelAssign(UDMA_CH14_SSI3RX);
  uDMAChannelAttributeDisable(UDMA_CH15_SSI3TX, UDMA_ATTR_ALL);
  uDMAChannelAttributeDisable(UDMA_CH14_SSI3RX, UDMA_ATTR_ALL);
}

void SPI_Init(void) {
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOQ);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI3);
  GPIOPinConfigure(GPIO_PQ0_SSI3CLK);
  GPIOPinConfigure(GPIO_PQ2_SSI3XDAT0);
  GPIOPinConfigure(GPIO_PQ3_SSI3XDAT1);
  GPIOPinTypeSSI(GPIO_PORTQ_BASE, GPIO_PIN_0 | GPIO_PIN_2 | GPIO_PIN_3);
  GPIOPinTypeGPIOOutput(FT_CS_PORT, FT_CS_PIN);
  SSIConfigSetExpClk(SSI3_BASE, SYSCLK, SSI_FRF_MOTO_MODE_0, SSI_MODE_MASTER,
                     10000000, 8);
  SSIEnable(SSI3_BASE);
  // SSIDMAEnable(SSI3_BASE, SSI_DMA_TX | SSI_DMA_RX);
}

void BuildSG(void) {
  // ----------------------------------------------------------------
  // TASK 0 (Scatter-Gather)
  // ----------------------------------------------------------------
  tDMAControlTable TaskTx0 = uDMATaskStructEntry(
      TRANSFER_SIZE,                  // Count (1024)
      UDMA_SIZE_8,                    // Item Size (8-bit)
      UDMA_SRC_INC_8,                 // Src Inc (Walk through buffer)
      buf0,                           // SRC: Start of Buffer 0
      UDMA_DST_INC_NONE,              // Dst Inc (Fixed)
      (void *)(SSI3_BASE + SSI_O_DR), // DST: SSI Data Register
      UDMA_ARB_4,                     // Arbitration
      UDMA_MODE_PER_SCATTER_GATHER    // Mode: Fetch Next Task
  );

  tDMAControlTable TaskRx0 = uDMATaskStructEntry(
      TRANSFER_SIZE, UDMA_SIZE_8,
      UDMA_SRC_INC_NONE,              // Src Inc (Fixed)
      (void *)(SSI3_BASE + SSI_O_DR), // SRC: SSI Data Register
      UDMA_DST_INC_NONE,              // Dst Inc (Fixed)
      &dummyRx,                       // DST: Dummy Variable
      UDMA_ARB_4, UDMA_MODE_PER_SCATTER_GATHER);

  // ----------------------------------------------------------------
  // TASK 1 (Scatter-Gather)
  // ----------------------------------------------------------------
  tDMAControlTable TaskTx1 =
      uDMATaskStructEntry(TRANSFER_SIZE, UDMA_SIZE_8, UDMA_SRC_INC_8,
                          buf1, // SRC: Start of Buffer 1
                          UDMA_DST_INC_NONE, (void *)(SSI3_BASE + SSI_O_DR),
                          UDMA_ARB_4, UDMA_MODE_PER_SCATTER_GATHER);

  tDMAControlTable TaskRx1 =
      uDMATaskStructEntry(TRANSFER_SIZE, UDMA_SIZE_8, UDMA_SRC_INC_NONE,
                          (void *)(SSI3_BASE + SSI_O_DR), UDMA_DST_INC_NONE,
                          &dummyRx, UDMA_ARB_4, UDMA_MODE_PER_SCATTER_GATHER);

  // ----------------------------------------------------------------
  // TASK 2 (Basic - STOPS DMA)
  // ----------------------------------------------------------------
  tDMAControlTable TaskTx2 = uDMATaskStructEntry(
      TRANSFER_SIZE, UDMA_SIZE_8, UDMA_SRC_INC_8,
      buf2, // SRC: Start of Buffer 2
      UDMA_DST_INC_NONE, (void *)(SSI3_BASE + SSI_O_DR), UDMA_ARB_4,
      UDMA_MODE_BASIC // Mode: Stop after this
  );

  tDMAControlTable TaskRx2 = uDMATaskStructEntry(
      TRANSFER_SIZE, UDMA_SIZE_8, UDMA_SRC_INC_NONE,
      (void *)(SSI3_BASE + SSI_O_DR), UDMA_DST_INC_NONE, &dummyRx, UDMA_ARB_4,
      UDMA_MODE_BASIC // Mode: Stop after this
  );

  // ----------------------------------------------------------------
  // ASSIGN TO GLOBAL LISTS
  // ----------------------------------------------------------------
  txTask[0] = TaskTx0;
  txTask[1] = TaskTx1;
  txTask[2] = TaskTx2;

  rxTask[0] = TaskRx0;
  rxTask[1] = TaskRx1;
  rxTask[2] = TaskRx2;
}

void StartSG(void) {
  // 1. Configure uDMA First
  uDMAChannelScatterGatherSet(UDMA_CH15_SSI3TX, NUM_TASKS, txTask, 1);
  uDMAChannelScatterGatherSet(UDMA_CH14_SSI3RX, NUM_TASKS, rxTask, 1);
  uDMAChannelEnable(UDMA_CH14_SSI3RX);
  uDMAChannelEnable(UDMA_CH15_SSI3TX);

  // 2. Enable SSI Triggers Second
  SSIDMAEnable(SSI3_BASE, SSI_DMA_TX | SSI_DMA_RX);

  // 3. Kickstart
  uDMAChannelRequest(UDMA_CH15_SSI3TX);

  // 4. Wait
  while (uDMAChannelIsEnabled(UDMA_CH15_SSI3TX))
    ;
  while (SSIBusy(SSI3_BASE))
    ;

  // 5. Cleanup
  SSIDMADisable(SSI3_BASE, SSI_DMA_TX | SSI_DMA_RX);
}

static uint32_t buffACount, buffBCount, buffCCount;
// ------------------------------------------------
// MAIN
// ------------------------------------------------
int main(void) {
  SysCtlClockFreqSet(SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_25MHZ,
                     SYSCLK);

  MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOM);
  MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOQ);
  // 1. Setup
  SPI_Init();
  DMA_Init();

  // 2. Wake Up & Check ID
  API_WakeUpScreen();

  // 3. Fill Buffers (Test Pattern)
  memset(buf0, 0xAA, TRANSFER_SIZE); // Stripe 1
  memset(buf1, 0x55, TRANSFER_SIZE); // Stripe 2
  memset(buf2, 0x77, TRANSFER_SIZE); // Stripe 3

  // 4. Build List
  BuildSG();

  // 5. DMA TRANSFER
  EVE_CS_LOW();
  FT_StartWrite(0x000000); // Start writing at Address 0
  StartSG();               // Blast the 3 buffers
  EVE_CS_HIGH();

  // ------------------------------------------------
  // 6. VERIFICATION
  // ------------------------------------------------

  // Clear read buffer to ensure we aren't seeing old data
  memset(readBackBuf, 0x00, sizeof(readBackBuf));

  // Read back all 3072 bytes from EVE RAM_G
  uint8_t dest = 1;

  uint32_t ui32Index = 0;
  buffACount = buffBCount = buffCCount = 0;
  for (; ui32Index < TRANSFER_SIZE * 3; ui32Index++) {

    FT_Read_RAM(ui32Index, &dest);

    if (dest == 0xAA)
      buffACount++;
    else if (dest == 0x55)
      buffBCount++;
    else if (dest == 0x77)
      buffCCount++;
  }

  // SUCCESS!
  while (1) {
    // Blink an LED here if you have one
    SysCtlDelay(MS_2_CLK(100));
  }
}
