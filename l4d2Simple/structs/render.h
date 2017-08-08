#pragma once

struct DrawModelState_t
{
	studiohdr_t*			m_pStudioHdr;
	unsigned long			m_pStudioHWData;
	CBaseEntity*		m_pRenderable;
	const matrix3x4_t		*m_pModelToWorld;
	unsigned long		m_decals;
	int						m_drawFlags;
	int						m_lod;
};

enum OverrideType_t
{
	OVERRIDE_NORMAL = 0,
	OVERRIDE_BUILD_SHADOWS,
	OVERRIDE_DEPTH_WRITE,
	OVERRIDE_SSAO_DEPTH_WRITE,
};

struct ModelRenderInfo_t
{
	Vector origin;
	QAngle angles;
	CBaseEntity *pRenderable;
	const model_t *pModel;
	const matrix3x4_t *pModelToWorld;
	const matrix3x4_t *pLightingOffset;
	const Vector *pLightingOrigin;
	int flags;
	int entity_index;
	int skin;
	int body;
	int hitboxset;
	unsigned short instance;
	ModelRenderInfo_t()
	{
		pModelToWorld = NULL;
		pLightingOffset = NULL;
		pLightingOrigin = NULL;
	}
};

enum MaterialVarFlags_t
{
	MATERIAL_VAR_DEBUG = (1 << 0),
	MATERIAL_VAR_NO_DEBUG_OVERRIDE = (1 << 1),
	MATERIAL_VAR_NO_DRAW = (1 << 2),
	MATERIAL_VAR_USE_IN_FILLRATE_MODE = (1 << 3),
	MATERIAL_VAR_VERTEXCOLOR = (1 << 4),
	MATERIAL_VAR_VERTEXALPHA = (1 << 5),
	MATERIAL_VAR_SELFILLUM = (1 << 6),
	MATERIAL_VAR_ADDITIVE = (1 << 7),
	MATERIAL_VAR_ALPHATEST = (1 << 8),
	MATERIAL_VAR_ZNEARER = (1 << 10),
	MATERIAL_VAR_MODEL = (1 << 11),
	MATERIAL_VAR_FLAT = (1 << 12),
	MATERIAL_VAR_NOCULL = (1 << 13),
	MATERIAL_VAR_NOFOG = (1 << 14),
	MATERIAL_VAR_IGNOREZ = (1 << 15),
	MATERIAL_VAR_DECAL = (1 << 16),
	MATERIAL_VAR_ENVMAPSPHERE = (1 << 17), // OBSOLETE
	MATERIAL_VAR_ENVMAPCAMERASPACE = (1 << 19), // OBSOLETE
	MATERIAL_VAR_BASEALPHAENVMAPMASK = (1 << 20),
	MATERIAL_VAR_TRANSLUCENT = (1 << 21),
	MATERIAL_VAR_NORMALMAPALPHAENVMAPMASK = (1 << 22),
	MATERIAL_VAR_NEEDS_SOFTWARE_SKINNING = (1 << 23), // OBSOLETE
	MATERIAL_VAR_OPAQUETEXTURE = (1 << 24),
	MATERIAL_VAR_ENVMAPMODE = (1 << 25), // OBSOLETE
	MATERIAL_VAR_SUPPRESS_DECALS = (1 << 26),
	MATERIAL_VAR_HALFLAMBERT = (1 << 27),
	MATERIAL_VAR_WIREFRAME = (1 << 28),
	MATERIAL_VAR_ALLOWALPHATOCOVERAGE = (1 << 29),
	MATERIAL_VAR_ALPHA_MODIFIED_BY_PROXY = (1 << 30),
	MATERIAL_VAR_VERTEXFOG = (1 << 31),
};

class CMaterial
{
public:
	const char* GetName()
	{
		typedef const char* (__thiscall* OriginalFn)(PVOID);
		return VMT.getvfunc<OriginalFn>(this, 0)(this);
	}
	const char* GetTextureGroupName()
	{
		typedef const char* (__thiscall* OriginalFn)(PVOID);
		return VMT.getvfunc<OriginalFn>(this, 1)(this);
	}
	void AlphaModulate(float alpha)
	{
		typedef void(__thiscall* OriginalFn)(PVOID, float);
		VMT.getvfunc<OriginalFn>(this, 27)(this, alpha);
	}
	void ColorModulate(float r, float g, float b)
	{
		typedef void(__thiscall* OriginalFn)(PVOID, float, float, float);
		VMT.getvfunc<OriginalFn>(this, 28)(this, r, g, b);
	}
	void SetMaterialVarFlag(MaterialVarFlags_t flag, bool on)
	{
		typedef void(__thiscall* OriginalFn)(PVOID, MaterialVarFlags_t, bool);
		VMT.getvfunc<OriginalFn>(this, 29)(this, flag, on);
	}
};


