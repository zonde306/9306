#pragma once

#define MAX_SHOOT_SOUNDS			16			// Maximum number of shoot sounds per shoot type
#define MAX_WEAPON_STRING			80
#define MAX_WEAPON_PREFIX			16
#define MAX_WEAPON_AMMO_NAME		32
#define WEAPON_PRINTNAME_MISSING	"!!! Missing printname on weapon"

enum WeaponSound_t
{
	EMPTY,
	SINGLE,
	SINGLE_NPC,
	WPN_DOUBLE, // Can't be "DOUBLE" because windows.h uses it.
	DOUBLE_NPC,
	BURST,
	RELOAD,
	RELOAD_NPC,
	MELEE_MISS,
	MELEE_HIT,
	MELEE_HIT_WORLD,
	SPECIAL1,
	SPECIAL2,
	SPECIAL3,
	TAUNT,
	DEPLOY,

	// Add new shoot sound types here

	NUM_SHOOT_SOUND_TYPES,
};

class FileWeaponInfo_t
{
public:
	// Each game can override this to get whatever values it wants from the script.
	virtual void Parse(void *pKeyValuesData, const char *szWeaponName) = NULL;

public:
	bool					bParsedScript;
	bool					bLoadedHudElements;

	// SHARED
	char					szClassName[MAX_WEAPON_STRING];
	char					szPrintName[MAX_WEAPON_STRING];			// Name for showing in HUD, etc.

	char					szViewModel[MAX_WEAPON_STRING];			// View model of this weapon
	char					szWorldModel[MAX_WEAPON_STRING];		// Model of this weapon seen carried by the player
	char					szAnimationPrefix[MAX_WEAPON_PREFIX];	// Prefix of the animations that should be used by the player carrying this weapon
	int						iSlot;									// inventory slot.
	int						iPosition;								// position in the inventory slot.
	int						iMaxClip1;								// max primary clip size (-1 if no clip)
	int						iMaxClip2;								// max secondary clip size (-1 if no clip)
	int						iDefaultClip1;							// amount of primary ammo in the gun when it's created
	int						iDefaultClip2;							// amount of secondary ammo in the gun when it's created
	int						iWeight;								// this value used to determine this weapon's importance in autoselection.
	int						iRumbleEffect;							// Which rumble effect to use when fired? (xbox)
	bool					bAutoSwitchTo;							// whether this weapon should be considered for autoswitching to
	bool					bAutoSwitchFrom;						// whether this weapon can be autoswitched away from when picking up another weapon or ammo
	int						iFlags;									// miscellaneous weapon flags
	char					szAmmo1[MAX_WEAPON_AMMO_NAME];			// "primary" ammo type
	char					szAmmo2[MAX_WEAPON_AMMO_NAME];			// "secondary" ammo type

																	// Sound blocks
	char					aShootSounds[NUM_SHOOT_SOUND_TYPES][MAX_WEAPON_STRING];

	int						iAmmoType;
	int						iAmmo2Type;
	bool					m_bMeleeWeapon;		// Melee weapons can always "fire" regardless of ammo.

												// This tells if the weapon was built right-handed (defaults to true).
												// This helps cl_righthand make the decision about whether to flip the model or not.
	bool					m_bBuiltRightHanded;
	bool					m_bAllowFlipping;	// False to disallow flipping the model, regardless of whether
												// it is built left or right handed.

												// CLIENT DLL
												// Sprite data, read from the data file
	int						iSpriteCount;
	void*					iconActive;
	void*					iconInactive;
	void*					iconAmmo;
	void*					iconAmmo2;
	void*					iconCrosshair;
	void*					iconAutoaim;
	void*					iconZoomedCrosshair;
	void*					iconZoomedAutoaim;
	void*					iconSmall;

	// TF2 specific
	bool					bShowUsageHint;							// if true, then when you receive the weapon, show a hint about it

																	// SERVER DLL

};

