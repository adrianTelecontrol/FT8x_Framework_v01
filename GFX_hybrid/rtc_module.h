#ifndef RTC_MODULE_H_
#define RTC_MODULE_H_


typedef struct tm RTC_timeDate;

bool RTC_getFormattedDate(char *out_buffer, size_t max_len);

bool RTC_getFormattedTime(char *out_buffer, size_t max_len);

void initRTCModule(void);

#endif // RTC_MODULE_H_