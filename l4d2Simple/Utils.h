﻿#pragma once
#include <TlHelp32.h>
#include "./d3dfont/D3DFont.h"
#include <mutex>
// #include "ntdll.h"

#define INRANGE( x, a, b ) ( x >= a && x <= b )
#define getBits( x ) ( INRANGE( ( x & ( ~0x20 ) ), 'A', 'F' ) ? ( ( x & ( ~0x20 ) ) - 'A' + 0xa ) : ( INRANGE( x, '0', '9' ) ? x - '0' : 0 ) )
#define getByte( x ) ( getBits( x[ 0 ] ) << 4 | getBits( x[ 1 ] ) )
#define ThreadBasicInformation 0

extern HINSTANCE g_hMyInstance;

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

// 将 D3DCOLOR 转换为 ImGui 的颜色
// DirectX 的颜色为 ARGB。ImGui 的颜色为 ABGR
#define D3DCOLOR_IMGUI(_clr)	((_clr & 0xFF000000) | ((_clr & 0x00FF0000) >> 16) | (_clr & 0x0000FF00) | ((_clr & 0x000000FF) << 16))

class DrawManager
{
public:
	DrawManager(IDirect3DDevice9* pDevice, int fontSize = 14);
	~DrawManager();

	// 在 Reset 之前调用
	void OnLostDevice();

	// 在 Reset 之后调用
	void OnResetDevice();

	// 在 EndScene 之前，绘制开始之前调用
	void BeginRendering();

	// 在 EndScene 之前，绘制完成之后调用
	void EndRendering();

	// 绘制一条线（必须在调用 BeginRendering 之后才可以使用）
	void RenderLine(D3DCOLOR color, int x1, int y1, int x2, int y2);

	// 绘制一个矩形（必须在调用 BeginRendering 之后才可以使用）
	void RenderRect(D3DCOLOR color, int x, int y, int w, int h);

	// 绘制一个圆形（必须在调用 BeginRendering 之后才可以使用）
	void RenderCircle(D3DCOLOR color, int x, int y, int r, size_t resolution = 64);

	// 绘制一个填充圆形
	void RenderFillCircle(D3DCOLOR color, int x, int y, int r, size_t resolution = 64);

	// 延迟绘制一个只有四个角的矩形
	void RenderCorner(D3DCOLOR color, int x, int y, int w, int h, size_t length = 0);

	// 绘制文本（必须在调用 BeginRendering 之后才可以使用）
	__declspec(deprecated) void RenderText(D3DCOLOR color, int x, int y, bool centered, const char* fmt, ...);

	// 绘制文本（必须在调用 BeginRendering 之后才可以使用）
	__declspec(deprecated) void RenderText(D3DCOLOR color, int x, int y, bool centered, const wchar_t* fmt, ...);

	// 绘制填充矩形（必须在调用 BeginRendering 之后才可以使用）
	void RenderFillRect(D3DCOLOR color, int x, int y, int w, int h);

#ifndef ORIGINAL_CD3DFONT
	HRESULT DrawString2(float x, float y, D3DCOLOR color, const char* text, ...);
	inline HRESULT DrawString2Begin();
	inline HRESULT DrawString2Finish();
#else
	void DrawString2(float x, float y, D3DCOLOR color, const wchar_t* text, ...);
#endif

	// 延迟绘制一条线
	void AddLine(D3DCOLOR color, int x1, int y1, int x2, int y2);

	// 延迟绘制一个矩形
	void AddRect(D3DCOLOR color, int x, int y, int w, int h);

	// 延迟绘制一个圆形
	void AddCircle(D3DCOLOR color, int x, int y, int r, size_t resolution = 64);

	// 延迟绘制一个填充圆形
	void AddFillCircle(D3DCOLOR color, int x, int y, int r, size_t resolution = 64);

	// 延迟绘制文本（只支持 ASCII 格式字符）
	void AddText(D3DCOLOR color, int x, int y, bool centered, const char* fmt, ...);

	// 延迟绘制一个填充矩形
	void AddFillRect(D3DCOLOR color, int x, int y, int w, int h);

	// 延迟绘制文本
	void AddString(D3DCOLOR color, int x, int y, bool centered, const char* fmt, ...);

	// 延迟绘制一个只有四个角的矩形
	void AddCorner(D3DCOLOR color, int x, int y, int w, int h, size_t length = 0);

	__declspec(deprecated) void DrawString(int x, int y, D3DCOLOR color, const char* text, ...);
	__declspec(deprecated) void DrawString(int x, int y, D3DCOLOR color, const wchar_t* text, ...);
	__declspec(deprecated) void DrawRect(int x, int y, int width, int height, D3DCOLOR color);
	__declspec(deprecated) void DrawBorderedRect(int x, int y, int width, int height, D3DCOLOR filled, D3DCOLOR color);
	__declspec(deprecated) void DrawLine(int x, int y, int x2, int y2, D3DCOLOR color);
	__declspec(deprecated) void DrawFilledRect(int x, int y, int width, int height, D3DCOLOR color);
	__declspec(deprecated) void DrawCircle(int x, int y, int radius, D3DCOLOR color);

	// 将文本添加到限时绘制队列，这些文本会在添加后的 5 秒之后自动消失
	void PushRenderText(D3DCOLOR color, const char* text, ...);

	// 获取当前字体的大小
	inline int GetFontSize();

	// 将游戏世界坐标转为屏幕上的位置
	// 如果提供的位置在屏幕可见范围内，输出屏幕坐标并返回 true。否则返回 false
	bool WorldToScreen(const Vector& origin, Vector& output);
	std::string DisassembleShader();

	/*
	* Below are some functions that you can implement yourself as an exercise
	*
	* void FillCircle(DWORD color, int x, int y, int r, int resolution = 64);
	* void RenderPolygon(DWORD color, int* x, int* y, int npoints);
	* void FillPolygon(DWORD color, int* x, int* y, int npoints);
	* void FillGradient(DWORD from, Color to, bool isVertical, int x, int y, int w, int h);
	*/

