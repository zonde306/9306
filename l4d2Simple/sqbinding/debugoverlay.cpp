#include "debugoverlay.h"
#include "../structs/debugoverlay.h"
#include "vector.h"

namespace sqDebugOverlay
{
	HSQOBJECT classObject;

#define COLOR_TO_RGBA(_clr,_r,_g,_b,_a)	{_a=(_clr>>24);_r=((_clr>>16)&0xFF);_g=((_clr>>8)&0xFF);_b=(_clr&0xFF);}

#define GET_VECTOR2(v,vec,_idx)		{\
		sq_pushstring(v, "x", -1);\
		if(SQ_SUCCEEDED(sq_get(v, _idx))) {\
			sq_getfloat(v, -1, &(vec.x));\
			sq_poptop(v);\
		}\
		sq_pushstring(v, "y", -1);\
		if(SQ_SUCCEEDED(sq_get(v, _idx))) {\
			sq_getfloat(v, -1, &(vec.y));\
			sq_poptop(v);\
		}\
		sq_pushstring(v, "z", -1);\
		if(SQ_SUCCEEDED(sq_get(v, _idx))) {\
			sq_getfloat(v, -1, &(vec.z));\
			sq_poptop(v);\
		}\
	}

#define FILLED_VECTOR2(v,vec)	{\
		sq_pushstring(v, _SC("x"), -1);\
		sq_pushfloat(v, vec.x);\
		sq_set(v, -3);\
		sq_pushstring(v, _SC("y"), -1);\
		sq_pushfloat(v, vec.y);\
		sq_set(v, -3);\
		sq_pushstring(v, _SC("z"), -1);\
		sq_pushfloat(v, vec.z);\
		sq_set(v, -3);\
	}

	SQInteger constructor(HSQUIRRELVM v);
	SQInteger addEntityText(HSQUIRRELVM v);
	SQInteger addBox(HSQUIRRELVM v);
	SQInteger addTriangle(HSQUIRRELVM v);
	SQInteger addLine(HSQUIRRELVM v);
	SQInteger addText(HSQUIRRELVM v);
	SQInteger addScreenText(HSQUIRRELVM v);
	SQInteger addSweptBox(HSQUIRRELVM v);
	SQInteger addGrid(HSQUIRRELVM v);
	SQInteger getScreenPostion(HSQUIRRELVM v);
	SQInteger addColordText(HSQUIRRELVM v);
	SQInteger addBox2(HSQUIRRELVM v);
};

void sqDebugOverlay::Initialization(HSQUIRRELVM v)
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
	sq_newclosure(v, sqDebugOverlay::constructor, 0);
	sq_newslot(v, -3, SQFalse);

	// 类方法
	sq_pushstring(v, _SC("addEntityText"), -1);
	sq_newclosure(v, sqDebugOverlay::addEntityText, 0);
	sq_setparamscheck(v, SQ_MATCHTYPEMASKSTRING, _SC(".isifi"));
	sq_newslot(v, -3, SQFalse);

	sq_pushstring(v, _SC("addBox"), -1);
	sq_newclosure(v, sqDebugOverlay::addBox, 0);
	sq_setparamscheck(v, SQ_MATCHTYPEMASKSTRING, _SC(".xxxxif"));
	sq_newslot(v, -3, SQFalse);

	sq_pushstring(v, _SC("addTriangle"), -1);
	sq_newclosure(v, sqDebugOverlay::addTriangle, 0);
	sq_setparamscheck(v, SQ_MATCHTYPEMASKSTRING, _SC(".xxxifb"));
	sq_newslot(v, -3, SQFalse);

	sq_pushstring(v, _SC("addLine"), -1);
	sq_newclosure(v, sqDebugOverlay::addLine, 0);
	sq_setparamscheck(v, SQ_MATCHTYPEMASKSTRING, _SC(".xxifb"));
	sq_newslot(v, -3, SQFalse);

	sq_pushstring(v, _SC("addText"), -1);
	sq_newclosure(v, sqDebugOverlay::addText, 0);
	sq_setparamscheck(v, SQ_MATCHTYPEMASKSTRING, _SC(".xsfi"));
	sq_newslot(v, -3, SQFalse);

	sq_pushstring(v, _SC("addScreenText"), -1);
	sq_newclosure(v, sqDebugOverlay::addScreenText, 0);
	sq_setparamscheck(v, SQ_MATCHTYPEMASKSTRING, _SC(".ffsif"));
	sq_newslot(v, -3, SQFalse);

	sq_pushstring(v, _SC("addSweptBox"), -1);
	sq_newclosure(v, sqDebugOverlay::addSweptBox, 0);
	sq_setparamscheck(v, SQ_MATCHTYPEMASKSTRING, _SC(".xxxxif"));
	sq_newslot(v, -3, SQFalse);

	sq_pushstring(v, _SC("addGrid"), -1);
	sq_newclosure(v, sqDebugOverlay::addGrid, 0);
	sq_setparamscheck(v, SQ_MATCHTYPEMASKSTRING, _SC("."));
	sq_newslot(v, -3, SQFalse);

	sq_pushstring(v, _SC("getScreenPostion"), -1);
	sq_newclosure(v, sqDebugOverlay::getScreenPostion, 0);
	sq_setparamscheck(v, SQ_MATCHTYPEMASKSTRING, _SC(".x"));
	sq_newslot(v, -3, SQFalse);

	sq_pushstring(v, _SC("addColordText"), -1);
	sq_newclosure(v, sqDebugOverlay::addColordText, 0);
	sq_setparamscheck(v, SQ_MATCHTYPEMASKSTRING, _SC(".xsfii"));
	sq_newslot(v, -3, SQFalse);

	sq_pushstring(v, _SC("addBox2"), -1);
	sq_newclosure(v, sqDebugOverlay::addBox2, 0);
	sq_setparamscheck(v, SQ_MATCHTYPEMASKSTRING, _SC(".xxxxiif"));
	sq_newslot(v, -3, SQFalse);

	sq_settop(v, top);
}

