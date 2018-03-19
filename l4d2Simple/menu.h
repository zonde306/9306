﻿#pragma once
#include <memory>
#include <vector>
#include <string>

class CBaseMenu
{
public:
	CBaseMenu();
	void Init();

	void DrawMenu();

public:
	struct SelectItem
	{
	public:
		SelectItem();
		SelectItem(const std::string& text, bool selected = false);

		std::string text;
		bool selected;
	};

	bool AddAntiQuery(const char* text, ...);
	bool AddAntiSetting(const char* text, ...);
	bool AddAntiExecute(const char* text, ...);

protected:
	void AddListSelect(const std::string& name, std::vector<SelectItem>& list);

public:
	std::vector<SelectItem> m_vConVarQuery;
	std::vector<SelectItem> m_vConVarSetting;
	std::vector<SelectItem> m_vCommandExecute;
	bool m_bStateUpdated;

	using FnOnEndMenu = void(*)(bool);
	FnOnEndMenu m_pfnOnMenuEnd;
};

namespace Config
{
	// 屏幕绘制：玩家
	extern bool bDrawBox;
	extern bool bDrawBone;
	extern bool bDrawName;
	extern bool bDrawDist;
	extern bool bDrawAmmo;
	extern bool bDrawCrosshairs;
	extern bool bDrawSpectator;
	extern bool bDrawOffScreen;
	extern bool bDrawCarryItem;
	extern bool bDrawDeadSurvivor;
	extern bool bDrawChamsSurvivor;
	extern bool bDrawChamsSpecial;
	extern bool bDrawChamsInfected;
	extern bool bDrawHealth;

	// 屏幕绘制：实体
	extern bool bDrawT1Weapon;
	extern bool bDrawT2Weapon;
	extern bool bDrawT3Weapon;
	extern bool bDrawMeleeWeapon;
	extern bool bDrawMedicalItem;
	extern bool bDrawGrenadeItem;
	extern bool bDrawAmmoStack;
	extern bool bDrawBulletTrace;

	// Direct3D 顶点透视
	extern bool bBufferSurvivor;
	extern bool bBufferSpecial;
	extern bool bBufferCommon;
	extern bool bBufferWeapon;
	extern bool bBufferGrenade;
	extern bool bBufferMedical;
	extern bool bBufferCarry;

	// 瞄准辅助
	extern bool bNoSpread;
	extern bool bAimbot;
	extern bool bAimbotKey;
	extern bool bAimbotPred;
	extern bool bAimbotRCS;
	extern bool bSilentAimbot;
	extern bool bTriggerBot;
	extern bool bTriggerBotHead;
	extern bool bAnitFirendlyFire;
	extern bool bForwardTrack;
	extern bool bBackTrack;
	extern bool bAntiAim;
	extern bool bKnifeBot;
	extern bool bAntiAimFix;

	// 自动操作
	extern bool bBunnyHop;
	extern bool bAutoStrafe;
	extern bool bNoRecoil;
	extern bool bRapidFire;

	// 杂项
	extern bool bCrcCheckBypass;
	extern bool bCvarFullBright;
	extern bool bCvarWireframe;
	extern bool bCvarGameMode;
	extern bool bCvarCheats;
	extern bool bThirdPersons;
	extern bool bSpeedHackActive;
	extern bool bSpeedHack;
	extern bool bRemoveFog;
	extern bool bTriggerExtra;
	extern bool bTeleportExploit;
	extern bool bSpecialSpawnnedHint;
	extern bool bClientConnectHint;
	extern bool bClientDisconnectHint;
	extern bool bAllowConsoleMessage;
	extern bool bNoVomitEffects;
	extern bool bNoSmokerEffects;
	extern bool bNoMudEffects;
	extern bool bNoBloodEffects;
	extern bool bNoAdrenalineEffects;
	extern bool bNoBurnEffects;

	extern float fAimbotFov;
	extern float fAimbotRCSX;
	extern float fAimbotRCSY;
	extern float fSpeedHackSpeed;
	extern int iDuckAimbotTick;

	extern int iFastMeleeTick;
	extern int iAutoShovTick;
	extern bool bMustFastMelee;
	extern float fBulletTraceDuration;

	// 不稳定的功能
	extern bool bTeleport;
	extern bool bAirStuck;
	extern bool bPositionAdjustment;
	extern bool bCrashServer;

	// 加速
	extern float fSpeedUse;
	extern float fSpeedShift;
	extern float fSpeedCapsLock;
};

extern std::unique_ptr<CBaseMenu> g_pBaseMenu;
