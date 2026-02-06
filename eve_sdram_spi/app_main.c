
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include <inc/hw_memmap.h>
#include <inc/hw_types.h>
#include <inc/hw_epi.h>

#include "driverlib/gpio.h"
#include "drivers/pinout.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "driverlib/epi.h"

#include "utils/uartstdio.h"

#include "helpers.h"
#include "gfx.h"
#include "tiva_spi.h"
#include "EVE.h"
#include "EVE_colors.h"

//*****************************************************************************
//
// The control table used by the uDMA controller.  This table must be aligned
// to a 1024 byte boundary.
//
//*****************************************************************************
#if defined(ewarm)
#pragma data_alignment=1024
uint8_t pui8ControlTable[1024];
#elif defined(ccs)
#pragma DATA_ALIGN(pui8ControlTable, 1024)
uint8_t pui8ControlTable[1024];
#else
uint8_t pui8ControlTable[1024] __attribute__ ((aligned(1024)));
#endif

//*****************************************************************************
//
// Use the following to specify the GPIO pins used by the SDRAM EPI bus.
//
//*****************************************************************************
#define EPI_PORTA_PINS (GPIO_PIN_7 | GPIO_PIN_6)
#define EPI_PORTB_PINS (GPIO_PIN_3 | GPIO_PIN_2)
#define EPI_PORTC_PINS (GPIO_PIN_7 | GPIO_PIN_6 | GPIO_PIN_5 | GPIO_PIN_4)
#define EPI_PORTG_PINS (GPIO_PIN_1 | GPIO_PIN_0)
#define EPI_PORTH_PINS (GPIO_PIN_3 | GPIO_PIN_2 | GPIO_PIN_1 | GPIO_PIN_0)
#define EPI_PORTK_PINS (GPIO_PIN_7 | GPIO_PIN_6 | GPIO_PIN_5 | GPIO_PIN_4)
#define EPI_PORTL_PINS (GPIO_PIN_5 | GPIO_PIN_4 | GPIO_PIN_3 | GPIO_PIN_2 | GPIO_PIN_1 | GPIO_PIN_0)
#define EPI_PORTM_PINS (GPIO_PIN_3 | GPIO_PIN_2 | GPIO_PIN_1 | GPIO_PIN_0)
#define EPI_PORTN_PINS (GPIO_PIN_5 | GPIO_PIN_4)
#define EPI_PORTP_PINS (GPIO_PIN_3 | GPIO_PIN_2)
#define EPI_PORTQ_PINS (GPIO_PIN_3 | GPIO_PIN_2 | GPIO_PIN_1 | GPIO_PIN_0)

//*****************************************************************************
//
// Use the following to specify the SDRAM location where the application will
// be downloaded from the SDCARD
//
//*****************************************************************************
#define SDRAM_APP_START_ADDRESS 0x60000000
#define SDRAM_APP_END_ADDRESS   0x61FFFFFF

uint32_t g_ui32SysClock;

uint32_t g_ui32EveColors[] = {EVE_DARK_BLUE, EVE_GREEN, EVE_RED, EVE_YELLOW, EVE_PINK, EVE_DEEP_PURPLE, EVE_THEAL, EVE_MAUVE};

const uint8_t g_ui8EveColorNum = sizeof(g_ui32EveColors) / sizeof(uint32_t); 

#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif

bool SDRAM_Test(void)
{
    uint32_t initialSDRAMAddress = 0;
    uint32_t *pui32SDRAM = (uint32_t*) (SDRAM_APP_START_ADDRESS
            + initialSDRAMAddress);
    uint32_t ui32ErrorCount = 0;
    const uint32_t ui32TestSize = 0x04000000 / sizeof(uint32_t);    //2e7;
    uint32_t i = 0;

    // Setup data
    #define TRANSFER_SIZE   256 // words

    UARTprintf("Starting SDRAM Access Test...\n");
    //UARTprintf("Initial address: %u MB\n", initialSDRAMAddress / 1000000);

    // Malloc test
    uint32_t *ptr;
    uint32_t n = 1000000;
    ptr = (uint32_t*) malloc(sizeof(uint32_t) * n);

    uint32_t j = 0;
    for (j = 0; j < n; j++)
    {
        ptr[j] = j;
    }

    // Verification
    for (i = 0; i < n; i++)
    {
        if (ptr[i] != i)
        {
            //UARTprintf("Correct! Malloc verification in address:\n");
            break;
        }
        // else
        // {
        //     UARTprintf("Error! Data corrupted in malloc:\n");
        // }
    }
    if(i != j)
        UARTprintf("Corrupted data in malloc verification! \n");
    else
        UARTprintf("Correct data verification in malloc! \n");

    free(ptr);
    UARTprintf("Ended malloc test\n");

    for (i = 0; i < ui32TestSize; i++)
    {
        pui32SDRAM[i] = (uint32_t) (SDRAM_APP_START_ADDRESS + i);
    }

    // --- Read & Verify ---
    for (i = 0; i < ui32TestSize; i++)
    {
        uint32_t ui32Read = pui32SDRAM[i];
        uint32_t ui32Expected = (uint32_t) (SDRAM_APP_START_ADDRESS + i);

        if (ui32Read != ui32Expected)
        {
            ui32ErrorCount++;
            if (ui32ErrorCount < 5)
            { // Print first few errors only
                UARTprintf("Error at offset\n");
            }
        }
    }
    if (ui32ErrorCount == 0)
    {
        UARTprintf("SDRAM Test PASSED. Memory accessible.\n");
    }
    else
    {
        UARTprintf("SDRAM Test FAILED.\n");

        return false;
    }


    return true;
}

