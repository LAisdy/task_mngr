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

void RequestMngr::start_async_send(DWORD pid)
{
    /*
    * get dll loaded by proc: 
    * get_process_dlls(pid) - this will store the dll seq as a string into json_dlls
    * 
    * encrypt(json_dlls, key)
    * 
    * create json body for msg: {"cmd:1", "rid:<some_random_val>","data: <encrypted_data>"}
    * send using curl 
    * TODO: implement curl functions 
    */
}

void RequestMngr::show_warning(std::wstring msg, std::wstring name )
{
    MessageBox(
        NULL,                           
        msg.c_str(),
        name.c_str(),
        MB_OK | MB_ICONWARNING          
    );
}

int RequestMngr::init_key()
{
    std::string keyFile = "protected_key.bin";
    std::vector<BYTE> protectedKey;
    
    if (!load_protected_key(keyFile, protectedKey)) 
    {
        key = gen_key();
        protectedKey = hide_key_DPAPI(key);
        if (!save_protected_key(keyFile, protectedKey))
        {
            show_warning(L"Failed to save key", L"Warning");
            return 1;
        }
    }
    else 
    {
        key = reveal_key_DPAPI(protectedKey);
        if (key.size() != crypto_secretbox_KEYBYTES) {
            show_warning(L"Incorrect key length after unpacking", L"Warning");
            return 1;
        }
    }
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

    nlohmann::json j = dlls;
    json_dlls = j.dump();

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


std::vector<BYTE> RequestMngr::hide_key_DPAPI(const std::vector<BYTE>& key)
{
    DATA_BLOB inBlob, outBlob;
    inBlob.pbData = const_cast<BYTE*>(key.data());
    inBlob.cbData = static_cast<DWORD>(key.size());

    if (!CryptProtectData(&inBlob, L"Libsodium Key", nullptr, nullptr, nullptr, 0, &outBlob))
        throw std::runtime_error("CryptProtectData failed");

    std::vector<BYTE> protectedData(outBlob.pbData, outBlob.pbData + outBlob.cbData);
    LocalFree(outBlob.pbData);
    return protectedData;
}

std::vector<BYTE> RequestMngr::reveal_key_DPAPI(const std::vector<BYTE>& protectedData)
{
    DATA_BLOB inBlob, outBlob;
    inBlob.pbData = const_cast<BYTE*>(protectedData.data());
    inBlob.cbData = static_cast<DWORD>(protectedData.size());

    if (!CryptUnprotectData(&inBlob, nullptr, nullptr, nullptr, nullptr, 0, &outBlob))
        throw std::runtime_error("CryptUnprotectData failed");

    std::vector<BYTE> key(outBlob.pbData, outBlob.pbData + outBlob.cbData);
    LocalFree(outBlob.pbData);
    return key;
}

bool RequestMngr::save_protected_key(const std::string& filePath, const std::vector<BYTE>& protectedKey)
{
    std::ofstream ofs(filePath, std::ios::binary);
    if (!ofs)
    {
        return false;
    }
    ofs.write(reinterpret_cast<const char*>(protectedKey.data()), protectedKey.size());
    return ofs.good();
}

bool RequestMngr::load_protected_key(const std::string& filePath, std::vector<BYTE>& protectedKey)
{
    std::ifstream ifs(filePath, std::ios::binary);
    if (!ifs)
        return false;
    protectedKey = std::vector<BYTE>((std::istreambuf_iterator<char>(ifs)),
        std::istreambuf_iterator<char>());
    return !protectedKey.empty();
}

std::vector<BYTE> RequestMngr::gen_key()
{
    std::vector<BYTE> key(crypto_secretbox_KEYBYTES);
    randombytes_buf(key.data(), key.size());
    return key;
}

std::vector<unsigned char> RequestMngr::encrypt(const std::string& message, const std::vector<unsigned char>& key)
{
    if (key.size() != crypto_secretbox_KEYBYTES)
        throw std::runtime_error("Incorrect key length");

    std::vector<unsigned char> nonce(crypto_secretbox_NONCEBYTES);
    randombytes_buf(nonce.data(), nonce.size());

    std::vector<unsigned char> ciphertext(crypto_secretbox_MACBYTES + message.size());
    if (crypto_secretbox_easy(ciphertext.data(),
        reinterpret_cast<const unsigned char*>(message.data()),
        message.size(),
        nonce.data(),
        key.data()) != 0)
    {
        throw std::runtime_error("crypto_secretbox_easy failed");
    }

    nonce.insert(nonce.end(), ciphertext.begin(), ciphertext.end());
    return nonce;
}

std::string RequestMngr::decrypt(const std::vector<unsigned char>& encrypted, const std::vector<unsigned char>& key)
{
    if (encrypted.size() < crypto_secretbox_NONCEBYTES + crypto_secretbox_MACBYTES)
        throw std::runtime_error("Incorrect length of encrypted data");

    std::vector<unsigned char> nonce(encrypted.begin(), encrypted.begin() + crypto_secretbox_NONCEBYTES);
    
    std::vector<unsigned char> ciphertext(encrypted.begin() + crypto_secretbox_NONCEBYTES, encrypted.end());

    std::vector<unsigned char> decrypted(ciphertext.size() - crypto_secretbox_MACBYTES);
    if (crypto_secretbox_open_easy(decrypted.data(),
        ciphertext.data(),
        ciphertext.size(),
        nonce.data(),
        key.data()) != 0)
    {
        throw std::runtime_error("crypto_secretbox_open_easy failed");
    }
    return std::string(decrypted.begin(), decrypted.end());
}