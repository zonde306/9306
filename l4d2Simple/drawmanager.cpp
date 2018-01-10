#include "drawmanager.h"
#include "menu.h"

// 将 D3DCOLOR 转换为 ImGui 的颜色
// DirectX 的颜色为 ARGB。ImGui 的颜色为 ABGR
#define D3DCOLOR_IMGUI(_clr)	((_clr & 0xFF000000) | ((_clr & 0x00FF0000) >> 16) | (_clr & 0x0000FF00) | ((_clr & 0x000000FF) << 16))

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

void DrawManager::OnResetDevice(int width, int height)
{
	CreateObjects();
	m_bRenderRunning = false;
	m_bImIsFirst = false;

	ImGui::GetIO().DisplaySize.x = (float)width;
	ImGui::GetIO().DisplaySize.y = (float)height;
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
	ImGui::GetIO().Fonts->Clear();
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
	m_imDrawList = new ImDrawList(ImGui::GetDrawListSharedData());

	char systemPath[MAX_PATH];
	GetWindowsDirectoryA(systemPath, MAX_PATH);

	std::string fontPath(systemPath);
	fontPath += "\\Fonts\\msyhl.ttc";

	// Utils::log("font %s loading...", fontPath.c_str());
	m_imFonts.AddFontFromFileTTF(fontPath.c_str(), (float)m_iFontSize, nullptr, m_imFonts.GetGlyphRangesChinese());
	ImGui::GetIO().Fonts->AddFontFromFileTTF(fontPath.data(), m_iFontSize, nullptr, m_imFonts.GetGlyphRangesChinese());

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

	for (;;)
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

	if(g_bIsShowMenu && g_pBaseMenu)
		g_pBaseMenu->DrawMenu();

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
		// Utils::log("%s (%d): %s", __FILE__, __LINE__, e.what());
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
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		// Utils::log("%s (%d): 0x%X", __FILE__, __LINE__, GetExceptionCode());
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
		// Utils::log("%s (%d): 0x%X", __FILE__, __LINE__, GetExceptionCode());
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
		// Utils::log("%s (%d): 0x%X", __FILE__, __LINE__, GetExceptionCode());
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
		// Utils::log("%s (%d): 0x%X", __FILE__, __LINE__, GetExceptionCode());
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
		// Utils::log("%s (%d): 0x%X", __FILE__, __LINE__, GetExceptionCode());
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

	if (m_imDrawList->_ClipRectStack.empty())
		m_imDrawList->PushClipRectFullScreen();

	m_imDrawList->PushTextureID(m_imFontTexture);
	__try
	{
		m_imDrawList->AddText(font, font->FontSize, ImVec2(x, y), D3DCOLOR_IMGUI(color), buffer);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		// Utils::log("%s (%d): 0x%X", __FILE__, __LINE__, GetExceptionCode());
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

	this->m_delayString.emplace_back(x, y, std::move(buffer), color, (centered ? D3DFONT_CENTERED | D3DFONT_ORIGINAL : D3DFONT_ORIGINAL), 0);
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

int DrawManager::GetFontSize()
{
	return m_iFontSize;
}

