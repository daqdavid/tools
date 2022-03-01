#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <linux/if_ether.h>
#include <net/if_arp.h>
#include <arpa/inet.h>
#include <net/route.h>
#include "spi_flash.h"
#include "common.h"
#include "common_net.h"

#define _PATH_PROCNET_ROUTE "/proc/net/route"

/*
获取IP地址
返回: 0=成功 -1=失败
*/
int sys_get_ip(char *if_name, char *ip, char *nm, char *bc)
{
    int skfd;
    int get_ip_ret = -1;
    int get_nm_ret = -1;
    int get_bc_ret = -1;
    struct ifreq ifr;

    //检查网卡名称是否有效
    /*if ( memcmp(if_name, "eth0", sizeof("eth0")) || memcmp(if_name, "eth1", sizeof("eth1"))) {
        return -1;
    }*/

    if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0)
    {
        memset((char *)&ifr, 0, sizeof(struct ifreq));

        strcpy(ifr.ifr_name, if_name);
        get_ip_ret = ioctl(skfd, SIOCGIFADDR, &ifr);
        if (!get_ip_ret)
        {
            strcpy(ip, inet_ntoa(((struct sockaddr_in *)(&ifr.ifr_addr))->sin_addr));
        }
        else
        {
            *ip = '\0';
        }

        get_nm_ret = ioctl(skfd, SIOCGIFNETMASK, &ifr);
        if (!get_ip_ret)
        {
            strcpy(nm, inet_ntoa(((struct sockaddr_in *)(&ifr.ifr_netmask))->sin_addr));
        }
        else
        {
            *nm = '\0';
        }

        get_bc_ret = ioctl(skfd, SIOCGIFBRDADDR, &ifr);
        if (!get_bc_ret)
        {
            strcpy(bc, inet_ntoa(((struct sockaddr_in *)(&ifr.ifr_broadaddr))->sin_addr));
        }
        else
        {
            *bc = '\0';
        }

        close(skfd);
        return (get_ip_ret | get_nm_ret | get_bc_ret);
    }

    return -1;
}

/*
获取路由,destnet和nm由调用者提供 格式为字符串198.120.0.99 255.255.0.0
destnet 目的网络
nm      子网掩码
gw      gateway 将destnet和nm相关的网关填入gw中

成功 返回0 将网关信息填入 gw中
失败 返回-1
*/
/*int sys_get_route(char* destnet, char* nm, char* gw)
{
    char buff[1024], iface[IFNAMSIZ];
    char gate_addr[64], net_addr[64], mask_addr[64];
    int num, iflags, metric, refcnt, use, mss, window, irtt;
    char *fmt;
    struct sockaddr_in target, mask;


    if (!destnet || (inet_aton(destnet, &target.sin_addr) == -1)){
        printf("%s : destnet err\n", __func__);
        return -1;
    }

    if (!target.sin_addr.s_addr) {
        printf("%s : destnet err\n", __func__);
        return -1;
    }

    if (nm){
        inet_aton(nm, &mask.sin_addr);
    }

    FILE *fp = fopen(_PATH_PROCNET_ROUTE, "r");

    if(!fp){
        printf("%s : can not open %s\n", __func__, _PATH_PROCNET_ROUTE);
        return -1;
    }

    fmt = proc_gen_fmt(_PATH_PROCNET_ROUTE, 0, fp,
            "Iface", "%16s",
            "Destination", "%128s",
            "Gateway", "%128s",
            "Flags", "%X",
            "RefCnt", "%d",
            "Use", "%d",
            "Metric", "%d",
            "Mask", "%128s",
            "MTU", "%d",
            "Window", "%d",
            "IRTT", "%d",
            NULL);

    if (!fmt){
        fclose(fp);
        return -1;
    }

    while (fgets(buff, 1023, fp)) {
        struct sockaddr_in snet_target, snet_gateway, snet_mask;
        num = sscanf(buff, fmt,
            iface, net_addr, gate_addr,
            &iflags, &refcnt, &use, &metric, mask_addr,
            &mss, &window, &irtt);
        if (num < 10 || !(iflags & RTF_UP))
            continue;

        inet_getsock(net_addr, &snet_target);
        inet_getsock(gate_addr, &snet_gateway);
        inet_getsock(mask_addr, &snet_mask);

        if (target.sin_addr.s_addr == snet_target.sin_addr.s_addr) {
            if (nm){
                if (mask.sin_addr.s_addr != snet_mask.sin_addr.s_addr)
                    continue;
            }

            strcpy(gw, inet_ntoa(snet_gateway.sin_addr));
            fclose(fp);
            free(fmt);
            return 0;
        }
    }

    fclose(fp);
    free(fmt);
    return -1;
}*/

