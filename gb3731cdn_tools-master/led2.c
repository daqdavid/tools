#include <pthread.h>
#include "led2.h"

//m_tick_num0在两个线程中
//pthread_mutex_t m_tick_lock;
//计时器，从按下菜单键开始计时
unsigned char m_tick_num0 = 0;
//跟踪每个按钮按下的时间，超过m_supper_tick_time秒必须重来
unsigned char m_tick_num1 = 0;
//跟踪每个按钮按下的进度
unsigned char m_supper_tick_num = 0;
//每个按钮的最大间隔时间，单位秒
unsigned char m_supper_tick_time = 3;

//是否允许OK键
bool m_ok_button_yes = true;
//是否允许数字键
bool m_189_button_yes = true;
//是否直接转发，不处理内容
bool m_direct;

const int LED_TOP_MENU_HEAER_LEN = 32;

const char m_click_1 = 0x15; // 1
const char m_click_2 = 0x25; // 2
const char m_click_3 = 0x35; // 3
const char m_click_4 = 0x16; // 4
const char m_click_5 = 0x26; // 5
const char m_click_6 = 0x36; // 6
const char m_click_7 = 0x17; // 7
const char m_click_8 = 0x27; // 8
const char m_click_9 = 0x37; // 9
const char m_click_x = 0x18; // *
const char m_click_0 = 0x28; // 0
const char m_click_j = 0x38; // #

const char m_click_menu = 0x45;  //菜单
const char m_click_down = 0x46;  //下
const char m_click_left = 0x47;  //左
const char m_click_right = 0x48; //右
const char m_click_ok = 0x75;    //OK

//1. 信息
unsigned char LED_TOP_MENU_1[32] = {0x11, 0xa1, 0x21, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0xf5, 0xd8, 0xdc, 0xa4, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x11, 0x20, 0x20, 0x20, 0x31, 0x2e, 0xd9, 0xb9, 0xe4, 0xa1, 0x20, 0x20, 0x20};
//2. 板式
unsigned char LED_TOP_MENU_2[32] = {0x11, 0xa1, 0x21, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0xf5, 0xd8, 0xdc, 0xa4, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x11, 0x20, 0x20, 0x20, 0x32, 0x2e, 0xed, 0xbb, 0xe3, 0xa3, 0x20, 0x20, 0x20};
//3. 纸张
unsigned char LED_TOP_MENU_3[32] = {0x11, 0xa1, 0x21, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0xf5, 0xd8, 0xdc, 0xa4, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x11, 0x20, 0x20, 0x20, 0x33, 0x2e, 0xf2, 0xf4, 0xe3, 0xac, 0x20, 0x20, 0x20};
//4. 图形
unsigned char LED_TOP_MENU_4[32] = {0x11, 0xa1, 0x21, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0xf5, 0xd8, 0xdc, 0xa4, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x11, 0x20, 0x20, 0x20, 0x34, 0x2e, 0xde, 0xae, 0xe3, 0xbc, 0x20, 0x20, 0x20};
//5. 系统设置
unsigned char LED_TOP_MENU_5[32] = {0x11, 0xa1, 0x21, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0xf5, 0xd8, 0xdc, 0xa4, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x11, 0x20, 0x35, 0x2e, 0xf1, 0xf1, 0xf3, 0xa3, 0xa1, 0xea, 0xf3, 0xb7, 0x20};
//6. 模拟
unsigned char LED_TOP_MENU_6[32] = {0x11, 0xa1, 0x21, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0xf5, 0xd8, 0xdc, 0xa4, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x11, 0x20, 0x20, 0x20, 0x36, 0x2e, 0xe9, 0xe9, 0xe5, 0xcd, 0x20, 0x20, 0x20};
//7. 图像管理
unsigned char LED_TOP_MENU_7[32] = {0x11, 0xa1, 0x21, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0xf5, 0xd8, 0xdc, 0xa4, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x11, 0x20, 0x37, 0x2e, 0xde, 0xae, 0xd9, 0xee, 0xf1, 0xc2, 0xee, 0xa1, 0x20};
//8. Network
unsigned char LED_TOP_MENU_8[32] = {0x11, 0xa1, 0x21, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0xf5, 0xd8, 0xdc, 0xa4, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x11, 0x20, 0x20, 0x38, 0x2e, 0x4e, 0x65, 0x74, 0x77, 0x6f, 0x72, 0x6b, 0x20};
//9. 直接 USB
unsigned char LED_TOP_MENU_9[32] = {0x11, 0xa1, 0x21, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0xf5, 0xd8, 0xdc, 0xa4, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x11, 0x20, 0x39, 0x2e, 0xef, 0xb9, 0xe5, 0xff, 0x20, 0x55, 0x53, 0x42, 0x20};
//清空屏幕
unsigned char LED_CLEAR[32] = {0x11, 0xa1, 0x21, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20};

