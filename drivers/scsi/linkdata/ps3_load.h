
#ifndef _PS3_LOAD_H_
#define _PS3_LOAD_H_

#ifdef _WINDOWS

#include "ps3_def.h"

struct ps3_instance;

S32 ps3_firmware_init(struct ps3_instance *instance);

void ps3_firmware_exit(struct ps3_instance *instance);

void ps3_remove(struct ps3_instance *instance);

#endif
#endif

