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

//#define printf(...)

#define MAXPACKET 4096 /* max packet size */
#define VERBOSE 1      /* verbose flag */
#define QUIET 2        /* quiet flag */
#define MAXHOSTNAMELEN 64

static int __get_timer(int sec, int nsec);
static void __send_icmp_pkt(unsigned char *outpack, unsigned char *hostname, int s, struct sockaddr *whereto, int datalen, int *ntransmitted, int ident);
static void __pr_pack(char *buf, int cc, struct sockaddr_in *from, int *nreceived, int pingflags, int ident);
static void __tvsub(struct timespec *out, struct timespec *in);
static unsigned short __in_cksum(void *, int len);

static void __inet_ntoa(char *buf, unsigned int addr);

static int __get_timer(int sec, int nsec)
{
    int timerfd;
    struct itimerspec new_value;
    struct timespec cur_time;

    if (clock_gettime(CLOCK_MONOTONIC, &cur_time) == -1)
    {
        return -1;
    }

    new_value.it_value.tv_sec = cur_time.tv_sec + sec;
    new_value.it_value.tv_nsec = cur_time.tv_nsec + nsec;

    if (new_value.it_value.tv_nsec > 1000000000)
    {
        new_value.it_value.tv_sec += new_value.it_value.tv_nsec / 1000000000;
        new_value.it_value.tv_nsec = new_value.it_value.tv_nsec % 1000000000;
    }

    new_value.it_interval.tv_sec = sec;
    new_value.it_interval.tv_nsec = nsec;

    if (new_value.it_interval.tv_nsec > 1000000000)
    {
        new_value.it_interval.tv_sec += new_value.it_interval.tv_nsec / 1000000000;
        new_value.it_interval.tv_nsec = new_value.it_interval.tv_nsec % 1000000000;
    }

    timerfd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (timerfd == -1)
    {
        return -1;
    }

    if (timerfd_settime(timerfd, TFD_TIMER_ABSTIME, &new_value, NULL))
    {
        close(timerfd);
        return -1;
    }

    return timerfd;
}

static void __send_icmp_pkt(unsigned char *outpack, unsigned char *hostname, int s, struct sockaddr *whereto, int datalen, int *ntransmitted, int ident)
{
    struct icmp *icp = (struct icmp *)outpack;
    int i, cc;
    struct timespec *tp = (struct timespec *)&outpack[8];
    unsigned char *datap = &outpack[8 + sizeof(struct timeval)];

    icp->icmp_type = ICMP_ECHO;
    icp->icmp_code = 0;
    icp->icmp_cksum = 0;
    icp->icmp_seq = (*ntransmitted)++;
    icp->icmp_id = ident;

    cc = datalen + 8;

    clock_gettime(CLOCK_MONOTONIC, tp);

    for (i = 8; i < datalen; i++)
        *datap++ = i;

    icp->icmp_cksum = __in_cksum((void *)icp, cc);

    i = sendto(s, outpack, cc, 0, whereto, sizeof(struct sockaddr));

    if (i < 0 || i != cc)
    {
        printf("ping: wrote %s %d chars, ret=%d\n", hostname, cc, i);
    }
}

