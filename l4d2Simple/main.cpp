#include "main.h"

#define USE_PLAYER_INFO
#define USE_CVAR_CHANGE
// #define USE_D3D_DRAW

// D3D 的函数 jmp 挂钩
static std::unique_ptr<DetourXS> g_pDetourReset, g_pDetourPresent, g_pDetourEndScene,
	g_pDetourDrawIndexedPrimitive, g_pDetourCreateQuery, g_pDetourCL_Move, g_pDetourDebugger;

std::unique_ptr<CNetVars> g_pNetVars;
CInterfaces g_cInterfaces;

// D3D Device 虚表挂钩
static std::unique_ptr<CVMTHookManager> g_pVMTDevice;

// 当前 dll 文件实例
HINSTANCE g_hMyInstance;

// D3D EndScene 绘制工具
static std::unique_ptr<DrawManager> g_pDrawRender;

// D3D9 Device 钩子
static std::unique_ptr<D3D9Hooker> g_pDeviceHooker;

DWORD WINAPI StartCheat(LPVOID params);

BOOL WINAPI DllMain(HINSTANCE module, DWORD reason, LPVOID reserved)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		g_hMyInstance = module;

		// 获取所有接口
		g_cInterfaces.GetInterfaces();

		// 初始化 NetProp 表
		g_pNetVars = std::make_unique<CNetVars>();

		// se异常捕获器
		_set_se_translator([](unsigned int expCode, EXCEPTION_POINTERS* pExp) -> void
		{
			std::stringstream expInfo;
			switch (expCode)
			{
			case EXCEPTION_ACCESS_VIOLATION:
				expInfo << "[SE] 访问冲突：IP 0x" << std::setiosflags(std::ios::hex | std::ios::uppercase) <<
					pExp->ExceptionRecord->ExceptionAddress << " 类型：" <<
					(pExp->ExceptionRecord->ExceptionInformation[0] ? "写入" : "读取") << " 地址：0x" <<
					pExp->ExceptionRecord->ExceptionInformation[1];
				break;
			case EXCEPTION_INT_DIVIDE_BY_ZERO:
				expInfo << "[SE] int 除数不能为 0";
				break;
			case EXCEPTION_FLT_DIVIDE_BY_ZERO:
				expInfo << "[SE] float 除数不能为 0.0";
				break;
			case EXCEPTION_ILLEGAL_INSTRUCTION:
				expInfo << "[SE] 无效的指令";
				break;
			case EXCEPTION_PRIV_INSTRUCTION:
				expInfo << "[SE] 无法访问私有的指令";
				break;
			case EXCEPTION_STACK_OVERFLOW:
				expInfo << "[SE] 栈溢出";
				break;
			default:
				expInfo << "[SE] 未知异常";
			}

			Utils::log(expInfo.str().c_str());
			throw std::exception(expInfo.str().c_str());
		});

		// 异常捕获器带参数
		_set_invalid_parameter_handler([](const wchar_t* expression, const wchar_t* function,
			const wchar_t* file, unsigned int line, uintptr_t pReserved) -> void
		{
			std::wstringstream expInfo;
#ifndef _DEBUG
			expInfo << L"请使用 DEBUG 模式编译以进行调试";
#else
			expInfo << L"[IP] 普通异常：" << std::endl << L"\t信息：" << expression << std::endl <<
				L"\t文件：" << file << L" (" << line << L")" << std::endl << L"\t函数：" << function;
#endif
			Utils::log(expInfo.str().c_str());
			throw std::exception(Utils::w2c(expInfo.str()).c_str());
		});

		// 虚函数调用异常捕获
		_set_purecall_handler([]() -> void
		{
			Utils::log("未知虚函数调用异常");
			throw std::exception("未知虚函数调用异常");
		});

		CreateThread(NULL, NULL, StartCheat, module, NULL, NULL);
	}
	else if (reason == DLL_PROCESS_DETACH)
	{
		DETOURXS_DESTORY(g_pDetourReset);
		DETOURXS_DESTORY(g_pDetourPresent);
		DETOURXS_DESTORY(g_pDetourEndScene);
		DETOURXS_DESTORY(g_pDetourDrawIndexedPrimitive);
		DETOURXS_DESTORY(g_pDetourCreateQuery);
		DETOURXS_DESTORY(g_pDetourDebugger);
		DETOURXS_DESTORY(g_pDetourCL_Move);
		VMTHOOK_DESTORY(g_cInterfaces.ClientModeHook);
		VMTHOOK_DESTORY(g_cInterfaces.PanelHook);
		VMTHOOK_DESTORY(g_cInterfaces.ClientHook);
		VMTHOOK_DESTORY(g_cInterfaces.PredictionHook);
		VMTHOOK_DESTORY(g_cInterfaces.ModelRenderHook);
		VMTHOOK_DESTORY(g_cInterfaces.GameEventHook);
		VMTHOOK_DESTORY(g_pVMTDevice);
	}

	return TRUE;
}

// -------------------------------- D3D Hook Function --------------------------------
typedef HRESULT(WINAPI* FnDrawIndexedPrimitive)(IDirect3DDevice9*, D3DPRIMITIVETYPE, INT, UINT, UINT, UINT, UINT);
HRESULT WINAPI Hooked_DrawIndexedPrimitive(IDirect3DDevice9*, D3DPRIMITIVETYPE, INT, UINT, UINT, UINT, UINT);
static FnDrawIndexedPrimitive oDrawIndexedPrimitive;

typedef HRESULT(WINAPI* FnEndScene)(IDirect3DDevice9*);
HRESULT WINAPI Hooked_EndScene(IDirect3DDevice9*);
static FnEndScene oEndScene;

typedef HRESULT(WINAPI* FnCreateQuery)(IDirect3DDevice9*, D3DQUERYTYPE, IDirect3DQuery9**);
HRESULT WINAPI Hooked_CreateQuery(IDirect3DDevice9*, D3DQUERYTYPE, IDirect3DQuery9**);
static FnCreateQuery oCreateQuery;

typedef HRESULT(WINAPI* FnReset)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);
HRESULT WINAPI Hooked_Reset(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);
static FnReset oReset;

typedef HRESULT(WINAPI* FnPresent)(IDirect3DDevice9*, const RECT*, const RECT*, HWND, const RGNDATA*);
HRESULT WINAPI Hooked_Present(IDirect3DDevice9*, const RECT*, const RECT*, HWND, const RGNDATA*);
static FnPresent oPresent;

// -------------------------------- Virtual Function Hook --------------------------------
typedef void(__thiscall* FnPaintTraverse)(void*, unsigned int, bool, bool);
void __fastcall Hooked_PaintTraverse(void*, void*, unsigned int, bool, bool);
static FnPaintTraverse oPaintTraverse;

typedef bool(__stdcall* FnCreateMoveShared)(float, CUserCmd*);
bool __stdcall Hooked_CreateMoveShared(float, CUserCmd*);
static FnCreateMoveShared oCreateMoveShared;

typedef void(__stdcall* FnCreateMove)(int, float, bool);
void __stdcall Hooked_CreateMove(int, float, bool);
static FnCreateMove oCreateMove;

typedef void(__stdcall* FnFrameStageNotify)(ClientFrameStage_t);
void __stdcall Hooked_FrameStageNotify(ClientFrameStage_t);
static FnFrameStageNotify oFrameStageNotify;

typedef int(__stdcall* FnInKeyEvent)(int, ButtonCode_t, const char *);
int __stdcall Hooked_InKeyEvent(int, ButtonCode_t, const char *);
static FnInKeyEvent oInKeyEvent;

typedef void(__thiscall* FnRunCommand)(void*, CBaseEntity*, CUserCmd*, CMoveHelper*);
void __fastcall Hooked_RunCommand(void*, void*, CBaseEntity*, CUserCmd*, CMoveHelper*);
static FnRunCommand oRunCommand;

typedef void(__stdcall* FnDrawModel)(PVOID, PVOID, const ModelRenderInfo_t&, matrix3x4_t*);
void __stdcall Hooked_DrawModel(PVOID, PVOID, const ModelRenderInfo_t&, matrix3x4_t*);
static FnDrawModel oDrawModel;

typedef bool(__stdcall* FnDispatchUserMessage)(int, bf_read*);
bool __stdcall Hooked_DispatchUserMessage(int, bf_read*);
FnDispatchUserMessage oDispatchUserMessage;

typedef void(__cdecl* FnVGUIPaint)();
void __cdecl Hooked_VGUIPaint();
FnVGUIPaint oVGUIPaint;

typedef void(__stdcall* FnDrawModelExecute)(const DrawModelState_t&, const ModelRenderInfo_t&, matrix3x4_t*);
void _DrawModelExecute_Hooker();
void __stdcall Hooked_DrawModelExecute(int, int, const DrawModelState_t&, const ModelRenderInfo_t&, matrix3x4_t*);
FnDrawModelExecute oDrawModelExecute;

// -------------------------------- General Function --------------------------------
typedef void(__cdecl* FnConColorMsg)(class Color const&, char const*, ...);
static FnConColorMsg PrintToConsoleColor;	// 打印信息到控制台（支持颜色）

typedef void(__cdecl* FnConMsg)(char const*, ...);
static FnConMsg PrintToConsole;				// 打印信息到控制台

typedef void(__cdecl* FnCL_Move)(float, bool);
static void _CL_Move_Hooker();
void __stdcall Hooked_CL_Move(byte, int, double, float, bool);
FnCL_Move oCL_Move;							// 玩家数据处理

typedef void(__cdecl* FnSharedRandomFloat)(const char*, float, float, int);
FnSharedRandomFloat SharedRandomFloat;		// 随机数

typedef int(__stdcall* FnTraceLine)(void* ebp, void* esi, const Vector&, const Vector&, unsigned int mask,
	const IHandleEntity*, int, trace_t*);
FnTraceLine UTIL_TraceRay;					// 光线跟踪

typedef const char*(__cdecl* FnWeaponIdToAlias)(unsigned int);
FnWeaponIdToAlias WeaponIdToAlias;			// 获取武器的名字

typedef bool(__thiscall* FnUserMessagesDispatch)(void*, int, bf_read&);
FnUserMessagesDispatch DispatchUserMessage;	// 用户消息

typedef bool(__cdecl* FnIsInDebugSession)();
bool __cdecl Hooked_IsInDebugSession();
FnIsInDebugSession oPlat_IsInDebugSession;

// -------------------------------- Cheats Function --------------------------------
void thirdPerson();
void showSpectator();
void bindAlias(int);

// -------------------------------- Golbals Variable --------------------------------
static bool g_bNewFrame = false;										// 现在是否运行在新的一帧
static bool* g_pbSendPacket;											// 数据包是否发送到服务器
static int g_iSpeedMultiple = 5;										// 加速倍数
static CUserCmd* g_pUserCommands;										// 本地玩家当前按键
std::map<std::string, ConVar*> g_tConVar;								// 控制台变量
static float g_fAimbotFieldOfView = 30.0f;								// 自动瞄准角度
static CBaseEntity* g_pCurrentAiming;									// 当前的自动瞄准目标
static CBaseEntity* g_pGameRulesProxy;									// 游戏规则实体，在这里会有一些有用的东西
static DWORD g_iClientBase, g_iEngineBase, g_iMaterialModules;			// 有用的 DLL 文件地址
static bool g_bDrawBoxEsp = true, g_bTriggerBot = false, g_bAimBot = false, g_bAutoBunnyHop = true,
	g_bRapidFire = true, g_bSilentAimbot = false, g_bAutoStrafe = false, g_bDrawCrosshairs = true;

std::string GetZombieClassName(CBaseEntity* player);
bool IsValidEntity(CBaseEntity* entity);