class IMaterialSystem
{
public:
	CMaterial* FindMaterial(char const* pMaterialName, const char *pTextureGroupName,
		bool complain = true, const char *pComplainPrefix = NULL)
	{
		typedef CMaterial*(__thiscall* Fn)(PVOID, char const*, const char*, bool, const char*);
		return VMT.getvfunc<Fn>(this, indexes::FindMaterial)(this, pMaterialName, pTextureGroupName,
			complain, pComplainPrefix);
	}

};

class CModelRender
{
public:
	void DrawModelExecute(void* context, DrawModelState_t &state, const ModelRenderInfo_t &pInfo, matrix3x4_t *pCustomBoneToWorld)
	{
		typedef void(__thiscall* OriginalFn)(PVOID, void* context, DrawModelState_t &state, const ModelRenderInfo_t &pInfo, matrix3x4_t *pCustomBoneToWorld);
		VMT.getvfunc<OriginalFn>(this, indexes::DrawModel)(this, context, state, pInfo, pCustomBoneToWorld);
	}

	void ForcedMaterialOverride(CMaterial *newMaterial, OverrideType_t nOverrideType = OVERRIDE_NORMAL)
	{
		typedef void(__thiscall* OriginalFn)(PVOID, CMaterial *newMaterial, OverrideType_t nOverrideType);
		VMT.getvfunc<OriginalFn>(this, indexes::ForcedMaterialOverride)(this, newMaterial, nOverrideType);
	}

	int DrawModel(int flags, IClientRenderable* pRenderable, ModelInstanceHandle_t instance,
		int entity_index, const model_t *model, Vector const& origin, QAngle const& angles,
		int skin, int body, int hitboxset, const matrix3x4_t *modelToWorld = NULL,
		const matrix3x4_t *pLightingOffset = NULL)
	{
		typedef int(__thiscall* Fn)(PVOID, int, IClientRenderable*, ModelInstanceHandle_t,
			int, const model_t*, Vector const&, QAngle const&, int, int, int, const matrix3x4_t*,
			const matrix3x4_t*);

		return VMT.getvfunc<Fn>(this, indexes::DrawModel)(this, flags, pRenderable, instance,
			entity_index, model, origin, angles, skin, body, hitboxset, modelToWorld, pLightingOffset);
	}

	int DrawModelEx(ModelRenderInfo_t& pInfo)
	{
		typedef int(__thiscall* Fn)(PVOID, ModelRenderInfo_t&);
		return VMT.getvfunc<Fn>(this, indexes::DrawModelEx)(this, pInfo);
	}
};

class CRenderView
{
public:
	void SetColorModulation(float  const* blend)
	{
		typedef void(__thiscall* OriginalFn)(PVOID, float  const* blend);
		VMT.getvfunc<OriginalFn>(this, indexes::SetColorModulation)(this, blend);
	}
	void SetBlend(float blend)
	{
		typedef void(__thiscall* OriginalFn)(PVOID, float blend);
		VMT.getvfunc<OriginalFn>(this, indexes::SetBlend)(this, blend);
	}
};

struct StaticPropRenderInfo_t
{
	const matrix3x4_t		*pModelToWorld;
	const model_t			*pModel;
	IClientRenderable		*pRenderable;
	Vector					*pLightingOrigin;
	short					skin;
	ModelInstanceHandle_t	instance;
};

typedef void* LightCacheHandle_t;
struct DrawModelInfo_t;

enum
{
	ADDDECAL_TO_ALL_LODS = -1
};

class IModelRender
{
public:
	virtual int		DrawModel(int flags,
		IClientRenderable *pRenderable,
		ModelInstanceHandle_t instance,
		int entity_index,
		const model_t *model,
		Vector const& origin,
		QAngle const& angles,
		int skin,
		int body,
		int hitboxset,
		const matrix3x4_t *modelToWorld = NULL,
		const matrix3x4_t *pLightingOffset = NULL) = 0;

	// This causes a material to be used when rendering the model instead 
	// of the materials the model was compiled with
	virtual void	ForcedMaterialOverride(IMaterial *newMaterial, OverrideType_t nOverrideType = OVERRIDE_NORMAL) = 0;

	virtual void	SetViewTarget(const studiohdr_t *pStudioHdr, int nBodyIndex, const Vector& target) = 0;

