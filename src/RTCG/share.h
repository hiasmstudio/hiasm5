#ifndef _SHAREH_
#define _SHAREH_

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <stdint.h>
#include "CGTShare.h"

#define HANDLE FILE*

extern void lowerCase(char *to, const char *from);
extern void upperCase(char *to, const char *from);

extern void int_to_str(intptr_t i, char *buf);

extern int min(int x, int y);
// Package paths are UTF-8 on every host, including Windows.
extern FILE *openUtf8(const char *path, const char *mode);

#endif
