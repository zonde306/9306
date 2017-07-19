#pragma once
#include "../libraries/utl_dict.h"
#include "../libraries/utl_vector.h"

// Client dispatch function for usermessages
typedef void(*pfnUserMsgHook)(bf_read &msg);

class CUserMessage
{
public:
	// byte size of message, or -1 for variable sized
	int				size;
	const char		*name;
	// Client only dispatch function for message
	CUtlVector<pfnUserMsgHook>	clienthooks;
};

class CUserMessages
{
public:

	CUserMessages();
	~CUserMessages();

	// Returns -1 if not found, otherwise, returns appropriate index
	int		LookupUserMessage(const char *name);
	int		GetUserMessageSize(int index);
	const char *GetUserMessageName(int index);
	bool	IsValidIndex(int index);

	// Server only
	void	Register(const char *name, int size);

	// Client only
	void	HookMessage(const char *name, pfnUserMsgHook hook);
	bool	DispatchUserMessage(int msg_type, bf_read &msg_data);

private:

	CUtlDict<CUserMessage*, int>	m_UserMessages;
};

// void RegisterUserMessages(void);

//-----------------------------------------------------------------------------
// Purpose: Force registration on .dll load
// FIXME:  Should this be a client/server system?
//-----------------------------------------------------------------------------
CUserMessages::CUserMessages()
{
	// Game specific registration function;
	// RegisterUserMessages();
}

CUserMessages::~CUserMessages()
{
	int c = m_UserMessages.Count();
	for (int i = 0; i < c; ++i)
	{
		delete m_UserMessages[i];
	}
	m_UserMessages.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
// Output : int
//-----------------------------------------------------------------------------
int CUserMessages::LookupUserMessage(const char *name)
{
	int idx = m_UserMessages.Find(name);
	if (idx == m_UserMessages.InvalidIndex())
	{
		return -1;
	}

	return idx;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
// Output : int
//-----------------------------------------------------------------------------
int CUserMessages::GetUserMessageSize(int index)
{
	if (index < 0 || index >= (int)m_UserMessages.Count())
	{
		Utils::log("CUserMessages::GetUserMessageSize( %i ) out of range!!!\n", index);
		throw std::exception("CUserMessages::GetUserMessageSize() out of range!");
	}

	CUserMessage *e = m_UserMessages[index];
	return e->size;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
// Output : char const
//-----------------------------------------------------------------------------
const char *CUserMessages::GetUserMessageName(int index)
{
	if (index < 0 || index >= (int)m_UserMessages.Count())
	{
		Utils::log("CUserMessages::GetUserMessageName(%i) out of range!!!\n", index);
		throw std::exception("CUserMessages::GetUserMessageName() out of range!");
	}

	return m_UserMessages.GetElementName(index);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : index - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CUserMessages::IsValidIndex(int index)
{
	return m_UserMessages.IsValidIndex(index);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//			size - -1 for variable size
//-----------------------------------------------------------------------------
void CUserMessages::Register(const char *name, int size)
{
	Assert(name);
	int idx = m_UserMessages.Find(name);
	if (idx != m_UserMessages.InvalidIndex())
	{
		Utils::log("CUserMessages::Register '%s' already registered", name);
		throw std::exception("CUserMessages::Register() already registered!");
	}

	CUserMessage * entry = new CUserMessage;
	entry->size = size;
	entry->name = name;

	m_UserMessages.Insert(name, entry);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//			hook - 
//-----------------------------------------------------------------------------
void CUserMessages::HookMessage(const char *name, pfnUserMsgHook hook)
{
	Assert(name);
	Assert(hook);

	int idx = m_UserMessages.Find(name);
	if (idx == m_UserMessages.InvalidIndex())
	{
		Utils::log("CUserMessages::HookMessage: no such message %s\n", name);
		Assert(0);
		return;
	}

	int i = m_UserMessages[idx]->clienthooks.AddToTail();
	m_UserMessages[idx]->clienthooks[i] = hook;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pszName - 
//			iSize - 
//			*pbuf - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CUserMessages::DispatchUserMessage(int msg_type, bf_read &msg_data)
{
	if (msg_type < 0 || msg_type >= (int)m_UserMessages.Count())
	{
		Utils::log("CUserMessages::DispatchUserMessage:  Bogus msg type %i (max == %i)\n", msg_type, m_UserMessages.Count());
		Assert(0);
		return false;
	}

	CUserMessage *entry = m_UserMessages[msg_type];

	if (!entry)
	{
		Utils::log("CUserMessages::DispatchUserMessage:  Missing client entry for msg type %i\n", msg_type);
		Assert(0);
		return false;
	}

	if (entry->clienthooks.Count() == 0)
	{
		Utils::log("CUserMessages::DispatchUserMessage:  missing client hook for %s\n", GetUserMessageName(msg_type));
		Assert(0);
		return false;
	}

	for (int i = 0; i < entry->clienthooks.Count(); i++)
	{
		bf_read msg_copy = msg_data;

		pfnUserMsgHook hook = entry->clienthooks[i];
		(*hook)(msg_copy);
	}
	return true;
}
