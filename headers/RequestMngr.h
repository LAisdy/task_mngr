#pragma once
#include <atomic>
#include <mutex>
#include <thread>
#include <future>
#include <optional>
#include <stdexcept>
#include <windows.h>
#include <wincrypt.h>
#include <rpc.h>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <tlhelp32.h>
#include <iostream>
#include <sstream>

#define SODIUM_STATIC
#include <sodium.h>


//nlohmann-json
#include <json.hpp>

//openssl
#include <evp.h>
#include <rand.h>
#include <bio.h>
//curl
#include <curl.h>

class RequestMngr
{
public:
	RequestMngr();
	~RequestMngr();

	void start_async_send(DWORD pid);
	void start_async_get();
	void get_process_dlls(DWORD pid);
	std::vector<std::wstring> dlls;
	int init_key(std::filesystem::path keyPath);
	void show_warning(const std::wstring msg, const std::wstring name);
	const std::string& get_last_rid() const;
	const std::string& get_last_error() const;
	std::wstring ConvertToWide(const std::string& str);

private:

	mutable std::mutex mutex;

	std::string json_dlls;
	std::string last_rid_;
	std::string last_error_;
	std::vector<BYTE> key;

	bool ends_with(const std::wstring& str, const std::wstring& endStr);
	void extract_filename(std::wstring& path);

	std::string gen_rid();
	void to_base64(std::vector<unsigned char>& b64_data, std::vector<unsigned char>& enc_data);
	void from_base64(std::vector<unsigned char>& dec_data, const std::string& b64_str);
	std::vector<BYTE> hide_key_DPAPI(const std::vector<BYTE>& key);
	std::vector<BYTE> reveal_key_DPAPI(const std::vector<BYTE>& protectedData);
	bool save_protected_key(const std::string& filePath, const std::vector<BYTE>& protectedKey);
	bool load_protected_key(const std::string& filePath, std::vector<BYTE>& protectedKey);
	std::vector<BYTE> gen_key();
	std::vector<unsigned char> encrypt(const std::string& message, const std::vector<unsigned char>& key);
	std::vector<uint8_t> decrypt(const std::vector<unsigned char>& encrypted, const std::vector<unsigned char>& key);
};

