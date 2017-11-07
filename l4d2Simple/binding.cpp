#include "binding.h"
#include <string>
#include <cstdarg>
#include "./structs/vector.h"

#include "./sqbinding/vector.h"
#include "./sqbinding/qangle.h"
#include "./sqbinding/draw.h"
#include "./sqbinding/surface.h"
#include "./sqbinding/debugoverlay.h"

extern std::string g_sCurPath;
typedef Vector QAngle;

namespace sqb
{
	HSQUIRRELVM vm = nullptr;
	HSQOBJECT cVector, cQAngle, cEntity, cPlayer, cClient, cEngine, cEntList, cGameEvent;

	void _sqprintf(HSQUIRRELVM v, const SQChar * text, ...);
	void _sqerrorf(HSQUIRRELVM v, const SQChar * text, ...);
	void _sqerror(HSQUIRRELVM v, const SQChar * desc, const SQChar * source,
		SQInteger line, SQInteger column);

	using string = std::basic_string<SQChar>;
};

void sqb::Initialization()
{
	vm = sq_open(1024);
	sqstd_seterrorhandlers(vm);
	sq_setprintfunc(vm, _sqprintf, _sqerrorf);
	sq_setcompilererrorhandler(vm, _sqerror);

	// 设置文件的加载环境
	sq_pushroottable(vm);
	sqstd_register_bloblib(vm);
	sqstd_register_iolib(vm);
	sqstd_register_systemlib(vm);
	sqstd_register_mathlib(vm);
	sqstd_register_stringlib(vm);

	// 加载文件
	sqstd_dofile(vm, std::string(g_sCurPath + "\\config.nut").c_str(), SQFalse, SQTrue);

	// 定义各种类
	sqVector::Initialization(vm);
	sqQAngle::Initialization(vm);
	sqDraw::Initialization(vm);
	sqSurface::Initialization(vm);
	sqDebugOverlay::Initialization(vm);
}

// 调用 main 函数，传递了目前 DLL 路径
void sqb::CheatStart()
{
	if (vm == nullptr)
		return;
	
	SQInteger top = sq_gettop(vm);
	
	sq_pushroottable(vm);
	sq_pushstring(vm, _SC("main"), -1);
	if (SQ_FAILED(sq_get(vm, -2)))
	{
		// 找不到 main 函数
		sq_settop(vm, top);
		return;
	}

	sq_pushroottable(vm);
	sq_pushstring(vm, g_sCurPath.c_str(), g_sCurPath.length());
	sq_call(vm, 2, SQFalse, SQTrue);

	sq_settop(vm, top);
}

bool sqb::DrawIndexedPrimitive(SQInteger stride, SQInteger vertices, SQInteger primitive)
{
	if (vm == nullptr)
		return false;
	
	SQInteger top = sq_gettop(vm);

	sq_pushroottable(vm);
	sq_pushstring(vm, _SC("Direct3DHook"), -1);
	if (SQ_FAILED(sq_get(vm, -2)))
	{
		sq_settop(vm, top);
		return false;
	}

	sq_pushstring(vm, _SC("DrawIndexedPrimitive"), -1);
	if (SQ_FAILED(sq_get(vm, -2)))
	{
		sq_settop(vm, top);
		return false;
	}

	// 返回值，返回最高的值
	SQBool height = SQFalse;

	// 检查是否为一个函数
	if (sq_gettype(vm, -1) == SQObjectType::OT_CLOSURE)
	{
		sq_pushroottable(vm);
		sq_pushinteger(vm, stride);
		sq_pushinteger(vm, vertices);
		sq_pushinteger(vm, primitive);
		sq_call(vm, 4, SQTrue, SQTrue);

		if(sq_gettype(vm, -1) == SQObjectType::OT_BOOL)
			sq_getbool(vm, -1, &height);
	}
	else if(sq_gettype(vm, -1) == SQObjectType::OT_TABLE)
	{
		sq_pushnull(vm);

		SQBool result = SQFalse;
		while (SQ_SUCCEEDED(sq_next(vm, -2)))
		{
			if (sq_gettype(vm, -1) == SQObjectType::OT_CLOSURE)
			{
				sq_pushroottable(vm);
				sq_pushinteger(vm, stride);
				sq_pushinteger(vm, vertices);
				sq_pushinteger(vm, primitive);
				sq_call(vm, 4, SQTrue, SQTrue);

				if (sq_gettype(vm, -1) == SQObjectType::OT_BOOL)
					sq_getbool(vm, -1, &result);

				if (result == SQTrue)
					height = SQTrue;
			}

			result = SQFalse;
			sq_pop(vm, 2);
		}

		sq_poptop(vm);
	}

	sq_settop(vm, top);
	return (height == SQTrue);
}

