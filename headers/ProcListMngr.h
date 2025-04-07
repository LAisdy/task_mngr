#pragma once
#include <vector>
#include <string>
#include <windows.h>
#include <tlhelp32.h>
#include <unordered_map>

class ProcListMngr
{
public:

	ProcListMngr();

	~ProcListMngr();

	void update();

	std::pair<DWORD, std::wstring> get(size_t ind);

	void getProcList();

	size_t size() const { return count; };

private:
	static std::vector<std::pair<DWORD, std::wstring>> processes;
	size_t count;
};