SQInteger sqDebugOverlay::constructor(HSQUIRRELVM v)
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

SQInteger sqDebugOverlay::addEntityText(HSQUIRRELVM v)
{
	sq_pushstring(v, _SC("pointer"), -1);
	if (SQ_FAILED(sq_get(v, 1)))
	{
		sq_throwerror(v, _SC("指针不存在"));
		return 0;
	}

	CDebugOverlay* pointer = nullptr;
	sq_getuserpointer(v, -1, (SQUserPointer*)&pointer);
	if (pointer == nullptr)
	{
		sq_throwerror(v, _SC("这是一个无效的对象！"));
		return 0;
	}

	SQFloat duration;
	const SQChar* text;
	SQInteger entity, lineOffset, color;
	sq_getinteger(v, 2, &entity);
	sq_getstring(v, 3, &text);
	sq_getinteger(v, 4, &color);
	sq_getfloat(v, 5, &duration);
	sq_getinteger(v, 6, &lineOffset);

	SQInteger r, g, b, a;
	COLOR_TO_RGBA(color, r, g, b, a);

	pointer->AddEntityTextOverlay(entity, lineOffset, duration, r, g, b, a, text);
	return 0;
}

SQInteger sqDebugOverlay::addBox(HSQUIRRELVM v)
{
	sq_pushstring(v, _SC("pointer"), -1);
	if (SQ_FAILED(sq_get(v, 1)))
	{
		sq_throwerror(v, _SC("指针不存在"));
		return 0;
	}

	CDebugOverlay* pointer = nullptr;
	sq_getuserpointer(v, -1, (SQUserPointer*)&pointer);
	if (pointer == nullptr)
	{
		sq_throwerror(v, _SC("这是一个无效的对象！"));
		return 0;
	}

	SQInteger color;
	SQFloat duration;
	Vector vPos, vAng, vMin, vMax;
	GET_VECTOR2(v, vPos, 2);
	GET_VECTOR2(v, vMin, 3);
	GET_VECTOR2(v, vMax, 4);
	GET_VECTOR2(v, vAng, 5);
	sq_getinteger(v, 6, &color);
	sq_getfloat(v, 7, &duration);

	SQInteger r, g, b, a;
	COLOR_TO_RGBA(color, r, g, b, a);

	pointer->AddBoxOverlay(vPos, vMin, vMax, vAng, r, g, b, a, duration);
	return 0;
}

SQInteger sqDebugOverlay::addTriangle(HSQUIRRELVM v)
{
	sq_pushstring(v, _SC("pointer"), -1);
	if (SQ_FAILED(sq_get(v, 1)))
	{
		sq_throwerror(v, _SC("指针不存在"));
		return 0;
	}

	CDebugOverlay* pointer = nullptr;
	sq_getuserpointer(v, -1, (SQUserPointer*)&pointer);
	if (pointer == nullptr)
	{
		sq_throwerror(v, _SC("这是一个无效的对象！"));
		return 0;
	}

	SQInteger color;
	SQFloat duration;
	Vector v1, v2, v3;
	SQBool noDepthTest;
	GET_VECTOR2(v, v1, 2);
	GET_VECTOR2(v, v2, 3);
	GET_VECTOR2(v, v3, 4);
	sq_getinteger(v, 5, &color);
	sq_getfloat(v, 6, &duration);
	sq_getbool(v, 7, &noDepthTest);

	SQInteger r, g, b, a;
	COLOR_TO_RGBA(color, r, g, b, a);

	pointer->AddTriangleOverlay(v1, v2, v3, r, g, b, a, (noDepthTest == SQTrue), duration);
	return 0;
}

