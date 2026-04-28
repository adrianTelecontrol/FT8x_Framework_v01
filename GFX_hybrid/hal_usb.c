#include "hal_usb.h"

// TivaWare e Includes de Hardware
#include <stdbool.h>
#include <stdint.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"

// TivaWare USB Library
#include "usblib/usblib.h"
#include "usblib/host/usbhost.h"
#include "usblib/host/usbhmsc.h"

// Integración con tu arquitectura
#include "event_engine.h" // Para disparar eventos a la GUI
#include "tiva_log.h"     // Asumiendo que usas esto para debugear

// --- MACROS Y VARIABLES GLOBALES DEL USB ---
#define HCD_MEMORY_SIZE 128
static uint8_t g_pHCDPool[HCD_MEMORY_SIZE]; // Memoria de trabajo del controlador

// Instancia global del Mass Storage Class (usada por fat_usbmsc.c)
tUSBHMSCInstance *g_psMSCInstance = NULL;

static volatile bool g_bUSBIsReady = false;
static const char *TAG = "HAL_USB";

// =====================================================================
// CALLBACKS DE EVENTOS USB
// =====================================================================

// Callback para eventos genéricos del Host (Errores de energía, etc.)
static void USBHCDEvents(void *pvData) {
    tEventInfo *pEventInfo = (tEventInfo *)pvData;

    switch (pEventInfo->ui32Event) {
        case USB_EVENT_POWER_FAULT:
            // TIVA_LOGE(TAG, "USB Power Fault detectado!");
            break;
        case USB_EVENT_UNKNOWN_CONNECTED:
            // TIVA_LOGW(TAG, "Dispositivo USB desconocido conectado.");
            break;
        default:
            break;
    }
}

// Callback específico para la clase de Almacenamiento Masivo (Pendrives)
static void MSCCallback(tUSBHMSCInstance *psMSCInstance, uint32_t ui32Event, void *pvEventData) {
    switch (ui32Event) {
        case USB_EVENT_CONNECTED:
            // El dispositivo fue enumerado y está listo
            g_bUSBIsReady = true;
            
            // Disparamos un evento para que la UI muestre un ícono de USB, por ejemplo
            // Event_Post(EVT_SYS_USB_CONNECTED, (EventParam_t){ .i32 = 1 });
            break;

        case USB_EVENT_DISCONNECTED:
            g_bUSBIsReady = false;
            
            // Avisar a la GUI para que cambie el ícono o aborte transferencias
            // Event_Post(EVT_SYS_USB_DISCONNECTED, (EventParam_t){ .i32 = 0 });
            break;
    }
}

// Registro de Drivers de Clase soportados por nuestro Host
DECLARE_EVENT_DRIVER(g_sUSBEventDriver, 0, 0, USBHCDEvents);
static tUSBHostClassDriver const * const g_ppHostClassDrivers[] = {
    &g_sUSBHostMSCClassDriver,
    &g_sUSBEventDriver
};

const uint32_t g_ui32NumHostClassDrivers = 
      sizeof(g_ppHostClassDrivers) / sizeof(tUSBHostClassDriver *);

// =====================================================================
// FUNCIONES PÚBLICAS (API)
// =====================================================================

bool HAL_USB_Init(void) {
    // 1. Habilitar periféricos necesarios
    SysCtlPeripheralEnable(SYSCTL_PERIPH_USB0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOL);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOQ);

    // Esperar a que los periféricos estén listos
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_USB0) || 
          !SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOQ));

    // 2. Configurar Pines Analógicos USB (D+, D-, ID, VBUS)
    // TM4C1294XL: PB0 = USB0ID, PB1 = USB0VBUS
    GPIOPinTypeUSBAnalog(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    // TM4C1294XL: PL6 = USB0DP, PL7 = USB0DM
    GPIOPinTypeUSBAnalog(GPIO_PORTL_BASE, GPIO_PIN_6 | GPIO_PIN_7);

    // 3. Configurar Pines Digitales de Control de Energía (EPEN, PFLT)
    // Para encender y monitorear el switch de 5V del USB Host
    GPIOPinConfigure(GPIO_PQ4_USB0EPEN);
    GPIOPinConfigure(GPIO_PQ3_USB0PFLT);
    GPIOPinTypeUSBDigital(GPIO_PORTQ_BASE, GPIO_PIN_3 | GPIO_PIN_4);

    // 4. Configurar el Reloj y Modo Host del USB
    // TivaC USB requiere reloj de PLL a 480 MHz o la frecuencia que estés usando
    USBStackModeSet(0, eUSBModeHost, 0);

    // 5. Inicializar el Host Controller Driver (HCD)
    // OJO: Esta función habilitará la interrupción INT_USB0 internamente
    USBHCDInit(0, g_pHCDPool, HCD_MEMORY_SIZE);

    // 6. Abrir la instancia del driver MSC (Mass Storage Class)
    g_psMSCInstance = USBHMSCDriveOpen(0, MSCCallback);

    if (g_psMSCInstance == NULL) {
        // TIVA_LOGE(TAG, "Error inicializando driver MSC USB");
        return false;
    }

    return true;
}

void HAL_USB_Task(void) {
    // Procesa la máquina de estados del hardware USB.
    // Esto detecta si se conectó/desconectó algo y lee descriptores.
    USBHCDMain();
}

bool HAL_USB_IsReady(void) {
    return g_bUSBIsReady;
}