int sys_get_gw(const char *_if_name, char *gw)
{
    char if_name[IFNAMSIZ];
    unsigned int dest = 0;
    unsigned int gateway = 0;
    char buf[256] = {};
    FILE *fp = NULL;

    fp = fopen(_PATH_PROCNET_ROUTE, "r");
    if (NULL == fp)
    {
        printf("GetDefaultGateway open file: %s failure\n", _PATH_PROCNET_ROUTE);
        return -1;
    }

    while (fgets(buf, 255, fp))
    {
        sscanf(buf, "%16s %X %X", if_name, &dest, &gateway);

        if (gateway && (strlen(if_name) == strlen(if_name)) && !memcmp(_if_name, if_name, strlen(if_name)))
        {
            fclose(fp);
            sprintf(gw, "%d.%d.%d.%d\0", gateway & 0xff, (gateway >> 8) & 0xff, (gateway >> 16) & 0xff, (gateway >> 24) & 0xff);

            return 0;
        }
    }

    fflush(fp);
    fclose(fp);

    return -1;
}

int print_all_iface()
{
    int socketfd = socket(AF_INET, SOCK_STREAM, 0);
    //int socketfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    //int socketfd = socket(AF_ROUTE, SOCK_RAW, 0);
    if (socketfd == -1)
    {
        printf("ERROR: %s %d\n", __func__, __LINE__);
        return -1;
    }

    struct ifconf ifc;
    struct ifreq req[10];
    ifc.ifc_len = sizeof(req);
    ifc.ifc_buf = (__caddr_t)req;
    if (ioctl(socketfd, SIOCGIFCONF, &ifc) == -1)
    {
        printf("ERROR: %s %d\n", __func__, __LINE__);
        close(socketfd);
        return -1;
    }

    int ifreq_size = ifc.ifc_len / sizeof(struct ifreq);
    for (int i = 0; i < ifreq_size; i++)
    {
        struct ifreq *p_ifr = &ifc.ifc_req[i];
        struct ifreq ifr;
        strcpy(ifr.ifr_name, p_ifr->ifr_name);
        if (ioctl(socketfd, SIOCGIFFLAGS, &ifr) == 0)
        {
            if (!(ifr.ifr_flags & IFF_LOOPBACK))
            {
                print_iface((const char *)(ifr.ifr_name));
            }
        }
        else
        {
            printf("ERROR: %s %d\n", __func__, __LINE__);
            close(socketfd);
            return -1;
        }
    }

    close(socketfd);
    return 0;
}

int print_usage(const char *a1)
{
    puts(
        "\n按照以下方式进行调用\n"
        "/bin/net_util eth0 ip=192.168.4.5 netmask=255.255.255.0 gw=192.168.4.254\n"
        "\n没有gw,按照以下方式调用\n"
        "/bin/net_util eth0 ip=192.168.4.5 netmask=255.255.255.0 gw=null\n"
        "\n如果只有一个参数,net_util向标准输出终端输出当前的网络参数\n"
        "例如直接执行 /bin/net_util eth0\n"
        "程序输出\n"
        "net_util:eth0 ip=192.168.4.5 netmask=255.255.255.0 gw=192.168.4.254\n"
        "\n如果系统当前没有网关,输出\n"
        "net_util:eth0 ip=192.168.4.5 netmask=255.255.255.0 gw=null\n"
        "\n如果没有参数,net_util向标准输出终端输出所有网络设备的网络参数\n"
        "net_util:eth0 ip=192.168.4.5 netmask=255.255.255.0 gw=192.168.4.254\n"
        "net_util:eth1 ip=10.1.4.5    netmask=255.255.255.0 gw=10.1.4.254\n"
        "\n或者没有网关的情况\n"
        "net_util:eth0 ip=192.168.4.5 netmask=255.255.255.0 gw=null\n"
        "net_util:eth1 ip=10.1.4.5    netmask=255.255.255.0 gw=10.1.4.254\n"
        "\n读取SPI FLASH的参数，并写入网卡\n"
        "net_util -s eth1\n"
        "\n\n调用时带 -v时，输出 版本号信息\n"
        "net_util -v 输出 \n"
        "net_util:V1.0.0\n");

    return 0;
}

int print_version(const char *a1)
{
    printf("net_util:V1.0.0\n");
    return 0;
}

