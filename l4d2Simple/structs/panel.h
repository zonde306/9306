#pragma once

enum ClientFrameStage_t
{
	FRAME_UNDEFINED = -1,
	FRAME_START,
	FRAME_NET_UPDATE_START,
	FRAME_NET_UPDATE_POSTDATAUPDATE_START,
	FRAME_NET_UPDATE_POSTDATAUPDATE_END,
	FRAME_NET_UPDATE_END,
	FRAME_RENDER_START,
	FRAME_RENDER_END
};

class CPanel
{
public:
	const char* GetName( unsigned int Panel )
	{
		typedef const char*(__thiscall* Fn)(void*, unsigned int);
		return ((Fn)VMT.GetFunction(this, indexes::GetName))(this, Panel );
	}

};