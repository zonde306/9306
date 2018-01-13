#include "menu.h"
#include "libraries/xorstr.h"
#include <imgui.h>
#include <vector>
#include <cstring>
#include <ctime>

std::unique_ptr<CBaseMenu> g_pBaseMenu;
extern bool g_bIsShowMenu;
extern std::vector<std::string> g_vsBannedQueryConVar;
extern std::vector<std::string> g_vsBannedSettingConVar;
extern std::vector<std::string> g_vsBannedExecuteCommand;

CBaseMenu::CBaseMenu() : m_bStateUpdated(false), m_pfnOnMenuEnd(nullptr)
{
	// Init();
}

void CBaseMenu::Init()
{
	m_vConVarQuery.clear();
	m_vConVarSetting.clear();
	m_vCommandExecute.clear();

	for (const std::string& value : g_vsBannedQueryConVar)
		m_vConVarQuery.emplace_back(value);

	for (const std::string& value : g_vsBannedSettingConVar)
		m_vConVarSetting.emplace_back(value);

	for (const std::string& value : g_vsBannedExecuteCommand)
		m_vCommandExecute.emplace_back(value);

	ImGui::StyleColorsDark();
}

void CBaseMenu::DrawMenu()
{
	if (!ImGui::Begin(XorStr("l4d2Simple Main Menu"), &g_bIsShowMenu))
	{
		ImGui::End();

		if (m_pfnOnMenuEnd)
			m_pfnOnMenuEnd(g_bIsShowMenu);

		return;
	}

	ImGui::Text(XorStr("Version: 1.0 | Created by zonde306"));
	ImGui::Text(XorStr(u8"此辅助免费且开源，如果你是通过购买获得，说明你被骗了。"));
	// ImGui::GetIO().MouseDrawCursor = true;

	// 显示系统时间
	{
		ImGui::Separator();

		tm timeInfo;
		time_t t = time(nullptr);
		localtime_s(&timeInfo, &t);

		/*
		int week = (timeInfo.tm_mday + 2 * (timeInfo.tm_mon + 1) + 3 *
			((timeInfo.tm_mon + 1) + 1) / 5 + (timeInfo.tm_year + 1900) +
			(timeInfo.tm_year + 1900) / 4 - (timeInfo.tm_year + 1900) / 100 +
			(timeInfo.tm_year + 1900) / 400) % 7;
		*/

		std::string weak;
		switch (timeInfo.tm_wday)
		{
		case 0:
		case 7:
			weak = XorStr(u8"星期日");
			break;
		case 1:
			weak = XorStr(u8"星期一");
			break;
		case 2:
			weak = XorStr(u8"星期二");
			break;
		case 3:
			weak = XorStr(u8"星期三");
			break;
		case 4:
			weak = XorStr(u8"星期四");
			break;
		case 5:
			weak = XorStr(u8"星期五");
			break;
		case 6:
			weak = XorStr(u8"星期六");
			break;
		}

		ImGui::Text("%4d/%2d/%2d %2d:%2d:%2d %s",
			timeInfo.tm_year + 1900, timeInfo.tm_mon + 1, timeInfo.tm_mday,
			timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec, weak.c_str());

		ImGui::Separator();
	}

	if (ImGui::TreeNode(XorStr("Aimbot")))
	{
		ImGui::Checkbox(XorStr("Auto Aim"), &Config::bAimbot);
		ImGui::Checkbox(XorStr("Only Firing"), &Config::bAimbotKey);
		ImGui::Checkbox(XorStr("Speed Prediction"), &Config::bAimbotPred);
		ImGui::Checkbox(XorStr("Silent Aim"), &Config::bSilentAimbot);
		// ImGui::DragFloat(XorStr("Aimbot FOV"), &Config::fAimbotFov, 1.0f, 1.0f, 360.0f);
		ImGui::SliderFloat(XorStr("Aimbot FOV"), &Config::fAimbotFov, 1.0f, 360.0f);

		ImGui::Separator();
		ImGui::Checkbox(XorStr("Recoil Control System"), &Config::bAimbotRCS);
		// ImGui::DragFloat(XorStr("RCS X"), &Config::fAimbotRCSX, 1.0f, 0.0f, 128.0f);
		// ImGui::DragFloat(XorStr("RCS Y"), &Config::fAimbotRCSX, 1.0f, 0.0f, 128.0f);
		ImGui::SliderFloat(XorStr("RCS X"), &Config::fAimbotRCSX, 0.0f, 128.0f);
		ImGui::SliderFloat(XorStr("RCS Y"), &Config::fAimbotRCSY, 0.0f, 128.0f);

		ImGui::Separator();
		ImGui::Checkbox(XorStr("Knife Bot"), &Config::bKnifeBot);
		ImGui::SliderInt(XorStr("Knife Bot Tick"), &Config::iAutoShovTick, 0, 250);
		ImGui::Checkbox(XorStr("Trigger Bot"), &Config::bTriggerBot);
		ImGui::Checkbox(XorStr("Trigger Only Head"), &Config::bTriggerBotHead);
		ImGui::Checkbox(XorStr("Trigger Heads"), &Config::bTriggerExtra);
		// ImGui::DragInt(XorStr("Trigger Heads Tick"), &Config::iDuckAimbotTick, 1.0f, 0, 250);
		ImGui::SliderInt(XorStr("Trigger Heads Tick"), &Config::iDuckAimbotTick, 0, 250);

		ImGui::Separator();
		ImGui::Checkbox(XorStr("No Spread"), &Config::bNoSpread);
		ImGui::Checkbox(XorStr("No Recoil"), &Config::bNoRecoil);
		ImGui::Checkbox(XorStr("Rapid Fire"), &Config::bRapidFire);
		ImGui::Checkbox(XorStr("Dont Fire Team"), &Config::bAnitFirendlyFire);
		ImGui::Checkbox(XorStr("Anti Aim"), &Config::bAntiAim);

		ImGui::TreePop();
	}

	if (ImGui::TreeNode(XorStr("Visual")))
	{
		ImGui::Checkbox(XorStr("Player Box"), &Config::bDrawBox);
		ImGui::Checkbox(XorStr("Player Name"), &Config::bDrawName);
		ImGui::Checkbox(XorStr("Player/Infected Bone"), &Config::bDrawBone);
		ImGui::Checkbox(XorStr("Player Distance"), &Config::bDrawDist);
		ImGui::Checkbox(XorStr("Survivor Ammo"), &Config::bDrawAmmo);
		ImGui::Checkbox(XorStr("Off Screen ESP"), &Config::bDrawOffScreen);
		ImGui::Checkbox(XorStr("Crosshairs"), &Config::bDrawCrosshairs);
		ImGui::Checkbox(XorStr("Spectator List"), &Config::bDrawSpectator);

		ImGui::Separator();
		ImGui::Checkbox(XorStr("Special Spawnned"), &Config::bSpecialSpawnnedHint);
		ImGui::Checkbox(XorStr("Client Connected"), &Config::bClientConnectHint);
		ImGui::Checkbox(XorStr("Client Disconnected"), &Config::bClientDisconnectHint);

		ImGui::TreePop();
	}

	if (ImGui::TreeNode(XorStr("Weapon/Items")))
	{
		ImGui::Checkbox(XorStr("T1 Weapon"), &Config::bDrawT1Weapon);
		ImGui::Checkbox(XorStr("T2 Weapon"), &Config::bDrawT2Weapon);
		ImGui::Checkbox(XorStr("T3 Weapon"), &Config::bDrawT3Weapon);
		ImGui::Checkbox(XorStr("Melee Weapon"), &Config::bDrawMeleeWeapon);
		ImGui::Checkbox(XorStr("Medical Items"), &Config::bDrawMedicalItem);
		ImGui::Checkbox(XorStr("Grenade"), &Config::bDrawGrenadeItem);
		ImGui::Checkbox(XorStr("Ammo Stack"), &Config::bDrawAmmoStack);
		ImGui::Checkbox(XorStr("Carry Items"), &Config::bDrawCarryItem);
		ImGui::TreePop();
	}

	if (ImGui::TreeNode(XorStr("DirectX ZBuffer")))
	{
		ImGui::Checkbox(XorStr("Survivors"), &Config::bBufferSurvivor);
		ImGui::Checkbox(XorStr("Special Infected"), &Config::bBufferSpecial);
		ImGui::Checkbox(XorStr("Common Infected"), &Config::bBufferCommon);
		ImGui::Checkbox(XorStr("Weapon"), &Config::bBufferWeapon);
		ImGui::Checkbox(XorStr("Grenade"), &Config::bBufferGrenade);
		ImGui::Checkbox(XorStr("Medical Items"), &Config::bBufferMedical);
		ImGui::Checkbox(XorStr("Carry Items"), &Config::bBufferCarry);
		ImGui::TreePop();
	}

	if (ImGui::TreeNode(XorStr("Anti SMAC")))
	{
		AddListSelect(XorStr("Anti ConVar Query"), m_vConVarQuery);
		AddListSelect(XorStr("Anti ConVar Change"), m_vConVarSetting);
		AddListSelect(XorStr("Anti Execute Command"), m_vCommandExecute);

		ImGui::TreePop();
	}

	if (ImGui::TreeNode(XorStr("Miscellaneous")))
	{
		ImGui::Checkbox(XorStr("Auto Bunny Hop"), &Config::bBunnyHop);
		ImGui::Checkbox(XorStr("Auto Strafe"), &Config::bAutoStrafe);
		ImGui::Checkbox(XorStr("CRC Bypass"), &Config::bCrcCheckBypass);
		ImGui::Checkbox(XorStr("Backtrack"), &Config::bBackTrack);
		ImGui::Checkbox(XorStr("Forwardtrack"), &Config::bForwardTrack);
		ImGui::Checkbox(XorStr("Thrid Persons"), &Config::bThirdPersons);

		ImGui::Separator();
		ImGui::Checkbox(XorStr("Fast Melee"), &Config::bMustFastMelee);
		// ImGui::DragInt(XorStr("Fast Melee Tick"), &Config::iFastMeleeTick);
		ImGui::SliderInt(XorStr("Fast Melee Tick"), &Config::iFastMeleeTick, 1, 100);

		ImGui::Separator();
		ImGui::Checkbox(XorStr("Remove Fog"), &Config::bRemoveFog);
		ImGui::Checkbox(XorStr("Full Bright"), &Config::bCvarFullBright);
		ImGui::Checkbox(XorStr("Wirteframe"), &Config::bCvarWireframe);
		ImGui::Checkbox(XorStr("sv_cheats 1"), &Config::bCvarCheats);
		// ImGui::Checkbox(XorStr("mp_gamemode coop"), &Config::bCvarGameMode);
		ImGui::Checkbox(XorStr("Output Debug Info"), &Config::bAllowConsoleMessage);

		ImGui::Separator();
		ImGui::Checkbox(XorStr("Teleport"), &Config::bTeleport);
		ImGui::Checkbox(XorStr("Teleport Exploit"), &Config::bTeleportExploit);
		ImGui::Checkbox(XorStr("Air Stuck"), &Config::bAirStuck);
		ImGui::Checkbox(XorStr("Position Adjustment"), &Config::bPositionAdjustment);
		ImGui::Checkbox(XorStr("Aimbot Kill Server"), &Config::bCrashServer);

		ImGui::TreePop();
	}

	ImGui::End();
	// ImGui::GetIO().MouseDrawCursor = false;
	m_bStateUpdated = true;
}

