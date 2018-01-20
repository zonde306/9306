#pragma once

typedef void *FileHandle_t;
#define MAX_STREAMS 2
#define MAX_OSPATH 260
#define MAX_FLOWS 2 // in & out

#define SUBCHANNEL_FREE 0	// subchannel is free to use
#define SUBCHANNEL_TOSEND 1  // subchannel has data, but not send yet
#define SUBCHANNEL_WAITING 2 // sbuchannel sent data, waiting for ACK
#define SUBCHANNEL_DIRTY 3   // subchannel is marked as dirty during changelevel

class INetChannelHandler;
class INetMessage;
class IDemoRecorder;

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
	{
		ip[0] = b1; ip[1] = b2; ip[2] = b3; ip[3] = b4;
	};
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
	virtual int GetTotalPackets(int) const = 0;
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

public: // netchan structurs
	typedef struct dataFragments_s
	{
		FileHandle_t file;				// open file handle
		char filename[MAX_OSPATH];		// filename
		char *buffer;					// if NULL it's a file
		unsigned int BYTEs;				// size in BYTEs
		unsigned int bits;				// size in bits
		unsigned int transferID;		// only for files
		bool isCompressed;				// true if data is bzip compressed
		unsigned int nUncompressedSize; // full size in BYTEs
		bool asTCP;						// send as TCP stream
		int numFragments;				// number of total fragments
		int ackedFragments;				// number of fragments send & acknowledged
		int pendingFragments;			// number of fragments send, but not acknowledged yet
	} dataFragments_t;

	struct subChannel_s
	{
		int startFraggment[MAX_STREAMS];
		int numFragments[MAX_STREAMS];
		int sendSeqNr;
		int state; // 0 = free, 1 = scheduled to send, 2 = send & waiting, 3 = dirty
		int index; // index in m_SubChannels[]

		void Free()
		{
			state = SUBCHANNEL_FREE;
			sendSeqNr = -1;
			for (int i = 0; i < MAX_STREAMS; i++)
			{
				numFragments[i] = 0;
				startFraggment[i] = -1;
			}
		}
	};

	// Client's now store the command they sent to the server and the entire results set of
	//  that command.
	typedef struct netframe_s
	{
		// Data received from server
		float time;		   // net_time received/send
		int size;		   // total size in BYTEs
		float latency;	 // raw ping for this packet, not cleaned. set when acknowledged otherwise -1.
		float avg_latency; // averaged ping for this packet
		bool valid;		   // false if dropped, lost, flushed
		int choked;		   // number of previously chocked packets
		int dropped;
		float m_flInterpolationAmount;
		unsigned short msggroups[INetChannelInfo::TOTAL]; // received BYTEs for each message group
	} netframe_t;

	typedef struct
	{
		float nextcompute;		  // Time when we should recompute k/sec data
		float avgBYTEspersec;	 // average BYTEs/sec
		float avgpacketspersec;   // average packets/sec
		float avgloss;			  // average packet loss [0..1]
		float avgchoke;			  // average packet choke [0..1]
		float avglatency;		  // average ping, not cleaned
		float latency;			  // current ping, more accurate also more jittering
		int totalpackets;		  // total processed packets
		int totalBYTEs;			  // total processed BYTEs
		int currentindex;		  // current frame index
		netframe_t frames[64];	// frame history
		netframe_t *currentframe; // current frame
	} netflow_t;

