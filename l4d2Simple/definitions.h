#pragma once
typedef void* (*CreateInterfaceFn)(const char *Name, int *ReturnCode);

#define _AssertMsg( _exp, _msg, _executeExp, _bFatal ) do { __analysis_assume( !!(_exp) ); _msg; } while (0)
#define _AssertMsgOnce( _exp, _msg, _bFatal ) do { __analysis_assume( !!(_exp) ); _msg; } while (0)
#define DBGFLAG_ASSERT
#define DBGFLAG_ASSERTFATAL
#define DBGFLAG_ASSERTDEBUG

#define  Assert( _exp )										((void)0)
#define  AssertOnce( _exp )									((void)0)
#define  AssertMsg( _exp, _msg, ... )						((void)0)
#define  AssertMsgOnce( _exp, _msg )						((void)0)
#define  AssertFunc( _exp, _f )								((void)0)
#define  AssertEquals( _exp, _expectedValue )				((void)0)
#define  AssertFloatEquals( _exp, _expectedValue, _tol )	((void)0)
#define  Verify( _exp )										(_exp)
#define	 VerifyMsg1( _exp, _msg, a1 )						(_exp)
#define	 VerifyMsg2( _exp, _msg, a1, a2 )					(_exp)
#define	 VerifyMsg3( _exp, _msg, a1, a2, a3 )				(_exp)
#define  VerifyEquals( _exp, _expectedValue )           	(_exp)
#define  DbgVerify( _exp )									(_exp)

#define  AssertMsg1( _exp, _msg, a1 )									((void)0)
#define  AssertMsg2( _exp, _msg, a1, a2 )								((void)0)
#define  AssertMsg3( _exp, _msg, a1, a2, a3 )							((void)0)
#define  AssertMsg4( _exp, _msg, a1, a2, a3, a4 )						((void)0)
#define  AssertMsg5( _exp, _msg, a1, a2, a3, a4, a5 )					((void)0)
#define  AssertMsg6( _exp, _msg, a1, a2, a3, a4, a5, a6 )				((void)0)
#define  AssertMsg6( _exp, _msg, a1, a2, a3, a4, a5, a6 )				((void)0)
#define  AssertMsg7( _exp, _msg, a1, a2, a3, a4, a5, a6, a7 )			((void)0)
#define  AssertMsg8( _exp, _msg, a1, a2, a3, a4, a5, a6, a7, a8 )		((void)0)
#define  AssertMsg9( _exp, _msg, a1, a2, a3, a4, a5, a6, a7, a8, a9 )	((void)0)

inline void AssertValidReadPtr(const void* ptr, int count = 1) { }
inline void AssertValidWritePtr(const void* ptr, int count = 1) { }
inline void AssertValidReadWritePtr(const void* ptr, int count = 1) { }
#define AssertValidStringPtr AssertValidReadPtr
#define AssertValidThis() AssertValidReadWritePtr(this,sizeof(*this))

#define COLORCODE(r, g, b, a) ((DWORD)((((r)&0xff) << 24) | (((g)&0xff) << 16) | (((b)&0xff) << 8) | ((a)&0xff)))
#define RED(COLORCODE) ((int)(COLORCODE >> 24))
#define BLUE(COLORCODE) ((int)(COLORCODE >> 8) & 0xFF)
#define GREEN(COLORCODE) ((int)(COLORCODE >> 16) & 0xFF)
#define ALPHA(COLORCODE) ((int)COLORCODE & 0xFF)
#define TIME_TO_TICKS(dt)		((int)(0.5f + (float)(dt) / g_interface.Globals->interval_per_tick))

#define COLORBOX_ENEMY_VISIBLE		D3DCOLOR_RGBA(255, 0, 0, 255)		// 红色
#define COLORBOX_FIREND_VISIBLE		D3DCOLOR_RGBA(0, 0, 255, 255)		// 深蓝色
#define COLORBOX_ENEMY				D3DCOLOR_RGBA(128, 0, 255, 255)		// 紫色
#define COLORBOX_FIREND				D3DCOLOR_RGBA(0, 255, 255, 255)		// 浅蓝色
#define COLORBOX_INFECTED_VISIBLE	D3DCOLOR_RGBA(255, 255, 0, 255)		// 黄色
#define COLORBOX_INFECTED			D3DCOLOR_RGBA(255, 128, 0, 255)		// 橙色
#define COLORBOX_WITCH_VISIBLE		D3DCOLOR_RGBA(255, 128, 255, 255)	// 粉色
#define COLORBOX_WITCH				D3DCOLOR_RGBA(255, 0, 128, 255)		// 深粉色

// These defines are for client button presses.
#define IN_ATTACK					(1 << 0)
#define IN_JUMP						(1 << 1)
#define IN_DUCK						(1 << 2)
#define IN_FORWARD					(1 << 3)
#define IN_BACK						(1 << 4)
#define IN_USE						(1 << 5)
#define IN_CANCEL					(1 << 6)
#define IN_LEFT						(1 << 7)
#define IN_RIGHT					(1 << 8)
#define IN_MOVELEFT					(1 << 9)
#define IN_MOVERIGHT				(1 << 10)
#define IN_ATTACK2					(1 << 11)
#define IN_RUN						(1 << 12)
#define IN_RELOAD					(1 << 13)
#define IN_ALT1						(1 << 14)
#define IN_ALT2						(1 << 15)
#define IN_SCORE					(1 << 16)   /**< Used by client.dll for when scoreboard is held down */
#define IN_SPEED					(1 << 17)	/**< Player is holding the speed key */
#define IN_WALK						(1 << 18)	/**< Player holding walk key */
#define IN_ZOOM						(1 << 19)	/**< Zoom key for HUD zoom */
#define IN_WEAPON1					(1 << 20)	/**< weapon defines these bits */
#define IN_WEAPON2					(1 << 21)	/**< weapon defines these bits */
#define IN_BULLRUSH					(1 << 22)
#define IN_GRENADE1					(1 << 23)	/**< grenade 1 */
#define IN_GRENADE2					(1 << 24)	/**< grenade 2 */
#define IN_ATTACK3					(1 << 25)

// Note: these are only for use with GetEntityFlags and SetEntityFlags
//       and may not match the game's actual, internal m_fFlags values.
// PLAYER SPECIFIC FLAGS FIRST BECAUSE WE USE ONLY A FEW BITS OF NETWORK PRECISION
#define	FL_ONGROUND					(1 << 0)	/**< At rest / on the ground */
#define FL_DUCKING					(1 << 1)	/**< Player flag -- Player is fully crouched */
#define	FL_WATERJUMP				(1 << 2)	/**< player jumping out of water */
#define FL_ONTRAIN					(1 << 3)	/**< Player is _controlling_ a train, so movement commands should be ignored on client during prediction. */
#define FL_INRAIN					(1 << 4)	/**< Indicates the entity is standing in rain */
#define FL_FROZEN					(1 << 5)	/**< Player is frozen for 3rd person camera */
#define FL_ATCONTROLS				(1 << 6)	/**< Player can't move, but keeps key inputs for controlling another entity */
#define	FL_CLIENT					(1 << 7)	/**< Is a player */
#define FL_FAKECLIENT				(1 << 8)	/**< Fake client, simulated server side; don't send network messages to them */
// NOTE if you move things up, make sure to change this value
#define PLAYER_FLAG_BITS		9
// NON-PLAYER SPECIFIC (i.e., not used by GameMovement or the client .dll ) -- Can still be applied to players, though
#define	FL_INWATER					(1 << 9)	/**< In water */
#define	FL_FLY						(1 << 10)	/**< Changes the SV_Movestep() behavior to not need to be on ground */
#define	FL_SWIM						(1 << 11)	/**< Changes the SV_Movestep() behavior to not need to be on ground (but stay in water) */
#define	FL_CONVEYOR					(1 << 12)
#define	FL_NPC						(1 << 13)
#define	FL_GODMODE					(1 << 14)
#define	FL_NOTARGET					(1 << 15)
#define	FL_AIMTARGET				(1 << 16)	/**< set if the crosshair needs to aim onto the entity */
#define	FL_PARTIALGROUND			(1 << 17)	/**< not all corners are valid */
#define FL_STATICPROP				(1 << 18)	/**< Eetsa static prop!		 */
#define FL_GRAPHED					(1 << 19)	/**< worldgraph has this ent listed as something that blocks a connection */
#define FL_GRENADE					(1 << 20)
#define FL_STEPMOVEMENT				(1 << 21)	/**< Changes the SV_Movestep() behavior to not do any processing */
#define FL_DONTTOUCH				(1 << 22)	/**< Doesn't generate touch functions, generates Untouch() for anything it was touching when this flag was set */
#define FL_BASEVELOCITY				(1 << 23)	/**< Base velocity has been applied this frame (used to convert base velocity into momentum) */
#define FL_WORLDBRUSH				(1 << 24)	/**< Not moveable/removeable brush entity (really part of the world, but represented as an entity for transparency or something) */
#define FL_OBJECT					(1 << 25)	/**< Terrible name. This is an object that NPCs should see. Missiles, for example. */
#define FL_KILLME					(1 << 26)	/**< This entity is marked for death -- will be freed by game DLL */
#define FL_ONFIRE					(1 << 27)	/**< You know... */
#define FL_DISSOLVING				(1 << 28)	/**< We're dissolving! */
#define FL_TRANSRAGDOLL				(1 << 29)	/**< In the process of turning into a client side ragdoll. */
#define FL_UNBLOCKABLE_BY_PLAYER	(1 << 30)	/**< pusher that can't be blocked by the player */
#define FL_FREEZING					(1 << 31)	/**< We're becoming frozen! */

