#pragma once
#include <memory>

class CClientEntityList;
class CTrace;
class ICvar;
class CDebugOverlay;
class IGameEventManager2;
class CModelRender;
class CRenderView;
class CMoveHelper;

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
	CGameMovement* GameMovement;
	CDebugOverlay* DebugOverlay;
	IGameEventManager2* GameEvent;
	IInputInternal* InputSystem;
	CModelRender* ModelRender;
	CRenderView* RenderView;
	CMoveHelper* MoveHelper;
	ICvar* Cvar;

	ClientModeShared* ClientMode;
	CVMTHookManager* ClientModeHook;
	CVMTHookManager* PanelHook;
	CVMTHookManager* ClientHook;
	CVMTHookManager* PredictionHook;
	CVMTHookManager* ModelRenderHook;
	CVMTHookManager* GameEventHook;

	void GetInterfaces()
	{
		Engine = (CEngine*)GetPointer("engine.dll", "VEngineClient");
		Trace = (CTrace*)GetPointer("engine.dll", "EngineTraceClient");
		Client = (CClient*)GetPointer("client.dll", "VClient");
		ClientEntList = (CClientEntityList*)GetPointer("client.dll", "VClientEntityList");
		ModelInfo = (CModelInfo*)GetPointer("engine.dll", "VModelInfoClient");
		Panel = (CPanel*)GetPointer("vgui2.dll", "VGUI_Panel");
		Surface = (CSurface*)GetPointer("vguimatsurface.dll", "VGUI_Surface");
		PlayerInfo = (CPlayerInfoManager*)GetPointer("server.dll", "PlayerInfoManager");
		Prediction = (CPrediction*)GetPointer("client.dll", "VClientPrediction");
		GameMovement = (CGameMovement*)GetPointer("client.dll", "GameMovement");
		DebugOverlay = (CDebugOverlay*)GetPointer("engine.dll", "VDebugOverlay");
		GameEvent = (IGameEventManager2*)GetPointer("engine.dll", "GAMEEVENTSMANAGER002");
		ModelRender = (CModelRender*)GetPointer("engine.dll", "VEngineModel");
		RenderView = (CRenderView*)GetPointer("engine.dll", "VEngineRenderView");
		InputSystem = (IInputInternal*)GetPointer("inputsystem.dll", "VGUI_InputInternal");
		Cvar = (ICvar*)GetPointer("vstdlib.dll", "VEngineCvar");
		
		PDWORD pdwClient = *(PDWORD*)Client;
		// Input = *(CInput**)((*(DWORD**)Client)[15] + 0x1);

		Input = (CInput*)*(PDWORD**)*(PDWORD**)(pdwClient[indexes::CreateMove] + 0x28);

		DWORD dwInitAddr = (DWORD)(pdwClient[0]);
		for (DWORD dwIter = 0; dwIter <= 0xFF; dwIter++)
		{
			if (*(PBYTE)(dwInitAddr + dwIter) == 0xA3)
			{
				Globals = **(CGlobalVarsBase***)(dwInitAddr + dwIter + 1);
				// Utils::log("找到了全局变量 0x%X", (DWORD)Globals);
				break;
			}
		}
		
		// Utils::log("找到了 Input 指针 0x%X", (DWORD)Input);

		PanelHook = new CVMTHookManager(Panel);
		ClientHook = new CVMTHookManager(Client);
		PredictionHook = new CVMTHookManager(Prediction);
		ModelRenderHook = new CVMTHookManager(ModelRender);
		GameEventHook = new CVMTHookManager(GameEvent);
	}

	void* GetPointer(const char* Module, const char* InterfaceName)
	{
		void* Interface = NULL;
		char PossibleInterfaceName[1024];

		CreateInterfaceFn CreateInterface = (CreateInterfaceFn)GetProcAddress(GetModuleHandleA(Module), "CreateInterface");
		Interface = (void*)CreateInterface(InterfaceName, NULL);
		if (Interface != NULL)
		{
			printf("%s Found: 0x%X\n", InterfaceName, (DWORD)Interface);
			return Interface;
		}
		
		for (int i = 1; i < 100; i++)
		{
			sprintf_s(PossibleInterfaceName, "%s0%i", InterfaceName, i);
			Interface = (void*)CreateInterface(PossibleInterfaceName, NULL);
			if (Interface != NULL)
			{
				// Utils::log("找到了 %s 的指针为 0x%X", PossibleInterfaceName, (DWORD)Interface);
				break;
			}
			sprintf_s(PossibleInterfaceName, "%s00%i", InterfaceName, i);
			Interface = (void*)CreateInterface(PossibleInterfaceName, NULL);
			if (Interface != NULL)
			{
				// Utils::log("找到了 %s 的指针为 0x%X", PossibleInterfaceName, (DWORD)Interface);
				break;
			}
		}

		return Interface;
	}

	template<typename Fn>
	inline Fn* GetFactoryPointer(const std::string& module, const std::string& interfaces)
	{
		return (Fn*)GetPointer(module.c_str(), interfaces.c_str());
	}
};
