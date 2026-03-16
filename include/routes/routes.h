#pragma once

#include <string>

//
//  GET
//

std::string HandleRoot(const std::string &jsonData);

std::string HandleLogin(const std::string &jsonData);

//
//  POST
//

std::string HandleLoginLogic(const std::string &jsonData);

std::string HandleReconcileMedication(const std::string &jsonData);

std::string HandleValidateDataQuality(const std::string &jsonData);
