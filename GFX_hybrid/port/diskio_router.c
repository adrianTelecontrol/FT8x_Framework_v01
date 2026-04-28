// diskio_router.c
#include "fatfs/src/diskio.h"
#include "fatfs/src/ff.h"

// Definimos nuestras unidades (Drive Numbers)
#define DRIVE_SD  0
#define DRIVE_USB 1

// Declaramos las funciones que acabamos de renombrar en los otros archivos
extern DSTATUS SD_disk_initialize (BYTE pdrv);
extern DSTATUS SD_disk_status (BYTE pdrv);
extern DRESULT SD_disk_read (BYTE pdrv, BYTE* buff, LBA_t sector, UINT count);
extern DRESULT SD_disk_write (BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count);
extern DRESULT SD_disk_ioctl (BYTE pdrv, BYTE cmd, void* buff);

extern DSTATUS USB_disk_initialize (BYTE pdrv);
extern DSTATUS USB_disk_status (BYTE pdrv);
extern DRESULT USB_disk_read (BYTE pdrv, BYTE* buff, LBA_t sector, UINT count);
extern DRESULT USB_disk_write (BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count);
extern DRESULT USB_disk_ioctl (BYTE pdrv, BYTE cmd, void* buff);


// =========================================================================
// IMPLEMENTACIÓN ESTÁNDAR DE FATFS (El Router)
// =========================================================================

DSTATUS disk_status (BYTE pdrv) {
    switch (pdrv) {
        case DRIVE_SD:  return SD_disk_status(pdrv);
        case DRIVE_USB: return USB_disk_status(pdrv);
    }
    return STA_NOINIT;
}

DSTATUS disk_initialize (BYTE pdrv) {
    switch (pdrv) {
        case DRIVE_SD:  return SD_disk_initialize(pdrv);
        case DRIVE_USB: return USB_disk_initialize(pdrv);
    }
    return STA_NOINIT;
}

DRESULT disk_read (BYTE pdrv, BYTE* buff, LBA_t sector, UINT count) {
    switch (pdrv) {
        case DRIVE_SD:  return SD_disk_read(pdrv, buff, sector, count);
        case DRIVE_USB: return USB_disk_read(pdrv, buff, sector, count);
    }
    return RES_PARERR;
}

DRESULT disk_write (BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count) {
    switch (pdrv) {
        case DRIVE_SD:  return SD_disk_write(pdrv, buff, sector, count);
        case DRIVE_USB: return USB_disk_write(pdrv, buff, sector, count);
    }
    return RES_PARERR;
}

DRESULT disk_ioctl (BYTE pdrv, BYTE cmd, void* buff) {
    switch (pdrv) {
        case DRIVE_SD:  return SD_disk_ioctl(pdrv, cmd, buff);
        case DRIVE_USB: return USB_disk_ioctl(pdrv, cmd, buff);
    }
    return RES_PARERR;
}