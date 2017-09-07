#include "main.h"

#define USE_PLAYER_INFO
#define USE_CVAR_CHANGE
// #define USE_D3D_DRAW

// D3D �ĺ��� jmp �ҹ�
static std::unique_ptr<DetourXS> g_pDetourReset, g_pDetourPresent, g_pDetourEndScene,
g_pDetourDrawIndexedPrimitive, g_pDetourCreateQuery, g_pDetourCL_Move, g_pDetourDebugger;

std::unique_ptr<CNetVars> g_pNetVars;
CInterfaces g_interface;

// D3D Device ���ҹ�
static std::unique_ptr<CVMTHookManager> g_pVMTDevice;

// ��ǰ dll �ļ�ʵ��
HINSTANCE g_hMyInstance;

// D3D EndScene ���ƹ���
static std::unique_ptr<DrawManager> g_pDrawRender;

// D3D9 Device ����
static std::unique_ptr<D3D9Hooker> g_pDeviceHooker;

DWORD WINAPI StartCheat(LPVOID params);

BOOL WINAPI DllMain(HINSTANCE module, DWORD reason, LPVOID reserved)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		g_hMyInstance = module;
		DisableThreadLibraryCalls(module);

		// ��ȡ���нӿ�
		g_interface.GetInterfaces();

		// ��ʼ�� NetProp ��
		g_pNetVars = std::make_unique<CNetVars>();

		// se�쳣������
		_set_se_translator([](unsigned int expCode, EXCEPTION_POINTERS* pExp) -> void
		{
			std::stringstream expInfo;
			switch (expCode)
			{
			case EXCEPTION_ACCESS_VIOLATION:
				expInfo << "[SE] ���ʳ�ͻ��IP 0x" << std::setiosflags(std::ios::hex | std::ios::uppercase) <<
					pExp->ExceptionRecord->ExceptionAddress << " ���ͣ�" <<
					(pExp->ExceptionRecord->ExceptionInformation[0] ? "д��" : "��ȡ") << " ��ַ��0x" <<
					pExp->ExceptionRecord->ExceptionInformation[1];
				break;
			case EXCEPTION_INT_DIVIDE_BY_ZERO:
				expInfo << "[SE] int ��������Ϊ 0";
				break;
			case EXCEPTION_FLT_DIVIDE_BY_ZERO:
				expInfo << "[SE] float ��������Ϊ 0.0";
				break;
			case EXCEPTION_ILLEGAL_INSTRUCTION:
				expInfo << "[SE] ��Ч��ָ��";
				break;
			case EXCEPTION_PRIV_INSTRUCTION:
				expInfo << "[SE] �޷�����˽�е�ָ��";
				break;
			case EXCEPTION_STACK_OVERFLOW:
				expInfo << "[SE] ջ���";
				break;
			default:
				expInfo << "[SE] δ֪�쳣";
			}

			Utils::log(expInfo.str().c_str());
			throw std::exception(expInfo.str().c_str());
		});

		// �쳣������������
		_set_invalid_parameter_handler([](const wchar_t* expression, const wchar_t* function,
			const wchar_t* file, unsigned int line, uintptr_t pReserved) -> void
		{
			std::wstringstream expInfo;
#ifndef _DEBUG
			expInfo << L"��ʹ�� DEBUG ģʽ�����Խ��е���";
#else
			expInfo << L"[IP] ��ͨ�쳣��" << std::endl << L"\t��Ϣ��" << expression << std::endl <<
				L"\t�ļ���" << file << L" (" << line << L")" << std::endl << L"\t������" << function;
#endif
			Utils::log(expInfo.str().c_str());
			throw std::exception(Utils::w2c(expInfo.str()).c_str());
		});

		// �麯�������쳣����
		_set_purecall_handler([]() -> void
		{
			Utils::log("δ֪�麯�������쳣");
			throw std::exception("δ֪�麯�������쳣");
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

		try
		{
			// ������ʱ��������Ϊ�����˳���Ϸ���ڴ��Ѿ��ͷŵ���
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
static FnConColorMsg PrintToConsoleColor;	// ��ӡ��Ϣ������̨��֧����ɫ��

typedef void(__cdecl* FnConMsg)(char const*, ...);
static FnConMsg PrintToConsole;				// ��ӡ��Ϣ������̨

typedef void(__cdecl* FnCL_Move)(float, bool);
void __stdcall Hooked_CL_Move(float, bool);
FnCL_Move oCL_Move;							// ������ݴ���

typedef void(__cdecl* FnSharedRandomFloat)(const char*, float, float, int);
FnSharedRandomFloat SharedRandomFloat;		// �����

typedef int(__stdcall* FnTraceLine)(void* ebp, void* esi, const Vector&, const Vector&, unsigned int mask,
	const IHandleEntity*, int, trace_t*);
FnTraceLine UTIL_TraceRay;					// ���߸���

typedef const char*(__cdecl* FnWeaponIdToAlias)(unsigned int);
FnWeaponIdToAlias WeaponIdToAlias;			// ��ȡ����������

typedef bool(__thiscall* FnUserMessagesDispatch)(void*, int, bf_read&);
FnUserMessagesDispatch DispatchUserMessage;	// �û���Ϣ

typedef bool(__cdecl* FnIsInDebugSession)();
bool __cdecl Hooked_IsInDebugSession();
FnIsInDebugSession oPlat_IsInDebugSession;

typedef void*(__thiscall* FnGetWpnData)(CBaseEntity*);
FnGetWpnData g_pCallGetWpnData;				// ��ȡ�������� (�����麯��)

typedef void(__thiscall* FnStartDrawing)(CSurface*);
FnStartDrawing PaintStartDrawing;			// ��ʼ���� (�� CEngineVGui::Paint ����Ʊ����ȵ������)

typedef void(__thiscall* FnFinishDrawing)(CSurface*);
FnFinishDrawing PaintFinishDrawing;			// ��ɻ��� (�� CEngineVGui::Paint ����ƽ���ʱ����������)

// -------------------------------- Cheats Function --------------------------------
void thirdPerson(bool);
void showSpectator();
void bindAlias(int);

// -------------------------------- Golbals Variable --------------------------------
static int g_iCurrentAiming = 0;										// ��ǰ������׼����ҵ� ID
static bool g_bNewFrame = false;										// �����Ƿ��������µ�һ֡
static bool* g_pbSendPacket;											// ���ݰ��Ƿ��͵�������
static int g_iSpeedMultiple = 5;										// ���ٱ���
static CUserCmd* g_pUserCommands;										// ������ҵ�ǰ����
std::map<std::string, ConVar*> g_conVar;								// ����̨����
static float g_fAimbotFieldOfView = 30.0f;								// �Զ���׼�Ƕ�
CBaseEntity* g_pCurrentAimbot;											// ��ǰ���Զ���׼Ŀ��
CBaseEntity* g_pCurrentAiming;											// ��ǰ������׼��Ŀ��
int g_iCurrentHitbox;													// ��ǰ������׼�ĵ��˵� Hitbox
CBaseEntity* g_pGameRulesProxy;											// ��Ϸ����ʵ�壬���������һЩ���õĶ���
CBaseEntity* g_pPlayerResource;											// �����Դ������������ȡĳЩ����
static DWORD g_iClientBase, g_iEngineBase, g_iMaterialModules;			// ���õ� DLL �ļ���ַ

std::string GetZombieClassName(CBaseEntity* player);
bool IsValidVictim(CBaseEntity* entity);

DWORD WINAPI StartCheat(LPVOID params)
{
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

	// ��������ǲ���ȷ��...
	g_interface.MoveHelper = nullptr;

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
		(g_interface.ClientMode = GetClientModeNormal()) != nullptr)
	{
		g_interface.ClientModeHook = std::make_unique<CVMTHookManager>(g_interface.ClientMode);
		// printo("ClientModePtr", g_interface.ClientMode);
		Utils::log("GetClientMode = client.dll + 0x%X", (DWORD)GetClientModeNormal - g_iClientBase);
		Utils::log("g_pClientMode = 0x%X", (DWORD)g_interface.ClientMode);
		Utils::log("m_pChatElement = 0x%X", (DWORD)g_interface.ClientMode->GetHudChat());
	}
	else
		Utils::log("ClientModeShared not found");

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
			// ���ε�������飬��Ϊ��鵽�������ͻ��Զ��ر���Ϸ��
			g_pDetourDebugger = std::make_unique<DetourXS>(oPlat_IsInDebugSession, Hooked_IsInDebugSession);
		}
	}

	if (g_interface.Cvar)
	{
#ifdef USE_CVAR_CHANGE
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
#else
		g_conVar["sv_cheats"] = nullptr;
		g_conVar["r_drawothermodels"] = nullptr;
		g_conVar["cl_drawshadowtexture"] = nullptr;
		g_conVar["mat_fullbright"] = nullptr;
		g_conVar["sv_pure"] = nullptr;
		g_conVar["sv_consistency"] = nullptr;
		g_conVar["mp_gamemode"] = nullptr;
		g_conVar["c_thirdpersonshoulder"] = nullptr;
		g_conVar["c_thirdpersonshoulderheight"] = nullptr;
		g_conVar["c_thirdpersonshoulderoffset"] = nullptr;
		g_conVar["cl_mouseenable"] = nullptr;
#endif
	}

	// ��ʼ�� ImGui
	while ((g_hwGameWindow = FindWindowA(nullptr, "Left 4 Dead 2")) == nullptr)
		std::this_thread::sleep_for(std::chrono::milliseconds(200));

	g_pOldWindowProccess = (WNDPROC)SetWindowLongPtrA(g_hwGameWindow, GWL_WNDPROC, (LONG_PTR)ImGui_WindowProccess);

	g_pDeviceHooker = std::make_unique<D3D9Hooker>();
	g_pDeviceHooker->StartDeviceHook([&](IDirect3D9* pD3D, IDirect3DDevice9* pDeivce, DWORD* pVMT) -> void
	{
		// �麯�����޸���ת
		g_pDetourReset = std::make_unique<DetourXS>((void*)pVMT[16], Hooked_Reset);
		g_pDetourPresent = std::make_unique<DetourXS>((void*)pVMT[17], Hooked_Present);
		g_pDetourEndScene = std::make_unique<DetourXS>((void*)pVMT[42], Hooked_EndScene);
		g_pDetourDrawIndexedPrimitive = std::make_unique<DetourXS>((void*)pVMT[82], Hooked_DrawIndexedPrimitive);
		g_pDetourCreateQuery = std::make_unique<DetourXS>((void*)pVMT[118], Hooked_CreateQuery);

		// ��ȡԭ����
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

	// ֻ��Ϊ�˱����������
	g_pDeviceHooker->GetDevice() = nullptr;

	// ������
	bindAlias(45);

	// �¼�������
	if (g_interface.GameEvent)
	{
		class EventListener : public IGameEventListener2
		{
		public:
			virtual void FireGameEvent(IGameEvent *event)
			{
				const char* eventName = event->GetName();
				if (_strcmpi(eventName, "player_death") == 0)
				{
					// �ܺ��ߣ����ߣ�
					int victim = g_interface.Engine->GetPlayerForUserID(event->GetInt("userid"));
					if (victim <= 0)
						victim = event->GetInt("entityid");

					// �����ߣ���ɱ�ߣ�
					int attacker = g_interface.Engine->GetPlayerForUserID(event->GetInt("attacker"));
					if (attacker <= 0)
						attacker = event->GetInt("attackerentid");

					if (victim <= 0)
						return;

					CBaseEntity* entity = g_interface.ClientEntList->GetClientEntity(victim);
					if (!IsValidVictim(entity))
						return;

					if ((DWORD)entity == (DWORD)g_pCurrentAimbot)
						g_pCurrentAimbot = nullptr;

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

					int attacker = g_interface.Engine->GetPlayerForUserID(event->GetInt("attacker"));

					CBaseEntity* entity = g_interface.ClientEntList->GetClientEntity(victim);
					if (!IsValidVictim(entity))
						return;

					if ((DWORD)entity == (DWORD)g_pCurrentAimbot)
						g_pCurrentAimbot = nullptr;

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
					// �ܺ���
					int victim = g_interface.Engine->GetPlayerForUserID(event->GetInt("userid"));

					// ������
					int attacker = g_interface.Engine->GetPlayerForUserID(event->GetInt("attacker"));

					// ��˭�Ĵ�
					int guilty = g_interface.Engine->GetPlayerForUserID(event->GetInt("guilty"));

					// �˺�����
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
					// �ܺ���
					int victim = g_interface.Engine->GetPlayerForUserID(event->GetInt("userid"));

					// ������
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
					if (g_conVar["c_thirdpersonshoulder"] == nullptr ||
						g_conVar["c_thirdpersonshoulder"]->GetInt() <= 0)
						return;

					int client = g_interface.Engine->GetPlayerForUserID(event->GetInt("userid"));
					const char* weapon = event->GetString("weapon");
					if (client != g_interface.Engine->GetLocalPlayer() ||
						weapon == nullptr || weapon[0] == '\0')
						return;

					if (_strcmpi(weapon, "autoshotgun") == 0)
						g_interface.Engine->ClientCmd("play \"weapons/auto_shotgun/gunfire/auto_shotgun_fire_1.wav\"");
					else if (_strcmpi(weapon, "shotgun_spas") == 0)
						g_interface.Engine->ClientCmd("play \"weapons/auto_shotgun_spas/gunfire/shotgun_fire_1.wav\"");
					else if (_strcmpi(weapon, "pumpshotgun") == 0)
						g_interface.Engine->ClientCmd("play \"weapons/shotgun/gunfire/shotgun_fire_1.wav\"");
					else if (_strcmpi(weapon, "shotgun_chrome") == 0)
						g_interface.Engine->ClientCmd("play \"weapons/shotgun_chrome/gunfire/shotgun_fire_1.wav\"");

				}
			}
		};

		// ע���¼�������
		EventListener* listener = new EventListener();
		g_interface.GameEvent->AddListener(listener, "player_spawn", false);
		g_interface.GameEvent->AddListener(listener, "player_death", false);
		g_interface.GameEvent->AddListener(listener, "infected_death", false);
		g_interface.GameEvent->AddListener(listener, "map_transition", false);
		g_interface.GameEvent->AddListener(listener, "mission_lost", false);
		g_interface.GameEvent->AddListener(listener, "weapon_fire", false);

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

	return 0;
}

void ResetDeviceHook(IDirect3DDevice9* device)
{
	// ������Ϸʹ�õ� IDirect3DDevice9 ����
	g_pDeviceHooker->GetDevice() = device;

	// ���޸ĵ� Jump ��ԭ
	DETOURXS_DESTORY(g_pDetourReset);
	DETOURXS_DESTORY(g_pDetourPresent);
	DETOURXS_DESTORY(g_pDetourEndScene);
	DETOURXS_DESTORY(g_pDetourDrawIndexedPrimitive);
	DETOURXS_DESTORY(g_pDetourCreateQuery);

	// ʹ�� VMT �� Hook
	g_pVMTDevice = std::make_unique<CVMTHookManager>(device);
	oReset = g_pVMTDevice->SetupHook(16, Hooked_Reset);
	oPresent = g_pVMTDevice->SetupHook(17, Hooked_Present);
	oEndScene = g_pVMTDevice->SetupHook(42, Hooked_EndScene);
	oDrawIndexedPrimitive = g_pVMTDevice->SetupHook(82, Hooked_DrawIndexedPrimitive);
	oCreateQuery = g_pVMTDevice->SetupHook(118, Hooked_CreateQuery);
	g_pVMTDevice->HookTable(true);

	// ��ʼ����ͼ
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
			// �����˳�
			local->SetNetProp<int>("m_hObserverTarget", 0, "DT_BasePlayer");
			local->SetNetProp<int>("m_iObserverMode", 1, "DT_BasePlayer");
			local->SetNetProp<int>("m_bDrawViewmodel", 0, "DT_BasePlayer");
		}
		else
		{
			// ��һ�˳�
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
				// 观察�
				if (mode != 4 && mode != 5)
					continue;

				target = (CBaseEntity*)player->GetNetProp<CBaseHandle*>("m_hObserverTarget", "DT_BasePlayer");
				if (target && (int)target != -1)
					target = g_interface.ClientEntList->GetClientEntityFromHandle((CBaseHandle*)target);

				if (target == nullptr || (int)target == -1)
					continue;

				if ((DWORD)target == (DWORD)local)
				{
					// 正在观察本地玩家
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
					// 正在观察其他玩家
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
				// 生还�
				if (player->IsAlive())
				{
					// 活着
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
					// 死亡
					if (mode != 4 && mode != 5)
						continue;

					target = (CBaseEntity*)player->GetNetProp<CBaseHandle*>("m_hObserverTarget", "DT_BasePlayer");
					if (target && (int)target != -1)
						target = g_interface.ClientEntList->GetClientEntityFromHandle((CBaseHandle*)target);

					if (target == nullptr || (int)target == -1)
						continue;

					if ((DWORD)target == (DWORD)local)
					{
						// 正在观察本地玩家
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
						// 正在观察其他玩家
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
				// 感染�
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
					// 活着
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
					// 幽灵状�
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
					// 死亡
					if (mode != 4 && mode != 5)
						continue;

					target = (CBaseEntity*)player->GetNetProp<CBaseHandle*>("m_hObserverTarget", "DT_BasePlayer");
					if (target && (int)target != -1)
						target = g_interface.ClientEntList->GetClientEntityFromHandle((CBaseHandle*)target);

					if (target == nullptr || (int)target == -1)
						continue;

					if ((DWORD)target == (DWORD)local)
					{
						// 正在观察本地玩家
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
						// 正在观察其他玩家
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
	g_interface.Engine->ClientCmd("net_graph 1");
	g_interface.Engine->ClientCmd("net_graphpos 1");
	g_interface.Engine->ClientCmd("net_graphshowinterp 0");
	g_interface.Engine->ClientCmd("net_graphshowlatency 0");
	g_interface.Engine->ClientCmd("net_graphtext 1");
	g_interface.Engine->ClientCmd("alias +shownetscores \"+showscores; net_graph 3; net_graphtext 1\"");
	g_interface.Engine->ClientCmd("alias -shownetscores \"-showscores; net_graph 1\"");
	g_interface.Engine->ClientCmd("bind tab +shownetscores");
	g_interface.Engine->ClientCmd("alias \"lerp_0\" \"rate 30000;cl_cmdrate 100;cl_updaterate 100;cl_interp 0.0;cl_interp_ratio -1;alias lerp_change lerp_16;echo Lerp set to 0 (rate 30000, cl_cmdrate 100, cl_updaterate 100, cl_interp 0.0, cl_interp_ratio -1).\";");
	g_interface.Engine->ClientCmd("alias \"lerp_16\" \"rate 30000;cl_cmdrate 100;cl_updaterate 100;cl_interp 0.0167;cl_interp_ratio -1;alias lerp_change lerp_33;echo Lerp set to 16.7 (rate 30000, cl_cmdrate 100, cl_updaterate 100, cl_interp 0.0167, cl_interp_ratio -1).\";");
	g_interface.Engine->ClientCmd("alias \"lerp_33\" \"rate 30000;cl_cmdrate 100;cl_updaterate 100;cl_interp 0.0334;cl_interp_ratio -1;alias lerp_change lerp_50;echo Lerp set to 33.4 (rate 30000, cl_cmdrate 100, cl_updaterate 100, cl_interp 0.0334, cl_interp_ratio -1).\";");
	g_interface.Engine->ClientCmd("alias \"lerp_50\" \"rate 30000;cl_cmdrate 100;cl_updaterate 100;cl_interp 0.0501;cl_interp_ratio -1;alias lerp_change lerp_66;echo Lerp set to 50.1 (rate 30000, cl_cmdrate 100, cl_updaterate 100, cl_interp 0.0501, cl_interp_ratio -1).\";");
	g_interface.Engine->ClientCmd("alias \"lerp_66\" \"rate 30000;cl_cmdrate 100;cl_updaterate 100;cl_interp 0.0667;cl_interp_ratio -1;alias lerp_change lerp_0;echo Lerp set to 66.7 (rate 30000, cl_cmdrate 100, cl_updaterate 100, cl_interp 0.0667, cl_interp_ratio -1).\";");
	g_interface.Engine->ClientCmd("echo \"echo \"========= alias end =========\"\"");
}

static CMoveData g_predMoveData;

// ����ʵ���Ƿ�Ϊ������/�ظ�/�ո�/Witch
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
		Utils::log("%s (%d) ���棺ָ�� 0x%X ������ʵ�壡", __FILE__, __LINE__, (DWORD)entity);
		throw std::exception("IsValidVictim �����˴���");
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

		// �ո�����ȼ��
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

// ��ȡʵ�������
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
			// ���� CEDA ��Ա
			return "ceda";
		case 12:
			// ����
			return "mud";
		case 13:
			// ��·����
			return "roadcrew";
		case 14:
			// ����Ⱦ���Ҵ���
			return "fallen";
		case 15:
			// ��������
			return "riot";
		case 16:
			// С��
			return "clown";
		case 17:
			// �����ּ���
			return "jimmy";
		}

		// ������Ⱦ��
		return "infected";
	}
	if (player->GetClientClass()->m_ClassID == ET_WITCH)
	{
		// ���� Witch
		if (player->GetNetProp<int>("m_Gender", "DT_Infected") == 19)
			return "bride";

		// ��ͨ Witch
		return "witch";
	}

	int zombie = player->GetNetProp<int>("m_zombieClass", "DT_TerrorPlayer");
	int character = player->GetNetProp<int>("m_survivorCharacter", "DT_TerrorPlayer");

	switch (zombie)
	{
	case ZC_SMOKER:
		// ��ͷ
		return "smoker";
	case ZC_BOOMER:
		// ����
		return "boomer";
	case ZC_HUNTER:
		// ����
		return "hunter";
	case ZC_SPITTER:
		// ��ˮ
		return "spitter";
	case ZC_JOCKEY:
		// ��
		return "jockey";
	case ZC_CHARGER:
		// ţ
		return "charger";
	case ZC_TANK:
		// ��
		return "tank";
	case ZC_SURVIVORBOT:
		switch (character)
		{
		case 0:
			// ��װ
			return "nick";
		case 1:
			// ����
			return "rochelle";
		case 2:
			// ����
			return "coach";
		case 3:
			// ñ��
			return "ellis";
		case 4:
			// ��ͷ
			return "bill";
		case 5:
			// Ů��
			return "zoey";
		case 6:
			// ���
			return "francis";
		case 7:
			// ��ͷ
			return "louis";
		}
	}

	// ��֪��
	return "unknown";
}

// ���ݹ�ͷ��ȡͷ��λ��
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

// ���ݻ��кл�ȡͷ��λ��
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
		Utils::log("%s (%d): δ֪�쳣 | 0x%X", __FILE__, __LINE__, (DWORD)entity);
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

// �ٶ�Ԥ��
inline Vector VelocityExtrapolate(CBaseEntity* player, const Vector& aimpos)
{
	return aimpos + (player->GetVelocity() * g_interface.Globals->interval_per_tick);
}

// ��ȡ��ǰ������׼�ĵ���
CBaseEntity* GetAimingTarget(int* hitbox = nullptr)
{
	CBaseEntity* client = GetLocalClient();
	if (client == nullptr || !client->IsAlive())
		return nullptr;

	Ray_t ray;
	trace_t trace;
	CTraceFilter filter;

	Vector src = client->GetEyePosition(), dst;

	// �ٶ�Ԥ�⣬��ֹ�ƶ�ʱ����׼
	src = VelocityExtrapolate(client, src);

	filter.pSkip1 = client;
	g_interface.Engine->GetViewAngles(dst);

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

	g_interface.Trace->TraceRay(ray, MASK_SHOT, &filter, &trace);

#ifdef _DEBUG_OUTPUT
	Utils::log("TraceRay: entity = 0x%X | hitbox = %d | bone = %d | hitGroup = %d | fraction = %.2f | classId = %d",
		trace.m_pEnt, trace.hitbox, trace.physicsBone, trace.hitGroup, trace.fraction,
		(trace.m_pEnt != nullptr ? trace.m_pEnt->GetClientClass()->m_ClassID : -1));
#endif

	// ����Ƿ���������Ϸ����
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

	// ����Ƿ�������һ�����Թ���������
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

// ��鱾������Ƿ���Կ���ĳ��ʵ��
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
		Utils::log("%s (%d): δ֪�쳣 | 0x%X", __FILE__, __LINE__, (DWORD)entity);
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
	g_interface.Trace->TraceRay(ray, MASK_SHOT, &filter, &trace);

	// ����Ƿ�Ϊָ��Ŀ��
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

	ImGui_ImplDX9_InvalidateDeviceObjects();
	g_pDrawRender->OnLostDevice();

	HRESULT result = oReset(device, pp);

	g_pDrawRender->OnResetDevice();
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

	// ���Ʋ˵�
	if (g_bIsShowMenu)
	{
		// ����һ������
		ImGui::Begin("L4D2Simple", &g_bIsShowMenu, ImVec2(300, 250), 0.75f);
		{
			// ����
			if (ImGui::CollapsingHeader("Draw"))
			{
				ImGui::Checkbox("2D Box", &Config::bDrawBox);
				ImGui::Checkbox("Bone Box", &Config::bDrawBone);
				ImGui::Checkbox("Name", &Config::bDrawName);
				ImGui::Checkbox("Distance", &Config::bDrawDist);
				ImGui::Checkbox("Ammo", &Config::bDrawAmmo);
				ImGui::Checkbox("Crosshairs", &Config::bDrawAmmo);
			}

			// Z ��
			if (ImGui::CollapsingHeader("DirextX ZBuffer"))
			{
				ImGui::Checkbox("Survivor", &Config::bBufferSurvivor);
				ImGui::Checkbox("Special Infected", &Config::bBufferSpecial);
				ImGui::Checkbox("Common Infected", &Config::bBufferCommon);
				ImGui::Checkbox("Grenade", &Config::bBufferGrenade);
				ImGui::Checkbox("Medickit", &Config::bBufferMedickit);
				ImGui::Checkbox("Guns", &Config::bBufferWeapon);
				ImGui::Checkbox("Carry", &Config::bBufferCarry);
			}

			// ��׼
			if (ImGui::CollapsingHeader("Aiming"))
			{
				ImGui::Checkbox("Aimbot", &Config::bAimbot);
				ImGui::SliderFloat("Aimbot FOV", &Config::fAimbotFov, 1.0f, 360.0f);
				ImGui::Checkbox("Silent Aimbot", &Config::bSilentAimbot);
				ImGui::Checkbox("Trigger Bot", &Config::bTriggerBot);
				ImGui::Checkbox("Trigger Bot Only Head", &Config::bTriggerBotHead);
				ImGui::Checkbox("Do not Fire Firendly", &Config::bAnitFirendlyFire);
			}

			// ����
			if (ImGui::CollapsingHeader("Misc"))
			{
				ImGui::Checkbox("Bunny Hop", &Config::bBunnyHop);
				ImGui::Checkbox("Auto Strafe", &Config::bAutoStrafe);
				ImGui::Checkbox("No Recoil", &Config::bNoRecoil);
				ImGui::Checkbox("Rapid Fire", &Config::bRapidFire);
				ImGui::Checkbox("CRC Check Bypass", &Config::bCrcCheckBypass);
				ImGui::Checkbox("Full Bright", &Config::bCvarFullBright);
				ImGui::Checkbox("Wireframe", &Config::bCvarWireframe);
				ImGui::Checkbox("Game Mode Change", &Config::bCvarGameMode);
				ImGui::Checkbox("Third Person", &Config::bThirdPersons);
			}
		}
		ImGui::End();	// ���һ�����ڵĻ���
	}

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

	// ����֮ǰ��������
	g_pDrawRender->BeginRendering();

#ifdef USE_D3D_DRAW

	CBaseEntity* local = GetLocalClient();
	if (g_interface.Engine->IsInGame() && local != nullptr)
	{
		// Ŀǰ��С����
		float distmin = 65535.0f;

		// ��ǰ����
		int team = local->GetTeam();

		// ��ǰ�Ƿ����Զ���׼��Ŀ��
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
			Utils::log("%s (%d): δ֪�쳣 | 0x%X", __FILE__, __LINE__, (DWORD)g_pCurrentAimbot);
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

		// һ���ո�ʵ���������� 512 �͹��ˣ�̫��Ῠ��
		int maxEntity = g_interface.ClientEntList->GetHighestEntityIndex();

		// ������ɫ
		D3DCOLOR color = 0;

		int aiming = *(int*)(local + m_iCrosshairsId);

		// ��ǰ������׼��Ŀ��
		CBaseEntity* target = (aiming > 0 ? g_interface.ClientEntList->GetClientEntity(aiming) : GetAimingTarget(&g_iCurrentHitbox));

#ifdef _DEBUG
		try
		{
#endif
			if (!IsValidVictim(target))
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
			Utils::log("%s (%d): δ֪�쳣 | 0x%X", __FILE__, __LINE__, (DWORD)target);
			target = nullptr;
		}
#endif

		// ����׼��
		if (Config::bDrawCrosshairs)
		{
			int width, height;
			g_interface.Engine->GetScreenSize(width, height);

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

			/*
			if (g_conVar["c_thirdpersonshoulder"] != nullptr &&
			g_conVar["c_thirdpersonshoulder"]->GetInt() > 0)
			{
				// ����ƫ��
				height += g_conVar["c_thirdpersonshoulderheight"]->GetInt();

				// ����ƫ��
				width += g_conVar["c_thirdpersonshoulderoffset"]->GetInt();
			}
			*/

			g_pDrawRender->AddLine(color, width - 10, height, width + 10, height);
			g_pDrawRender->AddLine(color, width, height - 10, width, height + 10);
		}

		// 0 Ϊ worldspawn��û������
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
					// ��� ������/�ظ�/�ո� �Ƿ�����Ч��
					if (!IsValidVictim(entity))
						continue;
#ifdef _DEBUG_OUTPUT_
				}
#endif
			}
			catch (std::exception e)
			{
				Utils::log("%s (%d): %s", __FILE__, __LINE__, e.what());
				continue;
			}
			catch (...)
			{
				Utils::log("%s (%d): δ֪�쳣 -> 0x%X", __FILE__, __LINE__, (DWORD)entity);
				continue;
			}

			Vector head, foot, headbox, origin;

			// Ŀ����µ�λ��
			origin = (IsSurvivor(classId) || IsSpecialInfected(classId) ?
				entity->GetAbsOrigin() :		// ���ר�ã�����ʵ����û�е�
				entity->GetNetProp<Vector>("m_vecOrigin", "DT_BaseCombatCharacter"));

			// Ŀ���ͷ����λ��
			headbox = (IsSurvivor(classId) || IsSpecialInfected(classId) || IsCommonInfected(classId) ?
				GetHeadHitboxPosition(entity) : origin);

			// ���Ŀ���Ƿ�����Ļ��
			if (!headbox.IsValid() || !WorldToScreen(headbox, head) ||
				!WorldToScreen(origin, foot))
				continue;

			// Ŀ���Ƿ�ɼ�
			bool visible = IsTargetVisible(entity, headbox, myEyeOrigin);

			// Ŀ�����Լ��ľ���
			float dist = myOrigin.DistTo(origin);

			// ��ȡ����Ĵ�С
			float height = fabs(head.y - foot.y);
			float width = height * 0.65f;

			// ����һ���һ����
			if (Config::bDrawBox)
			{
				if (IsSurvivor(classId) || IsSpecialInfected(classId))
				{
					color = (entity->GetTeam() == team ? DrawManager::BLUE : DrawManager::RED);
					if (IsSurvivor(classId))
					{
						if (entity->GetNetProp<byte>("m_bIsOnThirdStrike", "DT_TerrorPlayer") != 0)
						{
							// �ڰ�״̬ - ��ɫ
							color = DrawManager::WHITE;
						}
						else if (IsControlled(entity))
						{
							// ���� - ��ɫ
							color = DrawManager::ORANGE;
						}
						else if (IsIncapacitated(entity))
						{
							// ���عұ� - ��ɫ
							color = DrawManager::YELLOW;
						}
					}
					else if (IsSpecialInfected(classId) && IsGhostInfected(entity) && IsPlayerGhost(i))
					{
						// ����״̬ - ��ɫ
						color = DrawManager::PURPLE;
					}

					// ����һ����
					g_pDrawRender->AddRect(color, foot.x - width / 2, foot.y, width, -height);
				}
				else if (IsCommonInfected(classId))
				{
					// ��ֻ���ոж��ѣ�̫Զ��û��Ҫ��ʾ����
					if (dist > 3000.0f)
						continue;

					int size = 2;
					if (dist < 1000.0f)
						size = 4;

					color = DrawManager::GREEN;
					if (classId == ET_WITCH)
						color = DrawManager::PINK;

					// ��һ��С���Σ��Ա��Ϊͷ��
					if (visible)
						g_pDrawRender->AddFillCircle(color, head.x, head.y, size, 8);
					else
						g_pDrawRender->AddCircle(color, head.x, head.y, size, 8);
				}
				else
				{
					// ��������
					if (dist > 1000.0f)
						continue;

					if (classId == ET_TankRock)
					{
						// Tank ��ʯͷ
						g_pDrawRender->AddCircle(DrawManager::PURPLE, foot.x, foot.y, 9, 8);
					}
					else if (classId == ET_WeaponFirstAidKit || classId == ET_WeaponDefibrillator ||
						classId == ET_WeaponPainPills || classId == ET_WeaponPainPills)
					{
						// ҽ��Ʒ
						g_pDrawRender->AddFillRect(DrawManager::DARKORANGE, foot.x, foot.y, 2, 8);
					}
					else if (classId == ET_WeaponPipeBomb || classId == ET_WeaponMolotov ||
						classId == ET_WeaponVomitjar)
					{
						// Ͷ������
						g_pDrawRender->AddFillRect(DrawManager::DARKGRAY, foot.x, foot.y, 2, 8);
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
					if (hdr != nullptr)
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
								// �����еĹ���������������
								g_pDrawRender->AddLine(color, screenParent.x, screenParent.y,
									screenChild.x, screenChild.y);
							}
						}
					}
				}
			}

			if (Config::bDrawName)
			{
				// ���ڸ�ʽ���ַ���
				std::stringstream ss;

				// ȡ�� stream ͬ�����Լӿ������ٶ�
				ss.sync_with_stdio(false);
				ss.tie(nullptr);

				// ȥ�� float ��С��λ����Ϊû��Ҫ
				ss.setf(std::ios::fixed);
				ss.precision(0);

				// �������;������Ƶ�����
				if (IsSurvivor(classId) || IsSpecialInfected(classId))
				{
					// ����Ƿ�Ϊ������
					if (IsSurvivor(classId))
					{
						if (IsIncapacitated(entity))
						{
							// ����ʱֻ����ͨѪ��
							ss << "[" << entity->GetHealth() << " + incap] ";
						}
						else if (IsControlled(entity))
						{
							// ��ұ�����
							ss << "[" << (int)(entity->GetHealth() +
								entity->GetNetProp<float>("m_healthBuffer", "DT_TerrorPlayer")) <<
								" + grabbed] ";
						}
						else
						{
							// ��������ʾѪ������ʱѪ��
							ss << "[" << entity->GetHealth() << " + " << std::setprecision(0) <<
								(int)(entity->GetNetProp<float>("m_healthBuffer", "DT_TerrorPlayer")) << "] ";
						}
					}
					else
					{
						if (IsPlayerGhost(i) && IsGhostInfected(entity))
						{
							// ����״̬���ظ�
							ss << "[" << entity->GetHealth() << " ghost] ";
						}
						else
						{
							// ��������ֻ��ʾѪ���ͺ���
							ss << "[" << entity->GetHealth() << "] ";
						}
					}

					// �������
					ss << GetZombieClassName(entity);

					// ��ʾ����
					ss << "\n" << (int)dist;

					CBaseEntity* weapon = (CBaseEntity*)entity->GetActiveWeapon();
					if (weapon != nullptr)
						weapon = g_interface.ClientEntList->GetClientEntityFromHandle((CBaseHandle*)weapon);

					// ����������ʾ��ҩ
					if (IsSurvivor(classId))
					{
						if (Config::bDrawAmmo && weapon != nullptr)
						{
							int ammoType = weapon->GetNetProp<int>("m_iPrimaryAmmoType", "DT_BaseCombatWeapon");
							int clip = weapon->GetNetProp<int>("m_iClip1", "DT_BaseCombatWeapon");
							byte reloading = weapon->GetNetProp<byte>("m_bInReload", "DT_BaseCombatWeapon");

							// ��ʾ��ҩ�͵���
							if (ammoType > 0 && clip > -1)
							{
								if (reloading != 0)
								{
									// ���ڻ��ӵ�
									ss << " (reloading)";
								}
								else
								{
									// û�л��ӵ�
									ss << " (" << clip << "/" <<
										entity->GetNetProp<int>("m_iAmmo", "DT_TerrorPlayer", (size_t)ammoType) <<
										")";
								}
							}
						}
					}
					/*
					else
					{
					// ��Ⱦ�߼�����ȴʱ����ʾ

					float serverTime = GetServerTime();
					ss.precision(1);

					if (classId == ET_TANK)
					{
					if (weapon != nullptr)
					{
					float primary = weapon->GetNetProp<float>("m_flNextPrimaryAttack", "DT_BaseCombatWeapon");
					float secondary = weapon->GetNetProp<float>("m_flNextSecondaryAttack", "DT_BaseCombatWeapon");

					ss << "(";
					if ((primary -= serverTime) > 0.0f)
					ss << primary;
					else
					ss << "ready";

					ss << "/";
					if ((secondary -= serverTime) > 0.0f)
					ss << secondary;
					else
					ss << "ready";

					ss << ")";
					}
					}
					else
					{
					weapon = (CBaseEntity*)entity->GetNetProp<CBaseHandle*>("m_customAbility", "DT_TerrorPlayer");
					if (weapon != nullptr)
					weapon = g_interface.ClientEntList->GetClientEntityFromHandle((CBaseHandle*)weapon);
					if (weapon != nullptr)
					{
					float skill = weapon->GetNetProp2<float>("m_nextActivationTimer", "m_timestamp", "DT_AbilityBase");

					ss << "(";
					if ((skill -= serverTime) > 0.0f)
					ss << skill;
					else
					ss << "ready";

					ss << ")";
					}
					}

					ss.precision(0);
					}
					*/
				}

				if (!ss.str().empty())
				{
					color = (visible ? DrawManager::LAWNGREEN : DrawManager::GREEN);
					g_pDrawRender->AddText(color, foot.x, head.y, true, ss.str().c_str());
				}
			}

			// �Զ���׼Ѱ��Ŀ��
			if (Config::bAimbot && (!targetSelected || !(g_pUserCommands->buttons & IN_ATTACK)) &&
				((team == 2 && (IsSpecialInfected(classId) || classId == ET_INFECTED)) ||
				(team == 3 && IsSurvivor(classId))))
			{
				// �Ѿ�ѡ���Ŀ���ˣ���������һ������Ҫ�ĵ���
				if (classId == ET_INFECTED && specialSelected)
					continue;

				// ѡ��һ����ӽ����ظУ���Ϊ�ظ�Խ���������˵ԽΣ��
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

		g_pCurrentAiming = target;
	}

#endif

	g_bNewFrame = true;

	// ��ԭ���ݵ�����
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
			Config::bBufferMedickit && l4d2_healthitem(stride, numVertices, primitiveCount) ||
			Config::bBufferSpecial && l4d2_special(stride, numVertices, primitiveCount) ||
			Config::bBufferSurvivor && l4d2_survivor(stride, numVertices, primitiveCount) ||
			Config::bBufferWeapon && l4d2_weapons(stride, numVertices, primitiveCount))
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

	/*	��Ҫ����������ᵼ����Ļ��ڵ�
	if (type == D3DQUERYTYPE_OCCLUSION)
	type = D3DQUERYTYPE_TIMESTAMP;
	*/

	return oCreateQuery(device, type, query);
}