bool CBaseMenu::AddAntiQuery(const char * text, ...)
{
	va_list ap;
	va_start(ap, text);

	char buffer[255];
	vsprintf_s(buffer, text, ap);

	va_end(ap);

	if (buffer[0] == '\0')
		return false;

	for (const SelectItem& cvar : m_vConVarQuery)
	{
		if (cvar.text == buffer)
			return false;
	}

	m_vConVarQuery.emplace_back(buffer);
	return true;
}

bool CBaseMenu::AddAntiSetting(const char * text, ...)
{
	va_list ap;
	va_start(ap, text);

	char buffer[255];
	vsprintf_s(buffer, text, ap);

	va_end(ap);

	if (buffer[0] == '\0')
		return false;

	for (const SelectItem& cvar : m_vConVarSetting)
	{
		if (cvar.text == buffer)
			return false;
	}

	m_vConVarSetting.emplace_back(buffer);
	return true;
}

bool CBaseMenu::AddAntiExecute(const char * text, ...)
{
	va_list ap;
	va_start(ap, text);

	char buffer[255];
	vsprintf_s(buffer, text, ap);

	va_end(ap);

	if (buffer[0] == '\0')
		return false;

	for (const SelectItem& cvar : m_vCommandExecute)
	{
		if (cvar.text == buffer)
			return false;
	}

	m_vCommandExecute.emplace_back(buffer);
	return true;
}

