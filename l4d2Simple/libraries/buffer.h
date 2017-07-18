#pragma once

template <typename T>
inline T WordSwapAsm(T w)
{
	__asm
	{
		mov ax, w
		xchg al, ah
	}
}

template <typename T>
inline T DWordSwapAsm(T dw)
{
	__asm
	{
		mov eax, dw
		bswap eax
	}
}

#define IsPC()	true
#define BigShort(val) WordSwap(val)
#define BigWord(val) WordSwap(val)
#define BigLong(val) DWordSwap(val)
#define BigDWord(val) DWordSwap(val)
#define LittleShort(val) (val)
#define LittleWord(val) (val)
#define LittleLong(val) (val)
#define LittleDWord(val) (val)
#define LittleQWord(val) (val)
#define SwapShort(val) BigShort(val)
#define SwapWord(val) BigWord(val)
#define SwapLong(val) BigLong(val)
#define SwapDWord(val) BigDWord(val)

// Pass floats by pointer for swapping to avoid truncation in the fpu
#define BigFloat(pOut, pIn) SafeSwapFloat(pOut, pIn)
#define LittleFloat(pOut, pIn) (*pOut = *pIn)
#define SwapFloat(pOut, pIn) BigFloat(pOut, pIn)

__forceinline unsigned long LoadLittleDWord(const unsigned long *base, unsigned int dwordIndex)
{
	return LittleDWord(base[dwordIndex]);
}

__forceinline void StoreLittleDWord(unsigned long *base, unsigned int dwordIndex, unsigned long dword)
{
	base[dwordIndex] = LittleDWord(dword);
}

typedef enum
{
	BITBUFERROR_VALUE_OUT_OF_RANGE = 0,		// Tried to write a value with too few bits.
	BITBUFERROR_BUFFER_OVERRUN,				// Was about to overrun a buffer.

	BITBUFERROR_NUM_ERRORS
} BitBufErrorType;

typedef void(*BitBufErrorHandler)(BitBufErrorType errorType, const char *pDebugName);

#if defined( _DEBUG )
extern void InternalBitBufErrorHandler(BitBufErrorType errorType, const char *pDebugName);
#define CallErrorHandler( errorType, pDebugName ) InternalBitBufErrorHandler( errorType, pDebugName );
#else
#define CallErrorHandler( errorType, pDebugName ) ((void)0)
#endif

// Use this to install the error handler. Call with NULL to uninstall your error handler.
void SetBitBufErrorHandler(BitBufErrorHandler fn);

inline int BitByte(int bits)
{
	// return PAD_NUMBER( bits, 8 ) >> 3;
	return (bits + 7) >> 3;
}

namespace bitbuf
{
	// ZigZag Transform:  Encodes signed integers so that they can be
	// effectively used with varint encoding.
	//
	// varint operates on unsigned integers, encoding smaller numbers into
	// fewer bytes.  If you try to use it on a signed integer, it will treat
	// this number as a very large unsigned integer, which means that even
	// small signed numbers like -1 will take the maximum number of bytes
	// (10) to encode.  ZigZagEncode() maps signed integers to unsigned
	// in such a way that those with a small absolute value will have smaller
	// encoded values, making them appropriate for encoding using varint.
	//
	//       int32 ->     uint32
	// -------------------------
	//           0 ->          0
	//          -1 ->          1
	//           1 ->          2
	//          -2 ->          3
	//         ... ->        ...
	//  2147483647 -> 4294967294
	// -2147483648 -> 4294967295
	//
	//        >> encode >>
	//        << decode <<

	inline uint32_t ZigZagEncode32(int32_t n)
	{
		// Note:  the right-shift must be arithmetic
		return(n << 1) ^ (n >> 31);
	}

	inline int32_t ZigZagDecode32(uint32_t n)
	{
		return(n >> 1) ^ -static_cast<int32_t>(n & 1);
	}

	inline uint64_t ZigZagEncode64(int64_t n)
	{
		// Note:  the right-shift must be arithmetic
		return(n << 1) ^ (n >> 63);
	}

	inline int64_t ZigZagDecode64(uint64_t n)
	{
		return(n >> 1) ^ -static_cast<int64_t>(n & 1);
	}

	const int kMaxVarintBytes = 10;
	const int kMaxVarint32Bytes = 5;
}

//-----------------------------------------------------------------------------
// Used for serialization
//-----------------------------------------------------------------------------

class bf_write
{
public:
	bf_write();

	// nMaxBits can be used as the number of bits in the buffer. 
	// It must be <= nBytes*8. If you leave it at -1, then it's set to nBytes * 8.
	bf_write(void *pData, int nBytes, int nMaxBits = -1);
	bf_write(const char *pDebugName, void *pData, int nBytes, int nMaxBits = -1);

	// Start writing to the specified buffer.
	// nMaxBits can be used as the number of bits in the buffer. 
	// It must be <= nBytes*8. If you leave it at -1, then it's set to nBytes * 8.
	void			StartWriting(void *pData, int nBytes, int iStartBit = 0, int nMaxBits = -1);

	// Restart buffer writing.
	void			Reset();

	// Get the base pointer.
	unsigned char  *GetBasePointer() { return (unsigned char*)m_pData; }

	// Enable or disable assertion on overflow. 99% of the time, it's a bug that we need to catch,
	// but there may be the occasional buffer that is allowed to overflow gracefully.
	void			SetAssertOnOverflow(bool bAssert);

	// This can be set to assign a name that gets output if the buffer overflows.
	const char		*GetDebugName();
	void			SetDebugName(const char *pDebugName);


	// Seek to a specific position.
public:

	void			SeekToBit(int bitPos);


	// Bit functions.
public:

	void			WriteOneBit(int nValue);
	void			WriteOneBitNoCheck(int nValue);
	void			WriteOneBitAt(int iBit, int nValue);

	// Write signed or unsigned. Range is only checked in debug.
	void			WriteUBitLong(unsigned int data, int numbits, bool bCheckRange = true);
	void			WriteSBitLong(int data, int numbits);

	// Tell it whether or not the data is unsigned. If it's signed,
	// cast to unsigned before passing in (it will cast back inside).
	void			WriteBitLong(unsigned int data, int numbits, bool bSigned);

	// Write a list of bits in.
	bool			WriteBits(const void *pIn, int nBits);

	// writes an unsigned integer with variable bit length
	void			WriteUBitVar(unsigned int data);

	// writes a varint encoded integer
	void			WriteVarInt32(uint32_t data);
	void			WriteVarInt64(uint64_t data);
	void			WriteSignedVarInt32(int32_t data);
	void			WriteSignedVarInt64(int64_t data);
	int				ByteSizeVarInt32(uint32_t data);
	int				ByteSizeVarInt64(uint64_t data);
	int				ByteSizeSignedVarInt32(int32_t data);
	int				ByteSizeSignedVarInt64(int64_t data);

