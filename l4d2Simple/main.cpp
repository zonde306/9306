#include "main.h"
#include "peb.h"
#include "./sqbinding/vector.h"
#include "./sqbinding/qangle.h"

#define USE_PLAYER_INFO
#define USE_CVAR_CHANGE
// #define USE_D3D_DRAW

// D3D 的函数 jmp 挂钩
static std::unique_ptr<DetourXS> g_pDetourReset, g_pDetourPresent, g_pDetourEndScene,
	g_pDetourDrawIndexedPrimitive, g_pDetourCreateQuery, g_pDetourCL_Move, g_pDetourDebugger,
	g_pDetourCreateMove, g_pDetourGetCvarValue, g_pDetourSetConVar;

std::unique_ptr<CNetVars> g_pNetVars;
CInterfaces g_interface;
std::string g_sCurPath;

CCRC gCRC;
CVMT VMT;

// D3D Device 虚表挂钩
static std::unique_ptr<CVMTHookManager> g_pVMTDevice;

// D3D EndScene 绘制工具
std::unique_ptr<DrawManager> g_pDrawRender;

// D3D9 Device 钩子
static std::unique_ptr<D3D9Hooker> g_pDeviceHooker;

DWORD WINAPI StartCheat(LPVOID params);

extern "C" __declspec(dllexport)
LRESULT WINAPI InitWndProc(int code, WPARAM wparam, LPARAM lparam)
{
	return CallNextHookEx(nullptr, code, wparam, lparam);
}

_se_translator_function g_pfnOldSeHandler;
_invalid_parameter_handler g_pfnOldParamHandler;
_purecall_handler g_pfnOldPurecallHander;

