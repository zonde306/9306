#pragma once
#include <TlHelp32.h>
#include "./d3dfont/D3DFont.h"
#include <mutex>
// #include "ntdll.h"

#define INRANGE( x, a, b ) ( x >= a && x <= b )
#define getBits( x ) ( INRANGE( ( x & ( ~0x20 ) ), 'A', 'F' ) ? ( ( x & ( ~0x20 ) ) - 'A' + 0xa ) : ( INRANGE( x, '0', '9' ) ? x - '0' : 0 ) )
#define getByte( x ) ( getBits( x[ 0 ] ) << 4 | getBits( x[ 1 ] ) )
#define ThreadBasicInformation 0

CNetVars::CNetVars()
{
	_tables.clear();

	ClientClass* clientClass = g_interface.Client->GetAllClasses();
	if (clientClass == nullptr)
	{
		Utils::log("ERROR: ClientClass was not found");
		return;
	}

	while (clientClass)
	{
		RecvTable *recvTable = clientClass->m_pRecvTable;

		_tables.push_back(recvTable);

		clientClass = clientClass->m_pNext;
	}
}

int CNetVars::GetOffset(const char* tableName, const char* propName)
{
	int offset = GetProp(tableName, propName);
	if (offset <= -1)
	{
		Utils::log("ERROR: Failed to find offset for prop: %s from table: %s", propName, tableName);
		return -1;
	}


	return offset;
}

int CNetVars::GetProp(const char* tableName, const char* propName, RecvProp **prop)
{
	RecvTable* recvTable = GetTable(tableName);
	if (!recvTable)
	{
		Utils::log("ERROR: Failed to find table: %s", tableName);
		return -1;
	}
	int offset = GetProp(recvTable, propName, prop);
	if (offset <= -1)
	{
		Utils::log("ERROR: Failed to find offset for prop: %s from table: %s", propName, tableName);
		return -1;
	}

	return offset;
}

int CNetVars::GetProp(RecvTable *recvTable, const char *propName, RecvProp **prop)
{
	int extraOffset = 0;
	for (int i = 0; i < recvTable->m_nProps; ++i)
	{
		RecvProp* recvProp = &recvTable->m_pProps[i];

		RecvTable *child = recvProp->m_pDataTable;

		if (child && (child->m_nProps > 0))
		{
			int tmp = GetProp(child, propName, prop);
			if (tmp)
			{
				extraOffset += (recvProp->m_Offset + tmp);
			}
		}

		if (_stricmp(recvProp->m_pVarName, propName))
			continue;
		if (prop)
		{
			*prop = recvProp;
		}
		return (recvProp->m_Offset + extraOffset);
	}
	return extraOffset;
}

RecvTable *CNetVars::GetTable(const char *tableName)
{
	if (_tables.empty())
	{
		Utils::log("ERROR: Failed to find table: %s (_tables is empty)", tableName);
		return nullptr;
	}

	for each (RecvTable* table in _tables)
	{
		if (!table)
			continue;
		if (_stricmp(table->m_pNetTableName, tableName) == 0)
			return table;
	}
	return nullptr;
}

class D3D9Hooker
{
public:
	D3D9Hooker() : pD3D(nullptr), pDevice(nullptr), canRelease(false)
	{
		CreateDevice();
	}

	~D3D9Hooker()
	{
		ReleaseDevice();
	}

	bool CreateDevice()
	{
		HWND window = GetDesktopWindow();
		if (window == nullptr)
		{
			Utils::log("%s (%d) 错误：获取顶层窗口失败", __FILE__, __LINE__);
			return false;
		}

		pD3D = Direct3DCreate9(D3D_SDK_VERSION);
		if (pD3D == nullptr)
		{
			Utils::log("%s (%d) 错误：创建 D3D 对象失败", __FILE__, __LINE__);
			return false;
		}

		// 显示模式
		D3DDISPLAYMODE dm;
		if (FAILED(pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &dm)))
		{
			if (pD3D != nullptr)
				pD3D->Release();
			pD3D = nullptr;

			Utils::log("%s (%d) 错误：获取显示模式失败", __FILE__, __LINE__);
			return false;
		}