	// Copy the bits straight out of pIn. This seeks pIn forward by nBits.
	// Returns an error if this buffer or the read buffer overflows.
	bool			WriteBitsFromBuffer(class bf_read *pIn, int nBits);

	void			WriteBitAngle(float fAngle, int numbits);
	void			WriteBitCoord(const float f);
	void			WriteBitCoordMP(const float f, bool bIntegral, bool bLowPrecision);
	void			WriteBitFloat(float val);
	void			WriteBitVec3Coord(const Vector& fa);
	void			WriteBitNormal(float f);
	void			WriteBitVec3Normal(const Vector& fa);
	void			WriteBitAngles(const Vector& fa);

	// Byte functions.
public:

	void			WriteChar(int val);
	void			WriteByte(int val);
	void			WriteShort(int val);
	void			WriteWord(int val);
	void			WriteLong(long val);
	void			WriteLongLong(int64_t val);
	void			WriteFloat(float val);
	bool			WriteBytes(const void *pBuf, int nBytes);

	// Returns false if it overflows the buffer.
	bool			WriteString(const char *pStr);


	// Status.
public:

	// How many bytes are filled in?
	int				GetNumBytesWritten() const;
	int				GetNumBitsWritten() const;
	int				GetMaxNumBits();
	int				GetNumBitsLeft();
	int				GetNumBytesLeft();
	unsigned char   *GetData();
	const unsigned char *GetData() const;

	// Has the buffer overflowed?
	bool			CheckForOverflow(int nBits);
	inline bool		IsOverflowed() const { return m_bOverflow; }

	void			SetOverflowFlag();


public:
	// The current buffer.
	unsigned long  *m_pData;
	int				m_nDataBytes;
	int				m_nDataBits;

	// Where we are in the buffer.
	int				m_iCurBit;

private:

	// Errors?
	bool			m_bOverflow;

	bool			m_bAssertOnOverflow;
	const char		*m_pDebugName;
};

//-----------------------------------------------------------------------------
// Used for unserialization
//-----------------------------------------------------------------------------

class bf_read
{
public:
	bf_read();

	// nMaxBits can be used as the number of bits in the buffer. 
	// It must be <= nBytes*8. If you leave it at -1, then it's set to nBytes * 8.
	bf_read(const void *pData, int nBytes, int nBits = -1);
	bf_read(const char *pDebugName, const void *pData, int nBytes, int nBits = -1);

	// Start reading from the specified buffer.
	// pData's start address must be dword-aligned.
	// nMaxBits can be used as the number of bits in the buffer. 
	// It must be <= nBytes*8. If you leave it at -1, then it's set to nBytes * 8.
	void			StartReading(const void *pData, int nBytes, int iStartBit = 0, int nBits = -1);

	// Restart buffer reading.
	void			Reset();

	// Enable or disable assertion on overflow. 99% of the time, it's a bug that we need to catch,
	// but there may be the occasional buffer that is allowed to overflow gracefully.
	void			SetAssertOnOverflow(bool bAssert);

	// This can be set to assign a name that gets output if the buffer overflows.
	const char*		GetDebugName() const { return m_pDebugName; }
	void			SetDebugName(const char *pName);

	void			ExciseBits(int startbit, int bitstoremove);


	// Bit functions.
public:

	// Returns 0 or 1.
	int				ReadOneBit();


protected:

	unsigned int	CheckReadUBitLong(int numbits);		// For debugging.
	int				ReadOneBitNoCheck();				// Faster version, doesn't check bounds and is inlined.
	bool			CheckForOverflow(int nBits);


public:

	// Get the base pointer.
	const unsigned char *GetBasePointer() { return m_pData; }

	__forceinline int TotalBytesAvailable(void) const
	{
		return m_nDataBytes;
	}

	// Read a list of bits in.
	void            ReadBits(void *pOut, int nBits);
	// Read a list of bits in, but don't overrun the destination buffer.
	// Returns the number of bits read into the buffer. The remaining
	// bits are skipped over.
	int             ReadBitsClamped_ptr(void *pOut, size_t outSizeBytes, size_t nBits);
	// Helper 'safe' template function that infers the size of the destination
	// array. This version of the function should be preferred.
	// Usage: char databuffer[100];
	//        ReadBitsClamped( dataBuffer, msg->m_nLength );
	template <typename T, size_t N>
	int             ReadBitsClamped(T(&pOut)[N], size_t nBits)
	{
		return ReadBitsClamped_ptr(pOut, N * sizeof(T), nBits);
	}

	float			ReadBitAngle(int numbits);

	unsigned int	ReadUBitLong(int numbits);
	unsigned int	ReadUBitLongNoInline(int numbits);
	unsigned int	PeekUBitLong(int numbits);
	int				ReadSBitLong(int numbits);

	// reads an unsigned integer with variable bit length
	unsigned int	ReadUBitVar();
	unsigned int	ReadUBitVarInternal(int encodingType);

	// reads a varint encoded integer
	uint32_t			ReadVarInt32();
	uint64_t			ReadVarInt64();
	int32_t			ReadSignedVarInt32();
	int64_t			ReadSignedVarInt64();

	// You can read signed or unsigned data with this, just cast to 
	// a signed int if necessary.
	unsigned int	ReadBitLong(int numbits, bool bSigned);

	float			ReadBitCoord();
	float			ReadBitCoordMP(bool bIntegral, bool bLowPrecision);
	float			ReadBitFloat();
	float			ReadBitNormal();
	void			ReadBitVec3Coord(Vector& fa);
	void			ReadBitVec3Normal(Vector& fa);
	void			ReadBitAngles(Vector& fa);

	// Faster for comparisons but do not fully decode float values
	//unsigned int	ReadBitCoordBits();
	//unsigned int	ReadBitCoordMPBits(bool bIntegral, bool bLowPrecision);

	// Byte functions (these still read data in bit-by-bit).
public:

	__forceinline int	ReadChar() { return (char)ReadUBitLong(8); }
	__forceinline int	ReadByte() { return ReadUBitLong(8); }
	__forceinline int	ReadShort() { return (short)ReadUBitLong(16); }
	__forceinline int	ReadWord() { return ReadUBitLong(16); }
	__forceinline long ReadLong() { return ReadUBitLong(32); }
	int64_t			ReadLongLong();
	float			ReadFloat();
	bool			ReadBytes(void *pOut, int nBytes);

