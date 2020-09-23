#include <stdio.h>
#include <stdarg.h>
#include "../pub/Logger.h"

uint32_t log_level = LOG_TYPE_INFO;

static const char *level_names[] = {
    "TRACE: ",
    "DEBUG: ",
    "INFO: ",
    "WARN: ",
    "ERR: "
};

void LOG_Msg(LOG_LEVEL level, const char *msg, ...)
{
  /*if (log_level <= level) {
   va_list args;

   printf(level_names[log_level]);

   va_start(args, msg);
   vprintf(msg, args);
   va_end(args);
   }*/
}

void LOG_SetLevel(LOG_LEVEL level)
{
  if (level <= LOG_TYPE_ERR)
  {
    log_level = level;
  }
  else
  {
    log_level = LOG_TYPE_ERR;
  }
}
