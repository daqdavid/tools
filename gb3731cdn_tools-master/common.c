
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/syscall.h>
#include <sys/timerfd.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/socket.h>

#include <net/if.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include <linux/spi/spidev.h>

#include <arpa/inet.h>

#include <pthread.h>

#include "common.h"

int print_version2()
{
    printf("**************************************************************\n");
    printf("Git CommitLog: %s\n", MACROSTR(GIT_COMMIT_LOG));
    printf("Build Time: %s\n", MACROSTR(BUILD_TIME));
    printf("**************************************************************\n");

    return 0;
}

/* Like strncpy but make sure the resulting string is always 0 terminated. */
char *safe_strncpy(char *dst, const char *src, size_t size)
{
    if (!size)
        return dst;
    dst[--size] = '\0';
    return strncpy(dst, src, size);
}

pid_t gettid()
{
    return syscall(SYS_gettid);
}

void dbg_printf(const char *form, ...)
{
    va_list arg;
    struct timeval cur_time;
    struct tm tm;
    char prit_str[400];
    char tem_str[256];
    pid_t my_pid;
    pid_t my_tid;

    gettimeofday(&cur_time, NULL);
    gmtime_r(&cur_time.tv_sec, &tm);

    my_pid = getpid();
    my_tid = gettid();

    va_start(arg, form);
    vsprintf(tem_str, form, arg);

    sprintf(prit_str, "pid=0x%x tid=0x%x %d-%d-%d %d:%d:%d.%ld %s",
            my_pid, my_tid,
            tm.tm_year + 1900,
            tm.tm_mon + 1,
            tm.tm_mday,
            tm.tm_hour,
            tm.tm_min,
            tm.tm_sec,
            cur_time.tv_usec / 1000,
            tem_str);
    puts(prit_str);
}

void pabort(const char *a1)
{
    perror(a1);
    abort();
}

void printf4s(char *suff, unsigned char *data, int len)
{
    printf("%s[%d]: ", suff, len);
    for (int i = 0; i < len; i++)
    {
        printf("0x%x, ", data[i]);
    }
    printf("\n");
}

int start_main(void *func_start(void *), void *arg, unsigned int restart_msec)
{
    pid_t pid;

    pid = fork();

    if (pid < 0)
    {
        exit(1);
    }
    else if (pid > 0)
    {
        exit(0);
    }

    setsid();
    chdir("/");
    umask(0);

fork_child:
    pid = fork();
    if (pid < 0)
    {
        exit(1);
    }
    else if (pid > 0)
    {
        waitpid(pid, NULL, 0);
        usleep(restart_msec * 1000);
        goto fork_child;
    }
    else
    {
        func_start(arg);
    }

    return 0;
}

int create_periodic_timer(time_t ii_sec, long ii_nsec, time_t iv_sec, long iv_nsec)
{
    int fd;
    int ret;

    struct itimerspec timer;

    fd = timerfd_create(CLOCK_MONOTONIC, 0);

    if (fd < 0)
    {
        return -1;
    }

    timer.it_interval.tv_sec = 1;
    timer.it_interval.tv_nsec = 0;
    timer.it_value.tv_sec = 1;
    timer.it_value.tv_nsec = 0;

    ret = timerfd_settime(fd, 0, &timer, NULL);

    if (ret < 0)
    {
        close(fd);
        return -1;
    }

    return fd;
}
