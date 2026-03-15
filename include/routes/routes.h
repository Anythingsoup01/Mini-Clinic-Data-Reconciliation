#pragma once

#include <string>

std::string HandleRoot(const std::string &jsonData);

std::string HandleLogin(const std::string &jsonData);

std::string HandleDebugPage(const std::string &jsonData);


std::string HandleLoginLogic(const std::string &jsonData);

std::string HandleReconcileMedication(const std::string &jsonData);

std::string HandleValidateDataQuality(const std::string &jsonData);
