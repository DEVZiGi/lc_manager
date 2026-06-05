#include "utils.h"
#include <tlhelp32.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <objbase.h>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <iomanip>
#include <cmath>

namespace fs = std::filesystem;

DWORD GetProcessIdByName(const std::string& processName) {
    PROCESSENTRY32 processEntry;
    processEntry.dwSize = sizeof(PROCESSENTRY32);
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (Process32First(snapshot, &processEntry)) {
        do {
            if (processName == processEntry.szExeFile) {
                CloseHandle(snapshot);
                return processEntry.th32ProcessID;
            }
        } while (Process32Next(snapshot, &processEntry));
    }
    CloseHandle(snapshot);
    return 0;
}

std::string GetLocalLowPath() {
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, path))) {
        return std::string(path) + "\\AppData\\LocalLow\\zigisoftware\\lc_manager";
    }
    return "";
}

std::string GetExeDirectory() {
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    std::string fullPath(buffer);
    return fullPath.substr(0, fullPath.find_last_of("\\/"));
}

void SetupDirectories(const std::string& basePath) {
    if (basePath.empty()) return;
    std::vector<std::string> dirs = { basePath, basePath + "\\data", basePath + "\\database" };
    for (const auto& dir : dirs) {
        if (!fs::exists(dir)) fs::create_directories(dir);
    }
}

void SetupStaticDatabase(const std::string& dbFolder) {
    std::string exeDir = GetExeDirectory();
    std::string sourceDbFolder = exeDir + "\\database";
    fs::create_directories(dbFolder);
    
    if (fs::exists(sourceDbFolder) && fs::is_directory(sourceDbFolder)) {
        std::vector<std::string> jsonFiles = { "abno_db.json", "equip_db.json" };
        for (const auto& file : jsonFiles) {
            std::string sourceFile = sourceDbFolder + "\\" + file;
            std::string targetFile = dbFolder + "\\" + file;
            if (fs::exists(sourceFile)) {
                try { fs::copy(sourceFile, targetFile, fs::copy_options::overwrite_existing); }
                catch (...) {}
            }
        }
        try { fs::remove_all(sourceDbFolder); }
        catch (...) {}
    }
}

bool CreateDesktopShortcut() {
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);
    char desktopPath[MAX_PATH];
    if (FAILED(SHGetFolderPathA(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, desktopPath))) return false;
    std::string linkPath = std::string(desktopPath) + "\\LC Manager.lnk";
    HRESULT hres;
    IShellLink* psl;
    hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl);
    if (SUCCEEDED(hres)) {
        psl->SetPath(exePath);
        psl->SetDescription("LC Manager - QoL APP by zigisoftware");
        psl->SetIconLocation(exePath, 0);
        IPersistFile* ppf;
        hres = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);
        if (SUCCEEDED(hres)) {
            WCHAR wsz[MAX_PATH];
            MultiByteToWideChar(CP_ACP, 0, linkPath.c_str(), -1, wsz, MAX_PATH);
            hres = ppf->Save(wsz, TRUE);
            ppf->Release();
        }
        psl->Release();
    }
    return SUCCEEDED(hres);
}

std::string NormalizeText(const std::string& input) {
    std::string out = input;
    const char* replacements[][2] = {
        {"\xc4\x85", "a"}, {"\xc4\x87", "c"}, {"\xc4\x99", "e"}, {"\xc5\x82", "l"},
        {"\xc5\x84", "n"}, {"\xc3\xb3", "o"}, {"\xc5\x9b", "s"}, {"\xc5\xba", "z"},
        {"\xc5\xbc", "z"}, {"\xc4\x84", "A"}, {"\xc4\x86", "C"}, {"\xc4\x98", "E"},
        {"\xc5\x81", "L"}, {"\xc5\x83", "N"}, {"\xc3\x93", "O"}, {"\xc5\x9a", "S"},
        {"\xc5\xb9", "Z"}, {"\xc5\xbb", "Z"}, {"\xe2\x80\x99", "'"}, {"\xe2\x80\x98", "'"},
        {"\xe2\x80\x9c", "\""}, {"\xe2\x80\x9d", "\""}, {"\xe2\x80\x93", "-"}, {"\xe2\x80\x94", "-"}
    };
    for (int i = 0; i < 24; ++i) {
        size_t pos = 0;
        std::string from = replacements[i][0];
        std::string to = replacements[i][1];
        while ((pos = out.find(from, pos)) != std::string::npos) {
            out.replace(pos, from.length(), to);
            pos += to.length();
        }
    }
    return out;
}

