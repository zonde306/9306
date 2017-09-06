#pragma once
#include "../../Utils.h"
#include "../libraries/math.h"

static std::map<std::string, unsigned int> g_offsetList;
extern CBaseEntity* g_pPlayerResource;
extern CBaseEntity* g_pGameRulesProxy;

#define NETPROP_GET_MAKE(_f,_t,_p,_r)	_r& _f()\
{\
	static int offset = g_pNetVars->GetOffset(_t, _p);\
	return *(_r*)(this + offset);\
}

#define NETPROP_TGET_MAKE(_f,_t,_p)	template<typename T> T& _f()\
{\
	static int offset = g_pNetVars->GetOffset(_t, _p);\
	return *(T*)(this + offset);\
}

#define NETPROP_SET_MAKE(_f,_t,_p,_r)	_r& _f(const _r& value)\
{\
	static int offset = g_pNetVars->GetOffset(_t, _p);\
	return (*(_r*)(this + offset) = value);\
}

#define NETPROP_TSET_MAKE(_f,_t,_p,_r)	template<typename T> T& _f(const T& value)\
{\
	static int offset = g_pNetVars->GetOffset(_t, _p);\
	return (*(T*)(this + offset) = value);\
}

class CBaseEntity
{
public:
	float& GetFriction()
	{
		static int offset = g_pNetVars->GetOffset("DT_BasePlayer", "m_flFriction");
		return *(float*)(this + offset);
	}
	
	int& GetTeam()
	{
		static int offset = g_pNetVars->GetOffset("DT_BasePlayer", "m_iTeamNum");
		return *(int*)(this + offset);
	}

	Vector& GetVelocity()
	{
		static int offset = g_pNetVars->GetOffset("DT_BasePlayer", "m_vecVelocity[0]");
		return *(Vector*)(this + offset);
	}

	int& GetHealth()
	{
		static int offset = g_pNetVars->GetOffset("DT_BasePlayer", "m_iHealth");
		return *(int*)(this + offset);
	}

	int Index()
	{
		return *(int*)((DWORD)this + 64);
	}

	int& GetFlags()
	{
		static int offset = g_pNetVars->GetOffset("DT_BasePlayer", "m_fFlags");
		return *(int*)(this + offset);
	}

	QAngle& GetAimPunch()
	{
		static int offset = g_pNetVars->GetOffset("DT_BasePlayer", "m_aimPunchAngle");
		return *(QAngle*)(this + offset);
	}

	QAngle& GetViewPunch()
	{
		static int offset = g_pNetVars->GetOffset("DT_BasePlayer", "m_viewPunchAngle");
		return *(QAngle*)(this + offset);
	}

	int& GetTickBase()
	{
		static int offset = g_pNetVars->GetOffset("DT_BasePlayer", "m_nTickBase");
		return *(int*)(this + offset);
	}

	Vector GetEyePosition()
	{
		static int offset = g_pNetVars->GetOffset("DT_BasePlayer", "m_vecViewOffset[0]");
		Vector vecViewOffset = *(Vector*)(this + offset);
		return GetAbsOrigin() + vecViewOffset;
	}

	QAngle& GetEyeAngles() const
	{
		static int offset = g_pNetVars->GetOffset("DT_CSPlayer", "m_angEyeAngles[0]");
		return *(Vector*)(this + offset);
	}

	CBaseHandle* GetActiveWeapon()
	{
		static int offset = g_pNetVars->GetOffset("DT_BaseCombatCharacter", "m_hActiveWeapon");
		return *(CBaseHandle**)(this + offset);
	}

	/*
	const char* GetClassname()
	{
		static int offset = g_pNetVars->GetOffset("DT_BaseEntity", "m_iClassname");
		return *(const char**)(this + offset);
	}

	const char* GetModelName()
	{
		static int offset = g_pNetVars->GetOffset("DT_BaseEntity", "m_ModelName");
		return *(const char**)(this + offset);
	}

	const char* GetName()
	{
		static int offset = g_pNetVars->GetOffset("DT_BaseEntity", "m_iName");
		return *(const char**)(this + offset);
	}
	*/

