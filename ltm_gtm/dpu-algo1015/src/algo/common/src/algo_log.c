#include "algo_log.h"
#include <stdarg.h>
#include <stdio.h>

// log level control
#define ENABLE_LOG_DEBUG 1  // debug log
#define ENABLE_LOG_INFO  1  // information
#define ENABLE_LOG_WARN  1  // warnning
#define ENABLE_LOG_ERROR 1  // error

#define DumpLogToFile    0
#define LOG_FILE_NAME    "algo.log"

void log_printf(AlgoLogLevel level, const char* fmt, ...) {
    const char* level_str = NULL;

    // --- Single Switch for Level Check and String Mapping ---
    // This switch statement checks if the level is enabled AND gets the corresponding string.
    // If a level is disabled, it hits the 'if' and returns.
    // If the level is invalid, it falls through to the 'default' and returns.
    switch (level) {
        case ALGO_LOG_LEVEL_DEBUG:
            if (!ENABLE_LOG_DEBUG) return;
            level_str = "DEBUG";
            break;
        case ALGO_LOG_LEVEL_INFO:
            if (!ENABLE_LOG_INFO) return;
            level_str = "INFO";
            break;
        case ALGO_LOG_LEVEL_WARN:
            if (!ENABLE_LOG_WARN) return;
            level_str = "WARN";
            break;
        case ALGO_LOG_LEVEL_ERROR:
            if (!ENABLE_LOG_ERROR) return;
            level_str = "ERROR";
            break;
        default:
            // For any other invalid level, just exit.
            return;
    }

    va_list args;
    va_start(args, fmt);

    // --- Output Logic ---
    if (DumpLogToFile) {
        // Static file pointer, initialized to NULL only once.
        static FILE* log_file = NULL;

        // If the file hasn't been opened yet, open it.
        if (log_file == NULL) {
            log_file = fopen(LOG_FILE_NAME, "w");
            if (log_file == NULL) { 
                va_end(args); // Clean up va_list before returning
                return; 
            }
        }
        
        fprintf(log_file, "[%s] ", level_str);
        vfprintf(log_file, fmt, args);
        fflush(log_file); // Flush to ensure data is written immediately.
    } else {
        // Console output
        printf("[%s] ", level_str);
        vprintf(fmt, args);
    }
    
    va_end(args);
}
