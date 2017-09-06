#pragma once


class INetChannelInfo
{
public:
	enum
	{
		GENERIC = 0,  // must be first and is default group
		LOCALPLAYER,  // BYTEs for local player entity update
		OTHERPLAYERS, // BYTEs for other players update
		ENTITIES,	 // all other entity BYTEs
		SOUNDS,		  // game sounds
		EVENTS,		  // event messages
		USERMESSAGES, // user messages
		ENTMESSAGES,  // entity messages
		VOICE,		  // voice data
		STRINGTABLE,  // a stringtable update
		MOVE,		  // client move cmds
		STRINGCMD,	// string command
		SIGNON,		  // various signondata
		TOTAL,		  // must be last and is not a real group
	};

	virtual const char *GetName(void) const = 0;	// get channel name
	virtual const char *GetAddress(void) const = 0; // get channel IP address as string
	virtual float GetTime(void) const = 0;			// current net time
	virtual float GetTimeConnected(void) const = 0; // get connection time in seconds
	virtual int GetBufferSize(void) const = 0;		// netchannel packet history size
	virtual int GetDataRate(void) const = 0;		// send data rate in BYTE/sec

	virtual bool IsLoopback(void) const = 0;  // true if loopback channel
	virtual bool IsTimingOut(void) const = 0; // true if timing out
	virtual bool IsPlayback(void) const = 0;  // true if demo playback

	virtual float GetLatency(int flow) const = 0;								   // current latency (RTT), more accurate but jittering
	virtual float GetAvgLatency(int flow) const = 0;							   // average packet latency in seconds
	virtual float GetAvgLoss(int flow) const = 0;								   // avg packet loss[0..1]
	virtual float GetAvgChoke(int flow) const = 0;								   // avg packet choke[0..1]
	virtual float GetAvgData(int flow) const = 0;								   // data flow in BYTEs/sec
	virtual float GetAvgPackets(int flow) const = 0;							   // avg packets/sec
	virtual int GetTotalData(int flow) const = 0;								   // total flow in/out in BYTEs
	virtual int GetSequenceNr(int flow) const = 0;								   // last send seq number
	virtual bool IsValidPacket(int flow, int frame_number) const = 0;			   // true if packet was not lost/dropped/chocked/flushed
	virtual float GetPacketTime(int flow, int frame_number) const = 0;			   // time when packet was send
	virtual int GetPacketBYTEs(int flow, int frame_number, int group) const = 0;   // group size of this packet
	virtual bool GetStreamProgress(int flow, int *received, int *total) const = 0; // TCP progress if transmitting
	virtual float GetTimeSinceLastReceived(void) const = 0;						   // get time since last recieved packet in seconds
	virtual float GetCommandInterpolationAmount(int flow, int frame_number) const = 0;
	virtual void GetPacketResponseLatency(int flow, int frame_number, int *pnLatencyMsecs, int *pnChoke) const = 0;
	virtual void GetRemoteFramerate(float *pflFrameTime, float *pflFrameTimeStdDeviation) const = 0;

	virtual float GetTimeoutSeconds() const = 0;
};

class INetChannel;
class INetChannelHandler
{
public:
	virtual ~INetChannelHandler(void) {};

	virtual void ConnectionStart(INetChannel *chan) = 0; // called first time network channel is established

	virtual void ConnectionClosing(const char *reason) = 0; // network channel is being closed by remote site

	virtual void ConnectionCrashed(const char *reason) = 0; // network error occured

	virtual void PacketStart(int incoming_sequence, int outgoing_acknowledged) = 0; // called each time a new packet arrived

	virtual void PacketEnd(void) = 0; // all messages has been parsed

	virtual void FileRequested(const char *fileName, unsigned int transferID) = 0; // other side request a file for download

	virtual void FileReceived(const char *fileName, unsigned int transferID) = 0; // we received a file

	virtual void FileDenied(const char *fileName, unsigned int transferID) = 0; // a file request was denied by other side

	virtual void FileSent(const char *fileName, unsigned int transferID) = 0; // we sent a file
};

class INetMessage
{
public:
	virtual ~INetMessage() {};

	// Use these to setup who can hear whose voice.
	// Pass in client indices (which are their ent indices - 1).

	virtual void SetNetChannel(INetChannel *netchan) = 0; // netchannel this message is from/for
	virtual void SetReliable(bool state) = 0;			  // set to true if it's a reliable message

	virtual bool Process(void) = 0; // calles the recently set handler to process this message

