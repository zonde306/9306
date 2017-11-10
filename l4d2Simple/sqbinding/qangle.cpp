#include "qangle.h"
#include "vector.h"
#include <cstdio>
#include <cmath>

#ifndef M_PI
#define M_PI		3.14159265358979323846	// matches value in gcc v2 math.h
#define M_PI_F		((float)(M_PI))	// Shouldn't collide with anything.
#endif

#ifndef RAD2DEG
#define RAD2DEG(x)  ((float)(x) * (float)(180.f / M_PI_F))
#define RadiansToDegrees RAD2DEG
#endif

#ifndef DEG2RAD
#define DEG2RAD(x)  ((float)(x) * (float)(M_PI_F / 180.f))
#define DegreesToRadians DEG2RAD
#endif

namespace sqQAngle
{
	HSQOBJECT classObject;

#define FILLED_VECTOR(v,x,y,z)	{\
		sq_pushstring(v, _SC("x"), -1);\
		sq_pushfloat(v, x);\
		sq_set(v, -3);\
		sq_pushstring(v, _SC("y"), -1);\
		sq_pushfloat(v, y);\
		sq_set(v, -3);\
		sq_pushstring(v, _SC("z"), -1);\
		sq_pushfloat(v, z);\
		sq_set(v, -3);\
	}

#define GET_VECTOR(v,x,y,z,_idx)		{\
		sq_pushstring(v, _SC("x"), -1);\
		if(SQ_SUCCEEDED(sq_get(v, 1))) {\
			sq_getfloat(v, -1, &x);\
			sq_poptop(v);\
		}\
		sq_pushstring(v, _SC("y"), -1);\
		if(SQ_SUCCEEDED(sq_get(v, 1))) {\
			sq_getfloat(v, -1, &y);\
			sq_poptop(v);\
		}\
		sq_pushstring(v, _SC("z"), -1);\
		if(SQ_SUCCEEDED(sq_get(v, 1))) {\
			sq_getfloat(v, -1, &z);\
			sq_poptop(v);\
		}\
	}

	SQInteger constructor(HSQUIRRELVM v);

	SQInteger _typeof(HSQUIRRELVM v);

	SQInteger _cmp(HSQUIRRELVM v);

	SQInteger _add(HSQUIRRELVM v);

	SQInteger _sub(HSQUIRRELVM v);

	SQInteger _mul(HSQUIRRELVM v);

	SQInteger _div(HSQUIRRELVM v);

	SQInteger _tostring(HSQUIRRELVM v);

	void inline SinCos(float radians, float *sine, float *cosine)
	{
		*sine = sin(radians);
		*cosine = cos(radians);
	}

	SQInteger forward(HSQUIRRELVM v);

	SQInteger right(HSQUIRRELVM v);

	SQInteger up(HSQUIRRELVM v);

	SQInteger normalize(HSQUIRRELVM v);

	SQInteger clamp(HSQUIRRELVM v);
};