	// 常用颜色
	enum D3DColor
	{
		WHITE = D3DCOLOR_ARGB(255, 255, 255, 255),		// 白色
		BLACK = D3DCOLOR_ARGB(255, 0, 0, 0),			// 黑色
		RED = D3DCOLOR_ARGB(255, 255, 0, 0),			// 红色
		GREEN = D3DCOLOR_ARGB(255, 0, 128, 0),			// 绿色
		LAWNGREEN = D3DCOLOR_ARGB(255, 124, 252, 0),	// 草绿色
		BLUE = D3DCOLOR_ARGB(255, 0, 200, 255),			// 蓝色
		DEEPSKYBLUE = D3DCOLOR_ARGB(255, 0, 191, 255),	// 深蓝色
		SKYBLUE = D3DCOLOR_ARGB(255, 0, 122, 204),		// 天蓝色
		YELLOW = D3DCOLOR_ARGB(255, 255, 255, 0),		// 黄色
		ORANGE = D3DCOLOR_ARGB(255, 255, 165, 0),		// 橙色
		DARKORANGE = D3DCOLOR_ARGB(255, 255, 140, 0),	// 暗橙色
		PURPLE = D3DCOLOR_ARGB(255, 125, 0, 255),		// 紫色
		CYAN = D3DCOLOR_ARGB(255, 0, 255, 255),			// 青色
		PINK = D3DCOLOR_ARGB(255, 255, 20, 147),		// 粉色
		GRAY = D3DCOLOR_ARGB(255, 128, 128, 128),		// 灰色
		DARKGRAY = D3DCOLOR_ARGB(255, 73, 73, 73),		// 暗灰色
		DARKERGRAY = D3DCOLOR_ARGB(255, 31, 31, 31)		// 浅灰色
	};

#ifdef IMGUI_VERSION
	// 开始绘制菜单
	void BeginImGuiRender();

	// 完成绘制菜单
	void FinishImGuiRender();
#endif

protected:
	struct D3DVertex
	{
		D3DVertex(float _x, float _y, float _z, D3DCOLOR _color) :
			x(_x), y(_y), z(_z), rhw(0.0f), color(_color)
		{}

		D3DVertex(float _x, float _y, float _z, float _w, D3DCOLOR _color) :
			x(_x), y(_y), z(_z), rhw(_w), color(_color)
		{}

		D3DVertex() : x(0.0f), y(0.0f), z(0.0f), rhw(0.0f), color(0.0f)
		{}

		float x;
		float y;
		float z;
		float rhw;
		D3DCOLOR color;
	};

	struct TextQueue
	{
		TextQueue(D3DCOLOR color, const std::string& text, int second = 3) :
			text(text), color(color)
		{
			destoryTime = time(nullptr) + second;
		}

		std::string text;
		D3DCOLOR color;
		time_t destoryTime;
	};

	struct DelayDraw
	{
		DelayDraw(D3DPRIMITIVETYPE type, size_t count, const std::vector<D3DVertex>& vertex) : type(type)
		{
			this->vertex = std::move(vertex);
			vertexCount = count;
			if (vertexCount > this->vertex.size())
				throw std::exception("请提供一个有内容的数组");
		}
		
		DelayDraw(D3DVertex* vertex, size_t len, D3DPRIMITIVETYPE type) :
			type(type), vertexCount(len)
		{
			for (size_t i = 0; i < len; ++i)
				this->vertex.push_back(vertex[i]);
		}
		
		std::vector<D3DVertex> vertex;
		D3DPRIMITIVETYPE type;
		size_t vertexCount;
	};

	struct DelayString
	{
		DelayString(float _x, float _y, const std::string& _text, D3DCOLOR _color, DWORD _flags,
			D3DCOLOR bgcolor) : x(_x), y(_y), text(_text), color(_color), flags(_flags),
			background(bgcolor)
		{}

		float x, y;
		DWORD flags;
		D3DCOLOR color, background;
		std::string text;
	};

private:
	void ReleaseObjects();
	void CreateObjects();
	void DrawQueueObject();
	static std::string FindFonts(const std::string& name);

private:
	IDirect3DDevice9*			m_pDevice;
	IDirect3DStateBlock9*		m_pStateBlock;
	ID3DXFont*					m_pDefaultFont;
	CD3DFont*					m_pFont;
	ID3DXLine*					m_pLine;
	ID3DXSprite*				m_pTextSprite;

	// 字体大小
	int							m_iFontSize;

	// 当前是否正在绘制
	bool						m_bRenderRunning;
	std::mutex					m_hasDelayDrawing;

	// 文本绘制队列
	std::vector<TextQueue>		m_textDrawQueue;
	
	// 延迟绘制
	std::vector<DelayDraw>		m_delayDraw;
	std::vector<DelayString>	m_delayString;

#ifdef IMGUI_VERSION
	bool						m_bImIsFirst;
	ImDrawData					m_imDrawData;
	ImDrawList*					m_imDrawList;
	IDirect3DTexture9*			m_imFontTexture;
	ImFontAtlas					m_imFonts;
#endif
};

#define D3DFONT_ORIGINAL 0x80

