#pragma once
#define  Assert( _exp ) ((void)0)

class __declspec(align(16))VectorAligned : public Vector
{
public:
	VectorAligned& operator=(const Vector &vOther)
	{
		Init(vOther.x, vOther.y, vOther.z);
		w = 0.f;
		return *this;
	}

	// this space is used anyway
	float w;
};

struct Ray_t
{
	VectorAligned  m_Start;    // starting point, centered within the extents
	VectorAligned  m_Delta;    // direction + length of the ray
	VectorAligned  m_StartOffset;    // Add this to m_Start to get the actual ray start
	VectorAligned  m_Extents;    // Describes an axis aligned box extruded along a ray

	// 要不要都可以的
	// const matrix3x4_t *m_pWorldAxisTransform;

	bool    m_IsRay;    // are the extents zero?
	bool    m_IsSwept;    // is delta != 0?

	void Init(const Vector& vecStart, const Vector& vecEnd)
	{
		m_Delta = vecEnd - vecStart;

		m_IsSwept = (m_Delta.LengthSqr() != 0);

		m_Extents.x = m_Extents.y = m_Extents.z = 0.0f;
		m_IsRay = true;

		m_StartOffset.x = m_StartOffset.y = m_StartOffset.z = 0.0f;

		m_Start = vecStart;
	}

	void Init(Vector const& start, Vector const& end, Vector const& mins, Vector const& maxs)
	{
		Assert(&end);
		VectorSubtract(end, start, m_Delta);

		m_IsSwept = (m_Delta.LengthSqr() != 0);

		VectorSubtract(maxs, mins, m_Extents);
		m_Extents *= 0.5f;
		m_IsRay = (m_Extents.LengthSqr() < 1e-6);

		// Offset m_Start to be in the center of the box...
		VectorAdd(mins, maxs, m_StartOffset);
		m_StartOffset *= 0.5f;
		VectorAdd(start, m_StartOffset, m_Start);
		m_StartOffset *= -1.0f;
	}

	// compute inverse delta
	Vector InvDelta() const
	{
		Vector vecInvDelta;
		for (int iAxis = 0; iAxis < 3; ++iAxis)
		{
			if (m_Delta[iAxis] != 0.0f)
			{
				vecInvDelta[iAxis] = 1.0f / m_Delta[iAxis];
			}
			else
			{
				vecInvDelta[iAxis] = FLT_MAX;
			}
		}
		return vecInvDelta;
	}

private:
};

struct csurface_t
{
	const char *name;
	short surfaceProps;
	unsigned short flags;
};

struct cplane_t
{
	Vector normal;
	float dist;
	byte type;
	byte signbits;
	byte pad[2];
};

struct trace_t
{
	Vector start;
	Vector end;
	cplane_t plane;
	float fraction;
	int contents;
	WORD dispFlags;
	bool allsolid;
	bool startSolid;
	float fractionLeftSolid;
	csurface_t surface;
	int hitGroup;
	short physicsBone;
	CBaseEntity* m_pEnt;
	int hitbox;
};

enum TraceType_t
{
	TRACE_EVERYTHING = 0,
	TRACE_WORLD_ONLY,				// NOTE: This does *not* test static props!!!
	TRACE_ENTITIES_ONLY,			// NOTE: This version will *not* test static props
	TRACE_EVERYTHING_FILTER_PROPS,	// NOTE: This version will pass the IHandleEntity for props through the filter, unlike all other filters
};

class ITraceFilter
{
public:
	virtual bool			ShouldHitEntity(CBaseEntity* pEntity, int mask) = 0;
	virtual TraceType_t		GetTraceType() const = 0;
};

class CTraceFilter : public ITraceFilter
{
public:
	virtual bool ShouldHitEntity(CBaseEntity* pEntityHandle, int contentsMask) override
	{
		return !(pEntityHandle == pSkip1);
	}

	virtual TraceType_t GetTraceType() const override
	{
		return TRACE_EVERYTHING;
	}

	CBaseEntity* pSkip1;
};

