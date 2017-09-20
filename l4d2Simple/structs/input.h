#pragma once
#include "console.h"

class CBaseEntity;
enum ButtonCode_t;

struct CameraThirdData_t
{
	float	m_flPitch;
	float	m_flYaw;
	float	m_flDist;
	float	m_flLag;
	Vector	m_vecHullMin;
	Vector	m_vecHullMax;
};

struct kbutton_t
{
	// key nums holding it down
	int		down[2];
	// low bit is down state
	int		state;
};

class IInput
{
public:
	// Initialization/shutdown of the subsystem
	virtual	void		Init_All(void) = 0;
	virtual void		Shutdown_All(void) = 0;
	// Latching button states
	virtual int			GetButtonBits(int) = 0;
	// Create movement command
	virtual void		CreateMove(int sequence_number, float input_sample_frametime, bool active) = 0;
	virtual void		ExtraMouseSample(float frametime, bool active) = 0;
	virtual bool		WriteUsercmdDeltaToBuffer(bf_write *buf, int from, int to, bool isnewcommand) = 0;
	virtual void		EncodeUserCmdToBuffer(bf_write& buf, int slot) = 0;
	virtual void		DecodeUserCmdFromBuffer(bf_read& buf, int slot) = 0;

	virtual CUserCmd	*GetUserCmd(int givezero, int sequence_number) = 0;

	virtual void		MakeWeaponSelection(CBaseEntity* *weapon) = 0;

	// Retrieve key state
	virtual float		KeyState(kbutton_t *key) = 0;
	// Issue key event
	virtual int			KeyEvent(int eventcode, ButtonCode_t keynum, const char* pszCurrentBinding) = 0;

	// Look for key
	virtual kbutton_t	*FindKey(const char *name) = 0;

	// Issue commands from controllers
	virtual void		ControllerCommands(void) = 0;
	// Extra initialization for some joysticks
	virtual void		Joystick_Advanced(void) = 0;
	virtual void		Joystick_SetSampleTime(float frametime) = 0;
	virtual void		IN_SetSampleTime(float frametime) = 0;

	// Accumulate mouse delta
	virtual void		AccumulateMouse(void) = 0;
	// Activate/deactivate mouse
	virtual void		ActivateMouse(void) = 0;
	virtual void		DeactivateMouse(void) = 0;

	// Clear mouse state data
	virtual void		ClearStates(void) = 0;
	// Retrieve lookspring setting
	virtual float		GetLookSpring(void) = 0;

	// Retrieve mouse position
	virtual void		GetFullscreenMousePos(int *mx, int *my, int *unclampedx = 0, int *unclampedy = 0) = 0;
	virtual void		SetFullscreenMousePos(int mx, int my) = 0;
	virtual void		ResetMouse(void) = 0;
	virtual	float		GetLastForwardMove(void) = 0;
	virtual	float		Joystick_GetForward(void) = 0;
	virtual	float		Joystick_GetSide(void) = 0;
	virtual	float		Joystick_GetPitch(void) = 0;
	virtual	float		Joystick_GetYaw(void) = 0;

	// Third Person camera ( TODO/FIXME:  Move this to a separate interface? )
	virtual void		CAM_Think(void) = 0;
	virtual int			CAM_IsThirdPerson(void) = 0;
	virtual void		CAM_ToThirdPerson(void) = 0;
	virtual void		CAM_ToFirstPerson(void) = 0;
	virtual void		CAM_StartMouseMove(void) = 0;
	virtual void		CAM_EndMouseMove(void) = 0;
	virtual void		CAM_StartDistance(void) = 0;
	virtual void		CAM_EndDistance(void) = 0;
	virtual int			CAM_InterceptingMouse(void) = 0;

	// orthographic camera info	( TODO/FIXME:  Move this to a separate interface? )
	virtual void		CAM_ToOrthographic() = 0;
	virtual	bool		CAM_IsOrthographic() const = 0;
	virtual	void		CAM_OrthographicSize(float& w, float& h) const = 0;

