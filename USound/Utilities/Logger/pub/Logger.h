#ifndef _LOGGER_H_
#define _LOGGER_H_

#ifdef __cplusplus
extern "C" {
#endif

#define LOG_TRACE(X, ...) LOG_Msg(LOG_TYPE_TRACE, X, ##__VA_ARGS__ )
#define LOG_DEBUG(X, ...) LOG_Msg(LOG_TYPE_DEBUG, X, ##__VA_ARGS__ )
#define LOG_INFO(X, ...) LOG_Msg(LOG_TYPE_INFO, X, ##__VA_ARGS__ )
#define LOG_WARN(X, ...) LOG_Msg(LOG_TYPE_WARN, X, ##__VA_ARGS__ )
#define LOG_ERR(X, ...) LOG_Msg(LOG_TYPE_ERR, X, ##__VA_ARGS__ )

typedef enum
{
  LOG_TYPE_TRACE,
  LOG_TYPE_DEBUG,
  LOG_TYPE_INFO,
  LOG_TYPE_WARN,
  LOG_TYPE_ERR
} LOG_LEVEL;

void LOG_Msg(LOG_LEVEL level, const char *msg, ...);
void LOG_SetLevel(LOG_LEVEL level);

#ifdef __cplusplus
}
#endif

#endif