		// 创建参数
		D3DPRESENT_PARAMETERS pp;
		ZeroMemory(&pp, sizeof(pp));
		pp.Windowed = TRUE;
		pp.hDeviceWindow = window;
		pp.BackBufferCount = 1;
		pp.BackBufferWidth = 4;
		pp.BackBufferHeight = 4;
		pp.BackBufferFormat = dm.Format;
		pp.SwapEffect = D3DSWAPEFFECT_DISCARD;

		pDevice = nullptr;
		HRESULT hr = pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window,
			D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_DISABLE_DRIVER_MANAGEMENT, &pp, &pDevice);
		if (FAILED(hr))
		{
			if (hr == D3DERR_INVALIDCALL)
				Utils::log("%s (%d) 错误：参数错误，请检查指针是否有效", __FILE__, __LINE__);
			else if (hr == D3DERR_DEVICELOST)
				Utils::log("%s (%d) 错误：设备对象丢失了", __FILE__, __LINE__);
			else if (hr == D3DERR_NOTAVAILABLE)
				Utils::log("%s (%d) 错误：不支持这个功能", __FILE__, __LINE__);
			else if (hr == D3DERR_OUTOFVIDEOMEMORY)
				Utils::log("%s (%d) 错误：内存不足", __FILE__, __LINE__);
			else
				Utils::log("%s (%d) 错误：发生位置错误 %d", __FILE__, __LINE__, hr);

			return false;
		}

		if (pD3D == nullptr || pDevice == nullptr)
		{
			if (pD3D != nullptr)
				pD3D->Release();
			if (pDevice != nullptr)
				pDevice->Release();

			pD3D = nullptr;
			pDevice = nullptr;

			Utils::log("%s (%d) 错误：创建失败，错误未知", __FILE__, __LINE__);
			return false;
		}

		canRelease = true;
		return true;
	}

	bool ReleaseDevice()
	{
		if (!canRelease)
			return false;

		if (pD3D != nullptr)
			pD3D->Release();
		if (pDevice != nullptr)
			pDevice->Release();

		pD3D = nullptr;
		pDevice = nullptr;
		canRelease = false;
		return true;
	}

	bool StartDeviceHook(std::function<void(IDirect3D9*&, IDirect3DDevice9*&, DWORD*&)> func)
	{
		if (pDevice == nullptr && !CreateDevice())
			return false;

		// 虚函数表
		DWORD* pVMT = *(DWORD**)pDevice;
		func(pD3D, pDevice, pVMT);

		ReleaseDevice();

		return true;
	}

	IDirect3DDevice9*& GetDevice()
	{
		return pDevice;
	}

private:
	IDirect3D9* pD3D;
	IDirect3DDevice9* pDevice;
	bool canRelease;
};

// GBK 转 UTF-8
std::string Utils::g2u(const std::string& strGBK)
{
	std::string strOutUTF8 = "";
	WCHAR * str1;
	int n = MultiByteToWideChar(CP_ACP, 0, strGBK.c_str(), -1, NULL, 0);
	str1 = new WCHAR[n];
	MultiByteToWideChar(CP_ACP, 0, strGBK.c_str(), -1, str1, n);
	n = WideCharToMultiByte(CP_UTF8, 0, str1, -1, NULL, 0, NULL, NULL);
	char * str2 = new char[n];
	WideCharToMultiByte(CP_UTF8, 0, str1, -1, str2, n, NULL, NULL);
	strOutUTF8 = str2;
	delete[]str1;
	str1 = NULL;
	delete[]str2;
	str2 = NULL;
	return strOutUTF8;
}