#define EF_BONEMERGE				(1 << 0)	// Merges bones of names shared with a parent entity to the position and direction of the parent's.
#define EF_BRIGHTLIGHT				(1 << 1)	// Emits a dynamic light of RGB(250,250,250) and a random radius of 400 to 431 from the origin.
#define EF_DIMLIGHT					(1 << 2)	// Emits a dynamic light of RGB(100,100,100) and a random radius of 200 to 231 from the origin.
#define EF_NOINTERP					(1 << 3)	// Don't interpolate on the next frame.
#define EF_NOSHADOW					(1 << 4)	// Don't cast a shadow. To do: Does this also apply to shadow maps?
#define EF_NODRAW					(1 << 5)	// Entity is completely ignored by the client. Can cause prediction errors if a player proceeds to collide with it on the server.
#define EF_NORECEIVESHADOW			(1 << 6)	// Don't receive dynamic shadows.
#define EF_BONEMERGE_FASTCULL		(1 << 7)	// For use with EF_BONEMERGE. If set, the entity will use its parent's origin to calculate whether it is visible; if not set, it will set up parent's bones every frame even if the parent is not in the PVS.
#define EF_ITEM_BLINK				(1 << 8)	// Blink an item so that the user notices it. Added for Xbox 1, and really not very subtle.
#define EF_PARENT_ANIMATES			(1 << 9)	// Assume that the parent entity is always animating. Causes it to realign every frame.

#ifndef PLAYER_FLAG_BITS
#define PLAYER_FLAG_BITS			10
#endif

#define DISPSURF_FLAG_SURFACE		(1 << 0)
#define DISPSURF_FLAG_WALKABLE		(1 << 1)
#define DISPSURF_FLAG_BUILDABLE		(1 << 2)
#define DISPSURF_FLAG_SURFPROP1		(1 << 3)
#define DISPSURF_FLAG_SURFPROP2		(1 << 4)

#define	CONTENTS_EMPTY			0		/**< No contents. */
#define	CONTENTS_SOLID			0x1		/**< an eye is never valid in a solid . */
#define	CONTENTS_WINDOW			0x2		/**< translucent, but not watery (glass). */
#define	CONTENTS_AUX			0x4
#define	CONTENTS_GRATE			0x8		/**< alpha-tested "grate" textures.  Bullets/sight pass through, but solids don't. */
#define	CONTENTS_SLIME			0x10
#define	CONTENTS_WATER			0x20
#define	CONTENTS_MIST			0x40
#define CONTENTS_OPAQUE			0x80		/**< things that cannot be seen through (may be non-solid though). */
#define	LAST_VISIBLE_CONTENTS	0x80
#define ALL_VISIBLE_CONTENTS 	(LAST_VISIBLE_CONTENTS | (LAST_VISIBLE_CONTENTS - 1))
#define CONTENTS_TESTFOGVOLUME	0x100
#define CONTENTS_UNUSED5		0x200
#define CONTENTS_UNUSED6		0x4000
#define CONTENTS_TEAM1			0x800		/**< per team contents used to differentiate collisions. */
#define CONTENTS_TEAM2			0x1000		/**< between players and objects on different teams. */
#define CONTENTS_IGNORE_NODRAW_OPAQUE	0x2000		/**< ignore CONTENTS_OPAQUE on surfaces that have SURF_NODRAW. */
#define CONTENTS_MOVEABLE		0x4000		/**< hits entities which are MOVETYPE_PUSH (doors, plats, etc) */
#define	CONTENTS_AREAPORTAL		0x8000		/**< remaining contents are non-visible, and don't eat brushes. */
#define	CONTENTS_PLAYERCLIP		0x10000
#define	CONTENTS_MONSTERCLIP	0x20000

/**
* @section currents can be added to any other contents, and may be mixed
*/
#define	CONTENTS_CURRENT_0		0x40000
#define	CONTENTS_CURRENT_90		0x80000
#define	CONTENTS_CURRENT_180	0x100000
#define	CONTENTS_CURRENT_270	0x200000
#define	CONTENTS_CURRENT_UP		0x400000
#define	CONTENTS_CURRENT_DOWN	0x800000

/**
* @endsection
*/

#define	CONTENTS_ORIGIN			0x1000000	/**< removed before bsp-ing an entity. */
#define	CONTENTS_MONSTER		0x2000000	/**< should never be on a brush, only in game. */
#define	CONTENTS_DEBRIS			0x4000000
#define	CONTENTS_DETAIL			0x8000000	/**< brushes to be added after vis leafs. */
#define	CONTENTS_TRANSLUCENT	0x10000000	/**< auto set if any surface has trans. */
#define	CONTENTS_LADDER			0x20000000
#define CONTENTS_HITBOX			0x40000000	/**< use accurate hitboxes on trace. */

/**
* @section Trace masks.
*/
#define	MASK_ALL				(0xFFFFFFFF)
#define	MASK_SOLID				(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_MONSTER|CONTENTS_GRATE) 			/**< everything that is normally solid */
#define	MASK_PLAYERSOLID		(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_PLAYERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER|CONTENTS_GRATE) 	/**< everything that blocks player movement */
#define	MASK_NPCSOLID			(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_MONSTERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER|CONTENTS_GRATE) /**< blocks npc movement */
#define	MASK_WATER				(CONTENTS_WATER|CONTENTS_MOVEABLE|CONTENTS_SLIME) 							/**< water physics in these contents */
#define	MASK_OPAQUE				(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_OPAQUE) 							/**< everything that blocks line of sight for AI, lighting, etc */
#define MASK_OPAQUE_AND_NPCS	(MASK_OPAQUE|CONTENTS_MONSTER)										/**< everything that blocks line of sight for AI, lighting, etc, but with monsters added. */
#define	MASK_VISIBLE			(MASK_OPAQUE|CONTENTS_IGNORE_NODRAW_OPAQUE) 								/**< everything that blocks line of sight for players */
#define MASK_VISIBLE_AND_NPCS	(MASK_OPAQUE_AND_NPCS|CONTENTS_IGNORE_NODRAW_OPAQUE) 							/**< everything that blocks line of sight for players, but with monsters added. */
#define	MASK_SHOT				(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_MONSTER|CONTENTS_WINDOW|CONTENTS_DEBRIS|CONTENTS_HITBOX) 	/**< bullets see these as solid */
#define MASK_SHOT_HULL			(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_MONSTER|CONTENTS_WINDOW|CONTENTS_DEBRIS|CONTENTS_GRATE) 	/**< non-raycasted weapons see this as solid (includes grates) */
#define MASK_SHOT_PORTAL		(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW) 							/**< hits solids (not grates) and passes through everything else */
#define MASK_SOLID_BRUSHONLY	(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_GRATE) 					/**< everything normally solid, except monsters (world+brush only) */
#define MASK_PLAYERSOLID_BRUSHONLY	(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_PLAYERCLIP|CONTENTS_GRATE) 			/**< everything normally solid for player movement, except monsters (world+brush only) */
#define MASK_NPCSOLID_BRUSHONLY	(CONTENTS_SOLID|CONTENTS_MOVEABLE|CONTENTS_WINDOW|CONTENTS_MONSTERCLIP|CONTENTS_GRATE) 			/**< everything normally solid for npc movement, except monsters (world+brush only) */
#define MASK_NPCWORLDSTATIC		(CONTENTS_SOLID|CONTENTS_WINDOW|CONTENTS_MONSTERCLIP|CONTENTS_GRATE) 					/**< just the world, used for route rebuilding */
#define MASK_SPLITAREAPORTAL	(CONTENTS_WATER|CONTENTS_SLIME) 									/**< These are things that can split areaportals */

