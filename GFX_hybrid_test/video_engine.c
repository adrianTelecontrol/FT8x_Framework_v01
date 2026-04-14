#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>


#include "driverlib/ssi.h"
#include "inc/hw_memmap.h"

#include <fatfs/src/diskio.h>
#include <fatfs/src/ff.h>


#include "EVE.h"
#include "FT8xx.h"
#include "gfx.h"
#include "hal_spi.h"
#include "helpers.h"
#include "video_engine.h"

static const char *TASK_NAME = "video_engine";

// -----------------------------------------------------------------------------
// CONSTANTS & BUFFERS
// -----------------------------------------------------------------------------
#define SD_CHUNK_SIZE 4096

// Static buffer to save TM4C stack memory
static uint8_t fileBuffer[SD_CHUNK_SIZE];
static uint16_t cmdOffset = 0;

#define MEDIA_FIFO_ADDR 0x0F0000 
#define MEDIA_FIFO_SIZE (64 * 1024)

// -----------------------------------------------------------------------------
// HELPER: DMA BLOCK WRITER (Bypass Seguro)
// -----------------------------------------------------------------------------
void EVE_WriteMediaFIFOBlock(uint32_t destAddr, const uint8_t *pSrc, uint32_t length) {
    HAL_SPI_CS_Enable();
    
    // Formato estándar Single SPI (1-bit)
    EVE_AddrForWr(destAddr);
    
    // Bombear el payload byte por byte en Full-Duplex.
    // Usar ReadWrite8 garantiza que el TX y RX FIFO del TM4C se mantengan 100% sincronizados.
	uint32_t i = 0;
    for (; i < length; i++) {
        HAL_SPI_ReadWrite8(pSrc[i]);
    }
    
    HAL_SPI_CS_Disable();
}

// -----------------------------------------------------------------------------
// PREPARAR LIENZO Y LANZAR VIDEO
// -----------------------------------------------------------------------------
void EVE_PlayVideo_Start(void) {
    EVE_MemWrite8(REG_VOL_PB, 255);

    // PASO A: PREPARAR EL LIENZO EN NEGRO
    API_LIB_BeginCoProList();
    
    EVE_Write32(0xFFFFFF00); // CMD_DLSTART
    cmdOffset = EVE_IncCMDOffset(cmdOffset, 4);

    EVE_Write32(0x02000000); // CLEAR_COLOR_RGB (Negro)
    cmdOffset = EVE_IncCMDOffset(cmdOffset, 4);

    EVE_Write32(0x26000007); // CLEAR(1,1,1)
    cmdOffset = EVE_IncCMDOffset(cmdOffset, 4);

    EVE_Write32(0x00000000); // DISPLAY()
    cmdOffset = EVE_IncCMDOffset(cmdOffset, 4);

    EVE_Write32(0xFFFFFF01); // CMD_SWAP
    cmdOffset = EVE_IncCMDOffset(cmdOffset, 4);
    
    API_LIB_EndCoProList();
    API_LIB_AwaitCoProEmpty();

    // PASO B: ARRANCAR EL DECODIFICADOR
    API_LIB_BeginCoProList();
    
    EVE_Write32(0xFFFFFF3A); // CMD_PLAYVIDEO
    cmdOffset = EVE_IncCMDOffset(cmdOffset, 4);

    EVE_Write32(16);         // OPT_MEDIAFIFO (Sin Fullscreen)
    cmdOffset = EVE_IncCMDOffset(cmdOffset, 4);

    API_LIB_EndCoProList();
}

// -----------------------------------------------------------------------------
// FLUJO PRINCIPAL DE VIDEO
// -----------------------------------------------------------------------------
// Usaremos las direcciones físicas absolutas para evitar macros defectuosas de la librería
#define REG_CMD_READ_REAL        0x3020F8
#define REG_MEDIAFIFO_READ_REAL  0x309014
#define REG_MEDIAFIFO_WRITE_REAL 0x309018