bool DrawManager::WorldToScreen(const Vector& origin, Vector& output)
{
	D3DVIEWPORT9 viewport;
	m_pDevice->GetViewport(&viewport);

	D3DXMATRIX viewMatrix, projectionMatrix, worldMatrix;
	m_pDevice->GetTransform(D3DTS_VIEW, &viewMatrix);
	m_pDevice->GetTransform(D3DTS_PROJECTION, &projectionMatrix);

	// m_pDevice->GetTransform(D3DTS_WORLD, &worldMatrix);
	D3DXMatrixIdentity(&worldMatrix);
	/*
	D3DXMATRIX wm0, wm1, wm2, wm3;
	m_pDevice->GetTransform(D3DTS_WORLD, &wm0);
	m_pDevice->GetTransform(D3DTS_WORLD1, &wm1);
	m_pDevice->GetTransform(D3DTS_WORLD2, &wm2);
	m_pDevice->GetTransform(D3DTS_WORLD3, &wm3);
	worldMatrix = (wm0 * 0.2f) + (wm1 * 0.2f) + (wm2 * 0.2f) + (wm3 * 0.2f) + (wm0 * 0.2f);
	*/

	D3DXVECTOR3 worldPosition(origin.x, origin.y, origin.z), screenPosition;
	D3DXVec3Project(&screenPosition, &worldPosition, &viewport,
		&projectionMatrix, &viewMatrix, &worldMatrix);
	
	output.x = screenPosition.x;
	output.y = screenPosition.y;
	output.z = screenPosition.z;

	return (screenPosition.z < 1.0f);
}

std::string DrawManager::DisassembleShader()
{
	IDirect3DVertexShader9* shader;
	m_pDevice->GetVertexShader(&shader);

	UINT sizeOfData;
	shader->GetFunction(nullptr, &sizeOfData);

	BYTE* data = new BYTE[sizeOfData];
	shader->GetFunction(data, &sizeOfData);

	ID3DXBuffer* out;
	D3DXDisassembleShader(reinterpret_cast<DWORD*>(data), false, nullptr, &out);
	
	std::string result(static_cast<char*>(out->GetBufferPointer()));
	delete[] data;
	shader->Release();

	return result;
}

DrawManager::DrawManager(IDirect3DDevice9* pDevice, int fontSize)
{
	m_pDevice = pDevice;
	m_iFontSize = fontSize;
	m_bRenderRunning = false;
	CreateObjects();
}

DrawManager::~DrawManager()
{
	ReleaseObjects();
}

void DrawManager::OnLostDevice()
{
	ReleaseObjects();
	m_bRenderRunning = false;
	m_bImIsFirst = false;
}

void DrawManager::OnResetDevice()
{
	CreateObjects();
	m_bRenderRunning = false;
	m_bImIsFirst = false;
	
	int width, height;
	g_interface.Engine->GetScreenSize(width, height);
	ImGui::GetIO().DisplaySize.x = width;
	ImGui::GetIO().DisplaySize.y = height;
}

void DrawManager::ReleaseObjects()
{
	m_hasDelayDrawing.lock();
	
	if (m_pStateBlock)
	{
		m_pStateBlock->Release();
		m_pStateBlock = nullptr;
	}

	if (m_pDefaultFont)
	{
		m_pDefaultFont->Release();
		m_pDefaultFont = nullptr;
	}

	if (m_pLine)
	{
		m_pLine->Release();
		m_pLine = nullptr;
	}

	if (m_pTextSprite)
	{
		m_pTextSprite->Release();
		m_pTextSprite = nullptr;
	}

	if (m_pFont)
	{
		delete m_pFont;
		m_pFont = nullptr;
	}

	if (m_imFontTexture)
	{
		m_imFontTexture->Release();
		m_imFontTexture = nullptr;
	}

	if (m_imDrawList)
	{
		delete m_imDrawList;
		m_imDrawList = nullptr;
	}

	m_imFonts.Clear();
	m_hasDelayDrawing.unlock();
}

