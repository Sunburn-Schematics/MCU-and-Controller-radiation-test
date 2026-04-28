#include "terminal.h"

#include <stdio.h>
#include <string.h>

extern int _write(int file, char *ptr, int len);

static int Terminal_StrLenVolatile(const volatile char *msg)
{
  int len = 0;

  if (msg == NULL)
  {
    return 0;
  }

  while (msg[len] != '\0')
  {
    len++;
  }

  return len;
}

void Terminal_ErrorWrite(const char *msg)
{
  if (msg == NULL)
  {
    return;
  }

  _write(1, (char *)msg, (int)strlen(msg));
}

void Terminal_ErrorWriteVolatile(const volatile char *msg)
{
  if (msg == NULL)
  {
    return;
  }

  _write(1, (char *)msg, Terminal_StrLenVolatile(msg));
}

void Terminal_DebugCheckpoint(const char *tag, volatile const char **context_slot)
{
  char msg[96];
  const char *resolved_tag = (tag != NULL) ? tag : "null";

  if (context_slot != NULL)
  {
    *context_slot = resolved_tag;
  }

  snprintf(msg, sizeof(msg), "\r\nDBG:CTX:%s\r\n", resolved_tag);
  _write(1, msg, (int)strlen(msg));
}

void Terminal_Debug(const char *msg)
{
  if (msg == NULL) return;
  _write(1, (char *)msg, (int)strlen(msg));
}