class CTraceFilterShots : public ITraceFilter
{
public:
	virtual bool ShouldHitEntity(CBaseEntity* pEntityHandle, int contentsMask) override
	{
		// 检查是否活着
		if (pEntityHandle == nullptr || !pEntityHandle->IsAlive())
			return false;

		int classId = pEntityHandle->GetClientClass()->m_ClassID;
		if (!IsSurvivor(classId) && !IsSpecialInfected(classId) && !IsCommonInfected(classId))
		{
			// 开门复活点
			if (classId == ET_SurvivorRescue)
				return false;

			// Tank 的石头
			if (classId == ET_TankRock)
				return true;

			// 检查可见
			if (pEntityHandle->GetNetProp<int>("m_fEffects", "DT_BaseAnimating") & EF_NODRAW)
				return false;

			// 检查碰撞
			if (pEntityHandle->GetNetProp2<int>("m_Collision", "m_usSolidFlags", "DT_BasePlayer") & SF_NOT_SOLID)
				return false;
		}
		else if (IsSpecialInfected(classId))
		{
			// 灵魂状态的特感
			if (pEntityHandle->GetNetProp<byte>("m_isGhost", "DT_TerrorPlayer") != 0)
				return false;
		}
		
		return !(pEntityHandle == pSkip1);
	}

	virtual TraceType_t GetTraceType() const override
	{
		return TRACE_EVERYTHING;
	}

	CBaseEntity* pSkip1;
};

class CTraceFilterFunction : public CTraceFilter
{
public:
	CTraceFilterFunction()
	{
		pSkip1 = nullptr;
		filter = [this](CBaseEntity* entity, int contentsMask) -> bool
		{
			if (!entity->IsAlive())
			{
				// ClientClass* cc = entity->GetClientClass();
				// Utils::log("filter: Not Alive: %s (%d)", cc->m_pNetworkName, cc->m_ClassID);
				return false;
			}

			int classId = entity->GetClientClass()->m_ClassID;
			if (!IsSurvivor(classId) && !IsSpecialInfected(classId) && !IsCommonInfected(classId))
			{
				// 这玩意看不见，子弹可以穿过，但是却可以被 TraceRay 命中
				if (classId == ET_SurvivorRescue)
				{
					// Utils::log("filter: ClassID is ET_SurvivorRescue");
					return false;
				}
				else if (classId == ET_TankRock)
				{
					// Utils::log("filter: ClassID is ET_TankRock");
					return true;
				}

				// Infected 自带 SF_NOT_SOLID
				if (// (entity->GetNetProp2<int>("m_Collision", "m_nSolidType", "DT_BasePlayer") == SOLID_NONE) ||
					(entity->GetNetProp2<int>("m_Collision", "m_usSolidFlags", "DT_BasePlayer") & SF_NOT_SOLID))
				{
					// Utils::log("filter: SF_NOT_SOLID: %d", classId);
					return false;
				}
			}

			return true;
		};
	}
	
	virtual bool ShouldHitEntity(CBaseEntity* pEntityHandle, int contentsMask) override
	{
		CBaseEntity* entity = pEntityHandle;
		
		try
		{
			if (entity->GetClientClass() == nullptr || entity->IsDormant())
			{
				// Utils::log("TraceFilterError: InvalidEntity");
				return false;
			}

			if (filter && !filter(entity, contentsMask))
			{
				// Utils::log("TraceFilterError: m_usSolidFlags");
				// Utils::log("TraceFilterError: m_usSolidFlags: %s", entity->GetClientClass()->m_pNetworkName);
				return false;
			}

			return (pEntityHandle != pSkip1);
		}
		catch(...)
		{
			// Utils::log("TraceFilterError: GetClientClass");
		}

		return (pEntityHandle != pSkip1);
	}

	std::function<bool(CBaseEntity*, int)> filter;
};

class CTraceFilterSimple : public CTraceFilter
{
public:
	CTraceFilterSimple(const CBaseEntity* passentity, int collisionGroup)
	{
		m_pPassEnt = passentity;
		m_collisionGroup = collisionGroup;
	}

