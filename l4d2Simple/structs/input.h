#pragma once

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

	virtual CUserCmd	*GetUserCmd(int sequence_number) = 0;

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
	virtual CUserCmd *GetUserCmd(int sequence_number) override
	{
		typedef CUserCmd*(__thiscall* Fn)(void*, int, int);
		return ((Fn)VMT.GetFunction(this, indexes::GetUserCmd))(this, 0, sequence_number);
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

class CPrediction
{
public:
	void SetupMove(CBaseEntity *player, CUserCmd *ucmd, PVOID movehelper, PVOID moveData)
	{
		typedef void(__thiscall* OriginalFn)(PVOID, CBaseEntity*, CUserCmd*, PVOID, PVOID);
		VMT.getvfunc<OriginalFn>(this, indexes::SetupMove)(this, player, ucmd, movehelper, moveData);
	}

	void FinishMove(CBaseEntity *player, CUserCmd *ucmd, PVOID moveData)
	{
		typedef void(__thiscall* OriginalFn)(PVOID, CBaseEntity*, CUserCmd*, PVOID);
		VMT.getvfunc<OriginalFn>(this, indexes::FinishMove)(this, player, ucmd, moveData);
	}
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