class CCSWeaponInfo : public FileWeaponInfo_t	// 866 bytes
{
public:
	float m_fMaxPlayerSpeed; // 2192
	int m_iWeaponType;  // 2196
	int m_iTeam; // 2200 // ANY = 0, TR = 2, CT = 3
	float m_fBotAudiblerange; // 2204
	float m_fWeaponArmorRatio; // 2208
	int m_iCrosshairMinDistance; // 2212
	int m_iCrosshariDeltaDistance; // 2216
	bool m_bCanEquipWithShield; // 2220
	char m_aWrongTeamMsg[32]; // 2221
	char m_aPlayerAnimationExtension[16]; // 2253
	char m_aShieldViewModel[64]; // 2269
	char m_aAddonModel[80]; // 2333
	char m_aAddonAttachment[80]; // 2413
	int m_iMuzzleFlashStyle; // 2496 (not confirmed)
	float m_fMuzzleFlashScale; // 2500
	int m_iPenetration; // 2504
	int m_iDamage; // 2508
	float m_fRange; // 2512
	float m_fRangeModifier; // 2516
	int m_iBullets; // 2520
	float m_fCycleTime; // 2524
	int m_iAccuracyQuadratic; // 2528
	float m_fAccuracyDivisor; // 2532
	float m_fAccuracyOffset; // 2536
	float m_fMaxInaccuracy; // 2540
	float m_fTimeToIdle; // 2544
	float m_fIdleInterval; // 2548
	float m_fNoiseFactor; // 2552
	int m_iBlackMarketPrice; // 2556
	int m_iBlackMarketPrice2; // 2560
	int m_iBlackMarketPreviousPrice; // 2564 (not confirmed)
	char m_aResponseRulesName[80]; // 2568
	char m_aMuzzleFlashEffect_1stPerson[80]; // 2648
	char m_aMuzzleFlashEffect_3rdPerson[80]; // 2728
	char m_aEjectBrassEffect[80]; // 2808
	char m_aDisplayName[80]; // 2888
	char m_aDisplayNameAllCaps[80]; // 2968
	int m_iTier; // 3048
	float m_fMusicDynamicSpeed; // 3052
	bool m_bNewInL4D2; // 3056
	bool m_bCSWeapon; // 3057
};

class CTerrorWeaponInfo : public CCSWeaponInfo
{
public:
	int m_iLoadoutSlots; // 3060
	float m_fMaxAutoAimDeflection1; // 3064
	float m_fMaxAutoAimRange1; // 3068
	float m_fWeaponAutoAimScale; // 3072
	float m_fVerticalPunch; // 3076
	float m_fHorizontalPunch; // 3080
	float m_fHorizontalPunchDirChance; // 3084
	float m_fSpreadPerShot; // 3088
	float m_fMaxSpread; // 3092
	float m_fSpreadDecay; // 3096
	float m_fMinDuckingSpread; // 3100
	float m_fMinStandingSpread; // 3104
	float m_fMinInAirSpread; // 3108
	float m_fMaxMovementSpread; // 3112
	float m_fPelletScatterPitch; // 3116
	float m_fPelletScatterYaw; // 3120
	float m_fUnknown; // 3124
	float m_fReloadDuration; // 3128
	float m_fDualReloadDuration; // 3132
	float m_fDeployDuration; // 3136
	float m_fDualDeployDuration; // 3140
	float m_fPenetrationNumLayers; // 3144
	float m_fPenetrationPower; // 3148
	float m_fPenetrationMaxDistance; // 3152
	float m_fCharacterPenetrationMaxDistance; // 3156
	float m_fGainRange; // 3160
	byte m_tCharacterViewmodelAddon[28]; // 3164
	char m_aViewModelDual[80]; // 3192
	char m_aPlayerModelDual[80]; // 3272
	char m_aAddonAttachment2[80]; // 3352 Why is this duplicated from 2413
	Vector m_vAddonOffest; // 3432 vector
	Vector m_vAddonAngles; // 3444 vector
	char m_aWorldModel[80]; // 3456
};

typedef struct CMeleeWeaponInfo_t	// 3232 bytes
{
	void * pVtable;
	float m_fRefireDelay;
	char m_aViewModel[80];
	char m_aPlayerModel[80];
	char m_aAddonAttachment[80];
	Vector m_vAddonOffest;
	Vector m_vAddonAngles;
	char m_aAnimPrefix[16];
	float m_fDamage;
	int m_iDamageFlags;
	int m_iRumbleEffect;
	bool m_bDecapitates;
	char m_aActivityIdle[80];
	char m_aActivityWalk[80];
	char m_aActivityRun[80];
	char m_aActivityCrouchIdle[80];
	char m_aActivityCrouchWalk[80];
	char m_aActivityCrouchWalk2[80]; // possibly run + walk animations
	char m_aActivityIdleInjured[80];
	char m_aActivityWalkInjured[80];
	char m_aActivityRunInjured[80];
	char m_aActivityIdleCalm[80];
	char m_aActivityWalkCalm[80];
	char m_aActivityRunCalm[80];
	char m_aActivityJump[80];
	char m_aActivityDeploy[80];
	char m_aActivityShove[80];
	char SoundData[20][80]; // 1501
							// 3101: 3 bytes should be padding 
	byte PrimaryAttacks[20]; //3104
	byte SecondaryAttacks[20]; //3124
	int m_iPlayerAnimEvent; // 3144
	float m_fWeaponIdleTime; // 3148
	char src[80]; // 3152 name of the melee weapon
} CMeleeWeaponInfo;