void sqb::EndScene(SQUserPointer drawPointer)
{
	if (vm == nullptr)
		return;
	
	SQInteger top = sq_gettop(vm);

	sq_pushroottable(vm);
	sq_pushstring(vm, _SC("Direct3DHook"), -1);
	if (SQ_FAILED(sq_get(vm, -2)))
	{
		sq_settop(vm, top);
		return;
	}

	sq_pushstring(vm, _SC("EndScene"), -1);
	if (SQ_FAILED(sq_get(vm, -2)))
	{
		sq_settop(vm, top);
		return;
	}

	sq_pushobject(vm, sqDraw::classObject);
	sq_createinstance(vm, -1);

	HSQOBJECT inst;
	sq_getstackobj(vm, -1, &inst);
	sq_addref(vm, &inst);
	
	sq_pushstring(vm, _SC("pointer"), -1);
	sq_pushuserpointer(vm, drawPointer);
	sq_set(vm, -3);

	// 检查是否为一个函数
	if (sq_gettype(vm, -1) == SQObjectType::OT_CLOSURE)
	{
		sq_pushroottable(vm);
		sq_pushobject(vm, inst);
		sq_call(vm, 2, SQFalse, SQTrue);
	}
	else if (sq_gettype(vm, -1) == SQObjectType::OT_TABLE)
	{
		sq_pushnull(vm);

		while (SQ_SUCCEEDED(sq_next(vm, -2)))
		{
			if (sq_gettype(vm, -1) == SQObjectType::OT_CLOSURE)
			{
				sq_pushroottable(vm);
				sq_pushobject(vm, inst);
				sq_call(vm, 2, SQFalse, SQTrue);
			}

			sq_pop(vm, 2);
		}

		sq_poptop(vm);
	}

	sq_release(vm, &inst);
	sq_settop(vm, top);
}

SQInteger sqb::CreateQuery(SQInteger queryType)
{
	if (vm == nullptr)
		return -1;
	
	SQInteger top = sq_gettop(vm);

	sq_pushroottable(vm);
	sq_pushstring(vm, _SC("Direct3DHook"), -1);
	if (SQ_FAILED(sq_get(vm, -2)))
	{
		sq_settop(vm, top);
		return -1;
	}

	sq_pushstring(vm, _SC("CreateQuery"), -1);
	if (SQ_FAILED(sq_get(vm, -2)))
	{
		sq_settop(vm, top);
		return -1;
	}

	// 返回值
	SQInteger result = -1;

	// 检查是否为一个函数
	if (sq_gettype(vm, -1) == SQObjectType::OT_CLOSURE)
	{
		sq_pushroottable(vm);
		sq_pushinteger(vm, queryType);
		sq_call(vm, 2, SQTrue, SQTrue);

		if (sq_gettype(vm, -1) == SQObjectType::OT_INTEGER)
			sq_getinteger(vm, -1, &result);
	}
	else if (sq_gettype(vm, -1) == SQObjectType::OT_TABLE)
	{
		sq_pushnull(vm);

		while (SQ_SUCCEEDED(sq_next(vm, -2)))
		{
			if (sq_gettype(vm, -1) == SQObjectType::OT_CLOSURE)
			{
				sq_pushroottable(vm);
				sq_pushinteger(vm, queryType);
				sq_call(vm, 2, SQTrue, SQTrue);

				if (sq_gettype(vm, -1) == SQObjectType::OT_INTEGER)
					sq_getinteger(vm, -1, &result);
			}

			sq_pop(vm, 2);
		}

		sq_poptop(vm);
	}

	sq_settop(vm, top);
	return result;
}

