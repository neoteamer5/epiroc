#pragma once
#include <linux/can.h>

void init_socket(bool noBlock = false);
extern int sock;