#define HITBOX_COMMON			15	// 普感
#define HITBOX_PLAYER			10	// 生还者/特感
#define CLASSID_COMMON			263	// 普感
#define CLASSID_WORLD			260	// 游戏地图

#define HITBOX_COMMON_1			14
#define HITBOX_COMMON_2			15
#define HITBOX_COMMON_3			16
#define HITBOX_COMMON_4			17
#define HITBOX_JOCKEY			4
#define HITBOX_SPITTER			4
#define HITBOX_CHARGER			9
#define HITBOX_WITCH			10

/**
* @endsection
*/

enum MoveType
{
	MOVETYPE_NONE = 0,			/**< never moves */
	MOVETYPE_ISOMETRIC,			/**< For players */
	MOVETYPE_WALK,				/**< Player only - moving on the ground */
	MOVETYPE_STEP,				/**< gravity, special edge handling -- monsters use this */
	MOVETYPE_FLY,				/**< No gravity, but still collides with stuff */
	MOVETYPE_FLYGRAVITY,		/**< flies through the air + is affected by gravity */
	MOVETYPE_VPHYSICS,			/**< uses VPHYSICS for simulation */
	MOVETYPE_PUSH,				/**< no clip to world, push and crush */
	MOVETYPE_NOCLIP,			/**< No gravity, no collisions, still do velocity/avelocity */
	MOVETYPE_LADDER,			/**< Used by players only when going onto a ladder */
	MOVETYPE_OBSERVER,			/**< Observer movement, depends on player's observer mode */
	MOVETYPE_CUSTOM				/**< Allows the entity to describe its own physics */
};

enum RenderMode
{
	RENDER_NORMAL,				/**< src */
	RENDER_TRANSCOLOR,			/**< c*a+dest*(1-a) */
	RENDER_TRANSTEXTURE,		/**< src*a+dest*(1-a) */
	RENDER_GLOW,				/**< src*a+dest -- No Z buffer checks -- Fixed size in screen space */
	RENDER_TRANSALPHA,			/**< src*srca+dest*(1-srca) */
	RENDER_TRANSADD,			/**< src*a+dest */
	RENDER_ENVIRONMENTAL,		/**< not drawn, used for environmental effects */
	RENDER_TRANSADDFRAMEBLEND,	/**< use a fractional frame value to blend between animation frames */
	RENDER_TRANSALPHAADD,		/**< src + dest*(1-a) */
	RENDER_WORLDGLOW,			/**< Same as kRenderGlow but not fixed size in screen space */
	RENDER_NONE					/**< Don't render. */
};

enum RenderFx
{
	RENDERFX_NONE = 0,
	RENDERFX_PULSE_SLOW,
	RENDERFX_PULSE_FAST,
	RENDERFX_PULSE_SLOW_WIDE,
	RENDERFX_PULSE_FAST_WIDE,
	RENDERFX_FADE_SLOW,
	RENDERFX_FADE_FAST,
	RENDERFX_SOLID_SLOW,
	RENDERFX_SOLID_FAST,
	RENDERFX_STROBE_SLOW,
	RENDERFX_STROBE_FAST,
	RENDERFX_STROBE_FASTER,
	RENDERFX_FLICKER_SLOW,
	RENDERFX_FLICKER_FAST,
	RENDERFX_NO_DISSIPATION,
	RENDERFX_DISTORT,			/**< Distort/scale/translate flicker */
	RENDERFX_HOLOGRAM,			/**< kRenderFxDistort + distance fade */
	RENDERFX_EXPLODE,			/**< Scale up really big! */
	RENDERFX_GLOWSHELL,			/**< Glowing Shell */
	RENDERFX_CLAMP_MIN_SCALE,	/**< Keep this sprite from getting very small (SPRITES only!) */
	RENDERFX_ENV_RAIN,			/**< for environmental rendermode, make rain */
	RENDERFX_ENV_SNOW,			/**<  "        "            "    , make snow */
	RENDERFX_SPOTLIGHT,			/**< TEST CODE for experimental spotlight */
	RENDERFX_RAGDOLL,			/**< HACKHACK: TEST CODE for signalling death of a ragdoll character */
	RENDERFX_PULSE_FAST_WIDER,
	RENDERFX_MAX
};

enum CollisionGroup_t
{
	CG_NONE = 0,
	CG_DEBRIS,					// Collides with nothing but world and static stuff
	CG_DEBRIS_TRIGGER,			// Same as debris, but hits triggers
	CG_INTERACTIVE_DEBRIS,		// Collides with everything except other interactive debris or debris
	CG_INTERACTIVE,				// Collides with everything except interactive debris or debris
	CG_PLAYER,
	CG_BREAKABLE_GLASS,
	CG_VEHICLE,
	CG_PLAYER_MOVEMENT,			// For HL2, same as CG_Player  
	CG_NPC,						// Generic NPC group
	CG_IN_VEHICLE,				// for any entity inside a vehicle
	CG_WEAPON,					// for any weapons that need collision detection
	CG_VEHICLE_CLIP,			// vehicle clip brush to restrict vehicle movement
	CG_PROJECTILE,				// Projectiles!
	CG_DOOR_BLOCKER,			// Blocks entities not permitted to get near moving doors
	CG_PASSABLE_DOOR,			// Doors that the player shouldn't collide with
	CG_DISSOLVING,				// Things that are dissolving are in this group
	CG_PUSHAWAY,				// Nonsolid on client and server, pushaway in player code
	CG_NPC_ACTOR,				// Used so NPCs in scripts ignore the player.  
	CG_NPC_SCRIPTED				// USed for NPCs in scripts that should not collide with each other
};

enum SolidType_t
{
	SOLID_NONE = 0,				// no solid model
	SOLID_BSP,					// a BSP tree
	SOLID_BBOX,					// an AABB
	SOLID_OBB,					// an OBB (not implemented yet)
	SOLID_OBB_YAW,				// an OBB, constrained so that it can only yaw
	SOLID_CUSTOM,				// Always call into the entity for tests
	SOLID_VPHYSICS				// solid vphysics object, get vcollide from the model and collide with that
};

enum SolidFlags_t
{
	SF_CUSTOMRAYTEST = 0x0001,			// Ignore solid type + always call into the entity for ray tests
	SF_CUSTOMBOXTEST = 0x0002,			// Ignore solid type + always call into the entity for swept box tests
	SF_NOT_SOLID = 0x0004,				// Are we currently not solid?
	SF_TRIGGER = 0x0008,				// This is something may be collideable but fires touch functions
										// even when it's not collideable (when the SF_NOT_SOLID flag is set)
	SF_NOT_STANDABLE = 0x0010,			// You can't stand on this
	SF_VOLUME_CONTENTS = 0x0020,		// Contains volumetric contents (like water)
	SF_FORCE_WORLD_ALIGNED = 0x0040,	// Forces the collision rep to be world-aligned even if it's SOLID_BSP or SOLID_VPHYSICS
	SF_USE_TRIGGER_BOUNDS = 0x0080,		// Uses a special trigger bounds separate from the normal OBB
	SF_ROOT_PARENT_ALIGNED = 0x0100,	// Collisions are defined in root parent's local coordinate space
	SF_TRIGGER_TOUCH_DEBRIS = 0x0200	// This trigger will touch debris objects
};

// entity flags, CBaseEntity::m_iEFlags
enum
{
	EFL_KILLME = (1 << 0),	// This entity is marked for death -- This allows the game to actually delete ents at a safe time
	EFL_DORMANT = (1 << 1),	// Entity is dormant, no updates to client
	EFL_NOCLIP_ACTIVE = (1 << 2),	// Lets us know when the noclip command is active.
	EFL_SETTING_UP_BONES = (1 << 3),	// Set while a model is setting up its bones.
	EFL_KEEP_ON_RECREATE_ENTITIES = (1 << 4), // This is a special entity that should not be deleted when we restart entities only

