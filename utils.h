#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <windows.h>

using json = nlohmann::json;

DWORD GetProcessIdByName(const std::string& processName);
std::string GetLocalLowPath();
std::string GetExeDirectory();
void SetupDirectories(const std::string& basePath);
void SetupStaticDatabase(const std::string& dbFolder);
bool CreateDesktopShortcut();

std::string ExecHiddenCmd(const std::string& cmd);

std::string NormalizeText(const std::string& input);
std::string ToUpper(std::string value);
bool HasText(const std::string& value);

const json* FindAny(const json& object, std::initializer_list<const char*> keys);
std::string JsonToString(const json* value, const std::string& fallback = "-");
std::string JsonToString(const json& object, std::initializer_list<const char*> keys, const std::string& fallback = "-");
int JsonToInt(const json* value, int fallback = 0);
int JsonToInt(const json& object, std::initializer_list<const char*> keys, int fallback = 0);
double JsonToDouble(const json* value, double fallback = 0.0);
double JsonToDouble(const json& object, std::initializer_list<const char*> keys, double fallback = 0.0);
bool JsonToBool(const json& object, std::initializer_list<const char*> keys, bool fallback = false);
bool LoadLiveData(const std::string& liveDataPath, json& out, std::string& error);