void CBaseMenu::AddListSelect(const std::string & name, std::vector<SelectItem>& list)
{
	if (!ImGui::TreeNode(name.c_str()))
		return;

	size_t moveto = UINT_MAX;
	static char buffer[255]{'\0'};

	if (ImGui::Button("Add") && buffer[0] != '\0')
	{
		list.emplace_back(buffer);
		buffer[0] = '\0';
	}

	ImGui::SameLine();
	if (ImGui::Button("Remove"))
	{
		for (auto it = list.begin(); it != list.end(); )
		{
			if (it->selected)
				it = list.erase(it);
			else
				++it;
		}
	}

	ImGui::SameLine();
	if (ImGui::Button("Edit") && buffer[0] != '\0')
	{
		for (SelectItem& cvar : list)
		{
			if (!cvar.selected)
				continue;

			if (cvar.text != buffer)
			{
				cvar.text = buffer;
				break;
			}
		}

		buffer[0] = '\0';
	}

	ImGui::SameLine();
	if (ImGui::Button("Find") && buffer[0] != '\0')
	{
		for (size_t i = 0; i < list.size(); ++i)
		{
			if (list[i].text.find(buffer) != std::string::npos)
			{
				moveto = i;
				break;
			}
		}

		if (moveto < list.size())
		{
			for (SelectItem& cvar : list)
			{
				if (cvar.text != list[moveto].text)
					cvar.selected = false;
			}

			list[moveto].selected = true;
			buffer[0] = '\0';
		}
	}

	try
	{
		// 添加/重命名/搜索
		if (ImGui::InputText((name + "_input").c_str(), buffer, 255, ImGuiInputTextFlags_EnterReturnsTrue) && buffer[0] != '\0')
		{
			for (size_t i = 0; i < list.size(); ++i)
			{
				if (list[i].text.find(buffer) != std::string::npos)
				{
					moveto = i;
					break;
				}
			}

			if (moveto < list.size())
			{
				for (SelectItem& cvar : list)
				{
					if (cvar.text != list[moveto].text)
						cvar.selected = false;
				}

				list[moveto].selected = true;
				buffer[0] = '\0';
			}
		}
	}
	catch (...)
	{

	}

	ImGui::Separator();

	ImGui::BeginChild((name + "_unknown_list").c_str(), ImVec2(0, 300), true);

	size_t index = 0, stage = 0;
	for (auto it = list.begin(); it != list.end(); )
	{
		stage = 0;
		ImGui::Selectable(it->text.c_str(), &it->selected);

		// 右键菜单
		if (ImGui::BeginPopupContextItem())
		{
			// 删除
			if (ImGui::Selectable("delete"))
				stage = 1;

			static char buffer2[255];
			strcpy_s(buffer2, it->text.c_str());

			// 修改内容
			if (ImGui::InputText((name + "_rename").c_str(), buffer2, 255, ImGuiInputTextFlags_EnterReturnsTrue))
			{
				if (buffer2[0] == '\0')
					goto end_extra_menu;

				for (const SelectItem& i : list)
				{
					if (i.text == buffer2)
						goto end_extra_menu;
				}

				it->text = buffer2;
			}

		end_extra_menu:
			ImGui::EndPopup();
			buffer2[0] = '\0';
		}

		if (moveto == index)
			ImGui::SetScrollHere(index * 0.5f);

		if (stage == 1)
			it = list.erase(it);
		else
			++it;
	}

	ImGui::EndChild();
	ImGui::TreePop();
}

CBaseMenu::SelectItem::SelectItem() :
	text(""), selected(false)
{
}

CBaseMenu::SelectItem::SelectItem(const std::string & text, bool selected) :
	text(text), selected(selected)
{
}