public:
	bool m_bProcessingMessages;
	bool m_bShouldDelete;

	// last send outgoing sequence number
	int m_nOutSequenceNr;
	// last received incoming sequnec number
	int m_nInSequenceNr;
	// last received acknowledge outgoing sequnce number
	int m_nOutSequenceNrAck;

	// state of outgoing reliable data (0/1) flip flop used for loss detection
	int m_nOutReliableState;
	// state of incoming reliable data
	int m_nInReliableState;

	int m_nChokedPackets; //number of choked packets

						  // Reliable data buffer, send which each packet (or put in waiting list)
	bf_write m_StreamReliable;
	CUtlMemory<BYTE> m_ReliableDataBuffer;

	// unreliable message buffer, cleared which each packet
	bf_write m_StreamUnreliable;
	CUtlMemory<BYTE> m_UnreliableDataBuffer;

	bf_write m_StreamVoice;
	CUtlMemory<BYTE> m_VoiceDataBuffer;

	// don't use any vars below this (only in net_ws.cpp)

	int m_Socket;		// NS_SERVER or NS_CLIENT index, depending on channel.
	int m_StreamSocket; // TCP socket handle

	unsigned int m_MaxReliablePayloadSize; // max size of reliable payload in a single packet

										   // Address this channel is talking to.
	netadr_t remote_address;

	// For timeouts.  Time last message was received.
	float last_received;
	// Time when channel was connected.
	double connect_time;

	// Bandwidth choke
	// BYTEs per second
	int m_Rate;
	// If realtime > cleartime, free to send next packet
	double m_fClearTime;

	CUtlVector<dataFragments_t *> m_WaitingList[MAX_STREAMS]; // waiting list for reliable data and file transfer
	dataFragments_t m_ReceiveList[MAX_STREAMS];				  // receive buffers for streams
	subChannel_s m_SubChannels[8];

	unsigned int m_FileRequestCounter; // increasing counter with each file request
	bool m_bFileBackgroundTranmission; // if true, only send 1 fragment per packet
	bool m_bUseCompression;			   // if true, larger reliable data will be bzip compressed

									   // TCP stream state maschine:
	bool m_StreamActive;		   // true if TCP is active
	int m_SteamType;			   // STREAM_CMD_*
	int m_StreamSeqNr;			   // each blob send of TCP as an increasing ID
	int m_StreamLength;			   // total length of current stream blob
	int m_StreamReceived;		   // length of already received BYTEs
	char m_SteamFile[MAX_OSPATH];  // if receiving file, this is it's name
	CUtlMemory<BYTE> m_StreamData; // Here goes the stream data (if not file). Only allocated if we're going to use it.

								   // packet history
	netflow_t m_DataFlow[MAX_FLOWS];
	int m_MsgStats[INetChannelInfo::TOTAL]; // total BYTEs for each message group

	int m_PacketDrop; // packets lost before getting last update (was global net_drop)

	char m_Name[32]; // channel name

	unsigned int m_ChallengeNr; // unique, random challenge number

	float m_Timeout; // in seconds

	INetChannelHandler *m_MessageHandler;	// who registers and processes messages
	CUtlVector<INetMessage *> m_NetMessages; // list of registered message
	IDemoRecorder *m_DemoRecorder;			 // if != NULL points to a recording/playback demo object
	int m_nQueuedPackets;

	float m_flInterpolationAmount;
	float m_flRemoteFrameTime;
	float m_flRemoteFrameTimeStdDeviation;
	int m_nMaxRoutablePayloadSize;

	int m_nSplitPacketSequence;
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

class INetChannel : public INetChannelInfo
{
public:
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
	virtual int GetTotalPackets(int) const = 0;
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

public:
	virtual				~INetChannel() = 0;	// 27
	virtual void		SetDataRate(float rate) = 0;
	virtual bool		RegisterMessage(INetMessage *msg) = 0;
	virtual bool		StartStreaming(unsigned int challengeNr) = 0;
	virtual void		ResetStreaming(void) = 0;
	virtual void		SetTimeout(float seconds) = 0;
	virtual void		SetDemoRecorder(IDemoRecorder *recorder) = 0;
	virtual void		SetChallengeNr(unsigned int chnr) = 0;

	virtual void		Reset(void) = 0;
	virtual void		Clear(void) = 0;
	virtual void		Shutdown(const char * reason) = 0;

	virtual void		ProcessPlayback(void) = 0;	// 38
	virtual bool		ProcessStream(void) = 0;
	virtual void		ProcessPacket(/*netpacket_t*/void* packet, bool bHasHeader) = 0;