void sqb::Present(SQUserPointer drawPointer)
{
	if (vm == nullptr)
		return;
	
	SQInteger top = sq_gettop(vm);

	sq_pushroottable(vm);
	sq_pushstring(vm, _SC("Direct3DHook"), -1);
	if (SQ_FAILED(sq_get(vm, -2)))
	{
		sq_settop(vm, top);
		return;
	}

	sq_pushstring(vm, _SC("Present"), -1);
	if (SQ_FAILED(sq_get(vm, -2)))
	{
		sq_settop(vm, top);
		return;
	}

	sq_pushobject(vm, sqDraw::classObject);
	sq_createinstance(vm, -1);

	HSQOBJECT inst;
	sq_getstackobj(vm, -1, &inst);
	sq_addref(vm, &inst);

	sq_pushstring(vm, _SC("pointer"), -1);
	sq_pushuserpointer(vm, drawPointer);
	sq_set(vm, -3);

	// 检查是否为一个函数
	if (sq_gettype(vm, -1) == SQObjectType::OT_CLOSURE)
	{
		sq_pushroottable(vm);
		sq_pushobject(vm, inst);
		sq_call(vm, 2, SQFalse, SQTrue);
	}
	else if (sq_gettype(vm, -1) == SQObjectType::OT_TABLE)
	{
		sq_pushnull(vm);

		while (SQ_SUCCEEDED(sq_next(vm, -2)))
		{
			if (sq_gettype(vm, -1) == SQObjectType::OT_CLOSURE)
			{
				sq_pushroottable(vm);
				sq_pushobject(vm, inst);
				sq_call(vm, 2, SQFalse, SQTrue);
			}

			sq_pop(vm, 2);
		}

		sq_poptop(vm);
	}

	sq_release(vm, &inst);
	sq_settop(vm, top);
}

void sqb::PaintTraverse(const SQChar* panelName, SQUserPointer surfacePointer)
{
	if (vm == nullptr)
		return;
	
	SQInteger top = sq_gettop(vm);

	sq_pushroottable(vm);
	sq_pushstring(vm, _SC("GameHook"), -1);
	if (SQ_FAILED(sq_get(vm, -2)))
	{
		sq_settop(vm, top);
		return;
	}

	sq_pushstring(vm, _SC("PaintTraverse"), -1);
	if (SQ_FAILED(sq_get(vm, -2)))
	{
		sq_settop(vm, top);
		return;
	}

	sq_pushobject(vm, sqSurface::classObject);
	sq_createinstance(vm, -1);

	HSQOBJECT inst;
	sq_getstackobj(vm, -1, &inst);
	sq_addref(vm, &inst);

	sq_pushstring(vm, _SC("pointer"), -1);
	sq_pushuserpointer(vm, surfacePointer);
	sq_set(vm, -3);

	// 检查是否为一个函数
	if (sq_gettype(vm, -1) == SQObjectType::OT_CLOSURE)
	{
		sq_pushroottable(vm);
		sq_pushstring(vm, panelName, -1);
		sq_pushobject(vm, inst);
		sq_call(vm, 3, SQFalse, SQTrue);
	}
	else if (sq_gettype(vm, -1) == SQObjectType::OT_TABLE)
	{
		sq_pushnull(vm);

		while (SQ_SUCCEEDED(sq_next(vm, -2)))
		{
			if (sq_gettype(vm, -1) == SQObjectType::OT_CLOSURE)
			{
				sq_pushroottable(vm);
				sq_pushstring(vm, panelName, -1);
				sq_pushobject(vm, inst);
				sq_call(vm, 3, SQFalse, SQTrue);
			}

			sq_pop(vm, 2);
		}

		sq_poptop(vm);
	}

	sq_release(vm, &inst);
	sq_settop(vm, top);
}

