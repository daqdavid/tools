#include "led2.h"

struct led_uart
{
    int led_fd;
    int main_board_fd;
    int timer_fd;
    int timeout_cnt;
    unsigned char recv_buf[128];
    int cur_recv_len;
    unsigned char led_buf[128];
};

extern bool m_direct;
extern unsigned char m_supper_tick_time;
extern unsigned char m_tick_num0;

static void print_hex(unsigned char *buf, int len)
{
    int i;

    for (i = 0; i < len; i++)
    {
        if (i && (i % 8) == 0)
            printf("\n");

        printf(" %02x", buf[i]);
    }

    printf("\n");
}

static int process_frame_from_mainboard(struct led_uart *led_uart, unsigned char *buf, int len)
{
    int write_ret;

    led_uart->timeout_cnt = 0;

#if 0
    printf("recv frame from mainboard:\n");
    print_hex(buf, len);
#endif

    if (!m_direct)
    {
        check_control_a1_data(buf, len);
    }

    //return;
    write_ret = write(led_uart->led_fd, buf, len);

    assert(write_ret == len);

    return 0;
}

/*
打开串口
com_name 串口设备名
databits 数据位: 目前只支持8
stopbits 停止位: 目前只支持1
parity   校验方式:  1=无校验  2=奇校验  3=偶校验
baudrate 波特率 目前支持的波特率: 600 1200 2400 4800 9600 19200 57600 115200
*/
static int open_com_dev(int databits, int stopbits, int parity, int baudrate, char *dev_name)
{
    int com_fd;
    struct termios opt;

    /*以读写方式打开串口*/
    com_fd = open(dev_name, O_RDWR | O_NOCTTY);
    if (com_fd == -1)
    {
        return (-1);
    }

    if (tcgetattr(com_fd, &opt) != 0)
    {
        close(com_fd);
        return (-1);
    }

    /* 设置波特率 */
    switch (baudrate)
    {
    case 600:
        baudrate = B600;
        break;
    case 1200:
        baudrate = B1200;
        break;
    case 2400:
        baudrate = B2400;
        break;
    case 4800:
        baudrate = B4800;
        break;
    case 9600:
        baudrate = B9600;
        break;
    case 19200:
        baudrate = B19200;
        break;
    case 57600:
        baudrate = B57600;
        break;
    case 115200:
        baudrate = B115200;
        break;
    default:
        close(com_fd);
        return (-1);
    }
    cfsetispeed(&opt, baudrate);
    cfsetospeed(&opt, baudrate);

    /*设置数据位数*/
    opt.c_cflag &= ~CSIZE;
    switch (databits)
    {
    //case 7:
    //    opt.c_cflag |= CS7;
    //    break;
    case 8:
        opt.c_cflag |= CS8;
        break;
    default:
        close(com_fd);
        return (-1);
    }

    /* 设置校验 */
    switch (parity)
    {
    case 1:                     //无校验
        opt.c_cflag &= ~PARENB; /* Clear parity enable */
        opt.c_iflag &= ~INPCK;  /* disable parity checking */
        break;
    case 2:                               //奇校验
        opt.c_cflag |= (PARODD | PARENB); /* 设置为奇校验*/
        opt.c_iflag |= INPCK;
        break;
    case 3:                     //偶校验
        opt.c_cflag |= PARENB;  /* Enable parity */
        opt.c_cflag &= ~PARODD; /* 转换为偶校验*/
        opt.c_iflag |= INPCK;   /* enable parity checking */
        break;
    default:
        close(com_fd);
        return (-1);
        break;
    }

    /* 设置停止位*/
    switch (stopbits)
    {
    case 1:
        opt.c_cflag &= ~CSTOPB;
        break;
    //case 2:
    //    opt.c_cflag |= CSTOPB;
    //    break;
    default:
        close(com_fd);
        return (-1);
    }

    opt.c_cflag |= (CLOCAL | CREAD);

    opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

    opt.c_oflag &= ~OPOST;
    opt.c_oflag &= ~(ONLCR | OCRNL);

    opt.c_iflag &= ~(ICRNL | INLCR);
    opt.c_iflag &= ~(IXON | IXOFF | IXANY);

    opt.c_cc[VTIME] = 0;
    opt.c_cc[VMIN] = 0;

    tcflush(com_fd, TCIFLUSH);

    if (tcsetattr(com_fd, TCSANOW, &opt) != 0)
    {
        close(com_fd);
        return (-1);
    }

    return com_fd;
}

/*
从uart接收完整的一帧
报文头固定为 0x11 0xa1 0x21    0x21表示当前帧后续还有0x21个字节的数据
或者         0x11 0xac 0x03    0x03表示当前帧后续还有0x03个字节的数据
*/

