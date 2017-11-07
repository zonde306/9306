#include "draw.h"
#include "../drawmanager.h"

namespace sqDraw
{
	HSQOBJECT classObject;

	SQInteger constructor(HSQUIRRELVM v);

	SQInteger drawLine(HSQUIRRELVM v);
	SQInteger drawRect(HSQUIRRELVM v);
	SQInteger drawCircle(HSQUIRRELVM v);
	SQInteger drawString(HSQUIRRELVM v);
	SQInteger drawCorner(HSQUIRRELVM v);
	SQInteger fillRect(HSQUIRRELVM v);
	SQInteger fillCircle(HSQUIRRELVM v);
};

void sqDraw::Initialization(HSQUIRRELVM v)
{
	// 栈顶
	SQInteger top = sq_gettop(v);

	// 类作用域(一般是一个表)
	sq_pushroottable(v);

	// 设置类名字
	sq_pushstring(v, _SC("DrawManager"), -1);

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
	sq_newclosure(v, sqDraw::constructor, 0);
	sq_newslot(v, -3, SQFalse);

	// 类方法
	sq_pushstring(v, _SC("drawLine"), -1);
	sq_newclosure(v, sqDraw::drawLine, 0);
	sq_setparamscheck(v, SQ_MATCHTYPEMASKSTRING, _SC(".iiiii"));
	sq_newslot(v, -3, SQFalse);

	sq_pushstring(v, _SC("drawRect"), -1);
	sq_newclosure(v, sqDraw::drawRect, 0);
	sq_setparamscheck(v, SQ_MATCHTYPEMASKSTRING, _SC(".iiiii"));
	sq_newslot(v, -3, SQFalse);

	sq_pushstring(v, _SC("drawCircle"), -1);
	sq_newclosure(v, sqDraw::drawCircle, 0);
	sq_setparamscheck(v, SQ_MATCHTYPEMASKSTRING, _SC(".iiiii"));
	sq_newslot(v, -3, SQFalse);

	sq_pushstring(v, _SC("drawString"), -1);
	sq_newclosure(v, sqDraw::drawString, 0);
	sq_setparamscheck(v, SQ_MATCHTYPEMASKSTRING, _SC(".iisib"));
	sq_newslot(v, -3, SQFalse);

	sq_pushstring(v, _SC("drawCorner"), -1);
	sq_newclosure(v, sqDraw::drawCorner, 0);
	sq_setparamscheck(v, SQ_MATCHTYPEMASKSTRING, _SC(".iiiiii"));
	sq_newslot(v, -3, SQFalse);

	sq_pushstring(v, _SC("drawFillCircle"), -1);
	sq_newclosure(v, sqDraw::fillCircle, 0);
	sq_setparamscheck(v, SQ_MATCHTYPEMASKSTRING, _SC(".iiiii"));
	sq_newslot(v, -3, SQFalse);

	sq_pushstring(v, _SC("drawFillRect"), -1);
	sq_newclosure(v, sqDraw::fillRect, 0);
	sq_setparamscheck(v, SQ_MATCHTYPEMASKSTRING, _SC(".iiiii"));
	sq_newslot(v, -3, SQFalse);

	sq_settop(v, top);
}

SQInteger sqDraw::constructor(HSQUIRRELVM v)
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

SQInteger sqDraw::drawLine(HSQUIRRELVM v)
{
	sq_pushstring(v, _SC("pointer"), -1);
	if (SQ_FAILED(sq_get(v, 1)))
	{
		sq_throwerror(v, _SC("指针不存在"));
		return 0;
	}

	DrawManager* pointer = nullptr;
	sq_getuserpointer(v, -1, (SQUserPointer*)&pointer);
	if (pointer == nullptr)
	{
		sq_throwerror(v, _SC("这是一个无效的对象！"));
		return 0;
	}

	SQInteger x1, y1, x2, y2, color;
	sq_getinteger(v, 2, &x1);
	sq_getinteger(v, 3, &y1);
	sq_getinteger(v, 4, &x2);
	sq_getinteger(v, 5, &y2);
	sq_getinteger(v, 6, &color);

	pointer->AddLine(color, x1, y1, x2, y2);
	return 0;
}

SQInteger sqDraw::drawRect(HSQUIRRELVM v)
{
	sq_pushstring(v, _SC("pointer"), -1);
	if (SQ_FAILED(sq_get(v, 1)))
	{
		sq_throwerror(v, _SC("指针不存在"));
		return 0;
	}

	DrawManager* pointer = nullptr;
	sq_getuserpointer(v, -1, (SQUserPointer*)&pointer);
	if (pointer == nullptr)
	{
		sq_throwerror(v, _SC("这是一个无效的对象！"));
		return 0;
	}

	SQInteger x, y, w, h, color;
	sq_getinteger(v, 2, &x);
	sq_getinteger(v, 3, &y);
	sq_getinteger(v, 4, &w);
	sq_getinteger(v, 5, &h);
	sq_getinteger(v, 6, &color);

	pointer->AddRect(color, x, y, w, h);
	return 0;
}