	virtual void		LevelInit(void) = 0;

	// Causes an input to have to be re-pressed to become active
	virtual void		ClearInputButton(int bits) = 0;

	virtual	void		CAM_SetCameraThirdData(CameraThirdData_t *pCameraData, const QAngle &vecCameraOffset) = 0;
	virtual void		CAM_CameraThirdThink(void) = 0;

	virtual	bool		EnableJoystickMode() = 0;
};

class CInput : public IInput
{
public:
	virtual CUserCmd *GetUserCmd(int givezero, int sequence_number) override
	{
		typedef CUserCmd*(__thiscall* Fn)(void*, int, int);
		return ((Fn)VMT.GetFunction(this, indexes::GetUserCmd))(this, 0, sequence_number);
	}
	
	virtual void CAM_ToThirdPerson() override
	{
		typedef void(__thiscall* Fn)(void*);
		return ((Fn)VMT.GetFunction(this, indexes::CAM_ToThirdPerson))(this);
	}
	
	virtual void CAM_ToFirstPerson() override
	{
		typedef void(__thiscall* Fn)(void*);
		return ((Fn)VMT.GetFunction(this, indexes::CAM_ToFirstPerson))(this);
	}

	virtual int CAM_IsThirdPerson() override
	{
		typedef int(__thiscall* Fn)(void*);
		return ((Fn)VMT.GetFunction(this, indexes::CAM_IsThirdPerson))(this);
	}
};

class CGameMovement
{
public:
	void ProcessMovement(CBaseEntity *pPlayer, PVOID moveData)
	{
		typedef void(__thiscall* OriginalFn)(PVOID, CBaseEntity *pPlayer, PVOID moveData);
		VMT.getvfunc<OriginalFn>(this, indexes::ProccessMovement)(this, pPlayer, moveData);
	}
};

class CMoveHelper
{
public:
	void SetHost(CBaseEntity *pPlayer)
	{
		typedef void(__thiscall* OriginalFn)(PVOID, CBaseEntity *pPlayer);
		VMT.getvfunc<OriginalFn>(this, indexes::SetHost)(this, pPlayer);
	}
};

class CMoveData
{
public:
	bool			m_bFirstRunOfFunctions : 1;
	bool			m_bGameCodeMovedPlayer : 1;
	bool			m_bNoAirControl : 1;

	CBaseHandle		m_nPlayerHandle;	// edict index on server, client entity handle on client

	int				m_nImpulseCommand;	// Impulse command issued.
	QAngle			m_vecViewAngles;	// Command view angles (local space)
	QAngle			m_vecAbsViewAngles;	// Command view angles (world space)
	int				m_nButtons;			// Attack buttons.
	int				m_nOldButtons;		// From host_client->oldbuttons;
	float			m_flForwardMove;
	float			m_flSideMove;
	float			m_flUpMove;

	float			m_flMaxSpeed;
	float			m_flClientMaxSpeed;

	// Variables from the player edict (sv_player) or entvars on the client.
	// These are copied in here before calling and copied out after calling.
	Vector			m_vecVelocity;		// edict::velocity		// Current movement direction.
	Vector			m_vecOldVelocity;
	float			somefloat;
	QAngle			m_vecAngles;		// edict::angles
	QAngle			m_vecOldAngles;

	// Output only
	float			m_outStepHeight;	// how much you climbed this move
	Vector			m_outWishVel;		// This is where you tried 
	Vector			m_outJumpVel;		// This is your jump velocity

	// Movement constraints	(radius 0 means no constraint)
	Vector			m_vecConstraintCenter;
	float			m_flConstraintRadius;
	float			m_flConstraintWidth;
	float			m_flConstraintSpeedFactor;
	bool			m_bConstraintPastRadius;

	void			SetAbsOrigin(const Vector &vec);
	const Vector	&GetAbsOrigin() const;

public:
	Vector			m_vecAbsOrigin;		// edict::origin
};