void sqQAngle::Initialization(HSQUIRRELVM v)
{
	// 栈顶
	SQInteger top = sq_gettop(v);

	// 类作用域(一般是一个表)
	sq_pushroottable(v);

	// 设置类名字
	sq_pushstring(v, _SC("QAngle"), -1);

	// 使用刚才提供的名字声明一个类
	sq_newclass(v, SQFalse);

	// 获取类本身(不是实例)
	sq_getstackobj(v, -1, &classObject);
	sq_addref(v, &classObject);

	// 将类添加到作用域
	sq_newslot(v, -3, SQFalse);
	sq_pushobject(v, classObject);

	// 定义类成员
	sq_pushstring(v, _SC("x"), -1);
	sq_pushfloat(v, 0.0f);
	sq_newslot(v, -3, SQFalse);

	sq_pushstring(v, _SC("y"), -1);
	sq_pushfloat(v, 0.0f);
	sq_newslot(v, -3, SQFalse);

	sq_pushstring(v, _SC("z"), -1);
	sq_pushfloat(v, 0.0f);
	sq_newslot(v, -3, SQFalse);

	// 构造函数
	sq_pushstring(v, _SC("constructor"), -1);
	sq_newclosure(v, sqQAngle::constructor, 0);
	sq_newslot(v, -3, SQFalse);

	// 运算符
	sq_pushstring(v, _SC("_add"), -1);
	sq_newclosure(v, sqQAngle::_add, 0);
	sq_newslot(v, -3, SQFalse);

	sq_pushstring(v, _SC("_sub"), -1);
	sq_newclosure(v, sqQAngle::_sub, 0);
	sq_newslot(v, -3, SQFalse);

	sq_pushstring(v, _SC("_mul"), -1);
	sq_newclosure(v, sqQAngle::_mul, 0);
	sq_newslot(v, -3, SQFalse);

	sq_pushstring(v, _SC("_div"), -1);
	sq_newclosure(v, sqQAngle::_div, 0);
	sq_newslot(v, -3, SQFalse);

	// 特殊操作符
	sq_pushstring(v, _SC("_typeof"), -1);
	sq_newclosure(v, sqQAngle::_typeof, 0);
	sq_newslot(v, -3, SQFalse);

	sq_pushstring(v, _SC("_tostring"), -1);
	sq_newclosure(v, sqQAngle::_tostring, 0);
	sq_newslot(v, -3, SQFalse);

	// 类方法
	sq_pushstring(v, _SC("up"), -1);
	sq_newclosure(v, sqQAngle::up, 0);
	sq_setparamscheck(v, SQ_MATCHTYPEMASKSTRING, _SC("."));
	sq_newslot(v, -3, SQFalse);

	sq_pushstring(v, _SC("right"), -1);
	sq_newclosure(v, sqQAngle::right, 0);
	sq_setparamscheck(v, SQ_MATCHTYPEMASKSTRING, _SC("."));
	sq_newslot(v, -3, SQFalse);

	sq_pushstring(v, _SC("forward"), -1);
	sq_newclosure(v, sqQAngle::forward, 0);
	sq_setparamscheck(v, SQ_MATCHTYPEMASKSTRING, _SC("."));
	sq_newslot(v, -3, SQFalse);

	sq_pushstring(v, _SC("normalize"), -1);
	sq_newclosure(v, sqQAngle::normalize, 0);
	sq_setparamscheck(v, SQ_MATCHTYPEMASKSTRING, _SC("."));
	sq_newslot(v, -3, SQFalse);

	sq_pushstring(v, _SC("clamp"), -1);
	sq_newclosure(v, sqQAngle::clamp, 0);
	sq_setparamscheck(v, SQ_MATCHTYPEMASKSTRING, _SC("."));
	sq_newslot(v, -3, SQFalse);

	sq_settop(v, top);
}

SQInteger sqQAngle::constructor(HSQUIRRELVM v)
{
	SQInteger argc = sq_gettop(v);

	// 只传递了一个 this 过来(相当于没有参数的构造函数)
	if (argc == 1)
	{
		// 创建实例
		FILLED_VECTOR(v, 0.0f, 0.0f, 0.0f);
	}
	// 传递了一个 this 和三个参数过来(相当于 Vector(x, y, z) 方法)
	else if (argc == 4)
	{
		SQFloat x = 0.0, y = 0.0, z = 0.0;
		sq_getfloat(v, 4, &z);
		sq_getfloat(v, 3, &y);
		sq_getfloat(v, 2, &x);

		// this, x, y, z, 'x'
		sq_pushstring(v, _SC("x"), -1);
		// sq_push(v, 2);
		sq_pushfloat(v, x);
		sq_set(v, 1);

		// this, x, y, z, 'y'
		sq_pushstring(v, _SC("y"), -1);
		// sq_push(v, 3);
		sq_pushfloat(v, y);
		sq_set(v, 1);

		// this, x, y, z, 'z'
		sq_pushstring(v, _SC("z"), -1);
		// sq_push(v, 4);
		sq_pushfloat(v, z);
		sq_set(v, 1);
	}

	return 0;
}

SQInteger sqQAngle::_typeof(HSQUIRRELVM v)
{
	// 返回类名
	sq_pushstring(v, _SC("Vector"), -1);
	return 1;
}

