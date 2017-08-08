#pragma once
typedef enum
{
	DPT_Int = 0,
	DPT_Float,
	DPT_Vector,
	DPT_String,
	DPT_Array,	// An array of the base types (can't be of datatables).
	DPT_DataTable,
#if 0 // We can't ship this since it changes the size of DTVariant to be 20 bytes instead of 16 and that breaks MODs!!!
	DPT_Quaternion,
#endif
	DPT_NUMSendPropTypes
} SendPropType;

class DVariant
{
public:
	DVariant()				{ m_Type = DPT_Float; }
	DVariant(float val)		{ m_Type = DPT_Float; m_Float = val; }

	const char *ToString()
	{
		static char text[128];

		switch (m_Type)
		{
		case DPT_Int:
			sprintf_s(text, "%i", m_Int);
			break;
		case DPT_Float:
			sprintf_s(text,  "%.3f", m_Float);
			break;
		case DPT_Vector:
			sprintf_s(text,  "(%.3f,%.3f,%.3f)",
				m_Vector[0], m_Vector[1], m_Vector[2]);
			break;
#if 0 // We can't ship this since it changes the size of DTVariant to be 20 bytes instead of 16 and that breaks MODs!!!
		case DPT_Quaternion:
			Q_snprintf(text, sizeof(text), "(%.3f,%.3f,%.3f %.3f)",
				m_Vector[0], m_Vector[1], m_Vector[2], m_Vector[3]);
			break;
#endif
		case DPT_String:
			if (m_pString)
				return m_pString;
			else
				return "NULL";
			break;
		case DPT_Array:
			sprintf_s(text,  "Array");
			break;
		case DPT_DataTable:
			sprintf_s(text, "DataTable");
			break;
		default:
			sprintf_s(text,  "DVariant type %i unknown", m_Type);
			break;
		}

		return text;
	}

	union
	{
		float	m_Float;
		long	m_Int;
		char	*m_pString;
		void	*m_pData;	// For DataTables.
#if 0 // We can't ship this since it changes the size of DTVariant to be 20 bytes instead of 16 and that breaks MODs!!!
		float	m_Vector[4];
#else
		float	m_Vector[3];
#endif
	};
	SendPropType	m_Type;
};

struct RecvProp;
struct RecvTable;

class CRecvProxyData
{
public:
	const RecvProp	*m_pRecvProp;		// The property it's receiving.

	DVariant		m_Value;			// The value given to you to store.

	int				m_iElement;			// Which array element you're getting.

	int				m_ObjectID;			// The object being referred to.
};

typedef void(*RecvVarProxyFn)(const CRecvProxyData *pData, void *pStruct, void *pOut);
struct RecvProp {
	char *m_pVarName;
	int m_RecvType;
	int m_Flags;
	int m_StringBufferSize;
	bool m_bInsideArray;
	const void *m_pExtraData;
	RecvProp *m_pArrayProp;
	void *m_ArrayLengthProxy;
	RecvVarProxyFn m_ProxyFn;
	void *m_DataTableProxyFn;
	RecvTable *m_pDataTable;
	int m_Offset;
	int m_ElementStride;
	int m_nElements;
	const char *m_pParentArrayPropName;

	inline int GetOffset() const
	{
		return m_Offset;
	}
};

struct RecvTable {
	RecvProp *m_pProps;
	int m_nProps;
	void *m_pDecoder;
	char *m_pNetTableName;
	char _padding[2];

	inline const char* GetName()
	{
		return m_pNetTableName;
	}
	inline int GetNumProps()
	{
		return m_nProps;
	}
	inline RecvProp* GetProp(int i)
	{
		return &m_pProps[i];
	}

};

struct ClientClass
{
	char _padding[ 8 ];
	char *m_pNetworkName;
	RecvTable *m_pRecvTable;
	ClientClass	*m_pNext;
	int m_ClassID;
};

class CClient
{
public:
	ClientClass* GetAllClasses()
	{
		typedef ClientClass*(__thiscall* fn)(void*);
		return ((fn)VMT.GetFunction(this, indexes::GetAllClasses))(this);
	}

