#include "main.h"

#define USE_PLAYER_INFO
#define USE_CVAR_CHANGE
#define USE_D3D_DRAW

// D3D �ĺ��� jmp �ҹ�
static DetourXS *g_detReset, *g_detPresent, *g_detEndScene, *g_detDrawIndexedPrimitive,
*g_detCreateQuery;

CNetVars* g_pNetVars;
CInterfaces g_cInterfaces;

// D3D Device ���ҹ�
static CVMTHookManager* g_vmtDeviceHooker;

// ��ǰ dll �ļ�ʵ��
HINSTANCE g_hMyInstance;

DWORD WINAPI StartCheat(LPVOID params);

BOOL WINAPI DllMain(HINSTANCE module, DWORD reason, LPVOID reserved)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		g_hMyInstance = module;

		// ��ȡ���нӿ�
		g_cInterfaces.GetInterfaces();

		// ��ʼ�� NetProp ��
		g_pNetVars = new CNetVars();

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
		DETOURXS_DESTORY(g_detReset);
		DETOURXS_DESTORY(g_detPresent);
		DETOURXS_DESTORY(g_detEndScene);
		DETOURXS_DESTORY(g_detDrawIndexedPrimitive);
		DETOURXS_DESTORY(g_detCreateQuery);
		VMTHOOK_DESTORY(g_cInterfaces.ClientModeHook);
		VMTHOOK_DESTORY(g_cInterfaces.PanelHook);
		VMTHOOK_DESTORY(g_cInterfaces.ClientHook);
		VMTHOOK_DESTORY(g_cInterfaces.PredictionHook);
		VMTHOOK_DESTORY(g_cInterfaces.ModelRenderHook);
		VMTHOOK_DESTORY(g_vmtDeviceHooker);
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

// -------------------------------- Game Hook Function --------------------------------
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

typedef void(__stdcall* FnRunCommand)(CBaseEntity*, CUserCmd*, CMoveHelper*);
void __stdcall Hooked_RunCommand(CBaseEntity*, CUserCmd*, CMoveHelper*);
static FnRunCommand oRunCommand;

typedef void(__stdcall* FnDrawModel)(PVOID, PVOID, const ModelRenderInfo_t&, matrix3x4_t*);
void __stdcall Hooked_DrawModel(PVOID, PVOID, const ModelRenderInfo_t&, matrix3x4_t*);
static FnDrawModel oDrawModel;

// -------------------------------- Game Call Function --------------------------------
typedef void(__cdecl* FnConColorMsg)(class Color const&, char const*, ...);
static FnConColorMsg PrintToConsoleColor;	// ��ӡ��Ϣ������̨��֧����ɫ��

typedef void(__cdecl* FnConMsg)(char const*, ...);
static FnConMsg PrintToConsole;				// ��ӡ��Ϣ������̨

typedef int(__stdcall* FnCL_Move)(double, float, bool);
int __stdcall Hooked_CL_Move(double, float, bool);
FnCL_Move oCL_Move;							// ������ݴ���

typedef bool(__thiscall* FnDispatchUserMessage)(void*, int, bf_read&);
FnDispatchUserMessage DispatchUserMessage;	// �û���Ϣ����

typedef void(__cdecl* FnSharedRandomFloat)(const char*, float, float, int);
FnSharedRandomFloat SharedRandomFloat;		// �����

typedef int(__stdcall* FnTraceLine)(void* ebp, void* esi, const Vector&, const Vector&, unsigned int mask,
	const IHandleEntity*, int, trace_t*);
FnTraceLine UTIL_TraceRay;					// ���߸���

typedef const char*(__cdecl* FnWeaponIdToAlias)(unsigned int);
FnWeaponIdToAlias WeaponIdToAlias;			// ��ȡ����������

											// -------------------------------- Cheats Function --------------------------------
void thirdPerson();
void showSpectator();
void bindAlias(int);

// -------------------------------- Golbals Variable --------------------------------
static DrawManager* g_pDrawRender;										// D3D EndScene ���ƹ���
static D3D9Hooker* g_pDeviceHooker;										// D3D9 Device ����
static bool* g_pbSendPacket;											// ���ݰ��Ƿ��͵�������
static CUserCmd* g_pUserCommands;										// ������ҵ�ǰ����
std::map<std::string, ConVar*> g_tConVar;								// ����̨����
static float g_fAimbotFieldOfView = 30.0f;								// �Զ���׼�Ƕ�
static CBaseEntity* g_pCurrentAiming;									// ��ǰ���Զ���׼Ŀ��
static DWORD g_iClientModules, g_iEngineModules, g_iMaterialModules;	// ���õ� DLL �ļ���ַ
static bool g_bDrawBoxEsp = true, g_bTriggerBot = false, g_bAimBot = false, g_bAutoBunnyHop = true,
g_bRapidFire = true, g_bSilentAimbot = false, g_bAutoStrafe = false, g_bDrawCrosshairs = true;

std::string GetZombieClassName(CBaseEntity* player);
bool IsValidEntity(CBaseEntity* entity);