	static int GetNetPropOffset(const std::string& table, const std::string& prop)
	{
		if (g_offsetList.find(prop) == g_offsetList.end())
		{
			int offset = g_pNetVars->GetOffset(table.c_str(), prop.c_str());
			if (offset > -1)
			{
				Utils::log("%s::%s = 0x%X", table.c_str(), prop.c_str(), offset);
				g_offsetList[prop] = offset;
			}
			else
			{
				Utils::log("%s::%s not found", table.c_str(), prop.c_str());
			}
		}

		// 如果找不到的话就抛出错误
		return g_offsetList[prop];
	}

	template<typename... PropArg>
	static int GetNetPropOffsetEx(const std::string& table, const std::string& prop, const PropArg& ...arg)
	{
		return CBaseEntity::GetNetPropOffset(table, prop) + CBaseEntity::GetNetPropOffset(table, arg...);
	}

	template<typename T>
	inline T& GetNetProp(const std::string& prop, const std::string& table = "DT_BaseEntity", size_t element = 0) const
	{
		return *(T*)(this + this->GetNetPropOffset(table, prop) + element * sizeof(T));
	}

	template<typename T>
	inline T& SetNetProp(const std::string& prop, const T& value, const std::string& table = "DT_BaseEntity", size_t element = 0) const
	{
		return (*(T*)(this + this->GetNetPropOffset(table, prop) + element * sizeof(T)) = value);
	}

	template<typename T>
	inline T& GetNetProp2(const std::string& prop1, const std::string& prop2, const std::string& table = "DT_BaseEntity", size_t element = 0) const
	{
		return *(T*)(this + this->GetNetPropOffset(table, prop1) +
			this->GetNetPropOffset(table, prop2) + element * sizeof(T));
	}

	template<typename T>
	inline T& SetNetProp2(const std::string& prop1, const std::string& prop2, const T& value, const std::string& table = "DT_BaseEntity", size_t element = 0) const
	{
		return (*(T*)(this + this->GetNetPropOffset(table, prop1) +
			this->GetNetPropOffset(table, prop2) + element * sizeof(T)) = value);
	}

	template<typename T, typename... PropArg>
	inline T& GetNetPropEx(const std::string& table, size_t element, const PropArg& ...arg) const
	{
		return *(T*)(this + this->GetNetPropOffsetEx(table, arg...) + element * sizeof(T));
	}

	template<typename T, typename... PropArg>
	inline T& SetNetPropEx(const std::string& table, const T& value, size_t element, const PropArg& ...arg) const
	{
		return (*(T*)(this + this->GetNetPropOffsetEx(table, arg...) + element * sizeof(T)) = value);
	}

	template<typename T>
	inline T& GetLocalNetProp(const std::string& prop, size_t element = 0)
	{
		return (*(T*)(this + this->GetNetPropOffset("DT_BasePlayer", "m_Local") +
			this->GetNetPropOffset("DT_BasePlayer", prop) + element * sizeof(T)));
	}

	template<typename T>
	inline T& SetLocalNetProp(const std::string& prop, const T& value, size_t element = 0)
	{
		return (*(T*)(this + this->GetNetPropOffset("DT_BasePlayer", "m_Local") +
			this->GetNetPropOffset("DT_BasePlayer", prop) + element * sizeof(T)) = value);
	}

	Vector& GetAbsOrigin()
	{
		typedef Vector& (__thiscall* OriginalFn)(void*);
		return ((OriginalFn)VMT.GetFunction(this, indexes::GetAbsOrigin))(this);
	}

	Vector& GetAbsAngles()
	{
		typedef Vector& (__thiscall* OriginalFn)(void*);
		return ((OriginalFn)VMT.GetFunction(this, indexes::GetAbsAngles))(this);
	}

	ClientClass* GetClientClass()
	{
		void* Networkable = (void*)(this + 0x8);
		typedef ClientClass* (__thiscall* OriginalFn)(void*);
		return ((OriginalFn)VMT.GetFunction(Networkable, indexes::GetClientClass))(Networkable);
	}
	
	bool IsDormant()
	{
		// return *reinterpret_cast<bool*>((DWORD)this + 0xE9);
		PVOID pNetworkable = static_cast<PVOID>(this + 0x8);
		typedef bool(__thiscall * OriginalFn)(PVOID);

		bool result = true;

		try
		{
			result = ((OriginalFn)VMT.GetFunction(pNetworkable, indexes::IsDormant))(pNetworkable);
		}
		catch (...)
		{
			result = true;
		}

		return result;
	}

