#pragma once

#include <cstdlib>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <memory>
#include <ctype.h>
#include "../definitions.h"
#include "utl_memory.h"

typedef enum _fieldtypes
{
	FIELD_VOID = 0,			// No type or value
	FIELD_FLOAT,			// Any floating point value
	FIELD_STRING,			// A string ID (return from ALLOC_STRING)
	FIELD_VECTOR,			// Any vector, QAngle, or AngularImpulse
	FIELD_QUATERNION,		// A quaternion
	FIELD_INTEGER,			// Any integer or enum
	FIELD_BOOLEAN,			// boolean, implemented as an int, I may use this as a hint for compression
	FIELD_SHORT,			// 2 byte integer
	FIELD_CHARACTER,		// a byte
	FIELD_COLOR32,			// 8-bit per channel r,g,b,a (32bit color)
	FIELD_EMBEDDED,			// an embedded object with a datadesc, recursively traverse and embedded class/structure based on an additional typedescription
	FIELD_CUSTOM,			// special type that contains function pointers to it's read/write/parse functions

	FIELD_CLASSPTR,			// CBaseEntity *
	FIELD_EHANDLE,			// Entity handle
	FIELD_EDICT,			// edict_t *

	FIELD_POSITION_VECTOR,	// A world coordinate (these are fixed up across level transitions automagically)
	FIELD_TIME,				// a floating point time (these are fixed up automatically too!)
	FIELD_TICK,				// an integer tick count( fixed up similarly to time)
	FIELD_MODELNAME,		// Engine string that is a model name (needs precache)
	FIELD_SOUNDNAME,		// Engine string that is a sound name (needs precache)

	FIELD_INPUT,			// a list of inputed data fields (all derived from CMultiInputVar)
	FIELD_FUNCTION,			// A class function pointer (Think, Use, etc)

	FIELD_VMATRIX,			// a vmatrix (output coords are NOT worldspace)

	// NOTE: Use float arrays for local transformations that don't need to be fixed up.
	FIELD_VMATRIX_WORLDSPACE,// A VMatrix that maps some local space to world space (translation is fixed up on level transitions)
	FIELD_MATRIX3X4_WORLDSPACE,	// matrix3x4_t that maps some local space to world space (translation is fixed up on level transitions)

	FIELD_INTERVAL,			// a start and range floating point interval ( e.g., 3.2->3.6 == 3.2 and 0.4 )
	FIELD_MODELINDEX,		// a model index
	FIELD_MATERIALINDEX,	// a material index (using the material precache string table)

	FIELD_VECTOR2D,			// 2 floats
	FIELD_INTEGER64,		// 64bit integer


	FIELD_TYPECOUNT,		// MUST BE LAST
} fieldtype_t;

enum
{
	TD_OFFSET_NORMAL = 0,
	TD_OFFSET_PACKED = 1,

	// Must be last
	TD_OFFSET_COUNT,
};

struct datamap_t;
struct typedescription_t
{
	fieldtype_t			fieldType;
	const char			*fieldName;
	int					fieldOffset; // Local offset value
	unsigned short		fieldSize;
	short				flags;
	// the name of the variable in the map/fgd data, or the name of the action
	const char			*externalName;
	// pointer to the function set for save/restoring of custom data types
	void		*pSaveRestoreOps;
	// for associating function with string names
	void*			inputFunc;
	// For embedding additional datatables inside this one
	datamap_t			*td;

	// Stores the actual member variable size in bytes
	int					fieldSizeInBytes;

	// FTYPEDESC_OVERRIDE point to first baseclass instance if chains_validated has occurred
	struct typedescription_t *override_field;

	// Used to track exclusion of baseclass fields
	int					override_count;

	// Tolerance for field errors for float fields
	float				fieldTolerance;

	// For raw fields (including children of embedded stuff) this is the flattened offset
	int					flatOffset[TD_OFFSET_COUNT];
	unsigned short		flatGroup;
};

struct datamap_t
{
	typedescription_t	*dataDesc;
	int					dataNumFields;
	char const			*dataClassName;
	datamap_t			*baseMap;

	int					m_nPackedSize;
	void	*m_pOptimizedDataMap;

#if defined( _DEBUG )
	bool				bValidityChecked;
#endif // _DEBUG
};

class CByteswap
{
public:
	CByteswap()
	{
		// Default behavior sets the target endian to match the machine native endian (no swap).
		SetTargetBigEndian(IsMachineBigEndian());
	}

	//-----------------------------------------------------------------------------
	// Write a single field.
	//-----------------------------------------------------------------------------
	void SwapFieldToTargetEndian(void* pOutputBuffer, void *pData, typedescription_t *pField);

	//-----------------------------------------------------------------------------
	// Write a block of fields.  Works a bit like the saverestore code.  
	//-----------------------------------------------------------------------------
	void SwapFieldsToTargetEndian(void *pOutputBuffer, void *pBaseData, datamap_t *pDataMap);

	// Swaps fields for the templated type to the output buffer.
	template<typename T> inline void SwapFieldsToTargetEndian(T* pOutputBuffer, void *pBaseData, unsigned int objectCount = 1)
	{
		for (unsigned int i = 0; i < objectCount; ++i, ++pOutputBuffer)
		{
			SwapFieldsToTargetEndian((void*)pOutputBuffer, pBaseData, &T::m_DataMap);
			pBaseData = (byte*)pBaseData + sizeof(T);
		}
	}

	// Swaps fields for the templated type in place.
	template<typename T> inline void SwapFieldsToTargetEndian(T* pOutputBuffer, unsigned int objectCount = 1)
	{
		SwapFieldsToTargetEndian<T>(pOutputBuffer, (void*)pOutputBuffer, objectCount);
	}

	//-----------------------------------------------------------------------------
	// True if the current machine is detected as big endian. 
	// (Endienness is effectively detected at compile time when optimizations are
	// enabled)
	//-----------------------------------------------------------------------------
	static bool IsMachineBigEndian()
	{
		short nIsBigEndian = 1;

		// if we are big endian, the first byte will be a 0, if little endian, it will be a one.
		return (bool)(0 == *(char *)&nIsBigEndian);
	}

	//-----------------------------------------------------------------------------
	// Sets the target byte ordering we are swapping to or from.
	//
	// Braindead Endian Reference:
	//		x86 is LITTLE Endian
	//		PowerPC is BIG Endian
	//-----------------------------------------------------------------------------
	inline void SetTargetBigEndian(bool bigEndian)
	{
		m_bBigEndian = bigEndian;
		m_bSwapBytes = IsMachineBigEndian() != bigEndian;
	}

	// Changes target endian
	inline void FlipTargetEndian(void)
	{
		m_bSwapBytes = !m_bSwapBytes;
		m_bBigEndian = !m_bBigEndian;
	}

	// Forces byte swapping state, regardless of endianess
	inline void ActivateByteSwapping(bool bActivate)
	{
		SetTargetBigEndian(IsMachineBigEndian() != bActivate);
	}

	//-----------------------------------------------------------------------------
	// Returns true if the target machine is the same as this one in endianness.
	//
	// Used to determine when a byteswap needs to take place.
	//-----------------------------------------------------------------------------
	inline bool IsSwappingBytes(void)	// Are bytes being swapped?
	{
		return m_bSwapBytes;
	}

	inline bool IsTargetBigEndian(void)	// What is the current target endian?
	{
		return m_bBigEndian;
	}

	//-----------------------------------------------------------------------------
	// IsByteSwapped()
	//
	// When supplied with a chunk of input data and a constant or magic number
	// (in native format) determines the endienness of the current machine in
	// relation to the given input data.
	//
	// Returns:
	//		1  if input is the same as nativeConstant.
	//		0  if input is byteswapped relative to nativeConstant.
	//		-1 if input is not the same as nativeConstant and not byteswapped either.
	//
	// ( This is useful for detecting byteswapping in magic numbers in structure 
	// headers for example. )
	//-----------------------------------------------------------------------------
	template<typename T> inline int SourceIsNativeEndian(T input, T nativeConstant)
	{
		// If it's the same, it isn't byteswapped:
		if (input == nativeConstant)
			return 1;

		int output;
		LowLevelByteSwap<T>(&output, &input);
		if (output == nativeConstant)
			return 0;

		assert(0);		// if we get here, input is neither a swapped nor unswapped version of nativeConstant.
		return -1;
	}

	//-----------------------------------------------------------------------------
	// Swaps an input buffer full of type T into the given output buffer.
	//
	// Swaps [count] items from the inputBuffer to the outputBuffer.
	// If inputBuffer is omitted or NULL, then it is assumed to be the same as
	// outputBuffer - effectively swapping the contents of the buffer in place.
	//-----------------------------------------------------------------------------
	template<typename T> inline void SwapBuffer(T* outputBuffer, T* inputBuffer = NULL, int count = 1)
	{
		assert(count >= 0);
		assert(outputBuffer);

		// Fail gracefully in release:
		if (count <= 0 || !outputBuffer)
			return;

		// Optimization for the case when we are swapping in place.
		if (inputBuffer == NULL)
		{
			inputBuffer = outputBuffer;
		}

		// Swap everything in the buffer:
		for (int i = 0; i < count; i++)
		{
			LowLevelByteSwap<T>(&outputBuffer[i], &inputBuffer[i]);
		}
	}