	virtual bool ReadFromBuffer(bf_read &buffer) = 0; // returns true if parsing was OK
	virtual bool WriteToBuffer(bf_write &buffer) = 0; // returns true if writing was OK

	virtual bool IsReliable(void) const = 0; // true, if message needs reliable handling

	virtual int GetType(void) const = 0;		 // returns module specific header tag eg svc_serverinfo
	virtual int GetGroup(void) const = 0;		 // returns net message group of this message
	virtual const char *GetName(void) const = 0; // returns network message name, eg "svc_serverinfo"
	virtual INetChannel *GetNetChannel(void) const = 0;
	virtual const char *ToString(void) const = 0; // returns a human readable string about message content
};

class IDemoRecorder
{
public:
	~IDemoRecorder() {}

	virtual int *GetDemoFile() = 0;
	virtual int GetRecordingTick(void) = 0;

	virtual void StartRecording(const char *filename, bool bContinuously) = 0;
	virtual void SetSignonState(int state) = 0;
	virtual bool IsRecording(void) = 0;
	virtual void PauseRecording(void) = 0;
	virtual void ResumeRecording(void) = 0;
	virtual void StopRecording(void) = 0;

	virtual void RecordCommand(const char *cmdstring) = 0;		 // record a console command
	virtual void RecordUserInput(int cmdnumber) = 0;			 // record a user input command
	virtual void RecordMessages(bf_read &data, int bits) = 0;	// add messages to current packet
	virtual void RecordPacket(void) = 0;						 // packet finished, write all recorded stuff to file
	virtual void RecordServerClasses(int *pClasses) = 0; // packet finished, write all recorded stuff to file

	virtual void ResetDemoInterpolation() = 0;
};

#undef SetPort
typedef struct netadr_s
{
public:
	typedef enum
	{
		NA_NULL = 0,
		NA_LOOPBACK,
		NA_BROADCAST,
		NA_IP,
	} netadrtype_t;

	netadr_s()
	{
		SetIP(0);
		SetPort(0);
		SetType(NA_IP);
	}
	netadr_s(unsigned int unIP, uint16_t usPort)
	{
		SetIP(unIP);
		SetPort(usPort);
		SetType(NA_IP);
	}
	netadr_s(const char *pch) { SetFromString(pch); }
	void Clear() { ZeroMemory(ip, 4); }; // invalids Address

	void SetType(netadrtype_t type) { this->type = type; };
	void SetPort(unsigned short port) { this->port = port; };
	bool SetFromSockadr(const struct sockaddr *s);
	void SetIP(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4)
	{ ip[0] = b1; ip[1] = b2; ip[2] = b3; ip[3] = b4; };
	void SetIP(unsigned int unIP) // Sets IP.  unIP is in host order (little-endian)
	{
		ip[0] = ((unIP & 0xFF000000) >> 24);
		ip[1] = ((unIP & 0xFF0000) >> 16);
		ip[2] = ((unIP & 0xFF00) >> 8);
		ip[3] = (unIP & 0xFF);
	};
	void SetIPAndPort(unsigned int unIP, unsigned short usPort)
	{
		SetIP(unIP);
		SetPort(usPort);
	}
	void SetFromString(const char *pch, bool bUseDNS = false) // if bUseDNS is true then do a DNS lookup if needed
	{
		std::vector<std::string> addr = Utils::split(pch, ".");
		if (addr.size() < 4)
			return;

		if (addr[3].find(":") != std::string::npos)
		{
			addr.push_back(addr[3].substr(addr[3].find(":")));
			addr[3] = addr[3].substr(0, addr[3].find(":") - 1);
			port = atoi(addr[4].c_str());
		}

		ip[0] = atoi(addr[0].c_str());
		ip[1] = atoi(addr[1].c_str());
		ip[2] = atoi(addr[2].c_str());
		ip[3] = atoi(addr[3].c_str());
	};

	bool CompareAdr(const netadr_s &a, bool onlyBase = false) const
	{
		return (memcmp(ip, a.ip, 4) == 0 && port == a.port && type == a.type);
	};
	bool CompareClassBAdr(const netadr_s &a) const;
	bool CompareClassCAdr(const netadr_s &a) const;

	netadrtype_t GetType() const { return type; };
	unsigned short GetPort() const { return port; };
	const char *ToString(bool onlyBase = false) const // returns xxx.xxx.xxx.xxx:ppppp
	{
		std::stringstream ss;
		ss << ip[0] << "." << ip[1] << "." << ip[2] << "." << ip[3] << ":" << port;
		return ss.str().c_str();
	};
	void ToSockadr(struct sockaddr *s) const;
	unsigned int GetIP() const { return ((ip[0] << 24) | (ip[1] << 16) | (ip[2] << 8) | (ip[3])); };

	bool IsLocalhost() const { return (ip[0] == 127 && ip[1] == 0 && ip[2] == 0 && ip[3] == 1); };   // true, if this is the localhost IP
	bool IsLoopback() const { return (type == NA_LOOPBACK); };	// true if engine loopback buffers are used
	bool IsReservedAdr() const { return (ip[0] == 192 && ip[1] == 168); }; // true, if this is a private LAN IP
	bool IsValid() const { return (GetIP() > 0 && port > 0); };		// ip & port != 0
	void SetFromSocket(int hSocket);
	// These function names are decorated because the Xbox360 defines macros for ntohl and htonl
	unsigned long addr_ntohl() const;
	unsigned long addr_htonl() const;
	bool operator==(const netadr_s &netadr) const { return (CompareAdr(netadr)); }
	bool operator<(const netadr_s &netadr) const;

public: // members are public to avoid to much changes
	netadrtype_t type;
	unsigned char ip[4];
	unsigned short port;
} netadr_t;