	// Returns false if bufLen isn't large enough to hold the
	// string in the buffer.
	//
	// Always reads to the end of the string (so you can read the
	// next piece of data waiting).
	//
	// If bLine is true, it stops when it reaches a '\n' or a null-terminator.
	//
	// pStr is always null-terminated (unless bufLen is 0).
	//
	// pOutNumChars is set to the number of characters left in pStr when the routine is 
	// complete (this will never exceed bufLen-1).
	//
	bool			ReadString(char *pStr, int bufLen, bool bLine = false, int *pOutNumChars = NULL);

	// Reads a string and allocates memory for it. If the string in the buffer
	// is > 2048 bytes, then pOverflow is set to true (if it's not NULL).
	char*			ReadAndAllocateString(bool *pOverflow = 0);

	// Returns nonzero if any bits differ
	int				CompareBits(bf_read *other, int bits);
	int				CompareBitsAt(int offset, bf_read *other, int otherOffset, int bits);

	// Status.
public:
	int				GetNumBytesLeft();
	int				GetNumBytesRead();
	int				GetNumBitsLeft();
	int				GetNumBitsRead() const;

	// Has the buffer overflowed?
	inline bool		IsOverflowed() const { return m_bOverflow; }

	inline bool		Seek(int iBit);					// Seek to a specific bit.
	inline bool		SeekRelative(int iBitDelta);	// Seek to an offset from the current position.

													// Called when the buffer is overflowed.
	void			SetOverflowFlag();


public:

	// The current buffer.
	const unsigned char *m_pData;
	int						m_nDataBytes;
	int						m_nDataBits;

	// Where we are in the buffer.
	int				m_iCurBit;


private:
	// Errors?
	bool			m_bOverflow;

	// For debugging..
	bool			m_bAssertOnOverflow;

	const char		*m_pDebugName;
};

//-----------------------------------------------------------------------------
// Inlined methods
//-----------------------------------------------------------------------------

// How many bytes are filled in?
inline int bf_write::GetNumBytesWritten() const
{
	return BitByte(m_iCurBit);
}

inline int bf_write::GetNumBitsWritten() const
{
	return m_iCurBit;
}

inline int bf_write::GetMaxNumBits()
{
	return m_nDataBits;
}

inline int bf_write::GetNumBitsLeft()
{
	return m_nDataBits - m_iCurBit;
}

inline int bf_write::GetNumBytesLeft()
{
	return GetNumBitsLeft() >> 3;
}

inline unsigned char* bf_write::GetData()
{
	return (unsigned char*)m_pData;
}

inline const unsigned char* bf_write::GetData()	const
{
	return (unsigned char*)m_pData;
}

__forceinline bool bf_write::CheckForOverflow(int nBits)
{
	if (m_iCurBit + nBits > m_nDataBits)
	{
		SetOverflowFlag();
		CallErrorHandler(BITBUFERROR_BUFFER_OVERRUN, GetDebugName());
	}

	return m_bOverflow;
}

__forceinline void bf_write::SetOverflowFlag()
{
#ifdef DBGFLAG_ASSERT
	if (m_bAssertOnOverflow)
	{
		Assert(false);
	}
#endif
	m_bOverflow = true;
}

__forceinline void bf_write::WriteOneBitNoCheck(int nValue)
{
#if __i386__
	if (nValue)
		m_pData[m_iCurBit >> 5] |= 1u << (m_iCurBit & 31);
	else
		m_pData[m_iCurBit >> 5] &= ~(1u << (m_iCurBit & 31));
#else
	extern unsigned long g_LittleBits[32];
	if (nValue)
		m_pData[m_iCurBit >> 5] |= g_LittleBits[m_iCurBit & 31];
	else
		m_pData[m_iCurBit >> 5] &= ~g_LittleBits[m_iCurBit & 31];
#endif

	++m_iCurBit;
}

inline void bf_write::WriteOneBit(int nValue)
{
	if (m_iCurBit >= m_nDataBits)
	{
		SetOverflowFlag();
		CallErrorHandler(BITBUFERROR_BUFFER_OVERRUN, GetDebugName());
		return;
	}
	WriteOneBitNoCheck(nValue);
}


inline void	bf_write::WriteOneBitAt(int iBit, int nValue)
{
	if (iBit >= m_nDataBits)
	{
		SetOverflowFlag();
		CallErrorHandler(BITBUFERROR_BUFFER_OVERRUN, GetDebugName());
		return;
	}

#if __i386__
	if (nValue)
		m_pData[iBit >> 5] |= 1u << (iBit & 31);
	else
		m_pData[iBit >> 5] &= ~(1u << (iBit & 31));
#else
	extern unsigned long g_LittleBits[32];
	if (nValue)
		m_pData[iBit >> 5] |= g_LittleBits[iBit & 31];
	else
		m_pData[iBit >> 5] &= ~g_LittleBits[iBit & 31];
#endif
}

__forceinline void bf_write::WriteUBitLong(unsigned int curData, int numbits, bool bCheckRange)
{
	if (GetNumBitsLeft() < numbits)
	{
		m_iCurBit = m_nDataBits;
		SetOverflowFlag();
		CallErrorHandler(BITBUFERROR_BUFFER_OVERRUN, GetDebugName());
		return;
	}

	int iCurBitMasked = m_iCurBit & 31;
	int iDWord = m_iCurBit >> 5;
	m_iCurBit += numbits;

	// Mask in a dword.
	Assert((iDWord * 4 + sizeof(long)) <= (unsigned int)m_nDataBytes);
	unsigned long *pOut = &m_pData[iDWord];

	// Rotate data into dword alignment
	curData = (curData << iCurBitMasked) | (curData >> (32 - iCurBitMasked));

	// Calculate bitmasks for first and second word
	unsigned int temp = 1 << (numbits - 1);
	unsigned int mask1 = (temp * 2 - 1) << iCurBitMasked;
	unsigned int mask2 = (temp - 1) >> (31 - iCurBitMasked);

	// Only look beyond current word if necessary (avoid access violation)
	int i = mask2 & 1;
	unsigned long dword1 = LoadLittleDWord(pOut, 0);
	unsigned long dword2 = LoadLittleDWord(pOut, i);

	// Drop bits into place
	dword1 ^= (mask1 & (curData ^ dword1));
	dword2 ^= (mask2 & (curData ^ dword2));

	// Note reversed order of writes so that dword1 wins if mask2 == 0 && i == 0
	StoreLittleDWord(pOut, i, dword2);
	StoreLittleDWord(pOut, 0, dword1);
}