	bool IsAlive()
	{
		int id = 0, solid = 0, sequence = 0;
		ClientClass* cc = nullptr;

		try
		{
			// 检查是否为一个有效的指向实体的指针
			if (this->IsDormant() || !(cc = this->GetClientClass()) || (id = cc->m_ClassID) == ET_WORLD)
				return false;

			solid = this->GetNetProp<int>("m_usSolidFlags", "DT_BaseCombatCharacter");
			sequence = this->GetNetProp<int>("m_nSequence", "DT_BaseAnimating");
		}
		catch (std::exception& e)
		{
#ifdef _DEBUG
			Utils::log("%s (%d) 错误：", __FILE__, __LINE__, e.what());
#endif
			return false;
		}

		if (IsSurvivor(id))
		{
			// 生还者只需要检查生命状态 (生还者 0 血也可以活着的)
			if (this->GetNetProp<byte>("m_lifeState", "DT_BasePlayer") != 0)
				return false;
		}
		else if (IsSpecialInfected(id))
		{
			// 感染者检查血量和是否为灵魂状态
			if (this->GetNetProp<byte>("m_lifeState", "DT_BasePlayer") != 0 ||
				this->GetNetProp<byte>("m_iHealth", "DT_BasePlayer") < 1 ||
				this->GetNetProp<byte>("m_isGhost", "DT_TerrorPlayer"))
				return false;

			// 坦克检查是否处于僵直状态
			if (id == ET_TANK && sequence > 70)
				return false;
		}
		else if (IsCommonInfected(id))
		{
			// 检查普感是否处于死亡动作状态
			if ((solid & SF_NOT_SOLID) || sequence > 305)
				return false;

			// 检查普感是否被点燃了 (普感被点燃就是死亡)
			if (id == ET_INFECTED && this->GetNetProp<byte>("m_bIsBurning", "DT_Infected") != 0)
				return false;
		}
		
		// 如果不是生还者也不是感染者的话不需要检查是否活着，只要存在就是活着
		return true;
	}

	int GetIndex()
	{
		void* networkable = (void*)(this + 0x8);
		typedef int(__thiscall* OriginalFn)(PVOID);
		return ((OriginalFn)VMT.GetFunction(this, indexes::GetIndex))(this);
	}

	bool SetupBones(VMatrix *pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime)
	{
		void *pRenderable = (void*)(this + 0x4);
		typedef bool(__thiscall* OriginalFn)(PVOID, VMatrix*, int, int, float);
		return ((OriginalFn)VMT.GetFunction(pRenderable, indexes::SetupBones))(pRenderable, pBoneToWorldOut, nMaxBones, boneMask, currentTime);
	}

	model_t* GetModel()
	{
		void *pRenderable = (void*)(this + 0x4);
		typedef model_t*(__thiscall* OriginalFn)(PVOID);
		return VMT.getvfunc<OriginalFn>(pRenderable, indexes::GetModel)(pRenderable);
	}

	int GetWeaponID()
	{
		typedef int(__thiscall *OriginalFn)(PVOID);
		return VMT.getvfunc<OriginalFn>(this, indexes::GetWeaponId)(this);
	}

