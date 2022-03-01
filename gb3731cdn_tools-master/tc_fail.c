#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <time.h>
#include <sys/select.h>
#include <sys/timerfd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <termios.h>
#include <sys/stat.h>
#include "errno.h"

#define PROCESS_NORMAL 0   //进程正常
#define PROCESS_KILL 1     //进程应该被杀掉
#define PROCESS_WAIT 2     //父进程应该等待这个子进程
#define PROCESS_UNKNOWN -1 //

#define UI_UART "/dev/ttyS1"
#define KER_MSG_PATH "/opt/SafePrinter/data/log/ker_msg.txt"

#define UI_LED0_BIT 2    //OK键四周的LED 左上
#define UI_LED1_BIT 1    //OK键四周的LED 右上
#define UI_LED2_BIT 15   //OK键四周的LED 左下
#define UI_LED3_BIT 0    //OK键四周的LED 右下
#define UI_LED4_BIT 12   //
#define UI_LED5_BIT 11   //
#define UI_LED6_BIT 3    //
#define UI_LED3_G_BIT 9  //三色LED灯绿色
#define UI_LED3_R_BIT 10 //三色LED灯红色

#define UI_LED0_MASK ((unsigned short)(1 << UI_LED0_BIT))
#define UI_LED1_MASK ((unsigned short)(1 << UI_LED1_BIT))
#define UI_LED2_MASK ((unsigned short)(1 << UI_LED2_BIT))
#define UI_LED3_MASK ((unsigned short)(1 << UI_LED3_BIT))
#define UI_LED4_MASK ((unsigned short)(1 << UI_LED4_BIT))
#define UI_LED5_MASK ((unsigned short)(1 << UI_LED5_BIT))
#define UI_LED6_MASK ((unsigned short)(1 << UI_LED6_BIT))

#define UI_LED3_G_MASK ((unsigned short)(1 << UI_LED3_G_BIT))
#define UI_LED3_R_MASK ((unsigned short)(1 << UI_LED3_R_BIT))
#define UI_LED3_GR_MASK ((unsigned short)((1 << UI_LED3_G_BIT) | (1 << UI_LED3_R_BIT))) //三色LED灯红绿色

#define UI_LED_ALL_MASK ((unsigned short)((1 << UI_LED0_BIT) |   \
                                          (1 << UI_LED1_BIT) |   \
                                          (1 << UI_LED2_BIT) |   \
                                          (1 << UI_LED3_BIT) |   \
                                          (1 << UI_LED4_BIT) |   \
                                          (1 << UI_LED5_BIT) |   \
                                          (1 << UI_LED6_BIT) |   \
                                          (1 << UI_LED3_G_BIT) | \
                                          (1 << UI_LED3_R_BIT)))

#define TC_MSG_PORT 8999
static int udp_fd = -1;
static int ui_fd = -1;
static unsigned short led_status = 0xffff;

static pid_t __waitpid(pid_t pid, int *status, int options)
{
  pid_t ret;
  int __status;

  ret = waitpid(pid, &__status, options);

  if (status)
  {
    *status = __status;
  }

  return ret;
}

/*
取得指定进程的状态
成功返回0
失败返回-1
*/
static int pm_get_proc_status(pid_t pid, char *status)
{
  int fd, num_read;
  char *tmp;
  char path[64], sbuf[512];

  sprintf(path, "/proc/%d/status", pid);

  if ((fd = open(path, O_RDONLY, 0)) == -1)
  {
    return -1;
  }

  if ((num_read = read(fd, sbuf, sizeof(sbuf) - 1)) <= 0)
  {
    close(fd);
    return -1;
  }

  sbuf[num_read] = 0;
  close(fd);
  tmp = strstr(sbuf, "State");
  sscanf(tmp, "State:\t%c", status);

  return 0;
}

static int pm_check_proc_status(char status)
{
  int ret;

  switch (status)
  {
  case 'D': //"D (disk sleep)"
    ret = PROCESS_NORMAL;
    break;
  case 'R': //"R (running)"
    ret = PROCESS_NORMAL;
    break;
  case 'S': //"S (sleeping)"
    ret = PROCESS_NORMAL;
    break;
  case 'T': //"T (stopped)" "T (tracing stop)"
#if 0
            ret = PROCESS_KILL;
#endif
    ret = PROCESS_NORMAL;
    break;
  case 'X': //"X (dead)"
    ret = PROCESS_WAIT;
    break;
  case 'Z': //"Z (zombie)"
    ret = PROCESS_WAIT;
    break;
  default:
    ret = PROCESS_UNKNOWN;
    break;
  }

  return ret;
}