std::string ToUpper(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return value;
}

bool HasText(const std::string& value) {
    return !value.empty() && value != "-" && value != "null";
}

const json* FindAny(const json& object, std::initializer_list<const char*> keys) {
    if (!object.is_object()) return nullptr;

    for (const char* key : keys) {
        auto it = object.find(key);
        if (it != object.end() && !it->is_null()) return &(*it);
    }
    return nullptr;
}

std::string JsonToString(const json* value, const std::string& fallback) {
    if (value == nullptr || value->is_null()) return fallback;

    if (value->is_string()) {
        std::string text = value->get<std::string>();
        return text.empty() ? fallback : NormalizeText(text);
    }
    if (value->is_boolean()) return value->get<bool>() ? "true" : "false";
    if (value->is_number_integer() || value->is_number_unsigned()) return std::to_string(value->get<long long>());
    if (value->is_number_float()) {
        std::ostringstream out;
        out << std::fixed << std::setprecision(1) << value->get<double>();
        return out.str();
    }

    return fallback;
}

std::string JsonToString(const json& object, std::initializer_list<const char*> keys, const std::string& fallback) {
    return JsonToString(FindAny(object, keys), fallback);
}

int JsonToInt(const json* value, int fallback) {
    if (value == nullptr || value->is_null()) return fallback;
    try {
        if (value->is_number_integer() || value->is_number_unsigned()) return value->get<int>();
        if (value->is_number_float()) return static_cast<int>(std::round(value->get<double>()));
        if (value->is_string()) return std::stoi(value->get<std::string>());
    } catch (...) {}
    return fallback;
}

int JsonToInt(const json& object, std::initializer_list<const char*> keys, int fallback) {
    return JsonToInt(FindAny(object, keys), fallback);
}

double JsonToDouble(const json* value, double fallback) {
    if (value == nullptr || value->is_null()) return fallback;
    try {
        if (value->is_number()) return value->get<double>();
        if (value->is_string()) return std::stod(value->get<std::string>());
    } catch (...) {}
    return fallback;
}

double JsonToDouble(const json& object, std::initializer_list<const char*> keys, double fallback) {
    return JsonToDouble(FindAny(object, keys), fallback);
}

bool JsonToBool(const json& object, std::initializer_list<const char*> keys, bool fallback) {
    const json* value = FindAny(object, keys);
    if (value == nullptr || value->is_null()) return fallback;
    if (value->is_boolean()) return value->get<bool>();
    std::string text = ToUpper(JsonToString(value, ""));
    if (text == "TRUE" || text == "YES" || text == "1") return true;
    if (text == "FALSE" || text == "NO" || text == "0") return false;
    return fallback;
}

bool LoadLiveData(const std::string& liveDataPath, json& out, std::string& error) {
    if (!fs::exists(liveDataPath)) {
        error = "Waiting for live_data.json";
        return false;
    }

    try {
        std::ifstream file(liveDataPath);
        out = json::parse(file, nullptr, false);
        if (out.is_discarded()) {
            error = "live_data.json is being written; retrying";
            return false;
        }

        std::string status = JsonToString(out, {"status"}, "");
        if (status == "ERROR") {
            error = JsonToString(out, {"message"}, "Pipeline error");
            return true;
        }

        error.clear();
        return true;
    } catch (const std::exception& ex) {
        error = ex.what();
        return false;
    } catch (...) {
        error = "Unknown live data read error";
        return false;
    }
}

std::string ExecHiddenCmd(const std::string& cmd) {
    HANDLE hRead, hWrite;
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    if (!CreatePipe(&hRead, &hWrite, &sa, 0)) return "";

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;
    si.wShowWindow = SW_HIDE;

    ZeroMemory(&pi, sizeof(pi));

    std::string fullCmd = "cmd.exe /c " + cmd;
    if (!CreateProcessA(NULL, (LPSTR)fullCmd.c_str(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        CloseHandle(hRead);
        CloseHandle(hWrite);
        return "";
    }

    CloseHandle(hWrite);

    char buffer[128];
    DWORD bytesRead;
    std::string result = "";
    while (ReadFile(hRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        result += buffer;
    }

    CloseHandle(hRead);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return result;
}