	// Creates, destroys instance data to be associated with the model
	virtual ModelInstanceHandle_t CreateInstance(IClientRenderable *pRenderable, LightCacheHandle_t *pCache = NULL) = 0;
	virtual void DestroyInstance(ModelInstanceHandle_t handle) = 0;

	// Associates a particular lighting condition with a model instance handle.
	// FIXME: This feature currently only works for static props. To make it work for entities, etc.,
	// we must clean up the lightcache handles as the model instances are removed.
	// At the moment, since only the static prop manager uses this, it cleans up all LightCacheHandles 
	// at level shutdown.
	virtual void SetStaticLighting(ModelInstanceHandle_t handle, LightCacheHandle_t* pHandle) = 0;
	virtual LightCacheHandle_t GetStaticLighting(ModelInstanceHandle_t handle) = 0;

	// moves an existing InstanceHandle to a nex Renderable to keep decals etc. Models must be the same
	virtual bool ChangeInstance(ModelInstanceHandle_t handle, IClientRenderable *pRenderable) = 0;

	// Creates a decal on a model instance by doing a planar projection
	// along the ray. The material is the decal material, the radius is the
	// radius of the decal to create.
	virtual void AddDecal(ModelInstanceHandle_t handle, Ray_t const& ray,
		Vector const& decalUp, int decalIndex, int body, bool noPokeThru = false, int maxLODToDecal = ADDDECAL_TO_ALL_LODS) = 0;

	// Removes all the decals on a model instance
	virtual void RemoveAllDecals(ModelInstanceHandle_t handle) = 0;

	// Remove all decals from all models
	virtual void RemoveAllDecalsFromAllModels() = 0;

	// Shadow rendering, DrawModelShadowSetup returns the address of the bone-to-world array, NULL in case of error
	virtual matrix3x4_t* DrawModelShadowSetup(IClientRenderable *pRenderable, int body, int skin, DrawModelInfo_t *pInfo, matrix3x4_t *pCustomBoneToWorld = NULL) = 0;
	virtual void DrawModelShadow(IClientRenderable *pRenderable, const DrawModelInfo_t &info, matrix3x4_t *pCustomBoneToWorld = NULL) = 0;

	// This gets called when overbright, etc gets changed to recompute static prop lighting.
	virtual bool RecomputeStaticLighting(ModelInstanceHandle_t handle) = 0;

	virtual void ReleaseAllStaticPropColorData(void) = 0;
	virtual void RestoreAllStaticPropColorData(void) = 0;

	// Extended version of drawmodel
	virtual int	DrawModelEx(ModelRenderInfo_t &pInfo) = 0;

	virtual int	DrawModelExStaticProp(ModelRenderInfo_t &pInfo) = 0;

	virtual bool DrawModelSetup(ModelRenderInfo_t &pInfo, DrawModelState_t *pState, matrix3x4_t *pCustomBoneToWorld, matrix3x4_t** ppBoneToWorldOut) = 0;
	virtual void DrawModelExecute(const DrawModelState_t &state, const ModelRenderInfo_t &pInfo, matrix3x4_t *pCustomBoneToWorld = NULL) = 0;

	// Sets up lighting context for a point in space
	virtual void SetupLighting(const Vector &vecCenter) = 0;

	// doesn't support any debug visualization modes or other model options, but draws static props in the
	// fastest way possible
	virtual int DrawStaticPropArrayFast(StaticPropRenderInfo_t *pProps, int count, bool bShadowDepth) = 0;

	// Allow client to override lighting state
	virtual void SuppressEngineLighting(bool bSuppress) = 0;

	virtual void SetupColorMeshes(int nTotalVerts) = 0;

	virtual void SetupLightingEx(Vector const&, unsigned short) = 0;

	virtual void GetBrightestShadowingLightSource(Vector const&, Vector&, Vector&, bool) = 0;

	/*
	virtual void ComputeLightingState(int, LightingQuery_t const*, MaterialLightingState_t*, ITexture**) = 0;

	virtual void GetModelDecalHandles(StudioDecalHandle_t__**, int, int, unsigned short const*) = 0;

	virtual void ComputeStaticLightingState(int, StaticLightingQuery_t const*, MaterialLightingState_t*, MaterialLightingState_t*, ColorMeshInfo_t**, ITexture**, memhandle_t__**) = 0;

	virtual void CleanupStaticLightingState(int, memhandle_t__**) = 0;

	virtual void GetItemName(unsigned int, void const*, char*, unsigned int) = 0;
	*/
};
