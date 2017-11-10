#include "vector.h"
#include "qangle.h"
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

namespace sqVector
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

	SQInteger _typeof(HSQUIRRELVM v);

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

#define GET_VECTOR2(v,vec,_idx)		{\
		sq_pushstring(v, _SC("x"), -1);\
		if(SQ_SUCCEEDED(sq_get(v, 1))) {\
			sq_getfloat(v, -1, &(vec.x));\
			sq_poptop(v);\
		}\
		sq_pushstring(v, _SC("y"), -1);\
		if(SQ_SUCCEEDED(sq_get(v, 1))) {\
			sq_getfloat(v, -1, &(vec.y));\
			sq_poptop(v);\
		}\
		sq_pushstring(v, _SC("z"), -1);\
		if(SQ_SUCCEEDED(sq_get(v, 1))) {\
			sq_getfloat(v, -1, &(vec.z));\
			sq_poptop(v);\
		}\
	}

	SQInteger _cmp(HSQUIRRELVM v);

	SQInteger _add(HSQUIRRELVM v);

	SQInteger _sub(HSQUIRRELVM v);

	SQInteger _mul(HSQUIRRELVM v);

	SQInteger _div(HSQUIRRELVM v);

	SQInteger _tostring(HSQUIRRELVM v);

	SQInteger normalize(HSQUIRRELVM v);

	SQInteger length(HSQUIRRELVM v);

	SQInteger cross(HSQUIRRELVM v);

	SQInteger dot(HSQUIRRELVM v);

	SQInteger distance(HSQUIRRELVM v);

	SQInteger toangles(HSQUIRRELVM v);
};

void sqVector::Initialization(HSQUIRRELVM v)
{
	// 栈顶
	SQInteger top = sq_gettop(v);

	// 类作用域(一般是一个表)
	sq_pushroottable(v);

	// 设置类名字
	sq_pushstring(v, _SC("Vector"), -1);

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
	sq_newclosure(v, sqVector::constructor, 0);
	sq_newslot(v, -3, SQFalse);

	// 运算符
	sq_pushstring(v, _SC("_add"), -1);
	sq_newclosure(v, sqVector::_add, 0);
	sq_newslot(v, -3, SQFalse);

	sq_pushstring(v, _SC("_sub"), -1);
	sq_newclosure(v, sqVector::_sub, 0);
	sq_newslot(v, -3, SQFalse);

	sq_pushstring(v, _SC("_mul"), -1);
	sq_newclosure(v, sqVector::_mul, 0);
	sq_newslot(v, -3, SQFalse);

	sq_pushstring(v, _SC("_div"), -1);
	sq_newclosure(v, sqVector::_div, 0);
	sq_newslot(v, -3, SQFalse);

	// 特殊操作符
	sq_pushstring(v, _SC("_typeof"), -1);
	sq_newclosure(v, sqVector::_typeof, 0);
	sq_newslot(v, -3, SQFalse);

	sq_pushstring(v, _SC("_tostring"), -1);
	sq_newclosure(v, sqVector::_tostring, 0);
	sq_newslot(v, -3, SQFalse);

	// 类方法
	sq_pushstring(v, _SC("normalize"), -1);
	sq_newclosure(v, sqVector::normalize, 0);
	sq_setparamscheck(v, SQ_MATCHTYPEMASKSTRING, _SC("."));
	sq_newslot(v, -3, SQFalse);

	sq_pushstring(v, _SC("length"), -1);
	sq_newclosure(v, sqVector::length, 0);
	sq_setparamscheck(v, SQ_MATCHTYPEMASKSTRING, _SC("."));
	sq_newslot(v, -3, SQFalse);

	sq_pushstring(v, _SC("dot"), -1);
	sq_newclosure(v, sqVector::dot, 0);
	sq_setparamscheck(v, SQ_MATCHTYPEMASKSTRING, _SC("."));
	sq_newslot(v, -3, SQFalse);

	sq_pushstring(v, _SC("cross"), -1);
	sq_newclosure(v, sqVector::cross, 0);
	sq_setparamscheck(v, SQ_MATCHTYPEMASKSTRING, _SC("."));
	sq_newslot(v, -3, SQFalse);

	sq_pushstring(v, _SC("distance"), -1);
	sq_newclosure(v, sqVector::distance, 0);
	sq_setparamscheck(v, SQ_MATCHTYPEMASKSTRING, _SC("."));
	sq_newslot(v, -3, SQFalse);

	sq_pushstring(v, _SC("toangles"), -1);
	sq_newclosure(v, sqVector::toangles, 0);
	sq_setparamscheck(v, SQ_MATCHTYPEMASKSTRING, _SC("."));
	sq_newslot(v, -3, SQFalse);

	sq_settop(v, top);
}

