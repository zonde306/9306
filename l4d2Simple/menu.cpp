#include "menu.h"
#include "libraries/xorstr.h"
#include <imgui.h>
#include <vector>
#include <cstring>

std::unique_ptr<CBaseMenu> g_pBaseMenu;
extern bool g_bIsShowMenu;
extern std::vector<std::string> g_vsBannedQueryConVar;
extern std::vector<std::string> g_vsBannedSettingConVar;
extern std::vector<std::string> g_vsBannedExecuteCommand;

CBaseMenu::CBaseMenu() : m_bStateUpdated(false)
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
		return;
	}

	ImGui::Text(XorStr("Version: 1.0"));
	ImGui::Text(XorStr("Created by zonde306"));
	ImGui::GetIO().MouseDrawCursor = true;

	if (ImGui::TreeNode("Aimbot"))
	{
		ImGui::Checkbox("Auto Aim", &Config::bAimbot);
		ImGui::Checkbox("Only Firing", &Config::bAimbotKey);
		ImGui::Checkbox("Speed Prediction", &Config::bAimbotPred);
		ImGui::Checkbox("Silent Aim", &Config::bSilentAimbot);
		ImGui::DragFloat("Aimbot FOV", &Config::fAimbotFov, 1.0f, 1.0f, 360.0f);

		ImGui::Separator();
		ImGui::Checkbox("Recoil Control System", &Config::bAimbotRCS);
		ImGui::DragFloat("RCS X", &Config::fAimbotRCSX, 1.0f, 0.0f, 128.0f);
		ImGui::DragFloat("RCS Y", &Config::fAimbotRCSX, 1.0f, 0.0f, 128.0f);

		ImGui::Separator();
		ImGui::Checkbox("Trigger Bot", &Config::bTriggerBot);
		ImGui::Checkbox("Trigger Only Head", &Config::bTriggerBotHead);
		ImGui::Checkbox("Trigger Heads", &Config::bTriggerExtra);
		ImGui::DragInt("Trigger Heads Tick", &Config::iDuckAimbotTick, 10.0f, 0, 250);

		ImGui::Separator();
		ImGui::Checkbox("No Spread", &Config::bNoSpread);
		ImGui::Checkbox("No Recoil", &Config::bNoRecoil);
		ImGui::Checkbox("Rapid Fire", &Config::bRapidFire);
		ImGui::Checkbox("Dont Fire Team", &Config::bAnitFirendlyFire);

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Visual"))
	{
		ImGui::Checkbox("Player Box", &Config::bDrawBox);
		ImGui::Checkbox("Player Name", &Config::bDrawName);
		ImGui::Checkbox("Player/Infected Bone", &Config::bDrawBone);
		ImGui::Checkbox("Player Distance", &Config::bDrawDist);
		ImGui::Checkbox("Survivor Ammo", &Config::bDrawAmmo);
		ImGui::Checkbox("Off Screen ESP", &Config::bDrawOffScreen);
		ImGui::Checkbox("Crosshairs", &Config::bDrawCrosshairs);
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Weapon/Items"))
	{
		ImGui::Checkbox("T1 Weapon", &Config::bDrawT1Weapon);
		ImGui::Checkbox("T2 Weapon", &Config::bDrawT2Weapon);
		ImGui::Checkbox("T3 Weapon", &Config::bDrawT3Weapon);
		ImGui::Checkbox("Melee Weapon", &Config::bDrawMeleeWeapon);
		ImGui::Checkbox("Medical Items", &Config::bDrawMedicalItem);
		ImGui::Checkbox("Grenade", &Config::bDrawGrenadeItem);
		ImGui::Checkbox("Ammo Stack", &Config::bDrawAmmoStack);
		ImGui::Checkbox("Carry Items", &Config::bDrawCarryItem);
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("DirectX ZBuffer"))
	{
		ImGui::Checkbox("Survivors", &Config::bBufferSurvivor);
		ImGui::Checkbox("Special Infected", &Config::bBufferSpecial);
		ImGui::Checkbox("Common Infected", &Config::bBufferCommon);
		ImGui::Checkbox("Weapon", &Config::bBufferWeapon);
		ImGui::Checkbox("Grenade", &Config::bBufferGrenade);
		ImGui::Checkbox("Medical Items", &Config::bBufferMedical);
		ImGui::Checkbox("Carry Items", &Config::bBufferCarry);
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Anti SMAC"))
	{
		AddListSelect("Anti ConVar Query", m_vConVarQuery);
		AddListSelect("Anti ConVar Change", m_vConVarSetting);
		AddListSelect("Anti Execute Command", m_vCommandExecute);

		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Miscellaneous"))
	{
		ImGui::Checkbox("Auto Bunny Hop", &Config::bBunnyHop);
		ImGui::Checkbox("Auto Strafe", &Config::bAutoStrafe);
		ImGui::Checkbox("CRC Bypass", &Config::bCrcCheckBypass);
		ImGui::Checkbox("Backtrack", &Config::bBackTrack);
		ImGui::Checkbox("Forwardtrack", &Config::bForwardTrack);
		ImGui::Checkbox("Thrid Persons", &Config::bThirdPersons);

		ImGui::Separator();
		ImGui::Checkbox("Fast Melee", &Config::bMustFastMelee);
		ImGui::DragInt("Fast Melee Tick", &Config::iFastMeleeTick);

		ImGui::Separator();
		ImGui::Checkbox("Remove Fog", &Config::bRemoveFog);
		ImGui::Checkbox("Full Bright", &Config::bCvarFullBright);
		ImGui::Checkbox("Wirteframe", &Config::bCvarWireframe);
		ImGui::Checkbox("sv_cheats 1", &Config::bCvarCheats);
		// ImGui::Checkbox("mp_gamemode coop", &Config::bCvarGameMode);

		ImGui::Separator();
		ImGui::Checkbox("Teleport", &Config::bTeleport);
		ImGui::Checkbox("Teleport Exploit", &Config::bTeleportExploit);
		ImGui::Checkbox("Air Stuck", &Config::bAirStuck);
		ImGui::Checkbox("Position Adjustment", &Config::bPositionAdjustment);
		ImGui::Checkbox("Aimbot Kill Server", &Config::bCrashServer);

		ImGui::TreePop();
	}

	ImGui::End();
	ImGui::GetIO().MouseDrawCursor = false;
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