BOOL WINAPI DllMain(HINSTANCE module, DWORD reason, LPVOID reserved)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		char curPath[MAX_PATH];
		GetModuleFileNameA(GetModuleHandleA(NULL), curPath, MAX_PATH);
		std::string strPath = curPath;
		if (strPath.empty() ||
			_stricmp(strPath.substr(strPath.rfind('\\') + 1).c_str(), "left4dead2.exe") != 0)
		{
			// 不是目标进程，不做任何事情
			OutputDebugStringW(L"当前进程不是目标进程");
			return FALSE;
		}

		char buffer[MAX_PATH];
		GetModuleFileNameA(module, buffer, MAX_PATH);
		std::string tmp = buffer;
		g_sCurPath = tmp.substr(0, tmp.rfind('\\'));

		DisableThreadLibraryCalls(module);
		// HideDll(module);

		// se异常捕获器
		g_pfnOldSeHandler = _set_se_translator([](unsigned int expCode, EXCEPTION_POINTERS* pExp) -> void
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

			/*
			MyStackWalker sw;
			sw.ShowCallstack(GetCurrentThread(), pExp->ContextRecord);
			*/

			Utils::log(expInfo.str().c_str());
			// throw std::exception(expInfo.str().c_str());

			if(g_pfnOldSeHandler)
				g_pfnOldSeHandler(expCode, pExp);
		});

		// 异常捕获器带参数
		g_pfnOldParamHandler = _set_invalid_parameter_handler([](const wchar_t* expression,
			const wchar_t* function, const wchar_t* file, unsigned int line, uintptr_t pReserved) -> void
		{
			std::wstringstream expInfo;
#ifndef _DEBUG
			expInfo << L"请使用 DEBUG 模式编译以进行调试";
#else
			expInfo << L"[IP] 普通异常：" << std::endl << L"\t信息：" << expression << std::endl <<
				L"\t文件：" << file << L" (" << line << L")" << std::endl << L"\t函数：" << function;
#endif

			/*
			MyStackWalker sw;
			sw.ShowCallstack();
			*/

			Utils::log(expInfo.str().c_str());
			// throw std::exception(Utils::w2c(expInfo.str()).c_str());

			if(g_pfnOldParamHandler)
				g_pfnOldParamHandler(expression, function, file, line, pReserved);
		});

		// 虚函数调用异常捕获
		g_pfnOldPurecallHander = _set_purecall_handler([]() -> void
		{
			/*
			MyStackWalker sw;
			sw.ShowCallstack();
			*/

			Utils::log("未知虚函数异常");
			// throw std::exception("未知虚函数调用异常");

			if(g_pfnOldPurecallHander)
				g_pfnOldPurecallHander();
		});

		CreateThread(NULL, NULL, StartCheat, module, NULL, NULL);
	}
	else if (reason == DLL_PROCESS_DETACH)
	{
		// 到这里时可能是因为主动退出游戏，内存已经释放掉了
		try
		{
			DETOURXS_DESTORY(g_pDetourReset);
			DETOURXS_DESTORY(g_pDetourPresent);
			DETOURXS_DESTORY(g_pDetourEndScene);
			DETOURXS_DESTORY(g_pDetourDrawIndexedPrimitive);
			DETOURXS_DESTORY(g_pDetourCreateQuery);
			DETOURXS_DESTORY(g_pDetourDebugger);
			DETOURXS_DESTORY(g_pDetourCL_Move);
			DETOURXS_DESTORY(g_pDetourCreateMove);
			DETOURXS_DESTORY(g_pDetourGetCvarValue);
			DETOURXS_DESTORY(g_pDetourSetConVar);

			VMTHOOK_DESTORY(g_interface.ClientModeHook);
			VMTHOOK_DESTORY(g_interface.PanelHook);
			VMTHOOK_DESTORY(g_interface.ClientHook);
			VMTHOOK_DESTORY(g_interface.PredictionHook);
			VMTHOOK_DESTORY(g_interface.ModelRenderHook);
			VMTHOOK_DESTORY(g_interface.GameEventHook);
			VMTHOOK_DESTORY(g_pVMTDevice);
		}
		catch (std::exception& e)
		{
			std::cout << e.what() << std::endl;
		}
		catch (...)
		{
			__asm nop;
		}
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
typedef void(__thiscall* FnPaintTraverse)(CPanel*, unsigned int, bool, bool);
void __fastcall Hooked_PaintTraverse(CPanel*, void*, unsigned int, bool, bool);
static FnPaintTraverse oPaintTraverse;

typedef bool(__thiscall* FnCreateMoveShared)(ClientModeShared*, float, CUserCmd*);
bool __fastcall Hooked_CreateMoveShared(ClientModeShared*, void*, float, CUserCmd*);
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

typedef void(__thiscall* FnRunCommand)(CPrediction*, CBaseEntity*, CUserCmd*, CMoveHelper*);
void __fastcall Hooked_RunCommand(CPrediction*, void*, CBaseEntity*, CUserCmd*, CMoveHelper*);
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

typedef void(__thiscall* FnEnginePaint)(CEngineVGui*, PaintMode_t);
void __fastcall Hooked_EnginePaint(CEngineVGui*, void*, PaintMode_t);
FnEnginePaint oEnginePaint;

typedef bool(__thiscall* FnEngineKeyEvent)(CEngineVGui*, const InputEvent_t&);
bool __fastcall Hooked_EngineKeyEvent(CEngineVGui*, void*, const InputEvent_t&);
FnEngineKeyEvent oEngineKeyEvent;

typedef void(__stdcall* FnDrawModelExecute)(const DrawModelState_t&, const ModelRenderInfo_t&, matrix3x4_t*);
void __stdcall Hooked_DrawModelExecute(const DrawModelState_t&, const ModelRenderInfo_t&, matrix3x4_t*);
FnDrawModelExecute oDrawModelExecute;

// -------------------------------- General Function --------------------------------
typedef void(__cdecl* FnConColorMsg)(class Color const&, char const*, ...);
static FnConColorMsg PrintToConsoleColor;	// 打印信息到控制台（支持颜色）

typedef void(__cdecl* FnConMsg)(char const*, ...);
static FnConMsg PrintToConsole;				// 打印信息到控制台

typedef void(__cdecl* FnCL_Move)(float, bool);
void __stdcall Hooked_CL_Move(float, bool);
FnCL_Move oCL_Move;							// 玩家数据处理

typedef void(__cdecl* FnSharedRandomFloat)(const char*, float, float, int);
FnSharedRandomFloat SharedRandomFloat;		// 随机数

typedef int(__stdcall* FnTraceLine)(void* ebp, void* esi, const Vector&, const Vector&, unsigned int mask,
	const IHandleEntity*, int, trace_t*);
FnTraceLine UTIL_TraceRay;							// 光线跟踪

typedef const char*(__cdecl* FnWeaponIdToAlias)(unsigned int);
FnWeaponIdToAlias WeaponIdToAlias;					// 获取武器的名字

typedef bool(__thiscall* FnUserMessagesDispatch)(void*, int, bf_read&);
FnUserMessagesDispatch DispatchUserMessage;			// 用户消息

typedef bool(__cdecl* FnIsInDebugSession)();
bool __cdecl Hooked_IsInDebugSession();
FnIsInDebugSession oPlat_IsInDebugSession;

typedef void*(__thiscall* FnGetWpnData)(CBaseEntity*);
FnGetWpnData g_pCallGetWpnData;						// 获取武器数据 (不是虚函数)

typedef void(__thiscall* FnStartDrawing)(CSurface*);
FnStartDrawing PaintStartDrawing;					// 开始绘制 (在 CEngineVGui::Paint 里绘制必须先调用这个)

typedef void(__thiscall* FnFinishDrawing)(CSurface*);
FnFinishDrawing PaintFinishDrawing;					// 完成绘制 (在 CEngineVGui::Paint 里绘制结束时必须调用这个)

typedef int(__cdecl* FnSetPredictionRandomSeed)(int);
FnSetPredictionRandomSeed SetPredictionRandomSeed;	// 设置预测随机数种子

typedef CInput*(__thiscall* FnGetCurInput)(CInput*, int somevalue);
FnGetCurInput GetCurrentInput;

typedef ClientModeShared*(__cdecl *FnGetClientMode)();
FnGetClientMode GetClientModeNormal;

typedef bool(__thiscall* FnProcessGetCvarValue)(CBaseClientState*, SVC_GetCvarValue*);
bool __fastcall Hooked_ProcessGetCvarValue(CBaseClientState*, void*, SVC_GetCvarValue*);
FnProcessGetCvarValue oProcessGetCvarValue;

typedef bool(__thiscall* FnProcessSetConVar)(CBaseClientState*, NET_SetConVar*);
bool __fastcall Hooked_ProcessSetConVar(CBaseClientState*, void*, NET_SetConVar*);
FnProcessSetConVar oProcessSetConVar;

typedef int(__thiscall* FnKeyInput)(ClientModeShared*, int, ButtonCode_t, const char*);
int __fastcall Hooked_KeyInput(ClientModeShared*, void*, int, ButtonCode_t, const char*);
FnKeyInput oKeyInput;

// -------------------------------- Cheats Function --------------------------------
void thirdPerson(bool);
void showSpectator();
void bindAlias(int);

// -------------------------------- Golbals Variable --------------------------------
static int g_iCurrentAiming = 0;										// 当前正在瞄准的玩家的 ID
static bool g_bNewFrame = false;										// 现在是否运行在新的一帧
static bool* g_pbSendPacket;											// 数据包是否发送到服务器
static int g_iSpeedMultiple = 5;										// 加速倍数
static CUserCmd* g_pUserCommands;										// 本地玩家当前按键
std::map<std::string, ConVar*> g_conVar;								// 控制台变量
static float g_fAimbotFieldOfView = 30.0f;								// 自动瞄准角度
CBaseEntity* g_pCurrentAimbot;											// 当前的自动瞄准目标
CBaseEntity* g_pCurrentAiming;											// 当前正在瞄准的目标
int g_iCurrentHitbox;													// 当前正在瞄准的敌人的 Hitbox
CBaseEntity* g_pGameRulesProxy;											// 游戏规则实体，在这里会有一些有用的东西
CBaseEntity* g_pPlayerResource;											// 玩家资源，可以用来获取某些东西
DWORD g_iClientBase, g_iEngineBase, g_iMaterialModules;					// 有用的 DLL 文件地址
int* g_pPredictionRandomSeed;											// 随机数种子
std::map<std::string, std::string> g_serverConVar;						// 来自于服务器的 ConVar
VMatrix* g_pWorldToScreenMatrix;										// 屏幕绘制用的

std::string GetZombieClassName(CBaseEntity* player);
bool IsValidVictim(CBaseEntity* entity);

DWORD WINAPI StartCheat(LPVOID params)
{
	// 获取所有接口
	g_interface.GetInterfaces();

	// 初始化 NetProp 表
	g_pNetVars = std::make_unique<CNetVars>();
	
	g_iClientBase = Utils::GetModuleBase("client.dll");
	g_iEngineBase = Utils::GetModuleBase("engine.dll");
	g_iMaterialModules = Utils::GetModuleBase("materialsystem.dll");
	DWORD vgui = Utils::GetModuleBase("vguimatsurface.dll");

	Utils::log("client.dll 0x%X", g_iClientBase);
	Utils::log("engine.dll 0x%X", g_iEngineBase);
	Utils::log("materialsystem.dll 0x%X", g_iMaterialModules);
	Utils::log("VEngineClient 0x%X", (DWORD)g_interface.Engine);
	Utils::log("EngineTraceClient 0x%X", (DWORD)g_interface.Trace);
	Utils::log("VClient 0x%X", (DWORD)g_interface.Client);
	Utils::log("VClientEntityList 0x%X", (DWORD)g_interface.ClientEntList);
	Utils::log("VModelInfoClient 0x%X", (DWORD)g_interface.ModelInfo);
	Utils::log("VGUI_Panel 0x%X", (DWORD)g_interface.Panel);
	Utils::log("VGUI_Surface 0x%X", (DWORD)g_interface.Surface);
	Utils::log("PlayerInfoManager 0x%X", (DWORD)g_interface.PlayerInfo);
	Utils::log("VClientPrediction 0x%X", (DWORD)g_interface.Prediction);
	Utils::log("GameMovement 0x%X", (DWORD)g_interface.GameMovement);
	Utils::log("VDebugOverlay 0x%X", (DWORD)g_interface.DebugOverlay);
	Utils::log("GameEventsManager 0x%X", (DWORD)g_interface.GameEvent);
	Utils::log("VEngineModel 0x%X", (DWORD)g_interface.ModelRender);
	Utils::log("VEngineRenderView 0x%X", (DWORD)g_interface.RenderView);
	Utils::log("VEngineCvar 0x%X", (DWORD)g_interface.Engine);
	Utils::log("GlobalsVariable 0x%X", (DWORD)g_interface.Globals);
	Utils::log("InputSystem 0x%X", (DWORD)g_interface.InputSystem);
	Utils::log("MaterialSystem 0x%X", (DWORD)g_interface.MaterialSystem);
	Utils::log("Input 0x%X", (DWORD)g_interface.Input);
	Utils::log("UserMessages 0x%X", (DWORD)g_interface.UserMessage);
	Utils::log("MoveHelper 0x%X", (DWORD)g_interface.MoveHelper);

	// 这个好像是不正确的...
	g_interface.MoveHelper = nullptr;
	g_interface.ClientMode = nullptr;
	g_interface.ClientState = nullptr;
	// sqb::Initialization();

	if ((oCL_Move = (FnCL_Move)Utils::FindPattern("engine.dll",
		XorStr("55 8B EC B8 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 33 C5 89 45 FC 53 56 57 E8"))) != nullptr)
	{
		g_pbSendPacket = (bool*)((DWORD)oCL_Move + 0x91);
		Utils::log("CL_Move = engine.dll + 0x%X | bSendPacket = 0x%X",
			(DWORD)oCL_Move - g_iEngineBase, (DWORD)g_pbSendPacket);

		g_pDetourCL_Move = std::make_unique<DetourXS>(oCL_Move, Hooked_CL_Move);
		oCL_Move = (FnCL_Move)g_pDetourCL_Move->GetTrampoline();
		Utils::log("Trampoline oCL_Move = 0x%X", (DWORD)oCL_Move);
	}
	else
		Utils::log("CL_Move not found");

	if ((SetPredictionRandomSeed = (FnSetPredictionRandomSeed)Utils::FindPattern("client.dll",
		XorStr("55 8B EC 8B 45 08 85 C0 75 0C"))) != nullptr)
	{
		Utils::log("SetPredictionRandomSeed found client.dll + 0x%X", (DWORD)SetPredictionRandomSeed - g_iClientBase);
		g_pPredictionRandomSeed = *(int**)((DWORD)SetPredictionRandomSeed + 0x1A);

		for (DWORD i = 0; i <= 0x1F; ++i)
		{
			// A3 18 68 6A 00		mov g_pPredictionRandomSeed, eax
			if (*(PBYTE)((DWORD)g_pPredictionRandomSeed + i) == 0xA3)
			{
				g_pPredictionRandomSeed = *(int**)(g_pPredictionRandomSeed + i + 1);
				Utils::log("g_pPredictionRandomSeed found 0x%X", (DWORD)g_pPredictionRandomSeed);
				break;
			}
		}

		Utils::log("SetPredictionRandomSeed::g_pPredictionRandomSeed = 0x%X", (DWORD)g_pPredictionRandomSeed);
	}
	else
		Utils::log("SetPredictionRandomSeed not found");

	if ((SharedRandomFloat = (FnSharedRandomFloat)Utils::FindPattern("client.dll",
		XorStr("55 8B EC 83 EC 08 A1 ? ? ? ? 53 56 57 8B 7D 14 8D 4D 14 51 89 7D F8 89 45 FC E8 ? ? ? ? 6A 04 8D 55 FC 52 8D 45 14 50 E8 ? ? ? ? 6A 04 8D 4D F8 51 8D 55 14 52 E8 ? ? ? ? 8B 75 08 56 E8 ? ? ? ? 50 8D 45 14 56 50 E8 ? ? ? ? 8D 4D 14 51 E8 ? ? ? ? 8B 15 ? ? ? ? 8B 5D 14 83 C4 30 83 7A 30 00 74 26 57 53 56 68 ? ? ? ? 68 ? ? ? ? 8D 45 14 68 ? ? ? ? 50 C7 45 ? ? ? ? ? FF 15 ? ? ? ? 83 C4 1C 53 B9 ? ? ? ? FF 15 ? ? ? ? D9 45 10"))) != nullptr)
	{
		Utils::log("SharedRandomFloat = client.dll + 0x%X", (DWORD)SharedRandomFloat - g_iClientBase);
		g_pPredictionRandomSeed = *(int**)((DWORD)SharedRandomFloat + 0x7);
		Utils::log("SharedRandomFloat::g_pPredictionRandomSeed = 0x%X", (DWORD)g_pPredictionRandomSeed);
	}
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

	if ((GetClientModeNormal = (FnGetClientMode)Utils::FindPattern("client.dll",
		XorStr("8B 0D ? ? ? ? 8B 01 8B 90 ? ? ? ? FF D2 8B 04 85 ? ? ? ? C3"))) != nullptr)
		Utils::log("GetClientMode = client.dll + 0x%X", (DWORD)GetClientModeNormal - g_iClientBase);
	else
		Utils::log("GetClientMode not found");

	if ((g_pCallGetWpnData = (FnGetWpnData)Utils::FindPattern("client.dll",
		XorStr("0F B7 81 ? ? ? ? 50 E8 ? ? ? ? 83 C4 04"))) != nullptr)
		Utils::log("CBaseCombatWeapon::GetWpnData found client.dll + 0x%X", (DWORD)g_pCallGetWpnData - g_iClientBase);
	else
		Utils::log("CBaseCombatWeapon::GetWpnData not found");

	if((PaintStartDrawing = (FnStartDrawing)Utils::FindPattern("vguimatsurface.dll", 
		XorStr("55 8B EC 6A FF 68 ? ? ? ? 64 A1 ? ? ? ? 50 83 EC 14 56 57 A1 ? ? ? ? 33 C5 50 8D 45 F4 64 A3 ? ? ? ? 8B F9 80 3D"))) != nullptr)
		Utils::log("CMatSystemSurface::StartDrawing found client.dll + 0x%X", (DWORD)PaintStartDrawing - vgui);
	else
		Utils::log("CMatSystemSurface::StartDrawing not found");

	if ((PaintFinishDrawing = (FnFinishDrawing)Utils::FindPattern("vguimatsurface.dll",
		XorStr("55 8B EC 6A FF 68 ? ? ? ? 64 A1 ? ? ? ? 50 51 56 A1 ? ? ? ? 33 C5 50 8D 45 F4 64 A3 ? ? ? ? 6A 00"))) != nullptr)
		Utils::log("CMatSystemSurface::FinishDrawing found client.dll + 0x%X", (DWORD)PaintFinishDrawing - vgui);
	else
		Utils::log("CMatSystemSurface::FinishDrawing not found");

	if((GetCurrentInput = (FnGetCurInput)Utils::FindPattern("client.dll",
		XorStr("55 8B EC 8B 45 08 56 8B F1 83 F8 FF 75 10 8B 0D ? ? ? ? 8B 01 8B 90 ? ? ? ? FF D2 69 C0 ? ? ? ? 8D 44 30 34 5E 5D C2 04 00"))) != nullptr)
		Utils::log("CInput::_GetCurInput = client.dll + 0x%X", (DWORD)GetCurrentInput - g_iClientBase);

	/*
	if ((oProcessGetCvarValue = (FnProcessGetCvarValue)Utils::FindPattern("engine.dll",
		XorStr("55 8B EC 81 EC ? ? ? ? A1 ? ? ? ? 33 C5 89 45 FC 53 56 57 8B 7D 08 8B 47 10"))) != nullptr)
	{
		Utils::log("CBaseClientState::ProcessGetCvarValue = engine.dll + 0x%X", (DWORD)oProcessGetCvarValue - g_iEngineBase);
		g_pDetourGetCvarValue = std::make_unique<DetourXS>(oProcessGetCvarValue, Hooked_ProcessGetCvarValue);
		oProcessGetCvarValue = (FnProcessGetCvarValue)g_pDetourGetCvarValue->GetTrampoline();
		Utils::log("Trampoline oProcessGetCvarValue = 0x%X", (DWORD)oProcessGetCvarValue);
	}
	else
		Utils::log("CBaseClientState::ProcessGetCvarValue not found");
	*/

	if ((oProcessSetConVar = (FnProcessSetConVar)Utils::FindPattern("engine.dll",
		XorStr("55 8B EC 8B 49 08 8B 01 8B 50 18"))) != nullptr)
	{
		Utils::log("CBaseClientState::ProcessSetConVar = engine.dll + 0x%X", (DWORD)oProcessSetConVar - g_iEngineBase);
		g_pDetourSetConVar = std::make_unique<DetourXS>(oProcessSetConVar, Hooked_ProcessSetConVar);
		oProcessSetConVar = (FnProcessSetConVar)g_pDetourSetConVar->GetTrampoline();
		Utils::log("Trampoline oProcessSetConVar = 0x%X", (DWORD)oProcessSetConVar);
	}
	else
		Utils::log("CBaseClientState::ProcessSetConVar not found");

	if ((oCreateMoveShared = (FnCreateMoveShared)Utils::FindPattern("client.dll",
		XorStr("55 8B EC 6A FF E8 ? ? ? ? 83 C4 04 85 C0 75 06 B0 01"))) != nullptr)
	{
		Utils::log("ClientModeShared::CreateMove = client.dll + 0x%X", (DWORD)oCreateMoveShared - g_iClientBase);
		g_pDetourCreateMove = std::make_unique<DetourXS>(oCreateMoveShared, Hooked_CreateMoveShared);
		oCreateMoveShared = (FnCreateMoveShared)g_pDetourCreateMove->GetTrampoline();
		Utils::log("Trampoline oCreateMoveShared = 0x%X", (DWORD)oCreateMoveShared);
	}
	
	if (g_interface.PanelHook && indexes::PaintTraverse > -1)
	{
		oPaintTraverse = (FnPaintTraverse)g_interface.PanelHook->HookFunction(indexes::PaintTraverse, Hooked_PaintTraverse);
		g_interface.PanelHook->HookTable(true);
		Utils::log("oPaintTraverse = 0x%X", (DWORD)oPaintTraverse);
	}

	if (g_interface.ClientModeHook && indexes::SharedCreateMove > -1)
	{
		oCreateMoveShared = (FnCreateMoveShared)g_interface.ClientModeHook->HookFunction(indexes::SharedCreateMove, Hooked_CreateMoveShared);
		// g_interface.ClientModeHook->HookTable(true);
		Utils::log("oCreateMoveShared = 0x%X", (DWORD)oCreateMoveShared);
	}

	if (g_interface.ClientHook && indexes::CreateMove > -1)
	{
		oCreateMove = (FnCreateMove)g_interface.ClientHook->HookFunction(indexes::CreateMove, Hooked_CreateMove);
		g_interface.ClientHook->HookTable(true);
		Utils::log("oCreateMove = 0x%X", (DWORD)oCreateMove);
	}

	if (g_interface.ClientHook && indexes::FrameStageNotify > -1)
	{
		oFrameStageNotify = (FnFrameStageNotify)g_interface.ClientHook->HookFunction(indexes::FrameStageNotify, Hooked_FrameStageNotify);
		g_interface.ClientHook->HookTable(true);
		Utils::log("oFrameStageNotify = 0x%X", (DWORD)oFrameStageNotify);
	}

	if (g_interface.ClientHook && indexes::InKeyEvent > -1)
	{
		oInKeyEvent = (FnInKeyEvent)g_interface.ClientHook->HookFunction(indexes::InKeyEvent, Hooked_InKeyEvent);
		g_interface.ClientHook->HookTable(true);
		Utils::log("oInKeyEvent = 0x%X", (DWORD)oInKeyEvent);
	}

	if (g_interface.ClientHook && indexes::DispatchUserMessage > -1)
	{
		oDispatchUserMessage = (FnDispatchUserMessage)g_interface.ClientHook->HookFunction(indexes::DispatchUserMessage, Hooked_DispatchUserMessage);
		g_interface.ClientHook->HookTable(true);
		Utils::log("oDispatchUserMessage = 0x%X", (DWORD)oDispatchUserMessage);
	}

	if (g_interface.PredictionHook && indexes::RunCommand > -1)
	{
		oRunCommand = (FnRunCommand)g_interface.PredictionHook->HookFunction(indexes::RunCommand, Hooked_RunCommand);
		g_interface.PredictionHook->HookTable(true);
		Utils::log("oRunCommand = 0x%X", (DWORD)oRunCommand);
	}

	/*
	if (g_interface.ModelRenderHook && indexes::DrawModel > -1)
	{
		oDrawModel = (FnDrawModel)g_interface.ModelRenderHook->HookFunction(indexes::DrawModel, Hooked_DrawModel);
		g_interface.ModelRenderHook->HookTable(true);
		Utils::log("oDrawModel = 0x%X", (DWORD)oDrawModel);
	}
	*/

	/*
	if (g_interface.ViewRenderHook && indexes::VGui_Paint > -1)
	{
		oVGUIPaint = (FnVGUIPaint)g_interface.ViewRenderHook->HookFunction(indexes::VGui_Paint, &Hooked_VGUIPaint);
		g_interface.ViewRenderHook->HookTable(true);
		Utils::log("oVgui_Paint = 0x%X", (DWORD)oVGUIPaint);
	}
	*/

	if (g_interface.EngineVGuiHook && indexes::EnginePaint > -1)
	{
		oEnginePaint = (FnEnginePaint)g_interface.EngineVGuiHook->HookFunction(indexes::EnginePaint, &Hooked_EnginePaint);
		g_interface.EngineVGuiHook->HookTable(true);
		Utils::log("oEnginePaint = 0x%X", (DWORD)oEnginePaint);
	}

	/*
	if (g_interface.EngineVGuiHook && indexes::EngineKeyEvent > -1)
	{
		oEngineKeyEvent = (FnEngineKeyEvent)g_interface.EngineVGuiHook->HookFunction(indexes::EngineKeyEvent, &Hooked_EngineKeyEvent);
		g_interface.EngineVGuiHook->HookTable(true);
		Utils::log("oEngineKeyEvent = 0x%X", (DWORD)oEngineKeyEvent);
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

	if (g_interface.Cvar)
	{
		g_conVar["sv_cheats"] = g_interface.Cvar->FindVar("sv_cheats");
		g_conVar["r_drawothermodels"] = g_interface.Cvar->FindVar("r_drawothermodels");
		g_conVar["cl_drawshadowtexture"] = g_interface.Cvar->FindVar("cl_drawshadowtexture");
		g_conVar["mat_fullbright"] = g_interface.Cvar->FindVar("mat_fullbright");
		g_conVar["sv_pure"] = g_interface.Cvar->FindVar("sv_pure");
		g_conVar["sv_consistency"] = g_interface.Cvar->FindVar("sv_consistency");
		g_conVar["mp_gamemode"] = g_interface.Cvar->FindVar("mp_gamemode");
		g_conVar["c_thirdpersonshoulder"] = g_interface.Cvar->FindVar("c_thirdpersonshoulder");
		g_conVar["c_thirdpersonshoulderheight"] = g_interface.Cvar->FindVar("c_thirdpersonshoulderheight");
		g_conVar["c_thirdpersonshoulderoffset"] = g_interface.Cvar->FindVar("c_thirdpersonshoulderoffset");
		g_conVar["cl_mouseenable"] = g_interface.Cvar->FindVar("cl_mouseenable");
		g_conVar["cl_interp"] = g_interface.Cvar->FindVar("cl_interp");
		g_conVar["cl_updaterate"] = g_interface.Cvar->FindVar("cl_updaterate");
		g_conVar["sv_maxupdaterate"] = g_interface.Cvar->FindVar("sv_maxupdaterate");
		g_conVar["sv_minupdaterate"] = g_interface.Cvar->FindVar("sv_minupdaterate");
		g_conVar["cl_interp_ratio"] = g_interface.Cvar->FindVar("cl_interp_ratio");
		g_conVar["mat_hdr_enabled"] = g_interface.Cvar->FindVar("mat_hdr_enabled");
		g_conVar["mat_hdr_level"] = g_interface.Cvar->FindVar("mat_hdr_level");
		g_conVar["mat_texture_list"] = g_interface.Cvar->FindVar("mat_texture_list");
		g_conVar["r_dynamic"] = g_interface.Cvar->FindVar("r_dynamic");
		g_conVar["r_dynamiclighting"] = g_interface.Cvar->FindVar("r_dynamiclighting");
		g_conVar["mat_picmip"] = g_interface.Cvar->FindVar("mat_picmip");
		g_conVar["mat_showlowresimage"] = g_interface.Cvar->FindVar("mat_showlowresimage");

		Utils::log("sv_cheats = 0x%X", (DWORD)g_conVar["sv_cheats"]);
		Utils::log("r_drawothermodels = 0x%X", (DWORD)g_conVar["r_drawothermodels"]);
		Utils::log("cl_drawshadowtexture = 0x%X", (DWORD)g_conVar["cl_drawshadowtexture"]);
		Utils::log("mat_fullbright = 0x%X", (DWORD)g_conVar["mat_fullbright"]);
		Utils::log("sv_pure = 0x%X", (DWORD)g_conVar["sv_pure"]);
		Utils::log("sv_consistency = 0x%X", (DWORD)g_conVar["sv_consistency"]);
		Utils::log("mp_gamemode = 0x%X", (DWORD)g_conVar["mp_gamemode"]);
		Utils::log("c_thirdpersonshoulder = 0x%X", (DWORD)g_conVar["c_thirdpersonshoulder"]);
		Utils::log("c_thirdpersonshoulderheight = 0x%X", (DWORD)g_conVar["c_thirdpersonshoulderheight"]);
		Utils::log("c_thirdpersonshoulderoffset = 0x%X", (DWORD)g_conVar["c_thirdpersonshoulderoffset"]);
		Utils::log("cl_mouseenable = 0x%X", (DWORD)g_conVar["cl_mouseenable"]);
		Utils::log("cl_interp = 0x%X", (DWORD)g_conVar["cl_interp"]);
		Utils::log("cl_updaterate = 0x%X", (DWORD)g_conVar["cl_updaterate"]);
		Utils::log("sv_maxupdaterate = 0x%X", (DWORD)g_conVar["sv_maxupdaterate"]);
		Utils::log("sv_minupdaterate = 0x%X", (DWORD)g_conVar["sv_minupdaterate"]);
		Utils::log("cl_interp_ratio = 0x%X", (DWORD)g_conVar["cl_interp_ratio"]);
		Utils::log("mat_hdr_enabled = 0x%X", (DWORD)g_conVar["mat_hdr_enabled"]);
		Utils::log("mat_hdr_level = 0x%X", (DWORD)g_conVar["mat_hdr_level"]);
		Utils::log("mat_texture_list = 0x%X", (DWORD)g_conVar["mat_texture_list"]);
		Utils::log("r_dynamic = 0x%X", (DWORD)g_conVar["r_dynamic"]);
		Utils::log("r_dynamiclighting = 0x%X", (DWORD)g_conVar["r_dynamiclighting"]);
		Utils::log("mat_picmip = 0x%X", (DWORD)g_conVar["mat_picmip"]);
		Utils::log("mat_showlowresimage = 0x%X", (DWORD)g_conVar["mat_showlowresimage"]);
	}

	// 初始化 ImGui
	while ((g_hwGameWindow = FindWindowA(nullptr, "Left 4 Dead 2")) == nullptr)
		std::this_thread::sleep_for(std::chrono::milliseconds(200));

	g_pOldWindowProccess = (WNDPROC)SetWindowLongPtrA(g_hwGameWindow, GWL_WNDPROC, (LONG_PTR)ImGui_WindowProccess);

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
	if (g_interface.GameEvent)
	{
		class EventListener : public IGameEventListener2
		{
		public:
			virtual void FireGameEvent(IGameEvent *event)
			{
				const char* eventName = event->GetName();
				CBaseEntity* local = GetLocalClient();

				if (_strcmpi(eventName, "player_death") == 0)
				{
					// 受害者（死者）
					int victim = g_interface.Engine->GetPlayerForUserID(event->GetInt("userid"));
					if (victim <= 0)
						victim = event->GetInt("entityid");
					if (victim <= 0)
						return;

					// 攻击者（击杀者）
					int attacker = g_interface.Engine->GetPlayerForUserID(event->GetInt("attacker"));
					if (attacker <= 0)
						attacker = event->GetInt("attackerentid");

					CBaseEntity* victimEntity = g_interface.ClientEntList->GetClientEntity(victim);
					if (!IsValidVictim(victimEntity))
						return;

					if ((DWORD)victimEntity == (DWORD)g_pCurrentAimbot)
						g_pCurrentAimbot = nullptr;
				}
				else if (_strcmpi(eventName, "infected_death") == 0)
				{
					int victim = event->GetInt("infected_id");
					if (victim <= 0)
						return;

					CBaseEntity* entity = g_interface.ClientEntList->GetClientEntity(victim);
					if (!IsValidVictim(entity))
						return;

					if ((DWORD)entity == (DWORD)g_pCurrentAimbot)
						g_pCurrentAimbot = nullptr;
				}
				else if (_strcmpi(eventName, "player_spawn") == 0)
				{
					int client = g_interface.Engine->GetPlayerForUserID(event->GetInt("userid"));
					if (client <= 0)
						return;

					CBaseEntity* entity = g_interface.ClientEntList->GetClientEntity(client);
					if (!IsValidVictim(entity))
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
					int client = g_interface.Engine->GetPlayerForUserID(event->GetInt("userid"));
					const char* text = event->GetString("text");

					bool teamOnly = (eventName[7] == 'c' ? event->GetBool("teamonly") : false);

					if (client <= 0 || text == nullptr || text[0] == '\0' || !teamOnly)
						return;

					CBaseEntity* entity = g_interface.ClientEntList->GetClientEntity(client);
					if (!IsValidVictim(entity))
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

					int client = g_interface.Engine->GetPlayerForUserID(event->GetInt("userid"));
					int oldTeam = event->GetInt("oldteam");
					int newTeam = event->GetInt("team");
					if (client <= 0 || newTeam == 0)
						return;

					CBaseEntity* entity = g_interface.ClientEntList->GetClientEntity(client);
					if (!IsValidVictim(entity))
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
					int client = g_interface.Engine->GetPlayerForUserID(event->GetInt("userid"));
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

					CBaseEntity* entity = g_interface.ClientEntList->GetClientEntity(voter);
					if (!IsValidVictim(entity))
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

					CBaseEntity* entity = g_interface.ClientEntList->GetClientEntity(client);
					if (!IsValidVictim(entity))
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

					CBaseEntity* entity = g_interface.ClientEntList->GetClientEntity(client);
					if (!IsValidVictim(entity))
						return;

					if (g_pDrawRender)
					{
						g_pDrawRender->PushRenderText(DrawManager::PINK,
							"team %d player %s vote no", team, GetZombieClassName(entity).c_str());
					}
				}
				else if (_strcmpi(eventName, "tank_spawn") == 0)
				{
					int client = g_interface.Engine->GetPlayerForUserID(event->GetInt("userid"));
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
					int victim = g_interface.Engine->GetPlayerForUserID(event->GetInt("userid"));

					// 攻击者
					int attacker = g_interface.Engine->GetPlayerForUserID(event->GetInt("attacker"));

					// 是谁的错
					int guilty = g_interface.Engine->GetPlayerForUserID(event->GetInt("guilty"));

					// 伤害类型
					// int damageType = event->GetInt("type");

					if (victim <= 0 || attacker <= 0)
						return;

					CBaseEntity* entityAttacker = g_interface.ClientEntList->GetClientEntity(attacker);
					CBaseEntity* entityVictim = g_interface.ClientEntList->GetClientEntity(victim);
					if (!IsValidVictim(entityAttacker) || !IsValidVictim(entityVictim))
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
					int victim = g_interface.Engine->GetPlayerForUserID(event->GetInt("userid"));

					// 攻击者
					int attacker = g_interface.Engine->GetPlayerForUserID(event->GetInt("attacker"));

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
					g_pCurrentAimbot = nullptr;
				}
				else if (_strcmpi(eventName, "mission_lost") == 0)
				{
					Utils::log("*** round lost ***");
					g_pCurrentAimbot = nullptr;
				}
				else if (_strcmpi(eventName, "weapon_fire") == 0)
				{
					int client = g_interface.Engine->GetPlayerForUserID(event->GetInt("userid"));
					const char* weapon = event->GetString("weapon");
					if (client != g_interface.Engine->GetLocalPlayer() ||
						weapon == nullptr || weapon[0] == '\0')
						return;

					if (g_conVar["c_thirdpersonshoulder"] != nullptr &&
						g_conVar["c_thirdpersonshoulder"]->GetInt() > 0)
					{
						if (_strcmpi(weapon, "autoshotgun") == 0)
							g_interface.Engine->ClientCmd("play \"weapons/auto_shotgun/gunfire/auto_shotgun_fire_1.wav\"");
						else if (_strcmpi(weapon, "shotgun_spas") == 0)
							g_interface.Engine->ClientCmd("play \"weapons/auto_shotgun_spas/gunfire/shotgun_fire_1.wav\"");
						else if (_strcmpi(weapon, "pumpshotgun") == 0)
							g_interface.Engine->ClientCmd("play \"weapons/shotgun/gunfire/shotgun_fire_1.wav\"");
						else if (_strcmpi(weapon, "shotgun_chrome") == 0)
							g_interface.Engine->ClientCmd("play \"weapons/shotgun_chrome/gunfire/shotgun_fire_1.wav\"");
					}
					if (Config::bNoRecoil)
					{
						local->SetNetProp<int>("m_iShotsFired", 0, "DT_CSPlayer");
						local->SetLocalNetProp<QAngle>("m_vecPunchAngle", QAngle(0.0f, 0.0f, 0.0f));
					}
				}
				else if (_strcmpi(eventName, "player_hurt") == 0)
				{
					if (local == nullptr || !local->IsAlive())
						return;
					
					if (event->GetInt("dmg_health") <= 0)
						return;

					int victim = g_interface.Engine->GetPlayerForUserID(event->GetInt("userid"));
					if (victim == 0)
						return;

					CBaseEntity* entity = g_interface.ClientEntList->GetClientEntity(victim);
					if (!IsValidVictim(entity))
						return;

					// 被攻击自动瞄准
					if (entity->GetTeam() != local->GetTeam())
						g_pCurrentAimbot = entity;
				}
				else if (_strcmpi(eventName, "tongue_grab") == 0)
				{
					if (local == nullptr || !local->IsAlive())
						return;

					int victim = g_interface.Engine->GetPlayerForUserID(event->GetInt("victim"));
					int attacker = g_interface.Engine->GetPlayerForUserID(event->GetInt("userid"));
					if (victim == 0 || attacker == 0)
						return;

					CBaseEntity* victimEntity = g_interface.ClientEntList->GetClientEntity(victim);
					CBaseEntity* attackerEntity = g_interface.ClientEntList->GetClientEntity(attacker);
					if (!IsValidVictim(victimEntity) || !IsValidVictim(attackerEntity))
						return;

					// 被拉自动瞄准
					if((DWORD)victimEntity == (DWORD)local)
						g_pCurrentAimbot = attackerEntity;
				}
			}
		};

		// 注册事件监听器
		EventListener* listener = new EventListener();
		// g_interface.GameEvent->AddListener(listener, "player_spawn", false);
		g_interface.GameEvent->AddListener(listener, "player_death", false);
		g_interface.GameEvent->AddListener(listener, "infected_death", false);
		g_interface.GameEvent->AddListener(listener, "map_transition", false);
		g_interface.GameEvent->AddListener(listener, "mission_lost", false);
		g_interface.GameEvent->AddListener(listener, "weapon_fire", false);
		g_interface.GameEvent->AddListener(listener, "player_hurt", false);
		g_interface.GameEvent->AddListener(listener, "tongue_grab", false);

		/*
		g_interface.GameEvent->AddListener(listener, "player_connect", false);
		g_interface.GameEvent->AddListener(listener, "player_disconnect", false);
		g_interface.GameEvent->AddListener(listener, "player_chat", false);
		g_interface.GameEvent->AddListener(listener, "player_team", false);
		g_interface.GameEvent->AddListener(listener, "door_unlocked", false);
		g_interface.GameEvent->AddListener(listener, "vote_started", false);
		g_interface.GameEvent->AddListener(listener, "vote_passed", false);
		g_interface.GameEvent->AddListener(listener, "vote_cast_yes", false);
		g_interface.GameEvent->AddListener(listener, "vote_cast_no", false);
		g_interface.GameEvent->AddListener(listener, "tank_spawn", false);
		g_interface.GameEvent->AddListener(listener, "friendly_fire", false);
		g_interface.GameEvent->AddListener(listener, "tank_killed", false);
		*/
	}

	// 加载配置文件
	sqb::CheatStart();
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

	DWORD d3d9 = Utils::GetModuleBase("d3d9.dll");
	Utils::log("pD3DDevice = 0x%X", (DWORD)device);
	Utils::log("oReset = d3d9.dll + 0x%X", (DWORD)oReset - d3d9);
	Utils::log("oPresent = d3d9.dll + 0x%X", (DWORD)oPresent - d3d9);
	Utils::log("oEndScene = d3d9.dll + 0x%X", (DWORD)oEndScene - d3d9);
	Utils::log("oDrawIndexedPrimitive = d3d9.dll + 0x%X", (DWORD)oDrawIndexedPrimitive - d3d9);
	Utils::log("oCreateQuery = d3d9.dll + 0x%X", (DWORD)oCreateQuery - d3d9);
}