SQInteger sqVector::constructor(HSQUIRRELVM v)
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

SQInteger sqVector::_typeof(HSQUIRRELVM v)
{
	// 返回类名
	sq_pushstring(v, _SC("Vector"), -1);
	return 1;
}

SQInteger sqVector::_cmp(HSQUIRRELVM v)
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

SQInteger sqVector::_add(HSQUIRRELVM v)
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

SQInteger sqVector::_sub(HSQUIRRELVM v)
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

SQInteger sqVector::_mul(HSQUIRRELVM v)
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

SQInteger sqVector::_div(HSQUIRRELVM v)
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

SQInteger sqVector::_tostring(HSQUIRRELVM v)
{
	SQInteger argc = sq_gettop(v);
	SQFloat x = 0.0f, y = 0.0f, z = 0.0f;
	GET_VECTOR(v, x, y, z, -1);

	SQChar buffer[255];
	SQInteger len = sprintf_s(buffer, "%f, %f, %f", x, y, z);
	sq_pushstring(v, buffer, len);

	return 1;
}

SQInteger sqVector::normalize(HSQUIRRELVM v)
{
	SQFloat x = 0.0f, y = 0.0f, z = 0.0f;
	GET_VECTOR(v, x, y, z, -1);

	SQFloat magSq = x * x + y * y + z * z;
	if (magSq > 0.0f)
	{
		SQFloat oneOverMag = 1.0f / sqrt(magSq);

		sq_clone(v, 1);
		FILLED_VECTOR(v, x * oneOverMag, y * oneOverMag, z * oneOverMag);
	}
	else
		sq_throwerror(v, _SC("无效的向量"));

	return 1;
}

SQInteger sqVector::length(HSQUIRRELVM v)
{
	SQFloat x = 0.0f, y = 0.0f, z = 0.0f;
	GET_VECTOR(v, x, y, z, -1);

	sq_pushfloat(v, sqrt(x * x + y * y + z * z));

	return 1;
}

SQInteger sqVector::cross(HSQUIRRELVM v)
{
	SQFloat x1 = 0.0f, y1 = 0.0f, z1 = 0.0f;
	SQFloat x2 = 0.0f, y2 = 0.0f, z2 = 0.0f;
	GET_VECTOR(v, x1, y1, z1, -2);
	GET_VECTOR(v, x2, y2, z2, -1);

	sq_clone(v, 1);
	FILLED_VECTOR(v, y1 * z2 - z1 * y2, z1 * x2 - x1 * z2, x1 * y2 - y1 * x2);

	return 1;
}

SQInteger sqVector::dot(HSQUIRRELVM v)
{
	SQFloat x1 = 0.0f, y1 = 0.0f, z1 = 0.0f;
	SQFloat x2 = 0.0f, y2 = 0.0f, z2 = 0.0f;
	GET_VECTOR(v, x1, y1, z1, -2);
	GET_VECTOR(v, x2, y2, z2, -1);

	sq_pushfloat(v, x1 * x2 + y1 * y2 + z1 * z2);
	return 1;
}

SQInteger sqVector::distance(HSQUIRRELVM v)
{
	SQFloat x1 = 0.0f, y1 = 0.0f, z1 = 0.0f;
	SQFloat x2 = 0.0f, y2 = 0.0f, z2 = 0.0f;
	GET_VECTOR(v, x1, y1, z1, -2);
	GET_VECTOR(v, x2, y2, z2, -1);

	SQFloat dx = x1 - x2, dy = y1 - y2, dz = z1 - z2;
	sq_pushfloat(v, sqrt(dx * dx + dy * dy + dz * dz));

	return 1;
}

SQInteger sqVector::toangles(HSQUIRRELVM v)
{
	SQFloat x = 0.0f, y = 0.0f, z = 0.0f;
	GET_VECTOR(v, x, y, z, -1);

	SQFloat yaw, pitch;
	if (y == 0.0f && x == 0.0f)
	{
		yaw = 0.0f;
		if (z > 0.0f)
			pitch = 270.0f;
		else
			pitch = 90.0f;
	}
	else
	{
		yaw = (atan2(y, x) * 180.0f / M_PI_F);
		if (yaw < 0.0f)
			yaw += 360.0f;

		pitch = (atan2(-z, sqrt(x * x + y * y)) * 180.0f / M_PI_F);
		if (pitch < 0.0f)
			pitch += 360.0f;
	}

	sq_pushobject(v, sqVector::classObject);
	sq_createinstance(v, -1);
	FILLED_VECTOR(v, pitch, yaw, 0.0f);

	return 1;
}
