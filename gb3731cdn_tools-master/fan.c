#include <stdio.h>
#include <stdarg.h>
#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/time.h>
#include <sys/timerfd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <locale.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <poll.h>
#include <sys/poll.h>
#include <termios.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <mqueue.h>
#include <errno.h>
#include "common.h"

void init_fan()
{
    int export_fd = -1;
    int direction_fd = -2;

    if (access("/sys/class/gpio/gpio39/value", F_OK) != 0)
    {
        /*不存在生成gpio39管脚*/
        /*生成gpio39管脚*/
        export_fd = open("/sys/class/gpio/export", O_WRONLY);
        if (export_fd >= 0)
        {
            if (write(export_fd, "39", strlen("39")) != strlen("39"))
            {
                printf("%s:%d write file operation error, %s\n", __func__, __LINE__, strerror(errno));
            }
            close(export_fd);
        }

        /*配置成out模式*/
        direction_fd = open("/sys/class/gpio/gpio39/direction", O_WRONLY);
        if (direction_fd >= 0)
        {
            if (write(direction_fd, "out", strlen("out")) != strlen("out"))
            {
                printf("%s:%d write operation direction error, %s\n", __func__, __LINE__, strerror(errno));
            }
            close(direction_fd);
        }
    }
}

void set_fan(int in_state)
{
    const char *gpio39_value_file = "/sys/class/gpio/gpio39/value";

    printf("%s:%d set_fan %d\n", __func__, __LINE__, in_state);

    int fd = open(gpio39_value_file, O_WRONLY);
    assert(fd >= 0);

    if (write(fd, in_state ? "1" : "0", 1) != 1)
    {
        printf("%s:%d write file operation error, %s\n", __func__, __LINE__, strerror(errno));
    }

    close(fd);
}

void fan(void *arg)
{
    init_fan();
    char buf[128];
    int wendu = 0;
    int fd = -1;
    int ret = -1;

    int max_wendu = atoi(arg);

    printf("max: %d\n", max_wendu);

    const char *temp1_input = "/sys/class/hwmon/hwmon0/temp1_input";

    while (1)
    {
        fd = open(temp1_input, O_RDONLY);
        assert(fd >= 0);

        ret = read(fd, buf, 128);
        close(fd);
        assert(ret > 0);

        buf[ret] = '\0';
        wendu = atoi(buf);
        printf("%s: %d\n", temp1_input, wendu);

        if (wendu >= max_wendu)
        {
            set_fan(1);
            sleep(60);
        }
        else
        {
            set_fan(0);
        }

        sleep(1);
    }
}

int print_usage(const char *a1)
{
    printf("\nread version: %s -v\n", a1);
    printf("\nset max wendu: %s 55000\n", a1);
    return 0;
}

int main(int argc, char *argv[])
{
    switch (argc)
    {
    case 2:
    {
        if (!memcmp((const char *)(argv[1]), "-v", strlen("-v")))
        {
            return print_version2();
        }

        if (!memcmp((const char *)(argv[1]), "-h", strlen("-h")))
        {
            return print_usage(*(const char **)argv);
        }

        return start_main(fan, (const char *)(argv[1]), 1000);
    }
    }

    return print_usage(*(const char **)argv);
}