	EFL_HAS_PLAYER_CHILD = (1 << 4),	// One of the child entities is a player.

	EFL_DIRTY_SHADOWUPDATE = (1 << 5),	// Client only- need shadow manager to update the shadow...
	EFL_NOTIFY = (1 << 6),	// Another entity is watching events on this entity (used by teleport)

	// The default behavior in ShouldTransmit is to not send an entity if it doesn't
	// have a model. Certain entities want to be sent anyway because all the drawing logic
	// is in the client DLL. They can set this flag and the engine will transmit them even
	// if they don't have a model.
	EFL_FORCE_CHECK_TRANSMIT = (1 << 7),

	EFL_BOT_FROZEN = (1 << 8),	// This is set on bots that are frozen.
	EFL_SERVER_ONLY = (1 << 9),	// Non-networked entity.
	EFL_NO_AUTO_EDICT_ATTACH = (1 << 10), // Don't attach the edict; we're doing it explicitly

	// Some dirty bits with respect to abs computations
	EFL_DIRTY_ABSTRANSFORM = (1 << 11),
	EFL_DIRTY_ABSVELOCITY = (1 << 12),
	EFL_DIRTY_ABSANGVELOCITY = (1 << 13),
	EFL_DIRTY_SURROUNDING_COLLISION_BOUNDS = (1 << 14),
	EFL_DIRTY_SPATIAL_PARTITION = (1 << 15),
	//	UNUSED						= (1<<16),

	EFL_IN_SKYBOX = (1 << 17),	// This is set if the entity detects that it's in the skybox.
	// This forces it to pass the "in PVS" for transmission.
	EFL_USE_PARTITION_WHEN_NOT_SOLID = (1 << 18),	// Entities with this flag set show up in the partition even when not solid
	EFL_TOUCHING_FLUID = (1 << 19),	// Used to determine if an entity is floating

	// FIXME: Not really sure where I should add this...
	EFL_IS_BEING_LIFTED_BY_BARNACLE = (1 << 20),
	EFL_NO_ROTORWASH_PUSH = (1 << 21),		// I shouldn't be pushed by the rotorwash
	EFL_NO_THINK_FUNCTION = (1 << 22),
	EFL_NO_GAME_PHYSICS_SIMULATION = (1 << 23),

	EFL_CHECK_UNTOUCH = (1 << 24),
	EFL_DONTBLOCKLOS = (1 << 25),		// I shouldn't block NPC line-of-sight
	EFL_DONTWALKON = (1 << 26),		// NPC;s should not walk on this entity
	EFL_NO_DISSOLVE = (1 << 27),		// These guys shouldn't dissolve
	EFL_NO_MEGAPHYSCANNON_RAGDOLL = (1 << 28),	// Mega physcannon can't ragdoll these guys.
	EFL_NO_WATER_VELOCITY_CHANGE = (1 << 29),	// Don't adjust this entity's velocity when transitioning into water
	EFL_NO_PHYSCANNON_INTERACTION = (1 << 30),	// Physcannon can't pick these up or punt them
	EFL_NO_DAMAGE_FORCES = (1 << 31)	// Doesn't accept forces from physics damage
};

// Hud Element hiding flags
#define	HIDEHUD_WEAPONSELECTION		( 1<<0 )	// Hide ammo count & weapon selection
#define	HIDEHUD_FLASHLIGHT			( 1<<1 )
#define	HIDEHUD_ALL					( 1<<2 )
#define HIDEHUD_HEALTH				( 1<<3 )	// Hide health & armor / suit battery
#define HIDEHUD_PLAYERDEAD			( 1<<4 )	// Hide when local player's dead
#define HIDEHUD_NEEDSUIT			( 1<<5 )	// Hide when the local player doesn't have the HEV suit
#define HIDEHUD_MISCSTATUS			( 1<<6 )	// Hide miscellaneous status elements (trains, pickup history, death notices, etc)
#define HIDEHUD_CHAT				( 1<<7 )	// Hide all communication elements (saytext, voice icon, etc)
#define	HIDEHUD_CROSSHAIR			( 1<<8 )	// Hide crosshairs
#define	HIDEHUD_VEHICLE_CROSSHAIR	( 1<<9 )	// Hide vehicle crosshair
#define HIDEHUD_INVEHICLE			( 1<<10 )
#define HIDEHUD_BONUS_PROGRESS		( 1<<11 )	// Hide bonus progress display (for bonus map challenges)

// ---------------------------
//  Hit Group standards
// ---------------------------
#define	HITGROUP_GENERIC	0
#define	HITGROUP_HEAD		1
#define	HITGROUP_CHEST		2
#define	HITGROUP_STOMACH	3
#define HITGROUP_LEFTARM	4	
#define HITGROUP_RIGHTARM	5
#define HITGROUP_LEFTLEG	6
#define HITGROUP_RIGHTLEG	7
#define HITGROUP_GEAR		10			// alerts NPC, but doesn't do damage or bleed (1/100th damage)

// settings for m_takedamage
#define	DAMAGE_NO				0
#define DAMAGE_EVENTS_ONLY		1		// Call damage functions, but don't modify health
#define	DAMAGE_YES				2
#define	DAMAGE_AIM				3

// Spectator Movement modes m_iObserverMode
enum
{
	OBS_MODE_NONE = 0,	// not in spectator mode
	OBS_MODE_DEATHCAM,	// special mode for death cam animation
	OBS_MODE_FREEZECAM,	// zooms to a target, and freeze-frames on them
	OBS_MODE_FIXED,		// view from a fixed camera position
	OBS_MODE_IN_EYE,	// follow a player in first person view
	OBS_MODE_CHASE,		// follow a player in third person view
	OBS_MODE_POI,		// PASSTIME point of interest - game objective, big fight, anything interesting; added in the middle of the enum due to tons of hard-coded "<ROAMING" enum compares
	OBS_MODE_ROAMING,	// free roaming

	NUM_OBSERVER_MODES
};

enum LifeStates_t
{
	LIFE_ALIVE = 0,
	LIFE_DYING,
	LIFE_DEAD,
	LIFE_RESPAWNABLE,
	LIFE_DISCARDBODY
};

//----------------------------------
//----------------------------------
//--------------L4D2-------------
//----------------------------------
//----------------------------------
#define IClientModePointer		0x7A5380		// (IClientMode*)(client.dll + 0x794148)

// ConVar
#define sv_cheats				0x670758		// engine.dll
#define sv_pure					0x64307C		// engine.dll
#define sv_consistency			0x670998		// engine.dll
#define r_drawothermodels		0x6F85E8		// client.dll
#define cl_drawshadowtexture	0x7759B0		// client.dll
#define mat_fullbright			0xFE3F0			// materialsystem.dll
#define mp_gamemode				0x79679C		// client.dll - char*

// cbaseentity offset
#define m_iCrosshairsId			0x19D8
#define m_iLastCrosshairsId		0x19E8
#define m_iButtons				0x72A670		// client.dll
#define m_dwLocalPlayer			0x6FC9D8		// client.dll
#define m_dwGetClientMode		0x1D7100		// client.dll

// other
#define D3DDevice				0x173988		// shaderapidx9.dll

// 获取本地玩家
#define GetLocalClient()			g_interface.ClientEntList->GetClientEntity(g_interface.Engine->GetLocalPlayer())

// 倒地
#define IsFallDown(_e)				(_e->GetNetProp<byte>("m_isIncapacitated", "DT_TerrorPlayer") & 1)

// 挂边
#define IsHangingFromLedge(_e)		(_e->GetNetProp<byte>("m_isHangingFromLedge", "DT_TerrorPlayer") & 1)

// 被舌头拉
#define IsVictimSmoker(_e)			(_e->GetNetProp<int>("m_tongueOwner", "DT_TerrorPlayer") > 0)

// 被猴骑
#define IsVictimJockey(_e)			(_e->GetNetProp<int>("m_jockeyAttacker", "DT_TerrorPlayer") > 0)

// 被猎人扑
#define IsVictimHunter(_e)			(_e->GetNetProp<int>("m_pounceAttacker", "DT_TerrorPlayer") > 0)