/*
select 监听到 龙芯板至主控制板的uart串口可读时调用本函数
返回值 -1=读取串口出错
        0=正常
*/
static int recv_frame_from_main_board(struct led_uart *led_uart)
{
    int read_ret;
    unsigned char *buf;

    buf = led_uart->recv_buf;

    read_ret = read(led_uart->main_board_fd, buf + led_uart->cur_recv_len, sizeof(led_uart->recv_buf) - led_uart->cur_recv_len);
    if (read_ret <= 0)
        return -1;

    if (read_ret > 0)
    {

//打印接收到的报文
#if 0
        printf("recv uart data %d\n", read_ret);
        print_hex(buf+led_uart->cur_recv_len, read_ret);
#endif //write(led_uart->led_fd, buf+led_uart->cur_recv_len, read_ret);

        led_uart->cur_recv_len += read_ret;
    }

recv_header:
    if (led_uart->cur_recv_len < 3)
    { //报文头固定为3个字节
        while ((led_uart->cur_recv_len > 0) && (buf[0] != 0x11))
        {
            led_uart->cur_recv_len -= 1;
            memmove(buf, buf + 1, led_uart->cur_recv_len);
        }
    }
    else
    { //已经接收的字节数大于等于报文头,检测报文头
        if (buf[0] != 0x11)
        {
            led_uart->cur_recv_len -= 1;
            memmove(buf, buf + 1, led_uart->cur_recv_len);
            goto recv_header;
        }
        else
        {
            /*
            11 ac 03  +0x03
            11 b1 02  +0x02
            11 a1 21  +0x21
            */
            if (buf[1] == 0xa1 && buf[2] == 0x21)
            { //显示报文
                if (led_uart->cur_recv_len >= 0x24)
                { //帧长为 36
                    //收到了完整的一帧
                    process_frame_from_mainboard(led_uart, buf, 0x24);
                    led_uart->cur_recv_len -= 0x24;
                    memmove(buf, buf + 0x24, led_uart->cur_recv_len);
                    goto recv_header;
                }
            }
            else if (buf[1] == 0xac && buf[2] == 0x03)
            { //控制灯的报文
                if (led_uart->cur_recv_len >= 0x06)
                { //帧长为 6
                    //收到了完整的一帧
                    process_frame_from_mainboard(led_uart, buf, 0x06);
                    led_uart->cur_recv_len -= 0x06;
                    memmove(buf, buf + 0x06, led_uart->cur_recv_len);
                    goto recv_header;
                }
            }
            else if (buf[1] == 0xb1 && buf[2] == 0x02)
            {
                if (led_uart->cur_recv_len >= 0x05)
                { //帧长为 5
                    //收到了完整的一帧
                    process_frame_from_mainboard(led_uart, buf, 0x05);
                    led_uart->cur_recv_len -= 0x05;
                    memmove(buf, buf + 0x05, led_uart->cur_recv_len);
                    goto recv_header;
                }
            }
            else
            {
                led_uart->cur_recv_len -= 1;
                memmove(buf, buf + 1, led_uart->cur_recv_len);
                goto recv_header;
            }
        }
    }

    return 0;
}

static int recv_data_from_led(struct led_uart *led_uart)
{
    int read_ret;
    int write_ret;

    read_ret = read(led_uart->led_fd, led_uart->led_buf, sizeof(led_uart->led_buf));
    if (read_ret <= 0)
        return -1;

    //printf("recv frame from led:%d\n", read_ret);
    //print_hex(led_uart->led_buf, read_ret);

    bool ret = true;

    if (!m_direct)
    {
        ret = check_led_data(led_uart->led_buf, read_ret);
    }

    if (ret)
    {
        write_ret = write(led_uart->main_board_fd, led_uart->led_buf, read_ret);

        assert(write_ret == read_ret);
    }

    return 0;
}

static int process_periodic_timer(struct led_uart *led_uart)
{
    unsigned long long dummy;

    read(led_uart->timer_fd, &dummy, sizeof(dummy));

    led_uart->timeout_cnt++;

    if (m_tick_num0 > 0)
    {
        m_tick_num0 = (m_tick_num0 <= m_supper_tick_time * 8) ? m_tick_num0 + 1 : 0;
        //printf("process_periodic_timer: %d, %d, %d\n", m_tick_num0);
    }

    if (led_uart->timeout_cnt >= 5)
        exit(0);
}

int led_main(void *arg)
{
    fd_set fd_r;
    int max_fd;
    int nfd;
    struct led_uart led_uart;

    memset(&led_uart, 0, sizeof(struct led_uart));

    led_uart.led_fd = open_com_dev(8, 1, 1, 9600, "/dev/ttyS4");
    assert(led_uart.led_fd >= 0);

    led_uart.main_board_fd = open_com_dev(8, 1, 1, 9600, "/dev/ttyS3");
    assert(led_uart.main_board_fd >= 0);

    led_uart.timer_fd = create_periodic_timer(1, 0, 1, 0);
    assert(led_uart.timer_fd >= 0);

    max_fd = led_uart.timer_fd;
    if (led_uart.led_fd > max_fd)
    {
        max_fd = led_uart.led_fd;
    }

    if (led_uart.main_board_fd > max_fd)
    {
        max_fd = led_uart.main_board_fd;
    }

    while (1)
    {
        FD_ZERO(&fd_r);
        FD_SET(led_uart.led_fd, &fd_r);
        FD_SET(led_uart.main_board_fd, &fd_r);
        FD_SET(led_uart.timer_fd, &fd_r);

        nfd = select(max_fd + 1, &fd_r, NULL, NULL, NULL);
        if (nfd > 0)
        {
            if (FD_ISSET(led_uart.led_fd, &fd_r))
            {
                recv_data_from_led(&led_uart);
            }

            if (FD_ISSET(led_uart.main_board_fd, &fd_r))
            {
                recv_frame_from_main_board(&led_uart);
            }

            if (FD_ISSET(led_uart.timer_fd, &fd_r))
            {
                process_periodic_timer(&led_uart);
            }
        }
    }
}

int print_usage(const char *a1)
{
    printf("\nread help: %s -h\n", a1);
    printf("\nread version: %s -v\n", a1);
    return 0;
}

int main(int argc, char *argv[])
{
    switch (argc)
    {
    case 1:
    {
        return start_main(led_main, NULL, 5000);
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
