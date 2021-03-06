#pragma once
#pragma warning(disable : 4800)
#pragma warning(disable : 4244)
#pragma warning(disable : 4996)

#include <Windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <d3d9types.h>
#include "d3dumddi.h"
#include <dwmapi.h>

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <ctime>
#include <fstream>
#include <algorithm>
#include <thread>
#include <chrono>
#include <memory>
#include <map>

#include <./dx9/imgui_impl_dx9.h>
#pragma comment(lib, "d3d9")
#pragma comment(lib, "d3dx9")
#pragma comment(lib, "dwmapi")

#ifdef _DEBUG
#include <DbgHelp.h>
#pragma comment(lib, "dbghelp")
#endif

class Utils
{
public:
	static std::string g2u(const std::string& strGBK);
	static std::string u2g(const std::string& strUTF8);
	static char* utg(const char* utf8);
	static char* gtu(const char* gb2312);
	static std::string w2c(const std::wstring& ws);
	static std::wstring c2w(const std::string& s);
	static DWORD FindPattern(const std::string& szModules, const std::string& szPattern);
	static DWORD FindPattern(const std::string& szModules, const std::string& szPattern, std::string szMask);
	static HMODULE GetModuleHandleSafe(const std::string& pszModuleName);
	static DWORD GetModuleBase(const std::string& ModuleName, DWORD ProcessID = 0);
	static DWORD GetModuleBase(const std::string& ModuleName, DWORD* ModuleSize, DWORD ProcessID = 0);
	static DWORD FindProccess(const std::string& proccessName);
	template<typename T, size_t len> static inline int GetArrayLength(const T(&arr)[len]);
	template<typename T, typename ...Arg> static T readMemory(Arg... offset);
	template<typename T, typename ...Arg> static T writeMemory(T value, Arg... offset);
	static std::vector<std::string> split(const std::string& s, const std::string& delim = " ");
	static std::string trim(const std::string& s, const std::string& delim = " \r\n\t");
	static std::string GetPath();
	static void log(const char* text, ...);
	static void log(const wchar_t* text, ...);
};

struct RecvProp;
struct RecvTable;
#undef GetProp
class CNetVars
{
public:
	CNetVars();
	int GetOffset(const char* tableName, const char* propName);
private:
	int GetProp(const char* tableName, const char* propName, RecvProp **prop = 0);
	int GetProp(RecvTable *recvTable, const char *propName, RecvProp **prop = 0);
	RecvTable *GetTable(const char *tableName);
	std::vector<RecvTable*> _tables;
};

// 实体 NetProp 表
extern std::unique_ptr<CNetVars> g_pNetVars;

class CEngine;
class CClient;
class CInput;
class CClientEntityList;
class CModelInfo;
class CPanel;
class CSurface;
class CTrace;
class CGlobalVarsBase;
class CPlayerInfoManager;
class CPrediction;
class IGameMovement;
class CDebugOverlay;
class IGameEventManager2;
class IInputInternal;
class CModelRender;
class CRenderView;
class CMoveHelper;
class CUserMessages;
class ICvar;
class ClientModeShared;
class CVMTHookManager;
class IMaterialSystem;

class CInterfaces
{
public:
	CEngine *Engine;
	CClient *Client;
	CInput *Input;
	CClientEntityList *ClientEntList;
	CModelInfo *ModelInfo;
	CPanel* Panel;
	CSurface* Surface;
	CTrace* Trace;
	CGlobalVarsBase* Globals;

	CPlayerInfoManager* PlayerInfo;
	CPrediction* Prediction;
	IGameMovement* GameMovement;
	CDebugOverlay* DebugOverlay;
	IGameEventManager2* GameEvent;
	IInputInternal* InputSystem;
	CModelRender* ModelRender;
	CRenderView* RenderView;
	CMoveHelper* MoveHelper;
	CUserMessages* UserMessage;
	IMaterialSystem* MaterialSystem;
	ICvar* Cvar;

	ClientModeShared* ClientMode;
	std::unique_ptr<CVMTHookManager> ClientModeHook;
	std::unique_ptr<CVMTHookManager> PanelHook;
	std::unique_ptr<CVMTHookManager> ClientHook;
	std::unique_ptr<CVMTHookManager> PredictionHook;
	std::unique_ptr<CVMTHookManager> ModelRenderHook;
	std::unique_ptr<CVMTHookManager> GameEventHook;
	std::unique_ptr<CVMTHookManager> ViewRenderHook;

	void GetInterfaces();
	void* GetPointer(const char* Module, const char* InterfaceName);
	template<typename Fn>
	inline Fn* GetFactoryPointer(const std::string& module, const std::string& interfaces);
};

// 各类接口
extern CInterfaces g_interface;

namespace Config
{
	bool bDrawBox = true;
	bool bDrawBone = true;
	bool bDrawName = true;
	bool bDrawDist = true;
	bool bDrawAmmo = true;
	bool bDrawCrosshairs = true;

	bool bBufferSurvivor = false;
	bool bBufferSpecial = false;
	bool bBufferCommon = false;
	bool bBufferWeapon = false;
	bool bBufferGrenade = true;
	bool bBufferMedickit = true;
	bool bBufferCarry = false;

	bool bAimbot = false;
	bool bSilentAimbot = true;
	bool bTriggerBot = false;
	bool bTriggerBotHead = false;
	bool bAnitFirendlyFire = true;

	bool bBunnyHop = true;
	bool bAutoStrafe = false;
	bool bNoRecoil = true;
	bool bRapidFire = true;
	bool bCrcCheckBypass = true;
	bool bCvarFullBright = false;
	bool bCvarWireframe = false;
	bool bCvarGameMode = false;
	bool bCvarCheats = false;
	bool bThirdPersons = false;

	float fAimbotFov = 30.0f;
};

#include "./definitions.h"
#include "./indexes.h"
#include "./libraries/xorstr.h"

#include "./libraries/vmt.h"
#include "./detours/detourxs.h"
#include "./structs/keyvalues.h"
#include "./structs/handle.h"
#include "./structs/vector.h"
#include "./libraries/buffer.h"
#include "./structs/vmatrix.h"
#include "./structs/usercmd.h"
#include "./structs/playerinfo.h"
#include "./structs/engine.h"
#include "./structs/globals.h"
#include "./structs/input.h"
#include "./structs/panel.h"
#include "./structs/surface.h"
#include "./structs/client.h"
#include "./structs/crc32.h"
#include "./structs/quaternion.h"
#include "./structs/modelinfo.h"
#include "./structs/checksum_md5.h"
#include "./structs/messages.h"
#include "./structs/sound.h"
#include "./structs/clientstate.h"
#include "./libraries/interfaces.h"
#include "./Utils.h"
#include "./structs/weapon.h"
#include "./structs/baseentity.h"
#include "./structs/cliententlist.h"
#include "./structs/trace.h"
#include "./structs/debugoverlay.h"
#include "./structs/console.h"
#include "./structs/event.h"
#include "./structs/render.h"
#include "./libraries/math.h"
