#include "common.h"

void print_usage(const char *a1)
{
    printf("Usage: %s [-uv]\n", a1);
    puts(
        "  -u usb device (example /dev/ttyS1)\n"
        "  -w write mode\n"
        "  -r read mode\n"
        "  -v print version\n");
    exit(1);
}

unsigned char *m_usb_device = "/dev/ttyS4";
unsigned short m_data_len = 0;
unsigned int m_run_mode = 0;
unsigned long long m_data_count = 0;
unsigned int m_msleep_time = 0;
unsigned char *m_recv_full_data;
unsigned short m_recv_full_data_len;
bool m_debug = 0;

unsigned long long m_temp_count = 0;

void parse_opts(int a1, char **a2)
{
    int result;

    while (1)
    {
        result = getopt_long(a1, a2, "u:s:t:vdrw", NULL, 0LL);

        if (result == -1)
        {
            break;
        }

        switch (result)
        {
        case 'u':
            m_usb_device = (char *)optarg;
            break;
        case 'd':
            m_debug = true;
            break;
        case 'w':
            m_run_mode = 1;
            break;
        case 'r':
            m_run_mode = 2;
            break;
        case 's':
            m_data_len = atoi(optarg);
            break;
        case 't':
            m_msleep_time = atoi(optarg);
            break;
        case 'v':
            print_version2();
            exit(1);
        default:
            print_usage(*(const char **)a2);
            exit(1);
        }
    }
}

unsigned char calcXor(unsigned char *buf, unsigned char len)
{
    unsigned char i = 0;
    unsigned char xorVal = 0;

    for (i = 0; i < len; i++)
    {
        xorVal ^= buf[i];
    }

    return xorVal;
}

void write_data(int fd, unsigned char *data, unsigned short m_data_len)
{
    unsigned short len = m_data_len;

    while (1)
    {
        usleep(m_msleep_time * 1000);

        int ret = write(fd, data, len);
        if (ret <= 0) //发送失败
        {
       //     printf("%s:%d write file operation error, %s\n", __func__, __LINE__, strerror(errno));
            continue;
        }
        else if (ret == len) //发送成功
        {
            break;
        }

        printf("send sice: %d\n", ret);
        len = m_data_len - ret; //发送成功一部分
    }
}

void write_test()
{
    assert(m_data_len >= 10);
    unsigned char *data = (unsigned char *)malloc(m_data_len);
    data[0] = 0x11;
    data[1] = 0xA1;
    *(unsigned short *)(data + 2) = m_data_len - 3;
    memset(data + 12, 0xff, m_data_len - 13);

    int export_fd = open(m_usb_device, O_WRONLY);
    assert(export_fd >= 0);

    int flag = fcntl(export_fd, F_SETFL, O_NONBLOCK | fcntl(export_fd, F_GETFL, 0));
    assert(flag >= 0);

    while (1)
    {
        *(unsigned long long *)(data + 4) = m_data_count++;
        data[m_data_len - 1] = calcXor((unsigned char *)(data + 1), m_data_len - 2);

        if (m_debug)
        {
            printf4s("data: ", data, m_data_len);
        }
        printf("m_data_count: %lld\n", m_data_count);

        write_data(export_fd, data, m_data_len);
    }

    close(export_fd);
}

int BytesIndex(unsigned char *src, int src_len, unsigned char seq)
{
    for (int i = 0; i < src_len; i++)
    {
        if (src[i] == seq)
        {
            return i;
        }
    }

    return -1;
}