void DrawManager::CreateObjects()
{
	m_hasDelayDrawing.lock();
	
	if (FAILED(m_pDevice->CreateStateBlock(D3DSBT_ALL, &m_pStateBlock)))
	{
		throw std::exception("Failed to create the state block");
	}

	if (FAILED(D3DXCreateFontA(m_pDevice, m_iFontSize, 0, FW_BOLD, 1, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
		DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Tahoma", &m_pDefaultFont)))
	{
		throw std::exception("Failed to create the default font");
	}

	if (FAILED(D3DXCreateLine(m_pDevice, &m_pLine)))
	{
		throw std::exception("Failed to create the line");
	}

	if (FAILED(D3DXCreateSprite(m_pDevice, &m_pTextSprite)))
	{
		throw std::exception("Failed to create the sprite");
	}

#ifndef ORIGINAL_CD3DFONT
	m_pFont = new CD3DFont("Tahoma", m_iFontSize);
	m_pFont->InitializeDeviceObjects(m_pDevice);
	m_pFont->RestoreDeviceObjects();
#else
	m_pFont = new CD3DFont(L"Tahoma", m_iFontSize);
	m_pFont->InitDeviceObjects(m_pDevice);
	m_pFont->RestoreDeviceObjects();
#endif

#ifdef IMGUI_VERSION
	m_imDrawList = new ImDrawList();

	char systemPath[MAX_PATH];
	GetWindowsDirectoryA(systemPath, MAX_PATH);

	std::string fontPath(systemPath);
	fontPath += "\\Fonts\\msyhl.ttc";

	Utils::log("font %s loading...", fontPath.c_str());
	m_imFonts.AddFontFromFileTTF(fontPath.c_str(), m_iFontSize, nullptr, m_imFonts.GetGlyphRangesChinese());
	// ImGui::GetIO().Fonts->AddFontFromFileTTF(fontPath.data(), m_iFontSize, nullptr, m_imFonts.GetGlyphRangesChinese());
	
	uint8_t* pixel_data;
	int width, height, bytes_per_pixel;
	m_imFonts.GetTexDataAsRGBA32(&pixel_data, &width, &height, &bytes_per_pixel);

	if (FAILED(m_pDevice->CreateTexture(width, height, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8,
		D3DPOOL_DEFAULT, &m_imFontTexture, nullptr)))
	{
		throw std::exception("Failed to create the texture");
	}

	D3DLOCKED_RECT textureLock;
	if (FAILED(m_imFontTexture->LockRect(0, &textureLock, nullptr, 0)))
	{
		throw std::exception("Failed to lock the texture");
	}

	for (int y = 0; y < height; y++)
	{
		memcpy_s((uint8_t*)textureLock.pBits + textureLock.Pitch * y, (width * bytes_per_pixel),
			pixel_data + (width * bytes_per_pixel) * y, (width * bytes_per_pixel));
	}

	m_imFontTexture->UnlockRect(0);
	m_imFonts.TexID = m_imFontTexture;
#endif

	m_hasDelayDrawing.unlock();
}

std::string DrawManager::FindFonts(const std::string & name)
{
	//
	// This code is not as safe as it should be.
	// Assumptions we make:
	//  -> GetWindowsDirectoryA does not fail.
	//  -> The registry key exists.
	//  -> The subkeys are ordered alphabetically
	//  -> The subkeys name and data are no longer than 260 (MAX_PATH) chars.
	//

	char buffer[MAX_PATH];
	HKEY registryKey;

	GetWindowsDirectoryA(buffer, MAX_PATH);
	std::string fontsFolder = buffer + std::string("\\Fonts\\");

	if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts", 0,
		KEY_READ, &registryKey))
	{
		return "";
	}

	uint32_t valueIndex = 0;
	char valueName[MAX_PATH];
	uint8_t valueData[MAX_PATH];
	std::wstring wsFontFile;

	for(;;)
	{
		uint32_t valueNameSize = MAX_PATH;
		uint32_t valueDataSize = MAX_PATH;
		uint32_t valueType;

		auto error = RegEnumValueA(registryKey, valueIndex, valueName,
			reinterpret_cast<DWORD*>(&valueNameSize), 0, reinterpret_cast<DWORD*>(&valueType),
			valueData, reinterpret_cast<DWORD*>(&valueDataSize));

		valueIndex++;

		if (error == ERROR_NO_MORE_ITEMS)
		{
			RegCloseKey(registryKey);
			break;
		}

		if (error || valueType != REG_SZ)
		{
			continue;
		}

		if (_strnicmp(name.data(), valueName, name.size()) == 0)
		{
			RegCloseKey(registryKey);
			return (fontsFolder + std::string((char*)valueData, valueDataSize));
		}
	}

	return "";
}

void DrawManager::BeginImGuiRender()
{
	ImGui_ImplDX9_NewFrame();
	m_imDrawData.Valid = false;
}

void DrawManager::FinishImGuiRender()
{
	if (!m_imDrawList->VtxBuffer.empty())
	{
		m_imDrawData.Valid = true;
		m_imDrawData.CmdLists = &m_imDrawList;
		m_imDrawData.CmdListsCount = 1;
		m_imDrawData.TotalIdxCount = m_imDrawList->IdxBuffer.Size;
		m_imDrawData.TotalVtxCount = m_imDrawList->VtxBuffer.Size;
	}
	else
	{
		// 不要进行绘制
		m_imDrawData.Valid = false;
	}

	m_hasDelayDrawing.lock();
	ImGui_ImplDX9_RenderDrawLists(&m_imDrawData);
	ImGui::Render();
	
	m_imDrawList->Clear();
	m_imDrawList->PushClipRectFullScreen();
	m_hasDelayDrawing.unlock();
}

void DrawManager::BeginRendering()
{
	if (m_bRenderRunning)
		return;

	m_bRenderRunning = true;
	m_pStateBlock->Capture();

	m_pDevice->SetTexture(0, nullptr);
	m_pDevice->SetPixelShader(nullptr);
	m_pDevice->SetVertexShader(nullptr);
	m_pDevice->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);

	// 修复颜色不正确，某些东西绘制不出来
	m_pDevice->SetRenderState(D3DRS_COLORWRITEENABLE, 0xFFFFFFFF);
	m_pDevice->SetRenderState(D3DRS_LIGHTING, false);
	m_pDevice->SetRenderState(D3DRS_FOGENABLE, false);
	m_pDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
	m_pDevice->SetRenderState(D3DRS_ALPHATESTENABLE, false);
	m_pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	m_pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
	m_pDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, true);
	m_pDevice->SetRenderState(D3DRS_ZWRITEENABLE, false);
	m_pDevice->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, true);
	m_pDevice->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, true);
	m_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
	m_pDevice->SetRenderState(D3DRS_SRGBWRITEENABLE, false);
	m_pDevice->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, true);
	m_pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	m_pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	m_pDevice->SetRenderState(D3DRS_SRCBLENDALPHA, D3DBLEND_INVDESTALPHA);
	m_pDevice->SetRenderState(D3DRS_DESTBLENDALPHA, D3DBLEND_ONE);
}

void DrawManager::EndRendering()
{
	if (!m_bRenderRunning)
		return;

	m_bRenderRunning = false;
	
	m_hasDelayDrawing.lock();
	try
	{
		this->DrawQueueObject();
	}
	catch (std::exception e)
	{
		Utils::log("%s (%d): %s", __FILE__, __LINE__, e.what());
		this->m_delayDraw.clear();
		this->m_delayString.clear();
		this->m_textDrawQueue.clear();
	}
	m_hasDelayDrawing.unlock();

	// 将设备状态重置，防止原来的绘制出现问题
	m_pStateBlock->Apply();
}