bool sqb::CreateMove(CUserCmd* pCmd, SQBool bSendPacket)
{
	if (vm == nullptr)
		return true;
	
	SQInteger top = sq_gettop(vm);

	sq_pushroottable(vm);
	sq_pushstring(vm, _SC("GameHook"), -1);
	if (SQ_FAILED(sq_get(vm, -2)))
	{
		sq_settop(vm, top);
		return true;
	}

	sq_pushstring(vm, _SC("CreateMove"), -1);
	if (SQ_FAILED(sq_get(vm, -2)))
	{
		sq_settop(vm, top);
		return true;
	}
	
	SQInteger top_table = sq_gettop(vm);
	sq_newtable(sqb::vm);
	HSQOBJECT table;
	sq_getstackobj(sqb::vm, -1, &table);
	sq_addref(sqb::vm, &table);

	sq_pushstring(sqb::vm, _SC("command_number"), -1);
	sq_pushinteger(sqb::vm, pCmd->command_number);
	sq_newslot(sqb::vm, -3, SQFalse);

	sq_pushstring(sqb::vm, _SC("tick_count"), -1);
	sq_pushinteger(sqb::vm, pCmd->tick_count);
	sq_newslot(sqb::vm, -3, SQFalse);

	HSQOBJECT inst;
	SQInteger top_inst = sq_gettop(sqb::vm);
	sq_pushobject(sqb::vm, sqQAngle::classObject);
	sq_createinstance(sqb::vm, -1);
	sq_getstackobj(sqb::vm, -1, &inst);
	sq_addref(sqb::vm, &inst);

	sq_pushstring(sqb::vm, _SC("x"), -1);
	sq_pushfloat(sqb::vm, pCmd->viewangles.x);
	sq_set(sqb::vm, -3);

	sq_pushstring(sqb::vm, _SC("y"), -1);
	sq_pushfloat(sqb::vm, pCmd->viewangles.y);
	sq_set(sqb::vm, -3);

	sq_pushstring(sqb::vm, _SC("z"), -1);
	sq_pushfloat(sqb::vm, pCmd->viewangles.z);
	sq_set(sqb::vm, -3);

	sq_settop(sqb::vm, top_inst);

	sq_pushstring(sqb::vm, _SC("viewangles"), -1);
	sq_pushobject(sqb::vm, inst);
	sq_newslot(sqb::vm, -3, SQFalse);

	sq_pushstring(sqb::vm, _SC("fowardmove"), -1);
	sq_pushfloat(sqb::vm, pCmd->fowardmove);
	sq_newslot(sqb::vm, -3, SQFalse);

	sq_pushstring(sqb::vm, _SC("sidemove"), -1);
	sq_pushfloat(sqb::vm, pCmd->sidemove);
	sq_newslot(sqb::vm, -3, SQFalse);

	sq_pushstring(sqb::vm, _SC("upmove"), -1);
	sq_pushfloat(sqb::vm, pCmd->upmove);
	sq_newslot(sqb::vm, -3, SQFalse);

	sq_pushstring(sqb::vm, _SC("buttons"), -1);
	sq_pushinteger(sqb::vm, pCmd->buttons);
	sq_newslot(sqb::vm, -3, SQFalse);

	sq_pushstring(sqb::vm, _SC("random_seed"), -1);
	sq_pushinteger(sqb::vm, pCmd->random_seed);
	sq_newslot(sqb::vm, -3, SQFalse);

	sq_settop(vm, top_table);
	SQBool height = SQTrue;
	
	// 检查是否为一个函数
	if (sq_gettype(vm, -1) == SQObjectType::OT_CLOSURE)
	{
		sq_pushroottable(vm);
		sq_pushobject(vm, table);
		sq_pushbool(vm, bSendPacket);
		sq_call(vm, 3, SQTrue, SQTrue);

		if (sq_gettype(vm, -1) == SQObjectType::OT_BOOL)
			sq_getbool(vm, -1, &height);
	}
	else if (sq_gettype(vm, -1) == SQObjectType::OT_TABLE)
	{
		sq_pushnull(vm);
		SQBool result = SQTrue;

		while (SQ_SUCCEEDED(sq_next(vm, -2)))
		{
			if (sq_gettype(vm, -1) == SQObjectType::OT_CLOSURE)
			{
				sq_pushroottable(vm);
				sq_pushobject(vm, table);
				sq_pushbool(vm, bSendPacket);
				sq_call(vm, 3, SQTrue, SQTrue);

				if (sq_gettype(vm, -1) == SQObjectType::OT_BOOL)
				{
					sq_getbool(vm, -1, &result);
					if (result == SQFalse)
						height = SQFalse;
				}
			}

			result = SQTrue;
			sq_pop(vm, 2);
		}

		sq_poptop(vm);
	}

	sq_release(vm, &inst);
	sq_release(vm, &table);
	sq_settop(vm, top);

	return (height == SQTrue);
}