class IGameMovement
{
public:
	virtual			~IGameMovement(void) {}

	// Process the current movement command
	virtual void	ProcessMovement(CBaseEntity *pPlayer, CMoveData *pMove) = 0;
	virtual void	Reset(void) = 0;
	virtual void	StartTrackPredictionErrors(CBaseEntity *pPlayer) = 0;
	virtual void	FinishTrackPredictionErrors(CBaseEntity *pPlayer) = 0;
	virtual void	DiffPrint(char const *fmt, ...) = 0;

	// Allows other parts of the engine to find out the normal and ducked player bbox sizes
	virtual Vector	GetPlayerMins(bool ducked) const = 0;
	virtual Vector	GetPlayerMaxs(bool ducked) const = 0;
	virtual Vector  GetPlayerViewOffset(bool ducked) const = 0;

	virtual bool	IsMovingPlayerStuck(void) const = 0;
	virtual CBaseEntity*	GetMovingPlayer(void) const = 0;
	virtual void	UnblockPusher(CBaseEntity* pPlayer, CBaseEntity *pPusher) = 0;

	virtual void	SetupMovementBounds(CMoveData *pMove) = 0;
};

class CPrediction
{
public:
	void SetupMove(CBaseEntity *player, CUserCmd *ucmd, CMoveHelper* movehelper, CMoveData* moveData)
	{
		typedef void(__thiscall* OriginalFn)(PVOID, CBaseEntity*, CUserCmd*, CMoveHelper*, CMoveData*);
		VMT.getvfunc<OriginalFn>(this, indexes::SetupMove)(this, player, ucmd, movehelper, moveData);
	}

	void FinishMove(CBaseEntity *player, CUserCmd *ucmd, CMoveData* moveData)
	{
		typedef void(__thiscall* OriginalFn)(PVOID, CBaseEntity*, CUserCmd*, CMoveData*);
		VMT.getvfunc<OriginalFn>(this, indexes::FinishMove)(this, player, ucmd, moveData);
	}

	/*
	void StartPrediction(CUserCmd* pCmd)
	{
		CBaseEntity* client = GetLocalClient();
		if (g_interface.MoveHelper == nullptr || pCmd == nullptr || client == nullptr ||
			m_bPredictionRunning)
			return;

		m_bPredictionRunning = true;
		m_flOldCurtime = g_interface.Globals->curtime;
		m_flOldFrametime = g_interface.Globals->frametime;
		m_flOldFrametime = client->GetNetProp<int>("m_fFlags", "DT_BasePlayer");

		g_interface.Globals->curtime = client->GetTickBase() * g_interface.Globals->interval_per_tick;
		g_interface.Globals->frametime = g_interface.Globals->interval_per_tick;
		g_interface.GameMovement->StartTrackPredictionErrors(client);

		ZeroMemory(&m_MoveData, sizeof(m_MoveData));
		g_interface.MoveHelper->SetHost(client);
		g_interface.Prediction->SetupMove(client, pCmd, nullptr, &g_predMoveData);
		g_interface.GameMovement->ProcessMovement(client, &g_predMoveData);
		g_interface.Prediction->FinishMove(client, pCmd, &g_predMoveData);
	}

	void EndPrediction(CUserCmd* pCmd)
	{
		CBaseEntity* client = GetLocalClient();
		if (g_interface.MoveHelper == nullptr || pCmd == nullptr || client == nullptr ||
			!m_bPredictionRunning)
			return;

		g_interface.GameMovement->FinishTrackPredictionErrors(client);
		g_interface.MoveHelper->SetHost(nullptr);
		g_interface.Globals->curtime = m_flOldCurtime;
		g_interface.Globals->frametime = m_flOldFrametime;
		client->SetNetProp("m_fFlags", m_iOldFlags, "DT_BasePlayer");

		m_bPredictionRunning = false;
	}

private:
	float m_flOldCurtime;
	float m_flOldFrametime;
	int m_iOldFlags;
	CMoveData m_MoveData;
	bool m_bPredictionRunning = false;
	*/
};