//*****************************************************************************
//
// Configure the EPI and its pins.  This must be called before boot command is
// invoked.
//
//*****************************************************************************
int
ConfigureEPI(void)
{
    //
    // The EPI0 peripheral must be enabled for use.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_EPI0);

    //
    // For this example EPI0 is used with multiple pins on PortA, B, C, G, H,
    // K, L, M and N.  The actual port and pins used may be different on your
    // part, consult the data sheet for more information.
    // TODO: Update based upon the EPI pin assignment on your target part.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOH);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOL);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOM);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOP);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOQ);

    //
    // This step configures the internal pin muxes to set the EPI pins for use
    // with EPI.  Please refer to the datasheet for more information about pin
    // muxing.  Note that EPI0S27:20 are not used for the EPI SDRAM
    // implementation.
    //
    GPIOPinConfigure(GPIO_PH0_EPI0S0);
    GPIOPinConfigure(GPIO_PH1_EPI0S1);
    GPIOPinConfigure(GPIO_PH2_EPI0S2);
    GPIOPinConfigure(GPIO_PH3_EPI0S3);
    GPIOPinConfigure(GPIO_PC7_EPI0S4);
    GPIOPinConfigure(GPIO_PC6_EPI0S5);
    GPIOPinConfigure(GPIO_PC5_EPI0S6);
    GPIOPinConfigure(GPIO_PC4_EPI0S7);
    GPIOPinConfigure(GPIO_PA6_EPI0S8);
    GPIOPinConfigure(GPIO_PA7_EPI0S9);
    GPIOPinConfigure(GPIO_PG1_EPI0S10);
    GPIOPinConfigure(GPIO_PG0_EPI0S11);
    GPIOPinConfigure(GPIO_PM3_EPI0S12);
    GPIOPinConfigure(GPIO_PM2_EPI0S13);
    GPIOPinConfigure(GPIO_PM1_EPI0S14);
    GPIOPinConfigure(GPIO_PM0_EPI0S15);
    GPIOPinConfigure(GPIO_PL0_EPI0S16);
    GPIOPinConfigure(GPIO_PL1_EPI0S17);
    GPIOPinConfigure(GPIO_PL2_EPI0S18);
    GPIOPinConfigure(GPIO_PL3_EPI0S19);
    GPIOPinConfigure(GPIO_PQ0_EPI0S20);
    GPIOPinConfigure(GPIO_PQ1_EPI0S21);
    GPIOPinConfigure(GPIO_PQ2_EPI0S22);
    GPIOPinConfigure(GPIO_PQ3_EPI0S23);
    GPIOPinConfigure(GPIO_PK7_EPI0S24);
    GPIOPinConfigure(GPIO_PK6_EPI0S25);
    GPIOPinConfigure(GPIO_PL4_EPI0S26);
    GPIOPinConfigure(GPIO_PB2_EPI0S27);
    GPIOPinConfigure(GPIO_PB3_EPI0S28);
    GPIOPinConfigure(GPIO_PP2_EPI0S29);
    GPIOPinConfigure(GPIO_PP3_EPI0S30);
    GPIOPinConfigure(GPIO_PK5_EPI0S31);
    GPIOPinConfigure(GPIO_PK4_EPI0S32);
    GPIOPinConfigure(GPIO_PL5_EPI0S33);
    GPIOPinConfigure(GPIO_PN4_EPI0S34);
    GPIOPinConfigure(GPIO_PN5_EPI0S35);

    //
    // Configure the GPIO pins for EPI mode.  All the EPI pins require 8mA
    // drive strength in push-pull operation.  This step also gives control of
    // pins to the EPI module.
    //
    GPIOPinTypeEPI(GPIO_PORTA_BASE, EPI_PORTA_PINS);
    GPIOPinTypeEPI(GPIO_PORTB_BASE, EPI_PORTB_PINS);
    GPIOPinTypeEPI(GPIO_PORTC_BASE, EPI_PORTC_PINS);
    GPIOPinTypeEPI(GPIO_PORTG_BASE, EPI_PORTG_PINS);
    GPIOPinTypeEPI(GPIO_PORTH_BASE, EPI_PORTH_PINS);
    GPIOPinTypeEPI(GPIO_PORTK_BASE, EPI_PORTK_PINS);
    GPIOPinTypeEPI(GPIO_PORTL_BASE, EPI_PORTL_PINS);
    GPIOPinTypeEPI(GPIO_PORTM_BASE, EPI_PORTM_PINS);
    GPIOPinTypeEPI(GPIO_PORTN_BASE, EPI_PORTN_PINS);
    GPIOPinTypeEPI(GPIO_PORTP_BASE, EPI_PORTP_PINS);
    GPIOPinTypeEPI(GPIO_PORTQ_BASE, EPI_PORTQ_PINS);

    //
    // Set the EPI clock to half the system clock.
    //
    EPIDividerSet(EPI0_BASE, 1);

    //
    // Sets the usage mode of the EPI module.  For this example we will use
    // the SDRAM mode to talk to the external 64MB SDRAM daughter card.
    //
    EPIModeSet(EPI0_BASE, EPI_MODE_SDRAM);

    //
    // Configure the SDRAM mode.  We configure the SDRAM according to our core
    // clock frequency.  We will use the normal (or full power) operating
    // state which means we will not use the low power self-refresh state.
    // Set the SDRAM size to 64MB with a refresh interval of 468 clock ticks.
    //
    EPIConfigSDRAMSet(EPI0_BASE, (EPI_SDRAM_CORE_FREQ_50_100 | EPI_SDRAM_FULL_POWER |
                      EPI_SDRAM_SIZE_512MBIT), 468);

    //
    // Set the address map.  The EPI0 is mapped from 0x60000000 to 0x01FFFFFF.
    // For this example, we will start from a base address of 0x60000000 with
    // a size of 256MB.  Although our SDRAM is only 64MB, there is no 64MB
    // aperture option so we pick the next larger size.
    //
    EPIAddressMapSet(EPI0_BASE, EPI_ADDR_RAM_SIZE_256MB | EPI_ADDR_RAM_BASE_6);

    //
    // Wait for the SDRAM wake-up to complete by polling the SDRAM
    // initialization sequence bit.  This bit is true when the SDRAM interface
    // is going through the initialization and false when the SDRAM interface
    // it is not in a wake-up period.
    //
    while(HWREG(EPI0_BASE + EPI_O_STAT) &  EPI_STAT_INITSEQ)
    {
    }

    //
    // Write to the first 2 and last 2 address of the SDRAM card.  Since the
    // SDRAM card is word addressable, we will write words.
    //
    HWREGH(SDRAM_APP_START_ADDRESS) = 0xabcd;
    HWREGH(SDRAM_APP_START_ADDRESS + 0x2) = 0x1234;
    HWREGH(SDRAM_APP_END_ADDRESS - 0x3) = 0xdcba;
    HWREGH(SDRAM_APP_END_ADDRESS - 0x1) = 0x4321;

    if((HWREGH(SDRAM_APP_START_ADDRESS) == 0xabcd) &&
       (HWREGH(SDRAM_APP_START_ADDRESS + 0x2) == 0x1234) &&
       (HWREGH(SDRAM_APP_END_ADDRESS - 0x3) == 0xdcba) &&
       (HWREGH(SDRAM_APP_END_ADDRESS - 0x1) == 0x4321))
    {
        //
        // Read and write operations were successful.  Return with no errors.
        //
        return(1);
    }
    else
    {
        //
        // Read and write operations were failure.  Return with error.
        //
        return(0);
    }

}

