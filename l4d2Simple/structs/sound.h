#pragma once
#include "../libraries/utl_vector.h"

class IRecipientFilter
{
public:
	virtual ~IRecipientFilter() {}

	virtual bool IsReliable(void) const = 0;
	virtual bool IsInitMessage(void) const = 0;

	virtual int GetRecipientCount(void) const = 0;
	virtual int GetRecipientIndex(int slot) const = 0;
};

//-----------------------------------------------------------------------------
// Purpose: A generic filter for determining whom to send message/sounds etc. to and
//  providing a bit of additional state information
//-----------------------------------------------------------------------------
class CRecipientFilter : public IRecipientFilter
{
public:
	//CRecipientFilter();
	//virtual 		~CRecipientFilter();

	virtual bool	IsReliable(void) const { return true; };
	virtual bool	IsInitMessage(void) const { return false; };

	virtual int		GetRecipientCount(void) const { return -1; };
	virtual int		GetRecipientIndex(int slot) const { return -1; };

public:

	void			CopyFrom(const CRecipientFilter& src);

	void			Reset(void);

	void			MakeInitMessage(void);

	void			MakeReliable(void);

	void			AddAllPlayers(void);
	void			AddRecipientsByPVS(const Vector& origin);
	void			RemoveRecipientsByPVS(const Vector& origin);
	void			AddRecipientsByPAS(const Vector& origin);
	void			AddRecipient(CBaseEntity *player);
	void			RemoveAllRecipients(void);
	void			RemoveRecipient(CBaseEntity *player);
	void			RemoveRecipientByPlayerIndex(int playerindex);
	//void			AddRecipientsByTeam(CTeam *team);
	//void			RemoveRecipientsByTeam(CTeam *team);
	//void			RemoveRecipientsNotOnTeam(CTeam *team);

	void			UsePredictionRules(void);
	bool			IsUsingPredictionRules(void) const;

	bool			IgnorePredictionCull(void) const;
	void			SetIgnorePredictionCull(bool ignore);

	//void			AddPlayersFromBitMask(CBitVec< ABSOLUTE_PLAYER_LIMIT >& playerbits);
	//void			RemovePlayersFromBitMask(CBitVec< ABSOLUTE_PLAYER_LIMIT >& playerbits);

public:

	bool				m_bReliable;
	bool				m_bInitMessage;
	CUtlVector< int >	m_Recipients;

	// If using prediction rules, the filter itself suppresses local player
	bool				m_bUsingPredictionRules;
	// If ignoring prediction cull, then external systems can determine
	//  whether this is a special case where culling should not occur
	bool				m_bIgnorePredictionCull;
};

//-----------------------------------------------------------------------------
// Purpose: Simple class to create a filter for a single player ( unreliable )
//-----------------------------------------------------------------------------
class CSingleUserRecipientFilter : public CRecipientFilter
{
public:
	CSingleUserRecipientFilter(CBaseEntity *player)
	{
		AddRecipient(player);
	}
};

class IEngineSound
{
public:
	// Precache a particular sample
	virtual bool PrecacheSound(const char *pSample, bool bPreload = false, bool bIsUISound = false) = 0;
	virtual bool IsSoundPrecached(const char *pSample) = 0;
	virtual void PrefetchSound(const char *pSample) = 0;

	// Just loads the file header and checks for duration (not hooked up for .mp3's yet)
	// Is accessible to server and client though
	virtual float GetSoundDuration(const char *pSample) = 0;

	// Pitch of 100 is no pitch shift.  Pitch > 100 up to 255 is a higher pitch, pitch < 100
	// down to 1 is a lower pitch.   150 to 70 is the realistic range.
	// EmitSound with pitch != 100 should be used sparingly, as it's not quite as
	// fast (the pitchshift mixer is not native coded).

	// NOTE: setting iEntIndex to -1 will cause the sound to be emitted from the local
	// player (client-side only)

	//virtual void EmitSound(IRecipientFilter &filter, int iEntIndex, int iChannel, const char *pSample,
	//					   float flVolume, float flAttenuation, int iFlags, int iPitch /*= PITCH_NORM*/, int iSpecialDSP,
	//					   const Vector *pOrigin, const Vector *pDirection, void *pUtlVecOrigins, bool bUpdatePositions, float soundtime, int speakerentity) = 0;

	//virtual void EmitSound(IRecipientFilter &filter, int iEntIndex, int iChannel, const char *pSample,
	//					   float flVolume, soundlevel_t iSoundlevel, int iFlags, int iPitch /*= PITCH_NORM*/, int iSpecialDSP,
	//					   const Vector *pOrigin, const Vector *pDirection, void *pUtlVecOrigins, bool bUpdatePositions, float soundtime, int speakerentity) = 0;

	virtual void EmitSound(IRecipientFilter& filter, int iEntIndex, int iChannel, const char *pSample,
		float flVolume, float flAttenuation, int iFlags = 0, int iPitch = 100, int iSpecialDSP = 0,
		const Vector *pOrigin = NULL, const Vector *pDirection = NULL, CUtlVector< Vector >* pUtlVecOrigins = NULL, bool bUpdatePositions = true, float soundtime = 0.0f, int speakerentity = -1) = 0;

	virtual void EmitSound(IRecipientFilter& filter, int iEntIndex, int iChannel, const char *pSample,
		float flVolume, int iSoundlevel, int iFlags = 0, int iPitch = 100, int iSpecialDSP = 0,
		const Vector *pOrigin = NULL, const Vector *pDirection = NULL, CUtlVector< Vector >* pUtlVecOrigins = NULL, bool bUpdatePositions = true, float soundtime = 0.0f, int speakerentity = -1) = 0;

	virtual void EmitSentenceByIndex(IRecipientFilter &filter, int iEntIndex, int iChannel, int iSentenceIndex,
		float flVolume, int iSoundlevel, int iFlags, int iPitch, int iSpecialDSP,
		const Vector *pOrigin, const Vector *pDirection, void *pUtlVecOrigins, bool bUpdatePositions, float soundtime, int speakerentity) = 0;

	virtual void StopSound(int iEntIndex, int iChannel, const char *pSample) = 0;

	// stop all active sounds (client only)
	virtual void StopAllSounds(bool bClearBuffers) = 0;

	// Set the room type for a player (client only)
	virtual void SetRoomType(IRecipientFilter &filter, int roomType) = 0;

	// Set the dsp preset for a player (client only)
	virtual void SetPlayerDSP(IRecipientFilter &filter, int dspType, bool fastReset) = 0;

	// emit an "ambient" sound that isn't spatialized
	// only available on the client, assert on server
	virtual void EmitAmbientSound(const char *pSample, float flVolume, int iPitch, int flags, float soundtime) = 0;

	//	virtual EntChannel_t	CreateEntChannel() = 0;

	virtual float GetDistGainFromSoundLevel(int soundlevel, float dist) = 0;

	// Client .dll only functions
	virtual int GetGuidForLastSoundEmitted() = 0;
	virtual bool IsSoundStillPlaying(int guid) = 0;
	virtual void StopSoundByGuid(int guid) = 0;
	// Set's master volume (0.0->1.0)
	virtual void SetVolumeByGuid(int guid, float fvol) = 0;

	// Retrieves list of all active sounds
	virtual void GetActiveSounds(int &sndlist) = 0;

	virtual void PrecacheSentenceGroup(const char *pGroupName) = 0;
	virtual void NotifyBeginMoviePlayback() = 0;
	virtual void NotifyEndMoviePlayback() = 0;
};
