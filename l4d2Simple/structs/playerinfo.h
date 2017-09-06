#pragma once

class player_info_t
{
public:
	byte __pad00[0x08];		// 0x00
	char name[32];			// 0x08
	int32_t userId;			// 0x28
	char steamId[33];		// 0x2C
	byte __pad01[0x27];		// 0x4D
	bool isBot;				// 0x74
	byte __pad02[0x1B];		// 0x75
};	// sizeof 0x90

/*
typedef struct player_info_s
{
	// scoreboard information
	char			name[32];
	// local server user ID, unique while server is running
	int				userID;
	// global unique player identifer
	char			guid[33];
	// friends identification number
	uint32_t			friendsID;
	// friends name
	char			friendsName[32];
	// true, if player is a bot controlled by game.dll
	bool			fakeplayer;
	// true if player is the HLTV proxy
	bool			ishltv;
#if defined( REPLAY_ENABLED )
	// true if player is the Replay proxy
	bool			isreplay;
#endif
	// custom files CRC for this player
	unsigned long	customFiles[4];
	// this counter increases each time the server downloaded a new file
	unsigned char	filesDownloaded;
} player_info_t;
*/

/*
typedef struct player_info_s
{
private:
	int64_t unknown;						// 0x00

public:
	union									// 0x08 SteamID64
	{
		int64_t steamId64;

		struct
		{
			int32_t steamIdLow;
			int32_t steamIdHigh;
		};
	};

	char				name[128];				// 0x10 Player Name
	int32_t				userid;					// 0x90 Unique Server Identifier
	char				steamId[33];			// 0x94 SteamID2 (STEAM_\d:\d:\d+)
	uint32_t			friendsid;				// 0xB5
	char				friendsname[128];		// 0xB9
	bool				fakeplayer;				// 0x139
	bool				ishltv;					// 0x13A
	uint32_t			customfiles[4];			// 0x13B
	unsigned char		filesdownloaded;		// 0x147
} player_info_t;
*/

/*
struct player_info_t
{
private:
	char __pad[0x8];

public:
	int32_t					xuidlow;
	int32_t					xuidhigh;
	char					name[0x80];
	int32_t					userid;
	char					steamid[0x21];
	uint32_t				steam3id;
	char					friendsname[0x80];
	bool					fakeplayer;
	bool					ishltv;
	std::array<uint32_t, 4>	m_cCustomFiles;
	byte					m_FilesDownloaded;
private:
	char					___pad[0x4];
};
*/
