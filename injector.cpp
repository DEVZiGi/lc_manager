#include "injector.h"
#include "utils.h"
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

bool ExtractResourceToFile(const std::string& resourceName, const std::string& outputPath) {
    HRSRC hRes = FindResourceA(NULL, resourceName.c_str(), RT_RCDATA);
    if (!hRes) return false;
    HGLOBAL hMem = LoadResource(NULL, hRes);
    if (!hMem) return false;
    void* pData = LockResource(hMem);
    DWORD size = SizeofResource(NULL, hRes);
    if (!pData || size == 0) return false;

    std::ofstream file(outputPath, std::ios::binary);
    if (!file) return false;
    file.write(static_cast<const char*>(pData), size);
    return true;
}

bool InjectMono(DWORD pid) {
    std::string tempDir = GetLocalLowPath() + "\\temp_injector";
    try { fs::remove_all(tempDir); } catch (...) {}
    fs::create_directories(tempDir);
    
    std::string smiPath = tempDir + "\\smi.exe";
    std::string smiDllPath = tempDir + "\\SharpMonoInjector.dll";
    std::string sdkPath = tempDir + "\\lc_manager_sdk.dll";

    bool extracted = true;
    extracted &= ExtractResourceToFile("SMI_EXE", smiPath);
    extracted &= ExtractResourceToFile("SMI_DLL", smiDllPath);
    extracted &= ExtractResourceToFile("SDK_DLL", sdkPath);

    if (!extracted) {
        fs::remove_all(tempDir);
        return false;
    }

    std::string command = "/c \"\"" + smiPath + "\" inject -p LobotomyCorp -a \"" + sdkPath + "\" -n lc_manager_sdk -c Loader -m Init > \"" + tempDir + "\\inject_log.txt\" 2>&1\"";

    SHELLEXECUTEINFOA shExecInfo = {0};
    shExecInfo.cbSize = sizeof(SHELLEXECUTEINFOA);
    shExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
    shExecInfo.hwnd = NULL;
    shExecInfo.lpVerb = "runas";
    shExecInfo.lpFile = "cmd.exe";
    shExecInfo.lpParameters = command.c_str();
    shExecInfo.nShow = SW_HIDE;
    shExecInfo.hInstApp = NULL;

    bool success = false;
    if (ShellExecuteExA(&shExecInfo)) {
        WaitForSingleObject(shExecInfo.hProcess, 3000);
        CloseHandle(shExecInfo.hProcess);
        success = true;
    }
    
    return success;
}
