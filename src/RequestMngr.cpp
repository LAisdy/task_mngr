#include "RequestMngr.h"

RequestMngr::RequestMngr()
{

}

RequestMngr::~RequestMngr()
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