	virtual bool ShouldHitEntity(CBaseEntity* pHandleEntity, int contentsMask) override
	{
		return !(pHandleEntity == m_pPassEnt);
	}
	virtual void SetPassEntity(const CBaseEntity* pPassEntity) { m_pPassEnt = pPassEntity; }
	virtual void SetCollisionGroup(int iCollisionGroup) { m_collisionGroup = iCollisionGroup; }

	const CBaseEntity *GetPassEntity(void) { return m_pPassEnt; }

private:
	const CBaseEntity *m_pPassEnt;
	int m_collisionGroup;
};

class CTrace
{
public:
	void TraceRay(const Ray_t &ray, unsigned int fMask, ITraceFilter* filter, trace_t *trace)
	{
		typedef void(__thiscall* fn)(void*, const Ray_t&, unsigned int, ITraceFilter*, trace_t*);
		VMT.getvfunc<fn>(this, indexes::TraceRay)(this, ray, fMask, filter, trace);
	}
};

class IEntityEnumerator
{
public:
	// This gets called with each handle
	virtual bool EnumEntity(IHandleEntity *pHandleEntity) = 0;
};

enum IterationRetval_t
{
	ITERATION_CONTINUE = 0,
	ITERATION_STOP,
};

class IPartitionEnumerator
{
public:
	virtual IterationRetval_t EnumElement(IHandleEntity *pHandleEntity) = 0;
};

class CTraceListData;

class IEngineTrace
{
public:
	// Returns the contents mask + entity at a particular world-space position
	virtual int		GetPointContents(const Vector &vecAbsPosition, IHandleEntity** ppEntity = NULL) = 0;

	virtual int		GetPointContents_WorldOnly(Vector const&, int) = 0;

	// Get the point contents, but only test the specific entity. This works
	// on static props and brush models.
	//
	// If the entity isn't a static prop or a brush model, it returns CONTENTS_EMPTY and sets
	// bFailed to true if bFailed is non-null.
	virtual int		GetPointContents_Collideable(ICollideable *pCollide, const Vector &vecAbsPosition) = 0;

	// Traces a ray against a particular entity
	virtual void	ClipRayToEntity(const Ray_t &ray, unsigned int fMask, IHandleEntity *pEnt, trace_t *pTrace) = 0;

	// Traces a ray against a particular entity
	virtual void	ClipRayToCollideable(const Ray_t &ray, unsigned int fMask, ICollideable *pCollide, trace_t *pTrace) = 0;

	// A version that simply accepts a ray (can work as a traceline or tracehull)
	virtual void	TraceRay(const Ray_t &ray, unsigned int fMask, ITraceFilter *pTraceFilter, trace_t *pTrace) = 0;

	// A version that sets up the leaf and entity lists and allows you to pass those in for collision.
	virtual void	SetupLeafAndEntityListRay(const Ray_t &ray, CTraceListData &traceData) = 0;
	virtual void    SetupLeafAndEntityListBox(const Vector &vecBoxMin, const Vector &vecBoxMax, CTraceListData &traceData) = 0;
	virtual void	TraceRayAgainstLeafAndEntityList(const Ray_t &ray, CTraceListData &traceData, unsigned int fMask, ITraceFilter *pTraceFilter, trace_t *pTrace) = 0;

	// A version that sweeps a collideable through the world
	// abs start + abs end represents the collision origins you want to sweep the collideable through
	// vecAngles represents the collision angles of the collideable during the sweep
	virtual void	SweepCollideable(ICollideable *pCollide, const Vector &vecAbsStart, const Vector &vecAbsEnd,
		const QAngle &vecAngles, unsigned int fMask, ITraceFilter *pTraceFilter, trace_t *pTrace) = 0;

	// Enumerates over all entities along a ray
	// If triggers == true, it enumerates all triggers along a ray
	virtual void	EnumerateEntities(const Ray_t &ray, bool triggers, IEntityEnumerator *pEnumerator) = 0;