// UTF-8 转 GBK
std::string Utils::u2g(const std::string& strUTF8)
{
	int len = MultiByteToWideChar(CP_UTF8, 0, strUTF8.c_str(), -1, NULL, 0);
	WCHAR * wszGBK = new WCHAR[len + 1];
	memset(wszGBK, 0, len * 2 + 2);
	MultiByteToWideChar(CP_UTF8, 0, strUTF8.c_str(), -1, wszGBK, len);

	len = WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, NULL, 0, NULL, NULL);
	char *szGBK = new char[len + 1];
	memset(szGBK, 0, len + 1);
	WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, szGBK, len, NULL, NULL);
	//strUTF8 = szGBK;  
	std::string strTemp(szGBK);
	delete[]szGBK;
	delete[]wszGBK;
	return strTemp;
}

// UTF-8 转 GB2312
char* Utils::utg(const char* utf8)
{
	int len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
	wchar_t* wstr = new wchar_t[len + 1];
	memset(wstr, 0, len + 1);
	MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wstr, len);
	len = WideCharToMultiByte(CP_ACP, 0, wstr, -1, NULL, 0, NULL, NULL);
	char* str = new char[len + 1];
	memset(str, 0, len + 1);
	WideCharToMultiByte(CP_ACP, 0, wstr, -1, str, len, NULL, NULL);
	if (wstr) delete[] wstr;
	return str;
}

// GB2312 转 UTF-8
char* Utils::gtu(const char* gb2312)
{
	int len = MultiByteToWideChar(CP_ACP, 0, gb2312, -1, NULL, 0);
	wchar_t* wstr = new wchar_t[len + 1];
	memset(wstr, 0, len + 1);
	MultiByteToWideChar(CP_ACP, 0, gb2312, -1, wstr, len);
	len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
	char* str = new char[len + 1];
	memset(str, 0, len + 1);
	WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, len, NULL, NULL);
	if (wstr) delete[] wstr;
	return str;
}

// wchar 转 char
std::string Utils::w2c(const std::wstring& ws)
{
	size_t convertedChars = 0;
	std::string curLocale = setlocale(LC_ALL, NULL); //curLocale="C"
	setlocale(LC_ALL, "chs");
	const wchar_t* wcs = ws.c_str();
	size_t dByteNum = sizeof(wchar_t)*ws.size() + 1;
	// cout << "ws.size():" << ws.size() << endl;            //5

	char* dest = new char[dByteNum];
	wcstombs_s(&convertedChars, dest, dByteNum, wcs, _TRUNCATE);
	// cout << "convertedChars:" << convertedChars << endl; //8
	std::string result = dest;
	delete[] dest;
	setlocale(LC_ALL, curLocale.c_str());
	return result;
}

// char 转 wchar
std::wstring Utils::c2w(const std::string& s)
{
	size_t convertedChars = 0;
	std::string curLocale = setlocale(LC_ALL, NULL);   //curLocale="C"
	setlocale(LC_ALL, "chs");
	const char* source = s.c_str();
	size_t charNum = sizeof(char)*s.size() + 1;
	// cout << "s.size():" << s.size() << endl;   //7

	wchar_t* dest = new wchar_t[charNum];
	mbstowcs_s(&convertedChars, dest, charNum, source, _TRUNCATE);
	// cout << "s2ws_convertedChars:" << convertedChars << endl; //6
	std::wstring result = dest;
	delete[] dest;
	setlocale(LC_ALL, curLocale.c_str());
	return result;
}

