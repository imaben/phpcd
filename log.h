#ifndef __LOG_H__
#define __LOG_H__

#define PHPCD_LOG_DEBUG   0
#define PHPCD_LOG_NOTICE  1
#define PHPCD_LOG_WARN   2
#define PHPCD_LOG_ERROR   3

int phpcd_init_log(char *file, int mark);
void phpcd_log(int level, char *fmt, ...);
void phpcd_destroy_log();
FILE *phpcd_log_get_fp();

#define LOG_DEBUG(fmt, ...) \
    phpcd_log(PHPCD_LOG_DEBUG, fmt, ##__VA_ARGS__)

#define LOG_NOTICE(fmt, ...) \
    phpcd_log(PHPCD_LOG_NOTICE, fmt, ##__VA_ARGS__)

#define LOG_WARN(fmt, ...) \
    phpcd_log(PHPCD_LOG_WARN, fmt, ##__VA_ARGS__)

#define LOG_ERROR(fmt, ...) \
    phpcd_log(PHPCD_LOG_ERROR, fmt, ##__VA_ARGS__)
#endif