	Vector GetHitboxPosition(int Hitbox)
	{
		VMatrix matrix[128];
		model_t* mod;
		studiohdr_t* hdr;
		mstudiohitboxset_t* set;
		mstudiobbox_t* hitbox;
		Vector MIN, MAX, MIDDLE;

		try
		{
			if (!this->SetupBones(matrix, 128, 0x00000100, g_interface.Globals->curtime))
			{
				Utils::log("%s (%d) 错误：获取骨头位置失败 0x%X", __FILE__, __LINE__, (DWORD)this);
				return Vector();
			}

			if ((mod = this->GetModel()) == nullptr)
			{
				Utils::log("%s (%d) 错误：获取模型失败 0x%X", __FILE__, __LINE__, (DWORD)this);
				return Vector();
			}

			if ((hdr = g_interface.ModelInfo->GetStudioModel(mod)) == nullptr)
			{
				Utils::log("%s (%d) 错误：获取模型信息失败 0x%X", __FILE__, __LINE__, (DWORD)this);
				return Vector();
			}

			if ((set = hdr->pHitboxSet(0)) == nullptr)
			{
				Utils::log("%s (%d) 错误：获取 Hitbox 组失败 0x%X", __FILE__, __LINE__, (DWORD)this);
				return Vector();
			}

			if ((hitbox = set->pHitbox(Hitbox)) == nullptr)
			{
				Utils::log("%s (%d) 错误：搜索 Hitbox 失败 0x%X", __FILE__, __LINE__, (DWORD)this);
				return Vector();
			}
		}
		catch(...)
		{
			Utils::log("%s (%d) 错误：发生了未知错误 0x%X", __FILE__, __LINE__, (DWORD)this);
			return Vector();
		}
		
		VectorTransform(hitbox->bbmin, matrix[hitbox->bone], MIN);
		VectorTransform(hitbox->bbmax, matrix[hitbox->bone], MAX);

		MIDDLE = (MIN + MAX) * 0.5f;
		return MIDDLE;
	}

	Vector GetBonePosition(int bone)
	{
		VMatrix boneMatrix[128];
		try
		{
			if (this->SetupBones(boneMatrix, 128, 0x00000100, g_interface.Globals->curtime))
				return Vector(boneMatrix[bone][0][3], boneMatrix[bone][1][3], boneMatrix[bone][2][3]);
		}
		catch(...)
		{
			Utils::log("%s (%d) 错误：发生了未知错误 0x%X", __FILE__, __LINE__, (DWORD)this);
			return Vector();
		}

		return Vector();
	}

#ifdef m_iCrosshairsId
	int GetCrosshairsId()
	{
		return *(int*)(this + m_iCrosshairsId);
	}
#endif // m_iCrosshairsId

};

class ICollideable;
class IClientNetworkable;
class IClientRenderable;
class IClientEntity;
class IClientThinkable;
class IClientAlphaProperty;
class IClientModelRenderable;

class IClientUnknown : public IHandleEntity
{
public:
	virtual ICollideable*			GetCollideable() = 0;
	virtual IClientNetworkable*		GetClientNetworkable() = 0;
	virtual IClientRenderable*		GetClientRenderable() = 0;
	virtual IClientEntity*			GetIClientEntity() = 0;
	virtual CBaseEntity*			GetBaseEntity() = 0;
	virtual IClientThinkable*		GetClientThinkable() = 0;
	virtual IClientModelRenderable*	GetClientModelRenderable() = 0;
	virtual IClientAlphaProperty*	GetClientAlphaProperty() = 0;
};

class IClientNetworkable
{
public:
	virtual IClientUnknown*			GetIClientUnknown() = 0;
	virtual void					Release() = 0;
	virtual ClientClass*			GetClientClass() = 0;
	virtual void					NotifyShouldTransmit(int state) = 0;
	virtual void					OnPreDataChanged(int updateType) = 0;
	virtual void					OnDataChanged(int updateType) = 0;
	virtual void					PreDataUpdate(int updateType) = 0;
	virtual void					PostDataUpdate(int updateType) = 0;
	virtual void					__unkn(void) = 0;
	virtual bool					IsDormant(void) = 0;
	virtual int						EntIndex(void) const = 0;
	virtual void					ReceiveMessage(int classID, bf_read& msg) = 0;
	virtual void*					GetDataTableBasePtr() = 0;
	virtual void					SetDestroyedOnRecreateEntities(void) = 0;
};

typedef unsigned short ClientShadowHandle_t;
typedef unsigned short ClientRenderHandle_t;
typedef unsigned short ModelInstanceHandle_t;

