#include "RequestMngr.h"

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    
    std::string data(static_cast<char*>(contents), size * nmemb);
    
    std::ofstream log("curl.log", std::ios::app);
    log << "\nReceived chunk: " << size * nmemb << " bytes\n";
    

    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

RequestMngr::RequestMngr()
{
    if (sodium_init() < 0)
    {
        show_warning(L"Crypto library init failed", L"Error");
        return;
    }
}

RequestMngr::~RequestMngr()
{
}

const std::string& RequestMngr::get_last_rid() const
{
    return last_rid_;
}


void RequestMngr::start_send(DWORD pid)
{
    get_process_dlls(pid);

    if (dlls.empty())
    {
        show_warning(L"DLL list is empty.", L"Error");
        return;
    }

    std::vector<unsigned char> encrypted(crypto_secretbox_NONCEBYTES + json_dlls.size() + crypto_secretbox_MACBYTES);
    std::vector<unsigned char> nonce(crypto_secretbox_NONCEBYTES);
    std::vector<unsigned char> b64_data(encrypted.size() * 2);

    randombytes_buf(nonce.data(), nonce.size());

    encrypted = encrypt(json_dlls.data(), key);

    if (encrypted.empty())
    {
        show_warning(L"Data was not encrypted correctly", L"Error");
        return;
    }

    to_base64(b64_data, encrypted);
    size_t actual_length = strlen(reinterpret_cast<char*>(b64_data.data()));
    std::string b64_str(reinterpret_cast<char*>(b64_data.data()), actual_length);

    last_rid_ = gen_rid();
    nlohmann::json request =
    {
        {"cmd", 1},
        {"rid", last_rid_},
        {"data", b64_str}
    };
    std::string json_str = request.dump();
    
    //#### debug info ###
    std::ofstream log("curl_debug.log", std::ios::app);
    log << "Dlls was added to list and encrypted & encoded as a single string.Starting send.\n";
    log.close();
    //#### debug info ###

    CURL* curl = curl_easy_init();
    if (!curl)
    {
        log << "Curl is not initialized\n";
        show_warning(L"Failed to init CURL", L"Error");
        return;
    }

    std::string response;
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    FILE* fp = fopen("curl_info.log", "w");

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_URL, "http://172.245.127.93/p/applicants.php");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_HEADER, 0L);
    curl_easy_setopt(curl, CURLOPT_FRESH_CONNECT, 1L);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(curl, CURLOPT_STDERR, fp);

    CURLcode res = curl_easy_perform(curl);

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    std::ofstream log_send("curl.log", std::ios::app);
    log_send << "Sent data: " << json_str << "\nSent data size: " << json_str.size() << "\nRequest completed. HTTP: " << http_code
        << ", Received: " << response.size() << " bytes\n" << "The response: " << response << "\n\n";



    if (res != CURLE_OK)
    {
        last_error_ = "[CURL] " + std::string(curl_easy_strerror(res));
        show_warning(std::wstring(last_error_.begin(), last_error_.end()), L"Error");
    }
    else
    {
        try
        {
            auto json_response = nlohmann::json::parse(response);
            if (!json_response.contains("rid") || !json_response.contains("status"))
            {
                show_warning(L"Incorrect response from server", L"Error");
                return;
            }

            std::string sent_rid = request["rid"];
            if (json_response["rid"] != sent_rid)
            {
                show_warning(L"Response rid does not match the sent one", L"Error");
                return;
            }

            if (json_response["status"] != "true")
            {
                show_warning(L"Response status is not true", L"Error");
                return;
            }
            last_error_.clear();
        }
        catch (const std::exception& e)
        {
            last_error_ = "[JSON] " + std::string(e.what());
            show_warning(std::wstring(last_error_.begin(), last_error_.end()), L"Error");
        }
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    fclose(fp);

}

void RequestMngr::start_get()
{
    nlohmann::json request =
    {
        {"cmd", 2},
        {"rid", last_rid_}
    };

    std::string json_str = request.dump();
    std::string encrypted_line;

    CURL* curl = curl_easy_init();
    if (!curl)
    {
        show_warning(L"Failed to init CURL", L"Error");
        return;
    }

    std::string response;
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_URL, "http://172.245.127.93/p/applicants.php");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_HEADER, 0L);
    curl_easy_setopt(curl, CURLOPT_FRESH_CONNECT, 1L);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);


    std::ofstream log_get("curl.log", std::ios::app);
    log_get << "Sent data: " << json_str << "\nSent data size: " << json_str.size() << "\nRequest completed. HTTP: " << http_code
        << ", Received: " << response.size() << " bytes\n" << "The response: " << response << "\n\n";
    log_get.close();


    if (res != CURLE_OK)
    {
        last_error_ = "[CURL] " + std::string(curl_easy_strerror(res));
        show_warning(std::wstring(last_error_.begin(), last_error_.end()), L"Error");
    }
    else
    {
        try
        {
            auto json_response = nlohmann::json::parse(response);
            if (!json_response.contains("rid") || !json_response.contains("data"))
            {
                show_warning(L"Incorrect response from server", L"Error");
                return;
            }

            std::string sent_rid = request["rid"];
            if (json_response["rid"] != sent_rid)
            {
                show_warning(L"Response rid does not match the sent one", L"Error");
                return;
            }
            encrypted_line = json_response["data"].get<std::string>();

            log_get << "\nencrypted_line: " << encrypted_line << "\n";
            log_get.close();

            last_error_.clear();
        }
        catch (const std::exception& e)
        {
            last_error_ = "[JSON] " + std::string(e.what());
            show_warning(std::wstring(last_error_.begin(), last_error_.end()), L"Error");
        }
    }
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);


    std::vector<unsigned char> decoded_data;
    from_base64(decoded_data, encrypted_line.c_str());
    std::vector<uint8_t> decrypted = decrypt(decoded_data, key);
    std::string decrypted_str(decrypted.begin(), decrypted.end());
    auto j = nlohmann::json::parse(decrypted_str);
    std::vector<std::vector<uint8_t>> byteArrays = j.get<std::vector<std::vector<uint8_t>>>();
    std::vector<std::string> decodedStrings;
    for (const auto& bytes : byteArrays)
    {
        std::string str(bytes.begin(), bytes.end());
        decodedStrings.push_back(str);
    }
    
    std::stringstream combined;

    for (const auto& line : decodedStrings) 
    {
        combined << line << "\x20"; 
    }
    std::string finalMessage = combined.str();
    MessageBoxA(NULL, finalMessage.c_str(), "DLL list", MB_OK | MB_ICONINFORMATION);
}