class ClientModeShared
{
public:
	void* GetHudChat()
	{
		return *(void**)(this + 24);
	}

	void Printf(int iPlayerIndex, int unknown, const char* fmt, ...)
	{
		static void* m_pChatElement = this->GetHudChat();
		if (m_pChatElement == nullptr)
			return;

		char buffer[1024];

		va_list ap;
		va_start(ap, fmt);
		vsprintf_s(buffer, fmt, ap);
		va_end(ap);

		typedef void(__cdecl* Fn)(int, int, const char*, ...);
		VMT.getvfunc<Fn>(m_pChatElement, indexes::Printf)(iPlayerIndex, unknown, buffer);
	}

	void ChatPrintf(int iPlayerIndex, int iFilter, int unknown, const char* fmt, ...)
	{
		static void* m_pChatElement = this->GetHudChat();
		if (m_pChatElement == nullptr)
			return;

		char buffer[1024];

		va_list ap;
		va_start(ap, fmt);
		vsprintf_s(buffer, fmt, ap);
		va_end(ap);

		typedef void(__cdecl* Fn)(int, int, int, const char*, ...);
		VMT.getvfunc<Fn>(m_pChatElement, indexes::ChatPrintf)(iPlayerIndex, iFilter, unknown, buffer);
	}

};

inline const Vector &CMoveData::GetAbsOrigin() const
{
	return m_vecAbsOrigin;
}

inline void CMoveData::SetAbsOrigin(const Vector & vec)
{
	m_vecAbsOrigin = vec;
}

enum JoystickAxis_t
{
	JOY_AXIS_X = 0,
	JOY_AXIS_Y,
	JOY_AXIS_Z,
	JOY_AXIS_R,
	JOY_AXIS_U,
	JOY_AXIS_V,
	MAX_JOYSTICK_AXES,
};

enum
{
	JOYSTICK_MAX_BUTTON_COUNT = 32,
	JOYSTICK_POV_BUTTON_COUNT = 4,
	JOYSTICK_AXIS_BUTTON_COUNT = MAX_JOYSTICK_AXES * 2,
};

enum
{
	MAX_JOYSTICKS = 1,
	MOUSE_BUTTON_COUNT = 5,
	MAX_NOVINT_DEVICES = 2,
};

#define JOYSTICK_BUTTON_INTERNAL( _joystick, _button ) ( JOYSTICK_FIRST_BUTTON + ((_joystick) * JOYSTICK_MAX_BUTTON_COUNT) + (_button) )
#define JOYSTICK_POV_BUTTON_INTERNAL( _joystick, _button ) ( JOYSTICK_FIRST_POV_BUTTON + ((_joystick) * JOYSTICK_POV_BUTTON_COUNT) + (_button) )
#define JOYSTICK_AXIS_BUTTON_INTERNAL( _joystick, _button ) ( JOYSTICK_FIRST_AXIS_BUTTON + ((_joystick) * JOYSTICK_AXIS_BUTTON_COUNT) + (_button) )

#define JOYSTICK_BUTTON( _joystick, _button ) ( (ButtonCode_t)JOYSTICK_BUTTON_INTERNAL( _joystick, _button ) )
#define JOYSTICK_POV_BUTTON( _joystick, _button ) ( (ButtonCode_t)JOYSTICK_POV_BUTTON_INTERNAL( _joystick, _button ) )
#define JOYSTICK_AXIS_BUTTON( _joystick, _button ) ( (ButtonCode_t)JOYSTICK_AXIS_BUTTON_INTERNAL( _joystick, _button ) )

enum ButtonCode_t
{
	BUTTON_CODE_INVALID = -1,
	BUTTON_CODE_NONE = 0,

	KEY_FIRST = 0,