class IClientRenderable
{
public:
	virtual IClientUnknown*			GetIClientUnknown() = 0;
	virtual Vector const&			GetRenderOrigin(void) = 0;
	virtual QAngle const&			GetRenderAngles(void) = 0;
	virtual bool					ShouldDraw(void) = 0;
	virtual int						GetRenderFlags(void) = 0; // ERENDERFLAGS_xxx
	virtual void					Unused(void) const {}
	virtual ClientShadowHandle_t	GetShadowHandle() const = 0;
	virtual ClientRenderHandle_t&	RenderHandle() = 0;
	virtual const model_t*			GetModel() const = 0;
	virtual int						DrawModel(int flags, const int &instance) = 0;
	virtual int						GetBody() = 0;
	virtual void					GetColorModulation(float* color) = 0;
	virtual bool					LODTest() = 0;
	virtual bool					SetupBones(matrix3x4_t *pBoneToWorldOut, int nMaxBones, int boneMask, float currentTime) = 0;
	virtual void					SetupWeights(const matrix3x4_t *pBoneToWorld, int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights) = 0;
	virtual void					DoAnimationEvents(void) = 0;
	virtual void*					GetPVSNotifyInterface() = 0;
	virtual void					GetRenderBounds(Vector& mins, Vector& maxs) = 0;
	virtual void					GetRenderBoundsWorldspace(Vector& mins, Vector& maxs) = 0;
	virtual void					GetShadowRenderBounds(Vector &mins, Vector &maxs, int shadowType) = 0;
	virtual bool					ShouldReceiveProjectedTextures(int flags) = 0;
	virtual bool					GetShadowCastDistance(float *pDist, int shadowType) const = 0;
	virtual bool					GetShadowCastDirection(Vector *pDirection, int shadowType) const = 0;
	virtual bool					IsShadowDirty() = 0;
	virtual void					MarkShadowDirty(bool bDirty) = 0;
	virtual IClientRenderable*		GetShadowParent() = 0;
	virtual IClientRenderable*		FirstShadowChild() = 0;
	virtual IClientRenderable*		NextShadowPeer() = 0;
	virtual int						ShadowCastType() = 0;
	virtual void					CreateModelInstance() = 0;
	virtual ModelInstanceHandle_t	GetModelInstance() = 0;
	virtual const matrix3x4_t&		RenderableToWorldTransform() = 0;
	virtual int						LookupAttachment(const char *pAttachmentName) = 0;
	virtual bool					GetAttachment(int number, Vector &origin, QAngle &angles) = 0;
	virtual bool					GetAttachment(int number, matrix3x4_t &matrix) = 0;
	virtual float*					GetRenderClipPlane(void) = 0;
	virtual int						GetSkin() = 0;
	virtual void					OnThreadedDrawSetup() = 0;
	virtual bool					UsesFlexDelayedWeights() = 0;
	virtual void					RecordToolMessage() = 0;
	virtual bool					ShouldDrawForSplitScreenUser(int nSlot) = 0;
	virtual uint8_t					OverrideAlphaModulation(uint8_t nAlpha) = 0;
	virtual uint8_t					OverrideShadowAlphaModulation(uint8_t nAlpha) = 0;
};

void CRecipientFilter::AddRecipient(CBaseEntity *player)
{
	//Assert(player);

	if (!player)
		return;

	int index = player->GetIndex();

	// If we're predicting and this is not the first time we've predicted this sound
	//  then don't send it to the local player again.
	//if(m_bUsingPredictionRules)
	//{
	//	// Only add local player if this is the first time doing prediction
	//	if(g_RecipientFilterPredictionSystem.GetSuppressHost() == player)
	//	{
	//		return;
	//	}
	//}

	// Already in list
	//if(m_Recipients.Find(index) != m_Recipients.InvalidIndex())
	//	return;

	m_Recipients.AddToTail(index);
}

int GetPlayerPing(int player)
{
	if (g_pPlayerResource == nullptr || player <= 0 || player > 32)
		return -1;

	static int offset = g_pNetVars->GetOffset("DT_TerrorPlayerResource", "m_iPing");
	return *(int*)(g_pPlayerResource + offset + (player * 4));
}

int GetPlayerScore(int player)
{
	if (g_pPlayerResource == nullptr || player <= 0 || player > 32)
		return -1;

	static int offset = g_pNetVars->GetOffset("DT_TerrorPlayerResource", "m_iScore");
	return *(int*)(g_pPlayerResource + offset + (player * 4));
}

int GetPlayerDeath(int player)
{
	if (g_pPlayerResource == nullptr || player <= 0 || player > 32)
		return -1;

	static int offset = g_pNetVars->GetOffset("DT_TerrorPlayerResource", "m_iDeaths");
	return *(int*)(g_pPlayerResource + offset + (player * 4));
}