static unsigned short __in_cksum(void *addr, int len)
{
    int nleft = len;
    unsigned short *w = addr;
    unsigned short answer;
    int sum = 0;

    while (nleft > 1)
    {
        sum += *w++;
        nleft -= 2;
    }

    if (nleft == 1)
    {
        unsigned short u = 0;
        *(unsigned char *)(&u) = *(unsigned char *)w;
        sum += u;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    answer = ~sum;
    return (answer);
}

static void __pr_pack(char *buf, int cc, struct sockaddr_in *from, int *nreceived, int pingflags, int ident)
{
    struct ip *ip;
    struct icmp *icp;
    //	long *lp = (long *) buf;
    int i;

    struct timespec tv;
    struct timespec *tp;
    int hlen, triptime;
    char from_addr[64];

    from->sin_addr.s_addr = ntohl(from->sin_addr.s_addr);
    clock_gettime(CLOCK_MONOTONIC, &tv);

    __inet_ntoa(from_addr, from->sin_addr.s_addr);

    ip = (struct ip *)buf;
    hlen = ip->ip_hl << 2;
    if (cc < hlen + ICMP_MINLEN)
    {
        if (pingflags & VERBOSE)
            printf("packet too short (%d bytes) from %s\n", cc, from_addr);
        return;
    }
    cc -= hlen;
    icp = (struct icmp *)(buf + hlen);
    if ((!(pingflags & QUIET)) && icp->icmp_type != ICMP_ECHOREPLY)
    {
        /*printf("%d bytes from %s: icmp_type=%d (%s) icmp_code=%d\n", cc, from_addr, icp->icmp_type, __pr_type(icp->icmp_type), icp->icmp_code);
        if (pingflags & VERBOSE)
        {
            for (i = 0; i < 12; i++)
            printf("x%2.2x: x%8.8x\n", (unsigned int)(i * sizeof(long)), (unsigned int)(*lp++));
        }*/
        return;
    }
    if (icp->icmp_id != ident)
        return;

    tp = (struct timespec *)&icp->icmp_data[0];
    __tvsub(&tv, tp);
    triptime = tv.tv_sec * 1000000 + (tv.tv_nsec / 1000);

    if (!(pingflags & QUIET))
    {
        printf("%d bytes from %s: icmp_seq=%d time=%d us\n", cc, from_addr, icp->icmp_seq, triptime);
    }

    (*nreceived)++;
}

#if 0
static char * __pr_type( int t )
{
	static char *ttab[] = {
		"Echo Reply",
		"ICMP 1",
		"ICMP 2",
		"Dest Unreachable",
		"Source Quench",
		"Redirect",
		"ICMP 6",
		"ICMP 7",
		"Echo",
		"ICMP 9",
		"ICMP 10",
		"Time Exceeded",
		"Parameter Problem",
		"Timestamp",
		"Timestamp Reply",
		"Info Request",
		"Info Reply"
	};

	if( t < 0 || t > 16 )
		return("OUT-OF-RANGE");

	return(ttab[t]);
}
#endif

static void __tvsub(struct timespec *out, struct timespec *in)
{
    if ((out->tv_nsec -= in->tv_nsec) < 0)
    {
        out->tv_sec--;
        out->tv_nsec += 1000000000;
    }
    out->tv_sec -= in->tv_sec;
}

static void __inet_ntoa(char *buf, unsigned int addr)
{
    unsigned char ip0, ip1, ip2, ip3;

    ip0 = (addr >> 24) & 0xff;
    ip1 = (addr >> 16) & 0xff;
    ip2 = (addr >> 8) & 0xff;
    ip3 = (addr)&0xff;

    sprintf(buf, "%d.%d.%d.%d", ip0, ip1, ip2, ip3);
}

/*
paras:
1.char* host        "198.87.90.133" or "www.qq.com"
2.int flags         ping flag    0=display default information 1=display all information 2=do not display information
3.int options       ping options
4.int bk            break when receive one ICMP Echo Reply Pkt
5.int pkts          ping pkts
6.int datalen       ping pkt len
7.int interval_sec  ping interval sec
8.int interval_nsec ping interval nsec
return
 0=ping OK
-1=ping err

usage: see function main()
*/
int sys_ping(char *host, int flags, int options, int bk, int pkts, int datalen, int interval_sec, int interval_nsec)
{
    unsigned char packet_send[MAXPACKET];
    unsigned char packet_recv[MAXPACKET];

    unsigned long long expir_times;

    struct sockaddr whereto;
    struct sockaddr_in *to = (struct sockaddr_in *)&whereto;
    char hnamebuf[MAXHOSTNAMELEN];
    char ip_str[64];
    int host_name_len;
    struct protoent proto;
    struct protoent *p_proto;
    struct hostent host_ret;
    struct hostent *p_host_ret;
    int on = 1;
    int ident;
    int cc;

    int ntransmitted = 0;
    int nreceived = 0;
    int s;
    int timerfd;
    int maxfd;
    fd_set fds;

    struct sockaddr_in from;
    int fromlen = sizeof(from);

    if (interval_sec < 0 || interval_sec > 3600)
        interval_sec = 1;

    if (datalen <= 0)
    {
        datalen = 64 - 8;
    }
    else if (datalen > MAXPACKET)
    {
        datalen = MAXPACKET;
    }

    flags = flags & (VERBOSE | QUIET);
    options = options & (SO_DEBUG | SO_DONTROUTE);

    host_name_len = strlen(host);
    if (host_name_len >= MAXHOSTNAMELEN)
    {
        printf("ping: host name too long %s\n", host);
        return -1;
    }

    bzero((char *)&whereto, sizeof(whereto));
    bzero((char *)hnamebuf, sizeof(hnamebuf));
    to->sin_family = AF_INET;
    to->sin_addr.s_addr = inet_addr(host);
    if (to->sin_addr.s_addr != (unsigned)-1)
    {
        strcpy(hnamebuf, host);
    }
    else
    {
        if (gethostbyname_r(host, &host_ret, (char *)packet_recv, MAXPACKET, &p_host_ret, &cc))
        {
            printf("ping: unknown host %s\n", host);
            return -1;
        }
        else
        {
            to->sin_family = host_ret.h_addrtype;
            bcopy(host_ret.h_addr, (caddr_t)&to->sin_addr, host_ret.h_length);

            host_name_len = strlen(host_ret.h_name);
            memcpy(hnamebuf, host_ret.h_name, host_name_len >= MAXHOSTNAMELEN ? MAXHOSTNAMELEN - 1 : host_name_len);
        }
    }

    ident = syscall(__NR_gettid) & 0xFFFF;

    if (getprotobyname_r("icmp", &proto, (char *)packet_recv, MAXPACKET, &p_proto) != 0)
    {
        printf("icmp: unknown protocol\n");
        return -1;
    }

    if ((s = socket(AF_INET, SOCK_RAW, proto.p_proto)) < 0)
    {
        printf("ping: can not create socket\n");
        return -1;
    }

    if (options & SO_DEBUG)
    {
        if (flags & VERBOSE)
            printf("ping: debug on\n");
        setsockopt(s, SOL_SOCKET, SO_DEBUG, &on, sizeof(on));
    }
    if (options & SO_DONTROUTE)
    {
        if (flags & VERBOSE)
            printf("ping: no routing\n");
        setsockopt(s, SOL_SOCKET, SO_DONTROUTE, &on, sizeof(on));
    }

    if (flags & VERBOSE)
    {
        __inet_ntoa(ip_str, ntohl(to->sin_addr.s_addr));
        printf("PING %s (%s): %d data bytes\n", hnamebuf, ip_str, datalen);
    }

    timerfd = __get_timer(interval_sec, interval_nsec);
    if (timerfd == -1)
    {
        close(s);
        return -1;
    }

    __send_icmp_pkt(packet_send, (unsigned char *)hnamebuf, s, &whereto, datalen, &ntransmitted, ident);

    if (s > timerfd)
    {
        maxfd = s;
    }
    else
    {
        maxfd = timerfd;
    }

    while (1)
    {
        FD_ZERO(&fds);
        FD_SET(s, &fds);
        FD_SET(timerfd, &fds);
        select(maxfd + 1, &fds, NULL, NULL, NULL);

        if (FD_ISSET(s, &fds))
        {
            cc = recvfrom(s, packet_recv, sizeof(packet_recv), MSG_DONTWAIT, (struct sockaddr *)&from, (socklen_t *)&fromlen);
            if (cc < 0)
            {
                printf("ping: recvfrom %s err\n", hnamebuf);
            }
            else
            {
                __pr_pack((char *)packet_recv, cc, &from, &nreceived, flags, ident);

                if ((bk && nreceived) || (nreceived >= pkts))
                    //if ( nreceived >= pkts )
                    goto out_ok;
            }
        }

        if (FD_ISSET(timerfd, &fds))
        {
            read(timerfd, &expir_times, sizeof(expir_times));
            __send_icmp_pkt(packet_send, (unsigned char *)hnamebuf, s, &whereto, datalen, &ntransmitted, ident);
            if (ntransmitted >= pkts + 1)
                goto out_err;
        }
    }

out_ok:
    close(timerfd);
    close(s);
    return 0;

out_err:
    close(timerfd);
    close(s);
    return -1;
}

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

int main2(void *arg)
{
    while (1)
    {
        int ret = sys_ping("10.211.135.220", 1, 0, 0, 4, 500, 0, 500000000);
        printf("ret2: %d\n", ret);
        if (ret < 0)
        {
            printf("enable_ether_device eth0 0\n");
            enable_ether_device("eth0", 0);
            sleep(0.5);
            printf("enable_ether_device eth0 1\n");
            enable_ether_device("eth0", 1);
        }

        sleep(3);
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