// writes an unsigned integer with variable bit length
__forceinline void bf_write::WriteUBitVar(unsigned int data)
{
	/* Reference:
	if ( data < 0x10u )
	WriteUBitLong( 0, 2 ), WriteUBitLong( data, 4 );
	else if ( data < 0x100u )
	WriteUBitLong( 1, 2 ), WriteUBitLong( data, 8 );
	else if ( data < 0x1000u )
	WriteUBitLong( 2, 2 ), WriteUBitLong( data, 12 );
	else
	WriteUBitLong( 3, 2 ), WriteUBitLong( data, 32 );
	*/
	// a < b ? -1 : 0 translates into a CMP, SBB instruction pair
	// with no flow control. should also be branchless on consoles.
	int n = (data < 0x10u ? -1 : 0) + (data < 0x100u ? -1 : 0) + (data < 0x1000u ? -1 : 0);
	WriteUBitLong(data * 4 + n + 3, 6 + n * 4 + 12);
	if (data >= 0x1000u)
	{
		WriteUBitLong(data >> 16, 16);
	}
}

// write raw IEEE float bits in little endian form
__forceinline void bf_write::WriteBitFloat(float val)
{
	long intVal;

	Assert(sizeof(long) == sizeof(float));
	Assert(sizeof(float) == 4);

	intVal = *((long*)&val);
	WriteUBitLong(intVal, 32);
}

//-----------------------------------------------------------------------------
// This is useful if you just want a buffer to write into on the stack.
//-----------------------------------------------------------------------------

template<int SIZE>
class old_bf_write_static : public bf_write
{
public:
	inline old_bf_write_static() : bf_write(m_StaticData, SIZE) {}

	char	m_StaticData[SIZE];
};

inline int bf_read::GetNumBytesRead()
{
	return BitByte(m_iCurBit);
}

inline int bf_read::GetNumBitsLeft()
{
	return m_nDataBits - m_iCurBit;
}

inline int bf_read::GetNumBytesLeft()
{
	return GetNumBitsLeft() >> 3;
}

inline int bf_read::GetNumBitsRead() const
{
	return m_iCurBit;
}

inline bool bf_read::Seek(int iBit)
{
	if (iBit < 0 || iBit > m_nDataBits)
	{
		SetOverflowFlag();
		m_iCurBit = m_nDataBits;
		return false;
	}
	else
	{
		m_iCurBit = iBit;
		return true;
	}
}

// Seek to an offset from the current position.
inline bool	bf_read::SeekRelative(int iBitDelta)
{
	return Seek(m_iCurBit + iBitDelta);
}

inline bool bf_read::CheckForOverflow(int nBits)
{
	if (m_iCurBit + nBits > m_nDataBits)
	{
		SetOverflowFlag();
		CallErrorHandler(BITBUFERROR_BUFFER_OVERRUN, GetDebugName());
	}

	return m_bOverflow;
}

inline int bf_read::ReadOneBitNoCheck()
{
#if VALVE_LITTLE_ENDIAN
	unsigned int value = ((unsigned long * RESTRICT)m_pData)[m_iCurBit >> 5] >> (m_iCurBit & 31);
#else
	unsigned char value = m_pData[m_iCurBit >> 3] >> (m_iCurBit & 7);
#endif
	++m_iCurBit;
	return value & 1;
}

inline int bf_read::ReadOneBit()
{
	if (GetNumBitsLeft() <= 0)
	{
		SetOverflowFlag();
		CallErrorHandler(BITBUFERROR_BUFFER_OVERRUN, GetDebugName());
		return 0;
	}
	return ReadOneBitNoCheck();
}

inline float bf_read::ReadBitFloat()
{
	union { uint32_t u; float f; } c = { ReadUBitLong(32) };
	return c.f;
}

__forceinline unsigned int bf_read::ReadUBitVar()
{
	// six bits: low 2 bits for encoding + first 4 bits of value
	unsigned int sixbits = ReadUBitLong(6);
	unsigned int encoding = sixbits & 3;
	if (encoding)
	{
		// this function will seek back four bits and read the full value
		return ReadUBitVarInternal(encoding);
	}
	return sixbits >> 2;
}

__forceinline unsigned int bf_read::ReadUBitLong(int numbits)
{
	Assert(numbits > 0 && numbits <= 32);

	if (GetNumBitsLeft() < numbits)
	{
		m_iCurBit = m_nDataBits;
		SetOverflowFlag();
		CallErrorHandler(BITBUFERROR_BUFFER_OVERRUN, GetDebugName());
		return 0;
	}

	unsigned int iStartBit = m_iCurBit & 31u;
	int iLastBit = m_iCurBit + numbits - 1;
	unsigned int iWordOffset1 = m_iCurBit >> 5;
	unsigned int iWordOffset2 = iLastBit >> 5;
	m_iCurBit += numbits;

#if __i386__
	unsigned int bitmask = (2 << (numbits - 1)) - 1;
#else
	extern unsigned long g_ExtraMasks[33];
	unsigned int bitmask = g_ExtraMasks[numbits];
#endif

	unsigned int dw1 = LoadLittleDWord((unsigned long*)m_pData, iWordOffset1) >> iStartBit;
	unsigned int dw2 = LoadLittleDWord((unsigned long*)m_pData, iWordOffset2) << (32 - iStartBit);

	return (dw1 | dw2) & bitmask;
}

__forceinline int bf_read::CompareBits(bf_read *other, int numbits)
{
	return (ReadUBitLong(numbits) != other->ReadUBitLong(numbits));
}

#define FAST_BIT_SCAN 1
#define LOG2_BITS_PER_INT	5
#define BITS_PER_INT		32
static BitBufErrorHandler g_BitBufErrorHandler = 0;

inline int GetBitForBitnum(int bitNum)
{
	static int bitsForBitnum[] =
	{
		(1 << 0),
		(1 << 1),
		(1 << 2),
		(1 << 3),
		(1 << 4),
		(1 << 5),
		(1 << 6),
		(1 << 7),
		(1 << 8),
		(1 << 9),
		(1 << 10),
		(1 << 11),
		(1 << 12),
		(1 << 13),
		(1 << 14),
		(1 << 15),
		(1 << 16),
		(1 << 17),
		(1 << 18),
		(1 << 19),
		(1 << 20),
		(1 << 21),
		(1 << 22),
		(1 << 23),
		(1 << 24),
		(1 << 25),
		(1 << 26),
		(1 << 27),
		(1 << 28),
		(1 << 29),
		(1 << 30),
		(1 << 31),
	};

	return bitsForBitnum[(bitNum) & (BITS_PER_INT - 1)];
}

inline int BitForBitnum(int bitnum)
{
	return GetBitForBitnum(bitnum);
}

void InternalBitBufErrorHandler(BitBufErrorType errorType, const char *pDebugName)
{
	if (g_BitBufErrorHandler)
		g_BitBufErrorHandler(errorType, pDebugName);
}


void SetBitBufErrorHandler(BitBufErrorHandler fn)
{
	g_BitBufErrorHandler = fn;
}


// #define BB_PROFILING