void sqb::FrameStageNotify(SQInteger stage, SQUserPointer debugPointer)
{
	if (vm == nullptr)
		return;
	
	SQInteger top = sq_gettop(vm);

	sq_pushroottable(vm);
	sq_pushstring(vm, _SC("GameHook"), -1);
	if (SQ_FAILED(sq_get(vm, -2)))
	{
		sq_settop(vm, top);
		return;
	}

	sq_pushstring(vm, _SC("FrameStageNotify"), -1);
	if (SQ_FAILED(sq_get(vm, -2)))
	{
		sq_settop(vm, top);
		return;
	}

	sq_pushobject(vm, sqDebugOverlay::classObject);
	sq_createinstance(vm, -1);

	HSQOBJECT inst;
	sq_getstackobj(vm, -1, &inst);
	sq_addref(vm, &inst);

	sq_pushstring(vm, _SC("pointer"), -1);
	sq_pushuserpointer(vm, debugPointer);
	sq_set(vm, -3);

	// 检查是否为一个函数
	if (sq_gettype(vm, -1) == SQObjectType::OT_CLOSURE)
	{
		sq_pushroottable(vm);
		sq_pushinteger(vm, stage);
		sq_pushobject(vm, inst);
		sq_call(vm, 3, SQFalse, SQTrue);
	}
	else if (sq_gettype(vm, -1) == SQObjectType::OT_TABLE)
	{
		sq_pushnull(vm);

		while (SQ_SUCCEEDED(sq_next(vm, -2)))
		{
			if (sq_gettype(vm, -1) == SQObjectType::OT_CLOSURE)
			{
				sq_pushroottable(vm);
				sq_pushinteger(vm, stage);
				sq_pushobject(vm, inst);
				sq_call(vm, 3, SQFalse, SQTrue);
			}

			sq_pop(vm, 2);
		}

		sq_poptop(vm);
	}

	sq_release(vm, &inst);
	sq_settop(vm, top);
}

void sqb::KeyInput(SQInteger keyCode, SQBool hasDown, const SQChar * binding)
{
	if (vm == nullptr)
		return;
	
	SQInteger top = sq_gettop(vm);

	sq_pushroottable(vm);
	sq_pushstring(vm, _SC("GameHook"), -1);
	if (SQ_FAILED(sq_get(vm, -2)))
	{
		sq_settop(vm, top);
		return;
	}

	sq_pushstring(vm, _SC("KeyInput"), -1);
	if (SQ_FAILED(sq_get(vm, -2)))
	{
		sq_settop(vm, top);
		return;
	}

	// 检查是否为一个函数
	if (sq_gettype(vm, -1) == SQObjectType::OT_CLOSURE)
	{
		sq_pushroottable(vm);
		sq_pushinteger(vm, keyCode);
		sq_pushbool(vm, hasDown);
		sq_pushstring(vm, binding, -1);
		sq_call(vm, 4, SQFalse, SQTrue);
	}
	else if (sq_gettype(vm, -1) == SQObjectType::OT_TABLE)
	{
		sq_pushnull(vm);

		while (SQ_SUCCEEDED(sq_next(vm, -2)))
		{
			if (sq_gettype(vm, -1) == SQObjectType::OT_CLOSURE)
			{
				sq_pushroottable(vm);
				sq_pushinteger(vm, keyCode);
				sq_pushbool(vm, hasDown);
				sq_pushstring(vm, binding, -1);
				sq_call(vm, 4, SQFalse, SQTrue);
			}

			sq_pop(vm, 2);
		}

		sq_poptop(vm);
	}

	sq_settop(vm, top);
}

void sqb::Paint(SQInteger paintMode)
{
	if (vm == nullptr)
		return;
	
	SQInteger top = sq_gettop(vm);

	sq_pushroottable(vm);
	sq_pushstring(vm, _SC("GameHook"), -1);
	if (SQ_FAILED(sq_get(vm, -2)))
	{
		sq_settop(vm, top);
		return;
	}

	sq_pushstring(vm, _SC("KeyInput"), -1);
	if (SQ_FAILED(sq_get(vm, -2)))
	{
		sq_settop(vm, top);
		return;
	}

	// 检查是否为一个函数
	if (sq_gettype(vm, -1) == SQObjectType::OT_CLOSURE)
	{
		sq_pushroottable(vm);
		sq_pushinteger(vm, paintMode);
		sq_call(vm, 2, SQFalse, SQTrue);
	}
	else if (sq_gettype(vm, -1) == SQObjectType::OT_TABLE)
	{
		sq_pushnull(vm);

		while (SQ_SUCCEEDED(sq_next(vm, -2)))
		{
			if (sq_gettype(vm, -1) == SQObjectType::OT_CLOSURE)
			{
				sq_pushroottable(vm);
				sq_pushinteger(vm, paintMode);
				sq_call(vm, 2, SQFalse, SQTrue);
			}

			sq_pop(vm, 2);
		}

		sq_poptop(vm);
	}

	sq_settop(vm, top);
}