	KEY_NONE = KEY_FIRST,
	KEY_0,
	KEY_1,
	KEY_2,
	KEY_3,
	KEY_4,
	KEY_5,
	KEY_6,
	KEY_7,
	KEY_8,
	KEY_9,
	KEY_A,
	KEY_B,
	KEY_C,
	KEY_D,
	KEY_E,
	KEY_F,
	KEY_G,
	KEY_H,
	KEY_I,
	KEY_J,
	KEY_K,
	KEY_L,
	KEY_M,
	KEY_N,
	KEY_O,
	KEY_P,
	KEY_Q,
	KEY_R,
	KEY_S,
	KEY_T,
	KEY_U,
	KEY_V,
	KEY_W,
	KEY_X,
	KEY_Y,
	KEY_Z,
	KEY_PAD_0,
	KEY_PAD_1,
	KEY_PAD_2,
	KEY_PAD_3,
	KEY_PAD_4,
	KEY_PAD_5,
	KEY_PAD_6,
	KEY_PAD_7,
	KEY_PAD_8,
	KEY_PAD_9,
	KEY_PAD_DIVIDE,
	KEY_PAD_MULTIPLY,
	KEY_PAD_MINUS,
	KEY_PAD_PLUS,
	KEY_PAD_ENTER,
	KEY_PAD_DECIMAL,
	KEY_LBRACKET,
	KEY_RBRACKET,
	KEY_SEMICOLON,
	KEY_APOSTROPHE,
	KEY_BACKQUOTE,
	KEY_COMMA,
	KEY_PERIOD,
	KEY_SLASH,
	KEY_BACKSLASH,
	KEY_MINUS,
	KEY_EQUAL,
	KEY_ENTER,
	KEY_SPACE,
	KEY_BACKSPACE,
	KEY_TAB,
	KEY_CAPSLOCK,
	KEY_NUMLOCK,
	KEY_ESCAPE,
	KEY_SCROLLLOCK,
	KEY_INSERT,
	KEY_DELETE,
	KEY_HOME,
	KEY_END,
	KEY_PAGEUP,
	KEY_PAGEDOWN,
	KEY_BREAK,
	KEY_LSHIFT,
	KEY_RSHIFT,
	KEY_LALT,
	KEY_RALT,
	KEY_LCONTROL,
	KEY_RCONTROL,
	KEY_LWIN,
	KEY_RWIN,
	KEY_APP,
	KEY_UP,
	KEY_LEFT,
	KEY_DOWN,
	KEY_RIGHT,
	KEY_F1,
	KEY_F2,
	KEY_F3,
	KEY_F4,
	KEY_F5,
	KEY_F6,
	KEY_F7,
	KEY_F8,
	KEY_F9,
	KEY_F10,
	KEY_F11,
	KEY_F12,
	KEY_CAPSLOCKTOGGLE,
	KEY_NUMLOCKTOGGLE,
	KEY_SCROLLLOCKTOGGLE,

	KEY_LAST = KEY_SCROLLLOCKTOGGLE,
	KEY_COUNT = KEY_LAST - KEY_FIRST + 1,

	// Mouse
	MOUSE_FIRST = KEY_LAST + 1,

	MOUSE_LEFT = MOUSE_FIRST,
	MOUSE_RIGHT,
	MOUSE_MIDDLE,
	MOUSE_4,
	MOUSE_5,
	MOUSE_WHEEL_UP,		// A fake button which is 'pressed' and 'released' when the wheel is moved up 
	MOUSE_WHEEL_DOWN,	// A fake button which is 'pressed' and 'released' when the wheel is moved down

	MOUSE_LAST = MOUSE_WHEEL_DOWN,
	MOUSE_COUNT = MOUSE_LAST - MOUSE_FIRST + 1,

	// Joystick
	JOYSTICK_FIRST = MOUSE_LAST + 1,

