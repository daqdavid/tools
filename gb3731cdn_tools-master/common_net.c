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

#define mask_in_addr(x) (((struct sockaddr_in *)&((x).rt_genmask))->sin_addr.s_addr)

int save_net(char *ip, char *nm, char *gw)
{
    if (ip == NULL || nm == NULL || gw == NULL)
    {
        return -1;
    }

    if (!strlen(ip) || !strlen(nm) || !strlen(gw))
    {
        return -1;
    }

    unsigned char ncip[SPI_FLASH_NET_LEN] = {0};
    unsigned char ncnm[SPI_FLASH_NET_LEN] = {0};
    unsigned char ncgw[SPI_FLASH_NET_LEN] = {0};

    sscanf(ip, "%d.%d.%d.%d", &ncip[0], &ncip[1], &ncip[2], &ncip[3]);
    sscanf(nm, "%d.%d.%d.%d", &ncnm[0], &ncnm[1], &ncnm[2], &ncnm[3]);
    sscanf(gw, "%d.%d.%d.%d", &ncgw[0], &ncgw[1], &ncgw[2], &ncgw[3]);

    int ret1 = set_spi_flash(SPI_FLASH_IP_OFFSET, SPI_FLASH_NET_LEN, ncip);
    int ret2 = set_spi_flash(SPI_FLASH_NETMASK_OFFSET, SPI_FLASH_NET_LEN, ncnm);
    int ret3 = set_spi_flash(SPI_FLASH_GATEWAY_OFFSET, SPI_FLASH_NET_LEN, ncgw);

    return !ret1 && !ret2 && ret3;
}

int get_net(char *ip, char *nm, char *gw)
{
    unsigned char ncip[SPI_FLASH_NET_LEN] = {0};
    unsigned char ncnm[SPI_FLASH_NET_LEN] = {0};
    unsigned char ncgw[SPI_FLASH_NET_LEN] = {0};

    int ret1 = get_spi_flash(SPI_FLASH_IP_OFFSET, SPI_FLASH_NET_LEN, ncip);
    int ret2 = get_spi_flash(SPI_FLASH_NETMASK_OFFSET, SPI_FLASH_NET_LEN, ncnm);
    int ret3 = get_spi_flash(SPI_FLASH_GATEWAY_OFFSET, SPI_FLASH_NET_LEN, ncgw);

    sprintf(ip, "%d.%d.%d.%d", ncip[0], ncip[1], ncip[2], ncip[3], ncip[4], ncip[5]);
    sprintf(nm, "%d.%d.%d.%d", ncnm[0], ncnm[1], ncnm[2], ncnm[3], ncnm[4], ncnm[5]);
    sprintf(gw, "%d.%d.%d.%d", ncgw[0], ncgw[1], ncgw[2], ncgw[3], ncgw[4], ncgw[5]);

    return !ret1 && !ret2 && ret3;
}


/*
设置路由
cmd=0 删除指定的路由
cmd=1 添加指定的路由

返回: 0=成功 -1=失败
*/
int sys_set_route(char *destnet, char *nm, char *gw, int cmd)
{
    int skfd;
    int ret = -1;
    int route_cmd;
    struct rtentry rt;
    struct sockaddr_in *gw_sin, *dest_sin, *nm_sin;
    //printf("sys_set_route : dest=%s netmask=%s gw=%s\n", destnet, nm, gw);

    if (cmd == 0)
    {
        route_cmd = SIOCDELRT;
    }
    else if (cmd == 1)
    {
        route_cmd = SIOCADDRT;
    }
    else
    {
        printf("%s : wrong cmd=%d\n", __func__, cmd);
        return ret;
    }

    memset((char *)&rt, 0, sizeof(struct rtentry));
    rt.rt_flags = (RTF_UP | RTF_GATEWAY);
    rt.rt_flags &= ~RTF_HOST;

    gw_sin = (struct sockaddr_in *)(&rt.rt_gateway);
    dest_sin = (struct sockaddr_in *)(&rt.rt_dst);
    nm_sin = (struct sockaddr_in *)(&rt.rt_genmask);

    if (inet_aton(gw, &gw_sin->sin_addr) && inet_aton(destnet, &dest_sin->sin_addr) && inet_aton(nm, &nm_sin->sin_addr))
    {
        //if (dest_sin->sin_addr.s_addr) {
        gw_sin->sin_family = AF_INET;
        gw_sin->sin_port = 0;
        dest_sin->sin_family = AF_INET;
        dest_sin->sin_port = 0;
        nm_sin->sin_family = AF_INET;
        nm_sin->sin_port = 0;

        if (mask_in_addr(rt))
        {
            unsigned int mask = mask_in_addr(rt);
            mask = ~ntohl(mask);

            if (mask & (mask + 1))
            {
                printf("%s : bogus netmask %s\n", __func__, nm);
                return ret;
            }
            mask = ((struct sockaddr_in *)&rt.rt_dst)->sin_addr.s_addr;
            if (mask & ~mask_in_addr(rt))
            {
                printf("%s : netmask doesn't match route address %s %s\n", __func__, destnet, nm);
                return ret;
            }
        }

        if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        {
            printf("%s : create socket fail\n", __func__);
            return ret;
        }

        ret = ioctl(skfd, route_cmd, &rt);
        if (ret < 0)
        {
            //printf("%s : cmd=%d fail dest=%s nm=%s gw=%s\n", __func__, cmd, destnet, nm, gw);
        }

        close(skfd);
        //}
    }

    return ret;
}