int print_iface(const char *if_name)
{
    char ip[64];
    char nm[64];
    char bc[64];
    char gw[64];

    if (sys_get_ip(if_name, ip, nm, bc) != 0)
    {
        sprintf(ip, "null\0");
        sprintf(nm, "null\0");
    }

    if (sys_get_gw(if_name, gw) != 0)
    {
        sprintf(gw, "null\0");
    }

    printf("net_util:%s ip=%s\tnetmask=%s\tgw=%s\n", if_name, ip, nm, gw);

    return 0;
}

int sys_set_flag(char *if_name, short int flag)
{
    struct ifreq ifr;

    int skfd = socket(AF_INET, SOCK_STREAM, 0);
    if (skfd == -1)
    {
        printf("ERROR: %s %d\n", __func__, __LINE__);
        return skfd;
    }

    safe_strncpy(ifr.ifr_name, if_name, IFNAMSIZ);

    if (ioctl(skfd, SIOCGIFFLAGS, &ifr) < 0)
    {
        printf("ERROR: %s %d\n", __func__, __LINE__);
        return (-1);
    }

    safe_strncpy(ifr.ifr_name, if_name, IFNAMSIZ);

    ifr.ifr_flags |= flag;

    int ret = ioctl(skfd, SIOCSIFFLAGS, &ifr);

    close(skfd);

    return ret;
}

int sys_clr_flag(char *if_name, short int flag)
{
    struct ifreq ifr;

    int skfd = socket(AF_INET, SOCK_STREAM, 0);
    if (skfd == -1)
    {
        printf("ERROR: %s %d\n", __func__, __LINE__);
        return skfd;
    }

    safe_strncpy(ifr.ifr_name, if_name, IFNAMSIZ);

    if (ioctl(skfd, SIOCGIFFLAGS, &ifr) < 0)
    {
        printf("ERROR: %s %d\n", __func__, __LINE__);
        return -1;
    }

    safe_strncpy(ifr.ifr_name, if_name, IFNAMSIZ);

    ifr.ifr_flags &= ~flag;

    int ret = ioctl(skfd, SIOCSIFFLAGS, &ifr);

    close(skfd);

    return ret;
}

int main(int argc, char **argv)
{
    switch (argc)
    {
    case 1:
        return print_all_iface();

    case 2:
    {
        if (!memcmp((const char *)(argv[1]), "-v", strlen("-v")))
        {
            //return print_version(*(const char **)argv);
            return print_version2();
        }

        return print_iface((const char *)(argv[1]));
    }

    case 3:
    {
        if (!memcmp((const char *)(argv[2]), "down", strlen("down")))
        {
            int ret = sys_clr_flag((const char *)(argv[1]), IFF_UP);
            if (ret < 0)
            {
                printf("ERROR: %s %d\n", __func__, __LINE__);
            }
            return ret;
        }

        if (!memcmp((const char *)(argv[2]), "up", strlen("up")))
        {
            int ret = sys_set_flag((const char *)(argv[1]), (short)(IFF_UP | IFF_RUNNING));
            if (ret < 0)
            {
                printf("ERROR: %s %d\n", __func__, __LINE__);
            }
            return ret;
        }

        if (!memcmp((const char *)(argv[1]), "-s", strlen("-s")))
        {
            char ip[64];
            char nm[64];
            char gw[64];

            int ret = get_net(ip, nm, gw);
            if (ret >= 0)
            {
                return set_iface((const char *)(argv[2]), 0, ip, nm, gw);
            }

            return ret;
        }

        break;
    }

    case 5:
    {
        if (!memcmp(argv[2], "ip=", strlen("ip=")) && !memcmp(argv[3], "netmask=", strlen("netmask=")) && !memcmp(argv[4], "gw=", strlen("gw=")))
        {

            char ip[64];
            char nm[64];
            char gw[64];

            safe_strncpy(ip, (const char *)argv[2] + 3, strlen(argv[2]) - 2);
            safe_strncpy(nm, (const char *)argv[3] + 8, strlen(argv[3]) - 7);
            safe_strncpy(gw, (const char *)argv[4] + 3, strlen(argv[4]) - 2);

#ifdef __H6__
            int save = 1;
#elif __LX2K1000__
            int save = !memcmp((const char *)(argv[1]), "eth0", strlen("eth0")) ? 0 : 1;
#else
            int save = 0;
#endif

            return set_iface((const char *)(argv[1]), save, ip, nm, gw);
        }

        break;
    }
    }

    /*for(int i=0; i<argc; i++) {
		printf("%s\n", argv[i]);
	}*/

    return print_usage(*(const char **)argv);
}
