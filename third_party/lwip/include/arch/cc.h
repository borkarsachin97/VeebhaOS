#ifndef __ARCH_CC_H__
#define __ARCH_CC_H__

#include <stdint.h>
#include <stddef.h>

#define BYTE_ORDER LITTLE_ENDIAN

#define LWIP_NO_UNISTD_H 1
#define LWIP_NO_CTYPE_H 1

#define U16_F "u"
#define S16_F "d"
#define X16_F "x"
#define U32_F "u"
#define S32_F "d"
#define X32_F "x"
#define SZT_F "u"

#define PACK_STRUCT_FIELD(x) x
#define PACK_STRUCT_STRUCT __attribute__((packed))
#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_END

extern uint32_t os_log_printf(const char *fmt, ...);

#define LWIP_PLATFORM_DIAG(x) do { os_log_printf x; } while(0)
#define LWIP_PLATFORM_ASSERT(x) do { os_log_printf("LWIP ASSERT: %s\n", x); } while(0)

extern uint32_t timer_get_ms(void);
#define LWIP_RAND() ((u32_t)timer_get_ms())

#endif /* __ARCH_CC_H__ */
