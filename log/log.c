#include "log.h"

myLog *
init_log(const char *log_file_path, const char *log_file_name, const char *source_file_name)
{
	myLog *l;
	char logFile[152];

	l = (myLog *)malloc(sizeof(*l));
	if(NULL == l) return NULL;

	memset(l, 0, sizeof(*l));

	if(strlen(log_file_path) == 0 || strlen(log_file_name) == 0 || strlen(source_file_name) == 0) return NULL;
	if(strlen(log_file_path) > 128 || strlen(log_file_name) > 32 || strlen(source_file_name) > 32) return NULL;

	l->line = 0;
	strcpy(l->log_file_path, log_file_path);
	strcpy(l->log_file_name, log_file_name);
	strcpy(l->source_file_name, source_file_name);
	sprintf(logFile, "%s%s", log_file_path, log_file_name);
	l->log_file = fopen(logFile, "w+");

	return l;
}

int
write_log(myLog *l, uint32_t line, char *error_msg, const char *format, ...)
{
	va_list arg;
	int done;

	va_start(arg, format);

	l->line = line;
	l->error_msg = strdup(error_msg);

	time_t time_log = time(NULL);
	struct tm* tm_log = localtime(&time_log);
	fprintf(l->log_file, "log time is: %04d-%02d-%02d %02d:%02d:%02d \n", tm_log->tm_year + 1900, tm_log->tm_mon + 1, tm_log->tm_mday, tm_log->tm_hour, tm_log->tm_min, tm_log->tm_sec);
	fprintf(l->log_file, "source file name is: %s \n", l->source_file_name);
	fprintf(l->log_file, "source file line is: %d \n", l->line);
	fprintf(l->log_file, "error msg is: %s\n", l->error_msg);
	done = vfprintf (l->log_file, format, arg);
	if(done){
		fprintf(l->log_file, "end.\n\n");
	}

	va_end(arg);

	fflush(l->log_file);

	return done;
}

void
end_log(myLog *l)
{
	if(NULL == l) return;

	if(l->log_file != NULL) fclose(l->log_file);

	if(l != NULL){
		free(l->error_msg);
		free(l);
	}
}
