#include "log.h"

#include <stdio.h>

void LogInfo(const char *msg) {
  printf("INFO: %s\n", msg);
}

void LogInfo(const std::string &msg) {
  printf("INFO: %s\n", msg.c_str());
}

void LogWarning(const char *msg) {
  printf("WARNING: %s\n", msg);
}

void LogWarning(const std::string &msg) {
  printf("INFO: %s\n", msg.c_str());
}

void LogError(const char *msg) {
  printf("ERROR: %s\n", msg);
}

void LogError(const std::string &msg) {
  printf("INFO: %s\n", msg.c_str());
}
