#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/param.h>
#include <net/if.h>
#include <linux/if_ether.h>
#include <net/if_arp.h>
#include <arpa/inet.h>
#include <net/route.h>
#include "spi_flash.h"
#include "common.h"

#if 0
int print_all_mac(const char* if_name) {
    int skfd = socket(AF_ROUTE, SOCK_RAW, 0); //socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (skfd == -1) {
        printf("socket error\n");
        return -1;
    }

    struct ifconf ifc;
    struct ifreq req[10];
    ifc.ifc_len = sizeof(req);
    ifc.ifc_buf = (__caddr_t) req;
    if (ioctl(skfd, SIOCGIFCONF, &ifc) == -1) {
        printf("ioctl error\n");
        return -1;
    }

    int ifreq_size = ifc.ifc_len/sizeof(struct ifreq);
    for (int i=0; i<ifreq_size; i++) {
        struct ifreq *p_ifr = &ifc.ifc_req[i];

        if((strlen(p_ifr->ifr_name) == strlen(if_name)) && !memcmp(p_ifr->ifr_name, if_name, strlen(if_name))) {
            struct ifreq ifr;
            strcpy(ifr.ifr_name, p_ifr->ifr_name);
            //if (ioctl(skfd, SIOCGIFFLAGS, &ifr) == 0) {
                //if (! (ifr.ifr_flags & IFF_LOOPBACK)) {
                    if (ioctl(skfd, SIOCGIFHWADDR, &ifr) == 0) {
                        unsigned char * ptr = (unsigned char  *)&ifr.ifr_ifru.ifru_hwaddr.sa_data[0];
                        printf("%s %02X:%02X:%02X:%02X:%02X:%02X\n",ifr.ifr_name, *ptr,*(ptr+1),*(ptr+2),*(ptr+3),*(ptr+4),*(ptr+5));
                    }
                //}
            /*} else{
                printf("error\n");
                return -1;
            }*/

            return 0;
        }
    }

    return -1;
}
#endif

/*
获取MAC地址
返回: 0=成功 -1=失败
*/
int sys_get_mac(char *if_name, char *mac)
{
    int skfd = socket(AF_INET, SOCK_STREAM, 0);
    if (skfd == -1)
    {
        printf("socket error\n");
        return -1;
    }

    struct ifreq ifr;
    memset((char *)&ifr, 0, sizeof(struct ifreq));
    strcpy(ifr.ifr_name, if_name);

    if (ioctl(skfd, SIOCGIFHWADDR, &ifr) == 0)
    {
        unsigned char *ptr = (unsigned char *)&ifr.ifr_ifru.ifru_hwaddr.sa_data[0];
        sprintf(mac, "%02X:%02X:%02X:%02X:%02X:%02X", *ptr, *(ptr + 1), *(ptr + 2), *(ptr + 3), *(ptr + 4), *(ptr + 5));
    }
    else
    {
        sprintf(mac, "null\0");
    }

    close(skfd);
    return 0;
}

/*
设置MAC地址
*/
int sys_set_mac(char *if_name, char *mac)
{
    //组播判断
    if (mac[0] & 0x01 == 0x01)
    {
        return -1;
    }

    struct ifreq ifreq;
    char temp[SPI_FLASH_MAC_LEN] = {0};

    int skfd = socket(AF_INET, SOCK_STREAM, 0);

    ifreq.ifr_addr.sa_family = ARPHRD_ETHER;

    strcpy(ifreq.ifr_name, if_name);

    memcpy((unsigned char *)ifreq.ifr_hwaddr.sa_data, mac, SPI_FLASH_MAC_LEN);

    int ret1 = ioctl(skfd, SIOCSIFHWADDR, &ifreq);

    close(skfd);

    return ret1;
}

int print_mac(const char *if_name)
{
    char mac[64] = {0};
    assert(sys_get_mac(if_name, mac) >= 0);
    printf("%s\n", mac);
    return 0;
}

int print_usage(const char *a1)
{
    printf("\nprint mac: %s eth1\n", a1);
    printf("\nset mac: %s eth1 00:11:22:33:44:55\n", a1);
    printf("\nset mac from spi flash: %s -s eth1\n", a1);
    printf("\nhelp: %s -h\n", a1);
    return 0;
}

int main(int argc, char *argv[])
{
    switch (argc)
    {
    case 2:
    {
        if (!memcmp(argv[1], "-v", strlen("-v")))
        {
            return print_version2();
        }

        if (!memcmp((const char *)(argv[1]), "eth1", strlen("eth1")))
        {
            assert(print_mac("eth1") >= 0);
            return 0;
        }
    }

    case 3:
    {
        char mac[SPI_FLASH_MAC_LEN] = {0};

        if (!memcmp((const char *)(argv[1]), "-s", strlen("-s")))
        {
            assert(get_spi_flash(SPI_FLASH_MAC_OFFSET, SPI_FLASH_MAC_LEN, mac) >= 0);
            assert(sys_set_mac((const char *)(argv[2]), mac) >= 0);
            return 0;
        }

        if (!memcmp((const char *)(argv[1]), "eth1", strlen("eth1")))
        {
            sscanf((const char *)(argv[2]), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                   &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);

            assert(sys_set_mac("eth1", mac) >= 0);
            assert(set_spi_flash(SPI_FLASH_MAC_OFFSET, SPI_FLASH_MAC_LEN, mac) >= 0);
            return 0;
        }
    }
    }

    return print_usage(*(const char **)argv);
}