SQInteger sqDraw::drawCircle(HSQUIRRELVM v)
{
	sq_pushstring(v, _SC("pointer"), -1);
	if (SQ_FAILED(sq_get(v, 1)))
	{
		sq_throwerror(v, _SC("指针不存在"));
		return 0;
	}

	DrawManager* pointer = nullptr;
	sq_getuserpointer(v, -1, (SQUserPointer*)&pointer);
	if (pointer == nullptr)
	{
		sq_throwerror(v, _SC("这是一个无效的对象！"));
		return 0;
	}

	SQInteger x, y, r, resolution, color;
	sq_getinteger(v, 2, &x);
	sq_getinteger(v, 3, &y);
	sq_getinteger(v, 4, &r);
	sq_getinteger(v, 5, &color);
	sq_getinteger(v, 6, &resolution);

	if (resolution > 2)
		pointer->AddCircle(color, x, y, r, (size_t)resolution);

	return 0;
}

SQInteger sqDraw::drawString(HSQUIRRELVM v)
{
	sq_pushstring(v, _SC("pointer"), -1);
	if (SQ_FAILED(sq_get(v, 1)))
	{
		sq_throwerror(v, _SC("指针不存在"));
		return 0;
	}

	DrawManager* pointer = nullptr;
	sq_getuserpointer(v, -1, (SQUserPointer*)&pointer);
	if (pointer == nullptr)
	{
		sq_throwerror(v, _SC("这是一个无效的对象！"));
		return 0;
	}

	SQBool center;
	const SQChar* text;
	SQInteger x, y, color;
	sq_getinteger(v, 2, &x);
	sq_getinteger(v, 3, &y);
	sq_getstring(v, 4, &text);
	sq_getinteger(v, 5, &color);
	sq_getbool(v, 6, &center);

	if (text[0] != '\0')
		pointer->AddString(color, x, y, center == SQTrue, text);

	return 0;
}

SQInteger sqDraw::drawCorner(HSQUIRRELVM v)
{
	sq_pushstring(v, _SC("pointer"), -1);
	if (SQ_FAILED(sq_get(v, 1)))
	{
		sq_throwerror(v, _SC("指针不存在"));
		return 0;
	}

	DrawManager* pointer = nullptr;
	sq_getuserpointer(v, -1, (SQUserPointer*)&pointer);
	if (pointer == nullptr)
	{
		sq_throwerror(v, _SC("这是一个无效的对象！"));
		return 0;
	}
	
	SQInteger x, y, w, h, l, color;
	sq_getinteger(v, 2, &x);
	sq_getinteger(v, 3, &y);
	sq_getinteger(v, 4, &w);
	sq_getinteger(v, 5, &h);
	sq_getinteger(v, 6, &color);
	sq_getinteger(v, 7, &l);

	pointer->AddCorner(color, x, y, w, h, l);
	return 0;
}

SQInteger sqDraw::fillRect(HSQUIRRELVM v)
{
	sq_pushstring(v, _SC("pointer"), -1);
	if (SQ_FAILED(sq_get(v, 1)))
	{
		sq_throwerror(v, _SC("指针不存在"));
		return 0;
	}

	DrawManager* pointer = nullptr;
	sq_getuserpointer(v, -1, (SQUserPointer*)&pointer);
	if (pointer == nullptr)
	{
		sq_throwerror(v, _SC("这是一个无效的对象！"));
		return 0;
	}

	SQInteger x, y, w, h, color;
	sq_getinteger(v, 2, &x);
	sq_getinteger(v, 3, &y);
	sq_getinteger(v, 4, &w);
	sq_getinteger(v, 5, &h);
	sq_getinteger(v, 6, &color);

	pointer->AddFillRect(color, x, y, w, h);
	return 0;
}

SQInteger sqDraw::fillCircle(HSQUIRRELVM v)
{
	sq_pushstring(v, _SC("pointer"), -1);
	if (SQ_FAILED(sq_get(v, 1)))
	{
		sq_throwerror(v, _SC("指针不存在"));
		return 0;
	}

	DrawManager* pointer = nullptr;
	sq_getuserpointer(v, -1, (SQUserPointer*)&pointer);
	if (pointer == nullptr)
	{
		sq_throwerror(v, _SC("这是一个无效的对象！"));
		return 0;
	}

	SQInteger x, y, r, resolution, color;
	sq_getinteger(v, 2, &x);
	sq_getinteger(v, 3, &y);
	sq_getinteger(v, 4, &r);
	sq_getinteger(v, 5, &color);
	sq_getinteger(v, 6, &resolution);

	if (resolution > 2)
		pointer->AddFillCircle(color, x, y, r, (size_t)resolution);

	return 0;
}
