#pragma once

#include <windows.h>
#include <string>

bool ExtractResourceToFile(const std::string& resourceName, const std::string& outputPath);
bool InjectMono(DWORD pid);
