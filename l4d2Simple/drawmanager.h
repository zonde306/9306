#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <ctime>
#include <d3d9.h>
#include <d3dx9.h>
#include "../imgui/dx9/imgui_impl_dx9.h"
#include "./d3dfont/D3DFont.h"
#include "./structs/vector.h"

class DrawManager
{
public:
	DrawManager(IDirect3DDevice9* pDevice, int fontSize = 14);
	~DrawManager();

	// 在 Reset 之前调用
	void OnLostDevice();

	// 在 Reset 之后调用
	void OnResetDevice(int width, int height);

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
	int GetFontSize();

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

		D3DVertex() : x(0.0f), y(0.0f), z(0.0f), rhw(0.0f), color(0)
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
