#pragma once
#include <cmath>

#define M_PI 3.14159265358979323846

#define CHECK_VALID(_v)		Assert((_v).IsValid())
#define FLOAT32_NAN_BITS	(unsigned long)0x7FC00000
#define FLOAT32_NAN			BitsToFloat( FLOAT32_NAN_BITS )
#define VEC_T_NAN			FLOAT32_NAN

class Vector;
inline float IsFinite(float f);
inline float BitsToFloat(unsigned long i);
inline float VectorLength(const Vector & v);
inline float DotProduct(const Vector & a, const Vector & b);
inline void VectorSubtract(const Vector& a, const Vector& b, Vector& c);
inline void CrossProduct(const Vector & a, const Vector & b, Vector & result);

class Vector
{
public:
	float x, y, z;
	inline void Init(float ix, float iy, float iz)
	{
		x = ix; y = iy; z = iz;
	}

	Vector() { Invalidate(); };
	Vector(float X, float Y, float Z) { x = X; y = Y; z = Z; };

	float operator[](int i) const { if (i == 0) return x; if (i == 1) return y; return z; };
	float& operator[](int i) { if (i == 0) return x; if (i == 1) return y; return z; };

	bool operator==(const Vector& v) { return true; }
	bool operator!=(const Vector& v) { return true; }

	inline Vector operator-(const Vector& v) const { return Vector(x - v.x, y - v.y, z - v.z); }
	inline Vector operator+(const Vector& v) const { return Vector(x + v.x, y + v.y, z + v.z); }
	inline Vector operator*(const Vector& v) const { return Vector(x * v.x, y * v.y, z * v.z); }
	inline Vector operator/(const Vector& v) const { return Vector(x / v.x, y / v.y, z / v.z); }
	inline Vector operator-(const float& f) const { return Vector(x - f, y - f, z - f); }
	inline Vector operator+(const float& f) const { return Vector(x + f, y + f, z + f); }
	inline Vector operator*(const float& f) const { return Vector(x * f, y * f, z * f); }
	inline Vector operator/(const float& f) const { return Vector(x / f, y / f, z / f); }
	// inline Vector operator*(const int n) { return Vector(x*n, y*n, z*n); }
	// inline Vector operator*(const float n) { return Vector(x*n, y*n, z*n); }

	inline Vector operator-() { return Vector(-x, -y, -z); }

	inline Vector& operator+=(const Vector &v) { x += v.x; y += v.y; z += v.z; return *this; }
	inline Vector& operator-=(const Vector &v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
	inline Vector& operator*=(const Vector &v) { x *= v.x; y *= v.y; z *= v.z; return *this; }
	inline Vector& operator/=(const Vector &v) { x /= v.x; y /= v.y; z /= v.z; return *this; }
	inline Vector& operator+=(const float &f) { x += f; y += f; z += f; return *this; }
	inline Vector& operator-=(const float &f) { x -= f; y -= f; z -= f; return *this; }
	inline Vector& operator*=(const float &f) { x *= f; y *= f; z *= f; return *this; }
	inline Vector& operator/=(const float &f) { x /= f; y /= f; z /= f; return *this; }

	inline bool IsValid() const { return IsFinite(x) && IsFinite(y) && IsFinite(z); }
	inline void Invalidate() { x = y = z = VEC_T_NAN; }

	float LengthSqr(void) { return (x*x + y*y + z*z); }
	operator bool() { return IsValid(); };

	inline bool IsZero(float tolerance = 1e-6f) { return (x > -tolerance && x < tolerance && y > -tolerance && y < tolerance && z > -tolerance && z < tolerance); }
	inline bool WithinAABox(const Vector& boxmin, const Vector& boxmax) { return ((x >= boxmin.x) && (x <= boxmax.x) && (y >= boxmin.y) && (y <= boxmax.y) && (z >= boxmin.z) && (z <= boxmax.z)); }

	float DistToSqr(const Vector& vOther)
	{
		Vector delta;

		delta.x = x - vOther.x;
		delta.y = y - vOther.y;
		delta.z = z - vOther.z;

		return delta.LengthSqr();
	}

	void MulAdd(const Vector & a, const Vector & b, float scalar)
	{
		x = a.x + b.x * scalar;
		y = a.y + b.y * scalar;
		z = a.z + b.z * scalar;
	}

	inline float Dot(const Vector & vOther) { return DotProduct(*this, vOther); }
	inline float Length() { return VectorLength(*this); }
	inline float Length2D() { return sqrtf(x * x + y * y); }

	float DistTo(const Vector & vOther)
	{
		Vector delta;
		VectorSubtract(*this, vOther, delta);
		return delta.Length();
	}

	Vector Cross(const Vector & vOther) const
	{
		Vector res;
		CrossProduct(*this, vOther, res);
		return res;
	}

	Vector Min(const Vector & vOther) const { return Vector(x < vOther.x ? x : vOther.x, y < vOther.y ? y : vOther.y, z < vOther.z ? z : vOther.z); }
	Vector Max(const Vector & vOther) const { return Vector(x > vOther.x ? x : vOther.x, y > vOther.y ? y : vOther.y, z > vOther.z ? z : vOther.z); }

	

	//T must be between 0 and 1
	Vector lerp(Vector target, float t)
	{
		return *this * (1 - t) + target * t;
	}

	void lerpme(Vector target, float t)
	{
		*this = *this * (1 - t) + target * t;
	}
};

class Vector2D
{
public:
	Vector2D(float x, float y) : x(x), y(y)
	{}

