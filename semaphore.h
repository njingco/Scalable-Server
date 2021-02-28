#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include "constants.h"
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/types.h>

int initsem(key_t key);
void P(int sid);
void V(int sid);

#endif