DWORD Utils::FindPattern(const std::string& szModules, const std::string& szPattern)
{
	HMODULE hmModule = GetModuleHandleSafe(szModules);
	PIMAGE_DOS_HEADER pDOSHeader = (PIMAGE_DOS_HEADER)hmModule;
	PIMAGE_NT_HEADERS pNTHeaders = (PIMAGE_NT_HEADERS)(((DWORD)hmModule) + pDOSHeader->e_lfanew);
	
	DWORD dwAddress = ((DWORD)hmModule) + pNTHeaders->OptionalHeader.BaseOfCode;
	DWORD dwLength = ((DWORD)hmModule) + pNTHeaders->OptionalHeader.SizeOfCode;

	const char *pat = szPattern.c_str();
	DWORD firstMatch = NULL;
	for (DWORD pCur = dwAddress; pCur < dwLength; pCur++)
	{
		if (!*pat)
			return firstMatch;
		if (*(PBYTE)pat == '\?' || *(BYTE *)pCur == getByte(pat))
		{
			if (!firstMatch)
				firstMatch = pCur;
			if (!pat[2])
				return firstMatch;
			if (*(PWORD)pat == '\?\?' || *(PBYTE)pat != '\?')
				pat += 3;
			else
				pat += 2;
		}
		else
		{
			pat = szPattern.c_str();
			firstMatch = 0;
		}
	}

	return NULL;
}

DWORD Utils::FindPattern(const std::string & szModules, const std::string & szPattern, std::string szMask)
{
	HMODULE hmModule = GetModuleHandleSafe(szModules);
	PIMAGE_DOS_HEADER pDOSHeader = (PIMAGE_DOS_HEADER)hmModule;
	PIMAGE_NT_HEADERS pNTHeaders = (PIMAGE_NT_HEADERS)(((DWORD)hmModule) + pDOSHeader->e_lfanew);

	BYTE First = szPattern[0];
	PBYTE BaseAddress = (PBYTE)(((DWORD)hmModule) + pNTHeaders->OptionalHeader.BaseOfCode);
	PBYTE Max = (PBYTE)(BaseAddress + pNTHeaders->OptionalHeader.SizeOfCode - szPattern.length());

	const static auto CompareByteArray = [](PBYTE Data, const char* Signature, const char* Mask) -> bool
	{
		for (; *Signature; ++Signature, ++Data, ++Mask)
		{
			if (*Mask == '?' || *Signature == '\x00' || *Signature == '\x2A')
			{
				continue;
			}
			if (*Data != *Signature)
			{
				return false;
			}
		}
		return true;
	};

	if (szMask.length() < szPattern.length())
	{
		for (auto i = szPattern.begin() + (szPattern.length() - 1); i != szPattern.end(); ++i)
			szMask.push_back(*i == '\x2A' || *i == '\x00' ? '?' : 'x');
	}

	for (; BaseAddress < Max; ++BaseAddress)
	{
		if (*BaseAddress != First)
		{
			continue;
		}
		if (CompareByteArray(BaseAddress, szPattern.c_str(), szMask.c_str()))
		{
			return (DWORD)BaseAddress;
		}
	}

	return NULL;
}

HMODULE Utils::GetModuleHandleSafe(const std::string& pszModuleName)
{
	HMODULE hmModuleHandle = NULL;

	do
	{
		hmModuleHandle = GetModuleHandleA(pszModuleName.c_str());
		std::this_thread::sleep_for(std::chrono::microseconds(100));
	} while (hmModuleHandle == NULL);

	return hmModuleHandle;
}

// 获取 dll 地址
DWORD Utils::GetModuleBase(const std::string& ModuleName, DWORD ProcessID)
{
	if (ProcessID == 0)
		ProcessID = GetCurrentProcessId();

	MODULEENTRY32 lpModuleEntry = { 0 };
	HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, ProcessID);
	if (!hSnapShot)
		return NULL;

	lpModuleEntry.dwSize = sizeof(lpModuleEntry);
	BOOL __RunModule = Module32First(hSnapShot, &lpModuleEntry);
	while (__RunModule)
	{
		if (lpModuleEntry.szModule == ModuleName)
		{
			CloseHandle(hSnapShot);
			return (DWORD)lpModuleEntry.modBaseAddr;
		}
		__RunModule = Module32Next(hSnapShot, &lpModuleEntry);
	}
	CloseHandle(hSnapShot);
	return NULL;
}

