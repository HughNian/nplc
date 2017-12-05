#ifndef LOG_H_
#define LOG_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

typedef struct myLog {
	uint32_t line;
	char *error_msg;
	FILE *log_file;
	char log_file_path[128];
	char log_file_name[32];
	char source_file_name[32];
} myLog;

myLog *init_log(const char *log_file_path, const char *log_file_name, const char *source_file_name);
int write_log(myLog *l, uint32_t line, char *error_msg, const char *format, ...);
void end_log(myLog *l);

#endif