	//-----------------------------------------------------------------------------
	// Swaps an input buffer full of type T into the given output buffer.
	//
	// Swaps [count] items from the inputBuffer to the outputBuffer.
	// If inputBuffer is omitted or NULL, then it is assumed to be the same as
	// outputBuffer - effectively swapping the contents of the buffer in place.
	//-----------------------------------------------------------------------------
	template<typename T> inline void SwapBufferToTargetEndian(T* outputBuffer, T* inputBuffer = NULL, int count = 1)
	{
		assert(count >= 0);
		assert(outputBuffer);

		// Fail gracefully in release:
		if (count <= 0 || !outputBuffer)
			return;

		// Optimization for the case when we are swapping in place.
		if (inputBuffer == NULL)
		{
			inputBuffer = outputBuffer;
		}

		// Are we already the correct endienness? ( or are we swapping 1 byte items? )
		if (!m_bSwapBytes || (sizeof(T) == 1))
		{
			// If we were just going to swap in place then return.
			if (!inputBuffer)
				return;

			// Otherwise copy the inputBuffer to the outputBuffer:
			memcpy(outputBuffer, inputBuffer, count * sizeof(T));
			return;

		}

		// Swap everything in the buffer:
		for (int i = 0; i < count; i++)
		{
			LowLevelByteSwap<T>(&outputBuffer[i], &inputBuffer[i]);
		}
	}

private:
	//-----------------------------------------------------------------------------
	// The lowest level byte swapping workhorse of doom.  output always contains the 
	// swapped version of input.  ( Doesn't compare machine to target endianness )
	//-----------------------------------------------------------------------------
	template<typename T> static void LowLevelByteSwap(T *output, T *input)
	{
		T temp = *output;
#if defined( _X360 )
		// Intrinsics need the source type to be fixed-point
		DWORD* word = (DWORD*)input;
		switch (sizeof(T))
		{
		case 8:
		{
			__storewordbytereverse(*word, 0, &temp);
			__storewordbytereverse(*(word + 1), 4, &temp);
		}
		break;

		case 4:
			__storewordbytereverse(*word, 0, &temp);
			break;

		case 2:
			__storeshortbytereverse(*input, 0, &temp);
			break;

		default:
			Assert("Invalid size in CByteswap::LowLevelByteSwap" && 0);
		}
#else
		for (int i = 0; i < (int)sizeof(T); i++)
		{
			((unsigned char*)&temp)[i] = ((unsigned char*)input)[sizeof(T) - (i + 1)];
		}
#endif
		memcpy(output, &temp, sizeof(T));
	}

	unsigned int m_bSwapBytes : 1;
	unsigned int m_bBigEndian : 1;
};

//-----------------------------------------------------------------------------
// Copy a single field from the input buffer to the output buffer, swapping the bytes if necessary
//-----------------------------------------------------------------------------
void CByteswap::SwapFieldToTargetEndian(void* pOutputBuffer, void *pData, typedescription_t *pField)
{
	switch (pField->fieldType)
	{
	case FIELD_CHARACTER:
		SwapBufferToTargetEndian<char>((char*)pOutputBuffer, (char*)pData, pField->fieldSize);
		break;

	case FIELD_BOOLEAN:
		SwapBufferToTargetEndian<bool>((bool*)pOutputBuffer, (bool*)pData, pField->fieldSize);
		break;

	case FIELD_SHORT:
		SwapBufferToTargetEndian<short>((short*)pOutputBuffer, (short*)pData, pField->fieldSize);
		break;

	case FIELD_FLOAT:
		SwapBufferToTargetEndian<unsigned int>((unsigned int*)pOutputBuffer, (unsigned int*)pData, pField->fieldSize);
		break;

	case FIELD_INTEGER:
		SwapBufferToTargetEndian<int>((int*)pOutputBuffer, (int*)pData, pField->fieldSize);
		break;

	case FIELD_VECTOR:
		SwapBufferToTargetEndian<unsigned int>((unsigned int*)pOutputBuffer, (unsigned int*)pData, pField->fieldSize * 3);
		break;

	case FIELD_VECTOR2D:
		SwapBufferToTargetEndian<unsigned int>((unsigned int*)pOutputBuffer, (unsigned int*)pData, pField->fieldSize * 2);
		break;

	case FIELD_QUATERNION:
		SwapBufferToTargetEndian<unsigned int>((unsigned int*)pOutputBuffer, (unsigned int*)pData, pField->fieldSize * 4);
		break;

	case FIELD_EMBEDDED:
	{
		typedescription_t *pEmbed = pField->td->dataDesc;
		for (int i = 0; i < pField->fieldSize; ++i)
		{
			SwapFieldsToTargetEndian((char*)pOutputBuffer + pEmbed->fieldOffset,
				(char*)pData + pEmbed->fieldOffset,
				pField->td);

			pOutputBuffer = (char*)pOutputBuffer + pField->fieldSizeInBytes;
			pData = (char*)pData + pField->fieldSizeInBytes;
		}
	}
	break;

	}
}

//-----------------------------------------------------------------------------
// Write a block of fields. Works a bit like the saverestore code.  
//-----------------------------------------------------------------------------
void CByteswap::SwapFieldsToTargetEndian(void *pOutputBuffer, void *pBaseData, datamap_t *pDataMap)
{
	// deal with base class first
	if (pDataMap->baseMap)
	{
		SwapFieldsToTargetEndian(pOutputBuffer, pBaseData, pDataMap->baseMap);
	}

	typedescription_t *pFields = pDataMap->dataDesc;
	int fieldCount = pDataMap->dataNumFields;
	for (int i = 0; i < fieldCount; ++i)
	{
		typedescription_t *pField = &pFields[i];
		SwapFieldToTargetEndian((char*)pOutputBuffer + pField->fieldOffset,
			(char*)pBaseData + pField->fieldOffset,
			pField);
	}
}

//-----------------------------------------------------------------------------
// Description of character conversions for string output
// Here's an example of how to use the macros to define a character conversion
// BEGIN_CHAR_CONVERSION( CStringConversion, '\\' )
//	{ '\n', "n" },
//	{ '\t', "t" }
// END_CHAR_CONVERSION( CStringConversion, '\\' )
//-----------------------------------------------------------------------------
class CUtlCharConversion
{
public:
	struct ConversionArray_t
	{
		char m_nActualChar;
		const char *m_pReplacementString;
	};

	CUtlCharConversion(char nEscapeChar, const char *pDelimiter, int nCount, ConversionArray_t *pArray);
	char GetEscapeChar() const;
	const char *GetDelimiter() const;
	int GetDelimiterLength() const;

	const char *GetConversionString(char c) const;
	int GetConversionLength(char c) const;
	int MaxConversionLength() const;

	// Finds a conversion for the passed-in string, returns length
	virtual char FindConversion(const char *pString, int *pLength);

protected:
	struct ConversionInfo_t
	{
		int m_nLength;
		const char *m_pReplacementString;
	};

	char m_nEscapeChar;
	const char *m_pDelimiter;
	int m_nDelimiterLength;
	int m_nCount;
	int m_nMaxConversionLength;
	char m_pList[255];
	ConversionInfo_t m_pReplacements[255];
};

//-----------------------------------------------------------------------------
// Character conversions for C strings
//-----------------------------------------------------------------------------
class CUtlCStringConversion : public CUtlCharConversion
{
public:
	CUtlCStringConversion(char nEscapeChar, const char *pDelimiter, int nCount, ConversionArray_t *pArray);

	// Finds a conversion for the passed-in string, returns length
	virtual char FindConversion(const char *pString, int *pLength);

private:
	char m_pConversion[255];
};


//-----------------------------------------------------------------------------
// Character conversions for no-escape sequence strings
//-----------------------------------------------------------------------------
class CUtlNoEscConversion : public CUtlCharConversion
{
public:
	CUtlNoEscConversion(char nEscapeChar, const char *pDelimiter, int nCount, ConversionArray_t *pArray) :
		CUtlCharConversion(nEscapeChar, pDelimiter, nCount, pArray) {}

	// Finds a conversion for the passed-in string, returns length
	virtual char FindConversion(const char *pString, int *pLength) { *pLength = 0; return 0; }
};

struct characterset_t
{
	char set[256];
};

#define BEGIN_CHAR_CONVERSION( _name, _delimiter, _escapeChar )	\
	static CUtlCharConversion::ConversionArray_t s_pConversionArray ## _name[] = {

