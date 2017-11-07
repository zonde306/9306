#include "./surface.h"
#include "../structs/surface.h"
#include <string>

namespace sqSurface
{
	HSQOBJECT classObject;

	SQInteger constructor(HSQUIRRELVM v);

	SQInteger setColor(HSQUIRRELVM v);
	SQInteger drawRect(HSQUIRRELVM v);
	SQInteger drawFiledRect(HSQUIRRELVM v);
	SQInteger drawLine(HSQUIRRELVM v);

	SQInteger createFont(HSQUIRRELVM v);
	SQInteger setFontGlyphSet(HSQUIRRELVM v);
	SQInteger addFontFile(HSQUIRRELVM v);
	SQInteger setTextColor(HSQUIRRELVM v);
	SQInteger setTextFont(HSQUIRRELVM v);
	SQInteger setTextPosition(HSQUIRRELVM v);
	SQInteger drawText(HSQUIRRELVM v);
	SQInteger getDrawTextSize(HSQUIRRELVM v);

	std::wstring c2w(const std::string& s)
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
};

void sqSurface::Initialization(HSQUIRRELVM v)
{
	// 栈顶
	SQInteger top = sq_gettop(v);

	// 类作用域(一般是一个表)
	sq_pushroottable(v);

	// 设置类名字
	sq_pushstring(v, _SC("Surface"), -1);

	// 使用刚才提供的名字声明一个类
	sq_newclass(v, SQFalse);

	// 获取类本身(不是实例)
	sq_getstackobj(v, -1, &classObject);
	sq_addref(v, &classObject);

	// 将类添加到作用域
	sq_newslot(v, -3, SQFalse);
	sq_pushobject(v, classObject);

	// 定义类成员
	sq_pushstring(v, _SC("pointer"), -1);
	sq_pushuserpointer(v, nullptr);
	sq_newslot(v, -3, SQFalse);

	// 构造函数
	sq_pushstring(v, _SC("constructor"), -1);
	sq_newclosure(v, sqSurface::constructor, 0);
	sq_newslot(v, -3, SQFalse);

	// 类方法
	sq_pushstring(v, _SC("setColor"), -1);
	sq_newclosure(v, sqSurface::setColor, 0);
	sq_setparamscheck(v, SQ_MATCHTYPEMASKSTRING, _SC(".iiii"));
	sq_newslot(v, -3, SQFalse);

	sq_pushstring(v, _SC("drawRect"), -1);
	sq_newclosure(v, sqSurface::drawRect, 0);
	sq_setparamscheck(v, SQ_MATCHTYPEMASKSTRING, _SC(".iiii"));
	sq_newslot(v, -3, SQFalse);

	sq_pushstring(v, _SC("drawFiledRect"), -1);
	sq_newclosure(v, sqSurface::drawFiledRect, 0);
	sq_setparamscheck(v, SQ_MATCHTYPEMASKSTRING, _SC(".iiii"));
	sq_newslot(v, -3, SQFalse);

	sq_pushstring(v, _SC("drawLine"), -1);
	sq_newclosure(v, sqSurface::drawLine, 0);
	sq_setparamscheck(v, SQ_MATCHTYPEMASKSTRING, _SC(".iiii"));
	sq_newslot(v, -3, SQFalse);

	sq_pushstring(v, _SC("createFont"), -1);
	sq_newclosure(v, sqSurface::createFont, 0);
	sq_setparamscheck(v, SQ_MATCHTYPEMASKSTRING, _SC("."));
	sq_newslot(v, -3, SQFalse);

	sq_pushstring(v, _SC("setFontGlyphSet"), -1);
	sq_newclosure(v, sqSurface::setFontGlyphSet, 0);
	sq_setparamscheck(v, SQ_MATCHTYPEMASKSTRING, _SC(".isiiiii"));
	sq_newslot(v, -3, SQFalse);

	sq_pushstring(v, _SC("addFontFile"), -1);
	sq_newclosure(v, sqSurface::addFontFile, 0);
	sq_setparamscheck(v, SQ_MATCHTYPEMASKSTRING, _SC(".s"));
	sq_newslot(v, -3, SQFalse);

	sq_pushstring(v, _SC("setTextFont"), -1);
	sq_newclosure(v, sqSurface::setTextFont, 0);
	sq_setparamscheck(v, SQ_MATCHTYPEMASKSTRING, _SC(".i"));
	sq_newslot(v, -3, SQFalse);

	sq_pushstring(v, _SC("setTextPosition"), -1);
	sq_newclosure(v, sqSurface::setTextPosition, 0);
	sq_setparamscheck(v, SQ_MATCHTYPEMASKSTRING, _SC(".ii"));
	sq_newslot(v, -3, SQFalse);

	sq_pushstring(v, _SC("drawText"), -1);
	sq_newclosure(v, sqSurface::drawText, 0);
	sq_setparamscheck(v, SQ_MATCHTYPEMASKSTRING, _SC(".s"));
	sq_newslot(v, -3, SQFalse);

	sq_pushstring(v, _SC("getDrawTextSize"), -1);
	sq_newclosure(v, sqSurface::getDrawTextSize, 0);
	sq_setparamscheck(v, SQ_MATCHTYPEMASKSTRING, _SC(".is"));
	sq_newslot(v, -3, SQFalse);

	sq_pushstring(v, _SC("setTextColor"), -1);
	sq_newclosure(v, sqSurface::setTextColor, 0);
	sq_setparamscheck(v, SQ_MATCHTYPEMASKSTRING, _SC(".iiii"));
	sq_newslot(v, -3, SQFalse);

	sq_settop(v, top);
}

