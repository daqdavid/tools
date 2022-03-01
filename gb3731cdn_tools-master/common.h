#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <termios.h>
#include <unistd.h>
#include <getopt.h>
#include <stdarg.h>
#include <semaphore.h>
#include <assert.h>
#include <stdbool.h>
#include <errno.h>

#define _MACROSTR(x) #x
#define MACROSTR(x) _MACROSTR(x)

int print_version2();

char *safe_strncpy(char *dst, const char *src, size_t size);
void dbg_printf(const char *form, ...);

void pabort(const char *a1);

void printf4s(char *suff, unsigned char *data, int len);

int start_main(void *(void *), void *arg, unsigned int restart_msec);

int create_periodic_timer(time_t ii_sec, long ii_nsec, time_t iv_sec, long iv_nsec);

#endif