// -------------------------------- Game Hooked Function --------------------------------
void __fastcall Hooked_PaintTraverse(CPanel* pPanel, void* edx, unsigned int panel, bool forcePaint, bool allowForce)
{
	static bool showHint = true;
	if (showHint)
	{
		showHint = false;
		Utils::log("Hooked_PaintTraverse trigged.");
	}

	oPaintTraverse(pPanel, panel, forcePaint, allowForce);

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

	// ÿһ֡���� 1 ��
	if (FocusOverlayPanel > 0 && panel == FocusOverlayPanel)
	{
		// ��������������ƣ���Ϊ���������ȫ��Ļ��
	}

	// ÿһ֡�� �ܶ�� �����ﲻ�������Ľϴ������
	if (MatSystemTopPanel > 0 && panel == MatSystemTopPanel)
	{
		// ��������ƻᵼ����Ϸ�� fps �ǳ��ͣ���˱���Ҫ��������
#ifndef USE_D3D_DRAW
		if (!g_bNewFrame)
			goto finish_draw;

		g_bNewFrame = false;

		CBaseEntity* local = GetLocalClient();
		if (local == nullptr || !g_interface.Engine->IsInGame())
			goto finish_draw;

		// Ŀǰ��С����
		float distmin = 65535.0f;

		// ��ǰ����
		int team = local->GetTeam();

		// ��ǰ�Ƿ����Զ���׼��Ŀ��
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
			Utils::log("%s (%d): δ֪�쳣 | 0x%X", __FILE__, __LINE__, (DWORD)g_pCurrentAimbot);
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

		// һ���ո�ʵ���������� 512 �͹��ˣ�̫��Ῠ��
		int maxEntity = g_interface.ClientEntList->GetHighestEntityIndex();

		// ������ɫ
		D3DCOLOR color = 0;

		// ���ڸ�ʽ���ַ���
		std::stringstream ss;
		ss.sync_with_stdio(false);
		ss.tie(nullptr);
		ss.setf(std::ios::fixed);
		ss.precision(0);

		int aiming = *(int*)(local + m_iCrosshairsId);

		// ��ǰ������׼��Ŀ��
		CBaseEntity* target = (aiming > 0 ? g_interface.ClientEntList->GetClientEntity(aiming) : GetAimingTarget(&g_iCurrentHitbox));

#ifdef _DEBUG
		try
		{
#endif
			if (!IsValidVictim(target))
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
			Utils::log("%s (%d): δ֪�쳣 | 0x%X", __FILE__, __LINE__, (DWORD)target);
			target = nullptr;
		}
#endif

		if (Config::bDrawCrosshairs)
		{
			int width, height;
			g_interface.Engine->GetScreenSize(width, height);

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
			g_interface.Surface->drawCrosshair(width / 2, height / 2,
			(color & 0xFF0000) >> 16, (color & 0xFF00) >> 8, color & 0xFF);
			*/

			width /= 2;
			height /= 2;

			/*
			if (g_conVar["c_thirdpersonshoulder"] != nullptr &&
			g_conVar["c_thirdpersonshoulder"]->GetInt() > 0)
			{
			// ����ƫ��
			height += g_conVar["c_thirdpersonshoulderheight"]->GetInt();

			// ����ƫ��
			width += g_conVar["c_thirdpersonshoulderoffset"]->GetInt();
			}
			*/

			g_pDrawRender->AddLine(color, width - 10, height, width + 10, height);
			g_pDrawRender->AddLine(color, width, height - 10, width, height + 10);
		}

		// 0 Ϊ worldspawn��û������
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
					// ��� ������/�ظ�/�ո� �Ƿ�����Ч��
					if (!entity->IsAlive())
					{
						if (Config::bDrawSpectator)
						{
							ss.str("");

							if (IsSurvivor(classId) || IsSpecialInfected(classId))
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
											ss << info.name << " [1st]\n";
										else if (obsMode == OBS_MODE_CHASE)
											ss << info.name << " [3rd]\n";
									}
								}
							}

							if (!ss.str().empty())
							{
								color = (entity->GetTeam() == 2 ? DrawManager::SKYBLUE : DrawManager::RED);

								int width, height;
								g_interface.Engine->GetScreenSize(width, height);
								g_pDrawRender->AddText(color, width * 0.75, height * 0.75, false, ss.str().c_str());
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
				Utils::log("%s (%d): %s", __FILE__, __LINE__, e.what());
				continue;
			}
			catch (...)
			{
				Utils::log("%s (%d): δ֪�쳣 -> 0x%X", __FILE__, __LINE__, (DWORD)entity);
				continue;
			}

			Vector head, foot, headbox, origin;

			// Ŀ����µ�λ��
			origin = (IsSurvivor(classId) || IsSpecialInfected(classId) ?
				entity->GetAbsOrigin() :		// ���ר�ã�����ʵ����û�е�
				entity->GetNetProp<Vector>("m_vecOrigin", "DT_BaseCombatCharacter"));

			// Ŀ���ͷ����λ��
			headbox = (IsSurvivor(classId) || IsSpecialInfected(classId) || IsCommonInfected(classId) ?
				GetHeadHitboxPosition(entity) : origin);

			// ���Ŀ���Ƿ�����Ļ��
			if (!headbox.IsValid() || !WorldToScreen(headbox, head) ||
				!WorldToScreen(origin, foot))
				continue;

			// Ŀ���Ƿ�ɼ�
			bool visible = IsTargetVisible(entity, headbox, myEyeOrigin);

			// Ŀ�����Լ��ľ���
			float dist = myOrigin.DistTo(origin);

			// ��ȡ����Ĵ�С
			float height = fabs(head.y - foot.y);
			float width = height * 0.65f;

			// ����һ���һ����
			if (Config::bDrawBox)
			{
				if (IsSurvivor(classId) || IsSpecialInfected(classId))
				{
					color = (entity->GetTeam() == team ? DrawManager::BLUE : DrawManager::RED);
					if (IsSurvivor(classId))
					{
						if (entity->GetNetProp<byte>("m_bIsOnThirdStrike", "DT_TerrorPlayer") != 0)
						{
							// �ڰ�״̬ - ��ɫ
							color = DrawManager::WHITE;
						}
						else if (IsControlled(entity))
						{
							// ���� - ��ɫ
							color = DrawManager::ORANGE;
						}
						else if (IsIncapacitated(entity))
						{
							// ���عұ� - ��ɫ
							color = DrawManager::YELLOW;
						}
					}
					else if (IsSpecialInfected(classId) && IsGhostInfected(entity) && IsPlayerGhost(i))
					{
						// ����״̬ - ��ɫ
						color = DrawManager::PURPLE;
					}

					// ����һ������Ȼ�����ֻ�����������ߣ�
					/*
					g_interface.Surface->drawBox(foot.x - width / 2, foot.y, width, -height, 1,
					(color & 0xFF0000) >> 16, (color & 0xFF00) >> 8, color & 0xFF,
					(color & 0xFF000000) >> 24);
					*/

					g_pDrawRender->AddRect(color, foot.x - width / 2, foot.y, width, -height);
				}
				else if (IsCommonInfected(classId))
				{
					// ��ֻ���ոж��ѣ�̫Զ��û��Ҫ��ʾ����
					if (dist > 3000.0f)
						continue;

					int size = 2;
					if (dist < 1000.0f)
						size = 4;

					color = DrawManager::GREEN;
					if (classId == ET_WITCH)
						color = DrawManager::PINK;

					// ��һ��С���Σ��Ա��Ϊͷ��
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
					// ��������
					if (dist > 1000.0f)
						continue;

					if (classId == ET_TankRock)
					{
						// Tank ��ʯͷ
						g_pDrawRender->AddCircle(DrawManager::PURPLE, foot.x, foot.y, 9, 8);
					}
					else if (classId == ET_WeaponFirstAidKit || classId == ET_WeaponDefibrillator ||
						classId == ET_WeaponPainPills || classId == ET_WeaponPainPills)
					{
						// ҽ��Ʒ
						g_pDrawRender->AddFillRect(DrawManager::DARKORANGE, foot.x, foot.y, 2, 8);
					}
					else if (classId == ET_WeaponPipeBomb || classId == ET_WeaponMolotov ||
						classId == ET_WeaponVomitjar)
					{
						// Ͷ������
						g_pDrawRender->AddFillRect(DrawManager::DARKGRAY, foot.x, foot.y, 2, 8);
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

				// �������;������Ƶ�����
				if (IsSurvivor(classId) || IsSpecialInfected(classId))
				{
					player_info_t info;
					g_interface.Engine->GetPlayerInfo(i, &info);
					
					// ����Ƿ�Ϊ������
					if (IsSurvivor(classId))
					{
						if (IsIncapacitated(entity))
						{
							// ����ʱֻ����ͨѪ��
							ss << "[" << entity->GetHealth() << " + incap] ";
						}
						else if (IsControlled(entity))
						{
							// ��ұ�����
							ss << "[" << (int)(entity->GetHealth() +
								entity->GetNetProp<float>("m_healthBuffer", "DT_TerrorPlayer")) <<
								" + grabbed] ";
						}
						else
						{
							// ��������ʾѪ������ʱѪ��
							ss << "[" << entity->GetHealth() << " + " << std::setprecision(0) <<
								(int)(entity->GetNetProp<float>("m_healthBuffer", "DT_TerrorPlayer")) << "] ";
						}
					}
					else
					{
						if (IsPlayerGhost(i) && IsGhostInfected(entity))
						{
							// ����״̬���ظ�
							ss << "[" << entity->GetHealth() << " ghost] ";
						}
						else
						{
							// ��������ֻ��ʾѪ���ͺ���
							ss << "[" << entity->GetHealth() << "] ";
						}
					}

					// �������
					ss << info.name;

					// ��ʾ����
					ss << "\n" << (int)dist;

					CBaseEntity* weapon = (CBaseEntity*)entity->GetActiveWeapon();
					if (weapon != nullptr)
						weapon = g_interface.ClientEntList->GetClientEntityFromHandle((CBaseHandle*)weapon);

					// ����������ʾ��ҩ
					if (IsSurvivor(classId))
					{
						if (Config::bDrawAmmo && weapon != nullptr)
						{
							int ammoType = weapon->GetNetProp<int>("m_iPrimaryAmmoType", "DT_BaseCombatWeapon");
							int clip = weapon->GetNetProp<int>("m_iClip1", "DT_BaseCombatWeapon");
							byte reloading = weapon->GetNetProp<byte>("m_bInReload", "DT_BaseCombatWeapon");

							// ��ʾ��ҩ�͵���
							if (ammoType > 0 && clip > -1)
							{
								if (reloading != 0)
								{
									// ���ڻ��ӵ�
									ss << " (reloading)";
								}
								else
								{
									// û�л��ӵ�
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
							// ����ظв��ǻ����˵Ļ�����ʾ�ظ�����
							// �������ظ����־�������
							ss << " (" << GetZombieClassName(entity) << ")";
						}
					}
				}

				if (!ss.str().empty())
				{
					color = (visible ? DrawManager::LAWNGREEN : DrawManager::GREEN);

					/*
					g_interface.Surface->drawString(foot.x - width / 2, head.y + FontSize,
					(color & 0xFF0000) >> 16, (color & 0xFF00) >> 8, color & 0xFF,
					font, Utils::c2w(ss.str()).c_str());
					*/

					g_pDrawRender->AddText(color, foot.x, head.y, true, ss.str().c_str());
				}
			}

			// �Զ���׼Ѱ��Ŀ��
			if (Config::bAimbot && (!targetSelected || !(g_pUserCommands->buttons & IN_ATTACK)) &&
				((team == 2 && (IsSpecialInfected(classId) || classId == ET_INFECTED)) ||
				(team == 3 && IsSurvivor(classId))))
			{
				// �Ѿ�ѡ���Ŀ���ˣ���������һ������Ҫ�ĵ���
				if (classId == ET_INFECTED && specialSelected)
					continue;

				// ѡ��һ����ӽ����ظУ���Ϊ�ظ�Խ���������˵ԽΣ��
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

		g_pCurrentAiming = target;
#endif

	}

finish_draw:
	__asm nop;
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

	// �Զ�����
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

		// �����Զ���ת
		if (Config::bAutoStrafe && !(flags & FL_ONGROUND))
		{
			if (pCmd->mousedx < 0)
				pCmd->sidemove = -400.f;

			if (pCmd->mousedx > 0)
				pCmd->sidemove = 400.f;
		}
	}

	// ����Ԥ�ⱸ��
	float oldCurtime = g_interface.Globals->curtime;
	float oldFrametime = g_interface.Globals->frametime;

	// ����Ԥ��
	if (g_interface.MoveHelper)
	{
		// ������ҪԤ���ʱ�䣨֡��
		g_interface.Globals->curtime = serverTime;
		g_interface.Globals->frametime = g_interface.Globals->interval_per_tick;

		// ��ʼ������
		g_interface.GameMovement->StartTrackPredictionErrors(client);

		// ���Ԥ����������
		ZeroMemory(&g_predMoveData, sizeof(CMoveData));

		// ������ҪԤ������
		g_interface.MoveHelper->SetHost(client);

		// ��ʼԤ��
		g_interface.Prediction->SetupMove(client, pCmd, g_interface.MoveHelper, &g_predMoveData);
		g_interface.GameMovement->ProcessMovement(client, &g_predMoveData);
		g_interface.Prediction->FinishMove(client, pCmd, &g_predMoveData);
	}

	// �Զ���׼
	if (Config::bAimbot && weapon != nullptr && (GetAsyncKeyState(VK_LBUTTON) & 0x8000))
	{
		Vector myOrigin = client->GetEyePosition(), myAngles = pCmd->viewangles;

		// �Զ���׼���ݱ���
		static QAngle oldViewAngles;
		static float oldSidemove;
		static float oldForwardmove;
		static float oldUpmove;

		bool runAimbot = false;

		// Ŀ������һ���ط�ѡ��
		if (g_pCurrentAimbot != nullptr && IsGunWeapon(weaponId) && nextAttack <= serverTime)
		{
			// 目标位置
			Vector position;
			try
			{
				position = GetHeadHitboxPosition(g_pCurrentAimbot);
			}
			catch (...)
			{
				if (g_pCurrentAimbot->GetClientClass()->m_ClassID == ET_INFECTED)
					goto end_aimbot;

				// 获取骨骼位置失败
				position = g_pCurrentAimbot->GetEyePosition();
				Utils::log("CBasePlayer::SetupBone error");

				// 根据不同的情况确定高�
				int zombieClass = g_pCurrentAimbot->GetNetProp<int>("m_zombieClass", "DT_TerrorPlayer");
				if (zombieClass == ZC_JOCKEY)
					position.z = g_pCurrentAimbot->GetAbsOrigin().z + 30.0f;
				else if (zombieClass == ZC_HUNTER && (g_pCurrentAimbot->GetFlags() & FL_DUCKING))
					position.z -= 12.0f;
			}

			if (position.IsValid())
			{
				// ����ԭ����
				oldViewAngles = pCmd->viewangles;
				oldSidemove = pCmd->sidemove;
				oldForwardmove = pCmd->fowardmove;
				oldUpmove = pCmd->upmove;

				// �����Զ���׼
				runAimbot = true;

				// �ٶ�Ԥ��
				myOrigin = VelocityExtrapolate(client, myOrigin);
				position = VelocityExtrapolate(g_pCurrentAimbot, position);

				// ��׼��ת������ͷ��
				pCmd->viewangles = CalculateAim(myOrigin, position);
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

		if (Config::bSilentAimbot)
		{
			if (!runAimbot)
			{
				// ��ԭ�Ƕ�
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
			{
				// �޸ĽǶ�
				*bSendPacket = false;
			}
		}
	}

end_aimbot:

	// �Զ���ǹ
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
			Utils::log("%s (%d): δ֪�쳣 | 0x%X", __FILE__, __LINE__, (DWORD)g_pCurrentAiming);
			g_pCurrentAiming = nullptr;
			goto end_trigger_bot;
		}
#endif
		int classId = g_pCurrentAiming->GetClientClass()->m_ClassID;
		if (Config::bTriggerBotHead && IsCommonInfected(classId) &&
			(g_iCurrentHitbox < HITBOX_COMMON_1 || g_iCurrentHitbox > HITBOX_COMMON_4))
			goto end_trigger_bot;

		if (g_pCurrentAiming->GetTeam() != myTeam && classId != ET_WITCH &&					// ���������Ѻ� Witch
			g_pCurrentAiming->GetTeam() != 4 && (!IsSpecialInfected(classId) ||				// ������ L4D1 ������(��Ϊ�������޵е�)
				!IsPlayerGhost(g_iCurrentAiming) || !IsGhostInfected(g_pCurrentAiming)) &&	// ����������״̬���ظ�
				(myTeam == 2 || classId != ET_INFECTED))									// �ظж��鲻�����ո�
			pCmd->buttons |= IN_ATTACK;
	}

end_trigger_bot:

	// �ڿ�ǹǰ��飬��ֹ��������
	if (Config::bAnitFirendlyFire && (pCmd->buttons & IN_ATTACK) && IsGunWeapon(weaponId) && myTeam == 2)
	{
		if (IsValidVictim(g_pCurrentAiming) &&
			g_pCurrentAiming->GetTeam() == myTeam && !IsNeedRescue(g_pCurrentAiming))
		{
			// ȡ����ǹ
			pCmd->buttons &= ~IN_ATTACK;
		}
	}

	// ���������
	if (Config::bNoRecoil)
	{
		Vector punch = client->GetLocalNetProp<Vector>("m_vecPunchAngle");
		/*
		float modifiler = VectorNormalize(punch);
		modifiler -= (10.0f + modifiler * 0.5f) * g_interface.Globals->interval_per_tick;
		punch *= modifiler;
		*/
		pCmd->viewangles -= punch * 2.0f;
	}

	// ����Ԥ��
	if (g_interface.MoveHelper)
	{
		// ����Ԥ��
		// g_interface.Prediction->FinishMove(client, pCmd, &g_predMoveData);
		g_interface.GameMovement->FinishTrackPredictionErrors(client);
		g_interface.MoveHelper->SetHost(nullptr);

		// ��ԭ����
		g_interface.Globals->curtime = oldCurtime;
		g_interface.Globals->frametime = oldFrametime;

		// �޸�һЩ����
		client->SetNetProp("m_fFlags", flags, "DT_BasePlayer");

		static bool showPred = true;
		if (showPred)
		{
			showPred = false;
			Utils::log("engine prediction success.");
		}
	}

	// ��ǹ���� / Hunter ������
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
			// ����һ����ֹ��ǹ
			ignoreButton = false;
		}
	}

	// ��ս�������ٹ���
	if (myTeam == 2 && GetAsyncKeyState(VK_XBUTTON2) & 0x8000)
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
				// ��ս��������
				pCmd->buttons |= IN_ATTACK;
				fms = FMS_Primary;
			}
			else if (ignoreTick > 0)
				ignoreTick = 0;
			break;
		case FMS_Primary:
			if (weaponId == Weapon_Melee && nextAttack > serverTime)
			{
				// �ڹ���֮���л���������
				g_interface.Engine->ClientCmd("slot1");
				fms = FMS_Secondary;
			}
			else
				++ignoreTick;
			break;
		case FMS_Secondary:
			if (weaponId != Weapon_Melee)
			{
				// ��������ʱ�л�����ս����
				g_interface.Engine->ClientCmd("slot2");
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

			g_interface.Engine->ClientCmd("echo \"fastmelee stopped\"");
		}
	}

	// �޸��ǶȲ���ȷ
	ClampAngles(pCmd->viewangles);
	AngleNormalize(pCmd->viewangles);

	// ���͵�������
	pVerifiedCmd->m_cmd = *pCmd;
	pVerifiedCmd->m_crc = pCmd->GetChecksum();

	// ����ǰ��ť���浽ȫ�ֱ��������ڼ��һЩ����
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

	if (stage == FRAME_RENDER_START && g_interface.Engine->IsInGame())
	{
		if (client != nullptr && client->IsAlive())
		{
			punch = client->GetLocalNetProp<Vector>("m_vecPunchAngle");
			velocity = client->GetLocalNetProp<Vector>("m_vecPunchAngleVel");
		}

		// ���������ʹ�� DebugOverlay ������ 3D ����
	}

	oFrameStageNotify(stage);

	if (Config::bNoRecoil && client != nullptr && client->IsAlive() && punch.IsValid() && velocity.IsValid())
	{
		// ȥ����Ļ�ζ�
		client->SetLocalNetProp("m_vecPunchAngle", punch);
		client->SetLocalNetProp("m_vecPunchAngleVel", velocity);
	}

	static time_t nextUpdate = 0;
	time_t currentTime = time(NULL);
	if (nextUpdate <= currentTime)
	{
		// ��ʱ������ÿ�� 1 �봥��һ��
		nextUpdate = currentTime + 1;
		static DWORD fmWait = 45;
		static bool connected = false;

		if (g_interface.Engine->IsConnected())
		{
			static bool oldWireframe = false;
			if (GetAsyncKeyState(VK_INSERT) & 0x01)
				oldWireframe = !oldWireframe;

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
			if (GetAsyncKeyState(VK_HOME) & 0x01)
				oldWireframe = !oldWireframe;

			if (oldFullBright != Config::bCvarFullBright)
			{
#ifdef USE_CVAR_CHANGE
				CVAR_MAKE_FLAGS("mat_fullbright");
#endif

#ifdef USE_CVAR_CHANGE
				if (g_conVar["mat_fullbright"] != nullptr)
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
			if (GetAsyncKeyState(VK_PRIOR) & 0x01)
				oldWireframe = !oldWireframe;

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
			if (GetAsyncKeyState(VK_NEXT) & 0x01)
				oldWireframe = !oldWireframe;

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

			// ��ʾȫ�����
			/*
			if (GetAsyncKeyState(VK_CAPITAL) & 0x01)
			showSpectator();
			*/

			// ��/�ر� �Զ��������Զ������ٶ�
			if (GetAsyncKeyState(VK_F8) & 0x01)
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

			// ��/�ر� �Զ���ǹ
			if (GetAsyncKeyState(VK_F9) & 0x01)
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

			// ��/�ر� �Զ���׼
			if (GetAsyncKeyState(VK_F10) & 0x01)
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

			// ��/�ر� �ո��Զ�����
			if (GetAsyncKeyState(VK_F11) & 0x01)
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

			// ��/�ر� ������׼
			if (GetAsyncKeyState(VK_F12) & 0x01)
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

			// ȥ�� CRC ��֤
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
				g_pCurrentAimbot = nullptr;
				g_pCurrentAiming = nullptr;
				g_iCurrentAiming = 0;

				if (IsPlayerHost(g_interface.Engine->GetLocalPlayer()))
					Utils::log("*** connected by listen host ***");
				else
					Utils::log("*** connected ***");
			}

			connected = true;
		}
		else if (connected && !g_interface.Engine->IsInGame())
		{
			connected = false;
			g_pGameRulesProxy = nullptr;
			g_pCurrentAimbot = nullptr;
			g_pCurrentAiming = nullptr;
			g_iCurrentAiming = 0;

			Utils::log("*** disconnected ***");
		}

		/*
		if (GetAsyncKeyState(VK_ADD) & 0x8000)
		{
			g_interface.Engine->ClientCmd("alias fastmelee_loop \"+attack; slot1; wait 1; -attack; slot2; wait %d; fastmelee_launcher\"", ++fmWait);
			g_interface.Engine->ClientCmd("echo \"fastmelee wait set %d\"", fmWait);

			if (g_pDrawRender != nullptr)
				g_pDrawRender->PushRenderText(DrawManager::WHITE, "fast melee wait tick set %d", fmWait);
		}

		if (GetAsyncKeyState(VK_SUBTRACT) & 0x8000)
		{
			g_interface.Engine->ClientCmd("alias fastmelee_loop \"+attack; slot1; wait 1; -attack; slot2; wait %d; fastmelee_launcher\"", --fmWait);
			g_interface.Engine->ClientCmd("echo \"fastmelee wait set %d\"", fmWait);

			if (g_pDrawRender != nullptr)
				g_pDrawRender->PushRenderText(DrawManager::WHITE, "fast melee wait tick set %d", fmWait);
		}
		*/

		if (GetAsyncKeyState(VK_MULTIPLY) & 0x8000)
		{
			Config::fAimbotFov += 1.0f;
			g_interface.Engine->ClientCmd("echo \"aimbot fov set %.2f\"", Config::fAimbotFov);

			if (g_pDrawRender != nullptr)
				g_pDrawRender->PushRenderText(DrawManager::WHITE, "aimbot angles set %.0f", Config::fAimbotFov);
		}

		if (GetAsyncKeyState(VK_DIVIDE) & 0x8000)
		{
			Config::fAimbotFov -= 1.0f;
			g_interface.Engine->ClientCmd("echo \"aimbot fov set %.2f\"", Config::fAimbotFov);

			if (g_pDrawRender != nullptr)
				g_pDrawRender->PushRenderText(DrawManager::WHITE, "aimbot angles set %.0f", Config::fAimbotFov);
		}

		static byte iExitGame = 0;
		if (GetAsyncKeyState(VK_END) & 0x8000)
		{
			// ��ס End ������ǿ���˳���Ϸ��Ϸ��������Ϸ����Ӧ
			if (++iExitGame >= 3)
				ExitProcess(0);

			if (g_pDrawRender != nullptr)
				g_pDrawRender->PushRenderText(DrawManager::WHITE, "force exit proccess timer (%d/3)", iExitGame);
		}
		else if (iExitGame != 0)
		{
			// ��ס���� 3 ��ͷſ���ȡ����ʱ
			iExitGame = 0;
			if (g_pDrawRender != nullptr)
				g_pDrawRender->PushRenderText(DrawManager::WHITE, "force exit proccess timer stopped");
		}

		if (GetAsyncKeyState(VK_DELETE) & 0x01)
			g_interface.Engine->ClientCmd("disconnect");
	}
}