	JOYSTICK_FIRST_BUTTON = JOYSTICK_FIRST,
	JOYSTICK_LAST_BUTTON = JOYSTICK_BUTTON_INTERNAL(MAX_JOYSTICKS - 1, JOYSTICK_MAX_BUTTON_COUNT - 1),
	JOYSTICK_FIRST_POV_BUTTON,
	JOYSTICK_LAST_POV_BUTTON = JOYSTICK_POV_BUTTON_INTERNAL(MAX_JOYSTICKS - 1, JOYSTICK_POV_BUTTON_COUNT - 1),
	JOYSTICK_FIRST_AXIS_BUTTON,
	JOYSTICK_LAST_AXIS_BUTTON = JOYSTICK_AXIS_BUTTON_INTERNAL(MAX_JOYSTICKS - 1, JOYSTICK_AXIS_BUTTON_COUNT - 1),

	JOYSTICK_LAST = JOYSTICK_LAST_AXIS_BUTTON,

	BUTTON_CODE_LAST,
	BUTTON_CODE_COUNT = BUTTON_CODE_LAST - KEY_FIRST + 1,

	// Helpers for XBox 360
	KEY_XBUTTON_UP = JOYSTICK_FIRST_POV_BUTTON,	// POV buttons
	KEY_XBUTTON_RIGHT,
	KEY_XBUTTON_DOWN,
	KEY_XBUTTON_LEFT,

	KEY_XBUTTON_A = JOYSTICK_FIRST_BUTTON,		// Buttons
	KEY_XBUTTON_B,
	KEY_XBUTTON_X,
	KEY_XBUTTON_Y,
	KEY_XBUTTON_LEFT_SHOULDER,
	KEY_XBUTTON_RIGHT_SHOULDER,
	KEY_XBUTTON_BACK,
	KEY_XBUTTON_START,
	KEY_XBUTTON_STICK1,
	KEY_XBUTTON_STICK2,

	KEY_XSTICK1_RIGHT = JOYSTICK_FIRST_AXIS_BUTTON,	// XAXIS POSITIVE
	KEY_XSTICK1_LEFT,							// XAXIS NEGATIVE
	KEY_XSTICK1_DOWN,							// YAXIS POSITIVE
	KEY_XSTICK1_UP,								// YAXIS NEGATIVE
	KEY_XBUTTON_LTRIGGER,						// ZAXIS POSITIVE
	KEY_XBUTTON_RTRIGGER,						// ZAXIS NEGATIVE
	KEY_XSTICK2_RIGHT,							// UAXIS POSITIVE
	KEY_XSTICK2_LEFT,							// UAXIS NEGATIVE
	KEY_XSTICK2_DOWN,							// VAXIS POSITIVE
	KEY_XSTICK2_UP,								// VAXIS NEGATIVE
};

enum MouseCodeState_t
{
	BUTTON_RELEASED = 0,
	BUTTON_PRESSED,
	BUTTON_DOUBLECLICKED,
};

enum InputEventType_t
{
	IE_ButtonPressed = 0,	// m_nData contains a ButtonCode_t
	IE_ButtonReleased,		// m_nData contains a ButtonCode_t
	IE_ButtonDoubleClicked,	// m_nData contains a ButtonCode_t
	IE_AnalogValueChanged,	// m_nData contains an AnalogCode_t, m_nData2 contains the value

	IE_FirstSystemEvent = 100,
	IE_Quit = IE_FirstSystemEvent,
	IE_ControllerInserted,	// m_nData contains the controller ID
	IE_ControllerUnplugged,	// m_nData contains the controller ID

	IE_FirstVguiEvent = 1000,	// Assign ranges for other systems that post user events here
	IE_FirstAppEvent = 2000,
};

struct InputEvent_t
{
	int m_nType;				// Type of the event (see InputEventType_t)
	int m_nTick;				// Tick on which the event occurred
	int m_nData;				// Generic 32-bit data, what it contains depends on the event
	int m_nData2;				// Generic 32-bit data, what it contains depends on the event
	int m_nData3;				// Generic 32-bit data, what it contains depends on the event
};