static void pm_wait_proc(pid_t pid)
{
  //struct process_desc* p;
  int status_int;
  int check_cnt = 100;
  char status_char;
  int ret;

  //p = container_of(list_pos, struct process_desc, node);

  while (check_cnt && pid)
  {
    if (pm_get_proc_status(pid, &status_char) < 0)
    {
      //p->pid = 0;
      return;
    }
    else
    {
      status_int = pm_check_proc_status(status_char);
      if (status_int == PROCESS_WAIT)
      {
        ret = __waitpid(pid, NULL, WNOHANG);
        if (ret == pid)
        { //避免存在Z进程
          //p->pid = 0;
          return;
        }
      }
    }

    usleep(50000);
    check_cnt--;
  }

  //p->pid = 0;
  //printf("waitpid err(id=%d name=%s) \n", p->pid, p->exec_name);
  printf("waitpid err(id=%d) \n", pid);

  return;
}

/*
作用,杀掉pid对应的进程
*/
static void pm_kill_proc(pid_t pid)
{
  kill(pid, SIGKILL);
  pm_wait_proc(pid);
}

static int str_is_num(char *str)
{
  int ret = 0;
  int i = 0;
  char temp;

  while (1)
  {
    temp = str[i];
    if (temp == 0)
    {
      if (i)
      {
        return 1;
      }

      return 0;
    }
    else
    {
      if ((temp < '0') || (temp > '9'))
      {
        return 0;
      }
      else
      {
        i++;
      }
    }
  }
}

static int proc_is_match(char *proc_name, char *full_path)
{
  char *ptr;
  int name_len;

  ptr = strstr(full_path, proc_name);

  //dbg_printf("%s:%d %s,%s", __func__, __LINE__, full_path, proc_name);

  if (ptr)
  {
    //dbg_printf("%s:%d %s,%d", __func__, __LINE__, ptr);

    name_len = strlen(proc_name);
    if (ptr[-1] == '/' && ptr[name_len] == 0)
    {
      //dbg_printf("%s:%d", __func__, __LINE__);
      return 1;
    }
  }

  return 0;
}

/*
杀掉名称为 proc_name 的进程
*/
static void kill_proc_with_name(char *proc_name)
{
  DIR *pdir;
  struct dirent *pentry;
  int pid = 0;
  char link_path[128];
  char real_path[512];

  ssize_t readlink(const char *pathname, char *buf, size_t bufsiz);

  pdir = opendir("/proc");
  if (pdir == NULL)
  {
    return;
  }

  while ((pentry = readdir(pdir)))
  {
    if (strcmp(pentry->d_name, ".") == 0 || strcmp(pentry->d_name, "..") == 0)
    {
      continue;
    }

    //printf("1 pentry->d_name: %s\n", pentry->d_name);

    if (pentry->d_type == DT_DIR)
    {

      //printf("2 pentry->d_name: %s\n", pentry->d_name);

      if (str_is_num(pentry->d_name))
      {
        //printf("3 pentry->d_name: %s\n", pentry->d_name);

        pid = atoi(pentry->d_name);
        /*if (pid != 847)
        {
          continue;
        }*/

        memset(real_path, 0, sizeof(real_path));
        sprintf(link_path, "/proc/%d/cmdline", pid);

        readlink(link_path, real_path, sizeof(real_path) - 1);

        //printf("4 real_path: %s\n", real_path);

        if (proc_is_match(proc_name, real_path))
        {
          printf("kill process: %d name=%s\n", pid, real_path);
          pm_kill_proc(pid);
        }
        else
        {
          FILE *fd = fopen(link_path, "r");
          if (fd != NULL)
          {
            unsigned char match_buf[1024];
            int rc = fread(match_buf, 1, 1024, fd);
            fclose(fd);

            if (rc == 0)
            {
              continue;
            }

            //printf("fd(%d:%d): %s\n", rc, strlen(match_buf), match_buf);
            //printf4s(match_buf, rc);

            for (size_t i = 0; i < rc - 1; i++)
            {
              if (match_buf[i] == 0x0)
              {
                match_buf[i] = 0x2f;
              }
            }

            //printf("2fd(%d:%d): %s\n", rc, strlen(match_buf), match_buf);

            if (proc_is_match(proc_name, match_buf))
            {
              printf("kill process: %d name=%s\n", pid, match_buf);
              pm_kill_proc(pid);
            }
          }
        }
      }
    }
  }
}