DWORD WINAPI StartCheat(LPVOID params)
{
	g_iClientBase = Utils::GetModuleBase("client.dll");
	g_iEngineBase = Utils::GetModuleBase("engine.dll");
	g_iMaterialModules = Utils::GetModuleBase("materialsystem.dll");

	Utils::log("client.dll 0x%X", g_iClientBase);
	Utils::log("engine.dll 0x%X", g_iEngineBase);
	Utils::log("materialsystem.dll 0x%X", g_iMaterialModules);
	Utils::log("VEngineClient 0x%X", (DWORD)g_cInterfaces.Engine);
	Utils::log("EngineTraceClient 0x%X", (DWORD)g_cInterfaces.Trace);
	Utils::log("VClient 0x%X", (DWORD)g_cInterfaces.Client);
	Utils::log("VClientEntityList 0x%X", (DWORD)g_cInterfaces.ClientEntList);
	Utils::log("VModelInfoClient 0x%X", (DWORD)g_cInterfaces.ModelInfo);
	Utils::log("VGUI_Panel 0x%X", (DWORD)g_cInterfaces.Panel);
	Utils::log("VGUI_Surface 0x%X", (DWORD)g_cInterfaces.Surface);
	Utils::log("PlayerInfoManager 0x%X", (DWORD)g_cInterfaces.PlayerInfo);
	Utils::log("VClientPrediction 0x%X", (DWORD)g_cInterfaces.Prediction);
	Utils::log("GameMovement 0x%X", (DWORD)g_cInterfaces.GameMovement);
	Utils::log("VDebugOverlay 0x%X", (DWORD)g_cInterfaces.DebugOverlay);
	Utils::log("GameEventsManager 0x%X", (DWORD)g_cInterfaces.GameEvent);
	Utils::log("VEngineModel 0x%X", (DWORD)g_cInterfaces.ModelRender);
	Utils::log("VEngineRenderView 0x%X", (DWORD)g_cInterfaces.RenderView);
	Utils::log("VEngineCvar 0x%X", (DWORD)g_cInterfaces.Engine);
	Utils::log("GlobalsVariable 0x%X", (DWORD)g_cInterfaces.Globals);
	Utils::log("InputSystem 0x%X", (DWORD)g_cInterfaces.InputSystem);
	Utils::log("MaterialSystem 0x%X", (DWORD)g_cInterfaces.MaterialSystem);
	Utils::log("Input 0x%X", (DWORD)g_cInterfaces.Input);
	Utils::log("UserMessages 0x%X", (DWORD)g_cInterfaces.UserMessage);
	Utils::log("MoveHelper 0x%X", (DWORD)g_cInterfaces.MoveHelper);
	
	// 这个好像是不正确的...
	g_cInterfaces.MoveHelper = nullptr;

	if ((oCL_Move = (FnCL_Move)Utils::FindPattern("engine.dll",
		XorStr("55 8B EC B8 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 33 C5 89 45 FC 53 56 57 E8"))) != nullptr)
	{
		g_pbSendPacket = (bool*)((DWORD)oCL_Move + 0x91);
		Utils::log("CL_Move = engine.dll + 0x%X | bSendPacket = 0x%X",
			(DWORD)oCL_Move - g_iEngineBase, (DWORD)g_pbSendPacket);

		/*
		g_pDetourCL_Move = std::make_unique<DetourXS>(oCL_Move, _CL_Move_Hooker);
		oCL_Move = (FnCL_Move)g_pDetourCL_Move->GetTrampoline();
		Utils::log("Trampoline oCL_Move = 0x%X", (DWORD)oCL_Move);
		*/
	}
	else
		Utils::log("CL_Move not found");

	if ((SharedRandomFloat = (FnSharedRandomFloat)Utils::FindPattern("client.dll",
		XorStr("55 8B EC 83 EC 08 A1 ? ? ? ? 53 56 57 8B 7D 14 8D 4D 14 51 89 7D F8 89 45 FC E8 ? ? ? ? 6A 04 8D 55 FC 52 8D 45 14 50 E8 ? ? ? ? 6A 04 8D 4D F8 51 8D 55 14 52 E8 ? ? ? ? 8B 75 08 56 E8 ? ? ? ? 50 8D 45 14 56 50 E8 ? ? ? ? 8D 4D 14 51 E8 ? ? ? ? 8B 15 ? ? ? ? 8B 5D 14 83 C4 30 83 7A 30 00 74 26 57 53 56 68 ? ? ? ? 68 ? ? ? ? 8D 45 14 68 ? ? ? ? 50 C7 45 ? ? ? ? ? FF 15 ? ? ? ? 83 C4 1C 53 B9 ? ? ? ? FF 15 ? ? ? ? D9 45 10"))) != nullptr)
		Utils::log("SharedRandomFloat = client.dll + 0x%X", (DWORD)SharedRandomFloat - g_iClientBase);
	else
		Utils::log("SharedRandomFloat not found");

	if ((UTIL_TraceRay = (FnTraceLine)Utils::FindPattern("client.dll",
		XorStr("53 8B DC 83 EC 08 83 E4 F0 83 C4 04 55 8B 6B 04 89 6C 24 04 8B EC 83 EC 6C 56 8B 43 08"))) != nullptr)
		Utils::log("UTIL_TraceRay = client.dll + 0x%X", (DWORD)UTIL_TraceRay - g_iClientBase);
	else
		Utils::log("UTIL_TraceRay not found");

	if ((WeaponIdToAlias = (FnWeaponIdToAlias)Utils::FindPattern("client.dll",
		XorStr("55 8B EC 8B 45 08 83 F8 37"))) != nullptr)
		Utils::log("WeaponIdToAlias = client.dll + 0x%X", (DWORD)WeaponIdToAlias - g_iClientBase);
	else
		Utils::log("WeaponIdToAlias not found");

	if ((DispatchUserMessage = (FnUserMessagesDispatch)Utils::FindPattern("client.dll",
		XorStr("55 8B EC 8B 45 08 83 EC 28 85 C0"))) != nullptr)
		Utils::log("DispatchUserMessage = client.dll + 0x%X", (DWORD)DispatchUserMessage - g_iClientBase);
	else
		Utils::log("DispatchUserMessage not found");

	typedef ClientModeShared*(*FnGetClientMode)();
	FnGetClientMode GetClientModeNormal = nullptr;
	if ((GetClientModeNormal = (FnGetClientMode)Utils::FindPattern("client.dll",
		XorStr("8B 0D ? ? ? ? 8B 01 8B 90 ? ? ? ? FF D2 8B 04 85 ? ? ? ? C3"))) != nullptr &&
		(g_cInterfaces.ClientMode = GetClientModeNormal()) != nullptr)
	{
		g_cInterfaces.ClientModeHook = std::make_unique<CVMTHookManager>(g_cInterfaces.ClientMode);
		// printo("ClientModePtr", g_cInterfaces.ClientMode);
		Utils::log("GetClientMode = client.dll + 0x%X", (DWORD)GetClientModeNormal - g_iClientBase);
		Utils::log("g_pClientMode = 0x%X", (DWORD)g_cInterfaces.ClientMode);
		Utils::log("m_pChatElement = 0x%X", (DWORD)g_cInterfaces.ClientMode->GetHudChat());
	}
	else
		Utils::log("ClientModeShared not found");

	if (g_cInterfaces.PanelHook && indexes::PaintTraverse > -1)
	{
		oPaintTraverse = (FnPaintTraverse)g_cInterfaces.PanelHook->HookFunction(indexes::PaintTraverse, Hooked_PaintTraverse);
		g_cInterfaces.PanelHook->HookTable(true);
		Utils::log("oPaintTraverse = 0x%X", (DWORD)oPaintTraverse);
	}

	if (g_cInterfaces.ClientModeHook && indexes::SharedCreateMove > -1)
	{
		oCreateMoveShared = (FnCreateMoveShared)g_cInterfaces.ClientModeHook->HookFunction(indexes::SharedCreateMove, Hooked_CreateMoveShared);
		// g_cInterfaces.ClientModeHook->HookTable(true);
		Utils::log("oCreateMoveShared = 0x%X", (DWORD)oCreateMoveShared);
	}

	if (g_cInterfaces.ClientHook && indexes::CreateMove > -1)
	{
		oCreateMove = (FnCreateMove)g_cInterfaces.ClientHook->HookFunction(indexes::CreateMove, Hooked_CreateMove);
		g_cInterfaces.ClientHook->HookTable(true);
		Utils::log("oCreateMove = 0x%X", (DWORD)oCreateMove);
	}

	if (g_cInterfaces.ClientHook && indexes::FrameStageNotify > -1)
	{
		oFrameStageNotify = (FnFrameStageNotify)g_cInterfaces.ClientHook->HookFunction(indexes::FrameStageNotify, Hooked_FrameStageNotify);
		g_cInterfaces.ClientHook->HookTable(true);
		Utils::log("oFrameStageNotify = 0x%X", (DWORD)oFrameStageNotify);
	}

	if (g_cInterfaces.ClientHook && indexes::InKeyEvent > -1)
	{
		oInKeyEvent = (FnInKeyEvent)g_cInterfaces.ClientHook->HookFunction(indexes::InKeyEvent, Hooked_InKeyEvent);
		g_cInterfaces.ClientHook->HookTable(true);
		Utils::log("oInKeyEvent = 0x%X", (DWORD)oInKeyEvent);
	}

	if (g_cInterfaces.ClientHook && indexes::DispatchUserMessage > -1)
	{
		oDispatchUserMessage = (FnDispatchUserMessage)g_cInterfaces.ClientHook->HookFunction(indexes::DispatchUserMessage, Hooked_DispatchUserMessage);
		g_cInterfaces.ClientHook->HookTable(true);
		Utils::log("oDispatchUserMessage = 0x%X", (DWORD)oDispatchUserMessage);
	}

	if (g_cInterfaces.PredictionHook && indexes::RunCommand > -1)
	{
		oRunCommand = (FnRunCommand)g_cInterfaces.PredictionHook->HookFunction(indexes::RunCommand, Hooked_RunCommand);
		g_cInterfaces.PredictionHook->HookTable(true);
		Utils::log("oRunCommand = 0x%X", (DWORD)oRunCommand);
	}

	/*
	if (g_cInterfaces.ModelRenderHook && indexes::DrawModel > -1)
	{
		oDrawModel = (FnDrawModel)g_cInterfaces.ModelRenderHook->HookFunction(indexes::DrawModel, Hooked_DrawModel);
		g_cInterfaces.ModelRenderHook->HookTable(true);
		Utils::log("oDrawModel = 0x%X", (DWORD)oDrawModel);
	}
	*/

	/*
	if (g_cInterfaces.ViewRenderHook && indexes::VGui_Paint > -1)
	{
		oVGUIPaint = (FnVGUIPaint)g_cInterfaces.ViewRenderHook->HookFunction(indexes::VGui_Paint, &Hooked_VGUIPaint);
		g_cInterfaces.ViewRenderHook->HookTable(true);
		Utils::log("oVgui_Paint = 0x%X", (DWORD)oVGUIPaint);
	}
	*/

	HMODULE tier0 = Utils::GetModuleHandleSafe("tier0.dll");
	if (tier0 != NULL)
	{
		PrintToConsole = (FnConMsg)GetProcAddress(tier0, "?ConMsg@@YAXPBDZZ");
		PrintToConsoleColor = (FnConColorMsg)GetProcAddress(tier0, "?ConColorMsg@@YAXABVColor@@PBDZZ");
		oPlat_IsInDebugSession = (FnIsInDebugSession)GetProcAddress(tier0, "Plat_IsInDebugSession");
		Utils::log("PrintToConsole = 0x%X", (DWORD)PrintToConsole);
		Utils::log("PrintToConsoleColor = 0x%X", (DWORD)PrintToConsoleColor);
		Utils::log("Plat_IsInDebugSession = 0x%X", (DWORD)oPlat_IsInDebugSession);

		if (oPlat_IsInDebugSession)
		{
			// 屏蔽调试器检查，因为检查到调试器就会自动关闭游戏的
			g_pDetourDebugger = std::make_unique<DetourXS>(oPlat_IsInDebugSession, Hooked_IsInDebugSession);
		}
	}

	if (g_cInterfaces.Cvar)
	{
#ifdef USE_CVAR_CHANGE
		g_tConVar["sv_cheats"] = g_cInterfaces.Cvar->FindVar("sv_cheats");
		g_tConVar["r_drawothermodels"] = g_cInterfaces.Cvar->FindVar("r_drawothermodels");
		g_tConVar["cl_drawshadowtexture"] = g_cInterfaces.Cvar->FindVar("cl_drawshadowtexture");
		g_tConVar["mat_fullbright"] = g_cInterfaces.Cvar->FindVar("mat_fullbright");
		g_tConVar["sv_pure"] = g_cInterfaces.Cvar->FindVar("sv_pure");
		g_tConVar["sv_consistency"] = g_cInterfaces.Cvar->FindVar("sv_consistency");
		g_tConVar["mp_gamemode"] = g_cInterfaces.Cvar->FindVar("mp_gamemode");
		g_tConVar["c_thirdpersonshoulder"] = g_cInterfaces.Cvar->FindVar("c_thirdpersonshoulder");

		Utils::log("sv_cheats = 0x%X", (DWORD)g_tConVar["sv_cheats"]);
		Utils::log("r_drawothermodels = 0x%X", (DWORD)g_tConVar["r_drawothermodels"]);
		Utils::log("cl_drawshadowtexture = 0x%X", (DWORD)g_tConVar["cl_drawshadowtexture"]);
		Utils::log("mat_fullbright = 0x%X", (DWORD)g_tConVar["mat_fullbright"]);
		Utils::log("sv_pure = 0x%X", (DWORD)g_tConVar["sv_pure"]);
		Utils::log("sv_consistency = 0x%X", (DWORD)g_tConVar["sv_consistency"]);
		Utils::log("mp_gamemode = 0x%X", (DWORD)g_tConVar["mp_gamemode"]);
		Utils::log("c_thirdpersonshoulder = 0x%X", (DWORD)g_tConVar["c_thirdpersonshoulder"]);
#else
		g_tConVar["sv_cheats"] = nullptr;
		g_tConVar["r_drawothermodels"] = nullptr;
		g_tConVar["cl_drawshadowtexture"] = nullptr;
		g_tConVar["mat_fullbright"] = nullptr;
		g_tConVar["sv_pure"] = nullptr;
		g_tConVar["sv_consistency"] = nullptr;
		g_tConVar["mp_gamemode"] = nullptr;
		g_tConVar["c_thirdpersonshoulder"] = nullptr;
#endif
	}

	g_pDeviceHooker = std::make_unique<D3D9Hooker>();
	g_pDeviceHooker->StartDeviceHook([&](IDirect3D9* pD3D, IDirect3DDevice9* pDeivce, DWORD* pVMT) -> void
	{
		// 虚函数表修改跳转
		g_pDetourReset = std::make_unique<DetourXS>((void*)pVMT[16], Hooked_Reset);
		g_pDetourPresent = std::make_unique<DetourXS>((void*)pVMT[17], Hooked_Present);
		g_pDetourEndScene = std::make_unique<DetourXS>((void*)pVMT[42], Hooked_EndScene);
		g_pDetourDrawIndexedPrimitive = std::make_unique<DetourXS>((void*)pVMT[82], Hooked_DrawIndexedPrimitive);
		g_pDetourCreateQuery = std::make_unique<DetourXS>((void*)pVMT[118], Hooked_CreateQuery);

		// 获取原函数
		oReset = (FnReset)g_pDetourReset->GetTrampoline();
		oPresent = (FnPresent)g_pDetourPresent->GetTrampoline();
		oEndScene = (FnEndScene)g_pDetourEndScene->GetTrampoline();
		oDrawIndexedPrimitive = (FnDrawIndexedPrimitive)g_pDetourDrawIndexedPrimitive->GetTrampoline();
		oCreateQuery = (FnCreateQuery)g_pDetourCreateQuery->GetTrampoline();

		Utils::log("Trampoline oReset = 0x%X", (DWORD)oReset);
		Utils::log("Trampoline oPresent = 0x%X", (DWORD)oPresent);
		Utils::log("Trampoline oEndScene = 0x%X", (DWORD)oEndScene);
		Utils::log("Trampoline oDrawIndexedPrimitive = 0x%X", (DWORD)oDrawIndexedPrimitive);
		Utils::log("Trampoline oCreateQuery = 0x%X", (DWORD)oCreateQuery);
	});

	// 只是为了保险起见而已
	g_pDeviceHooker->GetDevice() = nullptr;

	// 按键绑定
	bindAlias(45);

	// 事件监听器
	if (g_cInterfaces.GameEvent)
	{
		class EventListener : public IGameEventListener2
		{
		public:
			virtual void FireGameEvent(IGameEvent *event)
			{
				const char* eventName = event->GetName();
				if (_strcmpi(eventName, "player_death") == 0)
				{
					// 受害者（死者）
					int victim = g_cInterfaces.Engine->GetPlayerForUserID(event->GetInt("userid"));
					if (victim <= 0)
						victim = event->GetInt("entityid");

					// 攻击者（击杀者）
					int attacker = g_cInterfaces.Engine->GetPlayerForUserID(event->GetInt("attacker"));
					if (attacker <= 0)
						attacker = event->GetInt("attackerentid");

					if (victim <= 0)
						return;

					CBaseEntity* entity = g_cInterfaces.ClientEntList->GetClientEntity(victim);
					if (!IsValidEntity(entity))
						return;

					if ((DWORD)entity == (DWORD)g_pCurrentAiming)
						g_pCurrentAiming = nullptr;

					/*
					Utils::log("player %d death, killed by %d%s", victim, attacker,
					(event->GetBool("headshot") ? " | headshot" : ""));
					*/

					if (g_pDrawRender)
					{
						g_pDrawRender->PushRenderText(DrawManager::BLUE, "player %s killed by %d%s",
							GetZombieClassName(entity).c_str(), attacker,
							(event->GetBool("headshot") ? " + headshot" : ""));
					}
				}
				else if (_strcmpi(eventName, "infected_death") == 0)
				{
					int victim = event->GetInt("infected_id");
					if (victim <= 0)
						return;

					int attacker = g_cInterfaces.Engine->GetPlayerForUserID(event->GetInt("attacker"));

					CBaseEntity* entity = g_cInterfaces.ClientEntList->GetClientEntity(victim);
					if (!IsValidEntity(entity))
						return;

					if ((DWORD)entity == (DWORD)g_pCurrentAiming)
						g_pCurrentAiming = nullptr;

					/*
					Utils::log("infected %d death, killed by %d%s", victim, attacker,
					(event->GetBool("headshot") ? " | headshot" : ""));
					*/

					if (g_pDrawRender)
					{
						g_pDrawRender->PushRenderText(DrawManager::BLUE, "infected %s killed by %d%s",
							GetZombieClassName(entity).c_str(), attacker,
							(event->GetBool("headshot") ? " + headshot" : ""));
					}
				}
				else if (_strcmpi(eventName, "player_spawn") == 0)
				{
					int client = g_cInterfaces.Engine->GetPlayerForUserID(event->GetInt("userid"));
					if (client <= 0)
						return;

					CBaseEntity* entity = g_cInterfaces.ClientEntList->GetClientEntity(client);
					if (!IsValidEntity(entity))
						return;

					if (g_pDrawRender)
					{
						g_pDrawRender->PushRenderText(DrawManager::BLUE, "player %s spawned",
							GetZombieClassName(entity).c_str());
					}
				}
				else if (_strcmpi(eventName, "player_connect") == 0)
				{
					if (event->GetInt("bot"))
						return;

					const char* name = event->GetString("name");
					const char* steamId = event->GetString("networkid");
					const char* ip = event->GetString("address");

					/*
					if (g_pDrawRender)
					{
						g_pDrawRender->PushRenderText(DrawManager::ORANGE,
							"client %s (%s) connect... ip %s", name, steamId, ip);
					}
					*/
				}
				else if (_strcmpi(eventName, "player_disconnect") == 0)
				{
					if (event->GetInt("bot"))
						return;

					const char* name = event->GetString("name");
					const char* steamId = event->GetString("networkid");
					const char* reason = event->GetString("reason");

					/*
					if (g_pDrawRender)
					{
						g_pDrawRender->PushRenderText(DrawManager::ORANGE,
							"client %s (%s) disconnect... reason %s", name, steamId, reason);
					}
					*/
				}
				else if (_strcmpi(eventName, "player_say") == 0 || _strcmpi(eventName, "player_chat") == 0)
				{
					int client = g_cInterfaces.Engine->GetPlayerForUserID(event->GetInt("userid"));
					const char* text = event->GetString("text");

					bool teamOnly = (eventName[7] == 'c' ? event->GetBool("teamonly") : false);

					if (client <= 0 || text == nullptr || text[0] == '\0' || !teamOnly)
						return;

					CBaseEntity* entity = g_cInterfaces.ClientEntList->GetClientEntity(client);
					if (!IsValidEntity(entity))
						return;

					if (g_pDrawRender)
					{
						int team = entity->GetTeam();
						if (team == 2)
						{
							g_pDrawRender->PushRenderText(DrawManager::SKYBLUE,
								"%s: %s", GetZombieClassName(entity).c_str(), text);
						}
						else if (team == 3)
						{
							g_pDrawRender->PushRenderText(DrawManager::RED,
								"%s: %s", GetZombieClassName(entity).c_str(), text);
						}
						else
						{
							g_pDrawRender->PushRenderText(DrawManager::WHITE,
								"%s: %s", GetZombieClassName(entity).c_str(), text);
						}
					}
				}
				else if (_strcmpi(eventName, "player_team") == 0)
				{
					if (event->GetBool("disconnect"))
						return;

					int client = g_cInterfaces.Engine->GetPlayerForUserID(event->GetInt("userid"));
					int oldTeam = event->GetInt("oldteam");
					int newTeam = event->GetInt("team");
					if (client <= 0 || newTeam == 0)
						return;

					CBaseEntity* entity = g_cInterfaces.ClientEntList->GetClientEntity(client);
					if (!IsValidEntity(entity))
						return;

					/*
					if (g_pDrawRender)
					{
						g_pDrawRender->PushRenderText(DrawManager::ORANGE,
							"player %s join %d from %d", GetZombieClassName(entity).c_str(),
							newTeam, oldTeam);
					}
					*/
				}
				else if (_strcmpi(eventName, "door_unlocked") == 0)
				{
					int client = g_cInterfaces.Engine->GetPlayerForUserID(event->GetInt("userid"));
					bool checkpoint = event->GetBool("checkpoint");
					if (client <= 0 || !checkpoint)
						return;

					g_pDrawRender->PushRenderText(DrawManager::ORANGE,
						"saferoom unlock by %d", client);
				}
				else if (_strcmpi(eventName, "vote_started") == 0)
				{
					const char* issue = event->GetString("issue");
					const char* param = event->GetString("param1");
					const char* data = event->GetString("votedata");
					int team = event->GetInt("team");
					int voter = event->GetInt("initiator");

					if (team <= 1 || voter <= 0)
						return;

					CBaseEntity* entity = g_cInterfaces.ClientEntList->GetClientEntity(voter);
					if (!IsValidEntity(entity))
						return;

					if (g_pDrawRender)
					{
						g_pDrawRender->PushRenderText(DrawManager::PINK,
							"%s start team %d vote, issue: %s, param: %s, data: %s",
							GetZombieClassName(entity).c_str(), team, issue, param, data);
					}
				}
				else if (_strcmpi(eventName, "vote_passed") == 0)
				{
					const char* param = event->GetString("param1");
					const char* details = event->GetString("details");
					int team = event->GetInt("team");

					if (g_pDrawRender)
					{
						g_pDrawRender->PushRenderText(DrawManager::PINK,
							"team %d vote passed, param: %s, details: %s",
							team, param, details);
					}
				}
				else if (_strcmpi(eventName, "vote_cast_yes") == 0)
				{
					int client = event->GetInt("entityid");
					int team = event->GetInt("team");

					if (client <= 0 || team <= 1)
						return;

					CBaseEntity* entity = g_cInterfaces.ClientEntList->GetClientEntity(client);
					if (!IsValidEntity(entity))
						return;

					if (g_pDrawRender)
					{
						g_pDrawRender->PushRenderText(DrawManager::PINK,
							"team %d player %s vote yes", team, GetZombieClassName(entity).c_str());
					}
				}
				else if (_strcmpi(eventName, "vote_cast_no") == 0)
				{
					int client = event->GetInt("entityid");
					int team = event->GetInt("team");

					if (client <= 0 || team <= 1)
						return;

					CBaseEntity* entity = g_cInterfaces.ClientEntList->GetClientEntity(client);
					if (!IsValidEntity(entity))
						return;

					if (g_pDrawRender)
					{
						g_pDrawRender->PushRenderText(DrawManager::PINK,
							"team %d player %s vote no", team, GetZombieClassName(entity).c_str());
					}
				}
				else if (_strcmpi(eventName, "tank_spawn") == 0)
				{
					int client = g_cInterfaces.Engine->GetPlayerForUserID(event->GetInt("userid"));
					if (client <= 0)
						return;

					if (g_pDrawRender)
					{
						g_pDrawRender->PushRenderText(DrawManager::RED, "tank spawn!!! %d", client);
					}
				}
				else if (_strcmpi(eventName, "friendly_fire") == 0)
				{
					// 受害者
					int victim = g_cInterfaces.Engine->GetPlayerForUserID(event->GetInt("userid"));

					// 攻击者
					int attacker = g_cInterfaces.Engine->GetPlayerForUserID(event->GetInt("attacker"));

					// 是谁的错
					int guilty = g_cInterfaces.Engine->GetPlayerForUserID(event->GetInt("guilty"));

					// 伤害类型
					// int damageType = event->GetInt("type");

					if (victim <= 0 || attacker <= 0)
						return;

					CBaseEntity* entityAttacker = g_cInterfaces.ClientEntList->GetClientEntity(attacker);
					CBaseEntity* entityVictim = g_cInterfaces.ClientEntList->GetClientEntity(victim);
					if (!IsValidEntity(entityAttacker) || !IsValidEntity(entityVictim))
						return;

					if (g_pDrawRender)
					{
						int team = entityAttacker->GetTeam();
						if (team == 2)
						{
							g_pDrawRender->PushRenderText(DrawManager::SKYBLUE,
								"%s attack firends %s guilty is %s",
								GetZombieClassName(entityAttacker).c_str(),
								GetZombieClassName(entityVictim).c_str(),
								(guilty == victim ? "victim" : "attacker"));
						}
						else if (team == 3)
						{
							g_pDrawRender->PushRenderText(DrawManager::RED,
								"%s attack firends %s guilty is %s",
								GetZombieClassName(entityAttacker).c_str(),
								GetZombieClassName(entityVictim).c_str(),
								(guilty == victim ? "victim" : "attacker"));
						}
						else
						{
							g_pDrawRender->PushRenderText(DrawManager::WHITE,
								"%s attack firends %s guilty is %s",
								GetZombieClassName(entityAttacker).c_str(),
								GetZombieClassName(entityVictim).c_str(),
								(guilty == victim ? "victim" : "attacker"));
						}
					}
				}
				else if (_strcmpi(eventName, "tank_killed") == 0)
				{
					// 受害者
					int victim = g_cInterfaces.Engine->GetPlayerForUserID(event->GetInt("userid"));

					// 攻击者
					int attacker = g_cInterfaces.Engine->GetPlayerForUserID(event->GetInt("attacker"));

					if (victim <= 0)
						return;

					if (g_pDrawRender)
					{
						g_pDrawRender->PushRenderText(DrawManager::GREEN,
							"tank %d killed by %d", victim, attacker);
					}
				}
				else if (_strcmpi(eventName, "map_transition") == 0)
				{
					Utils::log("*** map change ***");
					g_pCurrentAiming = nullptr;
				}
				else if (_strcmpi(eventName, "mission_lost") == 0)
				{
					Utils::log("*** round lost ***");
					g_pCurrentAiming = nullptr;
				}
			}
		};

		// 注册事件监听器
		EventListener* listener = new EventListener();
		g_cInterfaces.GameEvent->AddListener(listener, "player_spawn", false);
		g_cInterfaces.GameEvent->AddListener(listener, "player_death", false);
		g_cInterfaces.GameEvent->AddListener(listener, "infected_death", false);
		g_cInterfaces.GameEvent->AddListener(listener, "map_transition", false);
		g_cInterfaces.GameEvent->AddListener(listener, "mission_lost", false);

		/*
		g_cInterfaces.GameEvent->AddListener(listener, "player_connect", false);
		g_cInterfaces.GameEvent->AddListener(listener, "player_disconnect", false);
		g_cInterfaces.GameEvent->AddListener(listener, "player_chat", false);
		g_cInterfaces.GameEvent->AddListener(listener, "player_team", false);
		g_cInterfaces.GameEvent->AddListener(listener, "door_unlocked", false);
		g_cInterfaces.GameEvent->AddListener(listener, "vote_started", false);
		g_cInterfaces.GameEvent->AddListener(listener, "vote_passed", false);
		g_cInterfaces.GameEvent->AddListener(listener, "vote_cast_yes", false);
		g_cInterfaces.GameEvent->AddListener(listener, "vote_cast_no", false);
		g_cInterfaces.GameEvent->AddListener(listener, "tank_spawn", false);
		g_cInterfaces.GameEvent->AddListener(listener, "friendly_fire", false);
		g_cInterfaces.GameEvent->AddListener(listener, "tank_killed", false);
		*/
	}

	return 0;
}