SQInteger sqSurface::constructor(HSQUIRRELVM v)
{
	if (sq_gettop(v) < 2)
		return 0;

	SQUserPointer pointer = nullptr;
	sq_getuserpointer(v, -1, &pointer);

	sq_pushstring(v, _SC("pointer"), -1);
	sq_pushuserpointer(v, pointer);
	sq_set(v, 1);

	return 0;
}

SQInteger sqSurface::setColor(HSQUIRRELVM v)
{
	sq_pushstring(v, _SC("pointer"), -1);
	if (SQ_FAILED(sq_get(v, 1)))
	{
		sq_throwerror(v, _SC("指针不存在"));
		return 0;
	}

	CSurface* pointer = nullptr;
	sq_getuserpointer(v, -1, (SQUserPointer*)&pointer);
	if (pointer == nullptr)
	{
		sq_throwerror(v, _SC("这是一个无效的对象！"));
		return 0;
	}

	SQInteger r, g, b, a;
	sq_getinteger(v, 2, &r);
	sq_getinteger(v, 3, &g);
	sq_getinteger(v, 4, &b);
	sq_getinteger(v, 5, &a);

	pointer->DrawSetColor(r, g, b, a);
	return 0;
}

SQInteger sqSurface::drawLine(HSQUIRRELVM v)
{
	sq_pushstring(v, _SC("pointer"), -1);
	if (SQ_FAILED(sq_get(v, 1)))
	{
		sq_throwerror(v, _SC("指针不存在"));
		return 0;
	}

	CSurface* pointer = nullptr;
	sq_getuserpointer(v, -1, (SQUserPointer*)&pointer);
	if (pointer == nullptr)
	{
		sq_throwerror(v, _SC("这是一个无效的对象！"));
		return 0;
	}

	SQInteger x1, x2, y1, y2;
	sq_getinteger(v, 2, &x1);
	sq_getinteger(v, 3, &y1);
	sq_getinteger(v, 4, &x2);
	sq_getinteger(v, 5, &y2);

	pointer->DrawLine(x1, y1, x2, y2);
	return 0;
}

SQInteger sqSurface::drawRect(HSQUIRRELVM v)
{
	sq_pushstring(v, _SC("pointer"), -1);
	if (SQ_FAILED(sq_get(v, 1)))
	{
		sq_throwerror(v, _SC("指针不存在"));
		return 0;
	}

	CSurface* pointer = nullptr;
	sq_getuserpointer(v, -1, (SQUserPointer*)&pointer);
	if (pointer == nullptr)
	{
		sq_throwerror(v, _SC("这是一个无效的对象！"));
		return 0;
	}

	SQInteger x1, x2, y1, y2;
	sq_getinteger(v, 2, &x1);
	sq_getinteger(v, 3, &y1);
	sq_getinteger(v, 4, &x2);
	sq_getinteger(v, 5, &y2);

	pointer->DrawOutlinedRect(x1, y1, x2, y2);
	return 0;
}