class IInputInternal : public IInput
{
public:
	// processes input for a frame
	virtual void RunFrame() = 0;

	virtual void UpdateMouseFocus(int x, int y) = 0;

	// called when a panel becomes invalid
	virtual void PanelDeleted(unsigned int panel) = 0;

	// inputs into vgui input handling 
	virtual bool InternalCursorMoved(int x, int y) = 0; //expects input in surface space
	virtual bool InternalMousePressed(ButtonCode_t code) = 0;
	virtual bool InternalMouseDoublePressed(ButtonCode_t code) = 0;
	virtual bool InternalMouseReleased(ButtonCode_t code) = 0;
	virtual bool InternalMouseWheeled(int delta) = 0;
	virtual bool InternalKeyCodePressed(ButtonCode_t code) = 0;
	virtual void InternalKeyCodeTyped(ButtonCode_t code) = 0;
	virtual void InternalKeyTyped(wchar_t unichar) = 0;
	virtual bool InternalKeyCodeReleased(ButtonCode_t code) = 0;

	// Creates/ destroys "input" contexts, which contains information
	// about which controls have mouse + key focus, for example.
	virtual int CreateInputContext() = 0;
	virtual void DestroyInputContext(int context) = 0;

	// Associates a particular panel with an input context
	// Associating NULL is valid; it disconnects the panel from the context
	virtual void AssociatePanelWithInputContext(int context, unsigned int pRoot) = 0;

	// Activates a particular input context, use DEFAULT_INPUT_CONTEXT
	// to get the one normally used by VGUI
	virtual void ActivateInputContext(int context) = 0;

	// This method is called to post a cursor message to the current input context
	virtual void PostCursorMessage() = 0;

	// Cursor position; this is the current position read from the input queue.
	// We need to set it because client code may read this during Mouse Pressed
	// events, etc.
	virtual void UpdateCursorPosInternal(int x, int y) = 0;

	// Called to handle explicit calls to CursorSetPos after input processing is complete
	virtual void HandleExplicitSetCursor() = 0;

	// Updates the internal key/mouse state associated with the current input context without sending messages
	virtual void SetKeyCodeState(ButtonCode_t code, bool bPressed) = 0;
	virtual void SetMouseCodeState(ButtonCode_t code, MouseCodeState_t state) = 0;
	virtual void UpdateButtonState(const InputEvent_t &event) = 0;
};

#define JOYSTICK_AXIS_INTERNAL( _joystick, _axis ) ( JOYSTICK_FIRST_AXIS + ((_joystick) * MAX_JOYSTICK_AXES) + (_axis) )
#define JOYSTICK_AXIS( _joystick, _axis ) ( (AnalogCode_t)JOYSTICK_AXIS_INTERNAL( _joystick, _axis ) )
enum AnalogCode_t
{
	ANALOG_CODE_INVALID = -1,
	MOUSE_X = 0,
	MOUSE_Y,
	MOUSE_XY,		// Invoked when either x or y changes
	MOUSE_WHEEL,

	JOYSTICK_FIRST_AXIS,
	JOYSTICK_LAST_AXIS = JOYSTICK_AXIS_INTERNAL(MAX_JOYSTICKS - 1, MAX_JOYSTICK_AXES - 1),

	ANALOG_CODE_LAST,
};

class IInputSystem : public IAppSystem
{
public:
	// Attach, detach input system from a particular window
	// This window should be the root window for the application
	// Only 1 window should be attached at any given time.
	virtual void AttachToWindow(void* hWnd) = 0;
	virtual void DetachFromWindow() = 0;

	// Enables/disables input. PollInputState will not update current 
	// button/analog states when it is called if the system is disabled.
	virtual void EnableInput(bool bEnable) = 0;

	// Enables/disables the windows message pump. PollInputState will not
	// Peek/Dispatch messages if this is disabled
	virtual void EnableMessagePump(bool bEnable) = 0;

