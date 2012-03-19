#ifndef DSI_PRIVATE_PLATFORM_H
#define DSI_PRIVATE_PLATFORM_H


#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <limits.h>
#include <stdint.h>

#include <sys/types.h>


#define _DCMD_MISC      (dcmd_t)0x05

#define __DIOF(kind, cmd, data) ((sizeof(data)<<16) + ((kind)<<8) + (cmd) + 0x40000000)
#define __DIOT(kind, cmd, data) ((sizeof(data)<<16) + ((kind)<<8) + (cmd) + 0x80000000)
#define __DIOTF(kind, cmd, data)((sizeof(data)<<16) + ((kind)<<8) + (cmd) + 0xC0000000)
#define __DION(kind, cmd)       (((kind)<<8) + (cmd))

#ifndef NAME_MAX
#   define NAME_MAX 255
#endif

#ifdef __cplusplus
}   // extern "C"
#endif

#endif /* DSI_PRIVATE_PLATFORM_H */
