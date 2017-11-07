#pragma once
#include <Windows.h>
#include <cstdio>
#include "../indexes.h"
#include "../libraries/vmt.h"

enum FontDrawType_t
{
	// Use the "additive" value from the scheme file
	FONT_DRAW_DEFAULT = 0,

	// Overrides
	FONT_DRAW_NONADDITIVE,
	FONT_DRAW_ADDITIVE,

	FONT_DRAW_TYPE_COUNT = 2,
};

struct IntRect
{
	int x;
	int y;
	int w;
	int h;
};

class CSurface // : public IAppSystem
{
public:
	void DrawSetColor(int r, int g, int b, int a)
	{
		typedef void(__thiscall* Fn)(void*, int, int, int, int);
		return ((Fn)VMT.GetFunction(this, indexes::DrawSetColor))(this, r, g, b, a);
	}

	void DrawFilledRect( int x, int y, int x2, int y2 )
	{
		typedef void(__thiscall* Fn)(void*, int, int, int, int);
		return ((Fn)VMT.GetFunction(this, indexes::DrawFilledRect))(this, x, y, x2, y2);
	}

	void DrawFilledRectArray(IntRect* pRects, int numRects)
	{
		typedef void(__thiscall* Fn)(void*, void*, int);
		return ((Fn)VMT.GetFunction(this, indexes::DrawFilledRectArray))(this, pRects, numRects);
	}

	void DrawOutlinedRect(int x, int y, int x2, int y2)
	{
		typedef void(__thiscall* Fn)(void*, int, int, int, int);
		return ((Fn)VMT.GetFunction(this, indexes::DrawOutlinedRect))(this, x, y, x2, y2);
	}

	void DrawLine(int x, int y, int x2, int y2)
	{
		typedef void(__thiscall* Fn)(void*, int, int, int, int);
		return ((Fn)VMT.GetFunction(this, indexes::DrawLine))(this, x, y, x2, y2);
	}

	void DrawPolyLine(int* px, int* py, int numPoints)
	{
		typedef void(__thiscall* Fn)(void*, int*, int*, int);
		return ((Fn)VMT.GetFunction(this, indexes::DrawPolyLine))(this, px, py, numPoints);
	}

	unsigned int SCreateFont()
	{
		typedef unsigned int(__thiscall* Fn)(void*);
		return ((Fn)VMT.GetFunction(this, indexes::SCreateFont))(this);
	}

	bool SetFontGlyphSet(unsigned int font, const char *windowsFontName, int tall, int weight, int blur, int scanlines, int flags, int nRangeMin = 0, int nRangeMax = 0)
	{
		typedef bool(__thiscall* Fn)(void*, unsigned int, const char*, int, int, int, int, int, int, int);
		return ((Fn)VMT.GetFunction(this, indexes::SetFontGlyphSet))(this, font, windowsFontName, tall, weight, blur, scanlines, flags, nRangeMin, nRangeMax);
	}

	void DrawSetTextFont(unsigned int Font)
	{
		typedef void(__thiscall* Fn)(void*, unsigned int);
		return ((Fn)VMT.GetFunction(this, indexes::DrawSetTextFont))(this, Font);
	}

	void DrawSetTextColor(int r, int g, int b, int a)
	{
		typedef void(__thiscall* Fn)(void*, int, int, int, int);
		return ((Fn)VMT.GetFunction(this, indexes::DrawSetTextColor))(this, r, g, b, a);
	}

	void DrawSetTextPos( int x, int y )
	{
		typedef void(__thiscall* Fn)(void*, int, int);
		return ((Fn)VMT.GetFunction(this, indexes::DrawSetTextPos))(this, x, y);
	}

	void DrawPrintText(const wchar_t* Text, int Len, FontDrawType_t DrawType = FONT_DRAW_DEFAULT)
	{
		typedef void(__thiscall* Fn)(void*, wchar_t const*, int, int);
		return ((Fn)VMT.GetFunction(this, indexes::DrawPrintText))(this, Text, Len, DrawType);
	}

	void DrawUnicodeChar(wchar_t wch, FontDrawType_t drawType = FONT_DRAW_DEFAULT)
	{
		typedef void(__stdcall* Fn)(void*, wchar_t, int);
		((Fn)VMT.GetFunction(this, indexes::DrawUnicodeChar))(this, wch, drawType);
	}

	void DrawFlushText()
	{
		typedef void(__stdcall* Fn)(void*);
		((Fn)VMT.GetFunction(this, indexes::DrawFlushText))(this);
	}
	void DrawFilledRectFade(int x0, int y0, int x1, int y1, unsigned int alpha0, unsigned int alpha1, bool bHorizontal)
	{
		typedef void(__thiscall* Fn)(void*, int, int, int, int, unsigned int, unsigned int, bool);
		return ((Fn)VMT.GetFunction(this, indexes::DrawFilledRectFade))(this, x0, y0, x1, y1, alpha0, alpha1, bHorizontal);
	}

