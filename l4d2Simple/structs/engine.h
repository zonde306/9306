#pragma once
class CEngine
{
public:

	void GetScreenSize( int& Width, int& Height )
	{
		typedef void( __thiscall* Fn )(void*, int&, int&);
		return ((Fn)VMT.GetFunction(this, indexes::GetScreenSize))(this, Width, Height);
	}

	bool GetPlayerInfo( int Index, player_info_t* PlayerInfo )	
	{
		typedef bool(__thiscall* Fn)(void*, int, player_info_t*);
		return ((Fn)VMT.GetFunction(this, indexes::GetPlayerInfo))(this, Index, PlayerInfo);
	}

	int GetLocalPlayer()
	{
		typedef int(__thiscall* Fn)(void*);
		return ((Fn)VMT.GetFunction(this, indexes::GetLocalPlayer))(this);
	}

	void ClientCmd(const char* Command, ...)
	{
		char cmd[1024];

		va_list argp;
		va_start(argp, Command);
		vsprintf_s(cmd, Command, argp);
		va_end(argp);
		
		typedef void(__thiscall* Fn)(void*, const char*);
		return ((Fn)VMT.GetFunction(this, indexes::ClientCmd))(this, cmd);
	}

	void SetViewAngles(Vector& Angles)
	{
		typedef void(__thiscall* Fn)(void*, Vector&);
		return ((Fn)VMT.GetFunction(this, indexes::SetViewAngles))(this, Angles);
	}

	void GetViewAngles(Vector& angle)
	{
		typedef void(__thiscall* Fn)(void*, Vector&);
		return ((Fn)VMT.GetFunction(this, indexes::GetViewAngles))(this,angle);
	}

	VMatrix& WorldToScreenMatrix()
	{
		typedef VMatrix& (__thiscall* Fn)(void*);
		return ((Fn)VMT.GetFunction(this, indexes::WorldToScreenMatrix))(this);
	
	}

	bool IsConsoleVisible()
	{
		typedef bool(__thiscall* Fn)(void*);
		return ((Fn)VMT.GetFunction(this, indexes::Con_IsVisible))(this);
	}

	float GetTime()
	{
		typedef float(__thiscall* Fn)(void*);
		return ((Fn)VMT.GetFunction(this, indexes::Time))(this);
	}

	int GetMaxClients()
	{
		typedef int(__thiscall* Fn)(void*);
		return ((Fn)VMT.GetFunction(this, indexes::GetMaxClients))(this);
	}

	bool IsInGame()
	{
		typedef bool(__thiscall* Fn)(void*);
		return ((Fn)VMT.GetFunction(this, indexes::IsInGame))(this);
	}

	bool IsConnected()
	{
		typedef bool(__thiscall* Fn)(void*);
		return ((Fn)VMT.GetFunction(this, indexes::IsConnected))(this);
	}

	bool IsDrawingLoadingImage()
	{
		typedef bool(__thiscall* Fn)(void*);
		return ((Fn)VMT.GetFunction(this, indexes::IsDrawingLoadingImage))(this);
	}

	bool IsTakingScreenshot()
	{
		typedef bool(__thiscall* Fn)(void*);
		return ((Fn)VMT.GetFunction(this, indexes::IsTakingScreenShot))(this);
	}

	int GetPlayerForUserID(int userID)
	{
		typedef bool(__thiscall* Fn)(void*, int);
		return ((Fn)VMT.GetFunction(this, indexes::GetPlayerForUserId))(this, userID);
	}
};