SQInteger sqDebugOverlay::addLine(HSQUIRRELVM v)
{
	sq_pushstring(v, _SC("pointer"), -1);
	if (SQ_FAILED(sq_get(v, 1)))
	{
		sq_throwerror(v, _SC("指针不存在"));
		return 0;
	}

	CDebugOverlay* pointer = nullptr;
	sq_getuserpointer(v, -1, (SQUserPointer*)&pointer);
	if (pointer == nullptr)
	{
		sq_throwerror(v, _SC("这是一个无效的对象！"));
		return 0;
	}

	SQInteger color;
	Vector src, dst;
	SQFloat duration;
	SQBool noDepthTest;
	GET_VECTOR2(v, src, 2);
	GET_VECTOR2(v, dst, 3);
	sq_getinteger(v, 4, &color);
	sq_getfloat(v, 5, &duration);
	sq_getbool(v, 6, &noDepthTest);

	SQInteger r, g, b, a;
	COLOR_TO_RGBA(color, r, g, b, a);

	if(a < 255)
		pointer->AddLineOverlayAlpha(src, dst, r, g, b, a, (noDepthTest == SQTrue), duration);
	else
		pointer->AddLineOverlay(src, dst, r, g, b, (noDepthTest == SQTrue), duration);
	return 0;
}

SQInteger sqDebugOverlay::addText(HSQUIRRELVM v)
{
	sq_pushstring(v, _SC("pointer"), -1);
	if (SQ_FAILED(sq_get(v, 1)))
	{
		sq_throwerror(v, _SC("指针不存在"));
		return 0;
	}

	CDebugOverlay* pointer = nullptr;
	sq_getuserpointer(v, -1, (SQUserPointer*)&pointer);
	if (pointer == nullptr)
	{
		sq_throwerror(v, _SC("这是一个无效的对象！"));
		return 0;
	}

	Vector pos;
	SQFloat duration;
	const SQChar* text;
	SQInteger lineOffset;
	GET_VECTOR2(v, pos, 2);
	sq_getstring(v, 3, &text);
	sq_getfloat(v, 4, &duration);
	sq_getinteger(v, 5, &lineOffset);

	if (lineOffset > -1)
		pointer->AddTextOverlay(pos, lineOffset, duration, text);
	else
		pointer->AddTextOverlay(pos, duration, text);

	return 0;
}

SQInteger sqDebugOverlay::addScreenText(HSQUIRRELVM v)
{
	sq_pushstring(v, _SC("pointer"), -1);
	if (SQ_FAILED(sq_get(v, 1)))
	{
		sq_throwerror(v, _SC("指针不存在"));
		return 0;
	}

	CDebugOverlay* pointer = nullptr;
	sq_getuserpointer(v, -1, (SQUserPointer*)&pointer);
	if (pointer == nullptr)
	{
		sq_throwerror(v, _SC("这是一个无效的对象！"));
		return 0;
	}

	SQInteger color;
	const SQChar* text;
	SQFloat x, y, duration;
	sq_getfloat(v, 2, &x);
	sq_getfloat(v, 3, &y);
	sq_getstring(v, 4, &text);
	sq_getinteger(v, 5, &color);
	sq_getfloat(v, 6, &duration);

	SQInteger r, g, b, a;
	COLOR_TO_RGBA(color, r, g, b, a);

	pointer->AddScreenTextOverlay(x, y, duration, r, g, b, a, text);
	return 0;
}

SQInteger sqDebugOverlay::addSweptBox(HSQUIRRELVM v)
{
	sq_pushstring(v, _SC("pointer"), -1);
	if (SQ_FAILED(sq_get(v, 1)))
	{
		sq_throwerror(v, _SC("指针不存在"));
		return 0;
	}

	CDebugOverlay* pointer = nullptr;
	sq_getuserpointer(v, -1, (SQUserPointer*)&pointer);
	if (pointer == nullptr)
	{
		sq_throwerror(v, _SC("这是一个无效的对象！"));
		return 0;
	}

	SQInteger color;
	SQFloat duration;
	Vector start, end, min, max, ang;
	GET_VECTOR2(v, start, 2);
	GET_VECTOR2(v, end, 3);
	GET_VECTOR2(v, min, 4);
	GET_VECTOR2(v, max, 5);
	sq_getinteger(v, 6, &color);
	sq_getfloat(v, 7, &duration);

	SQInteger r, g, b, a;
	COLOR_TO_RGBA(color, r, g, b, a);

	pointer->AddSweptBoxOverlay(start, end, min, max, ang, r, g, b, a, duration);
	return 0;
}