	virtual bool		SendNetMsg(INetMessage &msg, bool bForceReliable = false, bool bVoice = false) = 0;	// 41
	virtual bool		SendData(bf_write &msg, bool bReliable = true) = 0;
	virtual bool		SendFile(const char *filename, unsigned int transferID) = 0;
	virtual void		DenyFile(const char *filename, unsigned int transferID, bool) = 0;
	virtual void		RequestFile_OLD(const char *filename, unsigned int transferID) = 0;
	virtual void		SetChoked(void) = 0;
	virtual int			SendDatagram(bf_write *data) = 0;
	virtual bool		Transmit(bool onlyReliable = false) = 0;
	
	virtual const netadr_t	&GetRemoteAddress(void) const = 0;
	virtual INetChannelHandler *GetMsgHandler(void) const = 0;
	virtual int				GetDropNumber(void) const = 0;
	virtual int				GetSocket(void) const = 0;
	virtual unsigned int	GetChallengeNr(void) const = 0;
	virtual void			GetSequenceData(int &nOutSequenceNr, int &nInSequenceNr, int &nOutSequenceNrAck) = 0;
	virtual void			SetSequenceData(int nOutSequenceNr, int nInSequenceNr, int nOutSequenceNrAck) = 0;

	virtual void		UpdateMessageStats(int msggroup, int bits) = 0;
	virtual bool		CanPacket(void) const = 0;
	virtual bool		IsOverflowed(void) const = 0;	// 58
	virtual bool		IsTimedOut(void) const = 0;
	virtual bool		HasPendingReliableData(void) = 0;
	virtual void		SetFileTransmissionMode(bool bBackgroundMode) = 0;
	virtual void		SetCompressionMode(bool bUseCompression) = 0;
	virtual unsigned int RequestFile(const char *filename) = 0;
	virtual void		SetMaxBufferSize(bool bReliable, int nBytes, bool bVoice = false) = 0;
	virtual bool		IsNull() const = 0;
	virtual int		GetNumBitsWritten(bool bReliable) = 0;
	virtual void	SetInterpolationAmount(float flInterpolationAmount) = 0;
	virtual void	SetRemoteFramerate(float flFrameTime, float flFrameTimeStdDeviation) = 0;
	virtual void	SetMaxRoutablePayloadSize(int nSplitSize) = 0;
	virtual int		GetMaxRoutablePayloadSize() = 0;
	virtual void		SetActiveChannel(INetChannel*) = 0;
	virtual void		AttachSplitPlayer(int, INetChannel*) = 0;
	virtual void		DetachSplitPlayer(int) = 0;
	virtual void		IsRemoteDisconnected() const = 0;	// 74
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
class PackedEntity;
class CServerClassInfo;
class CNetworkStringTableContainer;

#define STEAM_KEYSIZE 2048

class CClockDriftMgr
{
	friend class CBaseClientState;

public:
	CClockDriftMgr() {};

	// Is clock correction even enabled right now?
	static bool IsClockCorrectionEnabled();

	// Clear our state.
	void Clear();

	// This is called each time a server packet comes in. It is used to correlate
	// where the server is in time compared to us.
	void SetServerTick(int iServerTick);

	// Pass in the frametime you would use, and it will drift it towards the server clock.
	float AdjustFrameTime(float inputFrameTime);

	// Returns how many ticks ahead of the server the client is.
	float GetCurrentClockDifference() const;

private:
	void ShowDebugInfo(float flAdjustment);

	// This scales the offsets so the average produced is equal to the
	// current average + flAmount. This way, as we add corrections,
	// we lower the average accordingly so we don't keep responding
	// as much as we need to after we'd adjusted it a couple times.
	void AdjustAverageDifferenceBy(float flAmountInSeconds);

private:
	enum
	{
		// This controls how much it smoothes out the samples from the server.
		NUM_CLOCKDRIFT_SAMPLES = 16
	};

	// This holds how many ticks the client is ahead each time we get a server tick.
	// We average these together to get our estimate of how far ahead we are.
	float m_ClockOffsets[NUM_CLOCKDRIFT_SAMPLES];
	int m_iCurClockOffset;

	int m_nServerTick; // Last-received tick from the server.
	int m_nClientTick; // The client's own tick counter (specifically, for interpolation during rendering).
					   // The server may be on a slightly different tick and the client will drift towards it.
};

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