SQInteger sqSurface::drawFiledRect(HSQUIRRELVM v)
{
	sq_pushstring(v, _SC("pointer"), -1);
	if (SQ_FAILED(sq_get(v, 1)))
	{
		sq_throwerror(v, _SC("指针不存在"));
		return 0;
	}

	CSurface* pointer = nullptr;
	sq_getuserpointer(v, -1, (SQUserPointer*)&pointer);
	if (pointer == nullptr)
	{
		sq_throwerror(v, _SC("这是一个无效的对象！"));
		return 0;
	}

	SQInteger x1, x2, y1, y2;
	sq_getinteger(v, 2, &x1);
	sq_getinteger(v, 3, &y1);
	sq_getinteger(v, 4, &x2);
	sq_getinteger(v, 5, &y2);

	pointer->DrawFilledRect(x1, y1, x2, y2);
	return 0;
}

SQInteger sqSurface::createFont(HSQUIRRELVM v)
{
	sq_pushstring(v, _SC("pointer"), -1);
	if (SQ_FAILED(sq_get(v, 1)))
	{
		sq_throwerror(v, _SC("指针不存在"));
		return 0;
	}

	CSurface* pointer = nullptr;
	sq_getuserpointer(v, -1, (SQUserPointer*)&pointer);
	if (pointer == nullptr)
	{
		sq_throwerror(v, _SC("这是一个无效的对象！"));
		return 0;
	}

	sq_pushinteger(v, (SQInteger)pointer->SCreateFont());
	return 0;
}

SQInteger sqSurface::setFontGlyphSet(HSQUIRRELVM v)
{
	sq_pushstring(v, _SC("pointer"), -1);
	if (SQ_FAILED(sq_get(v, 1)))
	{
		sq_throwerror(v, _SC("指针不存在"));
		return 0;
	}

	CSurface* pointer = nullptr;
	sq_getuserpointer(v, -1, (SQUserPointer*)&pointer);
	if (pointer == nullptr)
	{
		sq_throwerror(v, _SC("这是一个无效的对象！"));
		return 0;
	}

	const SQChar* file;
	SQInteger font, tall, weight, blur, scanlines, flags;
	sq_getinteger(v, 2, &font);
	sq_getstring(v, 3, &file);
	sq_getinteger(v, 4, &tall);
	sq_getinteger(v, 5, &weight);
	sq_getinteger(v, 6, &blur);
	sq_getinteger(v, 7, &scanlines);
	sq_getinteger(v, 8, &flags);

	if (pointer->SetFontGlyphSet((unsigned int)font, file, tall, weight, blur, scanlines, flags))
		sq_pushbool(v, SQTrue);
	else
		sq_pushbool(v, SQFalse);

	return 1;
}

SQInteger sqSurface::addFontFile(HSQUIRRELVM v)
{
	sq_pushstring(v, _SC("pointer"), -1);
	if (SQ_FAILED(sq_get(v, 1)))
	{
		sq_throwerror(v, _SC("指针不存在"));
		return 0;
	}

	CSurface* pointer = nullptr;
	sq_getuserpointer(v, -1, (SQUserPointer*)&pointer);
	if (pointer == nullptr)
	{
		sq_throwerror(v, _SC("这是一个无效的对象！"));
		return 0;
	}

	const SQChar* file;
	sq_getstring(v, 2, &file);

	if (pointer->AddCustomFontFile(file))
		sq_pushbool(v, SQTrue);
	else
		sq_pushbool(v, SQFalse);

	return 1;
}

