#pragma once
#include <memory>

void CInterfaces::GetInterfaces()
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
	GameMovement = (IGameMovement*)GetPointer("client.dll", "GameMovement");
	DebugOverlay = (CDebugOverlay*)GetPointer("engine.dll", "VDebugOverlay");
	GameEvent = (IGameEventManager2*)GetPointer("engine.dll", "GAMEEVENTSMANAGER002");
	ModelRender = (CModelRender*)GetPointer("engine.dll", "VEngineModel");
	RenderView = (CRenderView*)GetPointer("engine.dll", "VEngineRenderView");
	InputSystem = (IInputInternal*)GetPointer("inputsystem.dll", "VGUI_InputInternal");
	MaterialSystem = (IMaterialSystem*)GetPointer("materialsystem.dll", "VMaterialSystem");
	Cvar = (ICvar*)GetPointer("vstdlib.dll", "VEngineCvar");

	PDWORD pdwClient = *(PDWORD*)Client;
	// Input = *(CInput**)((*(DWORD**)Client)[15] + 0x1);

	Input = (CInput*)*(PDWORD**)*(PDWORD**)(pdwClient[indexes::CreateMove] + 0x28);
	UserMessage = (CUserMessages*)*(PDWORD**)*(PDWORD**)(pdwClient[indexes::DispatchUserMessage] + 0x5);
	MoveHelper = (CMoveHelper*)((*(DWORD**)GameMovement)[indexes::PlayerMove] + 0x4E);

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

	PanelHook = std::make_unique<CVMTHookManager>(Panel);
	ClientHook = std::make_unique<CVMTHookManager>(Client);
	PredictionHook = std::make_unique<CVMTHookManager>(Prediction);
	ModelRenderHook = std::make_unique<CVMTHookManager>(ModelRender);
	GameEventHook = std::make_unique<CVMTHookManager>(GameEvent);
	ViewRenderHook = std::make_unique<CVMTHookManager>(RenderView);
}

void* CInterfaces::GetPointer(const char* Module, const char* InterfaceName)
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
inline Fn* CInterfaces::GetFactoryPointer(const std::string& module, const std::string& interfaces)
{
	return (Fn*)GetPointer(module.c_str(), interfaces.c_str());
}
