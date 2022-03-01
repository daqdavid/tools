# arm
# AR =arm-linux-gnueabihf-ar
# AS =arm-linux-gnueabihf-as
# CC =arm-linux-gnueabihf-gcc
# STRIP =arm-linux-gnueabihf-strip

# aarch64
#AR = /usr/local/gcc-linaro-5.2-2015.11-2-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-ar
#AS = /usr/local/gcc-linaro-5.2-2015.11-2-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-as
#CC = /usr/local/gcc-linaro-5.2-2015.11-2-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-gcc
#STRIP = /usr/local/gcc-linaro-5.2-2015.11-2-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-strip

# mips64el
#AR = /usr/local/gcc-4.9.3-64-gnu/bin/mips64el-linux-ar
#AS = /usr/local/gcc-4.9.3-64-gnu/bin/mips64el-linux-as
#CC = /usr/local/gcc-4.9.3-64-gnu/bin/mips64el-linux-gcc
#STRIP = /usr/local/gcc-4.9.3-64-gnu/bin/mips64el-linux-strip

# x86
#CC = /usr/bin/gcc

# x86
SPI_SOURCES = spi_flash.c
#STRIP = echo

#gcc 默认使用的是 C89 的标准，而 C89 的标准不支持在 for 中定义循环变量，而在 for 循环中需要定义循环变量的话，需要在 C99 标准中才支持，因此需要增加 -std=c99 或 -std=gun99 参数才能编译通过。
CFLAGS += -std=gnu99

#设置GCC_COLORS以发出着色诊断消息
CFLAGS += -fdiagnostics-color

#-O设置一共有五种：-O0、-O1、-O2、-O3和-Os。你只能在/etc/make.conf里面设置其中的一种。
#除了-O0以外，每一个-O设置都会多启用几个选项，请查阅gcc手册的优化选项章节，以便了解每个-O等级启用了哪些选项及它们有何作用。
#让我们来逐一考察各个优化等级：
#-O0： 这个等级（字母“O”后面跟个零）关闭所有优化选项。这样就不会优化代码，这通常不是我们想要的。
#-O1：这是最基本的优化等级。与-o等同， 编译器会在不花费太多编译时间的同时试图生成更快更小的代码。这些优化是非常基础的，但一般这些任务肯定能顺利完成。
#-O2：-O1的进阶。这是推荐的优化等级，除非你有特殊的需求。-O2会比-O1启用多一些标记。设置了-O2后，编译器会试图提高代码性能而不会增大体积和大量占用的编译时间。
#-O3：这是最高最危险的优化等级。用这个选项会延长编译代码的时间，并且在使用gcc4.x的系统里不应全局启用。自从3.x版本以来gcc的行为已经有了极大地改变。在3.x，-O3生成的代码也只是比-O2快一点点而已，而gcc4.x中还未必更快。用-O3来编译所有的软件包将产生更大体积更耗内存的二进制文件，大大增加编译失败的机会或不可预知的程序行为（包括错误）。这样做将得不偿失，记住过犹不及。在gcc 4.x.中使用-O3是不推荐的。
#-Os：这个等级用来优化代码尺寸。其中启用了-O2中不会增加磁盘空间占用的代码生成选项。这对于磁盘空间极其紧张或者CPU缓存较小的机器非常有用。但也可能产生些许问题，因此软件树中的大部分ebuild都过滤掉这个等级的优化。使用-Os是不推荐的。
CFLAGS += -O2

#可执行程序包含调试信息：加个-g 是为了gdb 用，不然gdb用不了
#CFLAGS += -g

#生成所有警告信息
CFLAGS += -Wall

#与-fexceptions类似，只是它只会生成任何所需的静态数据，但不会以任何其他方式影响生成的代码。您通常不会启用此选项；相反，需要此处理的语言处理器将代表您启用它。
CFLAGS += -funwind-tables

#GCC在编译时可以使用 -ffunction-sections 和 -fdata-sections 将每个函数或符号创建为一个sections，其中每个sections名与function或data名保持一致。而在链接阶段， -Wl,–gc-sections 指示链接器去掉不用的section（其中-wl, 表示后面的参数 -gc-sections 传递给链接器），这样就能减少最终的可执行程序的大小了。
CFLAGS += -ffunction-sections -fdata-sections

# GIT版本号
# 获取源码最近一次 git commit log，包含 commit sha 值，以及 commit message
_GIT_COMMIT_LOG=`git log --pretty=oneline -n 1`
# CFLAGS += -DGIT_COMMIT_LOG="$(_GIT_COMMIT_LOG)"

# 编译时间
_BUILD_TIME = $(shell date +"%Y-%m-%d %H:%M:%S")
CFLAGS += -DBUILD_TIME="$(_BUILD_TIME)"

net_util_src = $(SPI_SOURCES) common.c common_net.c net_util.c
net_util : $(net_util_src:.c=.o)
	$(CC) $(CFLAGS) $(LDFLAGS) $(net_util_src:.c=.o) -o $@

mac_util_src = $(SPI_SOURCES) common.c mac_util.c
mac_util : $(mac_util_src:.c=.o)
	$(CC) $(CFLAGS) $(LDFLAGS) $(mac_util_src:.c=.o) -o $@

device_util_src = $(SPI_SOURCES) common.c device_util.c
device_util : $(device_util_src:.c=.o)
	$(CC) $(CFLAGS) $(LDFLAGS) $(device_util_src:.c=.o) -o $@

fan_src = common.c fan.c
fan : $(fan_src:.c=.o)
	$(CC) $(CFLAGS) $(LDFLAGS) $(fan_src:.c=.o) -o $@

tc_fail_src = common.c tc_fail.c
tc_fail : $(tc_fail_src:.c=.o)
	$(CC) $(CFLAGS) $(LDFLAGS) $(tc_fail_src:.c=.o) -o $@

led_src = common.c led2.c led.c
led : $(led_src:.c=.o)
	$(CC) $(CFLAGS) $(LDFLAGS) $(led_src:.c=.o) -lpthread -o $@

2ping_src = common.c 2ping.c
2ping : $(2ping_src:.c=.o)
	$(CC) $(CFLAGS) $(LDFLAGS) $(2ping_src:.c=.o) -o $@

net_guard_src = $(SPI_SOURCES) common.c common_net.c net_guard.c
net_guard : $(net_guard_src:.c=.o)
	$(CC) $(CFLAGS) $(LDFLAGS) $(net_guard_src:.c=.o) -o $@

usb_test_src = common.c usb_test.c
usb_test : $(usb_test_src:.c=.o)
	$(CC) $(CFLAGS) $(LDFLAGS) $(usb_test_src:.c=.o) -o $@

clean_o:
	rm -fr *.o

all : clean net_util mac_util device_util tc_fail led fan net_guard usb_test clean_o strip_all install

strip_all:
	$(STRIP) --strip-debug net_util mac_util device_util tc_fail led fan net_guard usb_test
	$(STRIP) --strip-unneeded net_util mac_util device_util tc_fail led fan net_guard usb_test

install:
	rm -rf build
	mkdir build
	mv net_util mac_util device_util tc_fail led fan net_guard usb_test build/

clean:
	rm -fr *.o; rm -fr build net_util mac_util device_util tc_fail led fan net_guard usb_test