SQInteger sqSurface::setTextColor(HSQUIRRELVM v)
{
	sq_pushstring(v, _SC("pointer"), -1);
	if (SQ_FAILED(sq_get(v, 1)))
	{
		sq_throwerror(v, _SC("指针不存在"));
		return 0;
	}

	CSurface* pointer = nullptr;
	sq_getuserpointer(v, -1, (SQUserPointer*)&pointer);
	if (pointer == nullptr)
	{
		sq_throwerror(v, _SC("这是一个无效的对象！"));
		return 0;
	}

	SQInteger r, g, b, a;
	sq_getinteger(v, 2, &r);
	sq_getinteger(v, 3, &g);
	sq_getinteger(v, 4, &b);
	sq_getinteger(v, 5, &a);

	pointer->DrawSetTextColor(r, g, b, a);
	return 0;
}

SQInteger sqSurface::setTextFont(HSQUIRRELVM v)
{
	sq_pushstring(v, _SC("pointer"), -1);
	if (SQ_FAILED(sq_get(v, 1)))
	{
		sq_throwerror(v, _SC("指针不存在"));
		return 0;
	}

	CSurface* pointer = nullptr;
	sq_getuserpointer(v, -1, (SQUserPointer*)&pointer);
	if (pointer == nullptr)
	{
		sq_throwerror(v, _SC("这是一个无效的对象！"));
		return 0;
	}

	SQInteger font;
	sq_getinteger(v, 2, &font);

	pointer->DrawSetTextFont((unsigned int)font);
	return 0;
}

SQInteger sqSurface::setTextPosition(HSQUIRRELVM v)
{
	sq_pushstring(v, _SC("pointer"), -1);
	if (SQ_FAILED(sq_get(v, 1)))
	{
		sq_throwerror(v, _SC("指针不存在"));
		return 0;
	}

	CSurface* pointer = nullptr;
	sq_getuserpointer(v, -1, (SQUserPointer*)&pointer);
	if (pointer == nullptr)
	{
		sq_throwerror(v, _SC("这是一个无效的对象！"));
		return 0;
	}

	SQInteger x, y;
	sq_getinteger(v, 2, &x);
	sq_getinteger(v, 3, &y);

	pointer->DrawSetTextPos(x, y);
	return 0;
}

SQInteger sqSurface::drawText(HSQUIRRELVM v)
{
	sq_pushstring(v, _SC("pointer"), -1);
	if (SQ_FAILED(sq_get(v, 1)))
	{
		sq_throwerror(v, _SC("指针不存在"));
		return 0;
	}

	CSurface* pointer = nullptr;
	sq_getuserpointer(v, -1, (SQUserPointer*)&pointer);
	if (pointer == nullptr)
	{
		sq_throwerror(v, _SC("这是一个无效的对象！"));
		return 0;
	}

	const SQChar* text;
	sq_getstring(v, 2, &text);

	std::wstring print;
#ifndef SQUNICODE
	print = c2w(text);
#else
	print = text;
#endif

	pointer->DrawPrintText(print.c_str(), print.length());
	return 0;
}

SQInteger sqSurface::getDrawTextSize(HSQUIRRELVM v)
{
	sq_pushstring(v, _SC("pointer"), -1);
	if (SQ_FAILED(sq_get(v, 1)))
	{
		sq_throwerror(v, _SC("指针不存在"));
		return 0;
	}

	CSurface* pointer = nullptr;
	sq_getuserpointer(v, -1, (SQUserPointer*)&pointer);
	if (pointer == nullptr)
	{
		sq_throwerror(v, _SC("这是一个无效的对象！"));
		return 0;
	}

	SQInteger font;
	const SQChar* text;
	sq_getinteger(v, 2, &font);
	sq_getstring(v, 3, &text);

	std::wstring print;
#ifndef SQUNICODE
	print = c2w(text);
#else
	print = text;
#endif

	SQInteger wide, tall;
	pointer->GetTextSize((unsigned int)font, print.c_str(), wide, tall);

	sq_newtable(v);

	sq_pushstring(v, _SC("wide"), -1);
	sq_pushinteger(v, wide);
	sq_newslot(v, -3, SQFalse);

	sq_pushstring(v, _SC("tall"), -1);
	sq_pushinteger(v, tall);
	sq_newslot(v, -3, SQFalse);

	return 1;
}