/*
设置IP地址,子网掩码,广播地址
cmd=SIOCSIFADDR 设置IP地址
cmd=SIOCSIFNETMASK 设置子网掩码
cmd=SIOCSIFBRDADDR 广播地址
*/
int set_if_addr(char *if_name, char *if_addr, int cmd)
{
    int skfd;
    int ret = -1;
    struct ifreq ifr;
    struct sockaddr sa;
    struct sockaddr_in *sin;

    //printf("set_if_addr : cmd=%d if_name=%s if_addr=%s\n", cmd, if_name, if_addr);

    memset(&ifr, 0, sizeof(ifr));
    memset(&sa, 0, sizeof(sa));
    skfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (skfd >= 0)
    {
        safe_strncpy(ifr.ifr_name, if_name, IFNAMSIZ);
        sin = (struct sockaddr_in *)(&sa);
        sin->sin_family = AF_INET;
        sin->sin_port = 0;

        if (inet_aton(if_addr, &sin->sin_addr))
        {

            if (cmd == SIOCSIFADDR)
            {
                memcpy((char *)&ifr.ifr_addr, (char *)&sa, sizeof(struct sockaddr));
            }
            else if (cmd == SIOCSIFBRDADDR)
            {
                memcpy((char *)&ifr.ifr_broadaddr, (char *)&sa, sizeof(struct sockaddr));
            }
            else if (cmd == SIOCSIFNETMASK)
            {
                memcpy((char *)&ifr.ifr_netmask, (char *)&sa, sizeof(struct sockaddr));
            }
            else
            {
                close(skfd);
                return ret;
            }

            ret = ioctl(skfd, cmd, (char *)&ifr);
            //if(ioctl(skfd, cmd, (char *)&ifr) == 0) {
            //ret = 0;
            //}
        }

        close(skfd);
    }
    else
    {
        //printf("%s : can not create socket\n", __func__);
    }

    return ret;
}

/*
设置IP地址
返回: 0=成功 -1=失败
*/
int sys_set_ip(char *if_name, char *ip, char *nm, char *bc)
{
    struct sockaddr_in sin_ip, sin_nm, sin_bc;
    int set_ip_ret, set_nm_ret, set_bc_ret;
    unsigned int int_ip, int_nm, int_bc;
    char *new_bc;

    //检查网卡名称是否有效
    /*if ( memcmp(if_name, "eth0", sizeof("eth0")) || memcmp(if_name, "eth1", sizeof("eth1"))) {
        return -1;
    }*/

    memset((char *)&sin_ip, 0, sizeof(struct sockaddr_in));
    memset((char *)&sin_nm, 0, sizeof(struct sockaddr_in));
    memset((char *)&sin_bc, 0, sizeof(struct sockaddr_in));

    inet_aton(ip, &sin_ip.sin_addr);

    if (sin_ip.sin_addr.s_addr == 0)
    { //删除IP地址
        return set_if_addr(if_name, ip, SIOCSIFADDR);
    }
    else
    {
        inet_aton(nm, &sin_nm.sin_addr);
        inet_aton(bc, &sin_bc.sin_addr);

        int_ip = ntohl(sin_ip.sin_addr.s_addr);
        int_nm = ntohl(sin_nm.sin_addr.s_addr);

        //检查子网掩码是否合法
        if ((~int_nm) & ((~int_nm) + 1))
        {
            printf("%s : bogus netmask %s\n", __func__, nm);
            return -1;
        }

        //如果没有设置广播地址,通过ip地址和子网掩码来计算
        if (sin_bc.sin_addr.s_addr == 0)
        {
            int_bc = ((int_ip & int_nm) | (~int_nm));
            sin_bc.sin_addr.s_addr = htonl(int_bc);
            new_bc = inet_ntoa(sin_bc.sin_addr);
            //printf("new_bc=%s\n", new_bc);
        }
        else
        {
            new_bc = bc;
        }

        set_ip_ret = set_if_addr(if_name, ip, SIOCSIFADDR);
        //printf("set_ip_ret=%d %s\n", set_ip_ret, ip);
        set_nm_ret = set_if_addr(if_name, nm, SIOCSIFNETMASK);
        //printf("set_nm_ret=%d %s\n", set_ip_ret, nm);
        set_bc_ret = set_if_addr(if_name, new_bc, SIOCSIFBRDADDR);
        //printf("set_if_addr=%d %s\n", set_bc_ret, new_bc);

        return (set_ip_ret | set_nm_ret | set_bc_ret);
    }
}

int set_iface(char *if_name, int save, char *ip, char *nm, char *gw)
{
    assert(sys_set_ip(if_name, ip, nm, "") >= 0);

    int ret = 0;
    while (ret >= 0)
    {
        ret = sys_set_route("0.0.0.0", "0.0.0.0", "0.0.0.0", 0);
    }

    if (!memcmp(gw, "null", strlen(gw)))
    {
        sprintf(gw, "%s", "0.0.0.0");
    }

    if (memcmp(gw, "0.0.0.0", strlen(gw)))
    {
        assert(sys_set_route("0.0.0.0", "0.0.0.0", gw, 1) >= 0);
    }

    if (save)
    {
        assert(save_net(ip, nm, gw) >= 0);
    }

    return 0;
}