class INetChannel : public INetChannelInfo
{
public:
	virtual ~INetChannel(void) {};

	virtual void SetDataRate(float rate) = 0;
	virtual bool RegisterMessage(INetMessage *msg) = 0;
	virtual bool StartStreaming(unsigned int challengeNr) = 0;
	virtual void ResetStreaming(void) = 0;
	virtual void SetTimeout(float seconds) = 0;
	virtual void SetDemoRecorder(IDemoRecorder *recorder) = 0;
	virtual void SetChallengeNr(unsigned int chnr) = 0;

	virtual void Reset(void) = 0;
	virtual void Clear(void) = 0;
	virtual void Shutdown(const char *reason) = 0;

	virtual void ProcessPlayback(void) = 0;
	virtual bool ProcessStream(void) = 0;
	virtual void ProcessPacket(struct netpacket_s *packet, bool bHasHeader) = 0;

	virtual bool SendNetMsg(INetMessage &msg, bool bForceReliable = false, bool bVoice = false) = 0;
#ifdef POSIX
	FORCEINLINE bool SendNetMsg(INetMessage const &msg, bool bForceReliable = false, bool bVoice = false)
	{
		return SendNetMsg(*((INetMessage *)&msg), bForceReliable, bVoice);
	}
#endif
	virtual bool SendData(bf_write &msg, bool bReliable = true) = 0;
	virtual bool SendFile(const char *filename, unsigned int transferID) = 0;
	virtual void DenyFile(const char *filename, unsigned int transferID) = 0;
	virtual void RequestFile_OLD(const char *filename, unsigned int transferID) = 0; // get rid of this function when we version the
	virtual void SetChoked(void) = 0;
	virtual int SendDatagram(bf_write *data) = 0;
	virtual bool Transmit(bool onlyReliable = false) = 0;

	virtual const netadr_t &GetRemoteAddress(void) const = 0;
	virtual INetChannelHandler *GetMsgHandler(void) const = 0;
	virtual int GetDropNumber(void) const = 0;
	virtual int GetSocket(void) const = 0;
	virtual unsigned int GetChallengeNr(void) const = 0;
	virtual void GetSequenceData(int &nOutSequenceNr, int &nInSequenceNr, int &nOutSequenceNrAck) = 0;
	virtual void SetSequenceData(int nOutSequenceNr, int nInSequenceNr, int nOutSequenceNrAck) = 0;

	virtual void UpdateMessageStats(int msggroup, int bits) = 0;
	virtual bool CanPacket(void) const = 0;
	virtual bool IsOverflowed(void) const = 0;
	virtual bool IsTimedOut(void) const = 0;
	virtual bool HasPendingReliableData(void) = 0;

	virtual void SetFileTransmissionMode(bool bBackgroundMode) = 0;
	virtual void SetCompressionMode(bool bUseCompression) = 0;
	virtual unsigned int RequestFile(const char *filename) = 0;
	virtual float GetTimeSinceLastReceived(void) const = 0; // get time since last received packet in seconds

	virtual void SetMaxBufferSize(bool bReliable, int nBYTEs, bool bVoice = false) = 0;