int _system_pre_init(void)
{
    uint16_t ui32SysClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                             SYSCTL_OSC_MAIN |
                                             SYSCTL_USE_PLL |
                                             SYSCTL_CFG_VCO_240), 120000000);
    ConfigureEPI();
    // printf("Values");
    return 1;
}

void
ConfigureUART(void)
{
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

    MAP_GPIOPinConfigure(GPIO_PA0_U0RX);
    MAP_GPIOPinConfigure(GPIO_PA1_U0TX);
    MAP_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    UARTStdioConfig(0, 115200, g_ui32SysClock);
}


int
main(void)
{
    g_ui32SysClock = 120000000;
    PinoutSet(false, false);


    ConfigureUART();

    UARTprintf("Starting application... \n");
    //
    // Test if the SDRAM is properly controlled
    //
    UARTprintf("Checking EPI SDRAM state \n");
    uint32_t *pCheck = (uint32_t*) 0x60000000;
    *pCheck = 0xDEADBEEF;
    if (*pCheck == 0xDEADBEEF)
    {
        // Safe to use malloc now
        UARTprintf("SDRAM verification correct!\n");
    }
    //SDRAM_Test();
    UARTprintf("Setting up Screen SPI... \n");
    init_SPI_screen();
    UARTprintf("SPI set up successfully! \n");
    UARTprintf("Clearing the screen... \n");
    SysCtlDelay(MS_2_CLK(500));
    UARTprintf("Awaking screen... \n");
    API_WakeUpScreen();
    UARTprintf("Screen is awake! \n");

    gfx_start(EVE_GREY);
    API_CMD_ROTATE(0);
    gfx_end();
    EVE_MemWrite8(REG_PWM_DUTY, 128);

    while(1)
    {
        uint32_t ui32BgColor = g_ui32EveColors[rand() % (g_ui8EveColorNum)];
        gfx_start(ui32BgColor);
        API_CMD_ROTATE(0);
        gfx_end();

        UARTprintf("Running...\n");
        SysCtlDelay(MS_2_CLK(1000));

    }
}
