#ifndef SBGL_INTERNAL_LOG
#define SBGL_INTERNAL_LOG

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/**
 * @brief Logging severity levels.
 */
typedef enum {
  SBGL_LOG_DEBUG = 0,
  SBGL_LOG_INFO = 1,
  SBGL_LOG_WARN = 2,
  SBGL_LOG_ERROR = 3,
  SBGL_LOG_CRITICAL = 4,
} sbgl_LogLevel;

/**
 * @brief Logging categories for component filtering.
 */
typedef enum {
  SBGL_LOG_CAT_CORE = 0,
  SBGL_LOG_CAT_PLATFORM = 1,
  SBGL_LOG_CAT_GFX = 2,
  SBGL_LOG_CAT_INPUT = 3,
  SBGL_LOG_CAT_VOXEL = 4,
  SBGL_LOG_CAT_COUNT = 5,
} sbgl_LogCategory;

/**
 * @brief Logging configuration.
 */
typedef struct {
  sbgl_LogLevel minLevel;
  uint32_t      categoryMask;
  bool          fileEnabled;
  const char*   filePath;
  uint32_t      maxFiles;
  size_t        maxFileSize;
} sbgl_LogConfig;

void sbgl_internal_log_impl(
  sbgl_LogLevel    level,
  sbgl_LogCategory category,
  const char*      file,
  int              line,
  const char*      function,
  const char*      message
);

#define sbgl_log_impl(level, category, msg) \
    sbgl_internal_log_impl(level, category, __FILE__, __LINE__, __func__, msg)

void sbgl_LogSetLevel(sbgl_LogLevel minLevel);
void sbgl_LogSetOutput(const char* path);
void sbgl_LogSetFileRotation(uint32_t maxFiles, size_t maxSize);
const char* sbgl_ResultToString(int result);
const char* sbgl_VkResultToString(int32_t vkResult);

#endif // !SBGL_INTERNAL_LOG
