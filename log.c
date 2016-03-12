#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include "log.h"

static char *phpcd_error_titles[PHPCD_LOG_ERROR + 1] = {
    "DEBUG", "NOTICE", "WARN", "ERROR"
};

static FILE *phpcd_log_fp = NULL;
static int phpcd_log_mark = PHPCD_LOG_DEBUG;
static int phpcd_log_initialize = 0;
static char phpcd_log_buffer[4096];


int phpcd_init_log(char *file, int mark)
{
    if (phpcd_log_initialize) {
        return 0;
    }

    if (mark < PHPCD_LOG_DEBUG
        || mark > PHPCD_LOG_ERROR)
    {
        return -1;
    }

    if (file) {
        phpcd_log_fp = fopen(file, "a+");
        if (!phpcd_log_fp) {
            return -1;
        }

    } else {
        phpcd_log_fp = stderr;
    }

    phpcd_log_mark = mark;
    phpcd_log_initialize = 1;

    return 0;
}


void phpcd_log(int level, char *fmt, ...)
{
    va_list al;
    time_t current;
    struct tm *dt;
    int off1, off2;

    if (!phpcd_log_initialize
        || level < phpcd_log_mark
        || level > PHPCD_LOG_ERROR)
    {
        return;
    }

    /* Get current date and time */
    time(&current);
    dt = localtime(&current);

    off1 = sprintf(phpcd_log_buffer,
                   "[%04d-%02d-%02d %02d:%02d:%02d] %s: ",
                   dt->tm_year + 1900,
                   dt->tm_mon + 1,
                   dt->tm_mday,
                   dt->tm_hour,
                   dt->tm_min,
                   dt->tm_sec,
                   phpcd_error_titles[level]);

    va_start(al, fmt);
    off2 = vsprintf(phpcd_log_buffer + off1, fmt, al);
    va_end(al);

    phpcd_log_buffer[off1 + off2] = '\n';

    fwrite(phpcd_log_buffer, off1 + off2 + 1, 1, phpcd_log_fp);
}


void phpcd_destroy_log()
{
    if (!phpcd_log_initialize) {
        return;
    }

    if (phpcd_log_fp != stderr) {
        fclose(phpcd_log_fp);
    }
}

FILE *phpcd_log_get_fp()
{
    return phpcd_log_fp;
}
