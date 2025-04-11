#pragma once
#include <atomic>
#include <mutex>
#include <thread>
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
	void get_process_dlls(DWORD pid);
	std::vector<std::wstring> dlls;
	int init_key();
	void show_warning(std::wstring msg, std::wstring name);

private:

	mutable std::mutex dll_mutex;
	std::vector<std::wstring> dll_list;
	std::string json_dlls;
	std::string last_rid;
	std::vector<BYTE> key;
	//std::atomic<bool> is_loading{ false };
	std::thread worker_thread_;

	bool ends_with(const std::wstring& str, const std::wstring& endStr);
	void extract_filename(std::wstring& path);

	std::string gen_rid();
	std::string encode_string();

	std::vector<BYTE> hide_key_DPAPI(const std::vector<BYTE>& key);
	std::vector<BYTE> reveal_key_DPAPI(const std::vector<BYTE>& protectedData);
	bool save_protected_key(const std::string& filePath, const std::vector<BYTE>& protectedKey);
	bool load_protected_key(const std::string& filePath, std::vector<BYTE>& protectedKey);
	std::vector<BYTE> gen_key();
	std::vector<unsigned char> encrypt(const std::string& message, const std::vector<unsigned char>& key);
	std::string decrypt(const std::vector<unsigned char>& encrypted, const std::vector<unsigned char>& key);
};