// 被牛抓住锤地板
#define IsVictimCharger(_e)			(_e->GetNetProp<int>("m_pummelAttacker", "DT_TerrorPlayer") > 0)

// 被控
#define IsControlled(_e)			(IsVictimSmoker(_e) || IsVictimJockey(_e) || IsVictimHunter(_e) || IsVictimCharger(_e))

// 无法移动(倒地挂边)
#define IsIncapacitated(_e)			(IsFallDown(_e) || IsHangingFromLedge(_e))

// 是否需要队友救援
#define IsNeedRescue(_e)			(IsIncapacitated(_e) || IsControlled(_e))

// 是否为幽灵状态
#define IsGhostInfected(_e)			(_e->GetNetProp<byte>("m_isGhost", "DT_TerrorPlayer") & 1)

// 获取当前服务器时间
#define GetServerTime()				(g_interface.ClientEntList->GetClientEntity(g_interface.Engine->GetLocalPlayer())->GetTickBase() * g_interface.Globals->interval_per_tick)

// 输出偏移地址
#define printo(_s,_n)				std::cout << XorStr(_s) << XorStr(" = 0x") << std::setiosflags(std::ios::hex|std::ios::uppercase) << std::hex << (DWORD)_n << std::resetiosflags(std::ios::hex|std::ios::uppercase) << std::oct << std::endl
#define printv(_n)					printo(#_n,_n)

// 输出日志
#define logerr(_x)					errlog << XorStr(__FILE__) << "("<<__LINE__<<")" << XorStr(__FUNCTION__) << ": " << XorStr(_x) << "\r\n"
#define logfile(_s)					std::fstream f("segt.log", std::ios::out|std::ios::app|std::ios::ate); f << XorStr(__FILE__) << "("<<__LINE__<<")" << XorStr(__FUNCTION__) << ": " << XorStr(_s) << "\r\n"; f.close()

// 检查是否需要自动连点
#define IsWeaponSingle(_id)			(_id == Weapon_Pistol || _id == Weapon_ShotgunPump || _id == Weapon_ShotgunAuto || _id == Weapon_SniperHunting || _id == Weapon_ShotgunChrome || _id == Weapon_SniperMilitary || _id == Weapon_ShotgunSpas || _id == Weapon_PistolMagnum || _id == Weapon_SniperAWP || _id == Weapon_SniperScout)

// 该弹药类型是否为枪械
#define IsGunWeaponAmmotype(_at)	(_at == AT_Pistol || _at == AT_Magnum || _at == AT_Rifle || _at == AT_Smg || _at == AT_M60 || _at == AT_Shotgun || _at == AT_AutoShotgun || _at == AT_Hunting || _at == AT_Sniper || _at == AT_Grenade)

// 检查是否是一把枪
#define IsSubMachinegun(_id)		(_id == WeaponId_SubMachinegun || _id == WeaponId_Silenced || _id == WeaponId_MP5)
#define IsShotgun(_id)				(_id == WeaponId_PumpShotgun || _id == WeaponId_Chrome || _id == WeaponId_AutoShotgun || _id == WeaponId_SPAS)
#define IsAssaultRifle(_id)			(_id == WeaponId_AssaultRifle || _id == WeaponId_AK47 || _id == WeaponId_Desert || _id == WeaponId_SG552 || _id == WeaponId_M60)
#define IsSniper(_id)				(_id == WeaponId_SniperRifle || _id == WeaponId_Military || _id == WeaponId_Scout || _id == WeaponId_AWP)
#define IsPistol(_id)				(_id == WeaponId_Pistol || _id == WeaponId_MagnumPistol)
#define IsMelee(_id)				(_id == WeaponId_TerrorMeleeWeapon || _id == WeaponId_Chainsaw)
#define IsMedical(_id)				(_id == WeaponId_FirstAidKit || _id == WeaponId_ItemDefibrillator || _id == WeaponId_PainPills || _id == WeaponId_Adrenaline)
#define IsAmmoStack(_id)			(_id == WeaponId_ItemAmmoPack || _id == WeaponId_ItemUpgradePackExplosive || _id == WeaponId_ItemUpgradePackIncendiary)
#define IsWeaponT1(_id)				(IsSubMachinegun(_id) || _id == WeaponId_PumpShotgun || _id == WeaponId_Chrome || _id == WeaponId_Pistol)
#define IsWeaponT2(_id)				(_id == WeaponId_AutoShotgun || _id == WeaponId_SPAS || _id == WeaponId_AssaultRifle || _id == WeaponId_AK47 || _id == WeaponId_Desert || _id == WeaponId_SG552 || _id == WeaponId_MagnumPistol || IsSniper(_id))
#define IsWeaponT3(_id)				(_id == WeaponId_M60 || _id == WeaponId_GrenadeLauncher)

#define IsNotGunWeapon(_id)			(IsGrenadeWeapon(_id) || IsMedicalWeapon(_id) || IsPillsWeapon(_id) || IsCarryWeapon(_id) || _id == Weapon_Melee || _id == Weapon_Chainsaw)
#define IsGunWeapon(_id)			(IsSubMachinegun(_id) || IsShotgun(_id) || IsAssaultRifle(_id) || IsSniper(_id) || IsPistol(_id))
#define IsGrenadeWeapon(_id)		(_id == Weapon_Molotov || _id == Weapon_PipeBomb || _id == Weapon_Vomitjar)
#define IsMedicalWeapon(_id)		(_id == Weapon_FirstAidKit || _id == Weapon_Defibrillator || _id == Weapon_FireAmmo || _id == Weapon_ExplodeAmmo)
#define IsPillsWeapon(_id)			(_id == Weapon_PainPills || _id == Weapon_Adrenaline)
#define IsCarryWeapon(_id)			(_id == Weapon_Gascan || _id == Weapon_Fireworkcrate || _id == Weapon_Propanetank || _id == Weapon_Oxygentank || _id == Weapon_Gnome || _id == Weapon_Cola)

// 是否为特感
#define IsSpecialInfected(_id)		(_id == ET_BOOMER || _id == ET_HUNTER || _id == ET_SMOKER || _id == ET_SPITTER || _id == ET_JOCKEY || _id == ET_CHARGER || _id == ET_TANK)

// 是否为普感
#define IsCommonInfected(_id)		(_id == ET_INFECTED || _id == ET_WITCH)

// 是否为生还者
#define IsSurvivor(_id)				(_id == ET_SURVIVORBOT || _id == ET_CTERRORPLAYER)

// 检查是否为有用的实体
#define IsValidVictimId(_id)		(_id == ET_INFECTED || _id == ET_WITCH || _id == ET_BOOMER || _id == ET_HUNTER || _id == ET_SMOKER || _id == ET_SPITTER || _id == ET_JOCKEY || _id == ET_CHARGER || _id == ET_TANK || _id == ET_SURVIVORBOT || _id == ET_CTERRORPLAYER)

enum AmmoType_t
{
	AT_Pistol = 1,					// 小手枪弹药
	AT_Magnum,						// 马格南手枪弹药
	AT_Rifle,						// 步枪弹药
	AT_Minigun,						// 一代固定机枪（开枪有延迟那个）
	AT_Smg,							// 冲锋枪弹药
	AT_M60,							// 机枪弹药
	AT_Shotgun,						// 单喷弹药
	AT_AutoShotgun,					// 连喷弹药
	AT_Hunting,						// 猎枪（15发子弹）
	AT_Sniper,						// 狙击枪弹药
	AT_Turret,						// 二代固定机枪（射速慢点但无延迟）
	AT_PipeBomb,					// 土制炸弹
	AT_Molotov,						// 燃烧瓶
	AT_Vomitjar,					// 胆汁罐
	AT_PainPills,					// 止痛药
	AT_FirstAidKit,					// 医疗包
	AT_Grenade,						// 榴弹发射器的榴弹
	AT_Adrenline,					// 肾上腺素
	AT_Chainsaw						// 电锯
};

