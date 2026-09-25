/*
 * Freestanding Standard C & GCC Runtime Support Functions
 */

#include "cs_types.h"
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include "kernel/freertos/include/FreeRTOS.h"
#include "kernel/freertos/include/task.h"

void *memcpy(void *dst, const void *src, UINT32 n)
{
    UINT8 *d = (UINT8 *)dst;
    const UINT8 *s = (const UINT8 *)src;
    while (n--) *d++ = *s++;
    return dst;
}

void *memmove(void *dst, const void *src, UINT32 n)
{
    UINT8 *d = (UINT8 *)dst;
    const UINT8 *s = (const UINT8 *)src;
    if (d == s || n == 0) return dst;
    if (d < s) {
        while (n--) *d++ = *s++;
    } else {
        d += n;
        s += n;
        while (n--) *--d = *--s;
    }
    return dst;
}

void *memset(void *dst, int c, UINT32 n)
{
    UINT8 *d = (UINT8 *)dst;
    while (n--) *d++ = (UINT8)c;
    return dst;
}

int memcmp(const void *s1, const void *s2, UINT32 n)
{
    const UINT8 *p1 = (const UINT8 *)s1;
    const UINT8 *p2 = (const UINT8 *)s2;
    while (n--) {
        if (*p1 != *p2) return *p1 - *p2;
        p1++; p2++;
    }
    return 0;
}

UINT32 strlen(const char *s)
{
    UINT32 len = 0;
    while (s && *s++) len++;
    return len;
}

char *strcpy(char *dst, const char *src)
{
    char *ret = dst;
    while ((*dst++ = *src++));
    return ret;
}

int strcmp(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2)) {
        s1++; s2++;
    }
    return *(const UINT8 *)s1 - *(const UINT8 *)s2;
}

char *strncpy(char *dst, const char *src, UINT32 n)
{
    char *ret = dst;
    while (n && (*dst++ = *src++)) n--;
    while (n--) *dst++ = '\0';
    return ret;
}

int strncmp(const char *s1, const char *s2, UINT32 n)
{
    while (n && *s1 && (*s1 == *s2)) {
        s1++; s2++; n--;
    }
    if (n == 0) return 0;
    return *(const UINT8 *)s1 - *(const UINT8 *)s2;
}

char *strrchr(const char *s, int c)
{
    const char *last = 0;
    do {
        if (*s == (char)c) last = s;
    } while (*s++);
    return (char *)last;
}

char *strchr(const char *s, int c)
{
    while (*s != (char)c) {
        if (!*s++) return 0;
    }
    return (char *)s;
}

int __clzsi2(uint32_t val) {
    if (val == 0) return 32;
    int n = 0;
    if ((val & 0xFFFF0000) == 0) { n += 16; val <<= 16; }
    if ((val & 0xFF000000) == 0) { n += 8;  val <<= 8;  }
    if ((val & 0xF0000000) == 0) { n += 4;  val <<= 4;  }
    if ((val & 0xC0000000) == 0) { n += 2;  val <<= 2;  }
    if ((val & 0x80000000) == 0) { n += 1; }
    return n;
}

int __ffssi2(int val) {
    if (val == 0) return 0;
    uint32_t uval = (uint32_t)val;
    int r = 1;
    if ((uval & 0xFFFF) == 0) { r += 16; uval >>= 16; }
    if ((uval & 0xFF) == 0)   { r += 8;  uval >>= 8;  }
    if ((uval & 0xF) == 0)    { r += 4;  uval >>= 4;  }
    if ((uval & 0x3) == 0)    { r += 2;  uval >>= 2;  }
    if ((uval & 0x1) == 0)    { r += 1; }
    return r;
}

uint64_t __udivmoddi4(uint64_t num, uint64_t den, uint64_t *rem_p) {
    uint64_t quot = 0;
    uint64_t qbit = 1;

    if (den == 0) {
        if (rem_p) *rem_p = 0;
        return 0;
    }

    while ((int64_t)den >= 0 && den < num) {
        den <<= 1;
        qbit <<= 1;
    }

    while (qbit) {
        if (num >= den) {
            num -= den;
            quot |= qbit;
        }
        den >>= 1;
        qbit >>= 1;
    }

    if (rem_p) *rem_p = num;
    return quot;
}