void check_control_a1_data(unsigned char *p_data, int data_size)
{
    //printf4s("control data", p_data, data_size);

    if (data_size < LED_TOP_MENU_HEAER_LEN)
    {
        return;
    }

    if (memcmp(LED_TOP_MENU_1, p_data, 5) != 0)
    {
        return;
    }

    if (memcmp(LED_TOP_MENU_1, p_data, LED_TOP_MENU_HEAER_LEN) == 0 ||
        memcmp(LED_TOP_MENU_8, p_data, LED_TOP_MENU_HEAER_LEN) == 0 ||
        memcmp(LED_TOP_MENU_9, p_data, LED_TOP_MENU_HEAER_LEN) == 0)
    {
        //dbg_printf("%s:%d: memcmp LED_TOP_MENU_1/8/9 OK: Disabled m_ok_button_yes m_189_button_yes !!", __func__, __LINE__);
        m_ok_button_yes = false;
        m_189_button_yes = false;
        return;
    }

    if (memcmp(LED_TOP_MENU_2, p_data, LED_TOP_MENU_HEAER_LEN) == 0 ||
        memcmp(LED_TOP_MENU_3, p_data, LED_TOP_MENU_HEAER_LEN) == 0 ||
        memcmp(LED_TOP_MENU_4, p_data, LED_TOP_MENU_HEAER_LEN) == 0 ||
        memcmp(LED_TOP_MENU_5, p_data, LED_TOP_MENU_HEAER_LEN) == 0 ||
        memcmp(LED_TOP_MENU_6, p_data, LED_TOP_MENU_HEAER_LEN) == 0 ||
        memcmp(LED_TOP_MENU_7, p_data, LED_TOP_MENU_HEAER_LEN) == 0 ||
        memcmp(LED_CLEAR, p_data, LED_TOP_MENU_HEAER_LEN) == 0)
    {
        //dbg_printf("%s:%d: memcmp LED_TOP_MENU_2-7/C OK: Enabled m_ok_button_yes && Disabled m_189_button_yes !!", __func__, __LINE__);
        m_ok_button_yes = true;
        m_189_button_yes = false;
        return;
    }

    //dbg_printf("%s:%d: memcmp LED_TOP_MENU FALSE: Enabled m_ok_button_yes m_189_button_yes !!", __func__, __LINE__);

    m_ok_button_yes = true;
    m_189_button_yes = true;
}

void set_di(char *value)
{
    int fd;
    int len;
    int ret;
    char gpio_op[64];

    //1.export gpio41
    fd = open("/sys/class/gpio/export", O_WRONLY, 0400);
    if (fd >= 0)
    {
        len = sprintf(gpio_op, "41\n");
        ret = write(fd, gpio_op, len);
        close(fd);
    }

    //2.set gpio41 direction
    fd = open("/sys/class/gpio/gpio41/direction", O_WRONLY, 0644); //O_RDONLY
    if (fd >= 0)
    {
        len = sprintf(gpio_op, "out\n");
        ret = write(fd, gpio_op, len);
        close(fd);

        if (ret != len)
        {
            printf("set gpio41 direction to out fail\n");
        }
    }

    //3.set gpio41 default value to '1'
    fd = open("/sys/class/gpio/gpio41/value", O_WRONLY, 0644); //O_RDONLY
    if (fd >= 0)
    {
        len = sprintf(gpio_op, value);
        ret = write(fd, gpio_op, len);
        close(fd);

        if (ret != len)
        {
            printf("set gpio41 value to 0 fail\n");
        }
    }

    return 0;
}