	virtual void FileRequested(const char *fileName, unsigned int transferID) = 0;
	virtual void FileReceived(const char *fileName, unsigned int transferID) = 0;
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

public:
	// Connection to server.
	int m_Socket;				 // network socket
	INetChannel *m_NetChannel;   // Our sequenced channel to the remote server.
	unsigned int m_nChallengeNr; // connection challenge number
	double m_flConnectTime;		 // If gap of connect_time to net_time > 3000, then resend connect packet
	int m_nRetryNumber;			 // number of retry connection attemps
	char m_szRetryAddress[MAX_OSPATH];
	int m_nSignonState;		// see SIGNONSTATE_* definitions
	double m_flNextCmdTime; // When can we send the next command packet?
	int m_nServerCount;		// server identification for prespawns, must match the svs.spawncount which
	
							// is incremented on server spawning.  This supercedes svs.spawn_issued, in that
	// we can now spend a fair amount of time sitting connected to the server
	// but downloading models, sounds, etc.  So much time that it is possible that the
	// server might change levels again and, if so, we need to know that.
	int m_nCurrentSequence; // this is the sequence number of the current incoming packet

	CClockDriftMgr m_ClockDriftMgr;

	int m_nDeltaTick;  //	last valid received snapshot (server) tick
	bool m_bPaused;	// send over by server
	int m_nViewEntity; // cl_entitites[cl.viewentity] == player point of view

	int m_nPlayerSlot; // own player entity index-1. skips world. Add 1 to get cl_entitites index;

	char m_szLevelName[40];		 // for display on solo scoreboard
	char m_szLevelNameShort[40]; // removes maps/ and .bsp extension

	int m_nMaxClients; // max clients on server

	PackedEntity *m_pEntityBaselines[2][MAX_EDICTS]; // storing entity baselines

	// This stuff manages the receiving of data tables and instantiating of client versions
	// of server-side classes.
	CServerClassInfo *m_pServerClasses;
	int m_nServerClasses;
	int m_nServerClassBits;
	char m_szEncrytionKey[STEAM_KEYSIZE];
	unsigned int m_iEncryptionKeySize;

	CNetworkStringTableContainer *m_StringTableContainer;

	bool m_bRestrictServerCommands; // If true, then the server is only allowed to execute commands marked with FCVAR_SERVER_CAN_EXECUTE on the client.
	bool m_bRestrictClientCommands; // If true, then IVEngineClient::ClientCmd is only allowed to execute commands marked with FCVAR_CLIENTCMD_CAN_EXECUTE on the client.
};

#define DECLARE_BASE_MESSAGE(msgtype)           \
public:                                         \
	bool ReadFromBuffer(bf_read &buffer);       \
	bool WriteToBuffer(bf_write &buffer);       \
	const char *ToString() const { return ""; } \
	int GetType() const { return 0; }     \
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
	virtual void SetNetChannel(INetChannel *netchan) { m_NetChannel = netchan; }
	virtual void SetReliable(bool state) { m_bReliable = state; };
	virtual bool Process() { return false; }; // no handler set
	virtual bool ReadFromBuffer(bf_read &buffer) { return false; };
	virtual bool WriteToBuffer(bf_write &buffer) { return false; };
	virtual bool IsReliable() const { return m_bReliable; };
	virtual int GetType(void) const { return 0; };
	virtual int GetGroup() const { return INetChannelInfo::GENERIC; }
	virtual const char *GetName(void) const { return ""; };
	INetChannel *GetNetChannel() const { return m_NetChannel; }

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

class NET_SetConVar : public CNetMessage
{
public:
	DECLARE_NET_MESSAGE(SetConVar);

	int	GetGroup() const { return INetChannelInfo::STRINGCMD; }

	NET_SetConVar() {}
	NET_SetConVar(const char * name, const char * value)
	{
		cvar_t cvar;
		strncpy(cvar.name, name, 260);
		strncpy(cvar.value, value, 260);
		m_ConVars.AddToTail(cvar);
	}

public:

	typedef struct cvar_s
	{
		char	name[260];
		char	value[260];
	} cvar_t;

	CUtlVector<cvar_t> m_ConVars;
};

class SVC_GetCvarValue : public CNetMessage
{
public:
	DECLARE_SVC_MESSAGE(GetCvarValue);

	int					m_iCookie;
	const char			*m_szCvarName;	// The sender sets this, and it automatically points it at m_szCvarNameBuffer when receiving.

private:
	char		m_szCvarNameBuffer[256];
};

class CLC_RespondCvarValue : public CNetMessage
{
public:
	DECLARE_CLC_MESSAGE(RespondCvarValue);

	int						m_iCookie;

	const char				*m_szCvarName;
	const char				*m_szCvarValue;	// The sender sets this, and it automatically points it at m_szCvarNameBuffer when receiving.

	EQueryCvarValueStatus	m_eStatusCode;

private:
	char		m_szCvarNameBuffer[256];
	char		m_szCvarValueBuffer[256];
};

bool CLC_RespondCvarValue::ReadFromBuffer(bf_read& buffer)
{
	return false;
}

bool CLC_RespondCvarValue::WriteToBuffer(bf_write& buffer)
{
	return false;
}

class NET_StringCmd : public CNetMessage
{
	DECLARE_NET_MESSAGE(StringCmd);

	int	GetGroup() const { return INetChannelInfo::STRINGCMD; }

	NET_StringCmd() { m_szCommand = NULL; };
	NET_StringCmd(const char *cmd) { m_szCommand = cmd; };

public:
	const char	*m_szCommand;	// execute this command

private:
	char		m_szCommandBuffer[1024];	// buffer for received messages

};

class CLC_Move : public CNetMessage
{
	DECLARE_CLC_MESSAGE(Move);

	int	GetGroup() const { return INetChannelInfo::MOVE; }

	CLC_Move() { m_bReliable = false; }

public:
	int				m_nBackupCommands;
	int				m_nNewCommands;
	int				m_nLength;
	bf_read			m_DataIn;
	bf_write		m_DataOut;
};

class SVC_Print : public CNetMessage
{
	DECLARE_SVC_MESSAGE(Print);

	SVC_Print() { m_bReliable = false; m_szText = NULL; };

	SVC_Print(const char * text) { m_bReliable = false; m_szText = text; };

public:
	const char	*m_szText;	// show this text

private:
	char		m_szTextBuffer[2048];	// buffer for received messages
};

class SVC_ServerInfo : public CNetMessage
{
	DECLARE_SVC_MESSAGE(ServerInfo);

	int	GetGroup() const { return INetChannelInfo::SIGNON; }

public:	// member vars are public for faster handling
	int			m_nProtocol;	// protocol version
	int			m_nServerCount;	// number of changelevels since server start
	bool		m_bIsDedicated;  // dedicated server ?	
	bool		m_bIsHLTV;		// HLTV server ?
	char		m_cOS;			// L = linux, W = Win32
	CRC32_t		m_nMapCRC;		// server map CRC
	CRC32_t		m_nClientCRC;	// client.dll CRC server is using
	int			m_nMaxClients;	// max number of clients on server
	int			m_nMaxClasses;	// max number of server classes
	int			m_nPlayerSlot;	// our client slot number
	float		m_fTickInterval;// server tick interval
	const char	*m_szGameDir;	// game directory eg "tf2"
	const char	*m_szMapName;	// name of current map 
	const char	*m_szSkyName;	// name of current skybox 
	const char	*m_szHostName;	// server name

private:
	char		m_szGameDirBuffer[256];// game directory eg "left4dead2"
	char		m_szMapNameBuffer[256];// name of current map 
	char		m_szSkyNameBuffer[256];// name of current skybox 
	char		m_szHostNameBuffer[256];// name of current skybox 
};

template<typename T>
class CNetMessagePB : public INetMessage, public T
{
};

class CCLCMsg_Move
{
private:
	char __PAD0[0x8];
public:
	int numBackupCommands;
	int numNewCommands;
};

using CCLCMsg_Move_t = CNetMessagePB<CCLCMsg_Move>;