	// Polls the current input state
	virtual void PollInputState() = 0;

	// Gets the time of the last polling in ms
	virtual int GetPollTick() const = 0;

	// Is a button down? "Buttons" are binary-state input devices (mouse buttons, keyboard keys)
	virtual bool IsButtonDown(ButtonCode_t code) const = 0;

	// Returns the tick at which the button was pressed and released
	virtual int GetButtonPressedTick(ButtonCode_t code) const = 0;
	virtual int GetButtonReleasedTick(ButtonCode_t code) const = 0;

	// Gets the value of an analog input device this frame
	// Includes joysticks, mousewheel, mouse
	virtual int GetAnalogValue(AnalogCode_t code) const = 0;

	// Gets the change in a particular analog input device this frame
	// Includes joysticks, mousewheel, mouse
	virtual int GetAnalogDelta(AnalogCode_t code) const = 0;

	// Returns the input events since the last poll
	virtual int GetEventCount() const = 0;
	virtual const InputEvent_t* GetEventData() const = 0;

	// Posts a user-defined event into the event queue; this is expected
	// to be called in overridden wndprocs connected to the root panel.
	virtual void PostUserEvent(const InputEvent_t &event) = 0;

	// Returns the number of joysticks
	virtual int GetJoystickCount() const = 0;

	// Enable/disable joystick, it has perf costs
	virtual void EnableJoystickInput(int nJoystick, bool bEnable) = 0;

	// Enable/disable diagonal joystick POV (simultaneous POV buttons down)
	virtual void EnableJoystickDiagonalPOV(int nJoystick, bool bEnable) = 0;

	// Sample the joystick and append events to the input queue
	virtual void SampleDevices(void) = 0;

	// FIXME: Currently force-feedback is only supported on the Xbox 360
	virtual void SetRumble(float fLeftMotor, float fRightMotor, int userId = -1) = 0;
	virtual void StopRumble(void) = 0;

	// Resets the input state
	virtual void ResetInputState() = 0;

	// Sets a player as the primary user - all other controllers will be ignored.
	virtual void SetPrimaryUserId(int userId) = 0;

	// Convert back + forth between ButtonCode/AnalogCode + strings
	virtual const char *ButtonCodeToString(ButtonCode_t code) const = 0;
	virtual const char *AnalogCodeToString(AnalogCode_t code) const = 0;
	virtual ButtonCode_t StringToButtonCode(const char *pString) const = 0;
	virtual AnalogCode_t StringToAnalogCode(const char *pString) const = 0;

	// Sleeps until input happens. Pass a negative number to sleep infinitely
	virtual void SleepUntilInput(int nMaxSleepTimeMS = -1) = 0;

	// Convert back + forth between virtual codes + button codes
	// FIXME: This is a temporary piece of code
	virtual ButtonCode_t VirtualKeyToButtonCode(int nVirtualKey) const = 0;
	virtual int ButtonCodeToVirtualKey(ButtonCode_t code) const = 0;
	virtual ButtonCode_t ScanCodeToButtonCode(int lParam) const = 0;

	// How many times have we called PollInputState?
	virtual int GetPollCount() const = 0;

	// Sets the cursor position
	virtual void SetCursorPosition(int x, int y) = 0;

	// NVNT get address to haptics interface
	virtual void *GetHapticsInterfaceAddress() const = 0;

	virtual void SetNovintPure(bool bPure) = 0;

	// read and clear accumulated raw input values
	virtual bool GetRawMouseAccumulators(int& accumX, int& accumY) = 0;

	// tell the input system that we're not a game, we're console text mode.
	// this is used for dedicated servers to not initialize joystick system.
	// this needs to be called before CInputSystem::Init (e.g. in PreInit of
	// some system) if you want ot prevent the joystick system from ever
	// being initialized.
	virtual void SetConsoleTextMode(bool bConsoleTextMode) = 0;
};