	// Same thing, but enumerate entitys within a box
	virtual void	EnumerateEntities(const Vector &vecAbsMins, const Vector &vecAbsMaxs, IEntityEnumerator *pEnumerator) = 0;

	// Convert a handle entity to a collideable.  Useful inside enumer
	virtual ICollideable *GetCollideable(IHandleEntity *pEntity) = 0;

	// HACKHACK: Temp for performance measurments
	virtual int GetStatByIndex(int index, bool bClear) = 0;

	//finds brushes in an AABB, prone to some false positives
	virtual void GetBrushesInAABB(const Vector &vMins, const Vector &vMaxs, CUtlVector<int> *pOutput, int iContentsMask = 0xFFFFFFFF) = 0;

	//retrieve brush planes and contents, returns true if data is being returned in the output pointers, false if the brush doesn't exist
	virtual void GetBrushInfo(int, CUtlVector<Vector4D, CUtlMemory<Vector4D, int> >*, int*) = 0;

	//Tests a point to see if it's outside any playable area
	virtual bool PointOutsideWorld(const Vector &ptTest) = 0;

	// Walks bsp to find the leaf containing the specified point
	virtual int GetLeafContainingPoint(const Vector &ptTest) = 0;

	virtual CTraceListData* AllocTraceListData() = 0;

	virtual void FreeTraceListData(CTraceListData*) = 0;

	virtual void SetPhysics2World(void*) = 0;

	virtual void PrintDebugName(char*, CBaseEntity*) = 0;

	virtual void TraceRay_Physics2(Ray_t const&, unsigned int, ITraceFilter*, trace_t*) = 0;
};

#define TLD_DEF_LEAF_MAX	256
#define TLD_DEF_ENTITY_MAX	1024

class CTraceListData : public IPartitionEnumerator
{
public:

	CTraceListData(int nLeafMax = TLD_DEF_LEAF_MAX, int nEntityMax = TLD_DEF_ENTITY_MAX)
	{
		// MEM_ALLOC_CREDIT();
		m_nLeafCount = 0;
		m_aLeafList.SetSize(nLeafMax);

		m_nEntityCount = 0;
		m_aEntityList.SetSize(nEntityMax);
	}

	~CTraceListData()
	{
		m_nLeafCount = 0;
		m_aLeafList.RemoveAll();

		m_nEntityCount = 0;
		m_aEntityList.RemoveAll();
	}

	void Reset(void)
	{
		m_nLeafCount = 0;
		m_nEntityCount = 0;
	}

	bool	IsEmpty(void) const { return (m_nLeafCount == 0 && m_nEntityCount == 0); }

	int		LeafCount(void) const { return m_nLeafCount; }
	int		LeafCountMax(void) const { return m_aLeafList.Count(); }
	void    LeafCountReset(void) { m_nLeafCount = 0; }

	int		EntityCount(void) const { return m_nEntityCount; }
	int		EntityCountMax(void) const { return m_aEntityList.Count(); }
	void	EntityCountReset(void) { m_nEntityCount = 0; }

	// For leaves...
	void AddLeaf(int iLeaf)
	{
		if (m_nLeafCount >= m_aLeafList.Count())
		{
			// DevMsg("CTraceListData: Max leaf count along ray exceeded!\n");
			m_aLeafList.AddMultipleToTail(m_aLeafList.Count());
		}

		m_aLeafList[m_nLeafCount] = iLeaf;
		m_nLeafCount++;
	}

	// For entities...
	IterationRetval_t EnumElement(IHandleEntity *pHandleEntity)
	{
		if (m_nEntityCount >= m_aEntityList.Count())
		{
			// DevMsg("CTraceListData: Max entity count along ray exceeded!\n");
			m_aEntityList.AddMultipleToTail(m_aEntityList.Count());
		}

		m_aEntityList[m_nEntityCount] = pHandleEntity;
		m_nEntityCount++;

		return ITERATION_CONTINUE;
	}

public:

	int							m_nLeafCount;
	CUtlVector<int>				m_aLeafList;

	int							m_nEntityCount;
	CUtlVector<IHandleEntity*>	m_aEntityList;
};