	virtual bool IsNull() const = 0;
	virtual int GetNumBitsWritten(bool bReliable) = 0;
	virtual void SetInterpolationAmount(float flInterpolationAmount) = 0;
	virtual void SetRemoteFramerate(float flFrameTime, float flFrameTimeStdDeviation) = 0;

	// Max # of payload BYTEs before we must split/fragment the packet
	virtual void SetMaxRoutablePayloadSize(int nSplitSize) = 0;
	virtual int GetMaxRoutablePayloadSize() = 0;

	virtual int GetProtocolVersion() = 0;
};

class NET_Tick;
class NET_StringCmd;
class NET_SetConVar;
class NET_SignonState;
class NET_SplitScreenUser;
class SVC_Print;
class SVC_ServerInfo;
class SVC_SendTable;
class SVC_ClassInfo;
class SVC_SetPause;
class SVC_CreateStringTable;
class SVC_UpdateStringTable;
class SVC_SetView;
class SVC_PacketEntities;
class SVC_Menu;
class SVC_GameEventList;
class SVC_GetCvarValue;
class SVC_SplitScreen;
class SVC_CmdKeyValues;

class CBaseClientState
{
public:
	virtual ~CBaseClientState() = 0;

public:		// INetMsgHandler
	virtual void ConnectionStart(INetChannel *chan) = 0;
	virtual void ConnectionClosing(const char *reason) = 0;
	virtual void ConnectionCrashed(const char *reason) = 0;

	virtual void PacketStart(int incoming_sequence, int outgoing_acknowledged) {};
	virtual void PacketEnd(void) {};

	virtual void FileReceived(const char *fileName, unsigned int transferID) = 0;
	virtual void FileRequested(const char *fileName, unsigned int transferID) = 0;
	virtual void FileDenied(const char *fileName, unsigned int transferID) = 0;
	virtual void FileSent(char const*, unsigned int, bool) = 0;

public:		// IConnectionlessPacketHandler
	virtual bool ProcessConnectionlessPacket(struct netpacket_s *packet) = 0;

public:		// IServerMessageHandlers
	virtual bool ProcessTick(NET_Tick*) = 0;
	virtual bool ProcessStringCmd(NET_StringCmd*) = 0;
	virtual bool ProcessSetConVar(NET_SetConVar*) = 0;
	virtual bool ProcessSignonState(NET_SignonState*) = 0;
	virtual bool ProcessSplitScreenUser(NET_SplitScreenUser*) = 0;

	virtual bool ProcessPrint(SVC_Print*) = 0;
	virtual bool ProcessServerInfo(SVC_ServerInfo*) = 0;
	virtual bool ProcessSendTable(SVC_SendTable*) = 0;
	virtual bool ProcessClassInfo(SVC_ClassInfo*) = 0;
	virtual bool ProcessSetPause(SVC_SetPause*) = 0;
	virtual bool ProcessCreateStringTable(SVC_CreateStringTable*) = 0;
	virtual bool ProcessUpdateStringTable(SVC_UpdateStringTable*) = 0;
	virtual bool ProcessSetView(SVC_SetView*) = 0;
	virtual bool ProcessPacketEntities(SVC_PacketEntities*) = 0;
	virtual bool ProcessMenu(SVC_Menu*) = 0;
	virtual bool ProcessGameEventList(SVC_GameEventList*) = 0;
	virtual bool ProcessGetCvarValue(SVC_GetCvarValue*) = 0;
	virtual bool ProcessSplitScreen(SVC_SplitScreen*) = 0;
	virtual bool ProcessCmdKeyValues(SVC_CmdKeyValues*) = 0;

public:
	virtual void OnEvent(KeyValues*) = 0;
	virtual void Clear() = 0;
	virtual void FullConnect(netadr_t &adr) = 0; // a connection was established
	virtual void Connect(const char* adr, char const*) = 0; // start a connection challenge
	virtual void ConnectSplitScreen(const char* adr, char const*) = 0; // start a connection challenge
	virtual bool SetSignonState(int state, int count, NET_SignonState*) = 0;
	virtual void Disconnect(bool bShowMainMenu = true) = 0;
	virtual void SendConnectPacket(netadr_s const&, int, int, unsigned long long, bool) = 0;
	virtual const char *GetCDKeyHash() { return "123"; }
	virtual void RunFrame(void) = 0;
	virtual void CheckForResend(void) = 0;
	virtual void CheckForReservationResend() = 0;
	virtual void ResendGameDetailsRequest(netadr_s&) = 0;
	virtual void InstallStringTableCallback(char const *tableName) { }
	virtual bool HookClientStringTable(char const *tableName) { return false; }
	virtual bool LinkClasses(void) = 0;
	virtual int  GetConnectionRetryNumber() const { return 0; }
	virtual const char *GetClientName() { return ""; }
	virtual void ReserveServer(netadr_s const&, netadr_s const&, unsigned long long, KeyValues*, void*, void**) = 0;
	virtual void HandleReservationResponse(netadr_s&, bool) = 0;
	virtual void HandleReserveServerChallengeResponse(int) = 0;
	virtual void SetServerReservationCookie(unsigned long long) = 0;
};

