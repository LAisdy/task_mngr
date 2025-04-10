#include "RequestMngr.h"

RequestMngr::RequestMngr()
{

}

RequestMngr::~RequestMngr()
{
    if (worker_thread_.joinable()) 
    {
        worker_thread_.join();
    }
}

void RequestMngr::start_async_send()
{
}

std::string RequestMngr::gen_rid()
{
    UUID uuid;
    RPC_STATUS status = UuidCreate(&uuid);
    if (status != RPC_S_OK) {
        throw std::runtime_error("Error UuidCreate, RPC_STATUS: " + std::to_string(status));
    }
    RPC_CSTR str;
    if (UuidToStringA(&uuid, &str) != RPC_S_OK)
    {
        throw std::runtime_error("Error UuidCreate, RPC_STATUS: " + std::to_string(status));
    }

    std::string rid(reinterpret_cast<char*>(str));
    RpcStringFreeA(&str);

    return rid;
}

std::string RequestMngr::encode_string()
{
    
    return {};
}

void RequestMngr::get_process_dlls(DWORD pid)
{
    dlls.clear();
    
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (hSnapshot == INVALID_HANDLE_VALUE) 
    {
        return;
    }

    MODULEENTRY32W me;
    me.dwSize = sizeof(me);

    if (Module32FirstW(hSnapshot, &me)) {
        do 
        {
            std::wstring curPath = me.szExePath;
            std::wstring endStr_a = L".dll";
            std::wstring endStr_b = L".DLL";
            extract_filename(curPath);
            if (ends_with(curPath, endStr_a) || ends_with(curPath, endStr_b))
            {
                dlls.emplace_back(curPath);
            }
        } while (Module32NextW(hSnapshot, &me));
    }
    CloseHandle(hSnapshot);
}

void RequestMngr::extract_filename(std::wstring& path)
{
    std::filesystem::path fs_path(path);
    path = fs_path.filename().wstring();
}


bool RequestMngr::ends_with(const std::wstring& str, const std::wstring& endStr)
{
    if (str.size() < endStr.size())
        return false;
    return str.compare(str.size() - endStr.size(), endStr.size(), endStr) == 0;
}