void __fastcall Hooked_RunCommand(CPrediction* ecx, void* edx, CBaseEntity* pEntity, CUserCmd* pCmd, CMoveHelper* moveHelper)
{
	oRunCommand(ecx, pEntity, pCmd, moveHelper);

	static bool showHint = true;
	if (showHint || g_interface.MoveHelper == nullptr)
	{
		showHint = false;
		Utils::log("MoveHelperPointer = 0x%X", (DWORD)moveHelper);
	}

	g_interface.MoveHelper = moveHelper;
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
		// ��֧�� push byte. ����ֻ�� push word
		WORD wFinalTick = bFinalTick;

		__asm
		{
			// �Ĵ�������
			mov		bl, _bl
			mov		edi, _edi

			// ��ջ����
			push	wFinalTick
			push	accumulated_extra_samples

			// ����ԭ����(��ʵ�Ǹ��Ĵ�)
			call	oCL_Move

			// �����ջ
			add		esp, 6
		};
	};

	// Ĭ�ϵ� 1 �ε��ã���������ûᵼ����Ϸ����
	// ���� bFinalTick �൱�� bSendPacket
	CL_Move(accumulated_extra_samples, bFinalTick);

	if (GetAsyncKeyState(VK_CAPITAL) & 0x8000)
	{
		static bool showSpeed = true;
		if (showSpeed)
		{
			showSpeed = false;
			Utils::log("SpeedHack speed = %d", g_iSpeedMultiple);
		}

		for (int i = 1; i < g_iSpeedMultiple; ++i)
		{
			// ��ε��������м���Ч��
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

	// ��������ƶ���������� PaintTraverse ����
	// ��Ϊ PaintTraverse �ή����Ϸ�� fps 30~40 ����
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

void __fastcall Hooked_EnginePaint(CEngineVGui *ecx, void *edx, PaintMode_t mode)
{
	static bool showHint = true;
	if (showHint)
	{
		showHint = false;
		Utils::log("Hooked_EnginePaint trigged.");
	}
	
	oEnginePaint(ecx, mode);

	if (mode & PAINT_UIPANELS)
	{
		PaintStartDrawing(g_interface.Surface);

		// ��������ƶ����� PaintTraverse ����

		PaintFinishDrawing(g_interface.Surface);
	}
}

bool __fastcall Hooked_EngineKeyEvent(CEngineVGui *ecx, void *edx, const InputEvent_t &event)
{
	static bool showHint = true;
	if (showHint)
	{
		showHint = false;
		Utils::log("Hooked_EngineKeyEvent trigged.");
	}
	
	bool result = oEngineKeyEvent(ecx, event);

	bool isDown = event.m_nType != IE_ButtonReleased;
	ButtonCode_t code = (ButtonCode_t)event.m_nData;
	
	return result;
}
