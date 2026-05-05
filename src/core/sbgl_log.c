#include "sbgl_internal_log.h"
#include <stdio.h>

void sbgl_internal_log(sbgl_LogLevel level, const char* message) {
	switch (level) {
	case SBGL_LOG_INFO:
		printf("[SBGL INFO]: %s\n", message);
		break;
	case SBGL_LOG_WARN:
		fprintf(stderr, "[SBGL WARN]: %s\n", message);
		break;
	case SBGL_LOG_ERROR:
		fprintf(stderr, "[SBGL ERROR]: %s\n", message);
		break;
	case SBGL_LOG_CRITICAL:
		fprintf(stderr, "[SBGL CRITICAL]: %s\n", message);
		break;
	}
}