DWORD WINAPI StartCheat(LPVOID params)
{
	g_iClientModules = Utils::GetModuleBase("client.dll");
	g_iEngineModules = Utils::GetModuleBase("engine.dll");
	g_iMaterialModules = Utils::GetModuleBase("materialsystem.dll");

	Utils::log("client.dll 0x%X", g_iClientModules);
	Utils::log("engine.dll 0x%X", g_iEngineModules);
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
	Utils::log("Input 0x%X", (DWORD)g_cInterfaces.Input);
	Utils::log("UserMessages 0x%X", (DWORD)g_cInterfaces.UserMessage);
	Utils::log("MoveHelper 0x%X", (DWORD)g_cInterfaces.MoveHelper);

	if ((oCL_Move = (FnCL_Move)Utils::FindPattern("engine.dll",
		XorStr("55 8B EC B8 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 33 C5 89 45 FC 53 56 57 E8"))) != nullptr)
	{
		g_pbSendPacket = (bool*)((DWORD)oCL_Move + 0x91);
		Utils::log("CL_Move = 0x%X | bSendPacket = 0x%X", (DWORD)oCL_Move, (DWORD)g_pbSendPacket);
	}
	else
		Utils::log("CL_Move not found");

	if ((SharedRandomFloat = (FnSharedRandomFloat)Utils::FindPattern("client.dll",
		XorStr("55 8B EC 83 EC 08 A1 ? ? ? ? 53 56 57 8B 7D 14 8D 4D 14 51 89 7D F8 89 45 FC E8 ? ? ? ? 6A 04 8D 55 FC 52 8D 45 14 50 E8 ? ? ? ? 6A 04 8D 4D F8 51 8D 55 14 52 E8 ? ? ? ? 8B 75 08 56 E8 ? ? ? ? 50 8D 45 14 56 50 E8 ? ? ? ? 8D 4D 14 51 E8 ? ? ? ? 8B 15 ? ? ? ? 8B 5D 14 83 C4 30 83 7A 30 00 74 26 57 53 56 68 ? ? ? ? 68 ? ? ? ? 8D 45 14 68 ? ? ? ? 50 C7 45 ? ? ? ? ? FF 15 ? ? ? ? 83 C4 1C 53 B9 ? ? ? ? FF 15 ? ? ? ? D9 45 10"))) != nullptr)
		Utils::log("SharedRandomFloat = 0x%X", (DWORD)SharedRandomFloat);
	else
		Utils::log("SharedRandomFloat not found");

	if ((UTIL_TraceRay = (FnTraceLine)Utils::FindPattern("client.dll",
		XorStr("53 8B DC 83 EC 08 83 E4 F0 83 C4 04 55 8B 6B 04 89 6C 24 04 8B EC 83 EC 6C 56 8B 43 08"))) != nullptr)
		Utils::log("UTIL_TraceRay = 0x%X", (DWORD)UTIL_TraceRay);
	else
		Utils::log("UTIL_TraceRay not found");

	if ((WeaponIdToAlias = (FnWeaponIdToAlias)Utils::FindPattern("client.dll",
		XorStr("55 8B EC 8B 45 08 83 F8 37"))) != nullptr)
		Utils::log("WeaponIdToAlias = 0x%X", (DWORD)WeaponIdToAlias);
	else
		Utils::log("WeaponIdToAlias not found");

	if ((DispatchUserMessage = (FnDispatchUserMessage)Utils::FindPattern("client.dll",
		XorStr("55 8B EC 8B 45 08 83 EC 28 85 C0"))) != nullptr)
		Utils::log("DispatchUserMessage = 0x%X", (DWORD)DispatchUserMessage);
	else
		Utils::log("DispatchUserMessage not found");

	typedef ClientModeShared*(*FnGetClientMode)();
	FnGetClientMode GetClientModeNormal = nullptr;
	if ((GetClientModeNormal = (FnGetClientMode)Utils::FindPattern("client.dll",
		XorStr("8B 0D ? ? ? ? 8B 01 8B 90 ? ? ? ? FF D2 8B 04 85 ? ? ? ? C3"))) != nullptr &&
		(g_cInterfaces.ClientMode = GetClientModeNormal()) != nullptr)
	{
		g_cInterfaces.ClientModeHook = new CVMTHookManager(g_cInterfaces.ClientMode);
		// printo("ClientModePtr", g_cInterfaces.ClientMode);
		Utils::log("ClientModeShared = 0x%X", (DWORD)g_cInterfaces.ClientMode);
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

	/*
	if (g_cInterfaces.ClientModeHook && indexes::SharedCreateMove > -1)
	{
	oCreateMoveShared = (FnCreateMoveShared)g_cInterfaces.ClientModeHook->HookFunction(indexes::SharedCreateMove, Hooked_CreateMoveShared);
	g_cInterfaces.ClientModeHook->HookTable(true);
	Utils::log("oCreateMoveShared = 0x%X", (DWORD)oCreateMoveShared);
	}
	*/

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

	if (g_cInterfaces.PredictionHook && indexes::RunCommand > -1)
	{
		oRunCommand = (FnRunCommand)g_cInterfaces.ClientHook->HookFunction(indexes::RunCommand, Hooked_RunCommand);
		g_cInterfaces.ClientHook->HookTable(true);
		Utils::log("oRunCommand = 0x%X", (DWORD)oRunCommand);
	}

	if (g_cInterfaces.ModelRenderHook && indexes::DrawModel > -1)
	{
		oDrawModel = (FnDrawModel)g_cInterfaces.ClientHook->HookFunction(indexes::DrawModel, Hooked_DrawModel);
		g_cInterfaces.ClientHook->HookTable(true);
		Utils::log("oDrawModel = 0x%X", (DWORD)oDrawModel);
	}

	HMODULE tier0 = Utils::GetModuleHandleSafe("tier0.dll");
	if (tier0 != NULL)
	{
		PrintToConsole = (FnConMsg)GetProcAddress(tier0, "?ConMsg@@YAXPBDZZ");
		PrintToConsoleColor = (FnConColorMsg)GetProcAddress(tier0, "?ConColorMsg@@YAXABVColor@@PBDZZ");
		Utils::log("PrintToConsole = 0x%X", (DWORD)PrintToConsole);
		Utils::log("PrintToConsoleColor = 0x%X", (DWORD)PrintToConsoleColor);
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

	g_pDeviceHooker = new D3D9Hooker();
	g_pDeviceHooker->StartDeviceHook([&](IDirect3D9* pD3D, IDirect3DDevice9* pDeivce, DWORD* pVMT) -> void
	{
		// �麯�����޸���ת
		g_detReset = new DetourXS((void*)pVMT[16], Hooked_Reset);
		g_detPresent = new DetourXS((void*)pVMT[17], Hooked_Present);
		g_detEndScene = new DetourXS((void*)pVMT[42], Hooked_EndScene);
		g_detDrawIndexedPrimitive = new DetourXS((void*)pVMT[82], Hooked_DrawIndexedPrimitive);
		g_detCreateQuery = new DetourXS((void*)pVMT[118], Hooked_CreateQuery);

		// ��ȡԭ����
		oReset = (FnReset)g_detReset->GetTrampoline();
		oPresent = (FnPresent)g_detPresent->GetTrampoline();
		oEndScene = (FnEndScene)g_detEndScene->GetTrampoline();
		oDrawIndexedPrimitive = (FnDrawIndexedPrimitive)g_detDrawIndexedPrimitive->GetTrampoline();
		oCreateQuery = (FnCreateQuery)g_detCreateQuery->GetTrampoline();

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
					// �ܺ��ߣ����ߣ�
					int victim = g_cInterfaces.Engine->GetPlayerForUserID(event->GetInt("userid"));
					if (victim <= 0)
						victim = event->GetInt("entityid");

					// �����ߣ���ɱ�ߣ�
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

					if (g_pDrawRender)
					{
						g_pDrawRender->PushRenderText(DrawManager::ORANGE,
							"client %s (%s) connect... ip %s", name, steamId, ip);
					}
				}
				else if (_strcmpi(eventName, "player_disconnect") == 0)
				{
					if (event->GetInt("bot"))
						return;

					const char* name = event->GetString("name");
					const char* steamId = event->GetString("networkid");
					const char* reason = event->GetString("reason");

					if (g_pDrawRender)
					{
						g_pDrawRender->PushRenderText(DrawManager::ORANGE,
							"client %s (%s) disconnect... reason %s", name, steamId, reason);
					}
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

					if (g_pDrawRender)
					{
						g_pDrawRender->PushRenderText(DrawManager::ORANGE,
							"player %s join %d from %d", GetZombieClassName(entity).c_str(),
							newTeam, oldTeam);
					}
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
					// �ܺ���
					int victim = g_cInterfaces.Engine->GetPlayerForUserID(event->GetInt("userid"));

					// ������
					int attacker = g_cInterfaces.Engine->GetPlayerForUserID(event->GetInt("attacker"));

					// ��˭�Ĵ�
					int guilty = g_cInterfaces.Engine->GetPlayerForUserID(event->GetInt("guilty"));

					// �˺�����
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
					// �ܺ���
					int victim = g_cInterfaces.Engine->GetPlayerForUserID(event->GetInt("userid"));

					// ������
					int attacker = g_cInterfaces.Engine->GetPlayerForUserID(event->GetInt("attacker"));

					if (victim <= 0)
						return;

					if (g_pDrawRender)
					{
						g_pDrawRender->PushRenderText(DrawManager::GREEN,
							"tank %d killed by %d", victim, attacker);
					}
				}
			}
		};

		EventListener* listener = new EventListener();
		if (!g_cInterfaces.GameEvent->AddListener(listener, "player_death", false))
			Utils::log("HookEvent player_death fail");

		if (!g_cInterfaces.GameEvent->AddListener(listener, "infected_death", false))
			Utils::log("HookEvent infected_death fail");

		g_cInterfaces.GameEvent->AddListener(listener, "player_spawn", false);
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
	}

	return 0;
}