unsigned long g_LittleBits[32];

// Precalculated bit masks for WriteUBitLong. Using these tables instead of 
// doing the calculations gives a 33% speedup in WriteUBitLong.
unsigned long g_BitWriteMasks[32][33];

// (1 << i) - 1
unsigned long g_ExtraMasks[33];

class CBitWriteMasksInit
{
public:
	CBitWriteMasksInit()
	{
		for (unsigned int startbit = 0; startbit < 32; startbit++)
		{
			for (unsigned int nBitsLeft = 0; nBitsLeft < 33; nBitsLeft++)
			{
				unsigned int endbit = startbit + nBitsLeft;
				g_BitWriteMasks[startbit][nBitsLeft] = BitForBitnum(startbit) - 1;
				if (endbit < 32)
					g_BitWriteMasks[startbit][nBitsLeft] |= ~(BitForBitnum(endbit) - 1);
			}
		}

		for (unsigned int maskBit = 0; maskBit < 32; maskBit++)
			g_ExtraMasks[maskBit] = BitForBitnum(maskBit) - 1;
		g_ExtraMasks[32] = ~0ul;

		for (unsigned int littleBit = 0; littleBit < 32; littleBit++)
			StoreLittleDWord(&g_LittleBits[littleBit], 0, 1u << littleBit);
	}
};
static CBitWriteMasksInit g_BitWriteMasksInit;


// ---------------------------------------------------------------------------------------- //
// bf_write
// ---------------------------------------------------------------------------------------- //

bf_write::bf_write()
{
	m_pData = NULL;
	m_nDataBytes = 0;
	m_nDataBits = -1; // set to -1 so we generate overflow on any operation
	m_iCurBit = 0;
	m_bOverflow = false;
	m_bAssertOnOverflow = true;
	m_pDebugName = NULL;
}

bf_write::bf_write(const char *pDebugName, void *pData, int nBytes, int nBits)
{
	m_bAssertOnOverflow = true;
	m_pDebugName = pDebugName;
	StartWriting(pData, nBytes, 0, nBits);
}

bf_write::bf_write(void *pData, int nBytes, int nBits)
{
	m_bAssertOnOverflow = true;
	m_pDebugName = NULL;
	StartWriting(pData, nBytes, 0, nBits);
}

void bf_write::StartWriting(void *pData, int nBytes, int iStartBit, int nBits)
{
	// Make sure it's dword aligned and padded.
	Assert((nBytes % 4) == 0);
	Assert(((unsigned long)pData & 3) == 0);

	// The writing code will overrun the end of the buffer if it isn't dword aligned, so truncate to force alignment
	nBytes &= ~3;

	m_pData = (unsigned long*)pData;
	m_nDataBytes = nBytes;

	if (nBits == -1)
	{
		m_nDataBits = nBytes << 3;
	}
	else
	{
		Assert(nBits <= nBytes * 8);
		m_nDataBits = nBits;
	}

	m_iCurBit = iStartBit;
	m_bOverflow = false;
}

void bf_write::Reset()
{
	m_iCurBit = 0;
	m_bOverflow = false;
}


void bf_write::SetAssertOnOverflow(bool bAssert)
{
	m_bAssertOnOverflow = bAssert;
}


const char* bf_write::GetDebugName()
{
	return m_pDebugName;
}


void bf_write::SetDebugName(const char *pDebugName)
{
	m_pDebugName = pDebugName;
}


void bf_write::SeekToBit(int bitPos)
{
	m_iCurBit = bitPos;
}


// Sign bit comes first
void bf_write::WriteSBitLong(int data, int numbits)
{
	// Force the sign-extension bit to be correct even in the case of overflow.
	int nValue = data;
	int nPreserveBits = (0x7FFFFFFF >> (32 - numbits));
	int nSignExtension = (nValue >> 31) & ~nPreserveBits;
	nValue &= nPreserveBits;
	nValue |= nSignExtension;

	AssertMsg(nValue == data, "WriteSBitLong: 0x%08x does not fit in %d bits", data, numbits);

	WriteUBitLong(nValue, numbits, false);
}

void bf_write::WriteVarInt32(uint32_t data)
{
	// Check if align and we have room, slow path if not
	if ((m_iCurBit & 7) == 0 && (m_iCurBit + bitbuf::kMaxVarint32Bytes * 8) <= m_nDataBits)
	{
		uint8_t *target = ((uint8_t*)m_pData) + (m_iCurBit >> 3);

		target[0] = static_cast<uint8_t>(data | 0x80);
		if (data >= (1 << 7))
		{
			target[1] = static_cast<uint8_t>((data >> 7) | 0x80);
			if (data >= (1 << 14))
			{
				target[2] = static_cast<uint8_t>((data >> 14) | 0x80);
				if (data >= (1 << 21))
				{
					target[3] = static_cast<uint8_t>((data >> 21) | 0x80);
					if (data >= (1 << 28))
					{
						target[4] = static_cast<uint8_t>(data >> 28);
						m_iCurBit += 5 * 8;
						return;
					}
					else
					{
						target[3] &= 0x7F;
						m_iCurBit += 4 * 8;
						return;
					}
				}
				else
				{
					target[2] &= 0x7F;
					m_iCurBit += 3 * 8;
					return;
				}
			}
			else
			{
				target[1] &= 0x7F;
				m_iCurBit += 2 * 8;
				return;
			}
		}
		else
		{
			target[0] &= 0x7F;
			m_iCurBit += 1 * 8;
			return;
		}
	}
	else // Slow path
	{
		while (data > 0x7F)
		{
			WriteUBitLong((data & 0x7F) | 0x80, 8);
			data >>= 7;
		}
		WriteUBitLong(data & 0x7F, 8);
	}
}