	float x, y;
};

class Vector4D
{
public:
	Vector4D(float x, float y, float z, float w) : x(x), y(y), z(z), w(w)
	{}

	float x, y, z, w;
};

typedef Vector QAngle;

inline unsigned long & FloatBits(float & f)
{
	return *reinterpret_cast<unsigned long*>(&f);
}

inline unsigned long const& FloatBits(float const& f)
{
	return *reinterpret_cast<unsigned long const*>(&f);
}

inline float BitsToFloat(unsigned long i)
{
	return *reinterpret_cast<float*>(&i);
}

inline float IsFinite(float f)
{
	return ((FloatBits(f) & 0x7F800000) != 0x7F800000);
}

inline unsigned long FloatAbsBits(float f)
{
	return FloatBits(f) & 0x7FFFFFFF;
}

inline float FloatMakeNegative(float f)
{
	return BitsToFloat(FloatBits(f) | 0x80000000);
}

inline float FloatMakePositive(float f)
{
	return fabs(f);
}

inline float FloatNegate(float f)
{
	return BitsToFloat(FloatBits(f) ^ 0x80000000);
}

inline void VectorAdd(const Vector& a, const Vector& b, Vector& c)
{
	c.x = a.x + b.x;
	c.y = a.y + b.y;
	c.z = a.z + b.z;
}

inline void VectorSubtract(const Vector& a, const Vector& b, Vector& c)
{
	c.x = a.x - b.x;
	c.y = a.y - b.y;
	c.z = a.z - b.z;
}

inline void VectorMultiply(const Vector& a, const Vector& b, Vector& c)
{
	c.x = a.x * b.x;
	c.y = a.y * b.y;
	c.z = a.z * b.z;
}

inline void VectorScale(const Vector& in, const Vector& scale, Vector& result)
{
	VectorMultiply(in, scale, result);
}

inline void VectorDivide(const Vector& a, const Vector& b, Vector& c)
{
	c.x = a.x / b.x;
	c.y = a.y / b.y;
	c.z = a.z / b.z;
}

inline void VectorLerp(const Vector & src1, const Vector & src2, float t, Vector & dest)
{
	dest.x = src1.x + (src2.x - src1.x) * t;
	dest.y = src1.y + (src2.y - src1.y) * t;
	dest.z = src1.z + (src2.z - src1.z) * t;
}

inline float DotProduct(const Vector & a, const Vector & b)
{
	return (a.x * b.x + a.y * b.y + a.z * b.z);
}

inline void CrossProduct(const Vector & a, const Vector & b, Vector & result)
{
	result.x = a.y * b.z - a.z * b.y;
	result.y = a.z * b.x - a.x * b.z;
	result.z = a.x * b.y - a.y * b.x;
}

inline float VectorLength(const Vector & v)
{
	return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

inline void VectorMA(const QAngle & start, float scale, const QAngle & direction, QAngle & dest)
{
	dest.x = start.x + scale * direction.x;
	dest.y = start.y + scale * direction.y;
	dest.z = start.z + scale * direction.z;
}

inline void ComputeClosestPoint(const Vector & vecStart, float flMaxDist, const Vector & vecTarget, Vector * pResult)
{
	Vector vecDelta;
	VectorSubtract(vecTarget, vecStart, vecDelta);
	float flDistSqr = vecDelta.LengthSqr();
	if (flDistSqr <= flMaxDist * flMaxDist)
	{
		*pResult = vecTarget;
	}
	else
	{
		vecDelta /= sqrtf(flDistSqr);
		VectorMA((QAngle)vecStart, flMaxDist, (QAngle)vecDelta, (QAngle)*pResult);
	}
}

inline void VectorMin(const Vector & a, const Vector & b, Vector & result)
{
	result.x = fmin(a.x, b.x);
	result.y = fmin(a.y, b.y);
	result.z = fmin(a.z, b.z);
}

inline void VectorMax(const Vector & a, const Vector & b, Vector & result)
{
	result.x = fmax(a.x, b.x);
	result.y = fmax(a.y, b.y);
	result.z = fmax(a.z, b.z);
}

inline bool QAnglesAreEqual(const QAngle & src1, const QAngle & src2, float tolerance)
{
	if (FloatMakePositive(src1.x - src2.x) > tolerance)
		return false;
	if (FloatMakePositive(src1.y - src2.y) > tolerance)
		return false;
	return (FloatMakePositive(src1.z - src2.z) <= tolerance);
}