void ResetDeviceHook(IDirect3DDevice9* device)
{
	// ������Ϸʹ�õ� IDirect3DDevice9 ����
	g_pDeviceHooker->GetDevice() = device;

	// ���޸ĵ� Jump ��ԭ
	DETOURXS_DESTORY(g_detReset);
	DETOURXS_DESTORY(g_detPresent);
	DETOURXS_DESTORY(g_detEndScene);
	DETOURXS_DESTORY(g_detDrawIndexedPrimitive);
	DETOURXS_DESTORY(g_detCreateQuery);

	// ʹ�� VMT �� Hook
	g_vmtDeviceHooker = new CVMTHookManager(device);
	oReset = g_vmtDeviceHooker->SetupHook(16, Hooked_Reset);
	oPresent = g_vmtDeviceHooker->SetupHook(17, Hooked_Present);
	oEndScene = g_vmtDeviceHooker->SetupHook(42, Hooked_EndScene);
	oDrawIndexedPrimitive = g_vmtDeviceHooker->SetupHook(82, Hooked_DrawIndexedPrimitive);
	oCreateQuery = g_vmtDeviceHooker->SetupHook(118, Hooked_CreateQuery);
	g_vmtDeviceHooker->HookTable(true);

	// ��ʼ����ͼ
	g_pDrawRender = new DrawManager(device);

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
		// �����˳�
		local->SetNetProp<int>("m_hObserverTarget", 0, "DT_BasePlayer");
		local->SetNetProp<int>("m_iObserverMode", 1, "DT_BasePlayer");
		local->SetNetProp<int>("m_bDrawViewmodel", 0, "DT_BasePlayer");
		}
		else if (local->GetNetProp<int>("m_hObserverTarget", "DT_BasePlayer") == 0)
		{
		// ��һ�˳�
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
				// 观察�
				if (mode != 4 && mode != 5)
					continue;

				target = (CBaseEntity*)player->GetNetProp<CBaseHandle*>("m_iObserverMode", "DT_BasePlayer");
				if (target && (int)target != -1)
					target = g_cInterfaces.ClientEntList->GetClientEntityFromHandle((CBaseHandle*)target);

				if (target == nullptr || (int)target == -1)
					continue;

				if ((DWORD)target == (DWORD)local)
				{
					// 正在观察本地玩家
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
					// 正在观察其他玩家
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
				// 生还�
				if (player->IsAlive())
				{
					// 活着
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
					// 死亡
					if (mode != 4 && mode != 5)
						continue;

					target = (CBaseEntity*)player->GetNetProp<CBaseHandle*>("m_iObserverMode", "DT_BasePlayer");
					if (target && (int)target != -1)
						target = g_cInterfaces.ClientEntList->GetClientEntityFromHandle((CBaseHandle*)target);

					if (target == nullptr || (int)target == -1)
						continue;

					if ((DWORD)target == (DWORD)local)
					{
						// 正在观察本地玩家
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
						// 正在观察其他玩家
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
					g_cInterfaces.Engine->ClientCmd("echo \"[infected] player %s is %s (%d)\"",
						info.name, zombieName, player->GetHealth());
#else
					g_cInterfaces.Engine->ClientCmd("echo \"[infected] player %d is %s (%d)\"",
						player->GetIndex(), zombieName, player->GetHealth());
#endif
				}
				else if (player->GetNetProp<byte>("m_isGhost", "DT_TerrorPlayer"))
				{
					// 幽灵状�
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
					// 死亡
					if (mode != 4 && mode != 5)
						continue;

					target = (CBaseEntity*)player->GetNetProp<CBaseHandle*>("m_iObserverMode", "DT_BasePlayer");
					if (target && (int)target != -1)
						target = g_cInterfaces.ClientEntList->GetClientEntityFromHandle((CBaseHandle*)target);

					if (target == nullptr || (int)target == -1)
						continue;

					if ((DWORD)target == (DWORD)local)
					{
						// 正在观察本地玩家
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
						// 正在观察其他玩家
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

// ����ʵ���Ƿ�Ϊ������/�ظ�/�ո�/Witch
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
		Utils::log("%s (%d) ���棺ָ�� 0x%X ������ʵ�壡", __FILE__, __LINE__, (DWORD)entity);
		throw std::exception("IsValidEntity �����˴���");
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
			position.z = g_pCurrentAiming->GetAbsOrigin().z + 30.0f;
		else if (zombieClass == ZC_HUNTER && (g_pCurrentAiming->GetFlags() & FL_DUCKING))
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

	if (!position.IsValid())
		position = GetHeadPosition(entity);

	return position;
}

// ��ȡ��ǰ������׼�ĵ���
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

	// 检查目标是否为一个有效的实体
	if (trace.m_pEnt == nullptr || trace.m_pEnt->IsDormant() ||
		trace.m_pEnt->GetClientClass()->m_ClassID == ET_WORLD)
	{
#ifdef _DEBUG_OUTPUT
		g_cInterfaces.Engine->ClientCmd("echo \"invalid entity 0x%X | start (%.2f %.2f %.2f) end (%.2f %.2f %.2f)\"",
			(DWORD)trace.m_pEnt, trace.start.x, trace.start.y, trace.start.z, trace.end.x, trace.end.y, trace.end.z);
#endif
		return nullptr;
	}

	// 检查目标是否为一个可见的物体，或者该物体是否可以被击中，以及是否为指定位
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

// ��鱾������Ƿ���Կ���ĳ��ʵ��
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
		Utils::log("%s (%d): δ֪�쳣 | 0x%X", __FILE__, __LINE__, (DWORD)entity);
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

	// ����֮ǰ��������
	g_pDrawRender->BeginRendering();

#ifdef USE_D3D_DRAW
	CBaseEntity* local = GetLocalClient();
	if (local != nullptr && g_cInterfaces.Engine->IsInGame())
	{
		// Ŀǰ��С����
		float distmin = 65535.0f;

		// ��ǰ����
		int team = local->GetTeam();

		// ��ǰ�Ƿ����Զ���׼��Ŀ��
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
			Utils::log("%s (%d): δ֪�쳣 | 0x%X", __FILE__, __LINE__, (DWORD)g_pCurrentAiming);
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

		int maxEntity = g_cInterfaces.ClientEntList->GetHighestEntityIndex();
		for (int i = 1; i <= maxEntity; ++i)
		{
			CBaseEntity* entity = g_cInterfaces.ClientEntList->GetClientEntity(i);
			if (entity == nullptr || (DWORD)entity == (DWORD)local)
				continue;

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
				Utils::log("%s (%d): δ֪�쳣 | 0x%X", __FILE__, __LINE__, (DWORD)entity);
				continue;
			}
#endif

			Vector head, foot, headbox, origin;
			int classId = entity->GetClientClass()->m_ClassID;

			// Ŀ���ͷ����λ��
			headbox = GetHeadHitboxPosition(entity);

			// Ŀ����µ�λ��
			origin = (classId == ET_INFECTED || classId == ET_WITCH ?
				entity->GetNetProp<Vector>("m_vecOrigin", "DT_BaseCombatCharacter") : entity->GetAbsOrigin());

			// ���Ŀ���Ƿ�����Ļ��
			if (!headbox.IsValid() || !WorldToScreen(headbox, head) ||
				!WorldToScreen(origin, foot))
				continue;

			// Ŀ���Ƿ�ɼ�
			bool visible = IsTargetVisible(entity, headbox, myEyeOrigin);

			// Ŀ�����Լ��ľ���
			float dist = myOrigin.DistTo(origin);

			// ����һ���һ����
			if (g_bDrawBoxEsp)
			{
				// �������;������Ƶ�����
				if (classId != ET_INFECTED && classId != ET_WITCH)
				{
					// ���ڸ�ʽ���ַ���
					std::stringstream ss;

					// ȥ�� float ��С��λ����Ϊû��Ҫ
					ss << std::setprecision(0);

					// ����Ƿ�Ϊ������
					if (classId == ET_SURVIVORBOT || classId == ET_CTERRORPLAYER)
					{
						if (IsIncapacitated(entity))
						{
							// ����ʱֻ����ͨѪ��
							ss << "[" << entity->GetHealth() << " + incap] ";
						}
						else if (IsControlled(entity))
						{
							// ��ұ�����
							ss << "[" << entity->GetHealth() +
								entity->GetNetProp<float>("m_healthBuffer", "DT_TerrorPlayer") <<
								" + grabbed] ";
						}
						else
						{
							// ��������ʾѪ������ʱѪ��
							ss << "[" << entity->GetHealth() << " + " <<
								entity->GetNetProp<float>("m_healthBuffer", "DT_TerrorPlayer") << "] ";
						}
					}
					else
					{
						if (entity->GetNetProp<byte>("m_isGhost", "DT_TerrorPlayer") != 0)
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

					float height = fabs(head.y - foot.y);
					float width = height * 0.65f;

					g_pDrawRender->DrawString2Begin();

					// �������������ɫ
					if ((classId == ET_SURVIVORBOT || classId == ET_CTERRORPLAYER) &&
						entity->GetNetProp<byte>("m_bIsOnThirdStrike", "DT_TerrorPlayer") != 0)
					{
						// �����ߺڰ�ʱʹ�ð�ɫ
						g_pDrawRender->DrawString2(foot.x - width / 2, head.y, DrawManager::WHITE, ss.str().c_str());
					}
					else if ((classId == ET_BOOMER || classId == ET_SMOKER || classId == ET_HUNTER ||
						classId == ET_SPITTER || classId == ET_CHARGER || classId == ET_JOCKEY) &&
						entity->GetNetProp<byte>("m_isGhost", "DT_TerrorPlayer") != 0)
					{
						// ����״̬���ظУ���ɫ
						g_pDrawRender->DrawString2(foot.x - width / 2, head.y,
							DrawManager::PURPLE, ss.str().c_str());
					}
					else
					{
						// �����������ɫ
						g_pDrawRender->DrawString2(foot.x - width / 2, head.y,
							DrawManager::ORANGE, ss.str().c_str());
					}

					ss.str("");

					// ��ʾ����
					ss << dist;

					// ����������ʾ��ҩ
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
									ss << " (" << clip << " / " <<
										entity->GetNetProp<int>("m_iAmmo", "DT_TerrorPlayer", (size_t)ammoType) <<
										")";
								}
							}
						}
					}

					// ����Ƿ���Կ���
					if (visible)
					{
						// ���ü�����ʾ��ɫ
						g_pDrawRender->DrawString2(foot.x - width / 2,
							head.y + g_pDrawRender->GetFontSize(),
							DrawManager::SKYBLUE, ss.str().c_str());
					}
					else
					{
						// ����������ʾ��ɫ
						g_pDrawRender->DrawString2(foot.x - width / 2,
							head.y + g_pDrawRender->GetFontSize(),
							DrawManager::YELLOW, ss.str().c_str());
					}

					g_pDrawRender->DrawString2Finish();

					// ����һ����
					if(entity->GetTeam() == team)
					{
						// ������ɫ
						g_pDrawRender->RenderRect(DrawManager::DEEPSKYBLUE, foot.x - width / 2, foot.y,
							width, -height);
					}
					else
					{
						// ���˺�ɫ
						g_pDrawRender->RenderRect(DrawManager::RED, foot.x - width / 2, foot.y,
							width, -height);
					}
				}
				else
				{
					// ��ֻ���ոж��ѣ�̫Զ��û��Ҫ��ʾ����
					if (dist > 2500.0f)
						continue;

					// ��һ��С���Σ��Ա��Ϊͷ��
					if (classId == ET_WITCH)
					{
						if (visible)
						{
							// ��ɫ - ��
							g_pDrawRender->RenderFillRect(DrawManager::PINK, head.x, head.y, 4, 4);
						}
						else
						{
							// ��ɫ - С
							g_pDrawRender->RenderFillRect(DrawManager::PINK, head.x, head.y, 2, 2);
						}
					}
					else
					{
						if (visible)
						{
							// ��ɫ - ��
							g_pDrawRender->RenderFillRect(DrawManager::ORANGE, head.x, head.y, 4, 4);
						}
						else
						{
							// ��ɫ - С
							g_pDrawRender->RenderFillRect(DrawManager::ORANGE, head.x, head.y, 2, 2);
						}
					}
				}
			}

			if (g_bAimBot && (!targetSelected || !(g_pUserCommands->buttons & IN_ATTACK)) &&
				classId != ET_WITCH && (classId != ET_INFECTED || team == 2))
			{
				// �Ѿ�ѡ���Ŀ���ˣ���������һ������Ҫ�ĵ���
				if (classId == ET_INFECTED && distmin < 65535.0f)
					continue;

				// ѡ��һ����ӽ����ظУ���Ϊ�ظ�Խ���������˵ԽΣ��
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
			width /= 2;
			height /= 2;

			int aiming = *(int*)(local + m_iCrosshairsId);
			CBaseEntity* target = (aiming > 0 ? g_cInterfaces.ClientEntList->GetClientEntity(aiming) :
				GetAimingTarget(-1));

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
				Utils::log("%s (%d): δ֪�쳣 | 0x%X", __FILE__, __LINE__, (DWORD)target);
				target = nullptr;
			}
#endif

			D3DCOLOR color = DrawManager::GREEN;
			if (target != nullptr)
			{
				int classId = target->GetClientClass()->m_ClassID;
				if (target->GetTeam() == local->GetTeam())
				{
					// ���� - ��ɫ
					color = DrawManager::RED;
				}
				else if (classId == ET_INFECTED)
				{
					// �ո� - ��ɫ
					color = DrawManager::ORANGE;
				}
				else if (classId == ET_WITCH)
				{
					// Witch - ��ɫ
					color = DrawManager::PINK;
				}
			}
			
			// ����ʮ��׼��
			g_pDrawRender->RenderLine(color, width - 9, height, width + 9, height);
			g_pDrawRender->RenderLine(color, width, height - 9, width, height + 9);
		}
	}

