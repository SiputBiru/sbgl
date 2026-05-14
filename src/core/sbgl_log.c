#include "sbgl_internal_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

static sbgl_LogConfig s_config = {
  .minLevel = SBGL_LOG_INFO,
  .categoryMask = 0xFFFFFFFF,
  .fileEnabled = false,
  .filePath = NULL,
  .maxFiles = 3,
  .maxFileSize = 10 * 1024 * 1024,
};

static FILE* s_file = NULL;

static const char* const SBGL_RESULT_STRINGS[] = {
  [0]  = "SBGL_SUCCESS",
  [1]  = "SBGL_ERROR_NULL_CONTEXT",
  [2]  = "SBGL_ERROR_INVALID_ARGUMENT",
  [3]  = "SBGL_ERROR_INITIALIZATION_FAILED",
  [4]  = "SBGL_ERROR_WINDOW_CREATION_FAILED",
  [5]  = "SBGL_ERROR_GRAPHICS_FAILED",
  [6]  = "SBGL_ERROR_OUT_OF_MEMORY",
  [7]  = "SBGL_ERROR_DEVICE_BUSY",
  [8]  = "SBGL_ERROR_INVALID_HANDLE",
  [9]  = "SBGL_ERROR_SWAPCHAIN_FAILED",
  [10] = "SBGL_ERROR_SHADER_FAILED",
  [11] = "SBGL_ERROR_PIPELINE_FAILED",
  [12] = "SBGL_ERROR_PLATFORM_FAILED",
};

#define VK_RESULT_COUNT 23
static const int32_t VK_RESULT_CODES[] = {
  0, 1, 2, 3, 4, 5,
  -1, -2, -3, -4, -5, -6, -7, -8, -9, -10, -11,
  -13, -14, -15, -16, -17, -18
};

static const char* const VK_RESULT_STRINGS[] = {
  "VK_SUCCESS",
  "VK_NOT_READY",
  "VK_TIMEOUT",
  "VK_EVENT_SET",
  "VK_EVENT_RESET",
  "VK_INCOMPLETE",
  "VK_ERROR_OUT_OF_HOST_MEMORY",
  "VK_ERROR_OUT_OF_DEVICE_MEMORY",
  "VK_ERROR_INITIALIZATION_FAILED",
  "VK_ERROR_DEVICE_LOST",
  "VK_ERROR_MEMORY_MAP_FAILED",
  "VK_ERROR_LAYER_NOT_PRESENT",
  "VK_ERROR_EXTENSION_NOT_PRESENT",
  "VK_ERROR_FEATURE_NOT_PRESENT",
  "VK_ERROR_INCOMPATIBLE_DRIVER",
  "VK_ERROR_TOO_MANY_OBJECTS",
  "VK_ERROR_FORMAT_NOT_FOUND",
  "VK_ERROR_SURFACE_LOST_KHR",
  "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR",
  "VK_ERROR_OUT_OF_DATE_KHR",
  "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR",
  "VK_ERROR_VALIDATION_FAILED_EXT",
  "VK_ERROR_INVALID_SHADER_NV",
};

static const char* level_strings[] = {
  [SBGL_LOG_DEBUG]    = "DEBUG",
  [SBGL_LOG_INFO]    = "INFO",
  [SBGL_LOG_WARN]    = "WARN",
  [SBGL_LOG_ERROR]   = "ERROR",
  [SBGL_LOG_CRITICAL] = "CRITICAL",
};

static const char* category_strings[] = {
  [SBGL_LOG_CAT_CORE]    = "CORE",
  [SBGL_LOG_CAT_PLATFORM] = "PLATFORM",
  [SBGL_LOG_CAT_GFX]     = "GFX",
  [SBGL_LOG_CAT_INPUT]   = "INPUT",
  [SBGL_LOG_CAT_VOXEL]   = "VOXEL",
};

static void rotate_logs(void) {
  if (!s_config.filePath || s_config.maxFiles == 0) return;

  char old_path[512];
  char new_path[512];

  for (int i = s_config.maxFiles - 1; i > 0; i--) {
    snprintf(old_path, sizeof(old_path), "%s.%d", s_config.filePath, i - 1);
    snprintf(new_path, sizeof(new_path), "%s.%d", s_config.filePath, i);

    rename(old_path, new_path);
  }

  if (s_file) {
    fclose(s_file);
    s_file = NULL;
  }

  rename(s_config.filePath, old_path);
}

static void open_log_file(void) {
  if (!s_config.filePath) return;

  if (s_file) {
    fclose(s_file);
    s_file = NULL;
  }

  s_file = fopen(s_config.filePath, "a");
}

void sbgl_internal_log_impl(
  sbgl_LogLevel    level,
  sbgl_LogCategory category,
  const char*      file,
  int              line,
  const char*      function,
  const char*      message
) {
  if (level < s_config.minLevel) return;
  if (!((1u << category) & s_config.categoryMask)) return;

  time_t now = time(NULL);
  struct tm* tm_info = localtime(&now);
  char time_buf[32];
  strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);

  char out[1024];
  int len = snprintf(out, sizeof(out), "[%s] [%s] [%s] %s:%d (%s): %s\n",
    time_buf,
    level_strings[level],
    category_strings[category],
    file,
    line,
    function,
    message
  );

  if (len < 0) return;

  if (s_config.fileEnabled) {
    if (!s_file) {
      open_log_file();
    }

    if (s_file) {
      if (ftell(s_file) > (long)s_config.maxFileSize) {
        rotate_logs();
        open_log_file();
      }

      if (s_file) {
        fwrite(out, 1, len, s_file);
        fflush(s_file);
      }
    }
  }

  FILE* dst = (level >= SBGL_LOG_ERROR) ? stderr : stdout;
  fwrite(out, 1, len, dst);
}

void sbgl_LogSetLevel(sbgl_LogLevel minLevel) {
  s_config.minLevel = minLevel;
}

void sbgl_LogSetOutput(const char* path) {
  if (!path) {
    s_config.fileEnabled = false;
    if (s_file) {
      fclose(s_file);
      s_file = NULL;
    }
    return;
  }

  s_config.fileEnabled = true;
  s_config.filePath = path;
  open_log_file();
}

void sbgl_LogSetFileRotation(uint32_t maxFiles, size_t maxFileSize) {
  s_config.maxFiles = maxFiles;
  s_config.maxFileSize = maxFileSize;
}

const char* sbgl_ResultToString(int result) {
  if (result >= 0 && result < (int)(sizeof(SBGL_RESULT_STRINGS) / sizeof(SBGL_RESULT_STRINGS[0]))) {
    return SBGL_RESULT_STRINGS[result];
  }
  return "SBGL_UNKNOWN_ERROR";
}

const char* sbgl_VkResultToString(int32_t vkResult) {
  for (int i = 0; i < VK_RESULT_COUNT; i++) {
    if (VK_RESULT_CODES[i] == vkResult) {
      return VK_RESULT_STRINGS[i];
    }
  }
  return "VK_UNKNOWN";
}