void DrawManager::DrawQueueObject()
{
	if (!m_textDrawQueue.empty() || !m_delayString.empty())
	{
#ifndef ORIGINAL_CD3DFONT
		// 开始批量绘制
		m_pFont->BeginDrawing();
#endif
		
		int drawQueue = -1;
		time_t currentTime = time(nullptr);
		auto i = m_textDrawQueue.begin();;
		for (; i != m_textDrawQueue.end(); )
		{
#ifndef ORIGINAL_CD3DFONT
			m_pFont->DrawText(10.0f, m_iFontSize * ++drawQueue + 12.0f, i->color, i->text.c_str());
#else
			m_pFont->DrawText(25.0f, 15.0f * drawQueue++, i->color, Utils::c2w(i->text).c_str());
#endif

			if (i->destoryTime <= currentTime)
				i = m_textDrawQueue.erase(i);
			else
				++i;
		}

		RECT rect;
		m_pTextSprite->Begin(D3DXSPRITE_ALPHABLEND | D3DXSPRITE_SORT_TEXTURE);
		for (const auto& each : m_delayString)
		{
			if (each.text.empty())
				continue;

			if (each.flags & D3DFONT_ORIGINAL)
			{
				if (each.flags & D3DFONT_CENTERED)
				{
					rect = { 0, 0, 0, 0 };
					
					// 获取左上角位置 (不绘制)
					m_pDefaultFont->DrawTextA(m_pTextSprite, each.text.c_str(), each.text.length(),
						&rect, DT_NOCLIP | DT_CALCRECT, each.color);

					// 计算中心位置
					rect = { ((LONG)each.x) - rect.right / 2, ((LONG)each.y), 0, 0 };
				}
				else
				{
					// 左上角位置
					rect = { ((LONG)each.x), ((LONG)each.y), 1000, 100 };
				}

				m_pDefaultFont->DrawTextA(m_pTextSprite, each.text.c_str(), each.text.length(),
					&rect, DT_TOP | DT_LEFT | DT_NOCLIP, each.color);
			}
			else
			{
#ifndef ORIGINAL_CD3DFONT
				m_pFont->DrawText(each.x, each.y, each.color,
					each.text.c_str(), each.flags, each.background);
#else
				m_pFont->DrawText(each.x, each.y, each.color,
					Utils::c2w(each.text).c_str(), each.flags);
#endif
			}
		}

		m_delayString.clear();

#ifndef ORIGINAL_CD3DFONT
		// 完成批量绘制
		m_pFont->EndDrawing();
#endif
		m_pTextSprite->End();
	}
	
	// 延迟绘制图形
	if (!m_delayDraw.empty())
	{
		// 优化绘制速度
		m_pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);

		for (const auto& each : m_delayDraw)
		{
			if (each.vertex.empty() || each.vertexCount <= 0)
				continue;

			m_pDevice->DrawPrimitiveUP(each.type, each.vertexCount,
				&(each.vertex[0]), sizeof(D3DVertex));
		}

		m_delayDraw.clear();
		// m_pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	}
}

void DrawManager::RenderLine(D3DCOLOR color, int x1, int y1, int x2, int y2)
{
	D3DVertex vertices[2] =
	{
		D3DVertex((float)x1, (float)y1, 1.0f, color),
		D3DVertex((float)x2, (float)y2, 1.0f, color)
	};

	m_pDevice->DrawPrimitiveUP(D3DPT_LINELIST, 1, vertices, sizeof(D3DVertex));
}

void DrawManager::AddLine(D3DCOLOR color, int x1, int y1, int x2, int y2)
{
	if (!m_hasDelayDrawing.try_lock())
		return;

#ifdef IMGUI_VERSION
	__try
	{
		m_imDrawList->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), D3DCOLOR_IMGUI(color));
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		Utils::log("%s (%d): 0x%X", __FILE__, __LINE__, GetExceptionCode());
	}
#else
	this->m_delayDraw.emplace_back(D3DPT_LINELIST, 1, std::vector<D3DVertex>{
		D3DVertex((float)x1, (float)y1, 1.0f, color),
		D3DVertex((float)x2, (float)y2, 1.0f, color)
	});
#endif

	m_hasDelayDrawing.unlock();
}

void DrawManager::RenderRect(D3DCOLOR color, int x, int y, int w, int h)
{
	D3DVertex vertices[5] =
	{
		D3DVertex((float)x, (float)y, 1.0f, color),
		D3DVertex((float)(x + w), (float)y, 1.0f, color),
		D3DVertex((float)(x + w), (float)(y + h), 1.0f, color),
		D3DVertex((float)x, (float)(y + h), 1.0f, color),
		D3DVertex((float)x, (float)y, 1.0f, color)
	};
	m_pDevice->DrawPrimitiveUP(D3DPT_LINESTRIP, 4, vertices, sizeof(D3DVertex));
}

void DrawManager::AddRect(D3DCOLOR color, int x, int y, int w, int h)
{
	if (!m_hasDelayDrawing.try_lock())
		return;
	
#ifdef IMGUI_VERSION
	__try
	{
		m_imDrawList->AddRect(ImVec2(x, y), ImVec2(x + w, y + h), D3DCOLOR_IMGUI(color));
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		Utils::log("%s (%d): 0x%X", __FILE__, __LINE__, GetExceptionCode());
	}
#else
	this->m_delayDraw.emplace_back(D3DPT_LINESTRIP, 4, std::vector<D3DVertex>{
		D3DVertex((float)x, (float)y, 1.0f, color),
		D3DVertex((float)(x + w), (float)y, 1.0f, color),
		D3DVertex((float)(x + w), (float)(y + h), 1.0f, color),
		D3DVertex((float)x, (float)(y + h), 1.0f, color),
		D3DVertex((float)x, (float)y, 1.0f, color)
	});
#endif

	m_hasDelayDrawing.unlock();
}

#ifndef M_PI_F
#define M_PI		3.14159265358979323846
#define M_PI_F		((float)(M_PI))
#endif

void DrawManager::RenderCircle(D3DCOLOR color, int x, int y, int r, size_t resolution)
{
	float curAngle;
	float angle = (float)((2.0f * M_PI_F) / resolution);
	std::vector<D3DVertex> vertices;
	for (size_t i = 0; i <= resolution; ++i)
	{
		curAngle = i * angle;
		vertices.emplace_back(x + r * cos(curAngle), y - r * sin(curAngle), 0.0f, color);
	}

	m_pDevice->DrawPrimitiveUP(D3DPT_LINESTRIP, resolution, &(vertices[0]), sizeof(D3DVertex));
}