void ResetDeviceHook(IDirect3DDevice9* device)
{
	// 储存游戏使用的 IDirect3DDevice9 对象
	g_pDeviceHooker->GetDevice() = device;

	// 将修改的 Jump 还原
	DETOURXS_DESTORY(g_pDetourReset);
	DETOURXS_DESTORY(g_pDetourPresent);
	DETOURXS_DESTORY(g_pDetourEndScene);
	DETOURXS_DESTORY(g_pDetourDrawIndexedPrimitive);
	DETOURXS_DESTORY(g_pDetourCreateQuery);

	// 使用 VMT 来 Hook
	g_pVMTDevice = std::make_unique<CVMTHookManager>(device);
	oReset = g_pVMTDevice->SetupHook(16, Hooked_Reset);
	oPresent = g_pVMTDevice->SetupHook(17, Hooked_Present);
	oEndScene = g_pVMTDevice->SetupHook(42, Hooked_EndScene);
	oDrawIndexedPrimitive = g_pVMTDevice->SetupHook(82, Hooked_DrawIndexedPrimitive);
	oCreateQuery = g_pVMTDevice->SetupHook(118, Hooked_CreateQuery);
	g_pVMTDevice->HookTable(true);

	// 初始化绘图
	g_pDrawRender = std::make_unique<DrawManager>(device);

	Utils::log("pD3DDevice = 0x%X", (DWORD)device);
	Utils::log("oReset = 0x%X", (DWORD)oReset);
	Utils::log("oPresent = 0x%X", (DWORD)oPresent);
	Utils::log("oEndScene = 0x%X", (DWORD)oEndScene);
	Utils::log("oDrawIndexedPrimitive = 0x%X", (DWORD)oDrawIndexedPrimitive);
	Utils::log("oCreateQuery = 0x%X", (DWORD)oCreateQuery);
}

void thirdPerson()
{
	CBaseEntity* local = GetLocalClient();
	if (local && local->IsAlive())
	{
		if (local->GetNetProp<float>("m_TimeForceExternalView", "DT_TerrorPlayer") > 0.0f)
			local->SetNetProp<float>("m_TimeForceExternalView", 99999.3f, "DT_TerrorPlayer");
		else
			local->SetNetProp<float>("m_TimeForceExternalView", 0.0f, "DT_TerrorPlayer");

		/*
		if (local->GetNetProp<int>("m_hObserverTarget", "DT_BasePlayer") == -1)
		{
		// 第三人称
		local->SetNetProp<int>("m_hObserverTarget", 0, "DT_BasePlayer");
		local->SetNetProp<int>("m_iObserverMode", 1, "DT_BasePlayer");
		local->SetNetProp<int>("m_bDrawViewmodel", 0, "DT_BasePlayer");
		}
		else if (local->GetNetProp<int>("m_hObserverTarget", "DT_BasePlayer") == 0)
		{
		// 第一人称
		local->SetNetProp<int>("m_hObserverTarget", -1, "DT_BasePlayer");
		local->SetNetProp<int>("m_iObserverMode", 0, "DT_BasePlayer");
		local->SetNetProp<int>("m_bDrawViewmodel", 1, "DT_BasePlayer");
		}
		*/
	}

	std::this_thread::sleep_for(std::chrono::seconds(1));
}

void showSpectator()
{
	CBaseEntity* local = GetLocalClient();
	if (local && local->IsAlive())
	{
#ifdef USE_PLAYER_INFO
		player_info_t info;
#endif

		CBaseEntity* player = nullptr, *target = nullptr;

		g_cInterfaces.Engine->ClientCmd("echo \"========= player list =========\"");

		for (int i = 1; i < 64; ++i)
		{
			player = g_cInterfaces.ClientEntList->GetClientEntity(i);
			if (player == nullptr || player->IsDormant())
				continue;

			int team = player->GetTeam();
			int mode = player->GetNetProp<int>("m_hObserverTarget", "DT_BasePlayer");

#ifdef USE_PLAYER_INFO
			g_cInterfaces.Engine->GetPlayerInfo(i, &info);
#endif

			if (team == 1)
			{
				// 瑙傚療鑰
				if (mode != 4 && mode != 5)
					continue;

				target = (CBaseEntity*)player->GetNetProp<CBaseHandle*>("m_iObserverMode", "DT_BasePlayer");
				if (target && (int)target != -1)
					target = g_cInterfaces.ClientEntList->GetClientEntityFromHandle((CBaseHandle*)target);

				if (target == nullptr || (int)target == -1)
					continue;

				if ((DWORD)target == (DWORD)local)
				{
					// 姝ｅ湪瑙傚療鏈湴鐜╁
#ifdef USE_PLAYER_INFO
					g_cInterfaces.Engine->ClientCmd("echo \"[spectator] player %s %s you\"",
						info.name, (mode == 4 ? "watching" : "following"));
#else
					g_cInterfaces.Engine->ClientCmd("echo \"[spectator] player %d %s you\"",
						player->GetIndex(), (mode == 4 ? "watching" : "following"));
#endif
				}
				else
				{
					// 姝ｅ湪瑙傚療鍏朵粬鐜╁
#ifdef USE_PLAYER_INFO
					player_info_t other;
					g_cInterfaces.Engine->GetPlayerInfo(target->GetIndex(), &other);
					g_cInterfaces.Engine->ClientCmd("echo \"[spectator] player %s %s %s\"",
						info.name, (mode == 4 ? "watching" : "following"), other.name);
#else
					g_cInterfaces.Engine->ClientCmd("echo \"[spectator] player %d %s %d\"",
						player->GetIndex(), (mode == 4 ? "watching" : "following"), target->GetIndex());
#endif
				}
			}
			else if (team == 2)
			{
				// 鐢熻繕鑰
				if (player->IsAlive())
				{
					// 娲荤潃
#ifdef USE_PLAYER_INFO
					g_cInterfaces.Engine->ClientCmd("echo \"[survivor] player %s is alive (%d + %.0f)\"",
						info.name, player->GetHealth(), player->GetNetProp<float>("m_healthBuffer", "DT_TerrorPlayer"));
#else
					g_cInterfaces.Engine->ClientCmd("echo \"[survivor] player %d is alive (%d + %.0f)\"",
						player->GetIndex(), player->GetHealth(), player->GetNetProp<float>("m_healthBuffer", "DT_TerrorPlayer"));
#endif
				}
				else
				{
					// 姝讳骸
					if (mode != 4 && mode != 5)
						continue;

					target = (CBaseEntity*)player->GetNetProp<CBaseHandle*>("m_iObserverMode", "DT_BasePlayer");
					if (target && (int)target != -1)
						target = g_cInterfaces.ClientEntList->GetClientEntityFromHandle((CBaseHandle*)target);

					if (target == nullptr || (int)target == -1)
						continue;

					if ((DWORD)target == (DWORD)local)
					{
						// 姝ｅ湪瑙傚療鏈湴鐜╁
#ifdef USE_PLAYER_INFO
						g_cInterfaces.Engine->ClientCmd("echo \"[survivor] player %s %s you\"",
							info.name, (mode == 4 ? "watching" : "following"));
#else
						g_cInterfaces.Engine->ClientCmd("echo \"[survivor] player %d %s you\"",
							player->GetIndex(), (mode == 4 ? "watching" : "following"));
#endif
					}
					else
					{
						// 姝ｅ湪瑙傚療鍏朵粬鐜╁
#ifdef USE_PLAYER_INFO
						player_info_t other;
						g_cInterfaces.Engine->GetPlayerInfo(target->GetIndex(), &other);
						g_cInterfaces.Engine->ClientCmd("echo \"[survivor] player %s %s %s\"",
							info.name, (mode == 4 ? "watching" : "following"), other.name);
#else
						g_cInterfaces.Engine->ClientCmd("echo \"[survivor] player %d %s %d\"",
							player->GetIndex(), (mode == 4 ? "watching" : "following"), target->GetIndex());
#endif
					}
				}
			}
			else if (team == 3)
			{
				// 鎰熸煋鑰
				int zombie = player->GetNetProp<int>("m_zombieClass", "DT_TerrorPlayer");
				char zombieName[32];
				switch (zombie)
				{
				case ZC_SMOKER:
					strcpy_s(zombieName, "smoker");
					break;
				case ZC_BOOMER:
					strcpy_s(zombieName, "boomer");
					break;
				case ZC_HUNTER:
					strcpy_s(zombieName, "hunter");
					break;
				case ZC_SPITTER:
					strcpy_s(zombieName, "spitter");
					break;
				case ZC_JOCKEY:
					strcpy_s(zombieName, "jockey");
					break;
				case ZC_CHARGER:
					strcpy_s(zombieName, "charger");
					break;
				case ZC_TANK:
					strcpy_s(zombieName, "tank");
					break;
				default:
					ZeroMemory(zombieName, 32);
				}

				if (player->IsAlive())
				{
					// 娲荤潃
#ifdef USE_PLAYER_INFO
					g_cInterfaces.Engine->ClientCmd("echo \"[infected] player %s is %s (%d)\"",
						info.name, zombieName, player->GetHealth());
#else
					g_cInterfaces.Engine->ClientCmd("echo \"[infected] player %d is %s (%d)\"",
						player->GetIndex(), zombieName, player->GetHealth());
#endif
				}
				else if (player->GetNetProp<byte>("m_isGhost", "DT_TerrorPlayer"))
				{
					// 骞界伒鐘舵�
#ifdef USE_PLAYER_INFO
					g_cInterfaces.Engine->ClientCmd("echo \"[infected] player %s is ghost %s (%d)\"",
						info.name, zombieName, player->GetHealth());
#else
					g_cInterfaces.Engine->ClientCmd("echo \"[infected] player %d is ghost %s (%d)\"",
						player->GetIndex(), zombieName, player->GetHealth());
#endif
				}
				else
				{
					// 姝讳骸
					if (mode != 4 && mode != 5)
						continue;

					target = (CBaseEntity*)player->GetNetProp<CBaseHandle*>("m_iObserverMode", "DT_BasePlayer");
					if (target && (int)target != -1)
						target = g_cInterfaces.ClientEntList->GetClientEntityFromHandle((CBaseHandle*)target);

					if (target == nullptr || (int)target == -1)
						continue;

					if ((DWORD)target == (DWORD)local)
					{
						// 姝ｅ湪瑙傚療鏈湴鐜╁
#ifdef USE_PLAYER_INFO
						g_cInterfaces.Engine->ClientCmd("echo \"[infected] player %s %s you\"",
							info.name, (mode == 4 ? "watching" : "following"));
#else
						g_cInterfaces.Engine->ClientCmd("echo \"[infected] player %s %s you\"",
							player->GetIndex(), (mode == 4 ? "watching" : "following"));
#endif
					}
					else
					{
						// 姝ｅ湪瑙傚療鍏朵粬鐜╁
#ifdef USE_PLAYER_INFO
						player_info_t other;
						g_cInterfaces.Engine->GetPlayerInfo(target->GetIndex(), &other);
						g_cInterfaces.Engine->ClientCmd("echo \"[infected] player %s %s %s\"",
							info.name, (mode == 4 ? "watching" : "following"), other.name);
#else
						g_cInterfaces.Engine->ClientCmd("echo \"[infected] player %d %s %d\"",
							player->GetIndex(), (mode == 4 ? "watching" : "following"), target->GetIndex());
#endif
					}
				}
			}
		}

		g_cInterfaces.Engine->ClientCmd("echo \"========= list end =========\"");
	}

	std::this_thread::sleep_for(std::chrono::microseconds(1));
}