uint64_t __udivdi3(uint64_t num, uint64_t den) {
    return __udivmoddi4(num, den, NULL);
}

uint64_t __umoddi3(uint64_t num, uint64_t den) {
    uint64_t rem;
    __udivmoddi4(num, den, &rem);
    return rem;
}

int64_t __divdi3(int64_t num, int64_t den) {
    int minus = 0;
    if (num < 0) { num = -num; minus = !minus; }
    if (den < 0) { den = -den; minus = !minus; }
    uint64_t res = __udivmoddi4((uint64_t)num, (uint64_t)den, NULL);
    return minus ? -(int64_t)res : (int64_t)res;
}

int64_t __moddi3(int64_t num, int64_t den) {
    int minus = 0;
    if (num < 0) { num = -num; minus = 1; }
    if (den < 0) { den = -den; }
    uint64_t rem;
    __udivmoddi4((uint64_t)num, (uint64_t)den, &rem);
    return minus ? -(int64_t)rem : (int64_t)rem;
}

static inline int _tolower_c(int c) {
    return (c >= 'A' && c <= 'Z') ? (c + ('a' - 'A')) : c;
}

int strcasecmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2 || _tolower_c((unsigned char)*s1) == _tolower_c((unsigned char)*s2))) {
        s1++;
        s2++;
    }
    return _tolower_c((unsigned char)*s1) - _tolower_c((unsigned char)*s2);
}

int strncasecmp(const char *s1, const char *s2, UINT32 n) {
    while (n && *s1 && (*s1 == *s2 || _tolower_c((unsigned char)*s1) == _tolower_c((unsigned char)*s2))) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return _tolower_c((unsigned char)*s1) - _tolower_c((unsigned char)*s2);
}

char *strstr(const char *haystack, const char *needle) {
    if (!*needle) return (char *)haystack;
    for (; *haystack; haystack++) {
        if (*haystack == *needle) {
            const char *h = haystack;
            const char *n = needle;
            while (*h && *n && *h == *n) {
                h++;
                n++;
            }
            if (!*n) return (char *)haystack;
        }
    }
    return NULL;
}

int abs(int n) {
    return (n < 0) ? -n : n;
}

long labs(long n) {
    return (n < 0) ? -n : n;
}

static unsigned long s_next_rand = 12345;

int rand(void) {
    s_next_rand = s_next_rand * 1103515245UL + 12345UL;
    return (int)((s_next_rand >> 16) & 0x7FFF);
}

void srand(unsigned int seed) {
    s_next_rand = seed;
}

char *strcat(char *dest, const char *src) {
    char *rdest = dest;
    while (*dest) dest++;
    while ((*dest++ = *src++));
    return rdest;
}

char *strncat(char *dest, const char *src, UINT32 n) {
    char *rdest = dest;
    while (*dest) dest++;
    while (n && (*dest++ = *src++)) n--;
    if (n == 0) *dest = '\0';
    return rdest;
}

char *strdup(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char *d = (char *)pvPortMalloc(len);
    if (d) memcpy(d, s, len);
    return d;
}

/* FreeRTOS Memory Allocation Bridge */
void *malloc(size_t size) {
    return pvPortMalloc(size);
}

void free(void *ptr) {
    if (ptr) vPortFree(ptr);
}

void *calloc(size_t nmemb, size_t size) {
    size_t total = nmemb * size;
    void *p = pvPortMalloc(total);
    if (p) {
        memset(p, 0, total);
    }
    return p;
}

void *realloc(void *ptr, size_t size) {
    if (!ptr) return pvPortMalloc(size);
    if (size == 0) {
        vPortFree(ptr);
        return NULL;
    }
    void *new_p = pvPortMalloc(size);
    if (new_p) {
        memcpy(new_p, ptr, size);
        vPortFree(ptr);
    }
    return new_p;
}

/* Formatted I/O Bridge */
extern int lv_vsnprintf(char *buffer, size_t count, const char *format, va_list va);