bool IsPlayerConnected(int player)
{
	if (g_pPlayerResource == nullptr || player <= 0 || player > 32)
		return false;

	static int offset = g_pNetVars->GetOffset("DT_TerrorPlayerResource", "m_bConnected");
	return ((*(byte*)(g_pPlayerResource + offset + player)) & 1);
}

int GetPlayerTeam(int player)
{
	if (g_pPlayerResource == nullptr || player <= 0 || player > 32)
		return -1;

	static int offset = g_pNetVars->GetOffset("DT_TerrorPlayerResource", "m_iTeam");
	return *(int*)(g_pPlayerResource + offset + (player * 4));
}

bool IsPlayerAlive(int player)
{
	if (g_pPlayerResource == nullptr || player <= 0 || player > 32)
		return false;

	static int offset = g_pNetVars->GetOffset("DT_TerrorPlayerResource", "m_bAlive");
	return ((*(byte*)(g_pPlayerResource + offset + player)) & 1);
}

int GetPlayerHealth(int player)
{
	if (g_pPlayerResource == nullptr || player <= 0 || player > 32)
		return -1;

	static int offset = g_pNetVars->GetOffset("DT_TerrorPlayerResource", "m_iHealth");
	return *(int*)(g_pPlayerResource + offset + (player * 4));
}

int GetPlayerMaxHealth(int player)
{
	if (g_pPlayerResource == nullptr || player <= 0 || player > 32)
		return -1;

	static int offset = g_pNetVars->GetOffset("DT_TerrorPlayerResource", "m_maxHealth");
	return *(int*)(g_pPlayerResource + offset + (player * 4));
}

bool IsPlayerGhost(int player)
{
	if (g_pPlayerResource == nullptr || player <= 0 || player > 32)
		return false;

	static int offset = g_pNetVars->GetOffset("DT_TerrorPlayerResource", "m_isGhost");
	return ((*(byte*)(g_pPlayerResource + offset + player)) & 1);
}

bool IsPlayerIncapacitated(int player)
{
	if (g_pPlayerResource == nullptr || player <= 0 || player > 32)
		return false;

	static int offset = g_pNetVars->GetOffset("DT_TerrorPlayerResource", "m_isIncapacitated");
	return ((*(byte*)(g_pPlayerResource + offset + player)) & 1);
}

int GetPlayerClass(int player)
{
	if (g_pPlayerResource == nullptr || player <= 0 || player > 32)
		return -1;

	static int offset = g_pNetVars->GetOffset("DT_TerrorPlayerResource", "m_zombieClass");
	return *(int*)(g_pPlayerResource + offset + (player * 4));
}

bool IsPlayerHost(int player)
{
	if (g_pPlayerResource == nullptr || player <= 0 || player > 32)
		return false;

	static int offset = g_pNetVars->GetOffset("DT_TerrorPlayerResource", "m_listenServerHost");
	return ((*(byte*)(g_pPlayerResource + offset + player)) & 1);
}

int GetSharedRandomSeed()
{
	if (g_pPlayerResource == nullptr)
		return -1;

	static int offset = g_pNetVars->GetOffset("DT_TerrorPlayerResource", "m_sharedRandomSeed");
	return *(int*)(g_pPlayerResource + offset);
}

CBaseHandle* GetPlayerSlot(int player, int slot)
{
	if (g_pPlayerResource == nullptr || player <= 0 || player > 32)
		return nullptr;

	static int primary = g_pNetVars->GetOffset("DT_TerrorPlayerResource", "m_primaryWeapon");
	static int grenade = g_pNetVars->GetOffset("DT_TerrorPlayerResource", "m_grenade");
	static int medickit = g_pNetVars->GetOffset("DT_TerrorPlayerResource", "m_firstAidSlot");
	static int pills = g_pNetVars->GetOffset("DT_TerrorPlayerResource", "m_pillsSlot");

	switch (slot)
	{
	case 0:
		return *(CBaseHandle**)(g_pPlayerResource + primary + (player * 4));
	case 2:
		return *(CBaseHandle**)(g_pPlayerResource + grenade + (player * 4));
	case 3:
		return *(CBaseHandle**)(g_pPlayerResource + medickit + (player * 4));
	case 4:
		return *(CBaseHandle**)(g_pPlayerResource + pills + (player * 4));
	}

	return nullptr;
}