#endif

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

	/*	��Ҫ����������ᵼ����Ļ��ڵ�
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

	if (FocusOverlayPanel > 0 && panel == FocusOverlayPanel)
	{

	}

	if (MatSystemTopPanel > 0 && panel == MatSystemTopPanel)
	{
#ifndef USE_D3D_DRAW
		CBaseEntity* local = GetLocalClient();
		if (local == nullptr || !g_cInterfaces.Engine->IsInGame())
			goto finish_draw;

		// Ŀǰ��С����
		float distmin = 65535.0f;

		// ��ǰ����
		int team = local->GetTeam();

		// ��ǰ�Ƿ����Զ���׼��Ŀ��
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
			Utils::log("%s (%d): δ֪�쳣 | 0x%X", __FILE__, __LINE__, (DWORD)g_pCurrentAiming);
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

		// һ���ո�ʵ���������� 512 �͹��ˣ�̫��Ῠ��
		for (int i = 1; i <= 512; ++i)
		{
			CBaseEntity* entity = g_cInterfaces.ClientEntList->GetClientEntity(i);
			if ((DWORD)entity == (DWORD)local)
				continue;

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
				Utils::log("%s (%d): δ֪�쳣 | 0x%X", __FILE__, __LINE__, (DWORD)entity);
				continue;
			}
#endif

			Vector head, foot, headbox, origin;
			int classId = entity->GetClientClass()->m_ClassID;

			// Ŀ���ͷ����λ��
			headbox = GetHeadHitboxPosition(entity);

			// Ŀ����µ�λ��
			origin = (classId == ET_INFECTED || classId == ET_WITCH ?
				entity->GetNetProp<Vector>("m_vecOrigin", "DT_BaseCombatCharacter") : entity->GetAbsOrigin());

			// ���Ŀ���Ƿ�����Ļ��
			if (!headbox.IsValid() || !WorldToScreen(headbox, head) ||
				!WorldToScreen(origin, foot))
				continue;

			// Ŀ���Ƿ�ɼ�
			bool visible = IsTargetVisible(entity, headbox, myEyeOrigin);

			// Ŀ�����Լ��ľ���
			float dist = myOrigin.DistTo(origin);

			// ����һ���һ����
			if (g_bDrawBoxEsp)
			{
				// �������;������Ƶ�����
				if (classId != ET_INFECTED && classId != ET_WITCH)
				{
					// ���ڸ�ʽ���ַ���
					std::wstringstream ss;

					// ȥ�� float ��С��λ����Ϊû��Ҫ
					ss << std::setprecision(0);

					// ����Ƿ�Ϊ������
					if (classId == ET_SURVIVORBOT || classId == ET_CTERRORPLAYER)
					{
						if (IsIncapacitated(entity))
						{
							// ����ʱֻ����ͨѪ��
							ss << L"[" << entity->GetHealth() << L" + incap] ";
						}
						else if (IsControlled(entity))
						{
							// ��ұ�����
							ss << L"[" << entity->GetHealth() +
								entity->GetNetProp<float>("m_healthBuffer", "DT_TerrorPlayer") <<
								L" + grabbed] ";
						}
						else
						{
							// ��������ʾѪ������ʱѪ��
							ss << L"[" << entity->GetHealth() << L" + " <<
								entity->GetNetProp<float>("m_healthBuffer", "DT_TerrorPlayer") << "] ";
						}
					}
					else
					{
						if (entity->GetNetProp<byte>("m_isGhost", "DT_TerrorPlayer") != 0)
						{
							// ����״̬���ظ�
							ss << L"[" << entity->GetHealth() << L" ghost] ";
						}
						else
						{
							// ��������ֻ��ʾѪ���ͺ���
							ss << L"[" << entity->GetHealth() << L"] ";
						}
					}

					// �������
					ss << Utils::c2w(GetZombieClassName(entity));

					float height = fabs(head.y - foot.y);
					float width = height * 0.65f;

					// �������������ɫ
					if ((classId == ET_SURVIVORBOT || classId == ET_CTERRORPLAYER) &&
						entity->GetNetProp<byte>("m_bIsOnThirdStrike", "DT_TerrorPlayer") != 0)
					{
						// �����ߺڰ�ʱʹ�ð�ɫ
						g_cInterfaces.Surface->drawString(foot.x - width / 2, head.y, 255, 255, 255, font, ss.str().c_str());
					}
					else if ((classId == ET_BOOMER || classId == ET_SMOKER || classId == ET_HUNTER ||
						classId == ET_SPITTER || classId == ET_CHARGER || classId == ET_JOCKEY) &&
						entity->GetNetProp<byte>("m_isGhost", "DT_TerrorPlayer") != 0)
					{
						// ����״̬���ظУ���ɫ
						g_cInterfaces.Surface->drawString(foot.x - width / 2, head.y, 128, 0, 128, font, ss.str().c_str());
					}
					else
					{
						// �����������ɫ
						g_cInterfaces.Surface->drawString(foot.x - width / 2, head.y, 255, 128, 0, font, ss.str().c_str());
					}

					ss.str(L"");

					// ��ʾ����
					ss << dist;

					// ����������ʾ��ҩ
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
									ss << " (" << clip << " / " <<
										entity->GetNetProp<int>("m_iAmmo", "DT_TerrorPlayer", (size_t)ammoType) <<
										")";
								}
							}
						}
					}

					// ����Ƿ���Կ���
					if (visible)
					{
						// ���ü�����ʾ��ɫ
						g_cInterfaces.Surface->drawString(foot.x - width / 2, head.y + FontSize, 0, 255, 255, font, ss.str().c_str());
					}
					else
					{
						// ����������ʾ��ɫ
						g_cInterfaces.Surface->drawString(foot.x - width / 2, head.y + FontSize, 255, 255, 0, font, ss.str().c_str());
					}

					// ����һ������Ȼ�����ֻ�����������ߣ�
					g_cInterfaces.Surface->drawBox(foot.x - width / 2, foot.y, width, -height,
						1, 255, 0, 0, 255);
				}
				else
				{
					// ��ֻ���ոж��ѣ�̫Զ��û��Ҫ��ʾ����
					if (dist > 2500.0f)
						continue;

					// ��һ��С���Σ��Ա��Ϊͷ��
					if (classId == ET_WITCH)
					{
						if (visible)
							g_cInterfaces.Surface->FillRGBA(head.x, head.y, 4, 4, 128, 0, 255, 255);
						else
							g_cInterfaces.Surface->FillRGBA(head.x, head.y, 2, 2, 128, 0, 255, 255);
					}
					else
					{
						if (visible)
							g_cInterfaces.Surface->FillRGBA(head.x, head.y, 4, 4, 255, 128, 0, 255);
						else
							g_cInterfaces.Surface->FillRGBA(head.x, head.y, 2, 2, 255, 128, 0, 255);
					}
				}
			}

			if (g_bAimBot && (!targetSelected || !(g_pUserCommands->buttons & IN_ATTACK)) &&
				classId != ET_WITCH && (classId != ET_INFECTED || team == 2))
			{
				// �Ѿ�ѡ���Ŀ���ˣ���������һ������Ҫ�ĵ���
				if (classId == ET_INFECTED && distmin < 65535.0f)
					continue;

				// ѡ��һ����ӽ����ظУ���Ϊ�ظ�Խ���������˵ԽΣ��
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
				Utils::log("%s (%d): δ֪�쳣 | 0x%X", __FILE__, __LINE__, (DWORD)target);
				target = nullptr;
			}
#endif

			if (target != nullptr)
			{
				int classId = target->GetClientClass()->m_ClassID;
				if (target->GetTeam() == local->GetTeam())
					g_cInterfaces.Surface->drawCrosshair(width / 2, height / 2, 0, 0, 255);
				else if (classId == ET_INFECTED)
					g_cInterfaces.Surface->drawCrosshair(width / 2, height / 2, 255, 128, 0);
				else if (classId == ET_WITCH)
					g_cInterfaces.Surface->drawCrosshair(width / 2, height / 2, 128, 0, 255);
				else
					g_cInterfaces.Surface->drawCrosshair(width / 2, height / 2, 255, 0, 0);
			}
			else
				g_cInterfaces.Surface->drawCrosshair(width / 2, height / 2, 0, 255, 0);
		}
#endif
	}

#ifndef USE_D3D_DRAW
finish_draw:
#endif

	// ((FnPaintTraverse)g_cInterfaces.PanelHook->GetOriginalFunction(indexes::PaintTraverse))(ecx, panel, forcePaint, allowForce);
	oPaintTraverse(pPanel, panel, forcePaint, allowForce);
}

void __stdcall Hooked_CreateMove(int sequence_number, float input_sample_frametime, bool active)
{
	static bool showHint = true;
	if (showHint)
	{
		Utils::log("Hooked_CreateMove trigged.");
	}

	DWORD dwEBP = NULL;
	__asm mov dwEBP, ebp;
	bool* bSendPacket = (bool*)(*(byte**)dwEBP - 0x21);

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
	int weaponId = (weapon != nullptr ? weapon->GetWeaponID() : 0);
	int myTeam = client->GetTeam();

	// �Զ�����
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
			if (client->GetFlags() & FL_ONGROUND)
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
		if (g_bAutoStrafe && !(client->GetFlags() & FL_ONGROUND))
		{
			if (pCmd->mousedx < 0)
				pCmd->sidemove = -400.f;

			if (pCmd->mousedx > 0)
				pCmd->sidemove = 400.f;
		}
	}

	// ���������޸�
	// StartEnginePrediction(client, pCmd);

	// �Զ���׼
	if (g_bAimBot && weapon != nullptr && (GetAsyncKeyState(VK_LBUTTON) & 0x8000))
	{
		Vector myOrigin = client->GetEyePosition(), myAngles = pCmd->viewangles;

		// �Զ���׼���ݱ���
		static QAngle oldViewAngles;
		static float oldSidemove;
		static float oldForwardmove;
		static float oldUpmove;

		bool runAimbot = false;

		// Ŀ������һ���ط�ѡ��
		if (g_pCurrentAiming != nullptr && IsGunWeapon(weaponId) && nextAttack <= serverTime)
		{
			// 目标位置
			Vector position;
			try
			{
				position = GetHeadHitboxPosition(g_pCurrentAiming);
			}
			catch (...)
			{
				if (g_pCurrentAiming->GetClientClass()->m_ClassID == ET_INFECTED)
					goto end_aimbot;

				// 获取骨骼位置失败
				position = g_pCurrentAiming->GetEyePosition();
				Utils::log("CBasePlayer::SetupBone error");

				// 根据不同的情况确定高�
				int zombieClass = g_pCurrentAiming->GetNetProp<int>("m_zombieClass", "DT_TerrorPlayer");
				if (zombieClass == ZC_JOCKEY)
					position.z = g_pCurrentAiming->GetAbsOrigin().z + 30.0f;
				else if (zombieClass == ZC_HUNTER && (g_pCurrentAiming->GetFlags() & FL_DUCKING))
					position.z -= 12.0f;
			}

			// �ٶ�Ԥ�⣬���û����
			// position += (g_pCurrentAiming->GetVelocity() * g_cInterfaces.Globals->interval_per_tick);

			if (position.IsValid())
			{
				// ����ԭ����
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
				// ��ԭ����
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

	// �Զ���ǹ
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
			Utils::log("%s (%d): δ֪�쳣 | 0x%X", __FILE__, __LINE__, (DWORD)target);
			goto end_trigger_bot;
		}
#endif

		if (target->GetTeam() != myTeam && target->GetClientClass()->m_ClassID != ET_WITCH &&
			(myTeam == 2 || target->GetClientClass()->m_ClassID != ET_INFECTED))
			pCmd->buttons |= IN_ATTACK;
	}

end_trigger_bot:

	// ��ǹ����
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
			// ����һ����ֹ��ǹ
			ignoreButton = false;
		}
	}

	// �ڿ�ǹǰ��飬��ֹ��������
	if ((pCmd->buttons & IN_ATTACK) && IsGunWeapon(weaponId) && myTeam == 2)
	{
		int cid = client->GetCrosshairsId();
		CBaseEntity* aiming = (cid > 0 ? g_cInterfaces.ClientEntList->GetClientEntity(cid) : nullptr);

		if (aiming != nullptr && !aiming->IsDormant() && aiming->IsAlive() &&
			aiming->GetTeam() == myTeam && !IsNeedRescue(aiming) && !IsFallDown(client))
		{
			// ȡ����ǹ
			pCmd->buttons &= ~IN_ATTACK;
		}
	}

	// ��������޸�
	// EndEnginePrediction(client);

	// ��ս�������ٹ���
	if ((pCmd->buttons & IN_ZOOM) && weaponId == Weapon_Melee && myTeam == 2)
	{
		if (nextAttack <= serverTime)
		{
			pCmd->buttons |= IN_ATTACK;
			g_cInterfaces.Engine->ClientCmd("wait 5; slot1; wait 5; slot2");
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

	if (stage == FRAME_RENDER_START && g_cInterfaces.Engine->IsInGame())
	{
		if (client != nullptr && client->IsAlive())
		{
			punch = client->GetNetProp<Vector>("m_vecPunchAngle", "DT_BasePlayer");
			velocity = client->GetNetProp<Vector>("m_vecPunchAngleVel", "DT_BasePlayer");
		}

		// ���������ʹ�� DebugOverlay ������ 3D ����
	}

	oFrameStageNotify(stage);

	if (client != nullptr && client->IsAlive() && punch.IsValid() && velocity.IsValid())
	{
		// ȥ����Ļ�ζ�
		client->SetNetProp<Vector>("m_vecPunchAngle", punch, "DT_BasePlayer");
		client->SetNetProp<Vector>("m_vecPunchAngleVel", velocity, "DT_BasePlayer");
	}

	static time_t nextUpdate = 0;
	time_t currentTime = time(NULL);
	if (nextUpdate <= currentTime)
	{
		// ��ʱ������ÿ�� 1 �봥��һ��
		nextUpdate = currentTime + 1;
		static DWORD fmWait = 45;
		static bool connected = false;

		if (g_cInterfaces.Engine->IsConnected())
		{
			// �޸� r_drawothermodels ʵ�ּ򵥵� esp
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
					if (Utils::readMemory<int>(g_iClientModules + r_drawothermodels) == 2)
					{
						Utils::writeMemory(1, g_iClientModules + r_drawothermodels);
						Utils::writeMemory(0, g_iClientModules + cl_drawshadowtexture);
					}
					else
					{
						Utils::writeMemory(2, g_iClientModules + r_drawothermodels);
						Utils::writeMemory(1, g_iClientModules + cl_drawshadowtexture);
					}

					g_cInterfaces.Engine->ClientCmd("echo \"r_drawothermodels set %d\"",
						Utils::readMemory<int>(g_iClientModules + r_drawothermodels));
					g_cInterfaces.Engine->ClientCmd("echo \"cl_drawshadowtexture set %d\"",
						Utils::readMemory<int>(g_iClientModules + cl_drawshadowtexture));
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

			// �޸� mat_fullbright ʵ��ȫͼ����
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

			// �޸� mp_gamemode �ڶԿ�ģʽ���������˳�
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
					char* mode = Utils::readMemory<char*>(g_iClientModules + mp_gamemode);
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

			// �޸� sv_cheats ������� cvar ����
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

				if (Utils::readMemory<int>(g_iEngineModules + sv_cheats) != 1)
				{
					Utils::writeMemory(1, g_iEngineModules + sv_cheats);
					g_cInterfaces.Engine->ClientCmd("echo \"sv_cheats set %d\"",
						Utils::readMemory<int>(g_iEngineModules + sv_cheats));
				}
			}

			// �л������˳�
			if (GetAsyncKeyState(VK_NEXT) & 0x01)
				thirdPerson();

			// ��ʾȫ�����
			/*
			if (GetAsyncKeyState(VK_CAPITAL) & 0x01)
			showSpectator();
			*/

			// ��/�ر� �Զ��������Զ������ٶ�
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

			// ��/�ر� �Զ���ǹ
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

			// ��/�ر� �Զ���׼
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

			// ��/�ر� �ո��Զ�����
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

			// ��/�ر� ������׼
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

			// ȥ�� CRC ��֤
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

				if (Utils::readMemory<int>(g_iEngineModules + sv_pure) != 0 ||
					Utils::readMemory<int>(g_iEngineModules + sv_consistency) != 0)
				{
					Utils::writeMemory(0, g_iEngineModules + sv_pure);
					Utils::writeMemory(0, g_iEngineModules + sv_consistency);

					g_cInterfaces.Engine->ClientCmd("echo \"sv_pure and sv_consistency set %d\"",
						Utils::readMemory<int>(g_iEngineModules + sv_pure));
				}
			}

			if (!connected)
			{
				g_cInterfaces.Engine->ClientCmd("echo \"********* connected *********\"");

				Utils::log("*** connected ***");
			}

			connected = true;
		}
		else if (connected && !g_cInterfaces.Engine->IsInGame())
		{
			connected = false;
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
			g_cInterfaces.Engine->ClientCmd("disconnect");
	}
}

void __stdcall Hooked_RunCommand(CBaseEntity* pEntity, CUserCmd* pCmd, CMoveHelper* moveHelper)
{
	oRunCommand(pEntity, pCmd, moveHelper);

	if (g_cInterfaces.MoveHelper == nullptr)
		Utils::log("MoveHelperPointer = 0x%X", (DWORD)moveHelper);

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