enum WeaponID_t
{
	Weapon_Pistol = 1,				// 小手枪(包括双持) 手枪
	Weapon_ShotgunPump = 3,			// 木喷 半自动
	Weapon_ShotgunAuto = 4,			// 连喷 连点加快射速
	Weapon_SniperHunting = 6,		// 猎枪 半自动
	Weapon_ShotgunChrome = 8,		// 铁喷 半自动
	Weapon_SniperMilitary = 10,		// 连狙 半自动
	Weapon_ShotgunSpas = 11,		// 高级连喷 连点加快射速
	Weapon_PistolMagnum = 32,		// 马格南 手枪
	Weapon_SniperAWP = 35,			// 大鸟 半自动
	Weapon_SniperScout = 36,		// 鸟狙 半自动

	Weapon_Melee = 19,				// 近战武器
	Weapon_GrenadeLauncher = 21,	// 榴弹发射器
	Weapon_Chainsaw = 20,			// 电锯

	Weapon_FirstAidKit = 12,		// 医疗包
	Weapon_PainPills = 15,			// 止痛药
	Weapon_Defibrillator = 24,		// 电击器
	Weapon_Adrenaline = 23,			// 肾上腺素
	Weapon_Molotov = 13,			// 燃烧瓶
	Weapon_PipeBomb = 14,			// 土制炸弹
	Weapon_Vomitjar = 25,			// 胆汁罐
	Weapon_FireAmmo = 30,			// 燃烧子弹盒
	Weapon_ExplodeAmmo = 31,		// 爆炸子弹盒
	Weapon_Gascan = 16,				// 油桶（红色和黄色）
	Weapon_Fireworkcrate = 29,		// 烟花盒
	Weapon_Propanetank = 17,		// 煤气罐
	Weapon_Oxygentank = 18,			// 氧气瓶
	Weapon_Gnome = 27,				// 侏儒
	Weapon_Cola = 28,				// 可乐

	WeaponId_WeaponCSBase = 0,
	WeaponId_AssaultRifle = 5,
	WeaponId_AutoShotgun = 4,
	WeaponId_BaseBackpackItem = 0,
	WeaponId_BoomerClaw = 41,
	WeaponId_CarriedProp = 16,
	WeaponId_Chainsaw = 20,
	WeaponId_ChargerClaw = 40,
	WeaponId_ColaBottles = 28,
	WeaponId_FireworkCrate = 29,
	WeaponId_FirstAidKit = 12,
	WeaponId_GasCan = 16,
	WeaponId_Gnome = 27,
	WeaponId_GrenadeLauncher = 21,
	WeaponId_HunterClaw = 39,
	WeaponId_Adrenaline = 23,
	WeaponId_ItemAmmoPack = 22,
	WeaponId_ItemDefibrillator = 24,
	WeaponId_ItemUpgradePackExplosive = 31,
	WeaponId_ItemUpgradePackIncendiary = 30,
	WeaponId_VomitJar = 25,
	WeaponId_JockeyClaw = 44,
	WeaponId_Molotov = 13,
	WeaponId_OxygenTank = 18,
	WeaponId_PainPills = 15,
	WeaponId_PipeBomb = 14,
	WeaponId_Pistol = 1,
	WeaponId_MagnumPistol = 32,
	WeaponId_PropaneTank = 17,
	WeaponId_PumpShotgun = 3,
	WeaponId_AK47 = 26,
	WeaponId_Desert = 9,
	WeaponId_M60 = 37,
	WeaponId_SG552 = 34,
	WeaponId_Chrome = 8,
	WeaponId_SPAS = 11,
	WeaponId_MP5 = 33,
	WeaponId_Silenced = 7,
	WeaponId_SmokerClaw = 42,
	WeaponId_SniperRifle = 6,
	WeaponId_AWP = 35,
	WeaponId_Military = 10,
	WeaponId_Scout = 36,
	WeaponId_SpitterClaw = 43,
	WeaponId_SubMachinegun = 2,
	WeaponId_TankClaw = 38,
	WeaponId_TerrorMeleeWeapon = 19,
	WeaponId_WeaponSpawn = 8
};

enum EntityType_t
{
	ET_INVALID = -1,
	ET_WORLD = 260,
	ET_TerrorGameRulesProxy = 228,
	ET_TerrorPlayerResource = 232,
	ET_PlayerResource = 133,
	ET_CSGameRulesProxy = 47,
	ET_GameRulesProxy = 93,

	// 生还者
	ET_CTERRORPLAYER = 231,
	ET_SURVIVORBOT = 274,

	// 普感
	ET_INFECTED = 263,
	ET_WITCH = 276,

	// 特感
	ET_BOOMER = 0,
	ET_TANK = 275,
	ET_JOCKEY = 264,
	ET_SPITTER = 271,
	ET_CHARGER = 99,
	ET_HUNTER = 262,
	ET_SMOKER = 269,
	
	// 飞行物
	ET_TankRock = 13,				// 克的石头
	ET_VomitJarProjectile = 251,	// 胆汁
	ET_SpitterProjectile = 175,		// 口水的酸液球
	ET_PipeBombProjectile = 130,	// 土雷
	ET_MolotovProjectile = 119,		// 火瓶
	ET_GrenadeProjectile = 97,		// 榴弹发射器的榴弹

	// 杂物
	ET_DoorCheckpoint = 143,		// 安全门
	ET_SurvivorRescue = 185,		// 复活点
	ET_SurvivorDeathModel = 183,	// 死亡的生还者
	ET_PropHealthCabinet = 144,		// 医疗箱

	// 武器 - 其他
	ET_WeaponMountedGun = 146,
	ET_WeaponMinigun = 145,
	ET_WeaponAmmoSpawn = 255,
	ET_WeaponSpawn = 259,
	ET_ScavengeItemSpawn = 258,
	ET_BaseUpgradeItem = 29,

	// 武器 - 冲锋枪
	ET_WeaponMP5 = 164,
	ET_WeaponSilenced = 165,

	// 武器 - 霰弹枪
	ET_WeaponAuto = 2,
	ET_WeaponSpas = 162,
	ET_WeaponChrome = 161,
	ET_WeaponPump = 148,

	// 武器 - 步枪
	ET_WeaponAK47 = 152,
	ET_WeaponDesert = 153,
	ET_WeaponSG552 = 155,
	ET_WeaponRifle = 1,
	ET_WeaponM60 = 154,
	ET_WeaponGrenadeLauncher = 96,

	// 武器 - 狙击枪
	ET_WeaponScout = 170,
	ET_WeaponMilitary = 169,
	ET_WeaponAWP = 168,
	
	// 武器 - 手枪
	ET_WeaponMagnum = 116,
	ET_WeaponPistol = 131,
	
	// 武器 - 近战武器
	ET_WeaponChainsaw = 39,
	ET_WeaponMelee = 230,

	// 武器 - 投掷武器
	ET_WeaponPipeBomb = 129,
	ET_WeaponMolotov = 118,
	ET_WeaponVomitjar = 106,
	ET_ProjectilePipeBomb = 130,
	ET_ProjectileMolotov = 119,
	ET_ProjectileVomitJar = 251,
	ET_ProjectileSpitter = 175,
	ET_ProjectileGrenadeLauncher = 97,
	ET_ProjectileGrenade = 13,
	
	// 武器 - 医疗品/升级包
	ET_WeaponIncendiary = 111,
	ET_WeaponExplosive = 110,
	ET_WeaponDefibrillator = 109,
	ET_WeaponFirstAidKit = 73,
	ET_WeaponAmmoPack = 107,
	
	// 武器 - 药物
	ET_WeaponPainPills = 121,
	ET_WeaponAdrenaline = 105,
	
	// 武器 - 携带物品
	ET_WeaponOxygen = 120,
	ET_WeaponGnome = 95,
	ET_WeaponGascan = 94,
	ET_WeaponFirework = 72,
	ET_WeaponCola = 44
};

enum ZombieClass_t
{
	ZC_SMOKER = 1,
	ZC_BOOMER = 2,
	ZC_HUNTER = 3,
	ZC_SPITTER = 4,
	ZC_JOCKEY = 5,
	ZC_CHARGER = 6,
	ZC_WITCH = 7,
	ZC_TANK = 8,
	ZC_SURVIVORBOT = 9
};

enum SurvivorBone_t
{
	BONE_SURVIVOR_BAD = -1,
	BONE_ROCHELLE_HEAD = 16,
	BONE_COACH_HEAD = 17,
	BONE_ELLIS_HEAD = 14,
	BONE_NICK_HEAD = 14,
	BONE_FRANCIS_HEAD = 14,
	BONE_ZOEY_HEAD = 14,
	BONE_LOUIS_HEAD = 14,
	BONE_BILL_HEAD = 14
};