void DrawManager::RenderFillCircle(D3DCOLOR color, int x, int y, int r, size_t resolution)
{
	float curAngle;
	float angle = (float)((2.0f * M_PI_F) / resolution);
	std::vector<D3DVertex> vertices;
	for (size_t i = 0; i <= resolution; ++i)
	{
		curAngle = i * angle;
		vertices.emplace_back(x + r * cos(curAngle), y - r * sin(curAngle), 0.0f, color);
	}

	m_pDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, resolution, &(vertices[0]), sizeof(D3DVertex));
}

void DrawManager::AddCircle(D3DCOLOR color, int x, int y, int r, size_t resolution)
{
	if (!m_hasDelayDrawing.try_lock())
		return;

#ifdef IMGUI_VERSION
	__try
	{
		m_imDrawList->AddCircle(ImVec2(x, y), r, D3DCOLOR_IMGUI(color), resolution);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		Utils::log("%s (%d): 0x%X", __FILE__, __LINE__, GetExceptionCode());
	}
#else
	float curAngle;
	float angle = (float)((2.0f * M_PI_F) / resolution);
	std::vector<D3DVertex> vertices;
	for (size_t i = 0; i <= resolution; ++i)
	{
		curAngle = i * angle;
		vertices.emplace_back(x + r * cos(curAngle), y - r * sin(curAngle), 0.0f, color);
	}

	this->m_delayDraw.emplace_back(D3DPT_LINESTRIP, resolution, std::move(vertices));
#endif

	m_hasDelayDrawing.unlock();
}

void DrawManager::AddFillCircle(D3DCOLOR color, int x, int y, int r, size_t resolution)
{
	if (!m_hasDelayDrawing.try_lock())
		return;

#ifdef IMGUI_VERSION
	__try
	{
		m_imDrawList->AddCircleFilled(ImVec2(x, y), r, D3DCOLOR_IMGUI(color), resolution);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		Utils::log("%s (%d): 0x%X", __FILE__, __LINE__, GetExceptionCode());
	}
#else
	float curAngle;
	float angle = (float)((2.0f * M_PI_F) / resolution);
	std::vector<D3DVertex> vertices;
	for (size_t i = 0; i <= resolution; ++i)
	{
		curAngle = i * angle;
		vertices.emplace_back(x + r * cos(curAngle), y - r * sin(curAngle), 0.0f, color);
	}

	this->m_delayDraw.emplace_back(D3DPT_TRIANGLEFAN, resolution, std::move(vertices));
#endif

	m_hasDelayDrawing.unlock();
}

void DrawManager::RenderText(D3DCOLOR color, int x, int y, bool centered, const char* fmt, ...)
{
	char buffer[512];
	va_list args;
	va_start(args, fmt);
	int len = vsprintf_s(buffer, fmt, args);
	va_end(args);

	if (centered)
	{
		RECT rec = { 0, 0, 0, 0 };
		m_pDefaultFont->DrawTextA(NULL, buffer, len, &rec, DT_CALCRECT | DT_NOCLIP, color);
		rec = { x - rec.right / 2, y, 0, 0 };

		/*
		rec.left++;
		m_pDefaultFont->DrawTextA(NULL, buffer, len, &rec, DT_TOP | DT_LEFT | DT_NOCLIP, 0xFF000000);
		rec.top++;
		m_pDefaultFont->DrawTextA(NULL, buffer, len, &rec, DT_TOP | DT_LEFT | DT_NOCLIP, 0xFF000000);
		*/

		m_pDefaultFont->DrawTextA(NULL, buffer, len, &rec, DT_TOP | DT_LEFT | DT_NOCLIP, color);

	}
	else
	{
		RECT rec = { x, y, 1000, 100 };

		/*
		rec.left++;
		m_pDefaultFont->DrawTextA(NULL, buffer, len, &rec, DT_TOP | DT_LEFT | DT_NOCLIP, 0xFF000000);
		rec.top++;
		m_pDefaultFont->DrawTextA(NULL, buffer, len, &rec, DT_TOP | DT_LEFT | DT_NOCLIP, 0xFF000000);
		*/

		m_pDefaultFont->DrawTextA(NULL, buffer, len, &rec, DT_TOP | DT_LEFT | DT_NOCLIP, color);
	}
}

void DrawManager::RenderText(D3DCOLOR color, int x, int y, bool centered, const wchar_t * fmt, ...)
{
	wchar_t buffer[512];
	va_list args;
	va_start(args, fmt);
	int len = vswprintf_s(buffer, fmt, args);
	va_end(args);

	if (centered)
	{
		RECT rec = { 0, 0, 0, 0 };
		m_pDefaultFont->DrawTextW(NULL, buffer, len, &rec, DT_CALCRECT | DT_NOCLIP, color);
		rec = { x - rec.right / 2, y, 0, 0 };

		/*
		rec.left++;
		m_pDefaultFont->DrawTextW(NULL, buffer, len, &rec, DT_TOP | DT_LEFT | DT_NOCLIP, 0xFF000000);
		rec.top++;
		m_pDefaultFont->DrawTextW(NULL, buffer, len, &rec, DT_TOP | DT_LEFT | DT_NOCLIP, 0xFF000000);
		*/

		m_pDefaultFont->DrawTextW(NULL, buffer, len, &rec, DT_TOP | DT_LEFT | DT_NOCLIP, color);
	}
	else
	{
		RECT rec = { x, y, 1000, 100 };

		/*
		rec.left++;
		m_pDefaultFont->DrawTextW(NULL, buffer, len, &rec, DT_TOP | DT_LEFT | DT_NOCLIP, 0xFF000000);
		rec.top++;
		m_pDefaultFont->DrawTextW(NULL, buffer, len, &rec, DT_TOP | DT_LEFT | DT_NOCLIP, 0xFF000000);
		*/

		m_pDefaultFont->DrawTextW(NULL, buffer, len, &rec, DT_TOP | DT_LEFT | DT_NOCLIP, color);
	}
}