	bool IsKeyDown(const char* name, bool& isdown)
	{
		typedef bool(__stdcall* Fn)(void*, const char*, bool&);
		return ((Fn)VMT.GetFunction(this, indexes::IN_IsKeyDown))(this, name, isdown);
	}

};

/*
class CBaseClientState
{
public:
	virtual ~CBaseClientState() = 0;

public:		// INetMsgHandler
	virtual void ConnectionStart(INetChannel *chan);
	virtual void ConnectionClosing(const char *reason);
	virtual void ConnectionCrashed(const char *reason);

	virtual void PacketStart(int incoming_sequence, int outgoing_acknowledged) {};
	virtual void PacketEnd(void) {};

	virtual void FileReceived(const char *fileName, unsigned int transferID);
	virtual void FileRequested(const char *fileName, unsigned int transferID);
	virtual void FileDenied(const char *fileName, unsigned int transferID);
	virtual void FileSent(char const*, unsigned int, bool);

public:		// IConnectionlessPacketHandler
	virtual bool ProcessConnectionlessPacket(struct netpacket_s *packet);

public:		// IServerMessageHandlers
	virtual bool ProcessTick(NET_Tick*);
	virtual bool ProcessStringCmd(NET_StringCmd*);
	virtual bool ProcessSetConVar(NET_SetConVar*);
	virtual bool ProcessSignonState(NET_SignonState*);
	virtual bool ProcessSplitScreenUser(NET_SplitScreenUser*);

	virtual bool ProcessPrint(SVC_Print*);
	virtual bool ProcessServerInfo(SVC_ServerInfo*);
	virtual bool ProcessSendTable(SVC_SendTable*);
	virtual bool ProcessClassInfo(SVC_ClassInfo*);
	virtual bool ProcessSetPause(SVC_SetPause*);
	virtual bool ProcessCreateStringTable(SVC_CreateStringTable*);
	virtual bool ProcessUpdateStringTable(SVC_UpdateStringTable*);
	virtual bool ProcessSetView(SVC_SetView*);
	virtual bool ProcessPacketEntities(SVC_PacketEntities*);
	virtual bool ProcessMenu(SVC_Menu*);
	virtual bool ProcessGameEventList(SVC_GameEventList*);
	virtual bool ProcessGetCvarValue(SVC_GetCvarValue*);
	virtual bool ProcessSplitScreen(SVC_SplitScreen*);
	virtual bool ProcessCmdKeyValues(SVC_CmdKeyValues*);

public:
	virtual void OnEvent(KeyValues*);
	virtual void Clear();
	virtual void FullConnect(netadr_t &adr); // a connection was established
	virtual void Connect(const char* adr, char const*); // start a connection challenge
	virtual void ConnectSplitScreen(const char* adr, char const*); // start a connection challenge
	virtual bool SetSignonState(int state, int count, NET_SignonState*);
	virtual void Disconnect(bool bShowMainMenu = true);
	virtual void SendConnectPacket(netadr_s const&, int, int, unsigned long long, bool);
	virtual const char *GetCDKeyHash() { return "123"; }
	virtual void RunFrame(void);
	virtual void CheckForResend(void);
	virtual void CheckForReservationResend();
	virtual void ResendGameDetailsRequest(netadr_s&);
	virtual void InstallStringTableCallback(char const *tableName) { }
	virtual bool HookClientStringTable(char const *tableName) { return false; }
	virtual bool LinkClasses(void);
	virtual int  GetConnectionRetryNumber() const { return 0; }
	virtual const char *GetClientName() { return ""; }
	virtual void ReserveServer(netadr_s const&, netadr_s const&, unsigned long long, KeyValues*, IMatchAsyncOperationCallback*, IMatchAsyncOperation**);
	virtual void HandleReservationResponse(netadr_s&, bool);
	virtual void HandleReserveServerChallengeResponse(int);
	virtual void SetServerReservationCookie(unsigned long long);
};
*/
