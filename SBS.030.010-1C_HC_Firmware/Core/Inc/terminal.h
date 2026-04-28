#ifndef __TERMINAL_H__
#define __TERMINAL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void Terminal_DebugCheckpoint(const char *tag, volatile const char **context_slot);
void Terminal_ErrorWrite(const char *msg);
void Terminal_ErrorWriteVolatile(const volatile char *msg);
void Terminal_Debug(const char *msg);

#ifdef __cplusplus
}
#endif

#endif /* __TERMINAL_H__ */