void bindAlias(int wait)
{
	g_cInterfaces.Engine->ClientCmd("echo \"echo \"========= alias begin =========\"\"");
	g_cInterfaces.Engine->ClientCmd("alias +autofire \"alias autofire_launcher autofire_loop; autofire_launcher\"");
	g_cInterfaces.Engine->ClientCmd("alias -autofire \"alias autofire_launcher autofire_stop\"");
	g_cInterfaces.Engine->ClientCmd("alias autofire_launcher autofire_loop");
	g_cInterfaces.Engine->ClientCmd("alias autofire_loop \"+attack; wait 5; -attack; wait 5; autofire_launcher\"");
	g_cInterfaces.Engine->ClientCmd("alias autofire_stop \"-attack\"");
	g_cInterfaces.Engine->ClientCmd("alias +autojump \"alias autojump_launcher autojump_loop; autojump_launcher\"");
	g_cInterfaces.Engine->ClientCmd("alias -autojump \"alias autojump_launcher autojump_stop\"");
	g_cInterfaces.Engine->ClientCmd("alias autojump_launcher autojump_loop");
	g_cInterfaces.Engine->ClientCmd("alias autojump_loop \"+jump; wait 5; -jump; wait 5; autojump_launcher\"");
	g_cInterfaces.Engine->ClientCmd("alias autojump_stop \"-jump\"");
	g_cInterfaces.Engine->ClientCmd("alias +bigjump \"+jump; +duck\"");
	g_cInterfaces.Engine->ClientCmd("alias -bigjump \"-jump; -duck\"");
	g_cInterfaces.Engine->ClientCmd("alias +pistolspam \"alias pistolspam dopistolspam; dopistolspam\"");
	g_cInterfaces.Engine->ClientCmd("alias -pistolspam \"alias pistolspam -use\"");
	g_cInterfaces.Engine->ClientCmd("alias dopistolspam \"+use; wait 3; -use; wait 3; pistolspam\"");
	g_cInterfaces.Engine->ClientCmd("alias +fastmelee \"alias fastmelee_launcher fastmelee_loop; fastmelee_launcher\"");
	g_cInterfaces.Engine->ClientCmd("alias fastmelee_launcher fastmelee_loop");
	g_cInterfaces.Engine->ClientCmd("alias fastmelee_loop \"+attack; slot1; wait 1; -attack; slot2; wait 45; fastmelee_launcher\"");
	g_cInterfaces.Engine->ClientCmd("alias fastmelee_stop \"-attack\"");
	g_cInterfaces.Engine->ClientCmd("alias -fastmelee \"alias fastmelee_launcher fastmelee_stop\"");
	g_cInterfaces.Engine->ClientCmd("alias thirdperson_toggle \"thirdperson_enable\"");
	g_cInterfaces.Engine->ClientCmd("alias thirdperson_enable \"alias thirdperson_toggle thirdperson_disable; thirdpersonshoulder\"");
	g_cInterfaces.Engine->ClientCmd("alias thirdperson_disable \"alias thirdperson_toggle thirdperson_enable; thirdpersonshoulder; c_thirdpersonshoulder 0\"");
	g_cInterfaces.Engine->ClientCmd("c_thirdpersonshoulderoffset 0");
	g_cInterfaces.Engine->ClientCmd("c_thirdpersonshoulderaimdist 720");
	g_cInterfaces.Engine->ClientCmd("cam_ideallag 0");
	g_cInterfaces.Engine->ClientCmd("cam_idealdist 40");
	g_cInterfaces.Engine->ClientCmd("bind leftarrow \"incrementvar cam_idealdist 30 130 10\"");
	g_cInterfaces.Engine->ClientCmd("bind rightarrow \"incrementvar cam_idealdist 30 130 -10\"");
	g_cInterfaces.Engine->ClientCmd("bind uparrow \"incrementvar c_thirdpersonshoulderheight 5 25 5\"");
	g_cInterfaces.Engine->ClientCmd("bind downarrow \"incrementvar c_thirdpersonshoulderheight 5 25 -5\"");
	g_cInterfaces.Engine->ClientCmd("cl_crosshair_alpha 255");
	g_cInterfaces.Engine->ClientCmd("cl_crosshair_blue 0");
	g_cInterfaces.Engine->ClientCmd("cl_crosshair_green 50");
	g_cInterfaces.Engine->ClientCmd("cl_crosshair_red 190");
	g_cInterfaces.Engine->ClientCmd("cl_crosshair_dynamic 0");
	g_cInterfaces.Engine->ClientCmd("cl_crosshair_thickness 1.0");
	g_cInterfaces.Engine->ClientCmd("crosshair 1");
	g_cInterfaces.Engine->ClientCmd("con_enable 1");
	g_cInterfaces.Engine->ClientCmd("muzzleflash_light 1");
	g_cInterfaces.Engine->ClientCmd("sv_voiceenable 1");
	g_cInterfaces.Engine->ClientCmd("voice_enable 1");
	g_cInterfaces.Engine->ClientCmd("voice_forcemicrecord 1");
	g_cInterfaces.Engine->ClientCmd("mat_monitorgamma 1.6");
	g_cInterfaces.Engine->ClientCmd("mat_monitorgamma_tv_enabled 1");
	g_cInterfaces.Engine->ClientCmd("z_nightvision_r 255");
	g_cInterfaces.Engine->ClientCmd("z_nightvision_g 255");
	g_cInterfaces.Engine->ClientCmd("z_nightvision_b 255");
	g_cInterfaces.Engine->ClientCmd("bind space +jump");
	g_cInterfaces.Engine->ClientCmd("bind mouse1 +attack");
	g_cInterfaces.Engine->ClientCmd("bind mouse2 +attack2");
	g_cInterfaces.Engine->ClientCmd("bind mouse3 +zoom");
	g_cInterfaces.Engine->ClientCmd("bind ctrl +duck");
	g_cInterfaces.Engine->ClientCmd("unbind ins");
	g_cInterfaces.Engine->ClientCmd("unbind home");
	g_cInterfaces.Engine->ClientCmd("unbind pgup");
	g_cInterfaces.Engine->ClientCmd("unbind pgdn");
	g_cInterfaces.Engine->ClientCmd("unbind end");
	g_cInterfaces.Engine->ClientCmd("unbind del");
	g_cInterfaces.Engine->ClientCmd("unbind f9");
	g_cInterfaces.Engine->ClientCmd("unbind f10");
	g_cInterfaces.Engine->ClientCmd("unbind f11");
	g_cInterfaces.Engine->ClientCmd("unbind f12");
	g_cInterfaces.Engine->ClientCmd("unbind f8");
	g_cInterfaces.Engine->ClientCmd("echo \"echo \"========= alias end =========\"\"");
}

static CMoveData g_bMoveData;

#define StartEnginePrediction(_client,_cmd)	float curTime = g_cInterfaces.Globals->curtime;\
float frameTime = g_cInterfaces.Globals->frametime;\
int flags = _client->GetNetProp<int>("m_fFlags", "DT_BasePlayer");\
g_cInterfaces.Globals->curtime = _client->GetTickBase() * g_cInterfaces.Globals->interval_per_tick;\
g_cInterfaces.Globals->frametime = g_cInterfaces.Globals->interval_per_tick;\
g_cInterfaces.GameMovement->StartTrackPredictionErrors(_client);\
ZeroMemory(&g_bMoveData, sizeof(g_bMoveData));\
g_cInterfaces.MoveHelper->SetHost(_client);\
g_cInterfaces.Prediction->SetupMove(client, _cmd, nullptr, &g_bMoveData);\
g_cInterfaces.GameMovement->ProcessMovement(client, &g_bMoveData);\
g_cInterfaces.Prediction->FinishMove(client, _cmd, &g_bMoveData);

#define EndEnginePrediction(_client)	g_cInterfaces.GameMovement->FinishTrackPredictionErrors(_client);\
g_cInterfaces.MoveHelper->SetHost(nullptr);\
g_cInterfaces.Globals->curtime = curTime;\
g_cInterfaces.Globals->frametime = frameTime;\
client->SetNetProp("m_fFlags", flags, "DT_BasePlayer");

// engine pred not 100% perfect but almost
void RunEnginePrediction(CUserCmd* cmd)
{
	CBaseEntity* client = GetLocalClient();
	if (g_cInterfaces.MoveHelper == nullptr || cmd == nullptr || client == nullptr)
		return;

	float curTime = g_cInterfaces.Globals->curtime;
	float frameTime = g_cInterfaces.Globals->frametime;
	int flags = client->GetNetProp<int>("m_fFlags", "DT_BasePlayer");

	g_cInterfaces.Globals->curtime = client->GetTickBase() * g_cInterfaces.Globals->interval_per_tick;
	g_cInterfaces.Globals->frametime = g_cInterfaces.Globals->interval_per_tick;

	ZeroMemory(&g_bMoveData, sizeof(g_bMoveData));
	g_cInterfaces.GameMovement->StartTrackPredictionErrors(client);
	g_cInterfaces.MoveHelper->SetHost(client);
	g_cInterfaces.Prediction->SetupMove(client, cmd, nullptr, &g_bMoveData);
	g_cInterfaces.GameMovement->ProcessMovement(client, &g_bMoveData);
	g_cInterfaces.Prediction->FinishMove(client, cmd, &g_bMoveData);
	g_cInterfaces.GameMovement->FinishTrackPredictionErrors(client);
	g_cInterfaces.MoveHelper->SetHost(nullptr);

	g_cInterfaces.Globals->curtime = curTime;
	g_cInterfaces.Globals->frametime = frameTime;
	client->SetNetProp("m_fFlags", flags, "DT_BasePlayer");
}

// 检查该实体是否为生还者/特感/普感/Witch
bool IsValidEntity(CBaseEntity* entity)
{
	int id = 0, solid = 0, sequence = 0;
	ClientClass* cc = nullptr;

	try
	{
		if (entity == nullptr || !(cc = entity->GetClientClass()) || (id = cc->m_ClassID) == ET_WORLD ||
			!IsValidEntityId(id) || entity->IsDormant())
			return false;

		solid = entity->GetNetProp<int>("m_usSolidFlags", "DT_BaseCombatCharacter");
		sequence = entity->GetNetProp<int>("m_nSequence", "DT_BaseCombatCharacter");
	}
	catch (...)
	{
#ifdef _DEBUG
		Utils::log("%s (%d) 警告：指针 0x%X 并不是实体！", __FILE__, __LINE__, (DWORD)entity);
		throw std::exception("IsValidEntity 发生了错误");
#endif
		return false;
	}

	if (id == ET_BOOMER || id == ET_HUNTER || id == ET_SMOKER || id == ET_SPITTER ||
		id == ET_JOCKEY || id == ET_CHARGER || id == ET_TANK)
	{
		if (!entity->IsAlive() || entity->GetNetProp<byte>("m_isGhost", "DT_TerrorPlayer") != 0)
		{
#ifdef _DEBUG_OUTPUT
			g_cInterfaces.Engine->ClientCmd("echo \"Special 0x%X healh = %d, ghost = %d\"", (DWORD)entity,
				entity->GetNetProp<int>("m_iHealth", "DT_TerrorPlayer"), entity->GetNetProp<int>("m_isGhost", "DT_TerrorPlayer"));
#endif
			return false;
		}
	}
	else if (id == ET_INFECTED || id == ET_WITCH)
	{
		if ((solid & SF_NOT_SOLID))
		{
#ifdef _DEBUG_OUTPUT
			g_cInterfaces.Engine->ClientCmd("echo \"Common 0x%X is dead\"", (DWORD)entity);
#endif
			return false;
		}
	}
	else if (id == ET_CTERRORPLAYER || id == ET_SURVIVORBOT)
	{
		if (!entity->IsAlive())
		{
#ifdef _DEBUG_OUTPUT
			g_cInterfaces.Engine->ClientCmd("echo \"Survivor 0x%X is dead %d\"", (DWORD)entity,
				entity->GetNetProp<int>("m_iHealth", "DT_TerrorPlayer"));
#endif
			return false;
		}
	}
	else
	{
#ifdef _DEBUG_OUTPUT
		// Utils::log("Invalid ClassId = %d | sequence = %d | solid = %d", id, sequence, solid);
		g_cInterfaces.Engine->ClientCmd("echo \"Invalid Entity 0x%X ClassId %d\"", (DWORD)entity, id);
#endif
		return false;
	}

	return true;
}

// 获取实体的类型
std::string GetZombieClassName(CBaseEntity* player)
{
	if (player == nullptr || player->IsDormant())
		return "";

	if (player->GetClientClass()->m_ClassID == ET_INFECTED)
	{
		int zombie = player->GetNetProp<int>("m_Gender", "DT_Infected");
		switch (zombie)
		{
			/*
			case 1:
			return "male";
			case 2:
			return "female";
			*/
		case 11:
			// 防火 CEDA 人员
			return "ceda";
		case 12:
			// 泥人
			return "mud";
		case 13:
			// 修路工人
			return "roadcrew";
		case 14:
			// 被感染的幸存者
			return "fallen";
		case 15:
			// 防暴警察
			return "riot";
		case 16:
			// 小丑
			return "clown";
		case 17:
			// 赛车手吉米
			return "jimmy";
		}

		// 常见感染者
		return "infected";
	}
	if (player->GetClientClass()->m_ClassID == ET_WITCH)
	{
		// 新娘 Witch
		if (player->GetNetProp<int>("m_Gender", "DT_Infected") == 19)
			return "bride";

		// 普通 Witch
		return "witch";
	}

	int zombie = player->GetNetProp<int>("m_zombieClass", "DT_TerrorPlayer");
	int character = player->GetNetProp<int>("m_survivorCharacter", "DT_TerrorPlayer");

	switch (zombie)
	{
	case ZC_SMOKER:
		// 舌头
		return "smoker";
	case ZC_BOOMER:
		// 胖子
		return "boomer";
	case ZC_HUNTER:
		// 猎人
		return "hunter";
	case ZC_SPITTER:
		// 口水
		return "spitter";
	case ZC_JOCKEY:
		// 猴
		return "jockey";
	case ZC_CHARGER:
		// 牛
		return "charger";
	case ZC_TANK:
		// 克
		return "tank";
	case ZC_SURVIVORBOT:
		switch (character)
		{
		case 0:
			// 西装
			return "nick";
		case 1:
			// 黑妹
			return "rochelle";
		case 2:
			// 黑胖
			return "coach";
		case 3:
			// 帽子
			return "ellis";
		case 4:
			// 老头
			return "bill";
		case 5:
			// 女人
			return "zoey";
		case 6:
			// 马甲
			return "francis";
		case 7:
			// 光头
			return "louis";
		}
	}

	// 不知道
	return "unknown";
}

// 根据骨头获取头部位置
Vector GetHeadPosition(CBaseEntity* player)
{
	Vector position;
	int zombieClass = player->GetNetProp<int>("m_zombieClass", "DT_TerrorPlayer");
	if (zombieClass == ZC_SMOKER || zombieClass == ZC_HUNTER || zombieClass == ZC_TANK)
		position = player->GetBonePosition(BONE_NECK);
	else if (zombieClass == ZC_SPITTER || zombieClass == ZC_JOCKEY)
		position = player->GetBonePosition(BONE_JOCKEY_HEAD);
	else if (zombieClass == ZC_BOOMER)
		position = player->GetBonePosition(BONE_BOOMER_CHEST);
	else if (zombieClass == ZC_CHARGER)
		position = player->GetBonePosition(BONE_CHARGER_HEAD);
	else if (zombieClass == ZC_SURVIVORBOT)
	{
		int character = player->GetNetProp<int>("m_survivorCharacter", "DT_TerrorPlayer");
		switch (character)
		{
		case 0:
			position = player->GetBonePosition(BONE_NICK_HEAD);
			break;
		case 1:
			position = player->GetBonePosition(BONE_ROCHELLE_HEAD);
			break;
		case 2:
			position = player->GetBonePosition(BONE_COACH_HEAD);
			break;
		case 3:
			position = player->GetBonePosition(BONE_ELLIS_HEAD);
			break;
		case 4:
			position = player->GetBonePosition(BONE_BILL_HEAD);
			break;
		case 5:
			position = player->GetBonePosition(BONE_ZOEY_HEAD);
			break;
		case 6:
			position = player->GetBonePosition(BONE_FRANCIS_HEAD);
			break;
		case 7:
			position = player->GetBonePosition(BONE_LOUIS_HEAD);
			break;
		}
	}

	if (!position.IsValid() || position.IsZero(0.001f))
	{
		if (zombieClass == ZC_JOCKEY)
			position.z = g_pCurrentAiming->GetAbsOrigin().z + 30.0f;
		else if (zombieClass == ZC_HUNTER && (g_pCurrentAiming->GetFlags() & FL_DUCKING))
			position.z -= 12.0f;
	}

	return position;
}