void thirdPerson(bool active)
{
	CBaseEntity* local = GetLocalClient();
	if (local && local->IsAlive())
	{
		if (active)
			local->SetNetProp<float>("m_TimeForceExternalView", 99999.3f, "DT_TerrorPlayer");
		else
			local->SetNetProp<float>("m_TimeForceExternalView", 0.0f, "DT_TerrorPlayer");

		if (active)
		{
			// 第三人称
			local->SetNetProp<int>("m_hObserverTarget", 0, "DT_BasePlayer");
			local->SetNetProp<int>("m_iObserverMode", 1, "DT_BasePlayer");
			local->SetNetProp<int>("m_bDrawViewmodel", 0, "DT_BasePlayer");
		}
		else
		{
			// 第一人称
			local->SetNetProp<int>("m_hObserverTarget", -1, "DT_BasePlayer");
			local->SetNetProp<int>("m_iObserverMode", 0, "DT_BasePlayer");
			local->SetNetProp<int>("m_bDrawViewmodel", 1, "DT_BasePlayer");
		}
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

		g_interface.Engine->ClientCmd("echo \"========= player list =========\"");

		for (int i = 1; i < 64; ++i)
		{
			player = g_interface.ClientEntList->GetClientEntity(i);
			if (player == nullptr || player->IsDormant())
				continue;

			int team = player->GetTeam();
			int mode = player->GetNetProp<int>("m_iObserverMode", "DT_BasePlayer");

#ifdef USE_PLAYER_INFO
			g_interface.Engine->GetPlayerInfo(i, &info);
#endif

			if (team == 1)
			{
				// 瑙傚療鑰
				if (mode != 4 && mode != 5)
					continue;

				target = (CBaseEntity*)player->GetNetProp<CBaseHandle*>("m_hObserverTarget", "DT_BasePlayer");
				if (target && (int)target != -1)
					target = g_interface.ClientEntList->GetClientEntityFromHandle((CBaseHandle*)target);

				if (target == nullptr || (int)target == -1)
					continue;

				if ((DWORD)target == (DWORD)local)
				{
					// 姝ｅ湪瑙傚療鏈湴鐜╁
#ifdef USE_PLAYER_INFO
					g_interface.Engine->ClientCmd("echo \"[spectator] player %s %s you\"",
						info.name, (mode == 4 ? "watching" : "following"));
#else
					g_interface.Engine->ClientCmd("echo \"[spectator] player %d %s you\"",
						player->GetIndex(), (mode == 4 ? "watching" : "following"));
#endif
				}
				else
				{
					// 姝ｅ湪瑙傚療鍏朵粬鐜╁
#ifdef USE_PLAYER_INFO
					player_info_t other;
					g_interface.Engine->GetPlayerInfo(target->GetIndex(), &other);
					g_interface.Engine->ClientCmd("echo \"[spectator] player %s %s %s\"",
						info.name, (mode == 4 ? "watching" : "following"), other.name);
#else
					g_interface.Engine->ClientCmd("echo \"[spectator] player %d %s %d\"",
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
					g_interface.Engine->ClientCmd("echo \"[survivor] player %s is alive (%d + %.0f)\"",
						info.name, player->GetHealth(), player->GetNetProp<float>("m_healthBuffer", "DT_TerrorPlayer"));
#else
					g_interface.Engine->ClientCmd("echo \"[survivor] player %d is alive (%d + %.0f)\"",
						player->GetIndex(), player->GetHealth(), player->GetNetProp<float>("m_healthBuffer", "DT_TerrorPlayer"));
#endif
				}
				else
				{
					// 姝讳骸
					if (mode != 4 && mode != 5)
						continue;

					target = (CBaseEntity*)player->GetNetProp<CBaseHandle*>("m_hObserverTarget", "DT_BasePlayer");
					if (target && (int)target != -1)
						target = g_interface.ClientEntList->GetClientEntityFromHandle((CBaseHandle*)target);

					if (target == nullptr || (int)target == -1)
						continue;

					if ((DWORD)target == (DWORD)local)
					{
						// 姝ｅ湪瑙傚療鏈湴鐜╁
#ifdef USE_PLAYER_INFO
						g_interface.Engine->ClientCmd("echo \"[survivor] player %s %s you\"",
							info.name, (mode == 4 ? "watching" : "following"));
#else
						g_interface.Engine->ClientCmd("echo \"[survivor] player %d %s you\"",
							player->GetIndex(), (mode == 4 ? "watching" : "following"));
#endif
					}
					else
					{
						// 姝ｅ湪瑙傚療鍏朵粬鐜╁
#ifdef USE_PLAYER_INFO
						player_info_t other;
						g_interface.Engine->GetPlayerInfo(target->GetIndex(), &other);
						g_interface.Engine->ClientCmd("echo \"[survivor] player %s %s %s\"",
							info.name, (mode == 4 ? "watching" : "following"), other.name);
#else
						g_interface.Engine->ClientCmd("echo \"[survivor] player %d %s %d\"",
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
					g_interface.Engine->ClientCmd("echo \"[infected] player %s is %s (%d)\"",
						info.name, zombieName, player->GetHealth());
#else
					g_interface.Engine->ClientCmd("echo \"[infected] player %d is %s (%d)\"",
						player->GetIndex(), zombieName, player->GetHealth());
#endif
				}
				else if (player->GetNetProp<byte>("m_isGhost", "DT_TerrorPlayer"))
				{
					// 骞界伒鐘舵€
#ifdef USE_PLAYER_INFO
					g_interface.Engine->ClientCmd("echo \"[infected] player %s is ghost %s (%d)\"",
						info.name, zombieName, player->GetHealth());
#else
					g_interface.Engine->ClientCmd("echo \"[infected] player %d is ghost %s (%d)\"",
						player->GetIndex(), zombieName, player->GetHealth());
#endif
				}
				else
				{
					// 姝讳骸
					if (mode != 4 && mode != 5)
						continue;

					target = (CBaseEntity*)player->GetNetProp<CBaseHandle*>("m_hObserverTarget", "DT_BasePlayer");
					if (target && (int)target != -1)
						target = g_interface.ClientEntList->GetClientEntityFromHandle((CBaseHandle*)target);

					if (target == nullptr || (int)target == -1)
						continue;

					if ((DWORD)target == (DWORD)local)
					{
						// 姝ｅ湪瑙傚療鏈湴鐜╁
#ifdef USE_PLAYER_INFO
						g_interface.Engine->ClientCmd("echo \"[infected] player %s %s you\"",
							info.name, (mode == 4 ? "watching" : "following"));
#else
						g_interface.Engine->ClientCmd("echo \"[infected] player %s %s you\"",
							player->GetIndex(), (mode == 4 ? "watching" : "following"));
#endif
					}
					else
					{
						// 姝ｅ湪瑙傚療鍏朵粬鐜╁
#ifdef USE_PLAYER_INFO
						player_info_t other;
						g_interface.Engine->GetPlayerInfo(target->GetIndex(), &other);
						g_interface.Engine->ClientCmd("echo \"[infected] player %s %s %s\"",
							info.name, (mode == 4 ? "watching" : "following"), other.name);
#else
						g_interface.Engine->ClientCmd("echo \"[infected] player %d %s %d\"",
							player->GetIndex(), (mode == 4 ? "watching" : "following"), target->GetIndex());
#endif
					}
				}
			}
		}

		g_interface.Engine->ClientCmd("echo \"========= list end =========\"");
	}
}

void bindAlias(int wait)
{
	g_interface.Engine->ClientCmd("echo \"echo \"========= alias begin =========\"\"");
	g_interface.Engine->ClientCmd("alias +autofire \"alias autofire_launcher autofire_loop; autofire_launcher\"");
	g_interface.Engine->ClientCmd("alias -autofire \"alias autofire_launcher autofire_stop\"");
	g_interface.Engine->ClientCmd("alias autofire_launcher autofire_loop");
	g_interface.Engine->ClientCmd("alias autofire_loop \"+attack; wait 5; -attack; wait 5; autofire_launcher\"");
	g_interface.Engine->ClientCmd("alias autofire_stop \"-attack\"");
	g_interface.Engine->ClientCmd("alias +autojump \"alias autojump_launcher autojump_loop; autojump_launcher\"");
	g_interface.Engine->ClientCmd("alias -autojump \"alias autojump_launcher autojump_stop\"");
	g_interface.Engine->ClientCmd("alias autojump_launcher autojump_loop");
	g_interface.Engine->ClientCmd("alias autojump_loop \"+jump; wait 5; -jump; wait 5; autojump_launcher\"");
	g_interface.Engine->ClientCmd("alias autojump_stop \"-jump\"");
	g_interface.Engine->ClientCmd("alias +bigjump \"+jump; +duck\"");
	g_interface.Engine->ClientCmd("alias -bigjump \"-jump; -duck\"");
	g_interface.Engine->ClientCmd("alias +pistolspam \"alias pistolspam dopistolspam; dopistolspam\"");
	g_interface.Engine->ClientCmd("alias -pistolspam \"alias pistolspam -use\"");
	g_interface.Engine->ClientCmd("alias dopistolspam \"+use; wait 3; -use; wait 3; pistolspam\"");
	g_interface.Engine->ClientCmd("alias +fastmelee \"alias fastmelee_launcher fastmelee_loop; fastmelee_launcher\"");
	g_interface.Engine->ClientCmd("alias fastmelee_launcher fastmelee_loop");
	g_interface.Engine->ClientCmd("alias fastmelee_loop \"+attack; slot1; wait 1; -attack; slot2; wait 45; fastmelee_launcher\"");
	g_interface.Engine->ClientCmd("alias fastmelee_stop \"-attack\"");
	g_interface.Engine->ClientCmd("alias -fastmelee \"alias fastmelee_launcher fastmelee_stop\"");
	g_interface.Engine->ClientCmd("alias thirdperson_toggle \"thirdperson_enable\"");
	g_interface.Engine->ClientCmd("alias thirdperson_enable \"alias thirdperson_toggle thirdperson_disable; thirdpersonshoulder\"");
	g_interface.Engine->ClientCmd("alias thirdperson_disable \"alias thirdperson_toggle thirdperson_enable; thirdpersonshoulder; c_thirdpersonshoulder 0\"");
	g_interface.Engine->ClientCmd("c_thirdpersonshoulder 0");
	g_interface.Engine->ClientCmd("c_thirdpersonshoulderoffset 0");
	g_interface.Engine->ClientCmd("c_thirdpersonshoulderaimdist 720");
	g_interface.Engine->ClientCmd("cam_ideallag 0");
	g_interface.Engine->ClientCmd("cam_idealdist 40");
	g_interface.Engine->ClientCmd("bind leftarrow \"incrementvar cam_idealdist 30 130 10\"");
	g_interface.Engine->ClientCmd("bind rightarrow \"incrementvar cam_idealdist 30 130 -10\"");
	g_interface.Engine->ClientCmd("bind uparrow \"incrementvar c_thirdpersonshoulderheight 5 25 5\"");
	g_interface.Engine->ClientCmd("bind downarrow \"incrementvar c_thirdpersonshoulderheight 5 25 -5\"");
	g_interface.Engine->ClientCmd("c_thirdpersonshoulderoffset 0");
	g_interface.Engine->ClientCmd("cl_crosshair_alpha 255");
	g_interface.Engine->ClientCmd("cl_crosshair_blue 0");
	g_interface.Engine->ClientCmd("cl_crosshair_green 50");
	g_interface.Engine->ClientCmd("cl_crosshair_red 190");
	g_interface.Engine->ClientCmd("cl_crosshair_dynamic 0");
	g_interface.Engine->ClientCmd("cl_crosshair_thickness 1.0");
	g_interface.Engine->ClientCmd("crosshair 1");
	g_interface.Engine->ClientCmd("con_enable 1");
	g_interface.Engine->ClientCmd("muzzleflash_light 1");
	g_interface.Engine->ClientCmd("sv_voiceenable 1");
	g_interface.Engine->ClientCmd("voice_enable 1");
	g_interface.Engine->ClientCmd("voice_forcemicrecord 1");
	g_interface.Engine->ClientCmd("mat_monitorgamma 1.6");
	g_interface.Engine->ClientCmd("mat_monitorgamma_tv_enabled 1");
	g_interface.Engine->ClientCmd("cl_ignore_vpk_assocation 1");
	g_interface.Engine->ClientCmd("cl_observercrosshair 1");
	g_interface.Engine->ClientCmd("cl_showpluginmessages 0");
	g_interface.Engine->ClientCmd("cl_viewmodelfovsurvivor 67");
	g_interface.Engine->ClientCmd("cl_restrict_server_commands 1");
	g_interface.Engine->ClientCmd("con_enable 1");
	g_interface.Engine->ClientCmd("z_nightvision_r 255");
	g_interface.Engine->ClientCmd("z_nightvision_g 255");
	g_interface.Engine->ClientCmd("z_nightvision_b 255");
	g_interface.Engine->ClientCmd("bind space +jump");
	g_interface.Engine->ClientCmd("bind mouse1 +attack");
	g_interface.Engine->ClientCmd("bind mouse2 +attack2");
	g_interface.Engine->ClientCmd("bind mouse3 +zoom");
	g_interface.Engine->ClientCmd("bind ctrl +duck");
	g_interface.Engine->ClientCmd("unbind ins");
	g_interface.Engine->ClientCmd("unbind home");
	g_interface.Engine->ClientCmd("unbind pgup");
	g_interface.Engine->ClientCmd("unbind pgdn");
	g_interface.Engine->ClientCmd("unbind end");
	g_interface.Engine->ClientCmd("unbind del");
	g_interface.Engine->ClientCmd("unbind f9");
	g_interface.Engine->ClientCmd("unbind f10");
	g_interface.Engine->ClientCmd("unbind f11");
	g_interface.Engine->ClientCmd("unbind f12");
	g_interface.Engine->ClientCmd("unbind f8");
	g_interface.Engine->ClientCmd("net_graph 0");
	g_interface.Engine->ClientCmd("net_graphpos 1");
	g_interface.Engine->ClientCmd("net_graphshowinterp 0");
	g_interface.Engine->ClientCmd("net_graphshowlatency 0");
	g_interface.Engine->ClientCmd("net_graphtext 1");
	g_interface.Engine->ClientCmd("cl_showfps 1");
	g_interface.Engine->ClientCmd("alias +shownetscores \"+showscores; net_graph 3; net_graphtext 1\"");
	g_interface.Engine->ClientCmd("alias -shownetscores \"-showscores; net_graph 0\"");
	g_interface.Engine->ClientCmd("bind tab +shownetscores");
	g_interface.Engine->ClientCmd("alias \"lerp_0\" \"rate 30000;cl_cmdrate 100;cl_updaterate 100;cl_interp 0.0;cl_interp_ratio -1;alias lerp_change lerp_16;echo Lerp set to 0 (rate 30000, cl_cmdrate 100, cl_updaterate 100, cl_interp 0.0, cl_interp_ratio -1).\";");
	g_interface.Engine->ClientCmd("alias \"lerp_16\" \"rate 30000;cl_cmdrate 100;cl_updaterate 100;cl_interp 0.0167;cl_interp_ratio -1;alias lerp_change lerp_33;echo Lerp set to 16.7 (rate 30000, cl_cmdrate 100, cl_updaterate 100, cl_interp 0.0167, cl_interp_ratio -1).\";");
	g_interface.Engine->ClientCmd("alias \"lerp_33\" \"rate 30000;cl_cmdrate 100;cl_updaterate 100;cl_interp 0.0334;cl_interp_ratio -1;alias lerp_change lerp_50;echo Lerp set to 33.4 (rate 30000, cl_cmdrate 100, cl_updaterate 100, cl_interp 0.0334, cl_interp_ratio -1).\";");
	g_interface.Engine->ClientCmd("alias \"lerp_50\" \"rate 30000;cl_cmdrate 100;cl_updaterate 100;cl_interp 0.0501;cl_interp_ratio -1;alias lerp_change lerp_66;echo Lerp set to 50.1 (rate 30000, cl_cmdrate 100, cl_updaterate 100, cl_interp 0.0501, cl_interp_ratio -1).\";");
	g_interface.Engine->ClientCmd("alias \"lerp_66\" \"rate 30000;cl_cmdrate 100;cl_updaterate 100;cl_interp 0.0667;cl_interp_ratio -1;alias lerp_change lerp_0;echo Lerp set to 66.7 (rate 30000, cl_cmdrate 100, cl_updaterate 100, cl_interp 0.0667, cl_interp_ratio -1).\";");
	g_interface.Engine->ClientCmd("echo \"echo \"========= alias end =========\"\"");

	if (g_conVar["r_dynamic"])
	{
		CVAR_MAKE_FLAGS("r_dynamic");
		g_conVar["r_dynamic"]->SetValue(0);
		Utils::log("r_dynamic setting %d", g_conVar["r_dynamic"]->GetInt());
	}

	if (g_conVar["r_dynamiclighting"])
	{
		CVAR_MAKE_FLAGS("r_dynamiclighting");
		g_conVar["r_dynamiclighting"]->SetValue(0);
		Utils::log("r_dynamiclighting setting %d", g_conVar["r_dynamiclighting"]->GetInt());
	}

	/*
	if (g_conVar["mat_picmip"])
	{
		CVAR_MAKE_FLAGS("mat_picmip");
		g_conVar["mat_picmip"]->SetValue(4);
		Utils::log("mat_picmip setting %d", g_conVar["mat_picmip"]->GetInt());
	}
	*/
}

static CMoveData g_predMoveData;

// 检查该实体是否为生还者/特感/普感/Witch
bool IsValidVictim(CBaseEntity* entity)
{
	int id = 0, solid = 0, sequence = 0;
	ClientClass* cc = nullptr;

	try
	{
		if (entity == nullptr || !(cc = entity->GetClientClass()) || (id = cc->m_ClassID) == ET_WORLD ||
			!IsValidVictimId(id) || entity->IsDormant())
			return false;

		solid = entity->GetNetProp<int>("m_usSolidFlags", "DT_BaseCombatCharacter");
		sequence = entity->GetNetProp<int>("m_nSequence", "DT_BaseAnimating");
	}
	catch (...)
	{
#ifdef _DEBUG
		Utils::log("%s (%d) 警告：指针 0x%X 并不是实体！", __FILE__, __LINE__, (DWORD)entity);
		throw std::exception("IsValidVictim 发生了错误");
#endif
		return false;
	}

	if (id == ET_BOOMER || id == ET_HUNTER || id == ET_SMOKER || id == ET_SPITTER ||
		id == ET_JOCKEY || id == ET_CHARGER || id == ET_TANK)
	{
		if (!entity->IsAlive() || entity->GetNetProp<byte>("m_isGhost", "DT_TerrorPlayer") != 0)
		{
#ifdef _DEBUG_OUTPUT
			g_interface.Engine->ClientCmd("echo \"Special 0x%X healh = %d, ghost = %d\"", (DWORD)entity,
				entity->GetNetProp<int>("m_iHealth", "DT_TerrorPlayer"), entity->GetNetProp<int>("m_isGhost", "DT_TerrorPlayer"));
#endif
			return false;
		}

		if (id == ET_TANK && sequence > 70)
		{
#ifdef _DEBUG_OUTPUT
			g_interface.Engine->ClientCmd("echo \"tank 0x%X is dead\"", (DWORD)entity);
#endif
			return false;
		}
	}
	else if (id == ET_INFECTED || id == ET_WITCH)
	{
		if ((solid & SF_NOT_SOLID) || sequence > 305)
		{
#ifdef _DEBUG_OUTPUT
			g_interface.Engine->ClientCmd("echo \"Common 0x%X is dead\"", (DWORD)entity);
#endif
			return false;
		}

		// 普感正在燃烧
		if (id == ET_INFECTED && entity->GetNetProp<byte>("m_bIsBurning", "DT_Infected") != 0)
			return false;
	}
	else if (id == ET_CTERRORPLAYER || id == ET_SURVIVORBOT)
	{
		if (!entity->IsAlive())
		{
#ifdef _DEBUG_OUTPUT
			g_interface.Engine->ClientCmd("echo \"Survivor 0x%X is dead %d\"", (DWORD)entity,
				entity->GetNetProp<int>("m_iHealth", "DT_TerrorPlayer"));
#endif
			return false;
		}
	}
	else
	{
#ifdef _DEBUG_OUTPUT
		// Utils::log("Invalid ClassId = %d | sequence = %d | solid = %d", id, sequence, solid);
		g_interface.Engine->ClientCmd("echo \"Invalid Entity 0x%X ClassId %d\"", (DWORD)entity, id);
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
			position.z = g_pCurrentAimbot->GetAbsOrigin().z + 30.0f;
		else if (zombieClass == ZC_HUNTER && (g_pCurrentAimbot->GetFlags() & FL_DUCKING))
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
		if (!IsValidVictim(entity))
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

// 速度预测
inline Vector VelocityExtrapolate(CBaseEntity* player, const Vector& aimpos)
{
	return aimpos + (player->GetVelocity() * g_interface.Globals->interval_per_tick);
}

// 获取当前正在瞄准的敌人
CBaseEntity* GetAimingTarget(int* hitbox = nullptr)
{
	CBaseEntity* client = GetLocalClient();
	if (client == nullptr || !client->IsAlive())
		return nullptr;

	Ray_t ray;
	trace_t trace;
	CTraceFilter filter;

	Vector src = client->GetEyePosition(), dst;

	// 速度预测，防止移动时不精准
	src = VelocityExtrapolate(client, src);

	filter.pSkip1 = client;
	g_interface.Engine->GetViewAngles(dst);

	// angle (QAngle) to basic vectors.
	AngleVectors(dst, dst);

	// multiply our angles by shooting range.
	dst *= 3500.f;

	// end point = our eye position + shooting range.
	dst += src;

	ray.Init(src, dst);

#ifdef _DEBUG_OUTPUT
	Utils::log("TraceRay: skip = 0x%X | start = (%.2f %.2f %.2f) | end = (%.2f %.2f %.2f)",
		(DWORD)client, src.x, src.y, src.z, dst.x, dst.y, dst.z);
#endif

	__try
	{
		g_interface.Trace->TraceRay(ray, MASK_SHOT, &filter, &trace);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		Utils::log("%s (%d): TraceRayError: 0x%X", __FILE__, __LINE__, GetExceptionCode());
		return nullptr;
	}

#ifdef _DEBUG_OUTPUT
	Utils::log("TraceRay: entity = 0x%X | hitbox = %d | bone = %d | hitGroup = %d | fraction = %.2f | classId = %d",
		trace.m_pEnt, trace.hitbox, trace.physicsBone, trace.hitGroup, trace.fraction,
		(trace.m_pEnt != nullptr ? trace.m_pEnt->GetClientClass()->m_ClassID : -1));
#endif

	// 检查是否命中了游戏世界
	if (trace.m_pEnt == nullptr || trace.m_pEnt->IsDormant() ||
		trace.m_pEnt->GetClientClass()->m_ClassID == ET_WORLD)
	{
#ifdef _DEBUG_OUTPUT
		g_interface.Engine->ClientCmd("echo \"invalid entity 0x%X | start (%.2f %.2f %.2f) end (%.2f %.2f %.2f)\"",
			(DWORD)trace.m_pEnt, trace.start.x, trace.start.y, trace.start.z, trace.end.x, trace.end.y, trace.end.z);
#endif
		if (hitbox != nullptr)
			*hitbox = 0;

		return nullptr;
	}

	// 检查是否命中了一个可以攻击的物体
	if (trace.hitbox == 0)
	{
#ifdef _DEBUG_OUTPUT
		g_interface.Engine->ClientCmd("echo \"invalid hitbox 0x%X | hitbox = %d | bone = %d | group = %d\"",
			(DWORD)trace.m_pEnt, trace.hitbox, trace.physicsBone, trace.hitGroup);
#endif
		if (hitbox != nullptr)
			*hitbox = 0;

		return nullptr;
	}

	if (hitbox != nullptr)
		*hitbox = trace.hitbox;

	return trace.m_pEnt;
}

// 检查本地玩家是否可以看见某个实体
bool IsTargetVisible(CBaseEntity* entity, Vector end, Vector start)
{
#ifdef _DEBUG
	try
	{
#endif
		if (!IsValidVictim(entity))
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
	if (client == nullptr || !g_interface.Engine->IsInGame())
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

	try
	{
		g_interface.Trace->TraceRay(ray, MASK_SHOT, &filter, &trace);
	}
	catch(std::exception& e)
	{
		Utils::log("%s (%d): %s", __FILE__, __LINE__, e.what());
		return false;
	}
	catch (...)
	{
		return false;
	}

	/*
	// 检查是否为指定目标
	if ((DWORD)trace.m_pEnt != (DWORD)entity || trace.hitbox <= 0)
		return false;
	*/

	// return (trace.m_pEnt == entity || trace.fraction == 0.97f);
	return (trace.m_pEnt == entity || trace.fraction == 1.0f);
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

	ImGui_ImplDX9_InvalidateDeviceObjects();
	g_pDrawRender->OnLostDevice();

	HRESULT result = oReset(device, pp);

	int width, height;
	g_interface.Engine->GetScreenSize(width, height);

	g_pDrawRender->OnResetDevice(width, height);
	ImGui_ImplDX9_CreateDeviceObjects();

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

	if (!g_pDrawRender)
		g_pDrawRender = std::make_unique<DrawManager>(device);

	if (!g_pCallbackOnMenuToggle)
	{
		g_pCallbackOnMenuToggle = [](bool showMenu) -> void
		{
			if (g_conVar["cl_mouseenable"] != nullptr)
				g_conVar["cl_mouseenable"]->SetValue(showMenu);

			ImGui::GetIO().MouseDrawCursor = false;
			g_bIsShowMenu = false;
		};

		ImGui_ImplDX9_Init(g_hwGameWindow, device);
	}

	g_pDrawRender->BeginImGuiRender();
	ImGui::GetIO().MouseDrawCursor = g_bIsShowMenu;

	// 绘制菜单
	if (g_bIsShowMenu)
	{
		// 绘制一个窗口
		ImGui::Begin(u8"L4D2Simple", &g_bIsShowMenu, ImVec2(300, 250), 0.75f);
		{
			// 绘制
			if (ImGui::CollapsingHeader(u8"Draw"))
			{
				ImGui::Checkbox(u8"2D Box", &Config::bDrawBox);
				ImGui::Checkbox(u8"Bone Box", &Config::bDrawBone);
				ImGui::Checkbox(u8"Name", &Config::bDrawName);
				ImGui::Checkbox(u8"Distance", &Config::bDrawDist);
				ImGui::Checkbox(u8"Ammo", &Config::bDrawAmmo);
				ImGui::Checkbox(u8"Crosshairs", &Config::bDrawAmmo);
			}

			// Z 轴
			if (ImGui::CollapsingHeader(u8"DirextX ZBuffer"))
			{
				ImGui::Checkbox(u8"Survivor", &Config::bBufferSurvivor);
				ImGui::Checkbox(u8"Special Infected", &Config::bBufferSpecial);
				ImGui::Checkbox(u8"Common Infected", &Config::bBufferCommon);
				ImGui::Checkbox(u8"Grenade", &Config::bBufferGrenade);
				ImGui::Checkbox(u8"Medical", &Config::bBufferMedical);
				ImGui::Checkbox(u8"Guns", &Config::bBufferWeapon);
				ImGui::Checkbox(u8"Carry", &Config::bBufferCarry);
			}

			// 瞄准
			if (ImGui::CollapsingHeader("Aiming"))
			{
				ImGui::Checkbox(u8"Aimbot", &Config::bAimbot);
				ImGui::SliderFloat(u8"Aimbot FOV", &Config::fAimbotFov, 1.0f, 360.0f);
				ImGui::Checkbox(u8"Silent Aimbot", &Config::bSilentAimbot);
				ImGui::Checkbox(u8"Trigger Bot", &Config::bTriggerBot);
				ImGui::Checkbox(u8"Trigger Bot Only Head", &Config::bTriggerBotHead);
				ImGui::Checkbox(u8"Do not Fire Firendly", &Config::bAnitFirendlyFire);
			}

			// 其他
			if (ImGui::CollapsingHeader(u8"Misc"))
			{
				ImGui::Checkbox(u8"Bunny Hop", &Config::bBunnyHop);
				ImGui::Checkbox(u8"Auto Strafe", &Config::bAutoStrafe);
				ImGui::Checkbox(u8"No Recoil", &Config::bNoRecoil);
				ImGui::Checkbox(u8"Rapid Fire", &Config::bRapidFire);
				ImGui::Checkbox(u8"CRC Check Bypass", &Config::bCrcCheckBypass);
				ImGui::Checkbox(u8"Full Bright", &Config::bCvarFullBright);
				ImGui::Checkbox(u8"Wireframe", &Config::bCvarWireframe);
				ImGui::Checkbox(u8"Game Mode Change", &Config::bCvarGameMode);
				ImGui::Checkbox(u8"Third Person", &Config::bThirdPersons);
			}
		}
		ImGui::End();	// 完成一个窗口的绘制
	}

#ifdef USE_D3D_DRAW
	{
		CBaseEntity* local = GetLocalClient();
		if (local == nullptr || !g_interface.Engine->IsInGame())
			goto finish_draw;

		// 目前最小距离
		float distmin = 65535.0f;

		// 当前队伍
		int team = local->GetTeam();

		// 当前是否有自动瞄准的目标
		bool targetSelected = false;
		bool specialSelected = false;

#ifdef _DEBUG
		try
		{
#endif
			targetSelected = IsValidVictim(g_pCurrentAimbot);
#ifdef _DEBUG
		}
		catch (std::exception e)
		{
			Utils::log("%s (%d): %s | 0x%X", __FILE__, __LINE__, e.what(), (DWORD)g_pCurrentAimbot);
			targetSelected = false;
			g_pCurrentAimbot = nullptr;
		}
		catch (...)
		{
			Utils::log("%s (%d): 未知异常 | 0x%X", __FILE__, __LINE__, (DWORD)g_pCurrentAimbot);
			targetSelected = false;
			g_pCurrentAimbot = nullptr;
		}
#endif

		if (!targetSelected || !(GetAsyncKeyState(VK_LBUTTON) & 0x8000))
			g_pCurrentAimbot = nullptr;

		Vector myViewAngles;
		g_interface.Engine->GetViewAngles(myViewAngles);

		Vector myEyeOrigin = local->GetEyePosition();
		Vector myOrigin = local->GetAbsOrigin();

		// 一般普感实体索引上限 512 就够了，太大会卡的
		int maxEntity = g_interface.ClientEntList->GetHighestEntityIndex();

		// 绘制颜色
		D3DCOLOR color = 0;

		// 观众数量
		byte spectatorCount = 0;

		// 用于格式化字符串
		std::stringstream ss;
		ss.sync_with_stdio(false);
		ss.tie(nullptr);
		ss.setf(std::ios::fixed);
		ss.precision(0);

		if (Config::bDrawCrosshairs)
		{
			int width, height;
			g_interface.Engine->GetScreenSize(width, height);

			int classId = (!IsValidVictim(g_pCurrentAiming) ? ET_INVALID : g_pCurrentAiming->GetClientClass()->m_ClassID);
			if (classId == ET_INVALID)
				color = DrawManager::LAWNGREEN;
			else if (g_pCurrentAiming->GetTeam() == local->GetTeam())
				color = DrawManager::BLUE;
			else if (classId == ET_INFECTED)
				color = DrawManager::ORANGE;
			else if (classId == ET_WITCH)
				color = DrawManager::PINK;
			else
				color = DrawManager::RED;

			width /= 2;
			height /= 2;

			g_pDrawRender->AddLine(color, width - 10, height, width + 10, height);
			g_pDrawRender->AddLine(color, width, height - 10, width, height + 10);
		}

		// 0 为 worldspawn，没有意义
		for (int i = 1; i <= maxEntity; ++i)
		{
			CBaseEntity* entity = g_interface.ClientEntList->GetClientEntity(i);
			int classId = ET_INVALID;

			try
			{
				if (entity == nullptr || entity->IsDormant() || (DWORD)entity == (DWORD)local)
					continue;

				classId = entity->GetClientClass()->m_ClassID;
				if (g_pGameRulesProxy == nullptr && classId == ET_TerrorGameRulesProxy)
				{
					g_pGameRulesProxy = entity;
					Utils::log("TerrorGameRulesProxy Entity found 0x%X", (DWORD)g_pGameRulesProxy);
				}

				if (g_pPlayerResource == nullptr && classId == ET_TerrorPlayerResource)
				{
					g_pPlayerResource = entity;
					Utils::log("TerrorPlayerResource Entity found 0x%X", (DWORD)g_pPlayerResource);
				}

				if ((DWORD)g_pCurrentAiming == (DWORD)entity)
					g_iCurrentAiming = i;

#ifdef _DEBUG_OUTPUT_
				if (IsSurvivor(classId) || IsSpecialInfected(classId) || IsCommonInfected(classId))
				{
#endif
					// 检查 生还者/特感/普感 是否是有效的
					if (!entity->IsAlive())
					{
						if (Config::bDrawSpectator)
						{
							ss.str("");

							int team = entity->GetTeam();
							if (team == 1 || team == 2 || team == 3)
							{
								int obsMode = entity->GetNetProp<int>("m_iObserverMode", "DT_BasePlayer");
								if (obsMode == OBS_MODE_IN_EYE || obsMode == OBS_MODE_CHASE)
								{
									CBaseEntity* obsTarget = (CBaseEntity*)entity->GetNetProp<CBaseHandle*>("m_hObserverTarget", "DT_BasePlayer");
									if (obsTarget != nullptr)
										obsTarget = g_interface.ClientEntList->GetClientEntityFromHandle((CBaseHandle*)obsTarget);
									if (obsTarget != nullptr && (DWORD)obsTarget == (DWORD)local)
									{
										player_info_t info;
										g_interface.Engine->GetPlayerInfo(i, &info);

										if (obsMode == OBS_MODE_IN_EYE)
											ss << info.name << " [1st]" << std::endl;
										else if (obsMode == OBS_MODE_CHASE)
											ss << info.name << " [3rd]" << std::endl;
									}
								}
							}

							if (!ss.str().empty())
							{
								switch (team)
								{
								case 1:
									// 观察者
									color = DrawManager::WHITE;
									break;
								case 2:
									// 生还者
									color = DrawManager::SKYBLUE;
									break;
								case 3:
									// 感染者
									color = DrawManager::RED;
									break;
								case 4:
									// 非玩家生还者
									color = DrawManager::PURPLE;
									break;
								}

								int width, height;
								g_interface.Engine->GetScreenSize(width, height);
								g_pDrawRender->AddText(color,
									width * 0.75f, (height * 0.75f) +
									(spectatorCount++ * g_pDrawRender->GetFontSize()),
									false, ss.str().c_str());
							}
						}

						continue;
					}
#ifdef _DEBUG_OUTPUT_
				}
#endif
			}
			catch (std::exception e)
			{
				// Utils::log("%s (%d): entity %d %s", __FILE__, __LINE__, i, e.what());
				continue;
			}
			catch (...)
			{
				// Utils::log("%s (%d): entity %d 未知异常 -> 0x%X", __FILE__, __LINE__, i, (DWORD)entity);
				continue;
			}

			Vector head, foot, headbox, origin;

			// 目标脚下的位置
			origin = (IsSurvivor(classId) || IsSpecialInfected(classId) ?
				entity->GetAbsOrigin() :		// 玩家专用，其他实体是没有的
				entity->GetNetProp<Vector>("m_vecOrigin", "DT_BaseCombatCharacter"));

			// 目标的头部的位置
			headbox = (IsSurvivor(classId) || IsSpecialInfected(classId) || IsCommonInfected(classId) ?
				GetHeadHitboxPosition(entity) : origin);

			// 检查目标是否在屏幕内
			if (!headbox.IsValid() || !WorldToScreen(headbox, head) ||
				!WorldToScreen(origin, foot))
				continue;

			// 目标是否可见
			bool visible = IsTargetVisible(entity, headbox, myEyeOrigin);

			// 目标与自己的距离
			float dist = myOrigin.DistTo(origin);

			// 获取方框的大小
			float height = fabs(head.y - foot.y);
			float width = height * 0.65f;

			// 给玩家绘制一个框
			if (Config::bDrawBox)
			{
				if (IsSurvivor(classId) || IsSpecialInfected(classId))
				{
					color = (entity->GetTeam() == team ? DrawManager::BLUE : DrawManager::RED);
					if (IsSurvivor(classId))
					{
						if (entity->GetNetProp<byte>("m_bIsOnThirdStrike", "DT_TerrorPlayer") != 0)
						{
							// 黑白状态 - 白色
							color = DrawManager::WHITE;
						}
						else if (IsControlled(entity))
						{
							// 被控 - 橙色
							color = DrawManager::ORANGE;
						}
						else if (IsIncapacitated(entity))
						{
							// 倒地挂边 - 黄色
							color = DrawManager::YELLOW;
						}
					}
					else if (IsSpecialInfected(classId) && IsGhostInfected(entity))
					{
						// 幽灵状态 - 紫色
						color = DrawManager::PURPLE;
					}

					// 绘制一个框（虽然这个框只有上下两条线）
					
					// g_pDrawRender->AddRect(color, foot.x - width / 2, foot.y, width, -height);
					g_pDrawRender->AddCorner(color, foot.x - width / 2, foot.y, width, -height);
				}
				else if (IsCommonInfected(classId))
				{
					// 这只是普感而已，太远了没必要显示出来
					if (dist > 3000.0f)
						continue;

					int size = 2;
					if (dist < 1000.0f)
						size = 4;

					color = DrawManager::GREEN;
					if (classId == ET_WITCH)
						color = DrawManager::PINK;

					// 画一个小方形，以标记为头部
					/*
					g_cInterfaces.Surface->FillRGBA(head.x, head.y, size, size,
					(color & 0xFF0000) >> 16, (color & 0xFF00) >> 8, color & 0xFF,
					(color & 0xFF000000) >> 24);
					*/

					if (visible)
						g_pDrawRender->AddFillCircle(color, head.x, head.y, size, 8);
					else
						g_pDrawRender->AddCircle(color, head.x, head.y, size, 8);
				}
				else
				{
					// 其他东西
					if (dist > 1000.0f)
						continue;

					if (classId == ET_TankRock)
					{
						// Tank 的石头
						g_pDrawRender->AddCircle(DrawManager::PURPLE, foot.x, foot.y, 9, 8);
					}
					else if (classId == ET_ProjectilePipeBomb || classId == ET_ProjectileMolotov ||
						classId == ET_ProjectileVomitJar)
					{
						// 投掷武器飞行物
						g_pDrawRender->AddCircle(DrawManager::BLUE, foot.x, foot.y, 5, 8);
					}
					else if (classId == ET_ProjectileSpitter || classId == ET_ProjectileGrenadeLauncher)
					{
						// Spitter 的酸液 和 榴弹发射器的榴弹
						g_pDrawRender->AddCircle(DrawManager::GREEN, foot.x, foot.y, 5, 8);
					}
				}
			}

			if (Config::bDrawBone)
			{
				if ((team == 2 && entity->GetTeam() == 3) || (team == 3 && entity->GetTeam() == 2) ||
					(IsCommonInfected(classId) && dist < 1500.0f))
				{
					color = DrawManager::WHITE;
					studiohdr_t* hdr = g_interface.ModelInfo->GetStudioModel(entity->GetModel());
					if (hdr != nullptr && IsValidVictimId(classId))
					{
						Vector parent, child, screenParent, screenChild;
						for (int i = 0; i < hdr->numbones; ++i)
						{
							mstudiobone_t* bone = hdr->pBone(i);
							if (bone == nullptr || !(bone->flags & 0x100) || bone->parent == -1)
								continue;

							child = entity->GetBonePosition(i);
							parent = entity->GetBonePosition(bone->parent);
							if (child.IsValid() && parent.IsValid() &&
								WorldToScreen(parent, screenParent) && WorldToScreen(child, screenChild))
							{
								g_pDrawRender->AddLine(color, screenParent.x, screenParent.y,
									screenChild.x, screenChild.y);
							}
						}
					}
				}
			}

			if (Config::bDrawName)
			{
				ss.str("");

				// 根据类型决定绘制的内容
				if (IsSurvivor(classId) || IsSpecialInfected(classId))
				{
					player_info_t info;
					g_interface.Engine->GetPlayerInfo(i, &info);

					// 检查是否为生还者
					if (IsSurvivor(classId))
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
						if (IsGhostInfected(entity))
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

					// 玩家名字
					ss << info.name;

					// 显示距离
					ss << std::endl << (int)dist;

					CBaseEntity* weapon = (CBaseEntity*)entity->GetActiveWeapon();
					if (weapon != nullptr)
						weapon = g_interface.ClientEntList->GetClientEntityFromHandle((CBaseHandle*)weapon);

					// 给生还者显示弹药
					if (IsSurvivor(classId))
					{
						if (Config::bDrawAmmo && weapon != nullptr)
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
									ss << " (" << clip << "/" <<
										entity->GetNetProp<int>("m_iAmmo", "DT_TerrorPlayer", (size_t)ammoType) <<
										")";
								}
							}
						}
					}
					else
					{
						if (!info.isBot)
						{
							// 如果特感不是机器人的话就显示特感类型
							// 机器人特感名字就是类型
							ss << " (" << GetZombieClassName(entity) << ")";
						}
					}
				}
				else if (classId == ET_WeaponSpawn)
				{
					// 其他东西
					if (dist > 1000.0f)
						continue;

					if (WeaponIdToAlias)
					{
						int weaponId = entity->GetNetProp<short>("m_weaponID", "DT_WeaponSpawn");
						if (IsWeaponT1(weaponId) && Config::bDrawT1Weapon ||
							IsWeaponT2(weaponId) && Config::bDrawT2Weapon ||
							IsWeaponT3(weaponId) && Config::bDrawT3Weapon ||
							IsMelee(weaponId) && Config::bDrawMeleeWeapon ||
							IsMedical(weaponId) && Config::bDrawMedicalItem ||
							IsGrenadeWeapon(weaponId) && Config::bDrawGrenadeItem ||
							IsAmmoStack(weaponId) && Config::bDrawAmmoStack)
							ss << WeaponIdToAlias(weaponId);
					}
				}
				else
				{
					if (dist > 1000.0f)
						continue;
					
					if (classId == ET_WeaponAmmoPack || classId == ET_WeaponAmmoSpawn)
						ss << "ammo_stack";
					else if (classId == ET_WeaponPipeBomb)
						ss << "pipe_bomb";
					else if (classId == ET_WeaponMolotov)
						ss << "molotov";
					else if (classId == ET_WeaponVomitjar)
						ss << "vomitjar";
					else if (classId == ET_WeaponFirstAidKit)
						ss << "first_aid_kit";
					else if (classId == ET_WeaponDefibrillator)
						ss << "defibrillator";
					else if (classId == ET_WeaponPainPills)
						ss << "pain_pills";
					else if (classId == ET_WeaponAdrenaline)
						ss << "adrenaline";
					else if (classId == ET_WeaponMelee)
						ss << "melee";
					else if (classId == ET_WeaponMagnum)
						ss << "pistol_magnum";
				}

				if (!ss.str().empty())
				{
					color = (visible ? DrawManager::LAWNGREEN : DrawManager::YELLOW);

					g_pDrawRender->AddText(color, foot.x, head.y, true, ss.str().c_str());
				}
			}

			// 自动瞄准寻找目标
			if (Config::bAimbot && (!targetSelected || g_pUserCommands == nullptr || !(g_pUserCommands->buttons & IN_ATTACK)) &&
				((team == 2 && (IsSpecialInfected(classId) || classId == ET_INFECTED)) ||
				(team == 3 && IsSurvivor(classId))))
			{
				// 已经选择过目标了，并且这是一个不重要的敌人
				if (classId == ET_INFECTED && specialSelected)
					continue;

				// 选择一个最接近的特感，因为特感越近对玩家来说越危险
				if (entity->GetTeam() != team && dist < distmin && visible &&
					GetAnglesFieldOfView(myViewAngles, CalculateAim(myEyeOrigin, headbox)) <=
					Config::fAimbotFov)
				{
					g_pCurrentAimbot = entity;
					distmin = dist;

					if (IsSpecialInfected(classId))
						specialSelected = true;
				}
			}
		}
	}
finish_draw:
#endif

	sqb::Present(g_pDrawRender.get());
	g_pDrawRender->FinishImGuiRender();

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

	if (!g_pDrawRender)
		g_pDrawRender = std::make_unique<DrawManager>(device);

	// 备份之前绘制设置
	g_pDrawRender->BeginRendering();

	g_bNewFrame = true;
	sqb::EndScene(g_pDrawRender.get());

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

	if (g_interface.Engine->IsInGame())
	{
		static IDirect3DVertexBuffer9* stream = nullptr;
		UINT offsetByte, stride;
		device->GetStreamSource(0, &stream, &offsetByte, &stride);

		if (Config::bBufferCarry && l4d2_carry(stride, numVertices, primitiveCount) ||
			Config::bBufferCommon && l4d2_common(stride, numVertices, primitiveCount) ||
			Config::bBufferGrenade && l4d2_throw(stride, numVertices, primitiveCount) ||
			Config::bBufferMedical && l4d2_healthitem(stride, numVertices, primitiveCount) ||
			Config::bBufferSpecial && l4d2_special(stride, numVertices, primitiveCount) ||
			Config::bBufferSurvivor && l4d2_survivor(stride, numVertices, primitiveCount) ||
			Config::bBufferWeapon && l4d2_weapons(stride, numVertices, primitiveCount) ||
			sqb::DrawIndexedPrimitive(stride, numVertices, primitiveCount))
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

	if (type == D3DQUERYTYPE_OCCLUSION)
	{
		if(sqb::CreateQuery((SQInteger)type) == D3DQUERYTYPE_TIMESTAMP)
			type = D3DQUERYTYPE_TIMESTAMP;
	}

	return oCreateQuery(device, type, query);
}

// -------------------------------- Game Hooked Function --------------------------------
void __fastcall Hooked_PaintTraverse(CPanel* _ecx, void* _edx, unsigned int panel, bool forcePaint, bool allowForce)
{
	static bool showHint = true;
	if (showHint)
	{
		showHint = false;
		Utils::log("Hooked_PaintTraverse trigged.");
	}

	oPaintTraverse(_ecx, panel, forcePaint, allowForce);

	static unsigned int MatSystemTopPanel = 0;
	static unsigned int FocusOverlayPanel = 0;
	static const int FontSize = 16;

	if (MatSystemTopPanel == 0 || FocusOverlayPanel == 0)
	{
		const char* panelName = g_interface.Panel->GetName(panel);
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
		font = g_interface.Surface->SCreateFont();
		g_interface.Surface->SetFontGlyphSet(font, "arial", FontSize, FW_DONTCARE, 0, 0, 0x200);
	}

	// 每一帧调用 2 次
	if (FocusOverlayPanel > 0 && panel == FocusOverlayPanel)
	{
		// 不建议在这里绘制，因为这个并不是全屏幕的
		
		// 在这里获取不会出错
		g_pWorldToScreenMatrix = &g_interface.Engine->WorldToScreenMatrix();

		sqb::PaintTraverse(_SC("FocusOverlayPanel"), g_interface.Surface);
	}

	// 每一帧调 很多次 在这里不能做消耗较大的事情
	if (MatSystemTopPanel > 0 && panel == MatSystemTopPanel)
	{
		// 在这里绘制会导致游戏里 fps 非常低，因此必须要做出限制

		sqb::PaintTraverse(_SC("MatSystemTopPanel"), g_interface.Surface);
	}
}

static bool* bSendPacket = nullptr;
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

	CVerifiedUserCmd *pVerifiedCmd = &(*(CVerifiedUserCmd**)((DWORD)g_interface.Input + 0xE0))[sequence_number % 150];
	CUserCmd *pCmd = &(*(CUserCmd**)((DWORD_PTR)g_interface.Input + 0xDC))[sequence_number % 150];
	if (showHint)
	{
		showHint = false;
		Utils::log("Input->pVerifiedCmd = 0x%X", (DWORD)pVerifiedCmd);
		Utils::log("Input->pCmd = 0x%X", (DWORD)pCmd);
		Utils::log("CL_Move->bSendPacket = 0x%X | %d", (DWORD)bSendPacket, *bSendPacket);
	}

	CBaseEntity* client = GetLocalClient();
	if (client == nullptr || pCmd == nullptr || pVerifiedCmd == nullptr || !client->IsAlive() ||
		g_interface.Engine->IsConsoleVisible())
		return;

	QAngle viewAngles;
	g_interface.Engine->GetViewAngles(viewAngles);
	if (!IsNeedRescue(client) && client->GetNetProp<byte>("m_carryAttacker", "DT_TerrorPlayer") == 0)
	{
		viewAngles.z = 0.0f;
		g_interface.Engine->SetViewAngles(viewAngles);
	}

	float serverTime = GetServerTime();
	CBaseEntity* weapon = (CBaseEntity*)client->GetActiveWeapon();
	if (weapon != nullptr)
		weapon = g_interface.ClientEntList->GetClientEntityFromHandle((CBaseHandle*)weapon);
	else
		weapon = nullptr;

	float nextAttack = (weapon != nullptr ?
		weapon->GetNetProp<float>("m_flNextPrimaryAttack", "DT_BaseCombatWeapon") : FLT_MAX);

	int myTeam = client->GetTeam();
	int weaponId = (weapon != nullptr ? weapon->GetWeaponID() : 0);
	int flags = client->GetNetProp<int>("m_fFlags", "DT_BasePlayer");

	// 自动连跳
	if (Config::bBunnyHop && (GetAsyncKeyState(VK_SPACE) & 0x8000))
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
		if (Config::bAutoStrafe && !(flags & FL_ONGROUND))
		{
			/*
			if (pCmd->mousedx < 0)
				pCmd->sidemove = -450.f;

			if (pCmd->mousedx > 0)
				pCmd->sidemove = 450.f;
			*/

			Vector velocity = client->GetVelocity();
			velocity.z = 0.0f;

			float speed = velocity.Length();
			if (abs(speed) >= 0.1f)
			{
				float rad = GetDelta(speed, 300.0f, 1.0f);
				if (rad > 0.0f)
				{
					float deg = fmod(RAD2DEG(rad), 360);

					Vector forward;
					AngleVectors(pCmd->viewangles, forward);
					VectorNormalize(forward);

					if (abs(forward.Dot(velocity)) > 30.0f)
					{
						float lookYawDeg = fmod(pCmd->viewangles.x, 360);
						Vector wishMoveYaw(pCmd->fowardmove, pCmd->sidemove, 0.0f);
						VectorNormalize(wishMoveYaw);

						Vector o1, o2;
						VectorAngles(wishMoveYaw, o1);

						float wishMoveDegLocal = fmod(o1.x, 360);
						float wishMoveDeg = fmod(lookYawDeg + wishMoveDegLocal, 360);
						
						VectorNormalize(velocity);
						VectorAngles(velocity, o2);
						
						float veloYawDeg = fmod(o2.x, 360);

						float factor;
						if (veloYawDeg - wishMoveDeg > 0.0f)
							factor = 1.0f;
						else
							factor = -1.0f;

						float finalYawDeg = fmod(veloYawDeg + factor * deg, 360);
						float finalYawRad = DEG2RAD(finalYawDeg);

						Vector move(pCmd->fowardmove, pCmd->sidemove, 0.0f);
						speed = move.Length();

						pCmd->fowardmove = speed * cos(finalYawRad);
						pCmd->sidemove = speed * sin(finalYawRad);
					}
				}
			}
		}
	}

	// 引擎预测备份
	float oldCurtime = g_interface.Globals->curtime;
	float oldFrametime = g_interface.Globals->frametime;

	// 引擎预测
	if (g_interface.MoveHelper)
	{
		// 设置随机数种子
		// SetPredictionRandomSeed(MD5_PseudoRandom(pCmd->command_number) & 0x7FFFFFFF);
		*g_pPredictionRandomSeed = MD5_PseudoRandom(pCmd->command_number) & 0x7FFFFFFF;
		
		// 设置需要预测的时间（帧）
		g_interface.Globals->curtime = serverTime;
		g_interface.Globals->frametime = g_interface.Globals->interval_per_tick;

		// 开始检查错误
		g_interface.GameMovement->StartTrackPredictionErrors(client);

		// 清空预测结果的数据
		ZeroMemory(&g_predMoveData, sizeof(CMoveData));

		// 设置需要预测的玩家
		g_interface.MoveHelper->SetHost(client);

		// 开始预测
		g_interface.Prediction->SetupMove(client, pCmd, g_interface.MoveHelper, &g_predMoveData);
		g_interface.GameMovement->ProcessMovement(client, &g_predMoveData);
		g_interface.Prediction->FinishMove(client, pCmd, &g_predMoveData);
	}

	// 根据服务器配置使 tick 最大化
	if (Config::bPositionAdjustment)
	{
		float cl_interp = g_conVar["cl_interp"]->GetFloat();
		int cl_updaterate = g_conVar["cl_updaterate"]->GetInt();
		int sv_maxupdaterate = (g_serverConVar.find("sv_maxupdaterate") != g_serverConVar.end() ? atoi(g_serverConVar["sv_maxupdaterate"].c_str()) : g_conVar["sv_maxupdaterate"]->GetInt());
		int sv_minupdaterate = (g_serverConVar.find("sv_minupdaterate") != g_serverConVar.end() ? atoi(g_serverConVar["sv_minupdaterate"].c_str()) : g_conVar["sv_minupdaterate"]->GetInt());
		int cl_interp_ratio = g_conVar["cl_interp_ratio"]->GetInt();

		if (sv_maxupdaterate <= cl_updaterate)
			cl_updaterate = sv_maxupdaterate;
		if (sv_minupdaterate > cl_updaterate)
			cl_updaterate = sv_minupdaterate;

		float new_interp = (float)cl_interp_ratio / (float)cl_updaterate;
		if (new_interp > cl_interp)
			cl_interp = new_interp;

		float simulationTime = client->GetNetProp<float>("m_flSimulationTime", "DT_BasePlayer");
		// float animTime = client->GetNetProp<float>("m_flAnimTime", "DT_BasePlayer");
		// int tickdiff = (int)(0.5f + (simulationTime - animTime) / g_interface.Globals->interval_per_tick);
		int result = (int)floorf(TIME_TO_TICKS(cl_interp)) + (int)floorf(TIME_TO_TICKS(simulationTime));
		if ((result - pCmd->tick_count) >= -50)
		{
			// 优化 tick
			pCmd->tick_count = result;
		}
	}

	// 当前正在瞄准的目标
	int aiming = *(int*)(client + m_iCrosshairsId);
	g_pCurrentAiming = (aiming > 0 ? g_interface.ClientEntList->GetClientEntity(aiming) : GetAimingTarget(&g_iCurrentHitbox));

#ifdef _DEBUG
	try
	{
#endif
		if (!IsValidVictim(g_pCurrentAiming))
			g_pCurrentAiming = nullptr;
#ifdef _DEBUG
	}
	catch (std::exception e)
	{
		Utils::log("%s (%d): %s | 0x%X", __FILE__, __LINE__, e.what(), (DWORD)g_pCurrentAiming);
		g_pCurrentAiming = nullptr;
	}
	catch (...)
	{
		Utils::log("%s (%d): 未知异常 | 0x%X", __FILE__, __LINE__, (DWORD)g_pCurrentAiming);
		g_pCurrentAiming = nullptr;
	}
#endif

	// 自动瞄准
	if (Config::bAimbot && weapon != nullptr && (GetAsyncKeyState(VK_LBUTTON) & 0x8000))
	{
		Vector myOrigin = client->GetEyePosition(), myAngles = pCmd->viewangles;

		// 自动瞄准数据备份
		static QAngle oldViewAngles;
		static float oldSidemove;
		static float oldForwardmove;
		static float oldUpmove;

		bool runAimbot = false;

		// 目标在另一个地方选择
		if (g_pCurrentAimbot != nullptr && IsGunWeapon(weaponId) && nextAttack <= serverTime)
		{
			// 鐩爣浣嶇疆
			Vector position;
			try
			{
				position = GetHeadHitboxPosition(g_pCurrentAimbot);
			}
			catch (...)
			{
				if (g_pCurrentAimbot->GetClientClass()->m_ClassID == ET_INFECTED)
					goto end_aimbot;

				// 鑾峰彇楠ㄩ浣嶇疆澶辫触
				position = g_pCurrentAimbot->GetEyePosition();
				Utils::log("CBasePlayer::SetupBone error");

				// 鏍规嵁涓嶅悓鐨勬儏鍐电‘瀹氶珮搴
				int zombieClass = g_pCurrentAimbot->GetNetProp<int>("m_zombieClass", "DT_TerrorPlayer");
				if (zombieClass == ZC_JOCKEY)
					position.z = g_pCurrentAimbot->GetAbsOrigin().z + 30.0f;
				else if (zombieClass == ZC_HUNTER && (g_pCurrentAimbot->GetFlags() & FL_DUCKING))
					position.z -= 12.0f;
			}

			if (position.IsValid())
			{
				// 备份原数据
				if (!oldViewAngles.IsValid())
				{
					oldViewAngles = pCmd->viewangles;
					oldSidemove = pCmd->sidemove;
					oldForwardmove = pCmd->fowardmove;
					oldUpmove = pCmd->upmove;
				}

				// 隐藏自动瞄准
				runAimbot = true;

				// 速度预测
				if (Config::bAimbotPred)
				{
					myOrigin = VelocityExtrapolate(client, myOrigin);
					position = VelocityExtrapolate(g_pCurrentAimbot, position);
				}

				// 将准星转到敌人头部
				pCmd->viewangles = CalculateAim(myOrigin, position);
				CorrectMovement(viewAngles, pCmd, pCmd->fowardmove, pCmd->sidemove);
			}
		}
#ifdef _DEBUG_OUTPUT
		else
		{
			g_interface.Engine->ClientCmd("echo \"m_flNextPrimaryAttack = %.2f | serverTime = %.2f\"",
				nextAttack, serverTime);
			g_interface.Engine->ClientCmd("echo \"interval_per_tick = %.2f | m_nTickBase = %d\"",
				g_interface.Globals->interval_per_tick, client->GetTickBase());
		}
#endif

		QAngle lastPunch(0.0f, 0.0f, 0.0f);
		QAngle currentPunch = client->GetLocalNetProp<QAngle>("m_vecPunchAngle");
		
		/*
		if (!lastPunch.IsValid())
		{
			lastPunch = QAngle(0.0f, 0.0f, 0.0f);

			Vector punchAngleVel = client->GetLocalNetProp<Vector>("m_vecPunchAngleVel");
			Utils::log("m_Local.m_vecPunchAngle = (%f, %f, %f)",
				currentPunch.x, currentPunch.y, currentPunch.z);

			Utils::log("m_Local.m_vecPunchAngleVel = (%f, %f, %f)",
				punchAngleVel.x, punchAngleVel.y, punchAngleVel.z);
		}
		*/

		if (Config::bSilentAimbot)
		{
			if (!runAimbot)
			{
				// 还原角度
				*bSendPacket = true;

				if (oldViewAngles.IsValid())
				{
					pCmd->viewangles = oldViewAngles;
					pCmd->sidemove = oldSidemove;
					pCmd->fowardmove = oldForwardmove;
					pCmd->upmove = oldUpmove;

					if (Config::bAimbotRCS)
					{
						QAngle newPunch(currentPunch.x - lastPunch.x, currentPunch.y - lastPunch.y, 0.0f);
						pCmd->viewangles.x -= newPunch.x * Config::fAimbotRCSX;
						pCmd->viewangles.y -= newPunch.y * Config::fAimbotRCSY;
					}
				}

				oldViewAngles.Invalidate();
			}
			else
			{
				// 修改角度
				*bSendPacket = false;

				// 后坐力控制系统
				if (Config::bAimbotRCS)
				{
					pCmd->viewangles.x -= currentPunch.x * Config::fAimbotRCSX;
					pCmd->viewangles.y -= currentPunch.y * Config::fAimbotRCSY;
				}
			}
		}

		lastPunch = currentPunch;
	}

end_aimbot:

	// 自动开枪
	if (Config::bTriggerBot && !(pCmd->buttons & IN_USE) && IsGunWeapon(weaponId))
	{
#ifdef _DEBUG_OUTPUT
		if (g_pCurrentAiming != nullptr)
		{
			if (!IsValidVictim(g_pCurrentAiming))
				g_interface.Engine->ClientCmd("echo \"aiming dead 0x%X\"", (DWORD)g_pCurrentAiming);
			if (g_pCurrentAiming->GetTeam() == client->GetTeam())
				g_interface.Engine->ClientCmd("echo \"aiming team 0x%X\"", (DWORD)g_pCurrentAiming);
			if (g_pCurrentAiming->GetClientClass()->m_ClassID == ET_INFECTED)
				g_interface.Engine->ClientCmd("echo \"aiming infected 0x%X\"", (DWORD)g_pCurrentAiming);
		}
#endif

#ifdef _DEBUG
		try
		{
#endif
			if (!IsValidVictim(g_pCurrentAiming) || g_iCurrentHitbox <= 0)
				goto end_trigger_bot;
#ifdef _DEBUG
		}
		catch (std::exception e)
		{
			Utils::log("%s (%d): %s | 0x%X", __FILE__, __LINE__, e.what(), (DWORD)g_pCurrentAiming);
			g_pCurrentAiming = nullptr;
			goto end_trigger_bot;
		}
		catch (...)
		{
			Utils::log("%s (%d): 未知异常 | 0x%X", __FILE__, __LINE__, (DWORD)g_pCurrentAiming);
			g_pCurrentAiming = nullptr;
			goto end_trigger_bot;
		}
#endif
		int classId = g_pCurrentAiming->GetClientClass()->m_ClassID;
		if (Config::bTriggerBotHead && IsCommonInfected(classId) &&
			(g_iCurrentHitbox < HITBOX_COMMON_1 || g_iCurrentHitbox > HITBOX_COMMON_4))
			goto end_trigger_bot;

		if (g_pCurrentAiming->GetTeam() != myTeam && classId != ET_WITCH &&					// 不攻击队友和 Witch
			g_pCurrentAiming->GetTeam() != 4 && (!IsSpecialInfected(classId) ||				// 不攻击 L4D1 生还者(因为他们是无敌的)
				!IsPlayerGhost(g_iCurrentAiming) || !IsGhostInfected(g_pCurrentAiming)) &&	// 不攻击幽灵状态的特感
				(myTeam == 2 || classId != ET_INFECTED))									// 特感队伍不攻击普感
			pCmd->buttons |= IN_ATTACK;
	}

end_trigger_bot:

	// 在开枪前检查，防止攻击队友
	if (Config::bAnitFirendlyFire && (pCmd->buttons & IN_ATTACK) && IsGunWeapon(weaponId) && myTeam == 2)
	{
		if (IsValidVictim(g_pCurrentAiming) &&
			g_pCurrentAiming->GetTeam() == myTeam &&
			!IsNeedRescue(g_pCurrentAiming) && !IsFallDown(client))
		{
			// 取消开枪
			pCmd->buttons &= ~IN_ATTACK;
		}
	}

	// 后坐力检测
	if (Config::bNoRecoil)
	{
		/*
		Vector punch = client->GetLocalNetProp<Vector>("m_vecPunchAngle");
		if(punch.IsValid() && punch.Length2D() > 0.0f)
			pCmd->viewangles -= punch * 2.0f;
		*/

		client->SetNetProp<int>("m_iShotsFired", 0, "DT_CSPlayer");
		client->SetLocalNetProp<QAngle>("m_vecPunchAngle", QAngle(0.0f, 0.0f, 0.0f));
	}

	// 无扩散 (扩散就是子弹射击时不在同一个点上)
	if (Config::bNoSpread)
	{
		if (IsGunWeapon(weaponId) && weapon != nullptr)
		{
			float spread = weapon->GetSpread();
			// int seed = SetPredictionRandomSeed(pCmd->random_seed);
			int seed = *g_pPredictionRandomSeed;
			*g_pPredictionRandomSeed = pCmd->random_seed;
			
			// Utils::log("oldSeed = %d, newSeed = %d", seed, pCmd->random_seed);

			float horizontal = 0.0f, vertical = 0.0f;
			SharedRandomFloat("CTerrorGun::FireBullet HorizSpread", -spread, spread, 0);
			__asm fstp horizontal;
			SharedRandomFloat("CTerrorGun::FireBullet VertSpread", -spread, spread, 0);
			__asm fstp vertical;

			// Utils::log("spread = %f, horizontal = %f, vertical = %f", spread, horizontal, vertical);
			
			// SetPredictionRandomSeed(seed);
			*g_pPredictionRandomSeed = seed;
			pCmd->viewangles.x -= horizontal;
			pCmd->viewangles.y -= vertical;
		}
	}

	// 引擎预测
	if (g_interface.MoveHelper)
	{
		// 结束预测
		// g_interface.Prediction->FinishMove(client, pCmd, &g_predMoveData);
		g_interface.GameMovement->FinishTrackPredictionErrors(client);
		g_interface.MoveHelper->SetHost(nullptr);
		// SetPredictionRandomSeed(-1);
		*g_pPredictionRandomSeed = -1;

		// 还原备份
		g_interface.Globals->curtime = oldCurtime;
		g_interface.Globals->frametime = oldFrametime;

		// 修复一些错误
		client->SetNetProp("m_fFlags", flags, "DT_BasePlayer");

		static bool showPred = true;
		if (showPred)
		{
			showPred = false;
			Utils::log("engine prediction success.");
		}
	}

	// 手枪连射 / Hunter 连续扑
	if (Config::bRapidFire && weapon != nullptr && (pCmd->buttons & IN_ATTACK))
	{
		static bool ignoreButton = true;
		if ((IsWeaponSingle(weaponId) || myTeam == 3) && !ignoreButton && nextAttack > serverTime)
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

	// 近战武器快速攻击
	if (myTeam == 2 && ((GetAsyncKeyState(VK_CAPITAL) & 0x8000) ||
		(GetAsyncKeyState(VK_XBUTTON2) & 0x8000) || 
		(Config::bMustFastMelee && (GetAsyncKeyState(VK_LBUTTON) & 0x8000))))
	{
		static enum FastMeleeStatus
		{
			FMS_None = 0,
			FMS_Primary = 1,
			FMS_Secondary = 2
		} fms = FMS_None;
		static unsigned int ignoreTick = 0;

		switch (fms)
		{
		case FMS_None:
			if (weaponId == Weapon_Melee && nextAttack <= serverTime)
			{
				// 近战武器攻击
				pCmd->buttons |= IN_ATTACK;
				fms = FMS_Primary;
			}
			else if (ignoreTick > 0)
				ignoreTick = 0;
			break;
		case FMS_Primary:
			if (weaponId == Weapon_Melee && nextAttack > serverTime)
			{
				// 在攻击之后切换到主武器
				g_interface.Engine->ClientCmd("lastinv");
				fms = FMS_Secondary;
			}
			else
				++ignoreTick;
			break;
		case FMS_Secondary:
			if (weaponId != Weapon_Melee)
			{
				// 在主武器时切换到近战武器
				g_interface.Engine->ClientCmd("lastinv");
				fms = FMS_None;
			}
			else
				++ignoreTick;
			break;
		}

		if (ignoreTick >= 10)
		{
			ignoreTick = 0;
			fms = FMS_None;

			g_interface.Engine->ClientCmd("echo \"fastmelee reset\"");
		}
	}

	// 修复角度不正确
	ClampAngles(pCmd->viewangles);
	AngleNormalize(pCmd->viewangles);

	if (Config::bAirStuck)
	{
		if (!(pCmd->buttons & IN_ATTACK))
		{
			// 使用 0xFFFFF 或者 16777216
			pCmd->tick_count = INT_MAX;
		}
	}

	if (Config::bTeleport)
	{
		// 一旦使用无法恢复
		pCmd->viewangles.z = 9e+37f;
	}

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

	oFrameStageNotify(stage);

	if (stage == FRAME_RENDER_START && g_interface.Engine->IsInGame())
	{
		// 在这里可以使用 DebugOverlay 来进行 3D 绘制
		sqb::FrameStageNotify(stage, g_interface.DebugOverlay);
	}

	static time_t nextUpdate = 0;
	time_t currentTime = time(NULL);
	if (nextUpdate <= currentTime)
	{
		// 计时，用于每隔 1 秒触发一次
		nextUpdate = currentTime + 1;
		static DWORD fmWait = 45;
		static bool connected = false;

		if (g_interface.Engine->IsConnected())
		{
			// 去除 CRC 验证
			if (Config::bCrcCheckBypass)
			{
#ifdef USE_CVAR_CHANGE
				CVAR_MAKE_FLAGS("sv_pure");
				CVAR_MAKE_FLAGS("sv_consistency");
				CVAR_MAKE_FLAGS("c_thirdpersonshoulder");
#endif

#ifdef USE_CVAR_CHANGE
				if (g_conVar["sv_pure"] != nullptr && g_conVar["sv_pure"]->GetInt() != 0)
					g_conVar["sv_pure"]->SetValue(0);
				if (g_conVar["sv_consistency"] != nullptr && g_conVar["sv_consistency"]->GetInt() != 0)
					g_conVar["sv_consistency"]->SetValue(0);
#endif

				if (Utils::readMemory<int>(g_iEngineBase + sv_pure) != 0 ||
					Utils::readMemory<int>(g_iEngineBase + sv_consistency) != 0)
				{
					Utils::writeMemory(0, g_iEngineBase + sv_pure);
					Utils::writeMemory(0, g_iEngineBase + sv_consistency);

					g_interface.Engine->ClientCmd("echo \"sv_pure and sv_consistency set %d\"",
						Utils::readMemory<int>(g_iEngineBase + sv_pure));
				}
			}

			if (!connected)
			{
				g_pGameRulesProxy = nullptr;
				g_pPlayerResource = nullptr;
				g_pCurrentAimbot = nullptr;
				g_pCurrentAiming = nullptr;
				g_iCurrentAiming = 0;

				Utils::log("*** connected ***");
			}

			connected = true;
		}
		else if (connected && !g_interface.Engine->IsInGame())
		{
			connected = false;
			g_pGameRulesProxy = nullptr;
			g_pPlayerResource = nullptr;
			g_pCurrentAimbot = nullptr;
			g_pCurrentAiming = nullptr;
			g_iCurrentAiming = 0;
			g_serverConVar.clear();

			Utils::log("*** disconnected ***");
		}

		static byte iExitGame = 0;
		if (GetAsyncKeyState(VK_END) & 0x8000)
		{
			// 按住 End 键三秒强制退出游戏游戏，用于游戏无响应
			if (++iExitGame >= 3)
				ExitProcess(0);

			if (g_pDrawRender != nullptr)
				g_pDrawRender->PushRenderText(DrawManager::WHITE, "exit proccess timer (%d/3)", iExitGame);
		}
		else if (iExitGame != 0)
		{
			// 按住不足 3 秒就放开则取消计时
			iExitGame = 0;
			if (g_pDrawRender != nullptr)
				g_pDrawRender->PushRenderText(DrawManager::WHITE, "exit proccess timer stopped");
		}

		// 快速离开当前服务器
		if (GetAsyncKeyState(VK_DELETE) & 0x01)
			g_interface.Engine->ClientCmd("disconnect");
	}
}

void __fastcall Hooked_RunCommand(CPrediction* _ecx, void* _edx, CBaseEntity* pEntity, CUserCmd* pCmd, CMoveHelper* moveHelper)
{
	oRunCommand(_ecx, pEntity, pCmd, moveHelper);

	static bool showHint = true;
	if (showHint || g_interface.MoveHelper == nullptr)
	{
		showHint = false;
		Utils::log("MoveHelperPointer = 0x%X", (DWORD)moveHelper);
	}

	g_interface.MoveHelper = moveHelper;
}

bool __fastcall Hooked_CreateMoveShared(ClientModeShared* _ecx, void* _edx, float flInputSampleTime, CUserCmd* pCmd)
{
	oCreateMoveShared(_ecx, flInputSampleTime, pCmd);

	if (g_interface.ClientMode == nullptr)
	{
		DETOURXS_DESTORY(g_pDetourCreateMove);
		g_interface.ClientMode = _ecx;
		if (g_interface.ClientModeHook)
		{
			g_interface.ClientModeHook->HookTable(false);
			g_interface.ClientModeHook.release();
		}

		g_interface.ClientModeHook = std::make_unique<CVMTHookManager>(g_interface.ClientMode);
		oCreateMoveShared = (FnCreateMoveShared)g_interface.ClientModeHook->HookFunction(indexes::SharedCreateMove, Hooked_CreateMoveShared);
		oKeyInput = (FnKeyInput)g_interface.ClientModeHook->HookFunction(indexes::KeyInput, Hooked_KeyInput);
		g_interface.ClientModeHook->HookTable(true);

		Utils::log("oCreateMoveShared = 0x%X", (DWORD)oCreateMoveShared);
		Utils::log("oKeyInput = 0x%X", (DWORD)oKeyInput);
		Utils::log("ClientMode = 0x%X", (DWORD)_ecx);
	}

	CBaseEntity* client = GetLocalClient();

	if (!client || !pCmd || pCmd->command_number == 0)
		return false;

	static bool showHint = true;
	if (showHint && bSendPacket)
	{
		showHint = false;
		Utils::log("Hooked_CreateMoveShared trigged");
		Utils::log("Input->pCmd = 0x%X", (DWORD)pCmd);
		Utils::log("CL_Move->bSendPacket = 0x%X | %d", (DWORD)bSendPacket, *bSendPacket);
	}

	if (!sqb::CreateMove(pCmd, (*bSendPacket ? SQTrue : SQFalse)))
		*bSendPacket = false;

	return false;
}

int __stdcall Hooked_InKeyEvent(int eventcode, ButtonCode_t keynum, const char *pszCurrentBinding)
{
	return oInKeyEvent(eventcode, keynum, pszCurrentBinding);
}

int __fastcall Hooked_KeyInput(ClientModeShared* _ecx, void* _edx, int down, ButtonCode_t keynum, const char* pszCurrentBinding)
{
	int result = oKeyInput(_ecx, down, keynum, pszCurrentBinding);

	static bool showHint = true;
	if (showHint)
	{
		showHint = false;
		Utils::log("Hooked_KeyInput trigged.");
	}

	if (!down)
	{
		static bool oldWireframe = false;
		if (keynum == KEY_INSERT)
			Config::bCvarWireframe = !Config::bCvarWireframe;

		if (oldWireframe != Config::bCvarWireframe)
		{
#ifdef USE_CVAR_CHANGE
			CVAR_MAKE_FLAGS("r_drawothermodels");
			CVAR_MAKE_FLAGS("cl_drawshadowtexture");
#endif

#ifdef USE_CVAR_CHANGE
			if (g_conVar["r_drawothermodels"] != nullptr && g_conVar["cl_drawshadowtexture"] != nullptr)
			{
				if (Config::bCvarWireframe)
				{
					g_conVar["r_drawothermodels"]->SetValue(2);
					g_conVar["cl_drawshadowtexture"]->SetValue(1);
				}
				else
				{
					g_conVar["r_drawothermodels"]->SetValue(1);
					g_conVar["cl_drawshadowtexture"]->SetValue(0);
				}
			}
			else
			{
#endif
				if (Config::bCvarWireframe)
				{
					Utils::writeMemory(2, g_iClientBase + r_drawothermodels);
					Utils::writeMemory(1, g_iClientBase + cl_drawshadowtexture);
				}
				else
				{
					Utils::writeMemory(1, g_iClientBase + r_drawothermodels);
					Utils::writeMemory(0, g_iClientBase + cl_drawshadowtexture);
				}

				g_interface.Engine->ClientCmd("echo \"r_drawothermodels set %d\"",
					Utils::readMemory<int>(g_iClientBase + r_drawothermodels));
				g_interface.Engine->ClientCmd("echo \"cl_drawshadowtexture set %d\"",
					Utils::readMemory<int>(g_iClientBase + cl_drawshadowtexture));
#ifdef USE_CVAR_CHANGE
			}
#endif
			oldWireframe = Config::bCvarWireframe;
		}

		static bool oldFullBright = false;
		if (keynum == KEY_HOME)
			Config::bCvarFullBright = !Config::bCvarFullBright;

		if (oldFullBright != Config::bCvarFullBright)
		{
#ifdef USE_CVAR_CHANGE
			CVAR_MAKE_FLAGS("mat_fullbright");
#endif

#ifdef USE_CVAR_CHANGE
			if (g_conVar["mat_hdr_enabled"] != nullptr && g_conVar["mat_hdr_level"] != nullptr)
			{
				if (Config::bCvarFullBright)
				{
					g_conVar["mat_hdr_enabled"]->SetValue(0);
					g_conVar["mat_hdr_level"]->SetValue(0);
				}
				else
				{
					g_conVar["mat_hdr_enabled"]->SetValue(1);
					g_conVar["mat_hdr_level"]->SetValue(2);
				}
			}
			else if (g_conVar["mat_fullbright"] != nullptr)
			{
				if (Config::bCvarFullBright)
					g_conVar["mat_fullbright"]->SetValue(1);
				else
					g_conVar["mat_fullbright"]->SetValue(0);
			}
			else
			{
#endif
				if (Config::bCvarFullBright)
					Utils::writeMemory(1, g_iMaterialModules + mat_fullbright);
				else
					Utils::writeMemory(0, g_iMaterialModules + mat_fullbright);

				g_interface.Engine->ClientCmd("echo \"mat_fullbright set %d\"",
					Utils::readMemory<int>(g_iMaterialModules + mat_fullbright));
#ifdef USE_CVAR_CHANGE
			}
#endif
			oldFullBright = Config::bCvarFullBright;
		}

		static bool oldGameMode = false;

		if (keynum == KEY_PAGEUP)
			Config::bCvarGameMode = !Config::bCvarGameMode;

		if (oldGameMode != Config::bCvarGameMode)
		{
#ifdef USE_CVAR_CHANGE
			CVAR_MAKE_FLAGS("mp_gamemode");
#endif

#ifdef USE_CVAR_CHANGE
			if (g_conVar["mp_gamemode"] != nullptr)
			{
				const char* mode = g_conVar["mp_gamemode"]->GetString();
				if (_strcmpi(mode, "versus") == 0 || _strcmpi(mode, "realismversus") == 0)
				{
					g_conVar["mp_gamemode"]->SetValue("coop");
					strcpy_s(g_conVar["mp_gamemode"]->m_pszString, g_conVar["mp_gamemode"]->m_StringLength, "coop");
				}
				else
				{
					g_conVar["mp_gamemode"]->SetValue("versus");
					strcpy_s(g_conVar["mp_gamemode"]->m_pszString, g_conVar["mp_gamemode"]->m_StringLength, "versus");
				}

				g_interface.Engine->ClientCmd("echo \"[ConVar] mp_gamemode set %s\"",
					g_conVar["mp_gamemode"]->GetString());
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

					g_interface.Engine->ClientCmd("echo \"mp_gamemode set %s\"", mode);
				}
#ifdef USE_CVAR_CHANGE
			}
#endif
			oldGameMode = Config::bCvarGameMode;
		}

		static bool oldCheats = false;
		if (keynum == KEY_PAGEDOWN)
			Config::bCvarCheats = !Config::bCvarCheats;

		if (oldCheats != Config::bCvarCheats)
		{
#ifdef USE_CVAR_CHANGE
			CVAR_MAKE_FLAGS("sv_cheats");
#endif

#ifdef USE_CVAR_CHANGE
			if (g_conVar["sv_cheats"] != nullptr)
			{
				if (Config::bCvarCheats)
				{
					g_conVar["sv_cheats"]->SetValue(1);
					g_conVar["sv_cheats"]->m_fValue = 1.0f;
					g_conVar["sv_cheats"]->m_nValue = 1;
				}
				else
					g_conVar["sv_cheats"]->SetValue(0);

				g_interface.Engine->ClientCmd("echo \"[ConVar] sv_cheats set %d\"",
					g_conVar["sv_cheats"]->GetInt());
			}
#endif

			if (Config::bCvarCheats)
				Utils::writeMemory(1, g_iEngineBase + sv_cheats);
			else
				Utils::writeMemory(0, g_iEngineBase + sv_cheats);

			g_interface.Engine->ClientCmd("echo \"sv_cheats set %d\"",
				Utils::readMemory<int>(g_iEngineBase + sv_cheats));

			oldCheats = Config::bCvarCheats;
		}

		static bool oldThrid = false;
		if (oldThrid != Config::bThirdPersons)
		{
			thirdPerson(Config::bThirdPersons);
			oldThrid = Config::bThirdPersons;
		}

		/*
		if (keynum == KEY_PAGEUP)
		{
			if (g_interface.Input->CAM_IsThirdPerson())
				g_interface.Input->CAM_ToFirstPerson();
			else
				g_interface.Input->CAM_ToThirdPerson();
		}
		*/
		
		// 显示全部玩家
		/*
		if (keynum == KEY_CAPSLOCK)
			showSpectator();
		*/

		// 打开/关闭 自动连跳的自动保持速度
		if (keynum == KEY_F8)
		{
			Config::bAutoStrafe = !Config::bAutoStrafe;
			g_interface.Engine->ClientCmd("echo \"[segt] auto strafe set %s\"",
				(Config::bAutoStrafe ? "enable" : "disabled"));

			if (g_pDrawRender != nullptr)
			{
				g_pDrawRender->PushRenderText(DrawManager::WHITE, "auto strafe %s",
					(Config::bAutoStrafe ? "enable" : "disable"));
			}
		}

		// 打开/关闭 自动开枪
		if (keynum == KEY_F9)
		{
			Config::bTriggerBot = !Config::bTriggerBot;
			g_interface.Engine->ClientCmd("echo \"[segt] trigger bot set %s\"",
				(Config::bTriggerBot ? "enable" : "disabled"));

			if (g_pDrawRender != nullptr)
			{
				g_pDrawRender->PushRenderText(DrawManager::WHITE, "trigger bot %s",
					(Config::bTriggerBot ? "enable" : "disable"));
			}
		}

		// 打开/关闭 自动瞄准
		if (keynum == KEY_F10)
		{
			Config::bAimbot = !Config::bAimbot;
			g_interface.Engine->ClientCmd("echo \"[segt] aim bot set %s\"",
				(Config::bAimbot ? "enable" : "disabled"));

			if (g_pDrawRender != nullptr)
			{
				g_pDrawRender->PushRenderText(DrawManager::WHITE, "auto aim %s",
					(Config::bAimbot ? "enable" : "disable"));
			}
		}

		// 打开/关闭 空格自动连跳
		if (keynum == KEY_F11)
		{
			Config::bBunnyHop = !Config::bBunnyHop;
			g_interface.Engine->ClientCmd("echo \"[segt] auto bunnyHop set %s\"",
				(Config::bBunnyHop ? "enable" : "disabled"));

			if (g_pDrawRender != nullptr)
			{
				g_pDrawRender->PushRenderText(DrawManager::WHITE, "auto bhop %s",
					(Config::bBunnyHop ? "enable" : "disable"));
			}
		}

		// 打开/关闭 静音瞄准
		if (keynum == KEY_F12)
		{
			Config::bSilentAimbot = !Config::bSilentAimbot;
			g_interface.Engine->ClientCmd("echo \"[segt] silent aim %s\"",
				(Config::bSilentAimbot ? "enable" : "disabled"));

			if (g_pDrawRender != nullptr)
			{
				g_pDrawRender->PushRenderText(DrawManager::WHITE, "silent aimbot %s",
					(Config::bSilentAimbot ? "enable" : "disable"));
			}
		}

		// 打开/关闭 速砍
		if (keynum == KEY_F7)
		{
			Config::bMustFastMelee = !Config::bMustFastMelee;
			g_interface.Engine->ClientCmd("echo \"[segt] fast melee %s\"",
				(Config::bMustFastMelee ? "enable" : "disabled"));

			if (g_pDrawRender != nullptr)
			{
				g_pDrawRender->PushRenderText(DrawManager::WHITE, "fast melee %s",
					(Config::bMustFastMelee ? "enable" : "disable"));
			}
		}
	}
	else
	{
		if (keynum == KEY_PAD_PLUS)
		{
			Config::fAimbotFov += 1.0f;
			g_interface.Engine->ClientCmd("echo \"aimbot fov set %.2f\"", Config::fAimbotFov);

			if (g_pDrawRender != nullptr)
				g_pDrawRender->PushRenderText(DrawManager::WHITE, "aimbot angles set %.0f", Config::fAimbotFov);
		}

		if (keynum == KEY_PAD_MINUS)
		{
			Config::fAimbotFov -= 1.0f;
			g_interface.Engine->ClientCmd("echo \"aimbot fov set %.2f\"", Config::fAimbotFov);

			if (g_pDrawRender != nullptr)
				g_pDrawRender->PushRenderText(DrawManager::WHITE, "aimbot angles set %.0f", Config::fAimbotFov);
		}
	}

	sqb::KeyInput(keynum, down > 0, pszCurrentBinding);

	return result;
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

	// 去除屏幕摇晃和黑屏效果
	if (msg_id == 11 || msg_id == 12)
		return true;

	return oDispatchUserMessage(msg_id, msg_data);
}

void __stdcall Hooked_CL_Move(float accumulated_extra_samples, bool bFinalTick)
{
	DWORD _edi;
	BYTE _bl;

	__asm
	{
		mov		_edi, edi
		mov		_bl, bl
	};

	static bool showHint = true;
	if (showHint)
	{
		showHint = false;
		Utils::log("Hooked_CL_Move trigged.");
	}

	auto CL_Move = [&_bl, &_edi](float accumulated_extra_samples, bool bFinalTick) -> void
	{
		// 不支持 push byte. 所以只能 push word
		DWORD wFinalTick = bFinalTick;

		__asm
		{
			// 寄存器传参
			mov		bl, _bl
			mov		edi, _edi

			// 堆栈传参
			push	wFinalTick
			push	accumulated_extra_samples

			// 调用原函数(其实是个蹦床)
			call	oCL_Move

			// 清理堆栈(需要内存对齐)
			add		esp, 8
		};
	};

	/*
	if (g_interface.ClientMode == nullptr)
	{
		g_interface.ClientMode = GetClientModeNormal();
		g_interface.ClientModeHook = std::make_unique<CVMTHookManager>(g_interface.ClientMode);
		oCreateMoveShared = (FnCreateMoveShared)g_interface.ClientModeHook->HookFunction(indexes::SharedCreateMove, Hooked_CreateMoveShared);
		g_interface.ClientModeHook->HookTable(true);
		Utils::log("g_pClientMode = 0x%X", (DWORD)g_interface.ClientMode);
		Utils::log("ClientModeShared::CreateMove = client.dll + 0x%X", (DWORD)oCreateMoveShared + g_iClientBase);
		Utils::log("m_pChatElement = 0x%X", (DWORD)g_interface.ClientMode->GetHudChat());
	}
	*/

	// 默认的 1 次调用，如果不调用会导致游戏冻结
	// 参数 bFinalTick 相当于 bSendPacket
	CL_Move(accumulated_extra_samples, bFinalTick);

	if (g_bIsKeyPressing[VK_CAPITAL])
	{
		static bool showSpeed = true;
		if (showSpeed)
		{
			showSpeed = false;
			Utils::log("SpeedHack speed = %d", g_iSpeedMultiple);
		}

		for (int i = 1; i < g_iSpeedMultiple; ++i)
		{
			// 多次调用它会有加速效果
			CL_Move(accumulated_extra_samples, bFinalTick);
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

void __stdcall Hooked_DrawModelExecute(const DrawModelState_t &state,
	const ModelRenderInfo_t &pInfo, matrix3x4_t *pBoneToWorld)
{
	DWORD _ecx, _ebx;
	__asm
	{
		mov		_ecx, ecx
		mov		_ebx, ebx
	};

	auto DrawModelExecute = [&_ebx, &_ecx](const DrawModelState_t &state,
		const ModelRenderInfo_t &pInfo, matrix3x4_t *pBoneToWorld)
	{
		__asm
		{
			mov		ebx, _ebx
			mov		ecx, _ecx

			push	pBoneToWorld
			push	pInfo
			push	state

			call oDrawModelExecute
		};
	};

	DrawModelExecute(state, pInfo, pBoneToWorld);
	g_interface.ModelRender->ForcedMaterialOverride(nullptr);
}

void __fastcall Hooked_EnginePaint(CEngineVGui *_ecx, void *_edx, PaintMode_t mode)
{
	static bool showHint = true;
	if (showHint)
	{
		showHint = false;
		Utils::log("Hooked_EnginePaint trigged.");
	}
	
	oEnginePaint(_ecx, mode);

	if (mode & PAINT_UIPANELS)
	{
		PaintStartDrawing(g_interface.Surface);

		// 在这里绘制东西比 PaintTraverse 更快

#ifndef USE_D3D_DRAW
		{
			CBaseEntity* local = GetLocalClient();
			if (local == nullptr || !g_interface.Engine->IsInGame())
				goto finish_draw;

			// 目前最小距离
			float distmin = 65535.0f;

			// 当前队伍
			int team = local->GetTeam();

			// 当前是否有自动瞄准的目标
			bool targetSelected = false;
			bool specialSelected = false;

#ifdef _DEBUG
			try
			{
#endif
				targetSelected = IsValidVictim(g_pCurrentAimbot);
#ifdef _DEBUG
			}
			catch (std::exception e)
			{
				Utils::log("%s (%d): %s | 0x%X", __FILE__, __LINE__, e.what(), (DWORD)g_pCurrentAimbot);
				targetSelected = false;
				g_pCurrentAimbot = nullptr;
			}
			catch (...)
			{
				Utils::log("%s (%d): 未知异常 | 0x%X", __FILE__, __LINE__, (DWORD)g_pCurrentAimbot);
				targetSelected = false;
				g_pCurrentAimbot = nullptr;
			}
#endif

			if (!targetSelected || !(GetAsyncKeyState(VK_LBUTTON) & 0x8000))
				g_pCurrentAimbot = nullptr;

			Vector myViewAngles;
			g_interface.Engine->GetViewAngles(myViewAngles);

			Vector myEyeOrigin = local->GetEyePosition();
			Vector myOrigin = local->GetAbsOrigin();

			// 一般普感实体索引上限 512 就够了，太大会卡的
			int maxEntity = g_interface.ClientEntList->GetHighestEntityIndex();

			// 绘制颜色
			D3DCOLOR color = 0;

			// 观众数量
			byte spectatorCount = 0;

			// 用于格式化字符串
			std::stringstream ss;
			ss.sync_with_stdio(false);
			ss.tie(nullptr);
			ss.setf(std::ios::fixed);
			ss.precision(0);

			if (Config::bDrawCrosshairs)
			{
				int width, height;
				g_interface.Engine->GetScreenSize(width, height);

				int classId = (!IsValidVictim(g_pCurrentAiming) ? ET_INVALID : g_pCurrentAiming->GetClientClass()->m_ClassID);
				if (classId == ET_INVALID)
					color = DrawManager::LAWNGREEN;
				else if (g_pCurrentAiming->GetTeam() == local->GetTeam())
					color = DrawManager::BLUE;
				else if (classId == ET_INFECTED)
					color = DrawManager::ORANGE;
				else if (classId == ET_WITCH)
					color = DrawManager::PINK;
				else
					color = DrawManager::RED;

				width /= 2;
				height /= 2;

				g_pDrawRender->AddLine(color, width - 10, height, width + 10, height);
				g_pDrawRender->AddLine(color, width, height - 10, width, height + 10);
			}

			// 0 为 worldspawn，没有意义
			for (int i = 1; i <= maxEntity; ++i)
			{
				CBaseEntity* entity = g_interface.ClientEntList->GetClientEntity(i);
				int classId = ET_INVALID;

				try
				{
					if (entity == nullptr || entity->IsDormant() || (DWORD)entity == (DWORD)local)
						continue;

					classId = entity->GetClientClass()->m_ClassID;
					if (g_pGameRulesProxy == nullptr && classId == ET_TerrorGameRulesProxy)
					{
						g_pGameRulesProxy = entity;
						Utils::log("TerrorGameRulesProxy Entity found 0x%X", (DWORD)g_pGameRulesProxy);
					}

					if (g_pPlayerResource == nullptr && classId == ET_TerrorPlayerResource)
					{
						g_pPlayerResource = entity;
						Utils::log("TerrorPlayerResource Entity found 0x%X", (DWORD)g_pPlayerResource);
					}

					if ((DWORD)g_pCurrentAiming == (DWORD)entity)
						g_iCurrentAiming = i;

#ifdef _DEBUG_OUTPUT_
					if (IsSurvivor(classId) || IsSpecialInfected(classId) || IsCommonInfected(classId))
					{
#endif
						// 检查 生还者/特感/普感 是否是有效的
						if (!entity->IsAlive())
						{
							if (Config::bDrawSpectator)
							{
								ss.str("");

								int team = entity->GetTeam();
								if (team == 1 || team == 2 || team == 3)
								{
									int obsMode = entity->GetNetProp<int>("m_iObserverMode", "DT_BasePlayer");
									if (obsMode == OBS_MODE_IN_EYE || obsMode == OBS_MODE_CHASE)
									{
										CBaseEntity* obsTarget = (CBaseEntity*)entity->GetNetProp<CBaseHandle*>("m_hObserverTarget", "DT_BasePlayer");
										if (obsTarget != nullptr)
											obsTarget = g_interface.ClientEntList->GetClientEntityFromHandle((CBaseHandle*)obsTarget);
										if (obsTarget != nullptr && (DWORD)obsTarget == (DWORD)local)
										{
											player_info_t info;
											g_interface.Engine->GetPlayerInfo(i, &info);

											if (obsMode == OBS_MODE_IN_EYE)
												ss << info.name << " [1st]" << std::endl;
											else if (obsMode == OBS_MODE_CHASE)
												ss << info.name << " [3rd]" << std::endl;
										}
									}
								}

								if (!ss.str().empty())
								{
									switch (team)
									{
									case 1:
										// 观察者
										color = DrawManager::WHITE;
										break;
									case 2:
										// 生还者
										color = DrawManager::SKYBLUE;
										break;
									case 3:
										// 感染者
										color = DrawManager::RED;
										break;
									case 4:
										// 非玩家生还者
										color = DrawManager::PURPLE;
										break;
									}

									int width, height;
									g_interface.Engine->GetScreenSize(width, height);
									g_pDrawRender->AddText(color,
										width * 0.75f, (height * 0.75f) +
										(spectatorCount++ * g_pDrawRender->GetFontSize()),
										false, ss.str().c_str());
								}
							}

							continue;
						}
#ifdef _DEBUG_OUTPUT_
					}
#endif
				}
				catch (std::exception e)
				{
					// Utils::log("%s (%d): entity %d %s", __FILE__, __LINE__, i, e.what());
					continue;
				}
				catch (...)
				{
					// Utils::log("%s (%d): entity %d 未知异常 -> 0x%X", __FILE__, __LINE__, i, (DWORD)entity);
					continue;
				}

				Vector head, foot, headbox, origin;

				// 目标脚下的位置
				origin = (IsSurvivor(classId) || IsSpecialInfected(classId) ?
					entity->GetAbsOrigin() :		// 玩家专用，其他实体是没有的
					entity->GetNetProp<Vector>("m_vecOrigin", "DT_BaseCombatCharacter"));

				// 目标的头部的位置
				headbox = (IsSurvivor(classId) || IsSpecialInfected(classId) || IsCommonInfected(classId) ?
					GetHeadHitboxPosition(entity) : origin);

				// 检查目标是否在屏幕内
				if (!headbox.IsValid() || !WorldToScreen(headbox, head) ||
					!WorldToScreen(origin, foot))
					continue;

				// 目标是否可见
				bool visible = IsTargetVisible(entity, headbox, myEyeOrigin);

				// 目标与自己的距离
				float dist = myOrigin.DistTo(origin);

				// 获取方框的大小
				float height = fabs(head.y - foot.y);
				float width = height * 0.65f;

				// 给玩家绘制一个框
				if (Config::bDrawBox)
				{
					if (IsSurvivor(classId) || IsSpecialInfected(classId))
					{
						color = (entity->GetTeam() == team ? DrawManager::BLUE : DrawManager::RED);
						if (IsSurvivor(classId))
						{
							if (entity->GetNetProp<byte>("m_bIsOnThirdStrike", "DT_TerrorPlayer") != 0)
							{
								// 黑白状态 - 白色
								color = DrawManager::WHITE;
							}
							else if (IsControlled(entity))
							{
								// 被控 - 橙色
								color = DrawManager::ORANGE;
							}
							else if (IsIncapacitated(entity))
							{
								// 倒地挂边 - 黄色
								color = DrawManager::YELLOW;
							}
						}
						else if (IsSpecialInfected(classId) && IsGhostInfected(entity))
						{
							// 幽灵状态 - 紫色
							color = DrawManager::PURPLE;
						}

						// 绘制一个框（虽然这个框只有上下两条线）

						// g_pDrawRender->AddRect(color, foot.x - width / 2, foot.y, width, -height);
						g_pDrawRender->AddCorner(color, foot.x - width / 2, foot.y, width, -height);
					}
					else if (IsCommonInfected(classId))
					{
						// 这只是普感而已，太远了没必要显示出来
						if (dist > 3000.0f)
							continue;

						int size = 2;
						if (dist < 1000.0f)
							size = 4;

						color = DrawManager::GREEN;
						if (classId == ET_WITCH)
							color = DrawManager::PINK;

						// 画一个小方形，以标记为头部
						/*
						g_cInterfaces.Surface->FillRGBA(head.x, head.y, size, size,
						(color & 0xFF0000) >> 16, (color & 0xFF00) >> 8, color & 0xFF,
						(color & 0xFF000000) >> 24);
						*/

						if (visible)
							g_pDrawRender->AddFillCircle(color, head.x, head.y, size, 8);
						else
							g_pDrawRender->AddCircle(color, head.x, head.y, size, 8);
					}
					else
					{
						// 其他东西
						if (dist > 1000.0f)
							continue;

						if (classId == ET_TankRock)
						{
							// Tank 的石头
							g_pDrawRender->AddCircle(DrawManager::PURPLE, foot.x, foot.y, 9, 8);
						}
						else if (classId == ET_ProjectilePipeBomb || classId == ET_ProjectileMolotov ||
							classId == ET_ProjectileVomitJar)
						{
							// 投掷武器飞行物
							g_pDrawRender->AddCircle(DrawManager::BLUE, foot.x, foot.y, 5, 8);
						}
						else if (classId == ET_ProjectileSpitter || classId == ET_ProjectileGrenadeLauncher)
						{
							// Spitter 的酸液 和 榴弹发射器的榴弹
							g_pDrawRender->AddCircle(DrawManager::GREEN, foot.x, foot.y, 5, 8);
						}
					}
				}

				if (Config::bDrawBone)
				{
					if ((team == 2 && entity->GetTeam() == 3) || (team == 3 && entity->GetTeam() == 2) ||
						(IsCommonInfected(classId) && dist < 1500.0f))
					{
						color = DrawManager::WHITE;
						studiohdr_t* hdr = g_interface.ModelInfo->GetStudioModel(entity->GetModel());
						if (hdr != nullptr && IsValidVictimId(classId))
						{
							Vector parent, child, screenParent, screenChild;
							for (int i = 0; i < hdr->numbones; ++i)
							{
								mstudiobone_t* bone = hdr->pBone(i);
								if (bone == nullptr || !(bone->flags & 0x100) || bone->parent == -1)
									continue;

								child = entity->GetBonePosition(i);
								parent = entity->GetBonePosition(bone->parent);
								if (child.IsValid() && parent.IsValid() &&
									WorldToScreen(parent, screenParent) && WorldToScreen(child, screenChild))
								{
									g_pDrawRender->AddLine(color, screenParent.x, screenParent.y,
										screenChild.x, screenChild.y);
								}
							}
						}
					}
				}

				if (Config::bDrawName)
				{
					ss.str("");

					// 根据类型决定绘制的内容
					if (IsSurvivor(classId) || IsSpecialInfected(classId))
					{
						player_info_t info;
						g_interface.Engine->GetPlayerInfo(i, &info);

						// 检查是否为生还者
						if (IsSurvivor(classId))
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
							if (IsGhostInfected(entity))
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

						// 玩家名字
						ss << info.name;

						// 显示距离
						ss << std::endl << (int)dist;

						CBaseEntity* weapon = (CBaseEntity*)entity->GetActiveWeapon();
						if (weapon != nullptr)
							weapon = g_interface.ClientEntList->GetClientEntityFromHandle((CBaseHandle*)weapon);

						// 给生还者显示弹药
						if (IsSurvivor(classId))
						{
							if (Config::bDrawAmmo && weapon != nullptr)
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
										ss << " (" << clip << "/" <<
											entity->GetNetProp<int>("m_iAmmo", "DT_TerrorPlayer", (size_t)ammoType) <<
											")";
									}
								}
							}
						}
						else
						{
							if (!info.isBot)
							{
								// 如果特感不是机器人的话就显示特感类型
								// 机器人特感名字就是类型
								ss << " (" << GetZombieClassName(entity) << ")";
							}
						}
					}
					else if (classId == ET_WeaponSpawn)
					{
						// 其他东西
						if (dist > 1000.0f)
							continue;

						if (WeaponIdToAlias)
						{
							int weaponId = entity->GetNetProp<short>("m_weaponID", "DT_WeaponSpawn");
							if (IsWeaponT1(weaponId) && Config::bDrawT1Weapon ||
								IsWeaponT2(weaponId) && Config::bDrawT2Weapon ||
								IsWeaponT3(weaponId) && Config::bDrawT3Weapon ||
								IsMelee(weaponId) && Config::bDrawMeleeWeapon ||
								IsMedical(weaponId) && Config::bDrawMedicalItem ||
								IsGrenadeWeapon(weaponId) && Config::bDrawGrenadeItem ||
								IsAmmoStack(weaponId) && Config::bDrawAmmoStack)
								ss << WeaponIdToAlias(weaponId);
						}
					}
					else
					{
						if (dist > 1000.0f)
							continue;

						if (classId == ET_WeaponAmmoPack || classId == ET_WeaponAmmoSpawn)
							ss << "ammo_stack";
						else if (classId == ET_WeaponPipeBomb)
							ss << "pipe_bomb";
						else if (classId == ET_WeaponMolotov)
							ss << "molotov";
						else if (classId == ET_WeaponVomitjar)
							ss << "vomitjar";
						else if (classId == ET_WeaponFirstAidKit)
							ss << "first_aid_kit";
						else if (classId == ET_WeaponDefibrillator)
							ss << "defibrillator";
						else if (classId == ET_WeaponPainPills)
							ss << "pain_pills";
						else if (classId == ET_WeaponAdrenaline)
							ss << "adrenaline";
						else if (classId == ET_WeaponMelee)
							ss << "melee";
						else if (classId == ET_WeaponMagnum)
							ss << "pistol_magnum";
					}

					if (!ss.str().empty())
					{
						color = (visible ? DrawManager::LAWNGREEN : DrawManager::YELLOW);

						g_pDrawRender->AddText(color, foot.x, head.y, true, ss.str().c_str());
					}
				}

				// 自动瞄准寻找目标
				if (Config::bAimbot && (!targetSelected || !(g_pUserCommands->buttons & IN_ATTACK)) &&
					((team == 2 && (IsSpecialInfected(classId) || classId == ET_INFECTED)) ||
					(team == 3 && IsSurvivor(classId))))
				{
					// 已经选择过目标了，并且这是一个不重要的敌人
					if (classId == ET_INFECTED && specialSelected)
						continue;

					// 选择一个最接近的特感，因为特感越近对玩家来说越危险
					if (entity->GetTeam() != team && dist < distmin && visible &&
						GetAnglesFieldOfView(myViewAngles, CalculateAim(myEyeOrigin, headbox)) <=
						Config::fAimbotFov)
					{
						g_pCurrentAimbot = entity;
						distmin = dist;

						if (IsSpecialInfected(classId))
							specialSelected = true;
					}
				}
			}
		}
	finish_draw:
#endif

		sqb::Paint(mode);

		PaintFinishDrawing(g_interface.Surface);
	}
}

bool __fastcall Hooked_EngineKeyEvent(CEngineVGui *_ecx, void *_edx, const InputEvent_t &event)
{
	static bool showHint = true;
	if (showHint)
	{
		showHint = false;
		Utils::log("Hooked_EngineKeyEvent trigged.");
	}
	
	bool result = oEngineKeyEvent(_ecx, event);

	bool isDown = event.m_nType != IE_ButtonReleased;
	ButtonCode_t code = (ButtonCode_t)event.m_nData;
	
	return result;
}

bool __fastcall Hooked_ProcessGetCvarValue(CBaseClientState *_ecx, void *_edx, SVC_GetCvarValue *msg)
{
	/*
	if (g_interface.ClientState == nullptr)
	{
		DETOURXS_DESTORY(g_pDetourGetCvarValue);
		DETOURXS_DESTORY(g_pDetourSetConVar);
		g_interface.ClientState = _ecx;

		if (g_interface.ClientStateHook)
		{
			g_interface.ClientStateHook->HookTable(false);
			g_interface.ClientStateHook.release();
		}

		g_interface.ClientStateHook = std::make_unique<CVMTHookManager>(g_interface.ClientState);
		oProcessGetCvarValue = (FnProcessGetCvarValue)g_interface.ClientStateHook->HookFunction(indexes::ProcessGetCvarValue, Hooked_ProcessGetCvarValue);
		oProcessSetConVar = (FnProcessSetConVar)g_interface.ClientStateHook->HookFunction(indexes::ProcessSetConVar, Hooked_ProcessSetConVar);
		g_interface.ClientStateHook->HookTable(true);

		Utils::log("oProcessGetCvarValue = 0x%X", (DWORD)oProcessGetCvarValue);
		Utils::log("oProcessSetConVar = 0x%X", (DWORD)oProcessSetConVar);
		Utils::log("ClientState = 0x%X", (DWORD)_ecx);
	}
	*/
	
	CLC_RespondCvarValue returnMsg;
	returnMsg.m_iCookie = msg->m_iCookie;
	returnMsg.m_szCvarName = msg->m_szCvarName;
	returnMsg.m_szCvarValue = "";
	returnMsg.m_eStatusCode = eQueryCvarValueStatus_ValueIntact;

	char tempValue[256];
	ConVar* cvar = g_interface.Cvar->FindVar(msg->m_szCvarName);
	tempValue[0] = '\0';

	if (cvar != nullptr)
	{
		if (cvar->IsFlagSet(FCVAR_SERVER_CANNOT_QUERY))
		{
			// 这玩意不可以查询
			returnMsg.m_eStatusCode = eQueryCvarValueStatus_ValueIntact;
		}
		else
		{
			returnMsg.m_eStatusCode = eQueryCvarValueStatus_ValueIntact;

			if (g_serverConVar.find(msg->m_szCvarName) != g_serverConVar.end())
			{
				// 把服务器设置过的 Cvar 发送回去
				strcpy_s(tempValue, g_serverConVar[msg->m_szCvarName].c_str());
				returnMsg.m_szCvarValue = tempValue;
			}
			else
			{
				// 发送默认的 Cvar 给服务端，而不是自己的
				// 服务器一般情况下不会发现问题的...
				strcpy_s(tempValue, cvar->GetDefault());
				returnMsg.m_szCvarValue = tempValue;
			}
		}

		const SQChar* s = sqb::ProcessGetCvarValue(returnMsg.m_szCvarName, cvar->GetString());
		if (s != nullptr)
		{
			strcpy_s(tempValue, s);
			returnMsg.m_szCvarValue = tempValue;
		}
	}
	else
	{
		// 服务器查询了一个不存在的 Cvar
		returnMsg.m_eStatusCode = eQueryCvarValueStatus_CvarNotFound;
		returnMsg.m_szCvarValue = tempValue;
	}

	Utils::log("[GCV] %s returns %s", returnMsg.m_szCvarName, returnMsg.m_szCvarValue);

	// 把查询结果发送回去
	msg->GetNetChannel()->SendNetMsg(returnMsg);

	return true;
}

bool __fastcall Hooked_ProcessSetConVar(CBaseClientState *_ecx, void *_edx, NET_SetConVar *msg)
{
	if (g_interface.ClientState == nullptr)
	{
		DETOURXS_DESTORY(g_pDetourGetCvarValue);
		DETOURXS_DESTORY(g_pDetourSetConVar);
		g_interface.ClientState = _ecx;

		if (g_interface.ClientStateHook)
		{
			g_interface.ClientStateHook->HookTable(false);
			g_interface.ClientStateHook.release();
		}

		g_interface.ClientStateHook = std::make_unique<CVMTHookManager>(g_interface.ClientState);
		oProcessGetCvarValue = (FnProcessGetCvarValue)g_interface.ClientStateHook->HookFunction(indexes::ProcessGetCvarValue, Hooked_ProcessGetCvarValue);
		oProcessSetConVar = (FnProcessSetConVar)g_interface.ClientStateHook->HookFunction(indexes::ProcessSetConVar, Hooked_ProcessSetConVar);
		g_interface.ClientStateHook->HookTable(true);

		Utils::log("oProcessGetCvarValue = 0x%X", (DWORD)oProcessGetCvarValue);
		Utils::log("oProcessSetConVar = 0x%X", (DWORD)oProcessSetConVar);
		Utils::log("ClientState = 0x%X", (DWORD)_ecx);
	}
	
	for (auto& cvar : msg->m_ConVars)
	{
		if (cvar.name[0] == '\0')
			continue;
		
		if (g_serverConVar.find(cvar.name) != g_serverConVar.end())
		{
			// 修改已经改过的值
			Utils::log("[SCV] %s changing from %s to %s", cvar.name, g_serverConVar[cvar.name].c_str(), cvar.value);
		}
		else
		{
			// 第一次设置
			Utils::log("[SCV] %s setting to %s", cvar.name, cvar.value);
		}

		const SQChar* s = sqb::ProcessSetConVar(cvar.name, cvar.value);
		if (s != nullptr)
		{
			strcpy_s(cvar.value, s);
		}

		// 将服务器的更改进行记录，等服务器查询的时候把记录还回去
		g_serverConVar[cvar.name] = cvar.value;
	}

	return true;
}
