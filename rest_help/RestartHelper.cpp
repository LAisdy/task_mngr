#include <windows.h>
#include <tlhelp32.h>
#include <string>
#include <shellapi.h>
#include <filesystem>
#pragma comment(linker, "/ENTRY:mainCRTStartup")

namespace fs = std::filesystem;

bool IsProcessAlive(DWORD pid) 
{
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (hProcess) 
    {
        CloseHandle(hProcess);
        return true;
    }
    return false;
}

bool KillProcess(DWORD pid) 
{
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (!hProcess) return false;

    BOOL result = TerminateProcess(hProcess, 0);
    CloseHandle(hProcess);
    return result;
}

std::wstring GetCurrentExePath() 
{
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    return path;
}

int main(int argc, char* argv[]) 
{
    if (argc < 2) 
    {
        return 1; 
    }

    DWORD targetPID;
    try 
    {
        targetPID = std::stoul(argv[1]);
    }
    catch (...) 
    {
        return 2; 
    }

    
    if (IsProcessAlive(targetPID)) 
    {
        KillProcess(targetPID);
        Sleep(100); 
    }

    wchar_t helperPath[MAX_PATH];
    GetModuleFileNameW(NULL, helperPath, MAX_PATH);
    fs::path exePath = fs::path(helperPath).parent_path() / L"TaskMgr.exe";
    SHELLEXECUTEINFOW sei = { sizeof(sei) };
    sei.lpVerb = L"runas";
    sei.lpFile = exePath.c_str();

    if (!ShellExecuteExW(&sei)) 
    {
        return GetLastError(); 
    }

    return 0;
}