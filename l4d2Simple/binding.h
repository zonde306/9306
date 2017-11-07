#pragma once
#include <squirrel.h>
#include <squirrel.h>
#include <sqstdblob.h>
#include <sqstdsystem.h>
#include <sqstdio.h>
#include <sqstdmath.h>
#include <sqstdstring.h>
#include <sqstdaux.h>
#include "./structs/usercmd.h"

// 配置文件

namespace sqb
{
	extern HSQUIRRELVM vm;
	extern void Initialization();
	extern void CheatStart();

	// DirectX 钩子
	extern bool DrawIndexedPrimitive(SQInteger stride, SQInteger vertices, SQInteger primitive);
	extern void EndScene(SQUserPointer drawPointer);
	extern SQInteger CreateQuery(SQInteger queryType);
	extern void Present(SQUserPointer drawPointer);

	// 游戏内钩子
	extern void PaintTraverse(const SQChar* panelName, SQUserPointer surfacePointer);
	extern bool CreateMove(CUserCmd* pCmd, SQBool bSendPacket);
	extern void FrameStageNotify(SQInteger stage, SQUserPointer debugPointer);
	extern void KeyInput(SQInteger keyCode, SQBool hasDown, const SQChar* binding);
	extern void Paint(SQInteger paintMode);
	extern const SQChar* ProcessGetCvarValue(const SQChar* cvar, const SQChar* value);
	extern const SQChar* ProcessSetConVar(const SQChar* cvar, const SQChar* value);
};
