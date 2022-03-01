#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "spi_flash.h"
#include "common.h"

int print_usage(const char *a1)
{
    printf("\nread version: %s -v\n", a1);

    printf("set uid(max len:32): %s uid GB3031dn\n", a1);
    printf("read uid: %s uid\n\n", a1);

    printf("set model(max len:64): %s model GB3031dn\n", a1);
    printf("read model: %s model\n\n", a1);

    printf("set usb name: %s usb LANXUM GB3031dn\n", a1);
    printf("read usb name: %s usb\n\n", a1);

    return 0;
}

int check_data(char *data, int len)
{
    for (int i = 0; i < len; i++)
    {
        if ((data[i] != 0) && (data[i] != 0xff))
        {
            return 1;
        }
    }

    return 0;
}

int main(int argc, char *argv[])
{
    switch (argc)
    {
    case 2:
    {
        if (!memcmp((const char *)(argv[1]), "-v", strlen("-v")))
        {
            return print_version2();
        }

        const char *argv1 = (const char *)(argv[1]);

        if (!memcmp(argv1, "uid", strlen("uid")))
        {
            char data[SPI_FLASH_UID_LEN] = {0};
            int ret = get_spi_flash(SPI_FLASH_UID_OFFSET, SPI_FLASH_UID_LEN, data);
            if (ret >= 0)
            {
                if (check_data(data, SPI_FLASH_UID_LEN))
                {
                    printf("%s\n", data);
                }
            }
            return ret;
        }

        if (!memcmp(argv1, "model", strlen("model")))
        {
            char data[SPI_FLASH_MODEL_NAME_LEN] = {0};
            int ret = get_spi_flash(SPI_FLASH_MODEL_NAME_OFFSET, SPI_FLASH_MODEL_NAME_LEN, data);
            if (ret >= 0)
            {
                if (check_data(data, SPI_FLASH_MODEL_NAME_LEN))
                {
                    printf("%s\n", data);
                }
            }
            return ret;
        }

        if (!memcmp(argv1, "usb", strlen("usb")))
        {
            char data[SPI_FLASH_USB_NAME_LEN] = {0};
            int ret = get_spi_flash(SPI_FLASH_USB_NAME_OFFSET, SPI_FLASH_USB_NAME_LEN, data);
            if (ret >= 0)
            {
                if (check_data(data, SPI_FLASH_USB_NAME_LEN))
                {
                    printf("%s\n", data);
                }
            }
            return ret;
        }

        break;
    }

    case 3:
    {
        const char *argv1 = (const char *)(argv[1]);
        const char *argv2 = (const char *)(argv[2]);

        int len = strlen(argv2);
        if (len && check_data(argv2, len))
        {

            if (!memcmp(argv1, "uid", strlen("uid")))
            {
                char data[SPI_FLASH_UID_LEN] = {0};
                strncpy(data, argv2, len <= SPI_FLASH_UID_LEN ? len : SPI_FLASH_UID_LEN);
                return set_spi_flash(SPI_FLASH_UID_OFFSET, SPI_FLASH_UID_LEN, data);
            }

            if (!memcmp(argv1, "model", strlen("model")))
            {
                char data[SPI_FLASH_MODEL_NAME_LEN] = {0};
                strncpy(data, argv2, len <= SPI_FLASH_MODEL_NAME_LEN ? len : SPI_FLASH_MODEL_NAME_LEN);
                return set_spi_flash(SPI_FLASH_MODEL_NAME_OFFSET, SPI_FLASH_MODEL_NAME_LEN, data);
            }

            if (!memcmp(argv1, "usb", strlen("usb")))
            {
                char data[SPI_FLASH_USB_NAME_LEN] = {0};
                strncpy(data, argv2, len <= SPI_FLASH_USB_NAME_LEN ? len : SPI_FLASH_USB_NAME_LEN);
                return set_spi_flash(SPI_FLASH_USB_NAME_OFFSET, SPI_FLASH_USB_NAME_LEN, data);
            }
        }

        break;
    }
    }

    return print_usage(*(const char **)argv);
}