const SQChar * sqb::ProcessGetCvarValue(const SQChar * cvar, const SQChar * value)
{
	if (vm == nullptr)
		return nullptr;
	
	SQInteger top = sq_gettop(vm);

	sq_pushroottable(vm);
	sq_pushstring(vm, _SC("GameHook"), -1);
	if (SQ_FAILED(sq_get(vm, -2)))
	{
		sq_settop(vm, top);
		return nullptr;
	}

	sq_pushstring(vm, _SC("ProcessGetCvarValue"), -1);
	if (SQ_FAILED(sq_get(vm, -2)))
	{
		sq_settop(vm, top);
		return nullptr;
	}

	const SQChar* height = nullptr;

	// 检查是否为一个函数
	if (sq_gettype(vm, -1) == SQObjectType::OT_CLOSURE)
	{
		sq_pushroottable(vm);
		sq_pushstring(vm, cvar, -1);
		sq_pushstring(vm, value, -1);
		sq_call(vm, 3, SQTrue, SQTrue);

		if (sq_gettype(vm, -1) == SQObjectType::OT_STRING)
			sq_getstring(vm, -1, &height);
	}
	else if (sq_gettype(vm, -1) == SQObjectType::OT_TABLE)
	{
		sq_pushnull(vm);

		while (SQ_SUCCEEDED(sq_next(vm, -2)))
		{
			if (sq_gettype(vm, -1) == SQObjectType::OT_CLOSURE)
			{
				sq_pushroottable(vm);
				sq_pushstring(vm, cvar, -1);
				sq_pushstring(vm, value, -1);
				sq_call(vm, 3, SQTrue, SQTrue);

				if (sq_gettype(vm, -1) == SQObjectType::OT_STRING)
					sq_getstring(vm, -1, &height);
			}

			sq_pop(vm, 2);
		}

		sq_poptop(vm);
	}

	sq_settop(vm, top);
	return height;
}

const SQChar * sqb::ProcessSetConVar(const SQChar * cvar, const SQChar * value)
{
	if (vm == nullptr)
		return nullptr;
	
	SQInteger top = sq_gettop(vm);

	sq_pushroottable(vm);
	sq_pushstring(vm, _SC("GameHook"), -1);
	if (SQ_FAILED(sq_get(vm, -2)))
	{
		sq_settop(vm, top);
		return nullptr;
	}

	sq_pushstring(vm, _SC("ProcessSetConVar"), -1);
	if (SQ_FAILED(sq_get(vm, -2)))
	{
		sq_settop(vm, top);
		return nullptr;
	}

	const SQChar* height = nullptr;

	// 检查是否为一个函数
	if (sq_gettype(vm, -1) == SQObjectType::OT_CLOSURE)
	{
		sq_pushroottable(vm);
		sq_pushstring(vm, cvar, -1);
		sq_pushstring(vm, value, -1);
		sq_call(vm, 3, SQTrue, SQTrue);

		if (sq_gettype(vm, -1) == SQObjectType::OT_STRING)
			sq_getstring(vm, -1, &height);
	}
	else if (sq_gettype(vm, -1) == SQObjectType::OT_TABLE)
	{
		sq_pushnull(vm);

		while (SQ_SUCCEEDED(sq_next(vm, -2)))
		{
			if (sq_gettype(vm, -1) == SQObjectType::OT_CLOSURE)
			{
				sq_pushroottable(vm);
				sq_pushstring(vm, cvar, -1);
				sq_pushstring(vm, value, -1);
				sq_call(vm, 3, SQTrue, SQTrue);

				if (sq_gettype(vm, -1) == SQObjectType::OT_STRING)
					sq_getstring(vm, -1, &height);
			}

			sq_pop(vm, 2);
		}

		sq_poptop(vm);
	}

	sq_settop(vm, top);
	return height;
}

void sqb::_sqprintf(HSQUIRRELVM v, const SQChar * text, ...)
{
	
}

void sqb::_sqerrorf(HSQUIRRELVM v, const SQChar * text, ...)
{

}

void sqb::_sqerror(HSQUIRRELVM v, const SQChar * desc, const SQChar * source,
	SQInteger line, SQInteger column)
{

}