SQInteger sqDebugOverlay::addGrid(HSQUIRRELVM v)
{
	sq_pushstring(v, _SC("pointer"), -1);
	if (SQ_FAILED(sq_get(v, 1)))
	{
		sq_throwerror(v, _SC("指针不存在"));
		return 0;
	}

	CDebugOverlay* pointer = nullptr;
	sq_getuserpointer(v, -1, (SQUserPointer*)&pointer);
	if (pointer == nullptr)
	{
		sq_throwerror(v, _SC("这是一个无效的对象！"));
		return 0;
	}

	Vector pos;
	GET_VECTOR2(v, pos, 2);

	pointer->AddGridOverlay(pos);
	return 0;
}

SQInteger sqDebugOverlay::getScreenPostion(HSQUIRRELVM v)
{
	sq_pushstring(v, _SC("pointer"), -1);
	if (SQ_FAILED(sq_get(v, 1)))
	{
		sq_throwerror(v, _SC("指针不存在"));
		return 0;
	}

	CDebugOverlay* pointer = nullptr;
	sq_getuserpointer(v, -1, (SQUserPointer*)&pointer);
	if (pointer == nullptr)
	{
		sq_throwerror(v, _SC("这是一个无效的对象！"));
		return 0;
	}

	Vector src, dst;
	GET_VECTOR2(v, src, 2);

	pointer->ScreenPosition(src, dst);

	sq_pushobject(v, sqVector::classObject);
	sq_clone(v, -1);
	FILLED_VECTOR2(v, dst);

	return 1;
}

SQInteger sqDebugOverlay::addColordText(HSQUIRRELVM v)
{
	sq_pushstring(v, _SC("pointer"), -1);
	if (SQ_FAILED(sq_get(v, 1)))
	{
		sq_throwerror(v, _SC("指针不存在"));
		return 0;
	}

	CDebugOverlay* pointer = nullptr;
	sq_getuserpointer(v, -1, (SQUserPointer*)&pointer);
	if (pointer == nullptr)
	{
		sq_throwerror(v, _SC("这是一个无效的对象！"));
		return 0;
	}

	Vector pos;
	SQFloat duration;
	const SQChar* text;
	SQInteger lineOffset, color;
	GET_VECTOR2(v, pos, 2);
	sq_getstring(v, 3, &text);
	sq_getfloat(v, 4, &duration);
	sq_getinteger(v, 5, &color);
	sq_getinteger(v, 6, &lineOffset);

	SQInteger r, g, b, a;
	COLOR_TO_RGBA(color, r, g, b, a);

	pointer->AddTextOverlayRGB(pos, lineOffset, duration, r, g, b, a, text);
	return 0;
}

SQInteger sqDebugOverlay::addBox2(HSQUIRRELVM v)
{
	sq_pushstring(v, _SC("pointer"), -1);
	if (SQ_FAILED(sq_get(v, 1)))
	{
		sq_throwerror(v, _SC("指针不存在"));
		return 0;
	}

	CDebugOverlay* pointer = nullptr;
	sq_getuserpointer(v, -1, (SQUserPointer*)&pointer);
	if (pointer == nullptr)
	{
		sq_throwerror(v, _SC("这是一个无效的对象！"));
		return 0;
	}

	SQInteger face, edge;
	SQFloat duration;
	Vector vPos, vAng, vMin, vMax;
	GET_VECTOR2(v, vPos, 2);
	GET_VECTOR2(v, vMin, 3);
	GET_VECTOR2(v, vMax, 4);
	GET_VECTOR2(v, vAng, 5);
	sq_getinteger(v, 6, &face);
	sq_getinteger(v, 7, &edge);
	sq_getfloat(v, 8, &duration);

	SQInteger fa, fr, fg, fb, ea, er, eg, eb;
	COLOR_TO_RGBA(face, fr, fg, fb, fa);
	COLOR_TO_RGBA(edge, er, eg, eb, ea);

	pointer->AddBoxOverlay2(vPos, vMin, vMax, vAng, Color(fr, fg, fb, fa), Color(er, eg, eb, ea), duration);
	return 0;
}
