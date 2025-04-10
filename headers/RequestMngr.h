#pragma once
#include <optional>
#include <stdexcept>
#include <windows.h>
#include <rpc.h>
#include <string>
#include <json.hpp>
#include <ssl.h>

class RequestMngr
{
public:
	RequestMngr();
	~RequestMngr();
	std::string gen_rid();
	
	std::string encode_string();

private:

};

