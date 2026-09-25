#ifndef EXTERN_H
#define EXTERN_H

#include "cs_types.h"

void *memcpy(void *dst, const void *src, UINT32 n);
void *memset(void *dst, int c, UINT32 n);
int memcmp(const void *s1, const void *s2, UINT32 n);
UINT32 strlen(const char *s);
char *strcpy(char *dst, const char *src);
int strcmp(const char *s1, const char *s2);
char *strncpy(char *dst, const char *src, UINT32 n);
int strncmp(const char *s1, const char *s2, UINT32 n);
char *strrchr(const char *s, int c);
char *strchr(const char *s, int c);

#endif