void DrawManager::RenderFillRect(D3DCOLOR color, int x, int y, int w, int h)
{
	D3DVertex vertices[4] =
	{
		D3DVertex((float)x, (float)y, 1.0f, color),
		D3DVertex((float)x, (float)(y + h), 1.0f, color),
		D3DVertex((float)(x + w), (float)y, 1.0f, color),
		D3DVertex((float)(x + w), (float)(y + h), 1.0f, color)
	};

	m_pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, &vertices[0], sizeof(D3DVertex));
}

void DrawManager::AddFillRect(D3DCOLOR color, int x, int y, int w, int h)
{
	if (!m_hasDelayDrawing.try_lock())
		return;

#ifdef IMGUI_VERSION
	__try
	{
		m_imDrawList->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), D3DCOLOR_IMGUI(color));
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		Utils::log("%s (%d): 0x%X", __FILE__, __LINE__, GetExceptionCode());
	}
#else
	this->m_delayDraw.emplace_back(D3DPT_TRIANGLESTRIP, 2, std::vector<D3DVertex>{
		D3DVertex((float)x, (float)y, 1.0f, color),
		D3DVertex((float)x, (float)(y + h), 1.0f, color),
		D3DVertex((float)(x + w), (float)y, 1.0f, color),
		D3DVertex((float)(x + w), (float)(y + h), 1.0f, color)
	});
#endif

	m_hasDelayDrawing.unlock();
}

#ifndef ORIGINAL_CD3DFONT
HRESULT DrawManager::DrawString2(float x, float y, D3DCOLOR color, const char * text, ...)
{
	va_list ap;
	va_start(ap, text);

	char buffer[1024];
	vsprintf_s(buffer, text, ap);

	va_end(ap);

#undef DrawText
	return m_pFont->DrawText(x, y, color, buffer, 0);
}

void DrawManager::AddText(D3DCOLOR color, int x, int y, bool centered, const char * fmt, ...)
{
	if (!m_hasDelayDrawing.try_lock())
		return;
	
	va_list ap;
	va_start(ap, fmt);

	char buffer[1024];
	vsprintf_s(buffer, fmt, ap);

	va_end(ap);

#ifdef IMGUI_VERSION
	ImFont* font = m_imFonts.Fonts[0];
	ImVec2 textSize = font->CalcTextSizeA(font->FontSize, FLT_MAX, 0.0f, buffer);

	if (centered)
	{
		x -= textSize.x / 2;
		y -= textSize.y / 2;
	}

	m_imDrawList->PushTextureID(m_imFontTexture);
	__try
	{
		m_imDrawList->AddText(font, font->FontSize, ImVec2(x, y), D3DCOLOR_IMGUI(color), buffer);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		Utils::log("%s (%d): 0x%X", __FILE__, __LINE__, GetExceptionCode());
	}
	m_imDrawList->PopTextureID();
#else
	this->m_delayString.emplace_back(x, y, std::move(buffer), color, (centered ? D3DFONT_CENTERED : 0), 0);
#endif

	m_hasDelayDrawing.unlock();
}

void DrawManager::AddString(D3DCOLOR color, int x, int y, bool centered, const char * fmt, ...)
{
	if (!m_hasDelayDrawing.try_lock())
		return;

	va_list ap;
	va_start(ap, fmt);

	char buffer[1024];
	vsprintf_s(buffer, fmt, ap);

	va_end(ap);

	this->m_delayString.emplace_back(x, y, std::move(buffer), color, (centered ? D3DFONT_CENTERED|D3DFONT_ORIGINAL : D3DFONT_ORIGINAL), 0);
	m_hasDelayDrawing.unlock();
}

void DrawManager::RenderCorner(D3DCOLOR color, int x, int y, int w, int h, size_t length)
{
	if (length == 0)
	{
		if (w == 0)
			return;

		length = w / 3;
	}

	// 左上
	this->RenderLine(color, x, y, x + length, y);
	this->RenderLine(color, x, y, x, y - length);

	// 右上
	this->RenderLine(color, x + w, y, x + w - length, y);
	this->RenderLine(color, x + w, y, x + w, y - length);

	// 左下
	this->RenderLine(color, x, y + h, x + length, y + h);
	this->RenderLine(color, x, y + h, x, y + h + length);

	// 右下
	this->RenderLine(color, x + w, y + h, x + w - length, y + h);
	this->RenderLine(color, x + w, y + h, x + w, y + h + length);
}

void DrawManager::AddCorner(D3DCOLOR color, int x, int y, int w, int h, size_t length)
{
	if (length == 0)
	{
		if (w == 0)
			return;

		length = w / 3;
	}

	// 左上
	this->AddLine(color, x, y, x + length, y);
	this->AddLine(color, x, y, x, y - length);

	// 右上
	this->AddLine(color, x + w, y, x + w - length, y);
	this->AddLine(color, x + w, y, x + w, y - length);

	// 左下
	this->AddLine(color, x, y + h, x + length, y + h);
	this->AddLine(color, x, y + h, x, y + h + length);

	// 右下
	this->AddLine(color, x + w, y + h, x + w - length, y + h);
	this->AddLine(color, x + w, y + h, x + w, y + h + length);
}

inline HRESULT DrawManager::DrawString2Begin()
{
	return this->m_pFont->BeginDrawing();
}

inline HRESULT DrawManager::DrawString2Finish()
{
	return this->m_pFont->EndDrawing();
}

#else
void DrawManager::DrawString2(float x, float y, D3DCOLOR color, const wchar_t * text, ...)
{
	va_list ap;
	va_start(ap, text);

	wchar_t buffer[1024];
	int len = vswprintf_s(buffer, text, ap);

	va_end(ap);

#undef DrawText
	m_pFont->DrawText(x, y, color, buffer, D3DFONT_CENTERED_X | D3DFONT_CENTERED_Y);
}
#endif