int check_usb_data(unsigned char *recv_one_data, int recv_one_data_size)
{
    if (m_debug)
    {
        printf4s("recv_one_data2: ", recv_one_data, recv_one_data_size);
    }

    if (recv_one_data != NULL && recv_one_data_size > 0)
    {
        if (m_recv_full_data_len == 0)
        {
            if (m_debug)
            {
                printf4s("recv_one_data3: ", recv_one_data, recv_one_data_size);
            }
            int res0 = BytesIndex(recv_one_data, recv_one_data_size, 0x11);
            if (res0 >= 0)
            {
                memcpy(m_recv_full_data, recv_one_data + res0, recv_one_data_size - res0);
                m_recv_full_data_len = recv_one_data_size - res0;
            }
        }
        else
        {
            memcpy(m_recv_full_data + m_recv_full_data_len, recv_one_data, recv_one_data_size);
            m_recv_full_data_len += recv_one_data_size;
        }
    }

    if (m_debug)
    {
        printf4s("recv_full_data4: ", m_recv_full_data, m_recv_full_data_len);
    }

    if (m_recv_full_data_len >= m_data_len)
    {
        if (m_recv_full_data[1] == 0xa1)
        {
            unsigned short this_len = *(unsigned short *)(m_recv_full_data + 2) + 3;

            if (m_recv_full_data_len >= this_len)
            {
                if (m_recv_full_data[this_len - 1] == calcXor((unsigned char *)(m_recv_full_data + 1), this_len - 2))
                {
                    unsigned long long this_data_count = *(unsigned long long *)(m_recv_full_data + 4);

                    printf("data num in recv: %d\n", this_data_count);

                    if (m_data_count == 0 || this_data_count - m_data_count == 1)
                    {
                        m_data_count = this_data_count;
                    }
                    else
                    {
                        printf4s("recv_one_data8: ", recv_one_data, recv_one_data_size);
                        printf4s("recv_full_data9: ", m_recv_full_data, m_recv_full_data_len);
                        printf("m_data_count(%d) != this_data_count(%d)\n", m_data_count, this_data_count);
                        exit(0);
                    }

                    if (m_recv_full_data_len > this_len)
                    {
                        //printf4s("recv_one_data12: ", recv_one_data, recv_one_data_size);
                        //printf4s("recv_full_data13: ", m_recv_full_data, m_recv_full_data_len);
                        printf("m_recv_full_data_len(%d) != this_len(%d)\n", m_recv_full_data_len, this_len);

                        memmove(m_recv_full_data, (unsigned char *)m_recv_full_data + this_len, m_recv_full_data_len - this_len);
                        m_recv_full_data_len = m_recv_full_data_len - this_len;
                        //printf4s("recv_full_data6: ", m_recv_full_data, m_recv_full_data_len);
                        return check_usb_data(NULL, 0);
                    }

                    m_recv_full_data_len = 0;
                    memset(m_recv_full_data, 0x0, m_data_len * 3);
                    return 0;
                }
            }
        }

        printf4s("recv_one_data10: ", recv_one_data, recv_one_data_size);
        printf4s("recv_full_data11: ", m_recv_full_data, m_recv_full_data_len);

        int res0 = BytesIndex(m_recv_full_data + 1, m_recv_full_data_len - 1, 0x11);
        if (res0 >= 0)
        {
            m_recv_full_data_len = m_recv_full_data_len - res0 - 1;
            memmove(m_recv_full_data, m_recv_full_data + res0 + 1, m_recv_full_data_len);
            printf4s("recv_full_data12: ", m_recv_full_data, m_recv_full_data_len);
            return check_usb_data(NULL, 0);
        }

        m_recv_full_data_len = 0;
        memset(m_recv_full_data, 0x0, m_data_len * 3);
    }

    return 0;
}

void read_test()
{
    fd_set fs_read;
    int recv_one_data_size = 0;
    unsigned char *recv_one_data = (unsigned char *)malloc(m_data_len);
    m_recv_full_data = (unsigned char *)malloc(m_data_len * 3);
    memset(m_recv_full_data, 0x0, m_data_len * 3);
    m_recv_full_data_len = 0;

    int usb_device_fd = open(m_usb_device, O_RDONLY);
    assert(usb_device_fd >= 0);

    while (1)
    {
        FD_ZERO(&fs_read);
        FD_SET(usb_device_fd, &fs_read);

        int nfd = select(usb_device_fd + 1, &fs_read, NULL, NULL, NULL);
        if (nfd > 0)
        {
            if (FD_ISSET(usb_device_fd, &fs_read))
            {
                memset(recv_one_data, 0x0, m_data_len);
                recv_one_data_size = read(usb_device_fd, recv_one_data, m_data_len);
                assert(recv_one_data_size >= 0);
                printf("\nwhile num %lld: recv_one_data_size: %lld;\t", m_temp_count++, recv_one_data_size);
                check_usb_data(recv_one_data, recv_one_data_size);
            }
        }
    }
}

int main(int argc, char *argv[])
{
    parse_opts(argc, argv);

    if (m_run_mode == 1)
    {
        write_test();
    }

    if (m_run_mode == 2)
    {
        read_test();
    }

    return 0;
}