	void GetTextSize(unsigned int font, const wchar_t *text, int &wide, int &tall)
	{
		typedef void(__thiscall* Fn)(void*, unsigned int, const wchar_t*, int&, int&);
		return ((Fn)VMT.GetFunction(this, indexes::GetTextSize))(this, font, text, wide, tall);
	}

	bool AddCustomFontFile(const char *fontName)
	{
		typedef bool(__thiscall* Fn)(void*, const char*);
		return ((Fn)VMT.GetFunction(this, indexes::GetTextSize))(this, fontName);
	}

	void SetCursor(unsigned long cursor)
	{
		typedef void(__thiscall* Fn)(void*, unsigned long);
		return ((Fn)VMT.GetFunction(this, indexes::SetCursor))(this, cursor);
	}

	bool IsCursorVisible()
	{
		typedef bool(__thiscall* Fn)(void*);
		return ((Fn)VMT.GetFunction(this, indexes::IsCursorVisible))(this);
	}

	void UnlockCursor()
	{
		typedef void(__thiscall* Fn)(void*);
		return ((Fn)VMT.GetFunction(this, indexes::UnlockCursor))(this);
	}

	void LockCursor()
	{
		typedef void(__thiscall* Fn)(void*);
		return ((Fn)VMT.GetFunction(this, indexes::LockCursor))(this);
	}

	bool drawString(int x, int y, int r, int g, int b, unsigned long font, const wchar_t *pszText, ...)
	{
		wchar_t str[1024];
		va_list ap;
		va_start(ap, pszText);
		vswprintf_s(str, pszText, ap);
		va_end(ap);

		try
		{
			DrawSetTextPos(x, y);
			DrawSetTextFont(font);
			DrawSetTextColor(r, g, b, 255);
			DrawPrintText(str, wcslen(str));
		}
		catch(...)
		{
			// Utils::log("%s (%d) 错误：尝试绘制文本失败 0x%X", __FILE__, __LINE__, (DWORD)this);
			return false;
		}

		return true;
	}

	void FillRGBA(int x, int y, int w, int h, int r, int g, int b, int a)
	{
		DrawSetColor(r, g, b, a);
		DrawFilledRect(x, y, x + w, y + h);
	}

	void LineRGBA(int x1, int y1, int x2, int y2, int r, int g, int b, int a)
	{
		DrawSetColor(r, g, b, a);
		DrawLine(x1, y1, x2, y2);
	}

	void drawCrosshair(int x, int y, int r, int g, int b)
	{
		// Right
		FillRGBA(x, y, 9, 1, r, g, b, 255);
		// Bottom
		FillRGBA(x, y, 1, 9, r, g, b, 255);
		// Left
		FillRGBA(x - 9, y, 9, 1, r, g, b, 255);
		// Top
		FillRGBA(x, y - 9, 1, 9, r, g, b, 255);
	}

	void drawHeader(INT x, INT y, INT w, INT r, INT g, INT b, INT HealthBarWidth)
	{
		INT i = 0;
		INT xCoord = x;
		INT yCoord = y;

		// Male Male :)
		for (i = 0; i < 5; i++)
		{
			FillRGBA(x - i, y + i, w, 1, 0, 0, 0, 255);
			FillRGBA(x - i, y + i, HealthBarWidth + 2, 1, r, g, b, 255);

			FillRGBA(x - 3, y + 3, HealthBarWidth + 2, 1, (INT)(r / 1.5f),
				(INT)(g / 1.5f), (INT)(b / 1.5f), 255);

			FillRGBA(x - 4, y + 4, HealthBarWidth + 2, 1, (INT)(r / 1.5f),
				(INT)(g / 1.5f), (INT)(b / 1.5f), 255);
		}

		// Oben
		FillRGBA(x, y, w, 1, 255, 255, 255, 255);

		// Unten
		FillRGBA((x + 1) - 5, y + 5, w, 1, 255, 255, 255, 255);

		for (i = 0; i < 5; i++)
		{
			// Schr鋑 links
			FillRGBA(x, y, 1, 1, 255, 255, 255, 255);
			x--;
			y++;
		}

		x = xCoord;
		y = yCoord;

		for (i = 0; i < 5; i++)
		{
			// Schr鋑 rechts
			if (i != 0)
				FillRGBA(x + w, y, 1, 1, 255, 255, 255, 255);

			x--;
			y++;
		}

	}

	void drawHealthBox(int x, int y, int r, int g, int b, int a, int CurHealth, int MaxHealth)
	{
		if (CurHealth == 0 || MaxHealth == 0)
			return;

		if (CurHealth > MaxHealth)
			CurHealth = MaxHealth;

		float mx = MaxHealth / 2.0f;
		float wx = CurHealth / 2.0f;

		if (mx == 0)
			return;

		x -= (mx / 2);

		drawHeader(x, y, mx + 2, r, g, b, wx);
	}