void DrawManager::DrawString(int x, int y, D3DCOLOR color, const char * text, ...)
{
	va_list ap;
	va_start(ap, text);

	char buffer[1024];
	int len = vsprintf_s(buffer, text, ap);

	va_end(ap);

	RECT Position;
	/*
	Position.left = x + 1;
	Position.top = y + 1;
	m_pDefaultFont->DrawTextA(0, buffer, len, &Position, DT_NOCLIP, D3DCOLOR_ARGB(255, 0, 0, 0));
	*/
	Position.left = x;
	Position.top = y;

	m_pTextSprite->Begin(D3DXSPRITE_ALPHABLEND);
	m_pDefaultFont->DrawTextA(m_pTextSprite, buffer, len, &Position, DT_CENTER | DT_NOCLIP, color);
	m_pTextSprite->End();
}

void DrawManager::DrawString(int x, int y, D3DCOLOR color, const wchar_t * text, ...)
{
	va_list ap;
	va_start(ap, text);

	wchar_t buffer[1024];
	int len = vswprintf_s(buffer, text, ap);

	va_end(ap);

	RECT Position;
	/*
	Position.left = x + 1;
	Position.top = y + 1;
	m_pDefaultFont->DrawTextA(0, buffer, len, &Position, DT_NOCLIP, D3DCOLOR_ARGB(255, 0, 0, 0));
	*/
	Position.left = x;
	Position.top = y;

	m_pTextSprite->Begin(D3DXSPRITE_ALPHABLEND);
	m_pDefaultFont->DrawTextW(m_pTextSprite, buffer, len, &Position, DT_CENTER | DT_NOCLIP, color);
	m_pTextSprite->End();
}

void DrawManager::DrawRect(int x, int y, int width, int height, D3DCOLOR color)
{
	D3DXVECTOR2 Rect[5];
	Rect[0] = D3DXVECTOR2((float)x, (float)y);
	Rect[1] = D3DXVECTOR2((float)(x + width), (float)y);
	Rect[2] = D3DXVECTOR2((float)(x + width), (float)(y + height));
	Rect[3] = D3DXVECTOR2((float)x, (float)(y + height));
	Rect[4] = D3DXVECTOR2((float)x, (float)y);
	m_pLine->SetWidth(1);
	m_pLine->Draw(Rect, 5, color);
}

void DrawManager::DrawBorderedRect(int x, int y, int width, int height, D3DCOLOR filled, D3DCOLOR color)
{
	D3DXVECTOR2 Fill[2];
	Fill[0] = D3DXVECTOR2((float)(x + width / 2), (float)y);
	Fill[1] = D3DXVECTOR2((float)(x + width / 2), (float)(y + height));
	m_pLine->SetWidth((float)width);
	m_pLine->Draw(Fill, 2, filled);

	D3DXVECTOR2 Rect[5];
	Rect[0] = D3DXVECTOR2((float)x, (float)y);
	Rect[1] = D3DXVECTOR2((float)(x + width), (float)y);
	Rect[2] = D3DXVECTOR2((float)(x + width), (float)(y + height));
	Rect[3] = D3DXVECTOR2((float)x, (float)(y + height));
	Rect[4] = D3DXVECTOR2((float)x, (float)y);
	m_pLine->SetWidth(1);
	m_pLine->Draw(Rect, 5, color);
}

void DrawManager::DrawLine(int x, int y, int x2, int y2, D3DCOLOR color)
{
	D3DXVECTOR2 Line[2];
	Line[0] = D3DXVECTOR2((float)x, (float)y);
	Line[1] = D3DXVECTOR2((float)x2, (float)y2);
	m_pLine->SetWidth(1.0f);
	m_pLine->Draw(Line, 2, color);
}

void DrawManager::DrawFilledRect(int x, int y, int width, int height, D3DCOLOR color)
{
	D3DXVECTOR2 Rect[2];
	Rect[0] = D3DXVECTOR2((float)(x + width / 2), (float)y);
	Rect[1] = D3DXVECTOR2((float)(x + width / 2), (float)(y + height));
	m_pLine->SetWidth((float)width);
	m_pLine->Draw(Rect, 2, color);
}

void DrawManager::DrawCircle(int x, int y, int radius, D3DCOLOR color)
{
	D3DXVECTOR2 DotPoints[128];
	D3DXVECTOR2 Line[128];
	int Points = 0;
	for (float i = 0; i < M_PI * 2.0f; i += 0.1f)
	{
		float PointOriginX = x + radius * cosf(i);
		float PointOriginY = y + radius * sinf(i);
		float PointOriginX2 = radius * cosf(i + 0.1f) + x;
		float PointOriginY2 = radius * sinf(i + 0.1f) + y;
		DotPoints[Points] = D3DXVECTOR2(PointOriginX, PointOriginY);
		DotPoints[Points + 1] = D3DXVECTOR2(PointOriginX2, PointOriginY2);
		Points += 2;
	}

	m_pLine->Draw(DotPoints, Points, color);
}

void DrawManager::PushRenderText(D3DCOLOR color, const char* text, ...)
{
	va_list ap;
	va_start(ap, text);

	char buffer[1024];
	vsprintf_s(buffer, text, ap);

	va_end(ap);

	this->m_textDrawQueue.emplace_back(color, std::move(buffer), 5);
}

inline int DrawManager::GetFontSize()
{
	return m_iFontSize;
}

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
		if (w2c(lpModuleEntry.szModule) == ModuleName)
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
		if (w2c(lpModuleEntry.szModule) == ModuleName)
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
		if (w2c(pe.szExeFile) == proccessName)
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
	static std::string path;
	if (path.empty())
	{
		char buffer[MAX_PATH];
		GetModuleFileNameA(g_hMyInstance, buffer, MAX_PATH);
		std::string tmp = buffer;
		path = tmp.substr(0, tmp.rfind('\\'));
	}

	return path;
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
	g_interface.Engine->ClientCmd("echo \"%s\"", w2c(buffer));
	std::wcout << buffer << std::endl;
}
