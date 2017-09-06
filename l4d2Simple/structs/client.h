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