SQInteger sqQAngle::_cmp(HSQUIRRELVM v)
{
	SQFloat x1 = 0.0f, y1 = 0.0f, z1 = 0.0f;
	SQFloat x2 = 0.0f, y2 = 0.0f, z2 = 0.0f;

	GET_VECTOR(v, x1, y1, z1, -2);
	GET_VECTOR(v, x2, y2, z2, -1);

	if (x1 == x2 && y1 == y2 && z1 == z2)
		sq_pushinteger(v, 0);
	else
		sq_pushinteger(v, 1);

	return 1;
}

SQInteger sqQAngle::_add(HSQUIRRELVM v)
{
	SQFloat x1 = 0.0f, y1 = 0.0f, z1 = 0.0f;
	SQFloat x2 = 0.0f, y2 = 0.0f, z2 = 0.0f;

	GET_VECTOR(v, x1, y1, z1, -2);

	if (sq_gettype(v, -1) == SQObjectType::OT_FLOAT)
	{
		sq_getfloat(v, -1, &x2);
		y2 = z2 = x2;
	}
	else if (sq_gettype(v, -1) == SQObjectType::OT_INTEGER)
	{
		SQInteger intVal = 0;
		sq_getinteger(v, -1, &intVal);
		y2 = z2 = x2 = (SQFloat)intVal;
	}
	else
		GET_VECTOR(v, x2, y2, z2, -1);

	sq_clone(v, 1);
	FILLED_VECTOR(v, x1 + y1, x2 + y2, z1 + z2);

	return 1;
}

SQInteger sqQAngle::_sub(HSQUIRRELVM v)
{
	SQFloat x1 = 0.0f, y1 = 0.0f, z1 = 0.0f;
	SQFloat x2 = 0.0f, y2 = 0.0f, z2 = 0.0f;

	GET_VECTOR(v, x1, y1, z1, -2);

	if (sq_gettype(v, -1) == SQObjectType::OT_FLOAT)
	{
		sq_getfloat(v, -1, &x2);
		y2 = z2 = x2;
	}
	else if (sq_gettype(v, -1) == SQObjectType::OT_INTEGER)
	{
		SQInteger intVal = 0;
		sq_getinteger(v, -1, &intVal);
		y2 = z2 = x2 = (SQFloat)intVal;
	}
	else
		GET_VECTOR(v, x2, y2, z2, -1);

	sq_clone(v, 1);
	FILLED_VECTOR(v, x1 - y1, x2 - y2, z1 - z2);

	return 1;
}

SQInteger sqQAngle::_mul(HSQUIRRELVM v)
{
	SQFloat x1 = 0.0f, y1 = 0.0f, z1 = 0.0f;
	SQFloat x2 = 0.0f, y2 = 0.0f, z2 = 0.0f;

	GET_VECTOR(v, x1, y1, z1, -2);

	if (sq_gettype(v, -1) == SQObjectType::OT_FLOAT)
	{
		sq_getfloat(v, -1, &x2);
		y2 = z2 = x2;
	}
	else if (sq_gettype(v, -1) == SQObjectType::OT_INTEGER)
	{
		SQInteger intVal = 0;
		sq_getinteger(v, -1, &intVal);
		y2 = z2 = x2 = (SQFloat)intVal;
	}
	else
		GET_VECTOR(v, x2, y2, z2, -1);

	sq_clone(v, 1);
	FILLED_VECTOR(v, x1 * y1, x2 * y2, z1 * z2);

	return 1;
}

SQInteger sqQAngle::_div(HSQUIRRELVM v)
{
	SQFloat x1 = 0.0f, y1 = 0.0f, z1 = 0.0f;
	SQFloat x2 = 0.0f, y2 = 0.0f, z2 = 0.0f;

	GET_VECTOR(v, x1, y1, z1, -2);

	if (sq_gettype(v, -1) == SQObjectType::OT_FLOAT)
	{
		sq_getfloat(v, -1, &x2);
		y2 = z2 = x2;
	}
	else if (sq_gettype(v, -1) == SQObjectType::OT_INTEGER)
	{
		SQInteger intVal = 0;
		sq_getinteger(v, -1, &intVal);
		y2 = z2 = x2 = (SQFloat)intVal;
	}
	else
		GET_VECTOR(v, x2, y2, z2, -1);

	sq_clone(v, 1);
	FILLED_VECTOR(v, x1 / y1, x2 / y2, z1 / z2);

	return 1;
}