void EVE_StreamVideoFile(FIL *videoFile) {
    uint32_t mediaReadPtr;
    uint32_t mediaWritePtr = 0;
    uint32_t freeSpace;
    UINT bytesRead;
    
    // =================================================================
    // 1. REGLA DE ORO: Limpiar el puntero ANTES de definir el FIFO
    // =================================================================
    EVE_MemWrite32(REG_MEDIAFIFO_WRITE_REAL, 0);

    // =================================================================
    // 2. INFORMAR AL COPROCESADOR SOBRE EL MEDIA FIFO
    // =================================================================
    API_LIB_BeginCoProList();
    EVE_Write32(0xFFFFFF39);      // CMD_MEDIAFIFO
    cmdOffset = EVE_IncCMDOffset(cmdOffset, 4);
    EVE_Write32(MEDIA_FIFO_ADDR); 
    cmdOffset = EVE_IncCMDOffset(cmdOffset, 4);
    EVE_Write32(MEDIA_FIFO_SIZE); 
    cmdOffset = EVE_IncCMDOffset(cmdOffset, 4);
    API_LIB_EndCoProList();
    API_LIB_AwaitCoProEmpty();

    // =================================================================
    // 3. PRE-FILL (Llenar RAM_G con los primeros 60KB)
    // =================================================================
    while (mediaWritePtr < (MEDIA_FIFO_SIZE - SD_CHUNK_SIZE)) {
        f_read(videoFile, fileBuffer, SD_CHUNK_SIZE, &bytesRead);
        if (bytesRead == 0) break; 

        uint32_t ramgWriteAddr = MEDIA_FIFO_ADDR + mediaWritePtr;
        EVE_WriteMediaFIFOBlock(ramgWriteAddr, fileBuffer, bytesRead);
        mediaWritePtr += bytesRead;
    }

    // Actualizar puntero y encender el motor de video
    EVE_MemWrite32(REG_MEDIAFIFO_WRITE_REAL, mediaWritePtr);
    
    // Asegúrate de que EVE_PlayVideo_Start() use OPT_MEDIAFIFO | OPT_FULLSCREEN (24)
    EVE_PlayVideo_Start();

    // =================================================================
    // 4. KIOSKO DE STREAMING Y TELEMETRÍA
    // =================================================================
    uint32_t telemetryCounter = 0;

    while (1) { 
        if (f_eof(videoFile)) {
            while(1) {} // Congelar al terminar
        }

        mediaReadPtr = EVE_MemRead32(REG_MEDIAFIFO_READ_REAL);

        // --- SISTEMA DE TELEMETRÍA ---
        if (telemetryCounter++ % 20 == 0) {
             uint32_t eveCmdRead = EVE_MemRead32(REG_CMD_READ_REAL);
             TIVA_LOGI("VIDEO", "CMD_RD: 0x%03X | FIFO_RD: %6u | FIFO_WR: %6u", 
                        eveCmdRead, mediaReadPtr, mediaWritePtr);
        }

        if (mediaWritePtr >= mediaReadPtr) {
            freeSpace = MEDIA_FIFO_SIZE - (mediaWritePtr - mediaReadPtr);
        } else {
            freeSpace = mediaReadPtr - mediaWritePtr;
        }
        
        freeSpace -= 4; 

        if (freeSpace >= SD_CHUNK_SIZE) {
            f_read(videoFile, fileBuffer, SD_CHUNK_SIZE, &bytesRead);
            
            if (bytesRead > 0) {
                uint32_t ramgWriteAddr = MEDIA_FIFO_ADDR + mediaWritePtr;
                EVE_WriteMediaFIFOBlock(ramgWriteAddr, fileBuffer, bytesRead);

                mediaWritePtr = (mediaWritePtr + bytesRead) % MEDIA_FIFO_SIZE;
                EVE_MemWrite32(REG_MEDIAFIFO_WRITE_REAL, mediaWritePtr);
            }
        }
    }
}

// -----------------------------------------------------------------------------
// ENTRY POINT (Limpiado)
// -----------------------------------------------------------------------------
void EVE_PlayIntroVideo(void) {
    FIL myVideoFile;

    if (f_open(&myVideoFile, "splash.avi", FA_READ) != FR_OK) {
        return; 
    }

    // Ya no llamamos funciones sueltas, StreamVideoFile controla todo el flujo
    EVE_StreamVideoFile(&myVideoFile);

    f_close(&myVideoFile);
}