enum ZombieBone_t
{
	BONE_ZOMBIE_BAD = -1,
	BONE_NECK = 14,
	BONE_JOCKEY_HEAD = 7,
	BONE_CHARGER_HEAD = 16,
	BONE_INFECTED_HEAD = 29,
	BONE_TANK_CHEST = 12,
	BONE_BOOMER_CHEST = 11,
	BONE_SPITTER_CHEST = 3,
	BONE_JOCKEY_CHEST = 4,
	BONE_CHARGER_CHEST = 4,
	BONE_HUNTER_CHEST = 12
};

enum entityGender_t
{
	G_MALE = 1,
	G_FEMALE,
	G_BILL,
	G_ZOEY,
	G_FRANCIS,
	G_LOUIS,
	G_NICK,
	G_ROCHELLE,
	G_COACH,
	G_ELLIS
};

//---------------
//----PLAYERS----
//---------------

#define l4d2_ellisbody(_s,_n,_p)	(_s == 32 && _n == 4822 && _p == 8088) 
#define l4d2_ellishead(_s,_n,_p)	(_s == 32 && ((_n == 2037 && _p == 3832) || (_n == 2661 && _p == 4872))) 
#define l4d2_ellis(_s,_n,_p)		(l4d2_ellisbody(_s,_n,_p) || l4d2_ellishead(_s,_n,_p))

#define l4d2_coatchbody(_s,_n,_p)	(_s == 32 && _n == 5710 && _p == 9262) 
#define l4d2_coatchead(_s,_n,_p)	(_s == 32 && ( (_n == 1819 && _p == 3375) || (_n == 2387 && _p == 4527) ))
#define l4d2_coatch(_s,_n,_p)		(l4d2_coatchbody(_s,_n,_p) || l4d2_coatchead(_s,_n,_p))

#define l4d2_rachellebody(_s,_n,_p)	(_s == 32 && _n == 6443 && _p == 10138) 
#define l4d2_rachellehead(_s,_n,_p)	(_s == 32 && ( (_n == 1647 && _p == 3069) || (_n == 3947 && _p == 6910) ))
#define l4d2_rachelle(_s,_n,_p)		(l4d2_rachellebody(_s,_n,_p) || l4d2_rachellehead(_s,_n,_p))

#define l4d2_nickbody(_s,_n,_p)		(_s == 32 && ( (_n == 7957 && _p == 11314) || (_n == 7913 && _p == 12247) )) 
#define l4d2_nickhead(_s,_n,_p)		(_s == 32 && ( (_n == 1680 && _p == 3133) || (_n == 2384 && _p == 4540) )) 
#define l4d2_nick(_s,_n,_p)			(l4d2_nickbody(_s,_n,_p) || l4d2_nickhead(_s,_n,_p))

#define l4d2_survivor(_s,_n,_p)		(l4d2_ellis(_s,_n,_p) || l4d2_coatch(_s,_n,_p) || l4d2_rachelle(_s,_n,_p) || l4d2_nick(_s,_n,_p))

#define l4d2_pistolhands (_s == 32 && _n == 10318 && _p == 9340) 
#define l4d2_hands (_s == 32 && _n == 4262 && _p == 6468) 


//---------------
//----WEAPONS---
//--------------


//GUNS
#define l4d2_p220(_s,_n,_p)			(_s == 32 && _n == 3406 && _p == 2883)
#define l4d2_glock(_s,_n,_p)		(_s == 32 && _n == 961 && _p == 835) 
#define l4d2_deagle(_s,_n,_p)		(_s == 32 && _n == 2530 && _p == 1962) 

#define l4d2_guns(_s,_n,_p)			(l4d2_p220(_s,_n,_p) || l4d2_glock(_s,_n,_p) || l4d2_deagle(_s,_n,_p))

//SHOTGUNS
#define l4d2_spas12(_s,_n,_p)		(_s == 32 && _n == 5320 && _p == 5277) 
#define l4d2_chrome(_s,_n,_p)		(_s == 32 && _n == 3525 && _p == 3418) 
#define l4d2_autoshotgun(_s,_n,_p)	(_s == 32 && _n == 2942 && _p == 2224)  
#define l4d2_pumpshotgun(_s,_n,_p)	(_s == 32 && _n == 2507 && _p == 2362) 

#define l4d2_shotguns(_s,_n,_p)		(l4d2_spas12(_s,_n,_p) || l4d2_chrome(_s,_n,_p) || l4d2_autoshotgun(_s,_n,_p) || l4d2_pumpshotgun(_s,_n,_p))

//SMG
#define l4d2_uzi(_s,_n,_p)			(_s == 32 && _n == 2318 && _p == 1875)
#define l4d2_uzisilent(_s,_n,_p)	(_s == 32 && _n == 5283 && _p == 5221)
#define l4d2_mp5(_s,_n,_p)			(_s == 32 && _n == 502 && _p == 410) 

#define l4d2_smg(_s,_n,_p)			(l4d2_uzi(_s,_n,_p) || l4d2_uzisilent(_s,_n,_p) || l4d2_mp5(_s,_n,_p))

//MELEE
#define l4d2_axe(_s,_n,_p)			(_s == 32 && _n == 1594 && _p == 2638) //hache
#define l4d2_crowbar(_s,_n,_p)		(_s == 32 && _n == 1061 && _p == 1510) //barredefer
#define l4d2_baseballbat(_s,_n,_p)	(_s == 32 && _n == 641 && _p == 1132)
#define l4d2_chainsaw(_s,_n,_p)		(_s == 32 && _n == 4880 && _p == 4287) //tronconneuse
#define l4d2_katana(_s,_n,_p)		(_s == 32 && _n == 950 && _p == 1179) 
#define l4d2_cricketbat(_s,_n,_p)	(_s == 32 && _n == 1354 && _p == 2103) 

#define l4d2_melee(_s,_n,_p)		(l4d2_axe(_s,_n,_p) || l4d2_crowbar(_s,_n,_p) || l4d2_baseballbat(_s,_n,_p) || l4d2_chainsaw(_s,_n,_p) || l4d2_katana(_s,_n,_p) || l4d2_cricketbat(_s,_n,_p))

//RIFLES
#define l4d2_m4(_s,_n,_p)			(_s == 32 && _n == 2562 && _p == 2018) 
#define l4d2_scar(_s,_n,_p)			(_s == 32 && _n == 4878 && _p == 3765) 
#define l4d2_ak47(_s,_n,_p)			(_s == 32 && _n == 3574 && _p == 2817)
#define l4d2_m60(_s,_n,_p)			(_s == 32 && _n == 6366 && _p == 4785) 
#define l4d2_sig552(_s,_n,_p)		(_s == 32 && _n == 573 && _p == 417)

#define l4d2_rifles(_s,_n,_p) (l4d2_m4(_s,_n,_p) || l4d2_scar(_s,_n,_p) || l4d2_ak47(_s,_n,_p) || l4d2_m60(_s,_n,_p) || l4d2_sig552(_s,_n,_p))

//SNIPERS
#define l4d2_awp(_s,_n,_p)			(_s == 32 && _n == 681 && _p == 523) 
#define l4d2_scout(_s,_n,_p)		(_s == 32 && _n == 606 && _p == 477) 
#define l4d2_military(_s,_n,_p)		(_s == 32 && _n == 4278 && _p == 4059)
#define l4d2_huntingrifle(_s,_n,_p)	(_s == 32 && _n == 2514 && _p == 2069) 

#define l4d2_snipers(_s,_n,_p)		(l4d2_awp(_s,_n,_p) || l4d2_scout(_s,_n,_p) || l4d2_military(_s,_n,_p) || l4d2_huntingrifle(_s,_n,_p))

//GRENADELAUNCHER
#define l4d2_grenade(_s,_n,_p)		(_n == 2575 && _p == 2265) 

#define l4d2_weapons(_s,_n,_p)		(l4d2_guns(_s,_n,_p) || l4d2_shotguns(_s,_n,_p) || l4d2_smg(_s,_n,_p) || l4d2_melee(_s,_n,_p) || l4d2_rifles(_s,_n,_p) || l4d2_snipers(_s,_n,_p) || l4d2_grenade(_s,_n,_p))