#define END_CHAR_CONVERSION( _name, _delimiter, _escapeChar ) \
	}; \
	CUtlCharConversion _name( _escapeChar, _delimiter, sizeof( s_pConversionArray ## _name ) / sizeof( CUtlCharConversion::ConversionArray_t ), s_pConversionArray ## _name );

#define BEGIN_CUSTOM_CHAR_CONVERSION( _className, _name, _delimiter, _escapeChar ) \
	static CUtlCharConversion::ConversionArray_t s_pConversionArray ## _name[] = {

#define END_CUSTOM_CHAR_CONVERSION( _className, _name, _delimiter, _escapeChar ) \
	}; \
	_className _name( _escapeChar, _delimiter, sizeof( s_pConversionArray ## _name ) / sizeof( CUtlCharConversion::ConversionArray_t ), s_pConversionArray ## _name );


//-----------------------------------------------------------------------------
// List of character conversions
//-----------------------------------------------------------------------------
BEGIN_CUSTOM_CHAR_CONVERSION(CUtlCStringConversion, s_StringCharConversion, "\"", '\\')
{
	'\n', "n"
},
{ '\t', "t" },
{ '\v', "v" },
{ '\b', "b" },
{ '\r', "r" },
{ '\f', "f" },
{ '\a', "a" },
{ '\\', "\\" },
{ '\?', "\?" },
{ '\'', "\'" },
{ '\"', "\"" },
END_CUSTOM_CHAR_CONVERSION(CUtlCStringConversion, s_StringCharConversion, "\"", '\\')

CUtlCharConversion *GetCStringCharConversion()
{
	return &s_StringCharConversion;
}

BEGIN_CUSTOM_CHAR_CONVERSION(CUtlNoEscConversion, s_NoEscConversion, "\"", 0x7F)
{
0x7F, ""
},
END_CUSTOM_CHAR_CONVERSION(CUtlNoEscConversion, s_NoEscConversion, "\"", 0x7F)

	CUtlCharConversion *GetNoEscCharConversion()
{
	return &s_NoEscConversion;
}


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CUtlCStringConversion::CUtlCStringConversion(char nEscapeChar, const char *pDelimiter, int nCount, ConversionArray_t *pArray) :
	CUtlCharConversion(nEscapeChar, pDelimiter, nCount, pArray)
{
	memset(m_pConversion, 0x0, sizeof(m_pConversion));
	for (int i = 0; i < nCount; ++i)
	{
		m_pConversion[static_cast<size_t>(pArray[i].m_pReplacementString[0])] = pArray[i].m_nActualChar;
	}
}

// Finds a conversion for the passed-in string, returns length
char CUtlCStringConversion::FindConversion(const char *pString, int *pLength)
{
	char c = m_pConversion[static_cast<size_t>(pString[0])];
	*pLength = (c != '\0') ? 1 : 0;
	return c;
}



//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CUtlCharConversion::CUtlCharConversion(char nEscapeChar, const char *pDelimiter, int nCount, ConversionArray_t *pArray)
{
	m_nEscapeChar = nEscapeChar;
	m_pDelimiter = pDelimiter;
	m_nCount = nCount;
	m_nDelimiterLength = strlen(pDelimiter);
	m_nMaxConversionLength = 0;

	memset(m_pReplacements, 0, sizeof(m_pReplacements));

	for (int i = 0; i < nCount; ++i)
	{
		m_pList[i] = pArray[i].m_nActualChar;
		ConversionInfo_t &info = m_pReplacements[static_cast<size_t>(m_pList[i])];
		Assert(info.m_pReplacementString == 0);
		info.m_pReplacementString = pArray[i].m_pReplacementString;
		info.m_nLength = strlen(info.m_pReplacementString);
		if (info.m_nLength > m_nMaxConversionLength)
		{
			m_nMaxConversionLength = info.m_nLength;
		}
	}
}


//-----------------------------------------------------------------------------
// Escape character + delimiter
//-----------------------------------------------------------------------------
char CUtlCharConversion::GetEscapeChar() const
{
	return m_nEscapeChar;
}

const char *CUtlCharConversion::GetDelimiter() const
{
	return m_pDelimiter;
}

int CUtlCharConversion::GetDelimiterLength() const
{
	return m_nDelimiterLength;
}


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
const char *CUtlCharConversion::GetConversionString(char c) const
{
	return m_pReplacements[static_cast<size_t>(c)].m_pReplacementString;
}

int CUtlCharConversion::GetConversionLength(char c) const
{
	return m_pReplacements[static_cast<size_t>(c)].m_nLength;
}

int CUtlCharConversion::MaxConversionLength() const
{
	return m_nMaxConversionLength;
}


//-----------------------------------------------------------------------------
// Finds a conversion for the passed-in string, returns length
//-----------------------------------------------------------------------------
char CUtlCharConversion::FindConversion(const char *pString, int *pLength)
{
	for (int i = 0; i < m_nCount; ++i)
	{
		if (!strcmp(pString, m_pReplacements[static_cast<size_t>(m_pList[i])].m_pReplacementString))
		{
			*pLength = m_pReplacements[static_cast<size_t>(m_pList[i])].m_nLength;
			return m_pList[i];
		}
	}

	*pLength = 0;
	return '\0';
}


//-----------------------------------------------------------------------------
// Macro to set overflow functions easily
//-----------------------------------------------------------------------------
#define SetUtlBufferOverflowFuncs( _get, _put )	\
	SetOverflowFuncs( static_cast <UtlBufferOverflowFunc_t>( _get ), static_cast <UtlBufferOverflowFunc_t>( _put ) )


//-----------------------------------------------------------------------------
// Command parsing..
//-----------------------------------------------------------------------------
class CUtlBuffer
{
public:
	enum SeekType_t
	{
		SEEK_HEAD = 0,
		SEEK_CURRENT,
		SEEK_TAIL
	};

	// flags
	enum BufferFlags_t
	{
		TEXT_BUFFER = 0x1,			// Describes how get + put work (as strings, or binary)
		EXTERNAL_GROWABLE = 0x2,	// This is used w/ external buffers and causes the utlbuf to switch to reallocatable memory if an overflow happens when Putting.
		CONTAINS_CRLF = 0x4,		// For text buffers only, does this contain \n or \n\r?
		READ_ONLY = 0x8,			// For external buffers; prevents null termination from happening.
		AUTO_TABS_DISABLED = 0x10,	// Used to disable/enable push/pop tabs
	};

	// Overflow functions when a get or put overflows
	typedef bool (CUtlBuffer::*UtlBufferOverflowFunc_t)(int nSize);

	// Constructors for growable + external buffers for serialization/unserialization
	CUtlBuffer(int growSize = 0, int initSize = 0, int nFlags = 0);
	CUtlBuffer(const void* pBuffer, int size, int nFlags = 0);
	// This one isn't actually defined so that we catch contructors that are trying to pass a bool in as the third param.
	CUtlBuffer(const void *pBuffer, int size, bool crap);

	unsigned char	GetFlags() const;

	// NOTE: This will assert if you attempt to recast it in a way that
	// is not compatible. The only valid conversion is binary-> text w/CRLF
	void			SetBufferType(bool bIsText, bool bContainsCRLF);

	// Makes sure we've got at least this much memory
	void			EnsureCapacity(int num);

	// Attaches the buffer to external memory....
	void			SetExternalBuffer(void* pMemory, int nSize, int nInitialPut, int nFlags = 0);
	bool			IsExternallyAllocated() const;
	void			AssumeMemory(void *pMemory, int nSize, int nInitialPut, int nFlags = 0);

	inline void ActivateByteSwappingIfBigEndian(void)
	{
		
	}


	// Controls endian-ness of binary utlbufs - default matches the current platform
	void			ActivateByteSwapping(bool bActivate);
	void			SetBigEndian(bool bigEndian);
	bool			IsBigEndian(void);

	// Resets the buffer; but doesn't free memory
	void			Clear();

	// Clears out the buffer; frees memory
	void			Purge();

	// Read stuff out.
	// Binary mode: it'll just read the bits directly in, and characters will be
	//		read for strings until a null character is reached.
	// Text mode: it'll parse the file, turning text #s into real numbers.
	//		GetString will read a string until a space is reached
	char			GetChar();
	unsigned char	GetUnsignedChar();
	short			GetShort();
	unsigned short	GetUnsignedShort();
	int				GetInt();
	int				GetIntHex();
	unsigned int	GetUnsignedInt();
	float			GetFloat();
	double			GetDouble();
	void			GetString(char* pString, int nMaxChars = 0);
	void			Get(void* pMem, int size);
	void			GetLine(char* pLine, int nMaxChars = 0);

	// Used for getting objects that have a byteswap datadesc defined
	template <typename T> void GetObjects(T *dest, int count = 1);

	// This will get at least 1 byte and up to nSize bytes. 
	// It will return the number of bytes actually read.
	int				GetUpTo(void *pMem, int nSize);

	// This version of GetString converts \" to \\ and " to \, etc.
	// It also reads a " at the beginning and end of the string
	void			GetDelimitedString(CUtlCharConversion *pConv, char *pString, int nMaxChars = 0);
	char			GetDelimitedChar(CUtlCharConversion *pConv);

	// This will return the # of characters of the string about to be read out
	// NOTE: The count will *include* the terminating 0!!
	// In binary mode, it's the number of characters until the next 0
	// In text mode, it's the number of characters until the next space.
	int				PeekStringLength();

	// This version of PeekStringLength converts \" to \\ and " to \, etc.
	// It also reads a " at the beginning and end of the string
	// NOTE: The count will *include* the terminating 0!!
	// In binary mode, it's the number of characters until the next 0
	// In text mode, it's the number of characters between "s (checking for \")
	// Specifying false for bActualSize will return the pre-translated number of characters
	// including the delimiters and the escape characters. So, \n counts as 2 characters when bActualSize == false
	// and only 1 character when bActualSize == true
	int				PeekDelimitedStringLength(CUtlCharConversion *pConv, bool bActualSize = true);

	// Just like scanf, but doesn't work in binary mode
	int				Scanf(const char* pFmt, ...);
	int				VaScanf(const char* pFmt, va_list list);

	// Eats white space, advances Get index
	void			EatWhiteSpace();

	// Eats C++ style comments
	bool			EatCPPComment();

	// (For text buffers only)
	// Parse a token from the buffer:
	// Grab all text that lies between a starting delimiter + ending delimiter
	// (skipping whitespace that leads + trails both delimiters).
	// If successful, the get index is advanced and the function returns true,
	// otherwise the index is not advanced and the function returns false.
	bool			ParseToken(const char *pStartingDelim, const char *pEndingDelim, char* pString, int nMaxLen);

	// Advance the get index until after the particular string is found
	// Do not eat whitespace before starting. Return false if it failed
	// String test is case-insensitive.
	bool			GetToken(const char *pToken);

	// Parses the next token, given a set of character breaks to stop at
	// Returns the length of the token parsed in bytes (-1 if none parsed)
	int				ParseToken(characterset_t *pBreaks, char *pTokenBuf, int nMaxLen, bool bParseComments = true);

	// Write stuff in
	// Binary mode: it'll just write the bits directly in, and strings will be
	//		written with a null terminating character
	// Text mode: it'll convert the numbers to text versions
	//		PutString will not write a terminating character
	void			PutChar(char c);
	void			PutUnsignedChar(unsigned char uc);
	void			PutShort(short s);
	void			PutUnsignedShort(unsigned short us);
	void			PutInt(int i);
	void			PutUnsignedInt(unsigned int u);
	void			PutFloat(float f);
	void			PutDouble(double d);
	void			PutString(const char* pString);
	void			Put(const void* pMem, int size);

	// Used for putting objects that have a byteswap datadesc defined
	template <typename T> void PutObjects(T *src, int count = 1);

	// This version of PutString converts \ to \\ and " to \", etc.
	// It also places " at the beginning and end of the string
	void			PutDelimitedString(CUtlCharConversion *pConv, const char *pString);
	void			PutDelimitedChar(CUtlCharConversion *pConv, char c);

	// Just like printf, writes a terminating zero in binary mode
	void			Printf(const char* pFmt, ...);
	void			VaPrintf(const char* pFmt, va_list list);

	// What am I writing (put)/reading (get)?
	void* PeekPut(int offset = 0);
	const void* PeekGet(int offset = 0) const;
	const void* PeekGet(int nMaxSize, int nOffset);

	// Where am I writing (put)/reading (get)?
	int TellPut() const;
	int TellGet() const;

	// What's the most I've ever written?
	int TellMaxPut() const;

	// How many bytes remain to be read?
	// NOTE: This is not accurate for streaming text files; it overshoots
	int GetBytesRemaining() const;

	// Change where I'm writing (put)/reading (get)
	void SeekPut(SeekType_t type, int offset);
	void SeekGet(SeekType_t type, int offset);

	// Buffer base
	const void* Base() const;
	void* Base();

	// memory allocation size, does *not* reflect size written or read,
	//	use TellPut or TellGet for that
	int Size() const;

	// Am I a text buffer?
	bool IsText() const;

	// Can I grow if I'm externally allocated?
	bool IsGrowable() const;

	// Am I valid? (overflow or underflow error), Once invalid it stays invalid
	bool IsValid() const;

	// Do I contain carriage return/linefeeds? 
	bool ContainsCRLF() const;

	// Am I read-only
	bool IsReadOnly() const;

	// Converts a buffer from a CRLF buffer to a CR buffer (and back)
	// Returns false if no conversion was necessary (and outBuf is left untouched)
	// If the conversion occurs, outBuf will be cleared.
	bool ConvertCRLF(CUtlBuffer &outBuf);

	// Push/pop pretty-printing tabs
	void PushTab();
	void PopTab();

	// Temporarily disables pretty print
	void EnableTabs(bool bEnable);

protected:
	// error flags
	enum
	{
		PUT_OVERFLOW = 0x1,
		GET_OVERFLOW = 0x2,
		MAX_ERROR_FLAG = GET_OVERFLOW,
	};

	void SetOverflowFuncs(UtlBufferOverflowFunc_t getFunc, UtlBufferOverflowFunc_t putFunc);

	bool OnPutOverflow(int nSize);
	bool OnGetOverflow(int nSize);

protected:
	// Checks if a get/put is ok
	bool CheckPut(int size);
	bool CheckGet(int size);

	void AddNullTermination();

	// Methods to help with pretty-printing
	bool WasLastCharacterCR();
	void PutTabs();

	// Help with delimited stuff
	char GetDelimitedCharInternal(CUtlCharConversion *pConv);
	void PutDelimitedCharInternal(CUtlCharConversion *pConv, char c);

	// Default overflow funcs
	bool PutOverflow(int nSize);
	bool GetOverflow(int nSize);

	// Does the next bytes of the buffer match a pattern?
	bool PeekStringMatch(int nOffset, const char *pString, int nLen);

	// Peek size of line to come, check memory bound
	int	PeekLineLength();

	// How much whitespace should I skip?
	int PeekWhiteSpace(int nOffset);

	// Checks if a peek get is ok
	bool CheckPeekGet(int nOffset, int nSize);

	// Call this to peek arbitrarily long into memory. It doesn't fail unless
	// it can't read *anything* new
	bool CheckArbitraryPeekGet(int nOffset, int &nIncrement);

	template <typename T> void GetType(T& dest, const char *pszFmt);
	template <typename T> void GetTypeBin(T& dest);
	template <typename T> void GetObject(T *src);

	template <typename T> void PutType(T src, const char *pszFmt);
	template <typename T> void PutTypeBin(T src);
	template <typename T> void PutObject(T *src);

	CUtlMemory<unsigned char> m_Memory;
	int m_Get;
	int m_Put;

	unsigned char m_Error;
	unsigned char m_Flags;
	unsigned char m_Reserved;
#if defined( _X360 )
	unsigned char pad;
#endif

	int m_nTab;
	int m_nMaxPut;
	int m_nOffset;

	UtlBufferOverflowFunc_t m_GetOverflowFunc;
	UtlBufferOverflowFunc_t m_PutOverflowFunc;

	CByteswap	m_Byteswap;
};

//-----------------------------------------------------------------------------
// constructors
//-----------------------------------------------------------------------------
CUtlBuffer::CUtlBuffer(int growSize, int initSize, int nFlags) :
	m_Memory(growSize, initSize), m_Error(0)
{
	m_Get = 0;
	m_Put = 0;
	m_nTab = 0;
	m_nOffset = 0;
	m_Flags = nFlags;
	if ((initSize != 0) && !IsReadOnly())
	{
		m_nMaxPut = -1;
		AddNullTermination();
	}
	else
	{
		m_nMaxPut = 0;
	}
	SetOverflowFuncs(&CUtlBuffer::GetOverflow, &CUtlBuffer::PutOverflow);
}

CUtlBuffer::CUtlBuffer(const void *pBuffer, int nSize, int nFlags) :
	m_Memory((unsigned char*)pBuffer, nSize), m_Error(0)
{
	Assert(nSize != 0);

	m_Get = 0;
	m_Put = 0;
	m_nTab = 0;
	m_nOffset = 0;
	m_Flags = nFlags;
	if (IsReadOnly())
	{
		m_nMaxPut = nSize;
	}
	else
	{
		m_nMaxPut = -1;
		AddNullTermination();
	}
	SetOverflowFuncs(&CUtlBuffer::GetOverflow, &CUtlBuffer::PutOverflow);
}


//-----------------------------------------------------------------------------
// Modifies the buffer to be binary or text; Blows away the buffer and the CONTAINS_CRLF value. 
//-----------------------------------------------------------------------------
void CUtlBuffer::SetBufferType(bool bIsText, bool bContainsCRLF)
{
#ifdef _DEBUG
	// If the buffer is empty, there is no opportunity for this stuff to fail
	if (TellMaxPut() != 0)
	{
		if (IsText())
		{
			if (bIsText)
			{
				Assert(ContainsCRLF() == bContainsCRLF);
			}
			else
			{
				Assert(ContainsCRLF());
			}
		}
		else
		{
			if (bIsText)
			{
				Assert(bContainsCRLF);
			}
		}
	}
#endif

	if (bIsText)
	{
		m_Flags |= TEXT_BUFFER;
	}
	else
	{
		m_Flags &= ~TEXT_BUFFER;
	}
	if (bContainsCRLF)
	{
		m_Flags |= CONTAINS_CRLF;
	}
	else
	{
		m_Flags &= ~CONTAINS_CRLF;
	}
}


//-----------------------------------------------------------------------------
// Attaches the buffer to external memory....
//-----------------------------------------------------------------------------
void CUtlBuffer::SetExternalBuffer(void* pMemory, int nSize, int nInitialPut, int nFlags)
{
	m_Memory.SetExternalBuffer((unsigned char*)pMemory, nSize);

	// Reset all indices; we just changed memory
	m_Get = 0;
	m_Put = nInitialPut;
	m_nTab = 0;
	m_Error = 0;
	m_nOffset = 0;
	m_Flags = nFlags;
	m_nMaxPut = -1;
	AddNullTermination();
}

//-----------------------------------------------------------------------------
// Assumes an external buffer but manages its deletion
//-----------------------------------------------------------------------------
void CUtlBuffer::AssumeMemory(void *pMemory, int nSize, int nInitialPut, int nFlags)
{
	m_Memory.AssumeMemory((unsigned char*)pMemory, nSize);

	// Reset all indices; we just changed memory
	m_Get = 0;
	m_Put = nInitialPut;
	m_nTab = 0;
	m_Error = 0;
	m_nOffset = 0;
	m_Flags = nFlags;
	m_nMaxPut = -1;
	AddNullTermination();
}

//-----------------------------------------------------------------------------
// Makes sure we've got at least this much memory
//-----------------------------------------------------------------------------
void CUtlBuffer::EnsureCapacity(int num)
{
	// Add one extra for the null termination
	num += 1;
	if (m_Memory.IsExternallyAllocated())
	{
		if (IsGrowable() && (m_Memory.NumAllocated() < num))
		{
			m_Memory.ConvertToGrowableMemory(0);
		}
		else
		{
			num -= 1;
		}
	}

	m_Memory.EnsureCapacity(num);
}


//-----------------------------------------------------------------------------
// Base get method from which all others derive
//-----------------------------------------------------------------------------
void CUtlBuffer::Get(void* pMem, int size)
{
	if (CheckGet(size))
	{
		memcpy(pMem, &m_Memory[m_Get - m_nOffset], size);
		m_Get += size;
	}
}


//-----------------------------------------------------------------------------
// This will get at least 1 byte and up to nSize bytes. 
// It will return the number of bytes actually read.
//-----------------------------------------------------------------------------
int CUtlBuffer::GetUpTo(void *pMem, int nSize)
{
	if (CheckArbitraryPeekGet(0, nSize))
	{
		memcpy(pMem, &m_Memory[m_Get - m_nOffset], nSize);
		m_Get += nSize;
		return nSize;
	}
	return 0;
}


//-----------------------------------------------------------------------------
// Eats whitespace
//-----------------------------------------------------------------------------
void CUtlBuffer::EatWhiteSpace()
{
	if (IsText() && IsValid())
	{
		while (CheckGet(sizeof(char)))
		{
			if (!isspace(*(const unsigned char*)PeekGet()))
				break;
			m_Get += sizeof(char);
		}
	}
}


//-----------------------------------------------------------------------------
// Eats C++ style comments
//-----------------------------------------------------------------------------
bool CUtlBuffer::EatCPPComment()
{
	if (IsText() && IsValid())
	{
		// If we don't have a a c++ style comment next, we're done
		const char *pPeek = (const char *)PeekGet(2 * sizeof(char), 0);
		if (!pPeek || (pPeek[0] != '/') || (pPeek[1] != '/'))
			return false;

		// Deal with c++ style comments
		m_Get += 2;

		// read complete line
		for (char c = GetChar(); IsValid(); c = GetChar())
		{
			if (c == '\n')
				break;
		}
		return true;
	}
	return false;
}


//-----------------------------------------------------------------------------
// Peeks how much whitespace to eat
//-----------------------------------------------------------------------------
int CUtlBuffer::PeekWhiteSpace(int nOffset)
{
	if (!IsText() || !IsValid())
		return 0;

	while (CheckPeekGet(nOffset, sizeof(char)))
	{
		if (!isspace(*(unsigned char*)PeekGet(nOffset)))
			break;
		nOffset += sizeof(char);
	}

	return nOffset;
}


//-----------------------------------------------------------------------------
// Peek size of sting to come, check memory bound
//-----------------------------------------------------------------------------
int	CUtlBuffer::PeekStringLength()
{
	if (!IsValid())
		return 0;

	// Eat preceeding whitespace
	int nOffset = 0;
	if (IsText())
	{
		nOffset = PeekWhiteSpace(nOffset);
	}

	int nStartingOffset = nOffset;

	do
	{
		int nPeekAmount = 128;

		// NOTE: Add 1 for the terminating zero!
		if (!CheckArbitraryPeekGet(nOffset, nPeekAmount))
		{
			if (nOffset == nStartingOffset)
				return 0;
			return nOffset - nStartingOffset + 1;
		}

		const char *pTest = (const char *)PeekGet(nOffset);

		if (!IsText())
		{
			for (int i = 0; i < nPeekAmount; ++i)
			{
				// The +1 here is so we eat the terminating 0
				if (pTest[i] == 0)
					return (i + nOffset - nStartingOffset + 1);
			}
		}
		else
		{
			for (int i = 0; i < nPeekAmount; ++i)
			{
				// The +1 here is so we eat the terminating 0
				if (isspace((unsigned char)pTest[i]) || (pTest[i] == 0))
					return (i + nOffset - nStartingOffset + 1);
			}
		}

		nOffset += nPeekAmount;

	} while (true);
}


//-----------------------------------------------------------------------------
// Peek size of line to come, check memory bound
//-----------------------------------------------------------------------------
int	CUtlBuffer::PeekLineLength()
{
	if (!IsValid())
		return 0;

	int nOffset = 0;
	int nStartingOffset = nOffset;

	do
	{
		int nPeekAmount = 128;

		// NOTE: Add 1 for the terminating zero!
		if (!CheckArbitraryPeekGet(nOffset, nPeekAmount))
		{
			if (nOffset == nStartingOffset)
				return 0;
			return nOffset - nStartingOffset + 1;
		}

		const char *pTest = (const char *)PeekGet(nOffset);

		for (int i = 0; i < nPeekAmount; ++i)
		{
			// The +2 here is so we eat the terminating '\n' and 0
			if (pTest[i] == '\n' || pTest[i] == '\r')
				return (i + nOffset - nStartingOffset + 2);
			// The +1 here is so we eat the terminating 0
			if (pTest[i] == 0)
				return (i + nOffset - nStartingOffset + 1);
		}

		nOffset += nPeekAmount;

	} while (true);
}


//-----------------------------------------------------------------------------
// Does the next bytes of the buffer match a pattern?
//-----------------------------------------------------------------------------
bool CUtlBuffer::PeekStringMatch(int nOffset, const char *pString, int nLen)
{
	if (!CheckPeekGet(nOffset, nLen))
		return false;
	return !strncmp((const char*)PeekGet(nOffset), pString, nLen);
}


//-----------------------------------------------------------------------------
// This version of PeekStringLength converts \" to \\ and " to \, etc.
// It also reads a " at the beginning and end of the string
//-----------------------------------------------------------------------------
int CUtlBuffer::PeekDelimitedStringLength(CUtlCharConversion *pConv, bool bActualSize)
{
	if (!IsText() || !pConv)
		return PeekStringLength();

	// Eat preceeding whitespace
	int nOffset = 0;
	if (IsText())
	{
		nOffset = PeekWhiteSpace(nOffset);
	}

	if (!PeekStringMatch(nOffset, pConv->GetDelimiter(), pConv->GetDelimiterLength()))
		return 0;

	// Try to read ending ", but don't accept \"
	int nActualStart = nOffset;
	nOffset += pConv->GetDelimiterLength();
	int nLen = 1;	// Starts at 1 for the '\0' termination

	do
	{
		if (PeekStringMatch(nOffset, pConv->GetDelimiter(), pConv->GetDelimiterLength()))
			break;

		if (!CheckPeekGet(nOffset, 1))
			break;

		char c = *(const char*)PeekGet(nOffset);
		++nLen;
		++nOffset;
		if (c == pConv->GetEscapeChar())
		{
			int nLength = pConv->MaxConversionLength();
			if (!CheckArbitraryPeekGet(nOffset, nLength))
				break;

			pConv->FindConversion((const char*)PeekGet(nOffset), &nLength);
			nOffset += nLength;
		}
	} while (true);

	return bActualSize ? nLen : nOffset - nActualStart + pConv->GetDelimiterLength() + 1;
}


//-----------------------------------------------------------------------------
// Reads a null-terminated string
//-----------------------------------------------------------------------------
void CUtlBuffer::GetString(char* pString, int nMaxChars)
{
	if (!IsValid())
	{
		*pString = 0;
		return;
	}

	if (nMaxChars == 0)
	{
		nMaxChars = INT_MAX;
	}

	// Remember, this *includes* the null character
	// It will be 0, however, if the buffer is empty.
	int nLen = PeekStringLength();

	if (IsText())
	{
		EatWhiteSpace();
	}

	if (nLen == 0)
	{
		*pString = 0;
		m_Error |= GET_OVERFLOW;
		return;
	}

	// Strip off the terminating NULL
	if (nLen <= nMaxChars)
	{
		Get(pString, nLen - 1);
		pString[nLen - 1] = 0;
	}
	else
	{
		Get(pString, nMaxChars - 1);
		pString[nMaxChars - 1] = 0;
		SeekGet(SEEK_CURRENT, nLen - 1 - nMaxChars);
	}

	// Read the terminating NULL in binary formats
	if (!IsText())
	{
		VerifyEquals(GetChar(), 0);
	}
}


//-----------------------------------------------------------------------------
// Reads up to and including the first \n
//-----------------------------------------------------------------------------
void CUtlBuffer::GetLine(char* pLine, int nMaxChars)
{
	Assert(IsText() && !ContainsCRLF());

	if (!IsValid())
	{
		*pLine = 0;
		return;
	}

	if (nMaxChars == 0)
	{
		nMaxChars = INT_MAX;
	}

	// Remember, this *includes* the null character
	// It will be 0, however, if the buffer is empty.
	int nLen = PeekLineLength();
	if (nLen == 0)
	{
		*pLine = 0;
		m_Error |= GET_OVERFLOW;
		return;
	}

	// Strip off the terminating NULL
	if (nLen <= nMaxChars)
	{
		Get(pLine, nLen - 1);
		pLine[nLen - 1] = 0;
	}
	else
	{
		Get(pLine, nMaxChars - 1);
		pLine[nMaxChars - 1] = 0;
		SeekGet(SEEK_CURRENT, nLen - 1 - nMaxChars);
	}
}


//-----------------------------------------------------------------------------
// This version of GetString converts \ to \\ and " to \", etc.
// It also places " at the beginning and end of the string
//-----------------------------------------------------------------------------
char CUtlBuffer::GetDelimitedCharInternal(CUtlCharConversion *pConv)
{
	char c = GetChar();
	if (c == pConv->GetEscapeChar())
	{
		int nLength = pConv->MaxConversionLength();
		if (!CheckArbitraryPeekGet(0, nLength))
			return '\0';

		c = pConv->FindConversion((const char *)PeekGet(), &nLength);
		SeekGet(SEEK_CURRENT, nLength);
	}

	return c;
}

char CUtlBuffer::GetDelimitedChar(CUtlCharConversion *pConv)
{
	if (!IsText() || !pConv)
		return GetChar();
	return GetDelimitedCharInternal(pConv);
}

void CUtlBuffer::GetDelimitedString(CUtlCharConversion *pConv, char *pString, int nMaxChars)
{
	if (!IsText() || !pConv)
	{
		GetString(pString, nMaxChars);
		return;
	}

	if (!IsValid())
	{
		*pString = 0;
		return;
	}

	if (nMaxChars == 0)
	{
		nMaxChars = INT_MAX;
	}

	EatWhiteSpace();
	if (!PeekStringMatch(0, pConv->GetDelimiter(), pConv->GetDelimiterLength()))
		return;

	// Pull off the starting delimiter
	SeekGet(SEEK_CURRENT, pConv->GetDelimiterLength());

	int nRead = 0;
	while (IsValid())
	{
		if (PeekStringMatch(0, pConv->GetDelimiter(), pConv->GetDelimiterLength()))
		{
			SeekGet(SEEK_CURRENT, pConv->GetDelimiterLength());
			break;
		}

		char c = GetDelimitedCharInternal(pConv);

		if (nRead < nMaxChars)
		{
			pString[nRead] = c;
			++nRead;
		}
	}

	if (nRead >= nMaxChars)
	{
		nRead = nMaxChars - 1;
	}
	pString[nRead] = '\0';
}


//-----------------------------------------------------------------------------
// Checks if a get is ok
//-----------------------------------------------------------------------------
bool CUtlBuffer::CheckGet(int nSize)
{
	if (m_Error & GET_OVERFLOW)
		return false;

	if (TellMaxPut() < m_Get + nSize)
	{
		m_Error |= GET_OVERFLOW;
		return false;
	}

	if ((m_Get < m_nOffset) || (m_Memory.NumAllocated() < m_Get - m_nOffset + nSize))
	{
		if (!OnGetOverflow(nSize))
		{
			m_Error |= GET_OVERFLOW;
			return false;
		}
	}

	return true;
}


//-----------------------------------------------------------------------------
// Checks if a peek get is ok
//-----------------------------------------------------------------------------
bool CUtlBuffer::CheckPeekGet(int nOffset, int nSize)
{
	if (m_Error & GET_OVERFLOW)
		return false;

	// Checking for peek can't set the overflow flag
	bool bOk = CheckGet(nOffset + nSize);
	m_Error &= ~GET_OVERFLOW;
	return bOk;
}


//-----------------------------------------------------------------------------
// Call this to peek arbitrarily long into memory. It doesn't fail unless
// it can't read *anything* new
//-----------------------------------------------------------------------------
bool CUtlBuffer::CheckArbitraryPeekGet(int nOffset, int &nIncrement)
{
	if (TellGet() + nOffset >= TellMaxPut())
	{
		nIncrement = 0;
		return false;
	}

	if (TellGet() + nOffset + nIncrement > TellMaxPut())
	{
		nIncrement = TellMaxPut() - TellGet() - nOffset;
	}

	// NOTE: CheckPeekGet could modify TellMaxPut for streaming files
	// We have to call TellMaxPut again here
	CheckPeekGet(nOffset, nIncrement);
	int nMaxGet = TellMaxPut() - TellGet();
	if (nMaxGet < nIncrement)
	{
		nIncrement = nMaxGet;
	}
	return (nIncrement != 0);
}


//-----------------------------------------------------------------------------
// Peek part of the butt
//-----------------------------------------------------------------------------
const void* CUtlBuffer::PeekGet(int nMaxSize, int nOffset)
{
	if (!CheckPeekGet(nOffset, nMaxSize))
		return NULL;
	return &m_Memory[m_Get + nOffset - m_nOffset];
}


//-----------------------------------------------------------------------------
// Change where I'm reading
//-----------------------------------------------------------------------------
void CUtlBuffer::SeekGet(SeekType_t type, int offset)
{
	switch (type)
	{
	case SEEK_HEAD:
		m_Get = offset;
		break;

	case SEEK_CURRENT:
		m_Get += offset;
		break;

	case SEEK_TAIL:
		m_Get = m_nMaxPut - offset;
		break;
	}

	if (m_Get > m_nMaxPut)
	{
		m_Error |= GET_OVERFLOW;
	}
	else
	{
		m_Error &= ~GET_OVERFLOW;
		if (m_Get < m_nOffset || m_Get >= m_nOffset + Size())
		{
			OnGetOverflow(-1);
		}
	}
}


//-----------------------------------------------------------------------------
// Parse...
//-----------------------------------------------------------------------------

#ifdef _MSC_VER
#pragma warning ( disable : 4706 )
#endif

int CUtlBuffer::VaScanf(const char* pFmt, va_list list)
{
	Assert(pFmt);
	if (m_Error || !IsText())
		return 0;

	int numScanned = 0;
	int nLength;
	char c;
	char* pEnd;
	while ((c = *pFmt++))
	{
		// Stop if we hit the end of the buffer
		if (m_Get >= TellMaxPut())
		{
			m_Error |= GET_OVERFLOW;
			break;
		}

		switch (c)
		{
		case ' ':
			// eat all whitespace
			EatWhiteSpace();
			break;

		case '%':
		{
			// Conversion character... try to convert baby!
			char type = *pFmt++;
			if (type == 0)
				return numScanned;

			switch (type)
			{
			case 'c':
			{
				char* ch = va_arg(list, char *);
				if (CheckPeekGet(0, sizeof(char)))
				{
					*ch = *(const char*)PeekGet();
					++m_Get;
				}
				else
				{
					*ch = 0;
					return numScanned;
				}
			}
			break;

			case 'i':
			case 'd':
			{
				int* i = va_arg(list, int *);

				// NOTE: This is not bullet-proof; it assumes numbers are < 128 characters
				nLength = 128;
				if (!CheckArbitraryPeekGet(0, nLength))
				{
					*i = 0;
					return numScanned;
				}

				*i = strtol((char*)PeekGet(), &pEnd, 10);
				int nBytesRead = (int)(pEnd - (char*)PeekGet());
				if (nBytesRead == 0)
					return numScanned;
				m_Get += nBytesRead;
			}
			break;

			case 'x':
			{
				int* i = va_arg(list, int *);

				// NOTE: This is not bullet-proof; it assumes numbers are < 128 characters
				nLength = 128;
				if (!CheckArbitraryPeekGet(0, nLength))
				{
					*i = 0;
					return numScanned;
				}

				*i = strtol((char*)PeekGet(), &pEnd, 16);
				int nBytesRead = (int)(pEnd - (char*)PeekGet());
				if (nBytesRead == 0)
					return numScanned;
				m_Get += nBytesRead;
			}
			break;

			case 'u':
			{
				unsigned int* u = va_arg(list, unsigned int *);

				// NOTE: This is not bullet-proof; it assumes numbers are < 128 characters
				nLength = 128;
				if (!CheckArbitraryPeekGet(0, nLength))
				{
					*u = 0;
					return numScanned;
				}

				*u = strtoul((char*)PeekGet(), &pEnd, 10);
				int nBytesRead = (int)(pEnd - (char*)PeekGet());
				if (nBytesRead == 0)
					return numScanned;
				m_Get += nBytesRead;
			}
			break;

			case 'f':
			{
				float* f = va_arg(list, float *);

				// NOTE: This is not bullet-proof; it assumes numbers are < 128 characters
				nLength = 128;
				if (!CheckArbitraryPeekGet(0, nLength))
				{
					*f = 0.0f;
					return numScanned;
				}

				*f = (float)strtod((char*)PeekGet(), &pEnd);
				int nBytesRead = (int)(pEnd - (char*)PeekGet());
				if (nBytesRead == 0)
					return numScanned;
				m_Get += nBytesRead;
			}
			break;

			case 's':
			{
				char* s = va_arg(list, char *);
				GetString(s);
			}
			break;

			default:
			{
				// unimplemented scanf type
				Assert(0);
				return numScanned;
			}
			break;
			}

			++numScanned;
		}
		break;

		default:
		{
			// Here we have to match the format string character
			// against what's in the buffer or we're done.
			if (!CheckPeekGet(0, sizeof(char)))
				return numScanned;

			if (c != *(const char*)PeekGet())
				return numScanned;

			++m_Get;
		}
		}
	}
	return numScanned;
}

#ifdef _MSC_VER
#pragma warning ( default : 4706 )
#endif

int CUtlBuffer::Scanf(const char* pFmt, ...)
{
	va_list args;

	va_start(args, pFmt);
	int count = VaScanf(pFmt, args);
	va_end(args);

	return count;
}

//-----------------------------------------------------------------------------
// Finds a string in another string with a case insensitive test w/ length validation
//-----------------------------------------------------------------------------
char const* strnistr(char const* pStr, char const* pSearch, int n)
{
	AssertValidStringPtr(pStr);
	AssertValidStringPtr(pSearch);

	if (!pStr || !pSearch)
		return 0;

	char const* pLetter = pStr;

	// Check the entire string
	while (*pLetter != 0)
	{
		if (n <= 0)
			return 0;

		// Skip over non-matches
		if (tolower(*pLetter) == tolower(*pSearch))
		{
			int n1 = n - 1;

			// Check for match
			char const* pMatch = pLetter + 1;
			char const* pTest = pSearch + 1;
			while (*pTest != 0)
			{
				if (n1 <= 0)
					return 0;

				// We've run off the end; don't bother.
				if (*pMatch == 0)
					return 0;

				if (tolower(*pMatch) != tolower(*pTest))
					break;

				++pMatch;
				++pTest;
				--n1;
			}

			// Found a match!
			if (*pTest == 0)
				return pLetter;
		}

		++pLetter;
		--n;
	}

	return 0;
}

//-----------------------------------------------------------------------------
// Advance the get index until after the particular string is found
// Do not eat whitespace before starting. Return false if it failed
//-----------------------------------------------------------------------------
bool CUtlBuffer::GetToken(const char *pToken)
{
	Assert(pToken);

	// Look for the token
	int nLen = strlen(pToken);

	int nSizeToCheck = Size() - TellGet() - m_nOffset;

	int nGet = TellGet();
	do
	{
		int nMaxSize = TellMaxPut() - TellGet();
		if (nMaxSize < nSizeToCheck)
		{
			nSizeToCheck = nMaxSize;
		}
		if (nLen > nSizeToCheck)
			break;

		if (!CheckPeekGet(0, nSizeToCheck))
			break;

		const char *pBufStart = (const char*)PeekGet();
		const char *pFoundEnd = strnistr(pBufStart, pToken, nSizeToCheck);
		if (pFoundEnd)
		{
			size_t nOffset = (size_t)pFoundEnd - (size_t)pBufStart;
			SeekGet(CUtlBuffer::SEEK_CURRENT, nOffset + nLen);
			return true;
		}

		SeekGet(CUtlBuffer::SEEK_CURRENT, nSizeToCheck - nLen - 1);
		nSizeToCheck = Size() - (nLen - 1);

	} while (true);

	SeekGet(CUtlBuffer::SEEK_HEAD, nGet);
	return false;
}


//-----------------------------------------------------------------------------
// (For text buffers only)
// Parse a token from the buffer:
// Grab all text that lies between a starting delimiter + ending delimiter
// (skipping whitespace that leads + trails both delimiters).
// Note the delimiter checks are case-insensitive.
// If successful, the get index is advanced and the function returns true,
// otherwise the index is not advanced and the function returns false.
//-----------------------------------------------------------------------------
bool CUtlBuffer::ParseToken(const char *pStartingDelim, const char *pEndingDelim, char* pString, int nMaxLen)
{
	int nCharsToCopy = 0;
	int nCurrentGet = 0;

	size_t nEndingDelimLen;

	// Starting delimiter is optional
	char emptyBuf = '\0';
	if (!pStartingDelim)
	{
		pStartingDelim = &emptyBuf;
	}

	// Ending delimiter is not
	Assert(pEndingDelim && pEndingDelim[0]);
	nEndingDelimLen = strlen(pEndingDelim);

	int nStartGet = TellGet();
	char nCurrChar;
	int nTokenStart = -1;
	EatWhiteSpace();
	while (*pStartingDelim)
	{
		nCurrChar = *pStartingDelim++;
		if (!isspace((unsigned char)nCurrChar))
		{
			if (tolower(GetChar()) != tolower(nCurrChar))
				goto parseFailed;
		}
		else
		{
			EatWhiteSpace();
		}
	}

	EatWhiteSpace();
	nTokenStart = TellGet();
	if (!GetToken(pEndingDelim))
		goto parseFailed;

	nCurrentGet = TellGet();
	nCharsToCopy = (nCurrentGet - nEndingDelimLen) - nTokenStart;
	if (nCharsToCopy >= nMaxLen)
	{
		nCharsToCopy = nMaxLen - 1;
	}

	if (nCharsToCopy > 0)
	{
		SeekGet(CUtlBuffer::SEEK_HEAD, nTokenStart);
		Get(pString, nCharsToCopy);
		if (!IsValid())
			goto parseFailed;

		// Eat trailing whitespace
		for (; nCharsToCopy > 0; --nCharsToCopy)
		{
			if (!isspace((unsigned char)pString[nCharsToCopy - 1]))
				break;
		}
	}
	pString[nCharsToCopy] = '\0';

	// Advance the Get index
	SeekGet(CUtlBuffer::SEEK_HEAD, nCurrentGet);
	return true;

parseFailed:
	// Revert the get index
	SeekGet(SEEK_HEAD, nStartGet);
	pString[0] = '\0';
	return false;
}

#define IN_CHARACTERSET( SetBuffer, character )		((SetBuffer).set[static_cast<size_t>(character)])

//-----------------------------------------------------------------------------
// Parses the next token, given a set of character breaks to stop at
//-----------------------------------------------------------------------------
int CUtlBuffer::ParseToken(characterset_t *pBreaks, char *pTokenBuf, int nMaxLen, bool bParseComments)
{
	Assert(nMaxLen > 0);
	pTokenBuf[0] = 0;

	// skip whitespace + comments
	while (true)
	{
		if (!IsValid())
			return -1;
		EatWhiteSpace();
		if (bParseComments)
		{
			if (!EatCPPComment())
				break;
		}
		else
		{
			break;
		}
	}

	char c = GetChar();

	// End of buffer
	if (c == 0)
		return -1;

	// handle quoted strings specially
	if (c == '\"')
	{
		int nLen = 0;
		while (IsValid())
		{
			c = GetChar();
			if (c == '\"' || !c)
			{
				pTokenBuf[nLen] = 0;
				return nLen;
			}
			pTokenBuf[nLen] = c;
			if (++nLen == nMaxLen)
			{
				pTokenBuf[nLen - 1] = 0;
				return nMaxLen;
			}
		}

		// In this case, we hit the end of the buffer before hitting the end qoute
		pTokenBuf[nLen] = 0;
		return nLen;
	}

	// parse single characters
	if (IN_CHARACTERSET(*pBreaks, c))
	{
		pTokenBuf[0] = c;
		pTokenBuf[1] = 0;
		return 1;
	}

	// parse a regular word
	int nLen = 0;
	while (true)
	{
		pTokenBuf[nLen] = c;
		if (++nLen == nMaxLen)
		{
			pTokenBuf[nLen - 1] = 0;
			return nMaxLen;
		}
		c = GetChar();
		if (!IsValid())
			break;

		if (IN_CHARACTERSET(*pBreaks, c) || c == '\"' || c <= ' ')
		{
			SeekGet(SEEK_CURRENT, -1);
			break;
		}
	}

	pTokenBuf[nLen] = 0;
	return nLen;
}



//-----------------------------------------------------------------------------
// Serialization
//-----------------------------------------------------------------------------
void CUtlBuffer::Put(const void *pMem, int size)
{
	if (size && CheckPut(size))
	{
		memcpy(&m_Memory[m_Put - m_nOffset], pMem, size);
		m_Put += size;

		AddNullTermination();
	}
}


//-----------------------------------------------------------------------------
// Writes a null-terminated string
//-----------------------------------------------------------------------------
void CUtlBuffer::PutString(const char* pString)
{
	if (!IsText())
	{
		if (pString)
		{
			// Not text? append a null at the end.
			size_t nLen = strlen(pString) + 1;
			Put(pString, nLen * sizeof(char));
			return;
		}
		else
		{
			PutTypeBin<char>(0);
		}
	}
	else if (pString)
	{
		int nTabCount = (m_Flags & AUTO_TABS_DISABLED) ? 0 : m_nTab;
		if (nTabCount > 0)
		{
			if (WasLastCharacterCR())
			{
				PutTabs();
			}

			const char* pEndl = strchr(pString, '\n');
			while (pEndl)
			{
				size_t nSize = (size_t)pEndl - (size_t)pString + sizeof(char);
				Put(pString, nSize);
				pString = pEndl + 1;
				if (*pString)
				{
					PutTabs();
					pEndl = strchr(pString, '\n');
				}
				else
				{
					pEndl = NULL;
				}
			}
		}
		size_t nLen = strlen(pString);
		if (nLen)
		{
			Put(pString, nLen * sizeof(char));
		}
	}
}


//-----------------------------------------------------------------------------
// This version of PutString converts \ to \\ and " to \", etc.
// It also places " at the beginning and end of the string
//-----------------------------------------------------------------------------
inline void CUtlBuffer::PutDelimitedCharInternal(CUtlCharConversion *pConv, char c)
{
	int l = pConv->GetConversionLength(c);
	if (l == 0)
	{
		PutChar(c);
	}
	else
	{
		PutChar(pConv->GetEscapeChar());
		Put(pConv->GetConversionString(c), l);
	}
}

void CUtlBuffer::PutDelimitedChar(CUtlCharConversion *pConv, char c)
{
	if (!IsText() || !pConv)
	{
		PutChar(c);
		return;
	}

	PutDelimitedCharInternal(pConv, c);
}

void CUtlBuffer::PutDelimitedString(CUtlCharConversion *pConv, const char *pString)
{
	if (!IsText() || !pConv)
	{
		PutString(pString);
		return;
	}

	if (WasLastCharacterCR())
	{
		PutTabs();
	}
	Put(pConv->GetDelimiter(), pConv->GetDelimiterLength());

	int nLen = pString ? strlen(pString) : 0;
	for (int i = 0; i < nLen; ++i)
	{
		PutDelimitedCharInternal(pConv, pString[i]);
	}

	if (WasLastCharacterCR())
	{
		PutTabs();
	}
	Put(pConv->GetDelimiter(), pConv->GetDelimiterLength());
}

int vsnprintf(char *pDest, int maxLen, char const *pFormat, va_list params)
{
	Assert(maxLen > 0);
	AssertValidWritePtr(pDest, maxLen);
	AssertValidStringPtr(pFormat);

	int len = vsprintf_s(pDest, maxLen, pFormat, params);

	if (len < 0)
	{
		len = maxLen;
		pDest[maxLen - 1] = 0;
	}

	return len;
}

void CUtlBuffer::VaPrintf(const char* pFmt, va_list list)
{
	char temp[2048];
#ifdef _DEBUG	
	int nLen =
#endif
		vsnprintf(temp, sizeof(temp), pFmt, list);
	Assert(nLen < 2048);
	PutString(temp);
}

void CUtlBuffer::Printf(const char* pFmt, ...)
{
	va_list args;

	va_start(args, pFmt);
	VaPrintf(pFmt, args);
	va_end(args);
}


//-----------------------------------------------------------------------------
// Calls the overflow functions
//-----------------------------------------------------------------------------
void CUtlBuffer::SetOverflowFuncs(UtlBufferOverflowFunc_t getFunc, UtlBufferOverflowFunc_t putFunc)
{
	m_GetOverflowFunc = getFunc;
	m_PutOverflowFunc = putFunc;
}


//-----------------------------------------------------------------------------
// Calls the overflow functions
//-----------------------------------------------------------------------------
bool CUtlBuffer::OnPutOverflow(int nSize)
{
	return (this->*m_PutOverflowFunc)(nSize);
}

bool CUtlBuffer::OnGetOverflow(int nSize)
{
	return (this->*m_GetOverflowFunc)(nSize);
}


//-----------------------------------------------------------------------------
// Checks if a put is ok
//-----------------------------------------------------------------------------
bool CUtlBuffer::PutOverflow(int nSize)
{
	if (m_Memory.IsExternallyAllocated())
	{
		if (!IsGrowable())
			return false;

		m_Memory.ConvertToGrowableMemory(0);
	}

	while (Size() < m_Put - m_nOffset + nSize)
	{
		m_Memory.Grow();
	}

	return true;
}

bool CUtlBuffer::GetOverflow(int nSize)
{
	return false;
}


//-----------------------------------------------------------------------------
// Checks if a put is ok
//-----------------------------------------------------------------------------
bool CUtlBuffer::CheckPut(int nSize)
{
	if ((m_Error & PUT_OVERFLOW) || IsReadOnly())
		return false;

	if ((m_Put < m_nOffset) || (m_Memory.NumAllocated() < m_Put - m_nOffset + nSize))
	{
		if (!OnPutOverflow(nSize))
		{
			m_Error |= PUT_OVERFLOW;
			return false;
		}
	}
	return true;
}

void CUtlBuffer::SeekPut(SeekType_t type, int offset)
{
	int nNextPut = m_Put;
	switch (type)
	{
	case SEEK_HEAD:
		nNextPut = offset;
		break;

	case SEEK_CURRENT:
		nNextPut += offset;
		break;

	case SEEK_TAIL:
		nNextPut = m_nMaxPut - offset;
		break;
	}

	// Force a write of the data
	// FIXME: We could make this more optimal potentially by writing out
	// the entire buffer if you seek outside the current range

	// NOTE: This call will write and will also seek the file to nNextPut.
	OnPutOverflow(-nNextPut - 1);
	m_Put = nNextPut;

	AddNullTermination();
}


void CUtlBuffer::ActivateByteSwapping(bool bActivate)
{
	m_Byteswap.ActivateByteSwapping(bActivate);
}

void CUtlBuffer::SetBigEndian(bool bigEndian)
{
	m_Byteswap.SetTargetBigEndian(bigEndian);
}

bool CUtlBuffer::IsBigEndian(void)
{
	return m_Byteswap.IsTargetBigEndian();
}


//-----------------------------------------------------------------------------
// null terminate the buffer
//-----------------------------------------------------------------------------
void CUtlBuffer::AddNullTermination(void)
{
	if (m_Put > m_nMaxPut)
	{
		if (!IsReadOnly() && ((m_Error & PUT_OVERFLOW) == 0))
		{
			// Add null termination value
			if (CheckPut(1))
			{
				m_Memory[m_Put - m_nOffset] = 0;
			}
			else
			{
				// Restore the overflow state, it was valid before...
				m_Error &= ~PUT_OVERFLOW;
			}
		}
		m_nMaxPut = m_Put;
	}
}

const char* strnchr(const char* pStr, char c, int n)
{
	char const* pLetter = pStr;
	char const* pLast = pStr + n;

	// Check the entire string
	while ((pLetter < pLast) && (*pLetter != 0))
	{
		if (*pLetter == c)
			return pLetter;
		++pLetter;
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// Converts a buffer from a CRLF buffer to a CR buffer (and back)
// Returns false if no conversion was necessary (and outBuf is left untouched)
// If the conversion occurs, outBuf will be cleared.
//-----------------------------------------------------------------------------
bool CUtlBuffer::ConvertCRLF(CUtlBuffer &outBuf)
{
	if (!IsText() || !outBuf.IsText())
		return false;

	if (ContainsCRLF() == outBuf.ContainsCRLF())
		return false;

	int nInCount = TellMaxPut();

	outBuf.Purge();
	outBuf.EnsureCapacity(nInCount);

	bool bFromCRLF = ContainsCRLF();

	// Start reading from the beginning
	int nGet = TellGet();
	int nPut = TellPut();
	int nGetDelta = 0;
	int nPutDelta = 0;

	const char *pBase = (const char*)Base();
	int nCurrGet = 0;
	while (nCurrGet < nInCount)
	{
		const char *pCurr = &pBase[nCurrGet];
		if (bFromCRLF)
		{
			const char *pNext = strnistr(pCurr, "\r\n", nInCount - nCurrGet);
			if (!pNext)
			{
				outBuf.Put(pCurr, nInCount - nCurrGet);
				break;
			}

			int nBytes = (size_t)pNext - (size_t)pCurr;
			outBuf.Put(pCurr, nBytes);
			outBuf.PutChar('\n');
			nCurrGet += nBytes + 2;
			if (nGet >= nCurrGet - 1)
			{
				--nGetDelta;
			}
			if (nPut >= nCurrGet - 1)
			{
				--nPutDelta;
			}
		}
		else
		{
			const char *pNext = strnchr(pCurr, '\n', nInCount - nCurrGet);
			if (!pNext)
			{
				outBuf.Put(pCurr, nInCount - nCurrGet);
				break;
			}

			int nBytes = (size_t)pNext - (size_t)pCurr;
			outBuf.Put(pCurr, nBytes);
			outBuf.PutChar('\r');
			outBuf.PutChar('\n');
			nCurrGet += nBytes + 1;
			if (nGet >= nCurrGet)
			{
				++nGetDelta;
			}
			if (nPut >= nCurrGet)
			{
				++nPutDelta;
			}
		}
	}

	Assert(nPut + nPutDelta <= outBuf.TellMaxPut());

	outBuf.SeekGet(SEEK_HEAD, nGet + nGetDelta);
	outBuf.SeekPut(SEEK_HEAD, nPut + nPutDelta);

	return true;
}


//---------------------------------------------------------------------------
// Implementation of CUtlInplaceBuffer
//---------------------------------------------------------------------------
class CUtlInplaceBuffer : public CUtlBuffer
{
public:
	CUtlInplaceBuffer(int growSize = 0, int initSize = 0, int nFlags = 0);

	//
	// Routines returning buffer-inplace-pointers
	//
public:
	//
	// Upon success, determines the line length, fills out the pointer to the
	// beginning of the line and the line length, advances the "get" pointer
	// offset by the line length and returns "true".
	//
	// If end of file is reached or upon error returns "false".
	//
	// Note:	the returned length of the line is at least one character because the
	//			trailing newline characters are also included as part of the line.
	//
	// Note:	the pointer returned points into the local memory of this buffer, in
	//			case the buffer gets relocated or destroyed the pointer becomes invalid.
	//
	// e.g.:	-------------
	//
	//			char *pszLine;
	//			int nLineLen;
	//			while ( pUtlInplaceBuffer->InplaceGetLinePtr( &pszLine, &nLineLen ) )
	//			{
	//				...
	//			}
	//
	//			-------------
	//
	// @param	ppszInBufferPtr		on return points into this buffer at start of line
	// @param	pnLineLength		on return holds num bytes accessible via (*ppszInBufferPtr)
	//
	// @returns	true				if line was successfully read
	//			false				when EOF is reached or error occurs
	//
	bool InplaceGetLinePtr( /* out */ char **ppszInBufferPtr, /* out */ int *pnLineLength);

	//
	// Determines the line length, advances the "get" pointer offset by the line length,
	// replaces the newline character with null-terminator and returns the initial pointer
	// to now null-terminated line.
	//
	// If end of file is reached or upon error returns NULL.
	//
	// Note:	the pointer returned points into the local memory of this buffer, in
	//			case the buffer gets relocated or destroyed the pointer becomes invalid.
	//
	// e.g.:	-------------
	//
	//			while ( char *pszLine = pUtlInplaceBuffer->InplaceGetLinePtr() )
	//			{
	//				...
	//			}
	//
	//			-------------
	//
	// @returns	ptr-to-zero-terminated-line		if line was successfully read and buffer modified
	//			NULL							when EOF is reached or error occurs
	//
	char * InplaceGetLinePtr(void);
};

CUtlInplaceBuffer::CUtlInplaceBuffer(int growSize /* = 0 */, int initSize /* = 0 */, int nFlags /* = 0 */) :
	CUtlBuffer(growSize, initSize, nFlags)
{

}

bool CUtlInplaceBuffer::InplaceGetLinePtr(char **ppszInBufferPtr, int *pnLineLength)
{
	Assert(IsText() && !ContainsCRLF());

	int nLineLen = PeekLineLength();
	if (nLineLen <= 1)
	{
		SeekGet(SEEK_TAIL, 0);
		return false;
	}

	--nLineLen; // because it accounts for putting a terminating null-character

	char *pszLine = (char *) const_cast< void * >(PeekGet());
	SeekGet(SEEK_CURRENT, nLineLen);

	// Set the out args
	if (ppszInBufferPtr)
		*ppszInBufferPtr = pszLine;

	if (pnLineLength)
		*pnLineLength = nLineLen;

	return true;
}

char * CUtlInplaceBuffer::InplaceGetLinePtr(void)
{
	char *pszLine = NULL;
	int nLineLen = 0;

	if (InplaceGetLinePtr(&pszLine, &nLineLen))
	{
		Assert(nLineLen >= 1);

		switch (pszLine[nLineLen - 1])
		{
		case '\n':
		case '\r':
			pszLine[nLineLen - 1] = 0;
			if (--nLineLen)
			{
				switch (pszLine[nLineLen - 1])
				{
				case '\n':
				case '\r':
					pszLine[nLineLen - 1] = 0;
					break;
				}
			}
			break;

		default:
			Assert(pszLine[nLineLen] == 0);
			break;
		}
	}

	return pszLine;
}