#define DECLARE_BASE_MESSAGE(msgtype)           \
public:                                         \
	bool ReadFromBuffer(bf_read &buffer);       \
	bool WriteToBuffer(bf_write &buffer);       \
	const char *ToString() const { return ""; } \
	int GetType() const { return msgtype; }     \
	const char *GetName() const { return #msgtype; }

#define DECLARE_NET_MESSAGE(name)     \
	DECLARE_BASE_MESSAGE(NET_##name); \
	void *m_pMessageHandler;          \
	bool Process() { return true; }

#define DECLARE_SVC_MESSAGE(name)     \
	DECLARE_BASE_MESSAGE(SVC_##name); \
	void *m_pMessageHandler;          \
	bool Process() { return true; }

#define DECLARE_CLC_MESSAGE(name)     \
	DECLARE_BASE_MESSAGE(CLC_##name); \
	void *m_pMessageHandler;          \
	bool Process() { return true; }

#define DECLARE_MM_MESSAGE(name)     \
	DECLARE_BASE_MESSAGE(MM_##name); \
	void *m_pMessageHandler;         \
	bool Process() { return true; }

class CNetMessage : public INetMessage
{
public:
	CNetMessage()
	{
		m_bReliable = true;
		m_NetChannel = NULL;
	}

	virtual ~CNetMessage() {};

	virtual int GetGroup() const { return INetChannelInfo::GENERIC; }
	INetChannel *GetNetChannel() const { return m_NetChannel; }

	virtual void SetReliable(bool state) { m_bReliable = state; };
	virtual bool IsReliable() const { return m_bReliable; };
	virtual void SetNetChannel(INetChannel *netchan) { m_NetChannel = netchan; }
	virtual bool Process()
	{
		Assert(0);
		return false;
	}; // no handler set

protected:
	bool m_bReliable;		   // true if message should be send reliable
	INetChannel *m_NetChannel; // netchannel this message is from/for
};

typedef enum
{
	eQueryCvarValueStatus_ValueIntact = 0, // It got the value fine.
	eQueryCvarValueStatus_CvarNotFound = 1,
	eQueryCvarValueStatus_NotACvar = 2, // There's a ConCommand, but it's not a ConVar.
	eQueryCvarValueStatus_CvarProtected = 3  // The cvar was marked with FCVAR_SERVER_CAN_NOT_QUERY, so the server is not allowed to have its value.
} EQueryCvarValueStatus;

class NET_SetConVar
{
public:
	NET_SetConVar(const char *name, const char *value);

private:
	int32_t pad0[9];
};

class SVC_GetCvarValue : public CNetMessage
{
public:
	bool ReadFromBuffer(bf_read &buffer);
	bool WriteToBuffer(bf_write &buffer);
	const char *ToString() const { return ""; }
	int GetType() const { return 1; }
	const char *GetName() const { return "SVC_GetCvarValue"; }

	void *m_pMessageHandler;
	bool Process() { return true; }

	int m_iCookie;
	const char *m_szCvarName; // The sender sets this, and it automatically points it at m_szCvarNameBuffer when receiving.

private:
	char m_szCvarNameBuffer[256];
};

class CLC_RespondCvarValue : public CNetMessage
{
public:
	bool ReadFromBuffer(bf_read &buffer);
	bool WriteToBuffer(bf_write &buffer);
	const char *ToString() const { return ""; }
	int GetType() const { return 2; }
	const char *GetName() const { return "CLC_RespondCvarValue"; }

	void *m_pMessageHandler;
	bool Process() { return true; }

	int m_iCookie;

	const char *m_szCvarName;
	const char *m_szCvarValue; // The sender sets this, and it automatically points it at m_szCvarNameBuffer when receiving.

	EQueryCvarValueStatus m_eStatusCode;

private:
	char m_szCvarNameBuffer[256];
	char m_szCvarValueBuffer[256];
};