DWORD Utils::GetModuleBase(const std::string& ModuleName, DWORD* ModuleSize, DWORD ProcessID)
{
	if (ProcessID == 0)
		ProcessID = GetCurrentProcessId();

	MODULEENTRY32 lpModuleEntry = { 0 };
	HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, ProcessID);
	if (!hSnapShot)
		return NULL;

	lpModuleEntry.dwSize = sizeof(lpModuleEntry);
	BOOL __RunModule = Module32First(hSnapShot, &lpModuleEntry);
	while (__RunModule)
	{
		if (lpModuleEntry.szModule == ModuleName)
		{
			CloseHandle(hSnapShot);
			*ModuleSize = lpModuleEntry.dwSize;
			return (DWORD)lpModuleEntry.modBaseAddr;
		}
		__RunModule = Module32Next(hSnapShot, &lpModuleEntry);
	}
	CloseHandle(hSnapShot);
	return NULL;
}

// 根据进程名获取进程id
DWORD Utils::FindProccess(const std::string& proccessName)
{
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	PROCESSENTRY32 pe;
	pe.dwSize = sizeof(pe);

	if (!Process32First(snapshot, &pe))
		return 0;

	while (Process32Next(snapshot, &pe))
	{
		if (pe.szExeFile == proccessName)
			return pe.th32ProcessID;
	}

	return 0;
}

template<typename T, size_t len>
inline int Utils::GetArrayLength(const T(&arr)[len])
{
	return (int)len;
}

template<typename T, typename ...Arg>
T Utils::readMemory(Arg... offset)
{
	DWORD offsetList[] = { (DWORD)offset... };
	DWORD currentAddress = 0, finalAddress = 0, oldProtect = 0;
	int len = GetArrayLength(offsetList);
	if (len <= 0)
	{
		Utils::log("请提供至少一个地址！");
		return T();
	}

	try
	{
		for (int i = 0; i < len; ++i)
		{
			/*
			if (i == len - 1 || offsetList[i] == NULL)
			{
			if (finalAddress != NULL)
			return *(T*)finalAddress;

			printf("找不到任何东西。\n");
			return T();
			}
			*/

			currentAddress += offsetList[i];
			if (VirtualProtect((void*)currentAddress, sizeof(finalAddress), PAGE_EXECUTE_READWRITE, &oldProtect) == FALSE)
			{
				printf("错误：修改地址 0x%X 的保护失败。\n", currentAddress);
				return T();
			}

			if (i < len - 1)
			{
				finalAddress = *(DWORD*)currentAddress;
				// memcpy_s(&finalAddress, sizeof(finalAddress), (void*)current, sizeof(current));
			}
			else
				finalAddress = currentAddress;

#ifdef DEBUG
			if (VirtualProtect((void*)currentAddress, sizeof(finalAddress), oldProtect, NULL) == FALSE)
				printf("错误：恢复地址 0x%X 的保护失败。\n", currentAddress);
#else
			VirtualProtect((void*)currentAddress, sizeof(finalAddress), oldProtect, &oldProtect);
#endif

			// 将当前地址设置为最后的地址
			currentAddress = finalAddress;
		}

		if (finalAddress != NULL)
			return *(T*)finalAddress;
	}
	catch (...)
	{
		Utils::log("错误：读取地址为 0x%X 的内容失败。", currentAddress);
	}

	return T();
}

