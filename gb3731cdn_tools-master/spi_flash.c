#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "spi_flash.h"
#include "common.h"

char *get_spi_flash_name()
{
#ifdef __H6__
    return "/dev/mmcblk0boot1";
#elif __LX2K1000__
    return "/dev/mtdblock1";
#else
    return "";
#endif
}

int h6_system_echo_0()
{
#ifdef __H6__
    return system("echo 0 > /sys/block/mmcblk0boot1/force_ro");
#else
    return 0;
#endif
}

int h6_system_echo_1()
{
#ifdef __H6__
    return system("echo 1 > /sys/block/mmcblk0boot1/force_ro");
#else
    return 0;
#endif
}

int get_spi_flash(long __off, size_t __n, char *data)
{
    FILE *fp;

    char *spi_flash_name = get_spi_flash_name();
    if (spi_flash_name == "")
    {
        return 0;
    }

    fp = fopen(spi_flash_name, "r");
    if (!fp)
    {
        printf("can not fopen %s!!!\n", spi_flash_name);
        return -1;
    }

    if (fseek(fp, __off, SEEK_SET))
    {
        printf("can not fseek %s %d!!!\n", spi_flash_name, __off);
        fclose(fp);
        return -1;
    }

    size_t wlen = fread(data, 1, __n, fp);
    if (wlen != __n)
    {
        printf("can not fread %s %d %d %d!!!\n", spi_flash_name, __off, wlen, __n);
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return 0;
}

int set_spi_flash(long __off, size_t __n, char *data)
{
    FILE *fp;

    char *spi_flash_name = get_spi_flash_name();
    if (spi_flash_name == "")
    {
        return 0;
    }

    h6_system_echo_0();

    fp = fopen(spi_flash_name, "r+");
    if (!fp)
    {
        printf("can not fopen %s!!!\n", spi_flash_name);
        h6_system_echo_1();
        return -1;
    }

    if (fseek(fp, __off, SEEK_SET))
    {
        printf("can not fseek %s %d!!!\n", spi_flash_name, __off);
        fclose(fp);
        h6_system_echo_1();
        return -1;
    }

    size_t wlen = fwrite(data, 1, __n, fp);
    if (wlen != __n)
    {
        printf("can not fwrite %s %d %d %d!!!\n", spi_flash_name, __off, wlen, __n);
        fclose(fp);
        h6_system_echo_1();
        return -1;
    }

    fclose(fp);
    h6_system_echo_1();
    return 0;
}
