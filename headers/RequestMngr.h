#pragma once
#include <atomic>
#include <mutex>
#include <thread>
#include <optional>
#include <stdexcept>
#include <windows.h>
#include <rpc.h>
#include <string>
#include <filesystem>
#include <tlhelp32.h>
#include <iostream>
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

	void start_async_send(); 
	bool is_loading() const;
	void get_process_dlls(DWORD pid);
	std::vector<std::wstring> dlls;

	
	
private:

	mutable std::mutex dll_mutex;
	std::vector<std::wstring> dll_list;
	std::string json_dlls;
	//std::atomic<bool> is_loading{ false };
	std::thread worker_thread_;

	bool ends_with(const std::wstring& str, const std::wstring& endStr);
	void extract_filename(std::wstring& path);
	void gen_dlls_string() { nlohmann::json j = dlls; json_dlls = j.dump(); };
	std::string gen_rid();
	std::string encode_string();
};