void *set_di_func(void *arg)
{
    system("/sbin/iptables -P FORWARD ACCEPT");
    system("/sbin/iptables -t nat -I PREROUTING -p tcp --dport 9080 -j DNAT --to-destination 10.211.135.220:80");
    system("/sbin/iptables -t nat -I POSTROUTING -p tcp -d 10.211.135.220 --dport 80 -j SNAT --to-source 10.211.135.221");

    set_di("1\n");
    sleep(1);
    set_di("0\n");
}

void check_admin_click(unsigned char *p_data, int data_size)
{
    if (p_data[0] == 0xb0 ||
        p_data[0] == 0xaf)
    {
        return;
    }

    printf4s("check_admin_click", p_data, data_size);

    if (p_data[0] == m_click_menu)
    {
        //pthread_mutex_lock(&(m_tick_lock));
        m_tick_num0 = 1;
        //pthread_mutex_unlock(&(m_tick_lock));
        m_tick_num1 = 1;
        m_supper_tick_num = 1;
        printf("%d, %d, %d\n", m_tick_num0, m_tick_num1, m_supper_tick_num);
        return;
    }

    if (m_tick_num0 - m_tick_num1 < m_supper_tick_time)
    {
        if (m_supper_tick_num == 1 && p_data[0] == m_click_j)
        {
            m_tick_num1 = m_tick_num0;
            m_supper_tick_num = 2;
            printf("%d, %d, %d\n", m_tick_num0, m_tick_num1, m_supper_tick_num);
            return;
        }

        if (m_supper_tick_num == 2 && p_data[0] == m_click_2)
        {
            m_tick_num1 = m_tick_num0;
            m_supper_tick_num = 3;
            printf("%d, %d, %d\n", m_tick_num0, m_tick_num1, m_supper_tick_num);
            return;
        }

        if (m_supper_tick_num == 3 && p_data[0] == m_click_7)
        {
            m_tick_num1 = m_tick_num0;
            m_supper_tick_num = 4;
            printf("%d, %d, %d\n", m_tick_num0, m_tick_num1, m_supper_tick_num);
            return;
        }

        if (m_supper_tick_num == 4 && p_data[0] == m_click_6)
        {
            m_tick_num1 = m_tick_num0;
            m_supper_tick_num = 5;
            printf("%d, %d, %d\n", m_tick_num0, m_tick_num1, m_supper_tick_num);
            return;
        }

        if (m_supper_tick_num == 5 && p_data[0] == m_click_5)
        {
            m_direct = true;
            pthread_t di_thread;
            printf("check_admin_click SUSS !!\n");

            if (pthread_create(&di_thread, NULL, set_di_func, NULL) != 0)
            {
                pabort("pthread_create di_thread fail !!!");
            }
        }
    }

    m_supper_tick_num = 0;
    //pthread_mutex_lock(&(m_tick_lock));
    m_tick_num0 = 0;
    //pthread_mutex_unlock(&(m_tick_lock));
    m_tick_num1 = 0;
    printf("%d, %d, %d\n", m_tick_num0, m_tick_num1, m_supper_tick_num);
}

bool check_led_data(unsigned char *p_data, int data_size)
{
    if (p_data == NULL || data_size == 0)
    {
        return false;
    }

    if (strlen(p_data) == 0)
    {
        return false;
    }

    check_admin_click(p_data, data_size);

    if (!m_ok_button_yes && p_data[0] == 0x75) // OK
    {
        return false;
    }

    if (!m_189_button_yes &&
        (p_data[0] == 0x15 || // 1
         p_data[0] == 0x27 || // 8
         p_data[0] == 0x37)   // 9
    )
    {
        dbg_printf("%s:%d: return false !!", __func__, __LINE__);
        return false;
    }

    return true;
}
