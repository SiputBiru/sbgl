#ifndef SBGL_INTERNAL_LOG

#define SBGL_INTERNAL_LOG

/**
 * @brief Internal logging level.
 */
typedef enum {
	SBGL_LOG_INFO,
	SBGL_LOG_WARN,
	SBGL_LOG_ERROR,
	SBGL_LOG_CRITICAL,
} sbgl_LogLevel;

/**
 * @brief Internal logging level.
 *
 * By using strings instead of formatter (printf), we keep the core logic extremely lean.
 */
void sbgl_internal_log(sbgl_LogLevel level, const char* message);

#endif // !SBGL_INTERNAL_LOG