// 根据击中盒获取头部位置
Vector GetHeadHitboxPosition(CBaseEntity* entity)
{
	Vector position;

#ifdef _DEBUG
	try
	{
#endif
		if (!IsValidEntity(entity))
			return position;
#ifdef _DEBUG
	}
	catch (std::exception e)
	{
		Utils::log("%s (%d): %s | 0x%X", __FILE__, __LINE__, e.what(), (DWORD)entity);
		return position;
	}
	catch (...)
	{
		Utils::log("%s (%d): 未知异常 | 0x%X", __FILE__, __LINE__, (DWORD)entity);
		return position;
	}
#endif

	int classId = entity->GetClientClass()->m_ClassID;
	switch (classId)
	{
	case ET_TANK:
	case ET_WITCH:
	case ET_SMOKER:
	case ET_BOOMER:
	case ET_HUNTER:
	case ET_CTERRORPLAYER:
	case ET_SURVIVORBOT:
		position = entity->GetHitboxPosition(HITBOX_PLAYER);
		break;
	case ET_JOCKEY:
	case ET_SPITTER:
		position = entity->GetHitboxPosition(HITBOX_JOCKEY);
		break;
	case ET_CHARGER:
		position = entity->GetHitboxPosition(HITBOX_CHARGER);
		break;
	case ET_INFECTED:
		position = entity->GetHitboxPosition(HITBOX_COMMON);
		break;
	}

	/*
	if (!position.IsValid())
		position = GetHeadPosition(entity);
	*/

	return position;
}

// 获取当前正在瞄准的敌人
CBaseEntity* GetAimingTarget(int hitbox = 0)
{
	CBaseEntity* client = GetLocalClient();
	if (client == nullptr || !client->IsAlive())
		return nullptr;

	trace_t trace;
	Vector src = client->GetEyePosition(), dst;
	Ray_t ray;

	CTraceFilter filter;
	filter.pSkip1 = client;
	g_cInterfaces.Engine->GetViewAngles(dst);

	// angle (QAngle) to basic vectors.
	AngleVectors(dst, &dst);

	// multiply our angles by shooting range.
	dst *= 3500.f;

	// end point = our eye position + shooting range.
	dst += src;

	ray.Init(src, dst);

#ifdef _DEBUG_OUTPUT
	Utils::log("TraceRay: skip = 0x%X | start = (%.2f %.2f %.2f) | end = (%.2f %.2f %.2f)",
		(DWORD)client, src.x, src.y, src.z, dst.x, dst.y, dst.z);
#endif

	g_cInterfaces.Trace->TraceRay(ray, MASK_SHOT, &filter, &trace);

#ifdef _DEBUG_OUTPUT
	Utils::log("TraceRay: entity = 0x%X | hitbox = %d | bone = %d | hitGroup = %d | fraction = %.2f | classId = %d",
		trace.m_pEnt, trace.hitbox, trace.physicsBone, trace.hitGroup, trace.fraction,
		(trace.m_pEnt != nullptr ? trace.m_pEnt->GetClientClass()->m_ClassID : -1));
#endif

	// 妫�鏌ョ洰鏍囨槸鍚︿负涓�涓湁鏁堢殑瀹炰綋
	if (trace.m_pEnt == nullptr || trace.m_pEnt->IsDormant() ||
		trace.m_pEnt->GetClientClass()->m_ClassID == ET_WORLD)
	{
#ifdef _DEBUG_OUTPUT
		g_cInterfaces.Engine->ClientCmd("echo \"invalid entity 0x%X | start (%.2f %.2f %.2f) end (%.2f %.2f %.2f)\"",
			(DWORD)trace.m_pEnt, trace.start.x, trace.start.y, trace.start.z, trace.end.x, trace.end.y, trace.end.z);
#endif
		return nullptr;
	}

	// 妫�鏌ョ洰鏍囨槸鍚︿负涓�涓彲瑙佺殑鐗╀綋锛屾垨鑰呰鐗╀綋鏄惁鍙互琚嚮涓紝浠ュ強鏄惁涓烘寚瀹氫綅
	if (trace.hitbox == 0 || (hitbox > 0 && trace.hitbox != hitbox))
	{
#ifdef _DEBUG_OUTPUT
		g_cInterfaces.Engine->ClientCmd("echo \"invalid hitbox 0x%X | hitbox = %d | bone = %d | group = %d\"",
			(DWORD)trace.m_pEnt, trace.hitbox, trace.physicsBone, trace.hitGroup);
#endif
		return nullptr;
	}

	if (g_bAimBot && hitbox == 0 && trace.m_pEnt->GetClientClass()->m_ClassID == ET_INFECTED)
	{
		if (trace.hitbox < HITBOX_COMMON_1 || trace.hitbox > HITBOX_COMMON_4)
		{
#ifdef _DEBUG_OUTPUT
			g_cInterfaces.Engine->ClientCmd("echo \"invalid hitbox by infected %d\"", trace.hitbox);
#endif
			return nullptr;
		}
	}

	return trace.m_pEnt;
}

// 检查本地玩家是否可以看见某个实体
bool IsTargetVisible(CBaseEntity* entity, Vector end, Vector start)
{
#ifdef _DEBUG
	try
	{
#endif
		if (!IsValidEntity(entity))
			return false;
#ifdef _DEBUG
	}
	catch (std::exception e)
	{
		Utils::log("%s (%d): %s | 0x%X", __FILE__, __LINE__, e.what(), (DWORD)entity);
		return false;
	}
	catch (...)
	{
		Utils::log("%s (%d): 未知异常 | 0x%X", __FILE__, __LINE__, (DWORD)entity);
		return false;
	}
#endif

	CBaseEntity* client = GetLocalClient();
	if (client == nullptr || !g_cInterfaces.Engine->IsInGame())
		return false;

	trace_t trace;
	Ray_t ray;

	CTraceFilter filter;
	filter.pSkip1 = client;

	if (!start.IsValid())
		start = client->GetEyePosition();
	if (!end.IsValid())
		end = GetHeadHitboxPosition(entity);

	ray.Init(start, end);
	g_cInterfaces.Trace->TraceRay(ray, MASK_SHOT, &filter, &trace);

	// 检查是否为指定目标
	if ((DWORD)trace.m_pEnt != (DWORD)entity || trace.hitbox <= 0)
		return false;

	return true;
}

// -------------------------------- D3D9 Device Hooked Function --------------------------------
HRESULT WINAPI Hooked_Reset(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* pp)
{
	static bool showHint = true;
	if (showHint)
	{
		showHint = false;
		if ((DWORD)g_pDeviceHooker->GetDevice() == (DWORD)device)
			Utils::log("Hooked_Reset success");
	}

	if (g_pDeviceHooker->GetDevice() == nullptr)
	{
		ResetDeviceHook(device);
		showHint = true;
	}

	if (!g_pDrawRender)
		g_pDrawRender = std::make_unique<DrawManager>(device);

	g_pDrawRender->OnLostDevice();

	HRESULT result = oReset(device, pp);

	g_pDrawRender->OnResetDevice();

	return result;
}

HRESULT WINAPI Hooked_Present(IDirect3DDevice9* device, const RECT* source, const RECT* dest, HWND window, const RGNDATA* region)
{
	static bool showHint = true;
	if (showHint)
	{
		showHint = false;
		if ((DWORD)g_pDeviceHooker->GetDevice() == (DWORD)device)
			Utils::log("Hooked_Present success");
	}

	if (g_pDeviceHooker->GetDevice() == nullptr)
	{
		ResetDeviceHook(device);
		showHint = true;
	}

	return oPresent(device, source, dest, window, region);
}

HRESULT WINAPI Hooked_EndScene(IDirect3DDevice9* device)
{
	static bool showHint = true;
	if (showHint)
	{
		showHint = false;
		if ((DWORD)g_pDeviceHooker->GetDevice() == (DWORD)device)
			Utils::log("Hooked_EndScene success");
	}

	if (g_pDeviceHooker->GetDevice() == nullptr)
	{
		ResetDeviceHook(device);
		showHint = true;
	}

	// 初始化
	if (!g_pDrawRender)
		g_pDrawRender = std::make_unique<DrawManager>(device);

	// 备份之前绘制设置
	g_pDrawRender->BeginRendering();

#ifdef USE_D3D_DRAW
	CBaseEntity* local = GetLocalClient();
	if (local == nullptr || !g_cInterfaces.Engine->IsInGame())
		goto finish_draw;

	// 目前最小距离
	float distmin = 65535.0f;

	// 当前队伍
	int team = local->GetTeam();

	// 当前是否有自动瞄准的目标
	bool targetSelected = false;

#ifdef _DEBUG
	try
	{
#endif
		targetSelected = IsValidEntity(g_pCurrentAiming);
#ifdef _DEBUG
	}
	catch (std::exception e)
	{
		Utils::log("%s (%d): %s | 0x%X", __FILE__, __LINE__, e.what(), (DWORD)g_pCurrentAiming);
		targetSelected = false;
		g_pCurrentAiming = nullptr;
	}
	catch (...)
	{
		Utils::log("%s (%d): 未知异常 | 0x%X", __FILE__, __LINE__, (DWORD)g_pCurrentAiming);
		targetSelected = false;
		g_pCurrentAiming = nullptr;
	}
#endif

	if (!targetSelected || !(GetAsyncKeyState(VK_LBUTTON) & 0x8000))
		g_pCurrentAiming = nullptr;

	Vector myViewAngles;
	g_cInterfaces.Engine->GetViewAngles(myViewAngles);

	Vector myEyeOrigin = local->GetEyePosition();
	Vector myOrigin = local->GetAbsOrigin();

	// 一般普感实体索引上限 512 就够了，太大会卡的
	int maxEntity = g_cInterfaces.ClientEntList->GetHighestEntityIndex();

	// 绘制颜色
	D3DCOLOR color = 0;

	for (int i = 1; i <= maxEntity; ++i)
	{
		CBaseEntity* entity = g_cInterfaces.ClientEntList->GetClientEntity(i);
		if ((DWORD)entity == (DWORD)local)
			continue;

		if (g_pGameRulesProxy == nullptr && entity != nullptr && !entity->IsDormant() &&
			entity->GetClientClass()->m_ClassID == ET_TerrorGameRulesProxy)
		{
			g_pGameRulesProxy = entity;
			Utils::log("TerrorGameRulesProxy Entity found 0x%X", (DWORD)g_pGameRulesProxy);
		}

#ifdef _DEBUG
		try
		{
#endif
			if (!IsValidEntity(entity))
				continue;
#ifdef _DEBUG
}
		catch (std::exception e)
		{
			Utils::log("%s (%d): %s | 0x%X", __FILE__, __LINE__, e.what(), (DWORD)entity);
			continue;
		}
		catch (...)
		{
			Utils::log("%s (%d): 未知异常 | 0x%X", __FILE__, __LINE__, (DWORD)entity);
			continue;
		}
#endif

		Vector head, foot, headbox, origin;
		int classId = entity->GetClientClass()->m_ClassID;

		// 目标的头部的位置
		headbox = GetHeadHitboxPosition(entity);

		// 目标脚下的位置
		origin = (classId == ET_INFECTED || classId == ET_WITCH ?
			entity->GetNetProp<Vector>("m_vecOrigin", "DT_BaseCombatCharacter") : entity->GetAbsOrigin());

		// 检查目标是否在屏幕内
		if (!headbox.IsValid() || !WorldToScreen(headbox, head) ||
			!WorldToScreen(origin, foot))
			continue;

		// 目标是否可见
		bool visible = IsTargetVisible(entity, headbox, myEyeOrigin);

		// 目标与自己的距离
		float dist = myOrigin.DistTo(origin);

		// 给玩家绘制一个框
		if (g_bDrawBoxEsp)
		{
			// 根据类型决定绘制的内容
			if (classId != ET_INFECTED && classId != ET_WITCH)
			{
				// 用于格式化字符串
				std::stringstream ss;

				// 去除 float 的小数位，因为没必要
				ss << std::setprecision(0);

				// 检查是否为生还者
				if (classId == ET_SURVIVORBOT || classId == ET_CTERRORPLAYER)
				{
					if (IsIncapacitated(entity))
					{
						// 倒地时只有普通血量
						ss << "[" << entity->GetHealth() << " + incap] ";
					}
					else if (IsControlled(entity))
					{
						// 玩家被控了
						ss << "[" << (int)(entity->GetHealth() +
							entity->GetNetProp<float>("m_healthBuffer", "DT_TerrorPlayer")) <<
							" + grabbed] ";
					}
					else
					{
						// 生还者显示血量，临时血量
						ss << "[" << entity->GetHealth() << " + " << std::setprecision(0) <<
							(int)(entity->GetNetProp<float>("m_healthBuffer", "DT_TerrorPlayer")) << "] ";
					}
				}
				else
				{
					if (entity->GetNetProp<byte>("m_isGhost", "DT_TerrorPlayer") != 0)
					{
						// 幽灵状态的特感
						ss << "[" << entity->GetHealth() << " ghost] ";
					}
					else
					{
						// 非生还者只显示血量就好了
						ss << "[" << entity->GetHealth() << "] ";
					}
				}

				// 玩家类型
				ss << GetZombieClassName(entity);
				g_pDrawRender->DrawString2Begin();

				float height = fabs(head.y - foot.y);
				float width = height * 0.65f;

				// 根据情况决定颜色
				if ((classId == ET_SURVIVORBOT || classId == ET_CTERRORPLAYER) &&
					entity->GetNetProp<byte>("m_bIsOnThirdStrike", "DT_TerrorPlayer") != 0)
				{
					// 生还者黑白时使用白色
					color = DrawManager::WHITE;
				}
				else if ((classId == ET_BOOMER || classId == ET_SMOKER || classId == ET_HUNTER ||
					classId == ET_SPITTER || classId == ET_CHARGER || classId == ET_JOCKEY) &&
					entity->GetNetProp<byte>("m_isGhost", "DT_TerrorPlayer") != 0)
				{
					// 幽灵状态的特感，紫色
					color = DrawManager::PURPLE;
				}
				else
				{
					// 其他情况，橙色
					color = DrawManager::ORANGE;
				}

				g_pDrawRender->DrawString2(foot.x - width / 2, head.y, color, ss.str().c_str());

				ss.str("");

				// 显示距离
				ss << std::setprecision(0) << (int)dist;

				// 给生还者显示弹药
				if (classId == ET_SURVIVORBOT || classId == ET_CTERRORPLAYER)
				{
					CBaseEntity* weapon = (CBaseEntity*)entity->GetActiveWeapon();
					if (weapon != nullptr)
						weapon = g_cInterfaces.ClientEntList->GetClientEntityFromHandle((CBaseHandle*)weapon);
					if (weapon != nullptr)
					{
						int ammoType = weapon->GetNetProp<int>("m_iPrimaryAmmoType", "DT_BaseCombatWeapon");
						int clip = weapon->GetNetProp<int>("m_iClip1", "DT_BaseCombatWeapon");
						byte reloading = weapon->GetNetProp<byte>("m_bInReload", "DT_BaseCombatWeapon");

						// 显示弹药和弹夹
						if (ammoType > 0 && clip > -1)
						{
							if (reloading != 0)
							{
								// 正在换子弹
								ss << " (reloading)";
							}
							else
							{
								// 没有换子弹
								ss << " (" << clip << " / " <<
									entity->GetNetProp<int>("m_iAmmo", "DT_TerrorPlayer", (size_t)ammoType) <<
									")";
							}
						}
					}
				}
				// 显示特感技能冷却时间
				/*
				else if (classId == ET_BOOMER || classId == ET_HUNTER || classId == ET_SMOKER ||
				classId == ET_JOCKEY || classId == ET_CHARGER || classId == ET_SPITTER ||
				classId == ET_TANK)
				{
				CBaseHandle* handle = (classId != ET_TANK ?
				entity->GetNetProp<CBaseHandle*>("m_customAbility", "DT_TerrorPlayer") :
				entity->GetNetProp<CBaseHandle*>("m_hActiveWeapon", "DT_BaseCombatCharacter"));

				CBaseEntity* weapon = (handle->IsValid() ?
				g_cInterfaces.ClientEntList->GetClientEntityFromHandle(handle):
				nullptr);

				if (weapon != nullptr)
				{
				float serverTime = GetServerTime();

				if (classId == ET_TANK)
				{
				// 主要攻击（拍人）
				float primary = weapon->GetNetProp<float>("m_flNextPrimaryAttack", "DT_BaseCombatWeapon");

				// 次要攻击（掷饼）
				float secondary = weapon->GetNetProp<float>("m_flNextSecondaryAttack", "DT_BaseCombatWeapon");

				// 坦克的爪子
				if (primary <= serverTime)
				ss << " (ready / ";
				else
				ss << " (" << (int)(primary - serverTime) << " / ";

				// 坦克的投石
				if (secondary <= serverTime)
				ss << "ready)";
				else
				ss << (int)(secondary - serverTime) << ")";
				}
				else
				{
				float ability = weapon->GetNetProp<float>("m_duration");
				if (ability <= 0.0f)
				ss << " (ready)";
				else
				ss << "(" << (int)ability << ")";
				}
				}
				}
				*/

				// 检查是否可以看见
				if (visible)
				{
					// 看得见，显示蓝色

					color = DrawManager::BLUE;
				}
				else
				{
					// 看不见，显示黄色
					color = DrawManager::YELLOW;
				}

				g_pDrawRender->DrawString2(foot.x - width / 2, head.y + g_pDrawRender->GetFontSize(),
					color, ss.str().c_str());

				g_pDrawRender->DrawString2Finish();

				// 绘制一个框（虽然这个框只有上下两条线）
				g_pDrawRender->RenderRect(DrawManager::RED, foot.x - width / 2, foot.y, width, -height);
			}
			else
			{
				// 这只是普感而已，太远了没必要显示出来
				if (dist > 2500.0f)
					continue;

				int size = 0;

				if (visible)
					size = 4;
				else
					size = 2;

				if (classId == ET_WITCH)
					color = DrawManager::PINK;
				else
					color = DrawManager::GREEN;

				// 画一个小方形，以标记为头部
				g_pDrawRender->RenderFillRect(color, head.x, head.y, size, size);
			}
		}

		// 自动瞄准寻找目标
		if (g_bAimBot && (!targetSelected || !(g_pUserCommands->buttons & IN_ATTACK)) &&
			classId != ET_WITCH && (classId != ET_INFECTED || team == 2))
		{
			// 已经选择过目标了，并且这是一个不重要的敌人
			if (classId == ET_INFECTED && distmin < 65535.0f)
				continue;

			// 选择一个最接近的特感，因为特感越近对玩家来说越危险
			if (entity->GetTeam() != team && dist < distmin && visible &&
				GetAnglesFieldOfView(myViewAngles, CalculateAim(myEyeOrigin, headbox)) <= g_fAimbotFieldOfView)
			{
				g_pCurrentAiming = entity;
				distmin = dist;
			}
		}
	}

	// 绘制准星
	if (g_bDrawCrosshairs)
	{
		int width, height;
		g_cInterfaces.Engine->GetScreenSize(width, height);

		int aiming = *(int*)(local + m_iCrosshairsId);
		CBaseEntity* target = (aiming > 0 ? g_cInterfaces.ClientEntList->GetClientEntity(aiming) : GetAimingTarget(-1));

#ifdef _DEBUG
		try
		{
#endif
			if (!IsValidEntity(target))
				target = nullptr;
#ifdef _DEBUG
		}
		catch (std::exception e)
		{
			Utils::log("%s (%d): %s | 0x%X", __FILE__, __LINE__, e.what(), (DWORD)target);
			target = nullptr;
		}
		catch (...)
		{
			Utils::log("%s (%d): 未知异常 | 0x%X", __FILE__, __LINE__, (DWORD)target);
			target = nullptr;
		}
#endif
		int classId = (target == nullptr ? ET_INVALID : target->GetClientClass()->m_ClassID);
		if (classId == ET_INVALID)
			color = DrawManager::LAWNGREEN;
		else if (target->GetTeam() == local->GetTeam())
			color = DrawManager::BLUE;
		else if (classId == ET_INFECTED)
			color = DrawManager::ORANGE;
		else if (classId == ET_WITCH)
			color = DrawManager::PINK;
		else
			color = DrawManager::RED;

		width /= 2;
		height /= 2;
		g_pDrawRender->RenderLine(color, width - 10, height, width + 10, height);
		g_pDrawRender->RenderLine(color, width, height - 10, width, height + 10);
	}

finish_draw:
#endif

	g_bNewFrame = true;

	// 还原备份的设置
	g_pDrawRender->EndRendering();

	return oEndScene(device);
}