//---------------
//----STUFF------
//---------------

#define l4d2_medical(_s,_n,_p)		(_s == 32 && _n == 883 && _p == 902) 
#define l4d2_adrenaline(_s,_n,_p)	(_s == 32 && _n == 2035 && _p == 1842) 
#define l4d2_defib(_s,_n,_p)		(_s == 32 && _n == 4901 && _p == 5885) 
#define l4d2_painpills(_s,_n,_p)	(_s == 32 && _n == 240 && _p == 282) //(cachet)
#define l4d2_gascan(_s,_n,_p)		(_s == 32 && _n == 855 && _p == 952) //gericane
#define l4d2_propanetank(_s,_n,_p)	(_s == 32 && _n == 1105 && _p == 1092) //bombone de prop
#define l4d2_oxygentank(_s,_n,_p)	(_s == 32 && _n == 1779 && _p == 1742) //bombone d'oxy
#define l4d2_fireworkbox(_s,_n,_p)	(_s == 32 && _n == 222 && _p == 334)
#define l4d2_pipebomb(_s,_n,_p)		(_s == 32 && _n == 2375 && _p == 1956) 
#define l4d2_bilebomb(_s,_n,_p)		(_s == 32 && ( (_n == 98 && _p == 192) || (_n == 1526 && _p == 1491) )) 
#define l4d2_molotov(_s,_n,_p)		(_s == 32 && ( (_n == 442 && _p == 576) || (_n == 200 && _p == 296) )) 
#define l4d2_cola(_s,_n,_p)			(_s == 32 && ( (_n == 1353 && _p == 1337) || (_n == 930 && _p == 1446) ))
#define l4d2_gnome(_s,_n,_p)		(_s == 32 && _n == 1648 && _p == 2088)
#define l4d2_explosif(_s,_n,_p)		(_s == 32 && _n == 815 && _p == 736) //explosif bullet box
#define l4d2_incendiary(_s,_n,_p)	(_s == 32 && _n == 821 && _p == 736) //incendiary bullet box

#define l4d2_upgradepack(_s,_n,_p)	(l4d2_explosif(_s,_n,_p) || l4d2_incendiary(_s,_n,_p))
#define l4d2_throw(_s,_n,_p)		(l4d2_pipebomb(_s,_n,_p) || l4d2_bilebomb(_s,_n,_p) || l4d2_molotov(_s,_n,_p))
#define l4d2_healthitem(_s,_n,_p)	(l4d2_medical(_s,_n,_p) || l4d2_adrenaline(_s,_n,_p) || l4d2_defib(_s,_n,_p) || l4d2_painpills(_s,_n,_p))
#define l4d2_carry(_s,_n,_p)		(l4d2_gascan(_s,_n,_p) || l4d2_propanetank(_s,_n,_p) || l4d2_oxygentank(_s,_n,_p) || l4d2_fireworkbox(_s,_n,_p) || l4d2_cola(_s,_n,_p) || l4d2_gnome(_s,_n,_p))
#define l4d2_stuff(_s,_n,_p)		(l4d2_upgradepack(_s,_n,_p) || l4d2_throw(_s,_n,_p) || l4d2_healthitem(_s,_n,_p) || l4d2_carry(_s,_n,_p))

//---------------
//----ZOMBIE-----
//---------------

//INFECTED
#define l4d2_zombie1body(_s,_n,_p)	(_s == 32 && _n == 2035 && _p == 3038)
#define l4d2_zombie1head(_s,_n,_p)	(_s == 32 && _n == 834 && _p == 1289) 
#define l4d2_zombie2body(_s,_n,_p)	(_s == 32 && _n == 1673 && _p == 2608) 
#define l4d2_zombie2head(_s,_n,_p)	(_s == 32 && _n == 854 && _p == 1389) 
#define l4d2_zombie3body(_s,_n,_p)	(_s == 32 && _n == 1732 && _p == 2581)

//(NOT WORKING) too many zombies models, too lazy to log all of them :p

#define l4d2_common(_s,_n,_p)		(l4d2_zombie1body(_s,_n,_p) || l4d2_zombie1head(_s,_n,_p) || l4d2_zombie2body(_s,_n,_p) || l4d2_zombie2head(_s,_n,_p) || l4d2_zombie3body(_s,_n,_p))

//SPECIAL ZOMBIE
#define l4d2_tank(_s,_n,_p)			(_s == 32 && _n == 3600 && _p == 5812) 
#define l4d2_smoker(_s,_n,_p)		(_s == 32 && _n == 6571 && _p == 11361) 
#define l4d2_hunter(_s,_n,_p)		(_s == 32 && _n == 6896 && _p == 11701) 
#define l4d2_boomer(_s,_n,_p)		(_s == 32 && _n == 3248 && _p == 5257) 
#define l4d2_charger(_s,_n,_p)		(_s == 32 && _n == 3522 && _p == 6071) 
#define l4d2_jockey(_s,_n,_p)		(_s == 32 && ( (_n == 6340 && _p == 10382) || (_n == 6340 && _p == 504) ))
#define l4d2_witch(_s,_n,_p)		(_s == 32 && ( (_n == 3362 && _p == 5099) || (_n == 488 && _p == 566) ))
#define l4d2_spitter(_s,_n,_p)		(_s == 32 && ( (_n == 4811 && _p == 5621) || (_n == 345 && _p == 534) || (_n == 4811 && _p == 2381) ))

#define l4d2_special(_s,_n,_p)		(l4d2_tank(_s,_n,_p) || l4d2_smoker(_s,_n,_p) || l4d2_hunter(_s,_n,_p) || l4d2_boomer(_s,_n,_p) || l4d2_charger(_s,_n,_p) || l4d2_jockey(_s,_n,_p) || l4d2_witch(_s,_n,_p) || l4d2_spitter(_s,_n,_p))

#define l4d2_zombies(_s,_n,_p)		(l4d2_common(_s,_n,_p) || l4d2_special(_s,_n,_p))

#define CVAR_MAKE_FLAGS(_s)				if(g_conVar[_s] != nullptr)\
{\
	if(g_conVar[_s]->IsFlagSet(FCVAR_CHEAT | FCVAR_SPONLY | FCVAR_REPLICATED | FCVAR_NOT_CONNECTED | FCVAR_SERVER_CAN_EXECUTE))\
		g_conVar[_s]->RemoveFlags(FCVAR_CHEAT | FCVAR_SPONLY | FCVAR_REPLICATED | FCVAR_NOT_CONNECTED | FCVAR_SERVER_CAN_EXECUTE);\
	if(!g_conVar[_s]->IsFlagSet(FCVAR_SERVER_CANNOT_QUERY))\
		g_conVar[_s]->AddFlags(FCVAR_SERVER_CANNOT_QUERY);\
}

#define CVAR_MAKE_VALUE(_s,_v1,_v2)		if(g_conVar[_s] != nullptr)\
{\
	if(g_conVar[_s]->GetInt() == _v1)\
		g_conVar[_s]->SetValue(_v2);\
	else\
		g_conVar[_s]->SetValue(_v1);\
	g_interface.Engine->ClientCmd("echo \"[ConVar] %s set %d\"", _s, g_conVar[_s]->GetInt());\
}

#define VMTHOOK_DESTORY(_v)		if(_v)_v->HookTable(false)
#define DETOURXS_DESTORY(_v)	if(_v && _v->Created())_v->Destroy()

#define NETPROP_GET_MAKE(_f,_t,_p,_r)	_r& _f()\
{\
	static int offset = g_pNetVars->GetOffset(_t, _p);\
	return *(_r*)(this + offset);\
}

#define NETPROP_TGET_MAKE(_f,_t,_p)	template<typename T> T& _f()\
{\
	static int offset = g_pNetVars->GetOffset(_t, _p);\
	return *(T*)(this + offset);\
}

#define NETPROP_SET_MAKE(_f,_t,_p,_r)	_r& _f(const _r& value)\
{\
	static int offset = g_pNetVars->GetOffset(_t, _p);\
	return (*(_r*)(this + offset) = value);\
}

#define NETPROP_TSET_MAKE(_f,_t,_p,_r)	template<typename T> T& _f(const T& value)\
{\
	static int offset = g_pNetVars->GetOffset(_t, _p);\
	return (*(T*)(this + offset) = value);\
}