std::wstring RequestMngr::ConvertToWide(const std::string& str)
{
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);

    if (size_needed <= 0)
    {
        return L"[Conversion error]";
    }

    std::wstring wstr(size_needed, 0);

    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], size_needed);

    wstr.resize(size_needed - 1);

    return wstr;
}

const std::string& RequestMngr::get_last_error() const 
{
    return last_error_;
}

void RequestMngr::to_base64(std::vector<unsigned char>& b64_data, std::vector<unsigned char>& enc_data)
{
    sodium_bin2base64(
        reinterpret_cast<char*>(b64_data.data()),
        b64_data.size(),
        enc_data.data(),
        enc_data.size(),
        sodium_base64_VARIANT_ORIGINAL
    );
}

void RequestMngr::from_base64(std::vector<unsigned char>& dec_data, const std::string& b64_str)
{
    dec_data.resize(b64_str.size());
    size_t decoded_len = 0;
    int result = sodium_base642bin(
        dec_data.data(),               
        dec_data.size(),               
        b64_str.c_str(),               
        b64_str.size(),                
        nullptr,                       
        &decoded_len,                  
        nullptr,                       
        sodium_base64_VARIANT_ORIGINAL
    );
    if (result != 0)
    {
        return ; 
    }
    dec_data.resize(decoded_len);

}

void RequestMngr::show_warning(std::wstring msg, std::wstring name )
{
    if (msg.empty()) return;
    MessageBox(
        NULL,                           
        msg.c_str(),
        name.c_str(),
        MB_OK | MB_ICONWARNING          
    );
}

int RequestMngr::init_key(std::filesystem::path keyPath)
{
    key = gen_key();
    
    std::filesystem::path fsKeyFile = keyPath/"protected_key.bin";
    std::string keyFile = fsKeyFile.string();
    std::vector<BYTE> protectedKey;
    
    if (!load_protected_key(keyFile, protectedKey)) 
    {
        key = gen_key();
        protectedKey = hide_key_DPAPI(key);
        if (!save_protected_key(keyFile, protectedKey))
        {
            show_warning(L"Failed to save key", L"Warning");
            return 0;
        }
    }
    else 
    {
        key = reveal_key_DPAPI(protectedKey);
        if (key.size() != crypto_secretbox_KEYBYTES) {
            show_warning(L"Incorrect key length after unpacking", L"Warning");
            return 0;
        }
    }
    return 0;
}

std::string RequestMngr::gen_rid()
{
    UUID uuid;
    RPC_STATUS status = UuidCreate(&uuid);
    if (status != RPC_S_OK) 
    {
        std::wstring state = L"Error UuidCreate, RPC_STATUS:"+std::to_wstring(status);
        show_warning(state, L"Error");
        return{};
    }
    RPC_CSTR str;
    if (UuidToStringA(&uuid, &str) != RPC_S_OK)
    {
        std::wstring state = L"Error UuidCreate, RPC_STATUS:" + std::to_wstring(status);
        show_warning(state, L"Error");
        return{};
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
    {
        show_warning(L"Incorrect key length", L"Error");
        return {};
    };

    std::vector<unsigned char> nonce(crypto_secretbox_NONCEBYTES);
    randombytes_buf(nonce.data(), nonce.size());

    std::vector<unsigned char> ciphertext(crypto_secretbox_MACBYTES + message.size());
    if (crypto_secretbox_easy(ciphertext.data(),
        reinterpret_cast<const unsigned char*>(message.data()),
        message.size(),
        nonce.data(),
        key.data()) != 0)
    {
        show_warning(L"Encryption failed", L"Error");
        return {};
    }

    nonce.insert(nonce.end(), ciphertext.begin(), ciphertext.end());
    return nonce;
}

std::vector<uint8_t> RequestMngr::decrypt(const std::vector<unsigned char>& encrypted, const std::vector<unsigned char>& key)
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
    return decrypted;
}