HRESULT WINAPI Hooked_DrawIndexedPrimitive(IDirect3DDevice9* device, D3DPRIMITIVETYPE type, INT baseIndex,
	UINT minIndex, UINT numVertices, UINT startIndex, UINT primitiveCount)
{
	static bool showHint = true;
	if (showHint)
	{
		showHint = false;
		if ((DWORD)g_pDeviceHooker->GetDevice() == (DWORD)device)
			Utils::log("Hooked_DrawIndexedPrimitive success");
	}

	if (g_pDeviceHooker->GetDevice() == nullptr)
	{
		ResetDeviceHook(device);
		showHint = true;
	}

	if (g_cInterfaces.Engine->IsInGame())
	{
		IDirect3DVertexBuffer9* stream = nullptr;
		UINT offsetByte, stride;
		device->GetStreamSource(0, &stream, &offsetByte, &stride);
		if (l4d2_special(stride, numVertices, primitiveCount) ||
			l4d2_weapons(stride, numVertices, primitiveCount) ||
			l4d2_stuff(stride, numVertices, primitiveCount) ||
			l4d2_survivor(stride, numVertices, primitiveCount))
		{
			static DWORD oldZEnable;
			device->GetRenderState(D3DRS_ZENABLE, &oldZEnable);
			device->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
			device->SetRenderState(D3DRS_ZFUNC, D3DCMP_NEVER);
			oDrawIndexedPrimitive(device, type, baseIndex, minIndex, numVertices, startIndex, primitiveCount);
			device->SetRenderState(D3DRS_ZENABLE, oldZEnable);
			device->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
			// oDrawIndexedPrimitive(device, type, baseIndex, minIndex, numVertices, startIndex, primitiveCount);
		}
	}

	return oDrawIndexedPrimitive(device, type, baseIndex, minIndex, numVertices, startIndex, primitiveCount);
}

HRESULT WINAPI Hooked_CreateQuery(IDirect3DDevice9* device, D3DQUERYTYPE type, IDirect3DQuery9** query)
{
	static bool showHint = true;
	if (showHint)
	{
		showHint = false;
		if ((DWORD)g_pDeviceHooker->GetDevice() == (DWORD)device)
			Utils::log("Hooked_CreateQuery success");
	}

	if (g_pDeviceHooker->GetDevice() == nullptr)
	{
		ResetDeviceHook(device);
		showHint = true;
	}

	/*	不要启用这个，会导致屏幕变黑的
	if (type == D3DQUERYTYPE_OCCLUSION)
	type = D3DQUERYTYPE_TIMESTAMP;
	*/

	return oCreateQuery(device, type, query);
}

// -------------------------------- Game Hooked Function --------------------------------
void __fastcall Hooked_PaintTraverse(void* pPanel, void* edx, unsigned int panel, bool forcePaint, bool allowForce)
{
	static bool showHint = true;
	if (showHint)
	{
		showHint = false;
		Utils::log("Hooked_PaintTraverse trigged.");
	}

	static unsigned int MatSystemTopPanel = 0;
	static unsigned int FocusOverlayPanel = 0;
	static const int FontSize = 16;

	if (MatSystemTopPanel == 0 || FocusOverlayPanel == 0)
	{
		const char* panelName = g_cInterfaces.Panel->GetName(panel);
		if (panelName[0] == 'M' && panelName[3] == 'S' && panelName[9] == 'T')
		{
			MatSystemTopPanel = panel;
			Utils::log("panel %s found %d", panelName, MatSystemTopPanel);
		}
		else if (panelName[0] == 'F' && panelName[5] == 'O')
		{
			FocusOverlayPanel = panel;
			Utils::log("panel %s found %d", panelName, FocusOverlayPanel);
		}
	}

	static unsigned long font = 0;
	if (font == 0)
	{
		font = g_cInterfaces.Surface->SCreateFont();
		g_cInterfaces.Surface->SetFontGlyphSet(font, "arial", FontSize, FW_DONTCARE, 0, 0, 0x200);
	}

	/*	Panel Tree
		MatSystemTopPanel
			staticPanel
				GameUI Background Panel
				staticClientDLLToolsPanel
				FocusOverlayPanel
				staticClientDLLPanel
					Fullscene Root Panel
						CBaseViewport
							HudScriptedMode
							HudCloseCaption
	*/

	// 每一帧调用 1 次
	if (FocusOverlayPanel > 0 && panel == FocusOverlayPanel)
	{
		// 不建议在这里绘制，因为这个并不是全屏幕的
	}

	// 每一帧调 很多次 在这里不能做消耗较大的事情
	if (MatSystemTopPanel > 0 && panel == MatSystemTopPanel)
	{
		// 在这里绘制会导致游戏里 fps 非常低，因此必须要做出限制
#ifndef USE_D3D_DRAW
		if (!g_bNewFrame)
			goto finish_draw;

		g_bNewFrame = false;

		CBaseEntity* local = GetLocalClient();
		if (local == nullptr || !g_cInterfaces.Engine->IsInGame())
			goto finish_draw;

		// 目前最小距离
		float distmin = 65535.0f;

		// 当前队伍
		int team = local->GetTeam();

		// 当前是否有自动瞄准的目标
		bool targetSelected = false;

#ifdef _DEBUG
		try
		{
#endif
			targetSelected = IsValidEntity(g_pCurrentAiming);
#ifdef _DEBUG
		}
		catch (std::exception e)
		{
			Utils::log("%s (%d): %s | 0x%X", __FILE__, __LINE__, e.what(), (DWORD)g_pCurrentAiming);
			targetSelected = false;
			g_pCurrentAiming = nullptr;
		}
		catch (...)
		{
			Utils::log("%s (%d): 未知异常 | 0x%X", __FILE__, __LINE__, (DWORD)g_pCurrentAiming);
			targetSelected = false;
			g_pCurrentAiming = nullptr;
		}
#endif

		if (!targetSelected || !(GetAsyncKeyState(VK_LBUTTON) & 0x8000))
			g_pCurrentAiming = nullptr;

		Vector myViewAngles;
		g_cInterfaces.Engine->GetViewAngles(myViewAngles);

		Vector myEyeOrigin = local->GetEyePosition();
		Vector myOrigin = local->GetAbsOrigin();

		// 一般普感实体索引上限 512 就够了，太大会卡的
		int maxEntity = g_cInterfaces.ClientEntList->GetHighestEntityIndex();

		// 绘制颜色
		D3DCOLOR color = 0;

		for (int i = 1; i <= maxEntity; ++i)
		{
			CBaseEntity* entity = g_cInterfaces.ClientEntList->GetClientEntity(i);
			if ((DWORD)entity == (DWORD)local)
				continue;

			if (g_pGameRulesProxy == nullptr && entity != nullptr && !entity->IsDormant() &&
				entity->GetClientClass()->m_ClassID == ET_TerrorGameRulesProxy)
			{
				g_pGameRulesProxy = entity;
				Utils::log("TerrorGameRulesProxy Entity found 0x%X", (DWORD)g_pGameRulesProxy);
			}

#ifdef _DEBUG
			try
			{
#endif
				if (!IsValidEntity(entity))
					continue;
#ifdef _DEBUG
			}
			catch (std::exception e)
			{
				Utils::log("%s (%d): %s | 0x%X", __FILE__, __LINE__, e.what(), (DWORD)entity);
				continue;
			}
			catch (...)
			{
				Utils::log("%s (%d): 未知异常 | 0x%X", __FILE__, __LINE__, (DWORD)entity);
				continue;
			}
#endif

			Vector head, foot, headbox, origin;
			int classId = entity->GetClientClass()->m_ClassID;

			// 目标的头部的位置
			headbox = GetHeadHitboxPosition(entity);

			// 目标脚下的位置
			origin = (classId == ET_INFECTED || classId == ET_WITCH ?
				entity->GetNetProp<Vector>("m_vecOrigin", "DT_BaseCombatCharacter") : entity->GetAbsOrigin());

			// 检查目标是否在屏幕内
			if (!headbox.IsValid() || !WorldToScreen(headbox, head) ||
				!WorldToScreen(origin, foot))
				continue;

			// 目标是否可见
			bool visible = IsTargetVisible(entity, headbox, myEyeOrigin);

			// 目标与自己的距离
			float dist = myOrigin.DistTo(origin);

			// 给玩家绘制一个框
			if (g_bDrawBoxEsp)
			{
				// 根据类型决定绘制的内容
				if (classId != ET_INFECTED && classId != ET_WITCH)
				{
					// 用于格式化字符串
					std::stringstream ss;

					// 去除 float 的小数位，因为没必要
					ss << std::setprecision(0);

					// 检查是否为生还者
					if (classId == ET_SURVIVORBOT || classId == ET_CTERRORPLAYER)
					{
						if (IsIncapacitated(entity))
						{
							// 倒地时只有普通血量
							ss << "[" << entity->GetHealth() << " + incap] ";
						}
						else if (IsControlled(entity))
						{
							// 玩家被控了
							ss << "[" << (int)(entity->GetHealth() +
								entity->GetNetProp<float>("m_healthBuffer", "DT_TerrorPlayer")) <<
								" + grabbed] ";
						}
						else
						{
							// 生还者显示血量，临时血量
							ss << "[" << entity->GetHealth() << " + " << std::setprecision(0) <<
								(int)(entity->GetNetProp<float>("m_healthBuffer", "DT_TerrorPlayer")) << "] ";
						}
					}
					else
					{
						if (entity->GetNetProp<byte>("m_isGhost", "DT_TerrorPlayer") != 0)
						{
							// 幽灵状态的特感
							ss << "[" << entity->GetHealth() << " ghost] ";
						}
						else
						{
							// 非生还者只显示血量就好了
							ss << "[" << entity->GetHealth() << "] ";
						}
					}

					// 玩家类型
					ss << GetZombieClassName(entity);

					float height = fabs(head.y - foot.y);
					float width = height * 0.65f;

					// 根据情况决定颜色
					if ((classId == ET_SURVIVORBOT || classId == ET_CTERRORPLAYER) &&
						entity->GetNetProp<byte>("m_bIsOnThirdStrike", "DT_TerrorPlayer") != 0)
					{
						// 生还者黑白时使用白色
						color = DrawManager::WHITE;
					}
					else if ((classId == ET_BOOMER || classId == ET_SMOKER || classId == ET_HUNTER ||
						classId == ET_SPITTER || classId == ET_CHARGER || classId == ET_JOCKEY) &&
						entity->GetNetProp<byte>("m_isGhost", "DT_TerrorPlayer") != 0)
					{
						// 幽灵状态的特感，紫色
						color = DrawManager::PURPLE;
					}
					else
					{
						// 其他情况，橙色
						color = DrawManager::ORANGE;
					}

					/*
					g_cInterfaces.Surface->drawString(foot.x - width / 2, head.y,
						(color & 0xFF0000) >> 16, (color & 0xFF00) >> 8, color & 0xFF,
						font, Utils::c2w(ss.str()).c_str());
					*/

					g_pDrawRender->AddText(color, foot.x, head.y, true, ss.str().c_str());

					ss.str("");

					// 显示距离
					ss << std::setprecision(0) << (int)dist;

					// 给生还者显示弹药
					if (classId == ET_SURVIVORBOT || classId == ET_CTERRORPLAYER)
					{
						CBaseEntity* weapon = (CBaseEntity*)entity->GetActiveWeapon();
						if (weapon != nullptr)
							weapon = g_cInterfaces.ClientEntList->GetClientEntityFromHandle((CBaseHandle*)weapon);
						if (weapon != nullptr)
						{
							int ammoType = weapon->GetNetProp<int>("m_iPrimaryAmmoType", "DT_BaseCombatWeapon");
							int clip = weapon->GetNetProp<int>("m_iClip1", "DT_BaseCombatWeapon");
							byte reloading = weapon->GetNetProp<byte>("m_bInReload", "DT_BaseCombatWeapon");

							// 显示弹药和弹夹
							if (ammoType > 0 && clip > -1)
							{
								if (reloading != 0)
								{
									// 正在换子弹
									ss << " (reloading)";
								}
								else
								{
									// 没有换子弹
									ss << " (" << clip << " / " <<
										entity->GetNetProp<int>("m_iAmmo", "DT_TerrorPlayer", (size_t)ammoType) <<
										")";
								}
							}
						}
					}

					color = (visible ? DrawManager::BLUE : DrawManager::YELLOW);

					/*
					g_cInterfaces.Surface->drawString(foot.x - width / 2, head.y + FontSize,
						(color & 0xFF0000) >> 16, (color & 0xFF00) >> 8, color & 0xFF,
						font, Utils::c2w(ss.str()).c_str());
					*/

					g_pDrawRender->AddText(color, foot.x, head.y +
						g_pDrawRender->GetFontSize(), true, ss.str().c_str());

					// 绘制一个框（虽然这个框只有上下两条线）
					/*
					g_cInterfaces.Surface->drawBox(foot.x - width / 2, foot.y, width, -height, 1,
						(color & 0xFF0000) >> 16, (color & 0xFF00) >> 8, color & 0xFF,
						(color & 0xFF000000) >> 24);
					*/

					color = (entity->GetTeam() == team ? DrawManager::BLUE : DrawManager::RED);
					g_pDrawRender->AddRect(color, foot.x - width / 2, foot.y, width, -height);
				}
				else
				{
					// 这只是普感而已，太远了没必要显示出来
					if (dist > 2500.0f)
						continue;

					int size = 0;

					if (visible)
						size = 4;
					else
						size = 2;

					if (classId == ET_WITCH)
						color = DrawManager::PINK;
					else
						color = DrawManager::GREEN;

					// 画一个小方形，以标记为头部
					/*
					g_cInterfaces.Surface->FillRGBA(head.x, head.y, size, size,
						(color & 0xFF0000) >> 16, (color & 0xFF00) >> 8, color & 0xFF,
						(color & 0xFF000000) >> 24);
					*/
					g_pDrawRender->AddFillRect(color, head.x, head.y, size, size);
				}
			}

			if (g_bAimBot && (!targetSelected || !(g_pUserCommands->buttons & IN_ATTACK)) &&
				classId != ET_WITCH && (classId != ET_INFECTED || team == 2))
			{
				// 已经选择过目标了，并且这是一个不重要的敌人
				if (classId == ET_INFECTED && distmin < 65535.0f)
					continue;

				// 选择一个最接近的特感，因为特感越近对玩家来说越危险
				if (entity->GetTeam() != team && dist < distmin && visible &&
					GetAnglesFieldOfView(myViewAngles, CalculateAim(myEyeOrigin, headbox)) <= g_fAimbotFieldOfView)
				{
					g_pCurrentAiming = entity;
					distmin = dist;
				}
			}
		}

		if (g_bDrawCrosshairs)
		{
			int width, height;
			g_cInterfaces.Engine->GetScreenSize(width, height);

			int aiming = *(int*)(local + m_iCrosshairsId);
			CBaseEntity* target = (aiming > 0 ? g_cInterfaces.ClientEntList->GetClientEntity(aiming) : GetAimingTarget(-1));

#ifdef _DEBUG
			try
			{
#endif
				if (!IsValidEntity(target))
					target = nullptr;
#ifdef _DEBUG
			}
			catch (std::exception e)
			{
				Utils::log("%s (%d): %s | 0x%X", __FILE__, __LINE__, e.what(), (DWORD)target);
				target = nullptr;
			}
			catch (...)
			{
				Utils::log("%s (%d): 未知异常 | 0x%X", __FILE__, __LINE__, (DWORD)target);
				target = nullptr;
			}
#endif
			int classId = (target == nullptr ? ET_INVALID : target->GetClientClass()->m_ClassID);
			if (classId == ET_INVALID)
				color = DrawManager::LAWNGREEN;
			else if (target->GetTeam() == local->GetTeam())
				color = DrawManager::BLUE;
			else if (classId == ET_INFECTED)
				color = DrawManager::ORANGE;
			else if (classId == ET_WITCH)
				color = DrawManager::PINK;
			else
				color = DrawManager::RED;

			/*
			g_cInterfaces.Surface->drawCrosshair(width / 2, height / 2,
				(color & 0xFF0000) >> 16, (color & 0xFF00) >> 8, color & 0xFF);
			*/
			
			width /= 2;
			height /= 2;
			g_pDrawRender->AddLine(color, width - 10, height, width + 10, height);
			g_pDrawRender->AddLine(color, width, height - 10, width, height + 10);
		}
#endif
	}

#ifndef USE_D3D_DRAW
finish_draw:
#endif

	// ((FnPaintTraverse)g_cInterfaces.PanelHook->GetOriginalFunction(indexes::PaintTraverse))(ecx, panel, forcePaint, allowForce);
	oPaintTraverse(pPanel, panel, forcePaint, allowForce);
}

