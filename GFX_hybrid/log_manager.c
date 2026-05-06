
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "helpers.h"
#include "file_manager.h"
#include "rtc_module.h"

#include "log_manager.h"

#define MAX_LOGS_RUNNING	5
#define MAX_PATH_LENGTH		40

typedef struct {
	char logPath[MAX_PATH_LENGTH];
	uint8_t drive;
	uint8_t id;
	uint8_t itemCount;
	bool isOpen;
} log_entry_s;

static log_entry_s g_logEntries[MAX_LOGS_RUNNING] = {0};

static const char *DEFAULT_FOLDER = "logs";
static const char *TAG = "logManager";

// Default folder structure of the logs: x:/logs/DD-MM-YY/hh-mm-ss.csv
int LM_startLog(const uint8_t drive, const char *name, char **pcHeader, uint8_t size) {
	if(pcHeader == NULL || name == NULL) {
		TIVA_LOGE(TAG, "Error: Header or name is null");
		return -1;
	}

	if(size == 0) {
		TIVA_LOGE(TAG, "Error: Size of the header is zero");
		return -1;
	}

	// Check if the log is already created
	uint8_t i = 0;
	for(; i < MAX_LOGS_RUNNING; i++) {
		if(strcmp(name, g_logEntries[i].logPath) == 0) {
			return g_logEntries[i].id;
		}
	}

	// Select a free entry
	log_entry_s *log = NULL;
	for(i = 0; i < MAX_LOGS_RUNNING; i++) {
		if(!g_logEntries[i].isOpen) {
			log = &g_logEntries[i];
			break;
		}
	}

	if(log == NULL) return -1;

	if(i == MAX_LOGS_RUNNING - 1) {
		TIVA_LOGE(TAG, "ERROR: No log space left");
		return -1;
	}

	char dateStr[12];
	char timeStr[12];

	RTC_getFormattedTime(timeStr, sizeof(timeStr));
	RTC_getFormattedDate(dateStr, sizeof(dateStr));
	
	// snprintf(log->logPath, MAX_PATH_LENGTH, "%s/%s/%s.%s", DEFAULT_FOLDER, dateStr, timeStr, ".csv");
	snprintf(log->logPath, MAX_PATH_LENGTH, "%s.%s", name, "csv");

	log->itemCount = size;
	log->drive = drive;
	
	TIVA_LOGI(TAG, "Starting log: %s", log->logPath);

	char header[100] = {0};

	for(i = 0; i < size; i++) {
		if(pcHeader[i] != NULL) {
			if(i != 0) 
				snprintf(header, sizeof(header), "%s,%s", header, pcHeader[i]);
			else
				strcpy(header, pcHeader[i]);
				// snprintf(header, sizeof(header), "%s", header, pcHeader[i]);
		}
	}

	strcat(header, "\n");
	
	bool ret = FM_WriteFile(drive, log->logPath, ( uint8_t *)header, strlen(header));
	if(!ret) {
		TIVA_LOGE(TAG, "Error: The writing of the file failed!");
		return false;
	} 

	TIVA_LOGI(TAG, "Header written successfully!");
	log->isOpen = true;
	return log->id;
}

bool LM_addLine2Log(const uint8_t logId, const char* line) {
	// Find the log specified
	uint8_t i = 0;
	log_entry_s *log = NULL;

	for(; i < MAX_LOGS_RUNNING; i++) {
		log = &g_logEntries[i];
		if(log->id == logId) break;
	}

	if(i == MAX_LOGS_RUNNING) {
		TIVA_LOGE(TAG, "Error: No LogId match!");
		return false;
	}
	
	bool ret = FM_AppendLog(log->drive, log->logPath, line);
	if(!ret) {
		TIVA_LOGE(TAG, "Failed to append line to log");
		return false;
	}

	TIVA_LOGI(TAG, "Line appended to log successfully");
	return true;
}



