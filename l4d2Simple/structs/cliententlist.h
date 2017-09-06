#pragma once
class CClientEntityList
{
public:

	CBaseEntity* GetClientEntity( int Index )
	{
		typedef CBaseEntity*(__thiscall* Fn)(void*, int);
		return ((Fn)VMT.GetFunction(this, indexes::GetClientEntity))(this, Index);
	}

	int GetHighestEntityIndex()
	{
		typedef int(__thiscall* Fn)(void*);
		return ((Fn)VMT.GetFunction(this, indexes::GetHighestEntityIndex))(this);
	}

	CBaseEntity* GetClientEntityFromHandle(CBaseHandle* Handle)
	{
		typedef CBaseEntity*(__thiscall* Fn)(void*, void*);
		return ((Fn)VMT.GetFunction(this, indexes::GetClientEntityFromHandle))(this, Handle);
	}
};