void bf_write::WriteVarInt64(uint64_t data)
{
	// Check if align and we have room, slow path if not
	if ((m_iCurBit & 7) == 0 && (m_iCurBit + bitbuf::kMaxVarintBytes * 8) <= m_nDataBits)
	{
		uint8_t *target = ((uint8_t*)m_pData) + (m_iCurBit >> 3);

		// Splitting into 32-bit pieces gives better performance on 32-bit
		// processors.
		uint32_t part0 = static_cast<uint32_t>(data);
		uint32_t part1 = static_cast<uint32_t>(data >> 28);
		uint32_t part2 = static_cast<uint32_t>(data >> 56);

		int size;

		// Here we can't really optimize for small numbers, since the data is
		// split into three parts.  Cheking for numbers < 128, for instance,
		// would require three comparisons, since you'd have to make sure part1
		// and part2 are zero.  However, if the caller is using 64-bit integers,
		// it is likely that they expect the numbers to often be very large, so
		// we probably don't want to optimize for small numbers anyway.  Thus,
		// we end up with a hardcoded binary search tree...
		if (part2 == 0)
		{
			if (part1 == 0)
			{
				if (part0 < (1 << 14))
				{
					if (part0 < (1 << 7))
					{
						size = 1; goto size1;
					}
					else
					{
						size = 2; goto size2;
					}
				}
				else
				{
					if (part0 < (1 << 21))
					{
						size = 3; goto size3;
					}
					else
					{
						size = 4; goto size4;
					}
				}
			}
			else
			{
				if (part1 < (1 << 14))
				{
					if (part1 < (1 << 7))
					{
						size = 5; goto size5;
					}
					else
					{
						size = 6; goto size6;
					}
				}
				else
				{
					if (part1 < (1 << 21))
					{
						size = 7; goto size7;
					}
					else
					{
						size = 8; goto size8;
					}
				}
			}
		}
		else
		{
			if (part2 < (1 << 7))
			{
				size = 9; goto size9;
			}
			else
			{
				size = 10; goto size10;
			}
		}

		AssertMsg(false, "Can't get here.");

	size10: target[9] = static_cast<uint8_t>((part2 >> 7) | 0x80);
	size9: target[8] = static_cast<uint8_t>((part2) | 0x80);
	size8: target[7] = static_cast<uint8_t>((part1 >> 21) | 0x80);
	size7: target[6] = static_cast<uint8_t>((part1 >> 14) | 0x80);
	size6: target[5] = static_cast<uint8_t>((part1 >> 7) | 0x80);
	size5: target[4] = static_cast<uint8_t>((part1) | 0x80);
	size4: target[3] = static_cast<uint8_t>((part0 >> 21) | 0x80);
	size3: target[2] = static_cast<uint8_t>((part0 >> 14) | 0x80);
	size2: target[1] = static_cast<uint8_t>((part0 >> 7) | 0x80);
	size1: target[0] = static_cast<uint8_t>((part0) | 0x80);

		target[size - 1] &= 0x7F;
		m_iCurBit += size * 8;
	}
	else // slow path
	{
		while (data > 0x7F)
		{
			WriteUBitLong((data & 0x7F) | 0x80, 8);
			data >>= 7;
		}
		WriteUBitLong(data & 0x7F, 8);
	}
}

void bf_write::WriteSignedVarInt32(int32_t data)
{
	WriteVarInt32(bitbuf::ZigZagEncode32(data));
}

void bf_write::WriteSignedVarInt64(int64_t data)
{
	WriteVarInt64(bitbuf::ZigZagEncode64(data));
}

int	bf_write::ByteSizeVarInt32(uint32_t data)
{
	int size = 1;
	while (data > 0x7F) {
		size++;
		data >>= 7;
	}
	return size;
}

int	bf_write::ByteSizeVarInt64(uint64_t data)
{
	int size = 1;
	while (data > 0x7F) {
		size++;
		data >>= 7;
	}
	return size;
}

int bf_write::ByteSizeSignedVarInt32(int32_t data)
{
	return ByteSizeVarInt32(bitbuf::ZigZagEncode32(data));
}

int bf_write::ByteSizeSignedVarInt64(int64_t data)
{
	return ByteSizeVarInt64(bitbuf::ZigZagEncode64(data));
}

void bf_write::WriteBitLong(unsigned int data, int numbits, bool bSigned)
{
	if (bSigned)
		WriteSBitLong((int)data, numbits);
	else
		WriteUBitLong(data, numbits);
}

bool bf_write::WriteBits(const void *pInData, int nBits)
{
#if defined( BB_PROFILING )
	VPROF("bf_write::WriteBits");
#endif

	unsigned char *pOut = (unsigned char*)pInData;
	int nBitsLeft = nBits;

	// Bounds checking..
	if ((m_iCurBit + nBits) > m_nDataBits)
	{
		SetOverflowFlag();
		CallErrorHandler(BITBUFERROR_BUFFER_OVERRUN, GetDebugName());
		return false;
	}

	// Align output to dword boundary
	while (((unsigned long)pOut & 3) != 0 && nBitsLeft >= 8)
	{

		WriteUBitLong(*pOut, 8, false);
		++pOut;
		nBitsLeft -= 8;
	}

	if (IsPC() && (nBitsLeft >= 32) && (m_iCurBit & 7) == 0)
	{
		// current bit is byte aligned, do block copy
		int numbytes = nBitsLeft >> 3;
		int numbits = numbytes << 3;

		memcpy((char*)m_pData + (m_iCurBit >> 3), pOut, numbytes);
		pOut += numbytes;
		nBitsLeft -= numbits;
		m_iCurBit += numbits;
	}

	// X360TBD: Can't write dwords in WriteBits because they'll get swapped
	if (IsPC() && nBitsLeft >= 32)
	{
		unsigned long iBitsRight = (m_iCurBit & 31);
		unsigned long iBitsLeft = 32 - iBitsRight;
		unsigned long bitMaskLeft = g_BitWriteMasks[iBitsRight][32];
		unsigned long bitMaskRight = g_BitWriteMasks[0][iBitsRight];

		unsigned long *pData = &m_pData[m_iCurBit >> 5];

		// Read dwords.
		while (nBitsLeft >= 32)
		{
			unsigned long curData = *(unsigned long*)pOut;
			pOut += sizeof(unsigned long);

			*pData &= bitMaskLeft;
			*pData |= curData << iBitsRight;

			pData++;

			if (iBitsLeft < 32)
			{
				curData >>= iBitsLeft;
				*pData &= bitMaskRight;
				*pData |= curData;
			}

			nBitsLeft -= 32;
			m_iCurBit += 32;
		}
	}


	// write remaining bytes
	while (nBitsLeft >= 8)
	{
		WriteUBitLong(*pOut, 8, false);
		++pOut;
		nBitsLeft -= 8;
	}

	// write remaining bits
	if (nBitsLeft)
	{
		WriteUBitLong(*pOut, nBitsLeft, false);
	}

	return !IsOverflowed();
}


bool bf_write::WriteBitsFromBuffer(bf_read *pIn, int nBits)
{
	// This could be optimized a little by
	while (nBits > 32)
	{
		WriteUBitLong(pIn->ReadUBitLong(32), 32);
		nBits -= 32;
	}

	WriteUBitLong(pIn->ReadUBitLong(nBits), nBits);
	return !IsOverflowed() && !pIn->IsOverflowed();
}


void bf_write::WriteBitAngle(float fAngle, int numbits)
{
	int d;
	unsigned int mask;
	unsigned int shift;

	shift = BitForBitnum(numbits);
	mask = shift - 1;

	d = (int)((fAngle / 360.0) * shift);
	d &= mask;

	WriteUBitLong((unsigned int)d, numbits);
}