static bool* bSendPacket;
void __stdcall Hooked_CreateMove(int sequence_number, float input_sample_frametime, bool active)
{
	static bool showHint = true;
	if (showHint)
	{
		Utils::log("Hooked_CreateMove trigged.");
	}

	DWORD dwEBP = NULL;
	__asm mov dwEBP, ebp;
	bSendPacket = (bool*)(*(byte**)dwEBP - 0x21);

	oCreateMove(sequence_number, input_sample_frametime, active);

	CVerifiedUserCmd *pVerifiedCmd = &(*(CVerifiedUserCmd**)((DWORD)g_cInterfaces.Input + 0xE0))[sequence_number % 150];
	CUserCmd *pCmd = &(*(CUserCmd**)((DWORD_PTR)g_cInterfaces.Input + 0xDC))[sequence_number % 150];
	if (showHint)
	{
		showHint = false;
		Utils::log("Input->pVerifiedCmd = 0x%X", (DWORD)pVerifiedCmd);
		Utils::log("Input->pCmd = 0x%X", (DWORD)pCmd);
		Utils::log("CL_Move->bSendPacket = 0x%X | %d", (DWORD)bSendPacket, *bSendPacket);
	}

	CBaseEntity* client = GetLocalClient();
	if (client == nullptr || pCmd == nullptr || pVerifiedCmd == nullptr || !client->IsAlive() ||
		g_cInterfaces.Engine->IsConsoleVisible())
		return;

	float serverTime = GetServerTime();
	CBaseEntity* weapon = (CBaseEntity*)client->GetActiveWeapon();
	if (weapon != nullptr)
		weapon = g_cInterfaces.ClientEntList->GetClientEntityFromHandle((CBaseHandle*)weapon);
	else
		weapon = nullptr;

	float nextAttack = (weapon != nullptr ?
		weapon->GetNetProp<float>("m_flNextPrimaryAttack", "DT_BaseCombatWeapon") : FLT_MAX);
	
	int myTeam = client->GetTeam();
	int weaponId = (weapon != nullptr ? weapon->GetWeaponID() : 0);
	int flags = client->GetNetProp<int>("m_fFlags", "DT_BasePlayer");

	// 无法移动/无法转动视角修复
	if ((flags & FL_FREEZING) || (flags & FL_FROZEN))
	{
		// 删除无法移动和无法转动视角的标记
		flags &= ~(FL_FREEZING | FL_FROZEN);
		client->SetNetProp("m_fFlags", flags, "DT_BasePlayer");
	}

	// 自动连跳
	if (g_bAutoBunnyHop && (GetAsyncKeyState(VK_SPACE) & 0x8000))
	{
		static bool lastJump = false;
		static bool shouldFake = false;

		if (!lastJump && shouldFake)
		{
			shouldFake = false;
			pCmd->buttons |= IN_JUMP;
		}
		else if (pCmd->buttons & IN_JUMP)
		{
			if (flags & FL_ONGROUND)
			{
				lastJump = true;
				shouldFake = true;
			}
			else
			{
				pCmd->buttons &= ~IN_JUMP;
				lastJump = false;
			}
		}
		else
		{
			lastJump = false;
			shouldFake = false;
		}

		// 连跳自动旋转
		if (g_bAutoStrafe && !(flags & FL_ONGROUND))
		{
			if (pCmd->mousedx < 0)
				pCmd->sidemove = -400.f;

			if (pCmd->mousedx > 0)
				pCmd->sidemove = 400.f;
		}
	}

	// 启动声音修复
	// StartEnginePrediction(client, pCmd);
	// g_cInterfaces.Prediction->StartPrediction(pCmd);

	// 自动瞄准
	if (g_bAimBot && weapon != nullptr && (GetAsyncKeyState(VK_LBUTTON) & 0x8000))
	{
		Vector myOrigin = client->GetEyePosition(), myAngles = pCmd->viewangles;

		// 自动瞄准数据备份
		static QAngle oldViewAngles;
		static float oldSidemove;
		static float oldForwardmove;
		static float oldUpmove;

		bool runAimbot = false;

		// 目标在另一个地方选择
		if (g_pCurrentAiming != nullptr && IsGunWeapon(weaponId) && nextAttack <= serverTime)
		{
			// 鐩爣浣嶇疆
			Vector position;
			try
			{
				position = GetHeadHitboxPosition(g_pCurrentAiming);
			}
			catch (...)
			{
				if (g_pCurrentAiming->GetClientClass()->m_ClassID == ET_INFECTED)
					goto end_aimbot;

				// 鑾峰彇楠ㄩ浣嶇疆澶辫触
				position = g_pCurrentAiming->GetEyePosition();
				Utils::log("CBasePlayer::SetupBone error");

				// 鏍规嵁涓嶅悓鐨勬儏鍐电‘瀹氶珮搴
				int zombieClass = g_pCurrentAiming->GetNetProp<int>("m_zombieClass", "DT_TerrorPlayer");
				if (zombieClass == ZC_JOCKEY)
					position.z = g_pCurrentAiming->GetAbsOrigin().z + 30.0f;
				else if (zombieClass == ZC_HUNTER && (g_pCurrentAiming->GetFlags() & FL_DUCKING))
					position.z -= 12.0f;
			}

			// 速度预测，这个没做好
			// position += (g_pCurrentAiming->GetVelocity() * g_cInterfaces.Globals->interval_per_tick);

			if (position.IsValid())
			{
				// 备份原数据
				oldViewAngles = pCmd->viewangles;
				oldSidemove = pCmd->sidemove;
				oldForwardmove = pCmd->fowardmove;
				oldUpmove = pCmd->upmove;

				runAimbot = true;
				pCmd->viewangles = CalculateAim(myOrigin, position);
			}
		}
#ifdef _DEBUG_OUTPUT
		else
		{
			g_cInterfaces.Engine->ClientCmd("echo \"m_flNextPrimaryAttack = %.2f | serverTime = %.2f\"",
				nextAttack, serverTime);
			g_cInterfaces.Engine->ClientCmd("echo \"interval_per_tick = %.2f | m_nTickBase = %d\"",
				g_cInterfaces.Globals->interval_per_tick, client->GetTickBase());
		}
#endif

		if (g_bSilentAimbot)
		{
			if (!runAimbot)
			{
				// 还原数据
				*bSendPacket = true;

				if (oldViewAngles.IsValid())
				{
					pCmd->viewangles = oldViewAngles;
					// pCmd->sidemove = oldSidemove;
					// pCmd->fowardmove = oldForwardmove;
					// pCmd->upmove = oldUpmove;
				}
			}
			else
				*bSendPacket = false;
		}
	}

end_aimbot:

	// 自动开枪
	if (g_bTriggerBot && !(pCmd->buttons & IN_USE) && IsGunWeapon(weaponId))
	{
		CBaseEntity* target = GetAimingTarget();

#ifdef _DEBUG_OUTPUT
		if (target != nullptr)
		{
			if (!IsValidEntity(target))
				g_cInterfaces.Engine->ClientCmd("echo \"aiming dead 0x%X\"", (DWORD)target);
			if (target->GetTeam() == client->GetTeam())
				g_cInterfaces.Engine->ClientCmd("echo \"aiming team 0x%X\"", (DWORD)target);
			if (target->GetClientClass()->m_ClassID == ET_INFECTED)
				g_cInterfaces.Engine->ClientCmd("echo \"aiming infected 0x%X\"", (DWORD)target);
		}
#endif

#ifdef _DEBUG
		try
		{
#endif
			if (!IsValidEntity(target))
				goto end_trigger_bot;
#ifdef _DEBUG
		}
		catch (std::exception e)
		{
			Utils::log("%s (%d): %s | 0x%X", __FILE__, __LINE__, e.what(), (DWORD)target);
			goto end_trigger_bot;
		}
		catch (...)
		{
			Utils::log("%s (%d): 未知异常 | 0x%X", __FILE__, __LINE__, (DWORD)target);
			goto end_trigger_bot;
		}
#endif

		if (target->GetTeam() != myTeam && target->GetClientClass()->m_ClassID != ET_WITCH &&
			(myTeam == 2 || target->GetClientClass()->m_ClassID != ET_INFECTED))
			pCmd->buttons |= IN_ATTACK;
	}

end_trigger_bot:

	// 手枪连射
	if (g_bRapidFire && weapon != nullptr && (pCmd->buttons & IN_ATTACK))
	{
		static bool ignoreButton = true;
		if (IsWeaponSingle(weaponId) && !ignoreButton && nextAttack > serverTime)
		{
			pCmd->buttons &= ~IN_ATTACK;
			ignoreButton = true;
		}
		else
		{
			// 忽略一次阻止开枪
			ignoreButton = false;
		}
	}

	// 在开枪前检查，防止攻击队友
	if ((pCmd->buttons & IN_ATTACK) && IsGunWeapon(weaponId) && myTeam == 2)
	{
		int cid = client->GetCrosshairsId();
		CBaseEntity* aiming = (cid > 0 ? g_cInterfaces.ClientEntList->GetClientEntity(cid) : nullptr);

		if (aiming != nullptr && !aiming->IsDormant() && aiming->IsAlive() &&
			aiming->GetTeam() == myTeam && !IsNeedRescue(aiming) && !IsFallDown(client))
		{
			// 取消开枪
			pCmd->buttons &= ~IN_ATTACK;
		}
	}

	// 完成声音修复
	// EndEnginePrediction(client);
	// g_cInterfaces.Prediction->EndPrediction(pCmd);

	// 近战武器快速攻击
	if(myTeam == 2 && GetAsyncKeyState(VK_XBUTTON2) & 0x8000)
	{
		static enum FastMeleeStatus
		{
			FMS_None = 0,
			FMS_Primary = 1,
			FMS_Secondary = 2
		} fms = FMS_None;

		switch(fms)
		{
		case FMS_None:
			if (weaponId == Weapon_Melee && nextAttack <= serverTime)
			{
				// 近战武器攻击
				pCmd->buttons |= IN_ATTACK;
				fms = FMS_Primary;
			}
			break;
		case FMS_Primary:
			if (weaponId == Weapon_Melee && nextAttack > serverTime)
			{
				// 在攻击之后切换到主武器
				g_cInterfaces.Engine->ClientCmd("slot1");
				fms = FMS_Secondary;
			}
			break;
		case FMS_Secondary:
			if (weaponId != Weapon_Melee)
			{
				// 在主武器时切换到近战武器
				g_cInterfaces.Engine->ClientCmd("slot2");
				fms = FMS_None;
			}
			break;
		}
	}

	// 修复角度不正确
	ClampAngles(pCmd->viewangles);
	AngleNormalize(pCmd->viewangles);

	// 发送到服务器
	pVerifiedCmd->m_cmd = *pCmd;
	pVerifiedCmd->m_crc = pCmd->GetChecksum();

	// 将当前按钮保存到全局变量，用于检查一些东西
	g_pUserCommands = pCmd;
}

