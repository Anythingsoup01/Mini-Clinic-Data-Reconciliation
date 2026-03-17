#pragma once

#include <string>

//
//  GET
//

std::string HandleHome(const std::string &jsonData);

std::string HandleHomeStyles(const std::string &jsonData);

std::string HandleHomeScripts(const std::string &jsonData);

std::string HandleLogin(const std::string &jsonData);

//
//  POST
//

std::string HandleLoginLogic(const std::string &jsonData);

std::string HandleReconcileMedication(const std::string &jsonData);

std::string HandleValidateDataQuality(const std::string &jsonData);
