#ifndef __ALGO_LOG_H__
#define __ALGO_LOG_H__

// log level 
typedef enum {
    ALGO_LOG_LEVEL_DEBUG,
    ALGO_LOG_LEVEL_INFO,
    ALGO_LOG_LEVEL_WARN,
    ALGO_LOG_LEVEL_ERROR
} AlgoLogLevel;

#ifdef  __cplusplus
extern "C" {
#endif
    void log_printf(AlgoLogLevel level, const char* fmt, ...);

#ifdef  __cplusplus
}
#endif

#define LOGD(fmt, ...) do { log_printf(ALGO_LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__);} while (0)
#define LOGI(fmt, ...) do { log_printf(ALGO_LOG_LEVEL_INFO, fmt, ##__VA_ARGS__);} while (0) 
#define LOGW(fmt, ...) do { log_printf(ALGO_LOG_LEVEL_WARN, fmt, ##__VA_ARGS__);} while (0)
#define LOGE(fmt, ...) do { log_printf(ALGO_LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__);} while (0)

#endif // !__ALGO_LOG_H__