template<typename T, typename ...Arg>
T Utils::writeMemory(T value, Arg... offset)
{
	DWORD offsetList[] = { (DWORD)offset... };
	DWORD currentAddress = 0, finalAddress = 0, oldProtect = 0;
	int len = GetArrayLength(offsetList);
	if (len <= 0)
	{
		Utils::log("请提供至少一个地址！");
		return T();
	}

	try
	{
		for (int i = 0; i < len; ++i)
		{
			/*
			if (i == len - 1 || offsetList[i] == NULL)
			{
			if (finalAddress != NULL)
			return (*(T*)finalAddress = value);

			printf("找不到任何东西。\n");
			return T();
			}
			*/

			currentAddress += offsetList[i];
			if (VirtualProtect((void*)currentAddress, sizeof(finalAddress), PAGE_EXECUTE_READWRITE, &oldProtect) == FALSE)
			{
				Utils::log("错误：修改地址 0x%X 的保护失败。", currentAddress);
				return T();
			}

			if (i < len - 1)
			{
				finalAddress = *(DWORD*)currentAddress;
				// memcpy_s(&finalAddress, sizeof(finalAddress), (void*)current, sizeof(current));
			}
			else
				finalAddress = currentAddress;

#ifdef DEBUG
			if (VirtualProtect((void*)currentAddress, sizeof(finalAddress), oldProtect, NULL) == FALSE)
				printf("错误：恢复地址 0x%X 的保护失败。\n", currentAddress);
#else
			VirtualProtect((void*)currentAddress, sizeof(finalAddress), oldProtect, &oldProtect);
#endif

			// 将当前地址设置为最后的地址
			currentAddress = finalAddress;
		}

		if (finalAddress != NULL)
			return (*(T*)finalAddress = value);
	}
	catch (...)
	{
		Utils::log("错误：读取地址为 0x%X 的内容失败。", currentAddress);
	}

	return T();
}

std::vector<std::string> Utils::split(const std::string& s, const std::string& delim)
{
	std::vector<std::string> result;
	size_t last = 0;
	size_t index = s.find_first_of(delim, last);
	while (index != std::string::npos)
	{
		result.push_back(s.substr(last, index - last));
		last = index + 1;
		index = s.find_first_of(delim, last);
	}
	if (index - last > 0)
	{
		result.push_back(s.substr(last, index - last));
	}

	return result;
}

std::string Utils::trim(const std::string& s, const std::string& delim)
{
	if (s.empty())
		return s;

	std::string result = s;
	for (char c : delim)
	{
		result.erase(0, result.find_first_not_of(c));
		result.erase(result.find_last_not_of(c) + 1);
	}

	return result;
}

std::string Utils::GetPath()
{
	return g_sCurPath;
}

void Utils::log(const char* text, ...)
{
	char buffer[1024];

#ifdef _DEBUG

	time_t t;
	time(&t);

	tm tmp;
	localtime_s(&tmp, &t);

	// 文件创建日期
	strftime(buffer, 1024, "\\segt_%Y%m%d.log", &tmp);

	std::fstream file(GetPath() + buffer, std::ios::out | std::ios::app | std::ios::ate);

	// 日志写入时间
	strftime(buffer, 1024, "[%H:%M:%S] ", &tmp);
	file << buffer;

#endif

	// 格式化字符串
	va_list ap;
	va_start(ap, text);
	vsprintf_s(buffer, text, ap);
	va_end(ap);

#ifdef _DEBUG

	// 输出
	file << buffer << "\n";

	// 完毕
	file.close();

#endif

	// 输出到控制台
	if(Config::bAllowConsoleMessage)
		g_interface.Engine->ClientCmd("echo \"%s\"", buffer);

	std::cout << buffer << std::endl;
}

void Utils::log(const wchar_t* text, ...)
{
	wchar_t buffer[1024];

#ifdef _DEBUG

	char ctime[64];

	time_t t;
	time(&t);

	tm tmp;
	localtime_s(&tmp, &t);

	// 文件创建日期
	strftime(ctime, 64, "\\segt_%Y%m%d.log", &tmp);

	std::wfstream file(GetPath() + ctime, std::ios::out | std::ios::app | std::ios::ate);

	// 日志写入时间
	strftime(ctime, 64, "[%H:%M:%S] ", &tmp);
	file << c2w(ctime);

#endif

	// 格式化字符串
	va_list ap;
	va_start(ap, text);
	vswprintf_s(buffer, text, ap);
	va_end(ap);

#ifdef _DEBUG

	// 输出
	file << buffer << L"\n";

	// 完毕
	file.close();

#endif

	// 输出到控制台
	if (Config::bAllowConsoleMessage)
		g_interface.Engine->ClientCmd("echo \"%s\"", w2c(buffer));

	std::wcout << buffer << std::endl;
}