void bf_write::WriteChar(int val)
{
	WriteSBitLong(val, sizeof(char) << 3);
}

void bf_write::WriteByte(int val)
{
	WriteUBitLong(val, sizeof(unsigned char) << 3);
}

void bf_write::WriteShort(int val)
{
	WriteSBitLong(val, sizeof(short) << 3);
}

void bf_write::WriteWord(int val)
{
	WriteUBitLong(val, sizeof(unsigned short) << 3);
}

void bf_write::WriteLong(long val)
{
	WriteSBitLong(val, sizeof(long) << 3);
}

void bf_write::WriteLongLong(int64_t val)
{
	UINT *pLongs = (UINT*)&val;

	// Insert the two DWORDS according to network endian
	const short endianIndex = 0x0100;
	BYTE *idx = (BYTE *)&endianIndex;
	WriteUBitLong(pLongs[*idx++], sizeof(long) << 3);
	WriteUBitLong(pLongs[*idx], sizeof(long) << 3);
}

void bf_write::WriteFloat(float val)
{
	// Pre-swap the float, since WriteBits writes raw data
	LittleFloat(&val, &val);

	WriteBits(&val, sizeof(val) << 3);
}

bool bf_write::WriteBytes(const void *pBuf, int nBytes)
{
	return WriteBits(pBuf, nBytes << 3);
}

bool bf_write::WriteString(const char *pStr)
{
	if (pStr)
	{
		do
		{
			WriteChar(*pStr);
			++pStr;
		} while (*(pStr - 1) != 0);
	}
	else
	{
		WriteChar(0);
	}

	return !IsOverflowed();
}

// ---------------------------------------------------------------------------------------- //
// bf_read
// ---------------------------------------------------------------------------------------- //

bf_read::bf_read()
{
	m_pData = NULL;
	m_nDataBytes = 0;
	m_nDataBits = -1; // set to -1 so we overflow on any operation
	m_iCurBit = 0;
	m_bOverflow = false;
	m_bAssertOnOverflow = true;
	m_pDebugName = NULL;
}

bf_read::bf_read(const void *pData, int nBytes, int nBits)
{
	m_bAssertOnOverflow = true;
	StartReading(pData, nBytes, 0, nBits);
}

bf_read::bf_read(const char *pDebugName, const void *pData, int nBytes, int nBits)
{
	m_bAssertOnOverflow = true;
	m_pDebugName = pDebugName;
	StartReading(pData, nBytes, 0, nBits);
}

void bf_read::StartReading(const void *pData, int nBytes, int iStartBit, int nBits)
{
	// Make sure we're dword aligned.
	Assert(((size_t)pData & 3) == 0);

	m_pData = (unsigned char*)pData;
	m_nDataBytes = nBytes;

	if (nBits == -1)
	{
		m_nDataBits = m_nDataBytes << 3;
	}
	else
	{
		Assert(nBits <= nBytes * 8);
		m_nDataBits = nBits;
	}

	m_iCurBit = iStartBit;
	m_bOverflow = false;
}

void bf_read::Reset()
{
	m_iCurBit = 0;
	m_bOverflow = false;
}

void bf_read::SetAssertOnOverflow(bool bAssert)
{
	m_bAssertOnOverflow = bAssert;
}

void bf_read::SetDebugName(const char *pName)
{
	m_pDebugName = pName;
}

void bf_read::SetOverflowFlag()
{
	if (m_bAssertOnOverflow)
	{
		Assert(false);
	}
	m_bOverflow = true;
}

unsigned int bf_read::CheckReadUBitLong(int numbits)
{
	// Ok, just read bits out.
	int i, nBitValue;
	unsigned int r = 0;

	for (i = 0; i < numbits; i++)
	{
		nBitValue = ReadOneBitNoCheck();
		r |= nBitValue << i;
	}
	m_iCurBit -= numbits;

	return r;
}

void bf_read::ReadBits(void *pOutData, int nBits)
{
#if defined( BB_PROFILING )
	VPROF("bf_read::ReadBits");
#endif

	unsigned char *pOut = (unsigned char*)pOutData;
	int nBitsLeft = nBits;


	// align output to dword boundary
	while (((size_t)pOut & 3) != 0 && nBitsLeft >= 8)
	{
		*pOut = (unsigned char)ReadUBitLong(8);
		++pOut;
		nBitsLeft -= 8;
	}

	// X360TBD: Can't read dwords in ReadBits because they'll get swapped
	if (IsPC())
	{
		// read dwords
		while (nBitsLeft >= 32)
		{
			*((unsigned long*)pOut) = ReadUBitLong(32);
			pOut += sizeof(unsigned long);
			nBitsLeft -= 32;
		}
	}

	// read remaining bytes
	while (nBitsLeft >= 8)
	{
		*pOut = ReadUBitLong(8);
		++pOut;
		nBitsLeft -= 8;
	}

	// read remaining bits
	if (nBitsLeft)
	{
		*pOut = ReadUBitLong(nBitsLeft);
	}

}

int bf_read::ReadBitsClamped_ptr(void *pOutData, size_t outSizeBytes, size_t nBits)
{
	size_t outSizeBits = outSizeBytes * 8;
	size_t readSizeBits = nBits;
	int skippedBits = 0;
	if (readSizeBits > outSizeBits)
	{
		// Should we print a message when we clamp the data being read? Only
		// in debug builds I think.
		AssertMsg(0, "Oversized network packet received, and clamped.");
		readSizeBits = outSizeBits;
		skippedBits = (int)(nBits - outSizeBits);
		// What should we do in this case, which should only happen if nBits
		// is negative for some reason?
		//if ( skippedBits < 0 )
		//	return 0;
	}

	ReadBits(pOutData, readSizeBits);
	SeekRelative(skippedBits);

	// Return the number of bits actually read.
	return (int)readSizeBits;
}

float bf_read::ReadBitAngle(int numbits)
{
	float fReturn;
	int i;
	float shift;

	shift = (float)(BitForBitnum(numbits));

	i = ReadUBitLong(numbits);
	fReturn = (float)i * (360.0 / shift);

	return fReturn;
}

unsigned int bf_read::PeekUBitLong(int numbits)
{
	unsigned int r;
	int i, nBitValue;
#ifdef BIT_VERBOSE
	int nShifts = numbits;
#endif

	bf_read savebf;

	savebf = *this;  // Save current state info

	r = 0;
	for (i = 0; i < numbits; i++)
	{
		nBitValue = ReadOneBit();

		// Append to current stream
		if (nBitValue)
		{
			r |= BitForBitnum(i);
		}
	}

	*this = savebf;

#ifdef BIT_VERBOSE
	Con_Printf("PeekBitLong:  %i %i\n", nShifts, (unsigned int)r);
#endif

	return r;
}

