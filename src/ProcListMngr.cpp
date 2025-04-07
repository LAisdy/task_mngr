#include "ProcListMngr.h"

ProcListMngr::ProcListMngr()
    :count(0)
{
}

ProcListMngr::~ProcListMngr()
{
}

std::pair<DWORD,std::wstring> ProcListMngr::get(size_t ind)
{
    return processes[ind];
}

void ProcListMngr::getProcList()
{
    processes.clear();

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
        return;
    }
    PROCESSENTRY32W pe32 = { sizeof(pe32) };

    if (!Process32FirstW(hSnapshot, &pe32))
    {
        CloseHandle(hSnapshot);
        return;
    }

    do
    {
        processes.emplace_back(pe32.th32ProcessID, pe32.szExeFile);
    } while (Process32NextW(hSnapshot, &pe32));

    CloseHandle(hSnapshot);
}

void ProcListMngr::update()
{
    getProcList();
    count = processes.size();
}

std::vector<std::pair<DWORD, std::wstring>> ProcListMngr::processes;