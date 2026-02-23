/**
 *
 *
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <driverlib/sysctl.h>
#include <driverlib/systick.h>

#include "inc/hw_memmap.h"

#include <fatfs/src/diskio.h>
#include <fatfs/src/ff.h>

#include "bitmap_parser.h"
#include "helpers.h"

#include "sdspi_hal.h"

extern const uint32_t g_ui32SysClock;

#define PATH_BUF_SIZE 100
#define CMD_BUF_SIZE 64

static const char TASK_NAME[] = "SDSPI_TASK";

static char g_pcCwdBuf[PATH_BUF_SIZE] = "/";
static char g_pcTmpBuf[PATH_BUF_SIZE];

static FATFS g_sFatFs;
static DIR g_sDirObject;
static FILINFO g_sFileInfo;
static FIL g_sFileObject;

//
// A structure that holds a mapping between a FRESULT numerical code
// and a string representation
//
typedef struct {
  FRESULT iFResult;
  char *pcResultStr;
} tFResultString;

#define FRESULT_ENTRY(f) {(f), (#f)}

tFResultString g_psFResultStrings[] = {
    FRESULT_ENTRY(FR_OK),
    FRESULT_ENTRY(FR_DISK_ERR),
    FRESULT_ENTRY(FR_INT_ERR),
    FRESULT_ENTRY(FR_NOT_READY),
    FRESULT_ENTRY(FR_NO_FILE),
    FRESULT_ENTRY(FR_NO_PATH),
    FRESULT_ENTRY(FR_INVALID_NAME),
    FRESULT_ENTRY(FR_DENIED),
    FRESULT_ENTRY(FR_EXIST),
    FRESULT_ENTRY(FR_INVALID_OBJECT),
    FRESULT_ENTRY(FR_WRITE_PROTECTED),
    FRESULT_ENTRY(FR_INVALID_DRIVE),
    FRESULT_ENTRY(FR_NOT_ENABLED),
    FRESULT_ENTRY(FR_NO_FILESYSTEM),
    FRESULT_ENTRY(FR_MKFS_ABORTED),
    FRESULT_ENTRY(FR_TIMEOUT),
    FRESULT_ENTRY(FR_LOCKED),
    FRESULT_ENTRY(FR_NOT_ENOUGH_CORE),
    FRESULT_ENTRY(FR_TOO_MANY_OPEN_FILES),
    FRESULT_ENTRY(FR_INVALID_PARAMETER),
};

#define NUM_RESULT_CODES (sizeof(g_psFResultStrings) / sizeof(tFResultString))

const char *SDSPI_StringFromFResult(FRESULT iFResult) {
  uint_fast8_t ui8Index;

  for (ui8Index = 0; ui8Index < NUM_RESULT_CODES; ui8Index++) {
    if (g_psFResultStrings[ui8Index].iFResult == iFResult)
      return (g_psFResultStrings[ui8Index].pcResultStr);
  }

  return ("Unknown result code");
}

void SysTickHandler(void) {
  // Call the FatFs tick timer
  disk_timerproc();
}

int SDSPI_ls(int argc, char *argv[]) {
  uint32_t ui32TotalSize;
  uint32_t ui32FileCount;
  uint32_t ui32DirCount;
  FRESULT iFResult;
  FATFS *psFatFs;
  char *pcFileName;
#if _USE_LFN
  char pucLfn[_MAX_LFN + 1];
  g_sFileInfo.lfname = pucLfn;
  g_sFileInfo.lfsize = sizeof(pucLfn);
#endif

  // Open the current directory for access.
  iFResult = f_opendir(&g_sDirObject, g_pcCwdBuf);

  // Check for error and return if there is a problem.
  if (iFResult != FR_OK) {
    TIVA_LOGE(TASK_NAME, "Error opening dir: %s",
              SDSPI_StringFromFResult(iFResult));
    return ((int)iFResult);
  }

  ui32TotalSize = 0;
  ui32FileCount = 0;
  ui32DirCount = 0;

  TIVA_LOGI(TASK_NAME, "Contents of current directory");

  for (;;) {
    // Read an entry from the directory.
    iFResult = f_readdir(&g_sDirObject, &g_sFileInfo);

    // Check for error and return if there is a problem.
    if (iFResult != FR_OK) {
      return ((int)iFResult);
    }

    // If the file name is blank, then this is the end of the listing.
    if (!g_sFileInfo.fname[0]) {
      break;
    }

    // If the attribue is directory, then increment the directory count.
    if (g_sFileInfo.fattrib & AM_DIR) {
      ui32DirCount++;
    }

    // Otherwise, it is a file.  Increment the file count, and add in the
    // file size to the total.
    else {
      ui32FileCount++;
      ui32TotalSize += g_sFileInfo.fsize;
    }

#if _USE_LFN
    pcFileName =
        ((*g_sFileInfo.lfname) ? g_sFileInfo.lfname : g_sFileInfo.fname);
#else
    pcFileName = g_sFileInfo.fname;
#endif
    // Print the entry information on a single line with formatting to show
    // the attributes, date, time, size, and name.
    TIVA_LOGI(TASK_NAME, "%c%c%c%c%c %u/%02u/%02u %02u:%02u %9u  %s",
              (g_sFileInfo.fattrib & AM_DIR) ? 'D' : '-',
              (g_sFileInfo.fattrib & AM_RDO) ? 'R' : '-',
              (g_sFileInfo.fattrib & AM_HID) ? 'H' : '-',
              (g_sFileInfo.fattrib & AM_SYS) ? 'S' : '-',
              (g_sFileInfo.fattrib & AM_ARC) ? 'A' : '-',
              (g_sFileInfo.fdate >> 9) + 1980, (g_sFileInfo.fdate >> 5) & 15,
              g_sFileInfo.fdate & 31, (g_sFileInfo.ftime >> 11),
              (g_sFileInfo.ftime >> 5) & 63, g_sFileInfo.fsize, pcFileName);
  }

  // Print summary lines showing the file, dir, and size totals.
  TIVA_LOGI(TASK_NAME, "%4u File(s),%10u bytes total \t\t%4u Dir(s)",
            ui32FileCount, ui32TotalSize, ui32DirCount);

  // Get the free space.
  iFResult = f_getfree("/", (DWORD *)&ui32TotalSize, &psFatFs);

  // Check for error and return if there is a problem.
  if (iFResult != FR_OK) {
    return ((int)iFResult);
  }

  TIVA_LOGI(TASK_NAME, ", %10uK bytes free",
            (ui32TotalSize * psFatFs->free_clust / 2));

  return (0);
}

int SDSPI_cd(int argc, char *argv[]) {
  uint_fast8_t ui8Idx;
  FRESULT iFResult;

  // Copy the current working path into a temporary buffer so it can be
  // manipulated.
  strcpy(g_pcTmpBuf, g_pcCwdBuf);

  // If the first character is /, then this is a fully specified path, and it
  // should just be used as-is.
  if (argv[1][0] == '/') {
    // Make sure the new path is not bigger than the cwd buffer.
    if (strlen(argv[1]) + 1 > sizeof(g_pcCwdBuf)) {
      TIVA_LOGE(TASK_NAME, "Resulting path name is too long\n");
      return (0);
    }

    // If the new path name (in argv[1])  is not too long, then copy it
    // into the temporary buffer so it can be checked.
    else {
      strncpy(g_pcTmpBuf, argv[1], sizeof(g_pcTmpBuf));
    }
  }

  // If the argument is .. then attempt to remove the lowest level on the
  // CWD.
  else if (!strcmp(argv[1], "..")) {
    // Get the index to the last character in the current path.
    ui8Idx = strlen(g_pcTmpBuf) - 1;

    // Back up from the end of the path name until a separator (/) is
    // found, or until we bump up to the start of the path.
    while ((g_pcTmpBuf[ui8Idx] != '/') && (ui8Idx > 1)) {
      //
      // Back up one character.
      //
      ui8Idx--;
    }

    // Now we are either at the lowest level separator in the current path,
    // or at the beginning of the string (root).  So set the new end of
    // string here, effectively removing that last part of the path.
    g_pcTmpBuf[ui8Idx] = 0;
  }

  // Otherwise this is just a normal path name from the current directory,
  // and it needs to be appended to the current path.
  else {
    // Test to make sure that when the new additional path is added on to
    // the current path, there is room in the buffer for the full new path.
    // It needs to include a new separator, and a trailing null character.
    if (strlen(g_pcTmpBuf) + strlen(argv[1]) + 1 + 1 > sizeof(g_pcCwdBuf)) {
      TIVA_LOGE(TASK_NAME, "Resulting path name is too long\n");
      return (0);
    }

    // The new path is okay, so add the separator and then append the new
    // directory to the path.
    else {
      // If not already at the root level, then append a /
      if (strcmp(g_pcTmpBuf, "/")) {
        strcat(g_pcTmpBuf, "/");
      }

      // Append the new directory to the path.
      strcat(g_pcTmpBuf, argv[1]);
    }
  }

  // At this point, a candidate new directory path is in chTmpBuf.  Try to
  // open it to make sure it is valid.
  iFResult = f_opendir(&g_sDirObject, g_pcTmpBuf);

  // If it can't be opened, then it is a bad path.  Inform the user and
  // return.
  if (iFResult != FR_OK) {
    TIVA_LOGE(TASK_NAME, "cd: %s\n", g_pcTmpBuf);
    return ((int)iFResult);
  }

  // Otherwise, it is a valid new path, so copy it into the CWD.
  else {
    strncpy(g_pcCwdBuf, g_pcTmpBuf, sizeof(g_pcCwdBuf));
  }

  return (0);
}

int SDSPI_pwd(int argc, char *argv[]) {
  TIVA_LOGI(TASK_NAME, "Current directory: %s", g_pcCwdBuf);

  return (0);
}

int SDSPI_cat(int argc, char *argv[]) {
  FRESULT iFResult;
  uint32_t ui32BytesRead;

  if (strlen(g_pcCwdBuf) + strlen(argv[1]) + 1 + 1 > sizeof(g_pcTmpBuf)) {
    TIVA_LOGE(TASK_NAME, "Resulting path name is too long");
    return (0);
  }

  strcpy(g_pcTmpBuf, g_pcCwdBuf);

  // If not already at the root level, then append a separator.
  if (strcmp("/", g_pcCwdBuf)) {
    strcat(g_pcTmpBuf, "/");
  }

  // Now finally, append the file name to result in a fully specified file.
  strcat(g_pcTmpBuf, argv[1]);

  // Open the file for reading.
  iFResult = f_open(&g_sFileObject, g_pcTmpBuf, FA_READ);

  // If there was some problem opening the file, then return an error.
  if (iFResult != FR_OK) {
    return ((int)iFResult);
  }

  do {
    // Read a block of data from the file.  Read as much as can fit in the
    // temporary buffer, including a space for the trailing null.
    iFResult = f_read(&g_sFileObject, g_pcTmpBuf, sizeof(g_pcTmpBuf) - 1,
                      (UINT *)&ui32BytesRead);

    // If there was an error reading, then print a newline and return the
    // error to the user.
    if (iFResult != FR_OK) {
      return ((int)iFResult);
    }

    // Null terminate the last block that was read to make it a null
    // terminated string that can be used with printf.
    g_pcTmpBuf[ui32BytesRead] = 0;

    TIVA_LOGI(TASK_NAME, "%s", g_pcTmpBuf);
  } while (ui32BytesRead == sizeof(g_pcTmpBuf) - 1);

  return (0);
}

int SDSPI_FetchFile(const char *pcFileName, uint8_t *pui32SDRAMBuff,
                    uint32_t ui32BuffSize) {
  FRESULT iFResult;
  uint32_t ui32BytesRead;
  uint32_t ui32Index;
  uint32_t ui32SDRAMMemIndex;

  // First, check to make sure that the current path (CWD), plus the file
  // name, plus a separator and trailing null, will all fit in the temporary
  // buffer that will be used to hold the file name.  The file name must be
  // fully specified, with path, to FatFs.
  if (strlen(g_pcCwdBuf) + strlen(pcFileName) + 1 + 1 > sizeof(g_pcTmpBuf)) {
    TIVA_LOGI(TASK_NAME, "Resulting path name is too long\n");
    return (0);
  }

  strcpy(g_pcTmpBuf, g_pcCwdBuf);

  if (strcmp("/", g_pcCwdBuf)) {
    strcat(g_pcTmpBuf, "/");
  }

  strcat(g_pcTmpBuf, pcFileName);

  TIVA_LOGI(TASK_NAME, "File to load: %s", g_pcTmpBuf);

  // Open the file for reading.
  iFResult = f_open(&g_sFileObject, g_pcTmpBuf, FA_READ);

  // If there was some problem opening the file, then return an error.
  if (iFResult != FR_OK) {
    return ((int)iFResult);
  }

  ui32SDRAMMemIndex = 0;

  do {
    // Read a block of data from the file.  Read as much as can fit in the
    // temporary buffer, including a space for the trailing null.
    iFResult = f_read(&g_sFileObject, g_pcTmpBuf, sizeof(g_pcTmpBuf),
                      (UINT *)&ui32BytesRead);

    // If there was an error reading, then print a newline and return the
    // error to the user.
    if (iFResult != FR_OK) {
      return ((int)iFResult);
    }

    // Copy the data to the SDRAM
    for (ui32Index = 0; ui32Index < ui32BytesRead; ui32Index++) {
      if (ui32SDRAMMemIndex >= ui32BuffSize) {

        TIVA_LOGE(TASK_NAME, "Error: Not enough space reserved in SDRAM");
        return ((int)FR_DISK_ERR);
      }
      pui32SDRAMBuff[ui32SDRAMMemIndex] = g_pcTmpBuf[ui32Index];
      ui32SDRAMMemIndex++;
    }
  } while (ui32BytesRead == sizeof(g_pcTmpBuf));

  TIVA_LOGI(TASK_NAME, "SDSPI file copy complete!");

  return (0);
}

int SDSPI_FetchBitmap(const char *pcFileName, BitmapHandler_t *psBitmapHandler,
                      const uint32_t ui32BuffSize) {
  FRESULT iFResult;
  uint32_t ui32BytesRead;
  uint32_t ui32Index;

  if (strlen(g_pcCwdBuf) + strlen(pcFileName) + 1 + 1 > sizeof(g_pcTmpBuf)) {
    TIVA_LOGE(TASK_NAME, "Resulting path name is too long\n");
    return (int)FR_INVALID_NAME;
  }

  strcpy(g_pcTmpBuf, g_pcCwdBuf);
  if (strcmp("/", g_pcCwdBuf)) {
    strcat(g_pcTmpBuf, "/");
  }
  strcat(g_pcTmpBuf, pcFileName);
  TIVA_LOGI(TASK_NAME, "File to load: %s", g_pcTmpBuf);

  iFResult = f_open(&g_sFileObject, g_pcTmpBuf, FA_READ);
  if (iFResult != FR_OK) {
    TIVA_LOGE(TASK_NAME, "Failed to open file. Error: %d", iFResult);
    return ((int)iFResult);
  }

  bitmap_Parser(&g_sFileObject, psBitmapHandler);
  printBitmapHeader(&psBitmapHandler->sHeader);

  uint32_t ui32BytesPerRow = psBitmapHandler->sHeader.bitmap_width * 2;

  uint32_t ui32FileStride =
      ui32BytesPerRow + psBitmapHandler->sHeader.padding_bytes;

  uint32_t ui32PackedSize =
      ui32BytesPerRow * psBitmapHandler->sHeader.bitmap_height;

  psBitmapHandler->ui8Pixels = (uint8_t *)malloc(ui32PackedSize);
  memset(psBitmapHandler->ui8Pixels, 0x00, ui32PackedSize);

  if (psBitmapHandler->ui8Pixels == NULL) {
    TIVA_LOGE(TASK_NAME, "Malloc Failed: Not enough SDRAM");
    f_close(&g_sFileObject);
    return FR_NOT_ENOUGH_CORE;
  }

  f_lseek(&g_sFileObject, psBitmapHandler->sHeader.data_offset);

  uint32_t ui32RowByteCounter = 0;
  uint32_t ui32CurrentSourceRow = 0;
  uint32_t ui32Height = psBitmapHandler->sHeader.bitmap_height;
  uint32_t ui32TotalBytesProcessed = 0;

  do {
    iFResult = f_read(&g_sFileObject, g_pcTmpBuf, sizeof(g_pcTmpBuf),
                      (UINT *)&ui32BytesRead);
    if (iFResult != FR_OK)
      break;

    for (ui32Index = 0; ui32Index < ui32BytesRead; ui32Index++) {

      if (ui32CurrentSourceRow >= ui32Height)
        break; // Finished all rows

      // A. Are we in the 'Data' part of the line? (Skip padding)
      if (ui32RowByteCounter < ui32BytesPerRow) {

        // B. Calculate Inverted Destination (Bottom-Up -> Top-Down)
        uint32_t ui32DestRow = (ui32Height - 1) - ui32CurrentSourceRow;

        // C. Calculate column position within the row
        uint32_t ui32DestIndex =
            (ui32DestRow * ui32BytesPerRow) + ui32RowByteCounter;

        if (ui32DestIndex < ui32PackedSize) {
          psBitmapHandler->ui8Pixels[ui32DestIndex] = g_pcTmpBuf[ui32Index];
          ui32TotalBytesProcessed++;
        }
      }

      ui32RowByteCounter++;

      if (ui32RowByteCounter >= ui32FileStride) {
        ui32RowByteCounter = 0; // Reset byte counter
        ui32CurrentSourceRow++; // Move to next row
      }
    }

    if (ui32CurrentSourceRow >= ui32Height)
      break; // Exit outer loop too

  } while (ui32BytesRead == sizeof(g_pcTmpBuf));

  f_close(&g_sFileObject);
  TIVA_LOGI(TASK_NAME,
            "Bitmap loaded to SDRAM successfully. Bytes processed: %d",
            ui32TotalBytesProcessed);

  f_close(&g_sFileObject);
  TIVA_LOGI(TASK_NAME, "Bitmap loaded to SDRAM successfully.");

  UINT bytesRead;
  f_open(&g_sFileObject, "DAT.txt", FA_WRITE | FA_CREATE_NEW);
  FRESULT res =
      f_write(&g_sFileObject, psBitmapHandler->ui8Pixels,
              psBitmapHandler->sHeader.image_size_in_bytes, &bytesRead);
  if (res == FR_OK)
    TIVA_LOGI(TASK_NAME, "File written successfully!");
  else {
    TIVA_LOGE(TASK_NAME, "Failed to write file");
  }
  f_close(&g_sFileObject);

  return 0;
}

bool SDSPI_MountFilesystem(void) {
  FRESULT iFResult;

  SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI1);

  iFResult = f_mount(0, &g_sFatFs);
  if (iFResult != FR_OK) {
    TIVA_LOGI(TASK_NAME, "Error mounting the SD drive: %s",
              SDSPI_StringFromFResult(iFResult));
    return false;
  }

  return true;
}