int snprintf(char *str, size_t size, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    int ret = lv_vsnprintf(str, size, format, ap);
    va_end(ap);
    return ret;
}

int vsnprintf(char *str, size_t size, const char *format, va_list ap) {
    return lv_vsnprintf(str, size, format, ap);
}

int sprintf(char *str, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    int ret = lv_vsnprintf(str, 1024, format, ap);
    va_end(ap);
    return ret;
}

int printf(const char *format, ...) {
    (void)format;
    return 0;
}

int puts(const char *s) {
    (void)s;
    return 0;
}

#undef putchar
int putchar(int c) {
    return c;
}

/* Freestanding Standard File I/O Stubs */
int __swbuf_r(struct _reent *r, int c, FILE *fp) {
    (void)r;
    (void)fp;
    return c;
}

FILE *fopen(const char *path, const char *mode) {
    (void)path;
    (void)mode;
    return NULL;
}

int fclose(FILE *fp) {
    (void)fp;
    return 0;
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    (void)ptr; (void)size; (void)nmemb; (void)stream;
    return 0;
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
    (void)ptr; (void)size; (void)nmemb; (void)stream;
    return nmemb;
}

int fseek(FILE *stream, long offset, int whence) {
    (void)stream; (void)offset; (void)whence;
    return 0;
}

long ftell(FILE *stream) {
    (void)stream;
    return 0;
}

#undef fputc
int fputc(int c, FILE *stream) {
    (void)stream;
    return c;
}

int fputs(const char *s, FILE *stream) {
    (void)s; (void)stream;
    return 0;
}

int fflush(FILE *stream) {
    (void)stream;
    return 0;
}

#undef ferror
int ferror(FILE *stream) {
    (void)stream;
    return 0;
}

#undef feof
int feof(FILE *stream) {
    (void)stream;
    return 1;
}

char *fgets(char *s, int size, FILE *stream) {
    (void)s; (void)size; (void)stream;
    return NULL;
}

int remove(const char *pathname) {
    (void)pathname;
    return 0;
}

char *strcasestr(const char *haystack, const char *needle) {
    if (!*needle) return (char *)haystack;
    for (; *haystack; haystack++) {
        if (_tolower_c((unsigned char)*haystack) == _tolower_c((unsigned char)*needle)) {
            const char *h = haystack;
            const char *n = needle;
            while (*h && *n && _tolower_c((unsigned char)*h) == _tolower_c((unsigned char)*n)) {
                h++;
                n++;
            }
            if (!*n) return (char *)haystack;
        }
    }
    return NULL;
}

int mkdir(const char *pathname, unsigned int mode) {
    (void)pathname;
    (void)mode;
    return 0;
}


int mini_snprintf(char *buf, UINT32 size, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int ret = lv_vsnprintf(buf, size, fmt, ap);
    va_end(ap);
    return ret;
}

void hal_AifDmaIrqHandler(void) {
}


static const char s_ctype_table[1 + 256] = {
    0,
    040,040,040,040,040,040,040,040,040,050,050,050,050,050,040,040,
    040,040,040,040,040,040,040,040,040,040,040,040,040,040,040,040,
    0210,020,020,020,020,020,020,020,020,020,020,020,020,020,020,020,
    0104,0104,0104,0104,0104,0104,0104,0104,0104,0104,
    020,020,020,020,020,020,020,
    0101,0101,0101,0101,0101,0101,
    01,01,01,01,01,01,01,01,01,01,01,01,01,01,01,01,01,01,01,01,
    020,020,020,020,020,020,
    0102,0102,0102,0102,0102,0102,
    02,02,02,02,02,02,02,02,02,02,02,02,02,02,02,02,02,02,02,02,
    020,020,020,020,
    040,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};
const char *__ctype_ptr__ = s_ctype_table + 1;

struct _reent _impure_data;
struct _reent *_impure_ptr = &_impure_data;

int fprintf(FILE *stream, const char *format, ...) {
    (void)stream;
    (void)format;
    return 0;
}

FILE *popen(const char *command, const char *type) {
    (void)command;
    (void)type;
    return NULL;
}

int pclose(FILE *stream) {
    (void)stream;
    return 0;
}