void __stdcall Hooked_FrameStageNotify(ClientFrameStage_t stage)
{
	static bool showHint = true;
	if (showHint)
	{
		showHint = false;
		Utils::log("Hooked_FrameStageNotify trigged.");
	}

	QAngle punch, velocity;
	CBaseEntity* client = GetLocalClient();

	if (stage == FRAME_RENDER_START && g_cInterfaces.Engine->IsInGame())
	{
		if (client != nullptr && client->IsAlive())
		{
			punch = client->GetNetProp<Vector>("m_vecPunchAngle", "DT_BasePlayer");
			velocity = client->GetNetProp<Vector>("m_vecPunchAngleVel", "DT_BasePlayer");
		}

		// 在这里可以使用 DebugOverlay 来进行 3D 绘制
	}

	oFrameStageNotify(stage);

	if (client != nullptr && client->IsAlive() && punch.IsValid() && velocity.IsValid())
	{
		// 去除屏幕晃动
		client->SetNetProp<Vector>("m_vecPunchAngle", punch, "DT_BasePlayer");
		client->SetNetProp<Vector>("m_vecPunchAngleVel", velocity, "DT_BasePlayer");
	}

	static time_t nextUpdate = 0;
	time_t currentTime = time(NULL);
	if (nextUpdate <= currentTime)
	{
		// 计时，用于每隔 1 秒触发一次
		nextUpdate = currentTime + 1;
		static DWORD fmWait = 45;
		static bool connected = false;

		if (g_cInterfaces.Engine->IsConnected())
		{
			// 修改 r_drawothermodels 实现简单的 esp
			if (GetAsyncKeyState(VK_INSERT) & 0x01)
			{
#ifdef USE_CVAR_CHANGE
				CVAR_MAKE_FLAGS("r_drawothermodels");
				CVAR_MAKE_FLAGS("cl_drawshadowtexture");
#endif

#ifdef USE_CVAR_CHANGE
				if (g_tConVar["r_drawothermodels"] != nullptr && g_tConVar["cl_drawshadowtexture"] != nullptr)
				{
					CVAR_MAKE_VALUE("r_drawothermodels", 2, 1);
					CVAR_MAKE_VALUE("cl_drawshadowtexture", 1, 0);
				}
				else
				{
#endif
					if (Utils::readMemory<int>(g_iClientBase + r_drawothermodels) == 2)
					{
						Utils::writeMemory(1, g_iClientBase + r_drawothermodels);
						Utils::writeMemory(0, g_iClientBase + cl_drawshadowtexture);
					}
					else
					{
						Utils::writeMemory(2, g_iClientBase + r_drawothermodels);
						Utils::writeMemory(1, g_iClientBase + cl_drawshadowtexture);
					}

					g_cInterfaces.Engine->ClientCmd("echo \"r_drawothermodels set %d\"",
						Utils::readMemory<int>(g_iClientBase + r_drawothermodels));
					g_cInterfaces.Engine->ClientCmd("echo \"cl_drawshadowtexture set %d\"",
						Utils::readMemory<int>(g_iClientBase + cl_drawshadowtexture));
#ifdef USE_CVAR_CHANGE
				}
#endif
				g_bDrawBoxEsp = !g_bDrawBoxEsp;

				if (g_pDrawRender != nullptr)
				{
					g_pDrawRender->PushRenderText(DrawManager::WHITE, "box esp %s",
						(g_bDrawBoxEsp ? "enable" : "disable"));
				}
			}

			// 修改 mat_fullbright 实现全图高亮
			if (GetAsyncKeyState(VK_HOME) & 0x01)
			{
#ifdef USE_CVAR_CHANGE
				CVAR_MAKE_FLAGS("mat_fullbright");
#endif

#ifdef USE_CVAR_CHANGE
				if (g_tConVar["mat_fullbright"] != nullptr)
				{
					CVAR_MAKE_VALUE("mat_fullbright", 1, 0);
				}
				else
				{
#endif
					if (Utils::readMemory<int>(g_iMaterialModules + mat_fullbright) == 1)
						Utils::writeMemory(0, g_iMaterialModules + mat_fullbright);
					else
						Utils::writeMemory(1, g_iMaterialModules + mat_fullbright);

					g_cInterfaces.Engine->ClientCmd("echo \"mat_fullbright set %d\"",
						Utils::readMemory<int>(g_iMaterialModules + mat_fullbright));
#ifdef USE_CVAR_CHANGE
				}
#endif

			}

			// 修改 mp_gamemode 在对抗模式开启第三人称
			if (GetAsyncKeyState(VK_PRIOR) & 0x01)
			{
#ifdef USE_CVAR_CHANGE
				CVAR_MAKE_FLAGS("mp_gamemode");
#endif

#ifdef USE_CVAR_CHANGE
				if (g_tConVar["mp_gamemode"] != nullptr)
				{
					const char* mode = g_tConVar["mp_gamemode"]->GetString();
					if (_strcmpi(mode, "versus") == 0 || _strcmpi(mode, "realismversus") == 0)
					{
						g_tConVar["mp_gamemode"]->SetValue("coop");
						strcpy_s(g_tConVar["mp_gamemode"]->m_pszString, g_tConVar["mp_gamemode"]->m_StringLength, "coop");
					}
					else
					{
						g_tConVar["mp_gamemode"]->SetValue("versus");
						strcpy_s(g_tConVar["mp_gamemode"]->m_pszString, g_tConVar["mp_gamemode"]->m_StringLength, "versus");
					}

					g_cInterfaces.Engine->ClientCmd("echo \"[ConVar] mp_gamemode set %s\"",
						g_tConVar["mp_gamemode"]->GetString());
				}
				else
				{
#endif
					char* mode = Utils::readMemory<char*>(g_iClientBase + mp_gamemode);
					if (mode != nullptr)
					{
						DWORD oldProtect = NULL;

						if (VirtualProtect(mode, sizeof(char) * 16, PAGE_EXECUTE_READWRITE, &oldProtect) == TRUE)
						{
							if (_strcmpi(mode, "versus") == 0 || _strcmpi(mode, "realismversus") == 0)
								strcpy_s(mode, 16, "coop");
							else
								strcpy_s(mode, 16, "versus");
							VirtualProtect(mode, sizeof(char) * 16, oldProtect, &oldProtect);
						}
						else
							printf("VirtualProtect 0x%X Fail!\n", (DWORD)mode);

						g_cInterfaces.Engine->ClientCmd("echo \"mp_gamemode set %s\"", mode);
					}
#ifdef USE_CVAR_CHANGE
				}
#endif

			}

			// 修改 sv_cheats 解除更改 cvar 限制
			if (GetAsyncKeyState(VK_APPS) & 0x01)
			{
#ifdef USE_CVAR_CHANGE
				CVAR_MAKE_FLAGS("sv_cheats");
#endif

#ifdef USE_CVAR_CHANGE
				if (g_tConVar["sv_cheats"] != nullptr)
				{
					g_tConVar["sv_cheats"]->SetValue(1);
					g_tConVar["sv_cheats"]->m_fValue = 1.0f;
					g_tConVar["sv_cheats"]->m_nValue = 1;
					g_cInterfaces.Engine->ClientCmd("echo \"[ConVar] sv_cheats set %d\"",
						g_tConVar["sv_cheats"]->GetInt());
				}
#endif

				if (Utils::readMemory<int>(g_iEngineBase + sv_cheats) != 1)
				{
					Utils::writeMemory(1, g_iEngineBase + sv_cheats);
					g_cInterfaces.Engine->ClientCmd("echo \"sv_cheats set %d\"",
						Utils::readMemory<int>(g_iEngineBase + sv_cheats));
				}
			}

			// 切换第三人称
			if (GetAsyncKeyState(VK_NEXT) & 0x01)
				thirdPerson();

			// 显示全部玩家
			/*
			if (GetAsyncKeyState(VK_CAPITAL) & 0x01)
			showSpectator();
			*/

			// 打开/关闭 自动连跳的自动保持速度
			if (GetAsyncKeyState(VK_F8) & 0x01)
			{
				g_bAutoStrafe = !g_bAutoStrafe;
				g_cInterfaces.Engine->ClientCmd("echo \"[segt] auto strafe set %s\"",
					(g_bAutoStrafe ? "enable" : "disabled"));

				if (g_pDrawRender != nullptr)
				{
					g_pDrawRender->PushRenderText(DrawManager::WHITE, "auto strafe %s",
						(g_bAutoStrafe ? "enable" : "disable"));
				}
			}

			// 打开/关闭 自动开枪
			if (GetAsyncKeyState(VK_F9) & 0x01)
			{
				g_bTriggerBot = !g_bTriggerBot;
				g_cInterfaces.Engine->ClientCmd("echo \"[segt] trigger bot set %s\"",
					(g_bTriggerBot ? "enable" : "disabled"));

				if (g_pDrawRender != nullptr)
				{
					g_pDrawRender->PushRenderText(DrawManager::WHITE, "trigger bot %s",
						(g_bTriggerBot ? "enable" : "disable"));
				}
			}

			// 打开/关闭 自动瞄准
			if (GetAsyncKeyState(VK_F10) & 0x01)
			{
				g_bAimBot = !g_bAimBot;
				g_cInterfaces.Engine->ClientCmd("echo \"[segt] aim bot set %s\"",
					(g_bAimBot ? "enable" : "disabled"));

				if (g_pDrawRender != nullptr)
				{
					g_pDrawRender->PushRenderText(DrawManager::WHITE, "auto aim %s",
						(g_bAimBot ? "enable" : "disable"));
				}
			}

			// 打开/关闭 空格自动连跳
			if (GetAsyncKeyState(VK_F11) & 0x01)
			{
				g_bAutoBunnyHop = !g_bAutoBunnyHop;
				g_cInterfaces.Engine->ClientCmd("echo \"[segt] auto bunnyHop set %s\"",
					(g_bAutoBunnyHop ? "enable" : "disabled"));

				if (g_pDrawRender != nullptr)
				{
					g_pDrawRender->PushRenderText(DrawManager::WHITE, "auto bhop %s",
						(g_bAutoBunnyHop ? "enable" : "disable"));
				}
			}

			// 打开/关闭 静音瞄准
			if (GetAsyncKeyState(VK_F12) & 0x01)
			{
				g_bSilentAimbot = !g_bSilentAimbot;
				g_cInterfaces.Engine->ClientCmd("echo \"[segt] silent aim %s\"",
					(g_bSilentAimbot ? "enable" : "disabled"));

				if (g_pDrawRender != nullptr)
				{
					g_pDrawRender->PushRenderText(DrawManager::WHITE, "silent aimbot %s",
						(g_bSilentAimbot ? "enable" : "disable"));
				}
			}

			// 去除 CRC 验证
			// if (g_cInterfaces.Engine->IsConnected())
			{
#ifdef USE_CVAR_CHANGE
				CVAR_MAKE_FLAGS("sv_pure");
				CVAR_MAKE_FLAGS("sv_consistency");
				CVAR_MAKE_FLAGS("c_thirdpersonshoulder");
#endif

#ifdef USE_CVAR_CHANGE
				if (g_tConVar["sv_pure"] != nullptr && g_tConVar["sv_pure"]->GetInt() != 0)
					g_tConVar["sv_pure"]->SetValue(0);
				if (g_tConVar["sv_consistency"] != nullptr && g_tConVar["sv_consistency"]->GetInt() != 0)
					g_tConVar["sv_consistency"]->SetValue(0);
#endif

				if (Utils::readMemory<int>(g_iEngineBase + sv_pure) != 0 ||
					Utils::readMemory<int>(g_iEngineBase + sv_consistency) != 0)
				{
					Utils::writeMemory(0, g_iEngineBase + sv_pure);
					Utils::writeMemory(0, g_iEngineBase + sv_consistency);

					g_cInterfaces.Engine->ClientCmd("echo \"sv_pure and sv_consistency set %d\"",
						Utils::readMemory<int>(g_iEngineBase + sv_pure));
				}
			}

			if (!connected)
			{
				g_pGameRulesProxy = nullptr;
				g_cInterfaces.Engine->ClientCmd("echo \"********* connected *********\"");
				Utils::log("*** connected ***");
			}

			connected = true;
		}
		else if (connected && !g_cInterfaces.Engine->IsInGame())
		{
			connected = false;
			g_pGameRulesProxy = nullptr;
			Utils::log("*** disconnected ***");
		}

		if (GetAsyncKeyState(VK_ADD) & 0x8000)
		{
			g_cInterfaces.Engine->ClientCmd("alias fastmelee_loop \"+attack; slot1; wait 1; -attack; slot2; wait %d; fastmelee_launcher\"", ++fmWait);
			g_cInterfaces.Engine->ClientCmd("echo \"fastmelee wait set %d\"", fmWait);

			if (g_pDrawRender != nullptr)
				g_pDrawRender->PushRenderText(DrawManager::WHITE, "fast melee wait tick set %d", fmWait);
		}

		if (GetAsyncKeyState(VK_SUBTRACT) & 0x8000)
		{
			g_cInterfaces.Engine->ClientCmd("alias fastmelee_loop \"+attack; slot1; wait 1; -attack; slot2; wait %d; fastmelee_launcher\"", --fmWait);
			g_cInterfaces.Engine->ClientCmd("echo \"fastmelee wait set %d\"", fmWait);

			if (g_pDrawRender != nullptr)
				g_pDrawRender->PushRenderText(DrawManager::WHITE, "fast melee wait tick set %d", fmWait);
		}

		if (GetAsyncKeyState(VK_MULTIPLY) & 0x8000)
		{
			g_fAimbotFieldOfView += 1.0f;
			g_cInterfaces.Engine->ClientCmd("echo \"aimbot fov set %.2f\"", g_fAimbotFieldOfView);

			if (g_pDrawRender != nullptr)
				g_pDrawRender->PushRenderText(DrawManager::WHITE, "aimbot angles set %.0f", g_fAimbotFieldOfView);
		}

		if (GetAsyncKeyState(VK_DIVIDE) & 0x8000)
		{
			g_fAimbotFieldOfView -= 1.0f;
			g_cInterfaces.Engine->ClientCmd("echo \"aimbot fov set %.2f\"", g_fAimbotFieldOfView);

			if (g_pDrawRender != nullptr)
				g_pDrawRender->PushRenderText(DrawManager::WHITE, "aimbot angles set %.0f", g_fAimbotFieldOfView);
		}

		static byte iExitGame = 0;
		if (GetAsyncKeyState(VK_END) & 0x8000)
		{
			// 按住 End 键三秒强制退出游戏游戏，用于游戏无响应
			if (++iExitGame >= 3)
				ExitProcess(0);

			if (g_pDrawRender != nullptr)
				g_pDrawRender->PushRenderText(DrawManager::WHITE, "force exit proccess timer (%d/3)", iExitGame);
		}
		else if (iExitGame != 0)
		{
			// 按住不足 3 秒就放开则取消计时
			iExitGame = 0;
			if (g_pDrawRender != nullptr)
				g_pDrawRender->PushRenderText(DrawManager::WHITE, "force exit proccess timer stopped");
		}

		if (GetAsyncKeyState(VK_DELETE) & 0x01)
			g_cInterfaces.Engine->ClientCmd("disconnect");
	}
}

void __fastcall Hooked_RunCommand(void* ecx, void* edx, CBaseEntity* pEntity, CUserCmd* pCmd, CMoveHelper* moveHelper)
{
	oRunCommand(ecx, pEntity, pCmd, moveHelper);

	static bool showHint = true;
	if (showHint || g_cInterfaces.MoveHelper == nullptr)
	{
		showHint = false;
		Utils::log("MoveHelperPointer = 0x%X", (DWORD)moveHelper);
	}

	g_cInterfaces.MoveHelper = moveHelper;
}

bool __stdcall Hooked_CreateMoveShared(float flInputSampleTime, CUserCmd* cmd)
{
	static bool showHint = true;
	if (showHint)
	{
		showHint = false;
		std::cout << "Hooked_CreateMoveShared trigged." << std::endl;
		Utils::log("Hooked_CreateMoveShared success");
	}

	CBaseEntity* client = GetLocalClient();

	if (!client || !cmd)
		return false;

	return oCreateMoveShared(flInputSampleTime, cmd);
}

int __stdcall Hooked_InKeyEvent(int eventcode, ButtonCode_t keynum, const char *pszCurrentBinding)
{
	return oInKeyEvent(eventcode, keynum, pszCurrentBinding);
}

void __stdcall Hooked_DrawModel(PVOID context, PVOID state, const ModelRenderInfo_t& pInfo, matrix3x4_t* pCustomBoneToWorld)
{
	oDrawModel(context, state, pInfo, pCustomBoneToWorld);
}

bool __stdcall Hooked_DispatchUserMessage(int msg_id, bf_read* msg_data)
{
	static bool showHint = true;
	if (showHint)
	{
		showHint = false;
		Utils::log("Hooked_DispatchUserMessage trigged.");
	}
	
	return oDispatchUserMessage(msg_id, msg_data);
}

// __usercall
__declspec(naked) void _CL_Move_Hooker()
{
	__asm
	{
		push	ebp
		mov		ebp, esp
		
		// 参数传递...
		push	word ptr [ebp + 0x0C]	// bFinalTick	push 不支持 byte
		push	dword ptr [ebp + 0x08]	// accumulated_extra_samples
		fld		st(0)	// double
		push	edi		// int
		push	bx		// byte			push 不支持 byte

		// 调用自己的 Hook 函数
		call	Hooked_CL_Move

		// __usercall 相当于 __cdecl
		ret
	}
}

void __stdcall Hooked_CL_Move(byte toBL, int toEDI, double toST0, float accumulated_extra_samples, bool bFinalTick)
{
	// 把全局的 oCL_Move 修改一下
	auto CL_Move = [&]() -> int
	{
		WORD wFinalTick = bFinalTick;
		
		// 3 个字节
		typedef struct _TBYTE
		{
			WORD b[3];
		} PTBYTE, TBYTE;
		
		TBYTE* ToST0 = new TBYTE();
		ZeroMemory(ToST0, sizeof(TBYTE));

		float flTmp = (float)toST0;
		byte* tmp = (byte*)(&flTmp);
		memcpy_s(ToST0, sizeof(TBYTE), (&tmp) + sizeof(TBYTE) - 3, 3);

		__asm
		{
			// 寄存器传参
			mov		bl, toBL
			mov		edi, toEDI
			fbld	tbyte ptr ToST0

			// 堆栈传参
			push	wFinalTick		// 不支持 push byte
			push	accumulated_extra_samples

			// 调用原函数
			call	oCL_Move

			// 清理堆栈
			add		esp, 6
		}

	};

	// 默认的 1 次调用，如果不调用会导致游戏冻结
	CL_Move();

	if (g_pUserCommands != nullptr && g_pUserCommands->buttons & IN_SPEED)
	{
		for (int i = 1; i < g_iSpeedMultiple; ++i)
		{
			// 多次调用它会有加速效果
			CL_Move();
		}
	}

}

bool __cdecl Hooked_IsInDebugSession()
{
	return false;
}

void __cdecl Hooked_VGUIPaint()
{
	static bool showHint = false;
	if (showHint)
	{
		showHint = false;
		Utils::log("Hooked_VGUIPaint trigged");
	}
	
	oVGUIPaint();

	// 在这里绘制东西，这个比 PaintTraverse 更好
	// 因为 PaintTraverse 会降低游戏的 fps 30~40 左右
}

__declspec(naked) void _DrawModelExecute_Hooker()
{
	__asm
	{
		push	ebp
		mov		ebp, esp

		// 普通参数传递
		push	dword ptr [ebp + 0x10]
		push	dword ptr [ebp + 0x0C]
		push	dword ptr [ebp + 0x08]

		// 寄存器参数传递
		push	ebx
		push	ecx

		// 调用 Hook 函数
		call	Hooked_DrawModelExecute

		// 返回
		ret
	}
}

void __stdcall Hooked_DrawModelExecute(int ECX, int EBX, const DrawModelState_t &state,
	const ModelRenderInfo_t &pInfo, matrix3x4_t *pBoneToWorld)
{
	auto DrawModelExecute = [&]() -> void
	{
		__asm
		{
			// 寄存器传递参数
			mov		ebx, EBX
			mov		ecx, ECX

			// 堆栈传递参数
			push	pBoneToWorld
			push	dword ptr pInfo
			push	dword ptr state

			// 调用原函数
			call	oDrawModelExecute;
		}
	};

	DrawModelExecute();

	g_cInterfaces.ModelRender->ForcedMaterialOverride(nullptr);
}