	void boxESP(int x, int y, int radius, int R, int G, int B)
	{
		int iStep = (radius * 4);

		for (int i = 0; i < radius; i++)
		{
			// Background
			FillRGBA((x - iStep + i) - 1, (y - iStep) - 1, 3, 3, 0, 0, 0, 255);
			FillRGBA((x - iStep) - 1, (y - iStep + i) - 1, 3, 3, 0, 0, 0, 255);

			FillRGBA((x + iStep - i) - 1, (y - iStep) - 1, 3, 3, 0, 0, 0, 255);
			FillRGBA((x + iStep) - 1, (y - iStep + i) - 1, 3, 3, 0, 0, 0, 255);

			FillRGBA((x - iStep + i) - 1, (y + iStep) - 1, 3, 3, 0, 0, 0, 255);
			FillRGBA((x - iStep) - 1, (y + iStep - i) - 1, 3, 3, 0, 0, 0, 255);

			FillRGBA((x + iStep - i) - 1, (y + iStep) - 1, 3, 3, 0, 0, 0, 255);
			FillRGBA((x + iStep) - 1, (y + iStep - i) - 1, 3, 3, 0, 0, 0, 255);

		}

		for (int i = 0; i < radius; i++)
		{
			// Simple
			FillRGBA(x - iStep + i, y - iStep, 1, 1, R, G, B, 255);
			FillRGBA(x - iStep, y - iStep + i, 1, 1, R, G, B, 255);

			FillRGBA(x + iStep - i, y - iStep, 1, 1, R, G, B, 255);
			FillRGBA(x + iStep, y - iStep + i, 1, 1, R, G, B, 255);

			FillRGBA(x - iStep + i, y + iStep, 1, 1, R, G, B, 255);
			FillRGBA(x - iStep, y + iStep - i, 1, 1, R, G, B, 255);

			FillRGBA(x + iStep - i, y + iStep, 1, 1, R, G, B, 255);
			FillRGBA(x + iStep, y + iStep - i, 1, 1, R, G, B, 255);

		}
	}

	void drawBox(int x, int y, int w, int h, int lw, int r, int g, int b, int a)
	{
		FillRGBA(x, y, w, lw, r, g, b, a); // top
		FillRGBA(x, y + lw, lw, h - lw, r, g, b, a); // left
		FillRGBA(x + w - lw, y + lw, lw, h - lw, r, g, b, a); // right
		FillRGBA(x + lw, y + h - lw, w - lw * 2, lw, r, g, b, a); // bottom
	}

	void drawOutlinedRect(int x0, int y0, int x1, int y1, int R, int G, int B, int A)
	{
		int BoxWidth = x1 - x0;
		int BoxHeight = y1 - y0;

		if (BoxWidth < 10) BoxWidth = 10;
		if (BoxHeight < 15) BoxHeight = 15;


		DrawSetColor(R, G, B, A);

		DrawFilledRect(x0, y0, x0 + (BoxWidth / 5), y0 + 1); //left top
		DrawFilledRect(x0, y0, x0 + 1, y0 + (BoxHeight / 6)); //left top

		DrawFilledRect(x1 - (BoxWidth / 5) + 1, y0, x1, y0 + 1); //right top
		DrawFilledRect(x1, y0, x1 + 1, y0 + (BoxHeight / 6)); //right top

		DrawFilledRect(x0, y1, x0 + (BoxWidth / 5), y1 + 1); //left bottom
		DrawFilledRect(x0, y1 - (BoxHeight / 6) + 1, x0 + 1, y1 + 1); //left bottom

		DrawFilledRect(x1 - (BoxWidth / 5) + 1, y1, x1, y1 + 1); //right bottom
		DrawFilledRect(x1, y1 - (BoxHeight / 6) + 1, x1 + 1, y1 + 1); //right bottom

		// int HealthWidth = ((x1 - x0) - 2);
		// int HealthHeight = (y1 - y0) - 2;
		// DrawGUIBoxHorizontal( x0, y0 - (HealthHeight + 5), HealthWidth, HealthHeight, R, G, B, Health );
	}
};
enum EFontFlags
{
	FONTFLAG_NONE,
	FONTFLAG_ITALIC = 0x001,
	FONTFLAG_UNDERLINE = 0x002,
	FONTFLAG_STRIKEOUT = 0x004,
	FONTFLAG_SYMBOL = 0x008,
	FONTFLAG_ANTIALIAS = 0x010,
	FONTFLAG_GAUSSIANBLUR = 0x020,
	FONTFLAG_ROTARY = 0x040,
	FONTFLAG_DROPSHADOW = 0x080,
	FONTFLAG_ADDITIVE = 0x100,
	FONTFLAG_OUTLINE = 0x200,
	FONTFLAG_CUSTOM = 0x400,
	FONTFLAG_BITMAP = 0x800,
};