#pragma once
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
	unsigned long	friendsID;

	// friends name
	char			friendsName[32];

	// true, if player is a bot controlled by game.dll
	bool			fakeplayer;

	// true if player is the HLTV proxy
	bool			ishltv;

	// custom files CRC for this player
	unsigned long	customFiles[4];

	// this counter increases each time the server downloaded a new file
	unsigned char	filesDownloaded;

	// byte buffer[200];
} player_info_t;
*/

typedef struct player_info_s
{
private:
	int64_t unknown;
public:
	union
	{
		int64_t xuid;

		struct
		{
			int xuidlow;
			int xuidhigh;
		};
	};

	char			name[128];
	int				userid;
	char			guid[33];
	unsigned int	friendsid;
	char			friendsname[128];
	bool			fakeplayer;
	bool			ishltv;
	unsigned int	customfiles[4];
	unsigned char	filesdownloaded;
} player_info_t;

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
