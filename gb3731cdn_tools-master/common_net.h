#include <stdlib.h>
#include <stdio.h>

int get_net(char *ip, char *nm, char *gw);

int set_iface(char *if_name, int save, char *ip, char *nm, char *gw);

int set_if_addr(char *if_name, char *if_addr, int cmd);

int sys_set_route(char *destnet, char *nm, char *gw, int cmd);

int save_net(char *ip, char *nm, char *gw);