static int udp_init(void)
{
  struct sockaddr_in addr;

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(TC_MSG_PORT);
  addr.sin_addr.s_addr = inet_addr("127.0.0.1");

  if ((udp_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
  {
    return -1;
  }

  if (bind(udp_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
  {
    return -1;
  }

  return 0;
}

static unsigned char get_xor(unsigned char *data, int len)
{
  int i;
  unsigned char value = 0;

  for (i = 0; i < len; i++)
  {
    value ^= data[i];
  }

  return value;
}

/*
相应的bit清零
*/
static void set_led_on(unsigned short led_bits)
{
  unsigned short mask;
  unsigned char led_data[6];

  mask = (unsigned short)(~led_bits);

  //assert(sem_wait(&sem_led) == 0);
  led_status = led_status & mask;
  //assert(sem_wait(&sem_ui) == 0);

  led_data[0] = 0x11;
  led_data[1] = 0xAC;
  led_data[2] = 0x03;
  led_data[3] = (led_status >> 8) & 0xff;
  led_data[4] = (led_status)&0xff;
  led_data[5] = get_xor(led_data + 1, 4);
  write(ui_fd, led_data, sizeof(led_data));

  //assert(sem_post(&sem_ui) == 0);
  //assert(sem_post(&sem_led) == 0);
}

/*
相应的bit置1
*/
static void set_led_off(unsigned short led_bits)
{
  unsigned char led_data[6];

  //assert(sem_wait(&sem_led) == 0);
  led_status = led_status | led_bits;
  //assert(sem_wait(&sem_ui) == 0);

  led_data[0] = 0x11;
  led_data[1] = 0xAC;
  led_data[2] = 0x03;
  led_data[3] = (led_status >> 8) & 0xff;
  led_data[4] = (led_status)&0xff;
  led_data[5] = get_xor(led_data + 1, 4);
  write(ui_fd, led_data, sizeof(led_data));

  //assert(sem_post(&sem_ui) == 0);
  //assert(sem_post(&sem_led) == 0);
}

static void ALL_LED_ON(void)
{
  set_led_on(UI_LED_ALL_MASK);
}

static display_tpcm_fail(void)
{
  unsigned char data[36];

  data[0] = 0x11;
  data[1] = 0xA1;
  data[2] = 0x21;
  memset(data + 3, ' ', 32);

  //memcpy(data+3, msg, 32);
  //0xDE2C  固
  //0xD8D1  件
  //0xe266  度
  //0xa536  量
  //0xDFC7  失
  //0xA262  败
  data[3] = 0xde;
  data[4] = 0x2c;

  data[5] = 0xd8;
  data[6] = 0xd1;

  data[7] = 0xe2;
  data[8] = 0x66;

  data[9] = 0xa5;
  data[10] = 0x36;

  data[11] = 0xdf;
  data[12] = 0xc7;

  data[13] = 0xa2;
  data[14] = 0x62;

  data[35] = get_xor(data + 1, 34);

  write(ui_fd, data, sizeof(data));
}

/*
0=灯亮
1=灯灭
*/
static int set_gpio_value(int value)
{
  int fd;
  int len;
  int ret;
  char gpio_op[64];

  //3.set gpio38 default value to '0'
  fd = open("/sys/class/gpio/gpio38/value", O_WRONLY, 0644); //O_RDONLY
  if (fd < 0)
  {
    printf("open /sys/class/gpio/gpio38/value fail\n");
    return -1;
  }

  if (value)
  {
    len = sprintf(gpio_op, "1\n");
  }
  else
  {
    len = sprintf(gpio_op, "0\n");
  }

  ret = write(fd, gpio_op, len);
  close(fd);

  if (ret != len)
  {
    printf("set gpio38 value to 0 fail\n");
    return -1;
  }
}

static void save_ker_msg(char *msg)
{
  FILE *fp;

  fp = fopen(KER_MSG_PATH, "a");

  if (!fp)
  {
    printf("open %s fail\n", KER_MSG_PATH);
    return;
  }

  fprintf(fp, msg);
  fflush(fp);
  fclose(fp);
}

static int check_str(char *msg)
{
  if (strstr(msg, ":") == NULL)
  {
    return 1;
  }
  else if (strstr(msg, "TASK:"))
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

static int check_msg(char *msg)
{
  char msg_tmp[512];

  int ret;
  int ptr_cnt = 0;
  char *ptr = NULL;
  char *ptr_prop = NULL;
  char *last_ptr = NULL;
  char *saveptr = NULL;

  memset(msg_tmp, 0, sizeof(msg_tmp));
  sprintf(msg_tmp, "%s", msg);

  ptr = strtok_r(msg_tmp, "|", &saveptr);

  while (ptr)
  {
    ptr_cnt++;

    if (ptr_cnt == 2)
    {
      return check_str(ptr);
    }

    ptr = strtok_r(NULL, " ", &saveptr);
  }

  return 0;
}

void init_di()
{
  int export_fd = -1;
  int direction_fd = -2;

  if (access("/sys/class/gpio/gpio41/value", F_OK) != 0)
  {
    /*不存在生成gpio41管脚*/
    /*生成gpio41管脚*/
    export_fd = open("/sys/class/gpio/export", O_WRONLY);
    if (export_fd >= 0)
    {
      if (write(export_fd, "41", strlen("41")) != strlen("41"))
      {
        printf("%s:%d write file operation error, %s\n", __func__, __LINE__, strerror(errno));
      }
      close(export_fd);
    }

    /*配置成out模式*/
    direction_fd = open("/sys/class/gpio/gpio41/direction", O_WRONLY);
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

void set_di(char *value)
{
  int fd;
  int len;
  int ret;
  char gpio_op[64];

  init_di();

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
      return -1;
    }

    return 0;
  }

  return -1;
}

int tc_main(void *arg)
{
  fd_set fds;
  unsigned char buf[512];
  int ret;
  int tc_fail = 0;

  if (udp_init())
  {
    printf("%s %d fail\n", __func__, __LINE__);
    return 0;
  }

  while (1)
  {
    FD_ZERO(&fds);
    FD_SET(udp_fd, &fds);

    select(udp_fd + 1, &fds, NULL, NULL, NULL);

    if (FD_ISSET(udp_fd, &fds))
    {
      memset(buf, 0, sizeof(buf));
      ret = read(udp_fd, buf, sizeof(buf));
      if (ret > 0)
      {
        //save_ker_msg(buf);
        printf("recv msg ret=%d %s\n", ret, buf);
        //接下来解析tc的消息
        if (check_msg(buf))
        {
          tc_fail = 1;
          save_ker_msg(buf);
          if (tc_fail)
          {
            kill_proc_with_name("gb3731cdn_led");
            kill_proc_with_name("snmptrapd.conf");
            kill_proc_with_name("safeprinterd.sh");
            kill_proc_with_name("runService.sh");
            kill_proc_with_name("SafePrinterServer");

            set_di("1\n");
            sleep(2);
            set_di("0\n");
            sleep(1);
            set_di("1\n");
            sleep(2);
            set_di("0\n");
            sleep(1);
            set_di("1\n");
            sleep(2);
            set_di("0\n");
            //set_gpio_value(0);//亮灯
            //system("systemctl stop safeprinterd");
          }
        }
      }
    }
  }

  return 0;
}

int print_usage(const char *a1)
{
  printf("\run: %s\n", a1);
  printf("\nread version: %s -v\n", a1);
  return 0;
}

int main(int argc, char *argv[])
{
  switch (argc)
  {
  case 1:
  {
    return start_main(tc_main, NULL, 500);
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

/*
1.修改tc目录的相关权限
2.root用户不可登录串口或者ssh,串口不可登录任何用户
3.增加root1用户
4.删除 su sudo 等命令

*/