SQInteger sqQAngle::_tostring(HSQUIRRELVM v)
{
	SQInteger argc = sq_gettop(v);
	SQFloat x = 0.0f, y = 0.0f, z = 0.0f;
	GET_VECTOR(v, x, y, z, -1);

	SQChar buffer[255];
	SQInteger len = sprintf_s(buffer, "%f, %f, %f", x, y, z);
	sq_pushstring(v, buffer, len);

	return 1;
}

SQInteger sqQAngle::forward(HSQUIRRELVM v)
{
	SQInteger argc = sq_gettop(v);
	SQFloat x = 0.0f, y = 0.0f, z = 0.0f;
	GET_VECTOR(v, x, y, z, -1);

	SQFloat sp, sy, cp, cy;
	SinCos(DEG2RAD(y), &sy, &cy);
	SinCos(DEG2RAD(x), &sp, &cp);

	sq_pushobject(v, sqQAngle::classObject);
	sq_createinstance(v, -1);
	FILLED_VECTOR(v, cp * cy, cp * sy, -sp);

	return 1;
}

SQInteger sqQAngle::right(HSQUIRRELVM v)
{
	SQInteger argc = sq_gettop(v);
	SQFloat x = 0.0f, y = 0.0f, z = 0.0f;
	GET_VECTOR(v, x, y, z, -1);

	SQFloat sp, sy, cp, cy, sr, cr;
	SinCos(DEG2RAD(y), &sy, &cy);
	SinCos(DEG2RAD(x), &sp, &cp);
	SinCos(DEG2RAD(z), &sr, &cr);

	sq_pushobject(v, sqQAngle::classObject);
	sq_createinstance(v, -1);
	FILLED_VECTOR(v, (-1 * sr*sp*cy + -1 * cr*-sy), (-1 * sr*sp*sy + -1 * cr*cy), -1 * sr*cp);

	return 1;
}

SQInteger sqQAngle::up(HSQUIRRELVM v)
{
	SQInteger argc = sq_gettop(v);
	SQFloat x = 0.0f, y = 0.0f, z = 0.0f;
	GET_VECTOR(v, x, y, z, -1);

	SQFloat sp, sy, cp, cy, sr, cr;
	SinCos(DEG2RAD(y), &sy, &cy);
	SinCos(DEG2RAD(x), &sp, &cp);
	SinCos(DEG2RAD(z), &sr, &cr);

	sq_pushobject(v, sqQAngle::classObject);
	sq_createinstance(v, -1);
	FILLED_VECTOR(v, (cr*sp*cy + -sr*-sy), (cr*sp*sy + -sr*cy), cr*cp);

	return 1;
}

SQInteger sqQAngle::normalize(HSQUIRRELVM v)
{
	SQInteger argc = sq_gettop(v);
	SQFloat x = 0.0f, y = 0.0f, z = 0.0f;
	GET_VECTOR(v, x, y, z, -1);

	if (x < -180.0f)
		x += 360.0f;
	if (x > 180.0f)
		x -= 360.0f;

	if (y < -180.0f)
		y += 360.0f;
	if (y > 180.0f)
		y -= 360.0f;

	if (z < -180.0f)
		z += 360.0f;
	if (z > 180.0f)
		z -= 360.0f;

	sq_clone(v, 1);
	FILLED_VECTOR(v, x, y, z);

	return 1;
}

SQInteger sqQAngle::clamp(HSQUIRRELVM v)
{
	SQInteger argc = sq_gettop(v);
	SQFloat x = 0.0f, y = 0.0f, z = 0.0f;
	GET_VECTOR(v, x, y, z, -1);

	if (x > 89.0f && x <= 180.0f)
		x = 89.0f;

	if (x > 180.0f)
		x = x - 360.0f;

	if (x < -89.0f)
		x = -89.0f;

	y = fmodf(y + 180.0f, 360.0f) - 180.0f;

	z = 0;

	sq_clone(v, 1);
	FILLED_VECTOR(v, x, y, z);

	return 1;
}