unsigned int bf_read::ReadUBitLongNoInline(int numbits)
{
	return ReadUBitLong(numbits);
}

unsigned int bf_read::ReadUBitVarInternal(int encodingType)
{
	m_iCurBit -= 4;
	// int bits = { 4, 8, 12, 32 }[ encodingType ];
	int bits = 4 + encodingType * 4 + (((2 - encodingType) >> 31) & 16);
	return ReadUBitLong(bits);
}

// Append numbits least significant bits from data to the current bit stream
int bf_read::ReadSBitLong(int numbits)
{
	unsigned int r = ReadUBitLong(numbits);
	unsigned int s = 1 << (numbits - 1);
	if (r >= s)
	{
		// sign-extend by removing sign bit and then subtracting sign bit again
		r = r - s - s;
	}
	return r;
}

uint32_t bf_read::ReadVarInt32()
{
	uint32_t result = 0;
	int count = 0;
	uint32_t b;

	do
	{
		if (count == bitbuf::kMaxVarint32Bytes)
		{
			return result;
		}
		b = ReadUBitLong(8);
		result |= (b & 0x7F) << (7 * count);
		++count;
	} while (b & 0x80);

	return result;
}

uint64_t bf_read::ReadVarInt64()
{
	uint64_t result = 0;
	int count = 0;
	uint64_t b;

	do
	{
		if (count == bitbuf::kMaxVarintBytes)
		{
			return result;
		}
		b = ReadUBitLong(8);
		result |= static_cast<uint64_t>(b & 0x7F) << (7 * count);
		++count;
	} while (b & 0x80);

	return result;
}

int32_t bf_read::ReadSignedVarInt32()
{
	uint32_t value = ReadVarInt32();
	return bitbuf::ZigZagDecode32(value);
}

int64_t bf_read::ReadSignedVarInt64()
{
	uint32_t value = ReadVarInt64();
	return bitbuf::ZigZagDecode64(value);
}

unsigned int bf_read::ReadBitLong(int numbits, bool bSigned)
{
	if (bSigned)
		return (unsigned int)ReadSBitLong(numbits);
	else
		return ReadUBitLong(numbits);
}

int64_t bf_read::ReadLongLong()
{
	int64_t retval;
	UINT *pLongs = (UINT*)&retval;

	// Read the two DWORDs according to network endian
	const short endianIndex = 0x0100;
	BYTE *idx = (BYTE *)&endianIndex;
	pLongs[*idx++] = ReadUBitLong(sizeof(long) << 3);
	pLongs[*idx] = ReadUBitLong(sizeof(long) << 3);

	return retval;
}

float bf_read::ReadFloat()
{
	float ret;
	Assert(sizeof(ret) == 4);
	ReadBits(&ret, 32);

	// Swap the float, since ReadBits reads raw data
	LittleFloat(&ret, &ret);
	return ret;
}

bool bf_read::ReadBytes(void *pOut, int nBytes)
{
	ReadBits(pOut, nBytes << 3);
	return !IsOverflowed();
}

bool bf_read::ReadString(char *pStr, int maxLen, bool bLine, int *pOutNumChars)
{
	Assert(maxLen != 0);

	bool bTooSmall = false;
	int iChar = 0;
	while (1)
	{
		char val = ReadChar();
		if (val == 0)
			break;
		else if (bLine && val == '\n')
			break;

		if (iChar < (maxLen - 1))
		{
			pStr[iChar] = val;
			++iChar;
		}
		else
		{
			bTooSmall = true;
		}
	}

	// Make sure it's null-terminated.
	Assert(iChar < maxLen);
	pStr[iChar] = 0;

	if (pOutNumChars)
		*pOutNumChars = iChar;

	return !IsOverflowed() && !bTooSmall;
}

char* bf_read::ReadAndAllocateString(bool *pOverflow)
{
	char str[2048];

	int nChars;
	bool bOverflow = !ReadString(str, sizeof(str), false, &nChars);
	if (pOverflow)
		*pOverflow = bOverflow;

	// Now copy into the output and return it;
	char *pRet = new char[nChars + 1];
	for (int i = 0; i <= nChars; i++)
		pRet[i] = str[i];

	return pRet;
}

void bf_read::ExciseBits(int startbit, int bitstoremove)
{
	int endbit = startbit + bitstoremove;
	int remaining_to_end = m_nDataBits - endbit;

	bf_write temp;
	temp.StartWriting((void *)m_pData, m_nDataBits << 3, startbit);

	Seek(endbit);

	for (int i = 0; i < remaining_to_end; i++)
	{
		temp.WriteOneBit(ReadOneBit());
	}

	Seek(startbit);

	m_nDataBits -= bitstoremove;
	m_nDataBytes = m_nDataBits >> 3;
}

int bf_read::CompareBitsAt(int offset, bf_read *other, int otherOffset, int numbits)
{
	extern unsigned long g_ExtraMasks[33];

	if (numbits == 0)
		return 0;

	int overflow1 = offset + numbits > m_nDataBits;
	int overflow2 = otherOffset + numbits > other->m_nDataBits;

	int x = overflow1 | overflow2;
	if (x != 0)
		return x;

	unsigned int iStartBit1 = offset & 31u;
	unsigned int iStartBit2 = otherOffset & 31u;
	unsigned long *pData1 = (unsigned long*)m_pData + (offset >> 5);
	unsigned long *pData2 = (unsigned long*)other->m_pData + (otherOffset >> 5);
	unsigned long *pData1End = pData1 + ((offset + numbits - 1) >> 5);
	unsigned long *pData2End = pData2 + ((otherOffset + numbits - 1) >> 5);

	while (numbits > 32)
	{
		x = LoadLittleDWord((unsigned long*)pData1, 0) >> iStartBit1;
		x ^= LoadLittleDWord((unsigned long*)pData1, 1) << (32 - iStartBit1);
		x ^= LoadLittleDWord((unsigned long*)pData2, 0) >> iStartBit2;
		x ^= LoadLittleDWord((unsigned long*)pData2, 1) << (32 - iStartBit2);
		if (x != 0)
		{
			return x;
		}
		++pData1;
		++pData2;
		numbits -= 32;
	}

	x = LoadLittleDWord((unsigned long*)pData1, 0) >> iStartBit1;
	x ^= LoadLittleDWord((unsigned long*)pData1End, 0) << (32 - iStartBit1);
	x ^= LoadLittleDWord((unsigned long*)pData2, 0) >> iStartBit2;
	x ^= LoadLittleDWord((unsigned long*)pData2End, 0) << (32 - iStartBit2);
	return x & g_ExtraMasks[numbits];
}
