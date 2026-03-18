#pragma once


struct _Config {
  std::string ApiKey;
  int32_t Port;
};

_Config LoadConfig();
