#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/timerfd.h>
#include <sys/syscall.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <linux/rtnetlink.h>
#include <linux/if.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <assert.h>
#include "common_net.h"

/*
char* interface  网卡设备名,比如 eth0
int enable       使能还是禁用网卡  0=禁用 其他=使能
*/
static int enable_ether_device(char *interface, int enable)
{
    int skfd;
    struct ifreq ifr;

    if (!interface)
        return -1;

    memset(&ifr, 0, sizeof(struct ifreq));

    skfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (skfd >= 0)
    {
        strcpy(ifr.ifr_name, interface);
        if (ioctl(skfd, SIOCGIFFLAGS, &ifr) < 0)
        {
            close(skfd);
            return -1;
        }

        strcpy(ifr.ifr_name, interface);

        if (enable)
        {
            ifr.ifr_flags |= (IFF_UP | IFF_RUNNING);
        }
        else
        {
            ifr.ifr_flags &= ~(IFF_UP | IFF_RUNNING);
        }

        if (ioctl(skfd, SIOCSIFFLAGS, &ifr) < 0)
        {
            printf("SIOCSIFFLAGS up fail device=%s\n", ifr.ifr_name);
            close(skfd);
            return -1;
        }

        close(skfd);
        return 0;
    }
    else
    {
        printf("can not create socket\n");
        return -1;
    }
}

int get_num(char *filename)
{
    int fd = open(filename, O_RDONLY);
    assert(fd >= 0);

    char buf[4];
    int ret = read(fd, buf, 4);
    close(fd);
    buf[ret] = '\0';
    return atoi(buf);
}

int restart_interface(char *interface)
{
    int ret = 0;

    printf("net_guard: disable %s\n", interface);
    ret = enable_ether_device(interface, 0);
    assert(ret >= 0);

    usleep(1000 * 200);

    printf("net_guard: enable %s\n", interface);
    ret = enable_ether_device(interface, 1);
    assert(ret >= 0);

    if (!memcmp((const char *)(interface), "eth1", strlen("eth1")))
    {
        char ip[64];
        char nm[64];
        char gw[64];

        ret = get_net(ip, nm, gw);
        assert(ret >= 0);

        ret = set_iface((const char *)(interface), 0, ip, nm, gw);
        assert(ret >= 0);
    }
        
    sleep(6);
    return ret;
}

int main2(void *arg)
{
    int eth0_old_rx_errors = 0;
    int eth0_new_rx_errors = 0;

    int eth0_new_tx_errors = 0;
    int eth0_old_tx_errors = 0;

    int eth1_new_tx_errors = 0;
    int eth1_old_tx_errors = 0;

    int eth1_old_rx_errors = 0;
    int eth1_new_rx_errors = 0;

    int eth0_old_carrier = 0;
    int eth0_new_carrier = 0;

    int eth1_old_carrier = 0;
    int eth1_new_carrier = 0;

    sleep(5);

    while (1)
    {
        eth0_new_carrier = get_num("/sys/devices/platform/soc/40040000.ethernet/net/eth0/carrier");
        if (eth0_new_carrier == 1 && eth0_old_carrier == 0)
        {
            restart_interface("eth0");
        }
        eth0_old_carrier = eth0_new_carrier;

        eth0_new_rx_errors = get_num("/sys/devices/platform/soc/40040000.ethernet/net/eth0/statistics/rx_errors");
        eth0_new_tx_errors = get_num("/sys/devices/platform/soc/40040000.ethernet/net/eth0/statistics/tx_errors");

        if (eth0_new_rx_errors > eth0_old_rx_errors ||
            eth0_new_tx_errors > eth0_old_tx_errors)
        {
            restart_interface("eth0");
            eth0_old_rx_errors = eth0_new_rx_errors;
            eth0_old_tx_errors = eth0_new_tx_errors;
        }

        eth1_new_carrier = get_num("/sys/devices/platform/soc/40050000.ethernet/net/eth1/carrier");
        if (eth1_new_carrier == 1 && eth1_old_carrier == 0)
        {
            restart_interface("eth1");
        }
        eth1_old_carrier = eth1_new_carrier;

        eth1_new_rx_errors = get_num("/sys/devices/platform/soc/40050000.ethernet/net/eth1/statistics/rx_errors");
        eth1_new_tx_errors = get_num("/sys/devices/platform/soc/40050000.ethernet/net/eth1/statistics/tx_errors");

        if (eth1_new_rx_errors > eth1_old_rx_errors ||
            eth1_new_tx_errors > eth1_old_tx_errors)
        {
            restart_interface("eth1");
            eth1_old_rx_errors = eth1_new_rx_errors;
            eth1_old_tx_errors = eth1_new_tx_errors;
        }

        usleep(1000 * 100);
    }

    return 0;
}

int print_usage(const char *a1)
{
    printf("\nread version: %s -v\n", a1);
    printf("\nrun: %s\n", a1);
    return 0;
}

int main(int argc, char *argv[])
{
    switch (argc)
    {
    case 1:
    {
        return start_main(main2, NULL, 100);
    }
    case 2:
    {
        if (!memcmp((const char *)(argv[1]), "-v", strlen("-v")))
        {
            return print_version2();
        }
    }
    }

    return print_usage(*(const char **)argv);
}
