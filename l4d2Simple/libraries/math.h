#pragma once
/*
	TODO: Finish this
*/
#ifndef M_PI
#define M_PI		3.14159265358979323846	// matches value in gcc v2 math.h
#endif

#define M_PI_F		((float)(M_PI))	// Shouldn't collide with anything.
#ifndef RAD2DEG
#define RAD2DEG( x  )  ( (float)(x) * (float)(180.f / M_PI_F) )
#endif

#ifndef DEG2RAD
#define DEG2RAD( x  )  ( (float)(x) * (float)(M_PI_F / 180.f) )
#endif

enum
{
	PITCH = 0,	// up / down
	YAW,		// left / right
	ROLL		// fall over
};

// Math routines done in optimized assembly math package routines
void inline SinCos(float radians, float *sine, float *cosine)
{
	*sine = sin(radians);
	*cosine = cos(radians);
}

float VectorNormalize(Vector& v)
{
	int		i;
	float	length;

	length = 0;
	for (i = 0; i< 3; i++)
		length += v[i] * v[i];
	length = sqrtf(length);

	for (i = 0; i< 3; i++)
		v[i] /= length;

	return length;
}

void AngleNormalize(Vector& angles)
{
	for (int i = 0; i < 3; ++i)
	{
		if (angles[i] < -180.0f)
			angles[i] += 360.0f;

		if (angles[i] > 180.0f)
			angles[i] -= 360.0f;
	}
}

void ClampAngles(Vector& angles)
{
	if (angles.x > 89.0f && angles.x <= 180.0f)
		angles.x = 89.0f;

	if (angles.x > 180.0f)
		angles.x = angles.x - 360.0f;

	if (angles.x < -89.0f)
		angles.x = -89.0f;

	angles.y = fmodf(angles.y + 180, 360) - 180;

	angles.z = 0;
}

void AngleVectors(const QAngle &angles, Vector *forward)
{
	// Assert(s_bMathlibInitialized);
	// Assert(forward);

	float	sp, sy, cp, cy;

	SinCos(DEG2RAD(angles[YAW]), &sy, &cy);
	SinCos(DEG2RAD(angles[PITCH]), &sp, &cp);

	forward->x = cp*cy;
	forward->y = cp*sy;
	forward->z = -sp;
}

void AngleVectors(const QAngle &angles, Vector *forward, Vector *right, Vector *up)
{
	// Assert(s_bMathlibInitialized);

	float sr, sp, sy, cr, cp, cy;

#ifdef _X360
	fltx4 radians, scale, sine, cosine;
	radians = LoadUnaligned3SIMD(angles.Base());
	scale = ReplicateX4(M_PI_F / 180.f);
	radians = MulSIMD(radians, scale);
	SinCos3SIMD(sine, cosine, radians);
	sp = SubFloat(sine, 0);	sy = SubFloat(sine, 1);	sr = SubFloat(sine, 2);
	cp = SubFloat(cosine, 0);	cy = SubFloat(cosine, 1);	cr = SubFloat(cosine, 2);
#else
	SinCos(DEG2RAD(angles[YAW]), &sy, &cy);
	SinCos(DEG2RAD(angles[PITCH]), &sp, &cp);
	SinCos(DEG2RAD(angles[ROLL]), &sr, &cr);
#endif

	if (forward)
	{
		forward->x = cp*cy;
		forward->y = cp*sy;
		forward->z = -sp;
	}

	if (right)
	{
		right->x = (-1 * sr*sp*cy + -1 * cr*-sy);
		right->y = (-1 * sr*sp*sy + -1 * cr*cy);
		right->z = -1 * sr*cp;
	}

	if (up)
	{
		up->x = (cr*sp*cy + -sr*-sy);
		up->y = (cr*sp*sy + -sr*cy);
		up->z = cr*cp;
	}
}

void AngleVectorsTranspose(const QAngle &angles, Vector *forward, Vector *right, Vector *up)
{
	// Assert(s_bMathlibInitialized);
	float sr, sp, sy, cr, cp, cy;

	SinCos(DEG2RAD(angles[YAW]), &sy, &cy);
	SinCos(DEG2RAD(angles[PITCH]), &sp, &cp);
	SinCos(DEG2RAD(angles[ROLL]), &sr, &cr);

	if (forward)
	{
		forward->x = cp*cy;
		forward->y = (sr*sp*cy + cr*-sy);
		forward->z = (cr*sp*cy + -sr*-sy);
	}

	if (right)
	{
		right->x = cp*sy;
		right->y = (sr*sp*sy + cr*cy);
		right->z = (cr*sp*sy + -sr*cy);
	}

	if (up)
	{
		up->x = -sp;
		up->y = sr*cp;
		up->z = cr*cp;
	}
}

void VectorAngles(const Vector& forward, QAngle &angles)
{
	// Assert(s_bMathlibInitialized);
	float	tmp, yaw, pitch;

	if (forward[1] == 0 && forward[0] == 0)
	{
		yaw = 0;
		if (forward[2] > 0)
			pitch = 270;
		else
			pitch = 90;
	}
	else
	{
		yaw = (atan2(forward[1], forward[0]) * 180 / M_PI);
		if (yaw < 0)
			yaw += 360;

		tmp = sqrt(forward[0] * forward[0] + forward[1] * forward[1]);
		pitch = (atan2(-forward[2], tmp) * 180 / M_PI);
		if (pitch < 0)
			pitch += 360;
	}

	angles[0] = pitch;
	angles[1] = yaw;
	angles[2] = 0;
}

void VectorAngles(const Vector & forward, const Vector & pseudoup, QAngle & angles)
{
	Vector left;

	CrossProduct(pseudoup, forward, left);
	VectorNormalize(left);

	float xyDist = sqrtf(forward[0] * forward[0] + forward[1] * forward[1]);

	// enough here to get angles?
	if (xyDist > 0.001f)
	{
		// (yaw)	y = ATAN( forward.y, forward.x );		-- in our space, forward is the X axis
		angles[1] = RAD2DEG(atan2f(forward[1], forward[0]));

		// The engine does pitch inverted from this, but we always end up negating it in the DLL
		// UNDONE: Fix the engine to make it consistent
		// (pitch)	x = ATAN( -forward.z, sqrt(forward.x*forward.x+forward.y*forward.y) );
		angles[0] = RAD2DEG(atan2f(-forward[2], xyDist));

		float up_z = (left[1] * forward[0]) - (left[0] * forward[1]);

		// (roll)	z = ATAN( left.z, up.z );
		angles[2] = RAD2DEG(atan2f(left[2], up_z));
	}
	else	// forward is mostly Z, gimbal lock-
	{
		// (yaw)	y = ATAN( -left.x, left.y );			-- forward is mostly z, so use right for yaw
		angles[1] = RAD2DEG(atan2f(-left[0], left[1])); //This was originally copied from the "void MatrixAngles( const matrix3x4_t& matrix, float *angles )" code, and it's 180 degrees off, negated the values and it all works now (Dave Kircher)

														// The engine does pitch inverted from this, but we always end up negating it in the DLL
														// UNDONE: Fix the engine to make it consistent
														// (pitch)	x = ATAN( -forward.z, sqrt(forward.x*forward.x+forward.y*forward.y) );
		angles[0] = RAD2DEG(atan2f(-forward[2], xyDist));

		// Assume no roll in this case as one degree of freedom has been lost (i.e. yaw == roll)
		angles[2] = 0;
	}
}

void CrossProduct(const float* v1, const float* v2, float* cross)
{
	// Assert(s_bMathlibInitialized);
	// Assert(v1 != cross);
	// Assert(v2 != cross);
	cross[0] = v1[1] * v2[2] - v1[2] * v2[1];
	cross[1] = v1[2] * v2[0] - v1[0] * v2[2];
	cross[2] = v1[0] * v2[1] - v1[1] * v2[0];
}

/*
inline void _SSE_RSqrtInline(float a, float* out)
{
	__m128  xx = _mm_load_ss(&a);
	__m128  xr = _mm_rsqrt_ss(xx);
	__m128  xt;
	xt = _mm_mul_ss(xr, xr);
	xt = _mm_mul_ss(xt, xx);
	xt = _mm_sub_ss(_mm_set_ss(3.f), xt);
	xt = _mm_mul_ss(xt, _mm_set_ss(0.5f));
	xr = _mm_mul_ss(xr, xt);
	_mm_store_ss(out, xr);
}

FORCEINLINE float VectorNormalize(Vector& vec)
{
#ifndef DEBUG // stop crashing my edit-and-continue!
#if defined(__i386__) || defined(_M_IX86)
#define DO_SSE_OPTIMIZATION
#endif
#endif

#if defined( DO_SSE_OPTIMIZATION )
	float sqrlen = vec.LengthSqr() + 1.0e-10f, invlen;
	_SSE_RSqrtInline(sqrlen, &invlen);
	vec.x *= invlen;
	vec.y *= invlen;
	vec.z *= invlen;
	return sqrlen * invlen;
#else
	extern float (FASTCALL *pfVectorNormalize)(Vector& v);
	return (*pfVectorNormalize)(vec);
#endif
}
*/

void MatrixGetColumn(const matrix3x4_t& in, int column, Vector &out)
{
	out.x = in[0][column];
	out.y = in[1][column];
	out.z = in[2][column];
}

void MatrixSetColumn(const Vector &in, int column, matrix3x4_t& out)
{
	out[0][column] = in.x;
	out[1][column] = in.y;
	out[2][column] = in.z;
}

void SetIdentityMatrix(matrix3x4_t& matrix)
{
	memset(matrix.Base(), 0, sizeof(float)* 3 * 4);
	matrix[0][0] = 1.0;
	matrix[1][1] = 1.0;
	matrix[2][2] = 1.0;
}

void SetScaleMatrix(float x, float y, float z, matrix3x4_t &dst);
void MatrixBuildRotationAboutAxis(const Vector &vAxisOfRot, float angleDegrees, matrix3x4_t &dst);
void VectorTransform(class Vector const &, struct matrix3x4_t const &, class Vector &);

float QuaternionNormalize(Quaternion &q)
{
	// Assert( s_bMathlibInitialized );
	float radius, iradius;

	// Assert(q.IsValid());

	radius = q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3];

	if (radius) // > FLT_EPSILON && ((radius < 1.0f - 4*FLT_EPSILON) || (radius > 1.0f + 4*FLT_EPSILON))
	{
		radius = sqrt(radius);
		iradius = 1.0f / radius;
		q[3] *= iradius;
		q[2] *= iradius;
		q[1] *= iradius;
		q[0] *= iradius;
	}
	return radius;
}
void MatrixAngles(const matrix3x4_t &matrix, Quaternion &q, Vector &pos)
{
#ifdef _VPROF_MATHLIB
	VPROF_BUDGET("MatrixQuaternion", "Mathlib");
#endif
	float trace;
	trace = matrix[0][0] + matrix[1][1] + matrix[2][2] + 1.0f;
	if (trace > 1.0f + FLT_EPSILON)
	{
		// VPROF_INCREMENT_COUNTER("MatrixQuaternion A",1);
		q.x = (matrix[2][1] - matrix[1][2]);
		q.y = (matrix[0][2] - matrix[2][0]);
		q.z = (matrix[1][0] - matrix[0][1]);
		q.w = trace;
	}
	else if (matrix[0][0] > matrix[1][1] && matrix[0][0] > matrix[2][2])
	{
		// VPROF_INCREMENT_COUNTER("MatrixQuaternion B",1);
		trace = 1.0f + matrix[0][0] - matrix[1][1] - matrix[2][2];
		q.x = trace;
		q.y = (matrix[1][0] + matrix[0][1]);
		q.z = (matrix[0][2] + matrix[2][0]);
		q.w = (matrix[2][1] - matrix[1][2]);
	}
	else if (matrix[1][1] > matrix[2][2])
	{
		// VPROF_INCREMENT_COUNTER("MatrixQuaternion C",1);
		trace = 1.0f + matrix[1][1] - matrix[0][0] - matrix[2][2];
		q.x = (matrix[0][1] + matrix[1][0]);
		q.y = trace;
		q.z = (matrix[2][1] + matrix[1][2]);
		q.w = (matrix[0][2] - matrix[2][0]);
	}
	else
	{
		// VPROF_INCREMENT_COUNTER("MatrixQuaternion D",1);
		trace = 1.0f + matrix[2][2] - matrix[0][0] - matrix[1][1];
		q.x = (matrix[0][2] + matrix[2][0]);
		q.y = (matrix[2][1] + matrix[1][2]);
		q.z = trace;
		q.w = (matrix[1][0] - matrix[0][1]);
	}

	QuaternionNormalize(q);

#if 0
	// check against the angle version
	RadianEuler ang;
	MatrixAngles(matrix, ang);
	Quaternion test;
	AngleQuaternion(ang, test);
	float d = QuaternionDotProduct(q, test);
	Assert(fabs(d) > 0.99 && fabs(d) < 1.01);
#endif

	MatrixGetColumn(matrix, 3, pos);
}

void VectorVectors(const Vector & forward, Vector & right, Vector & up)
{
	Vector tmp;

	if (forward[0] == 0 && forward[1] == 0)
	{
		// pitch 90 degrees up/down from identity
		right[0] = 0;
		right[1] = -1;
		right[2] = 0;
		up[0] = -forward[2];
		up[1] = 0;
		up[2] = 0;
	}
	else
	{
		tmp[0] = 0; tmp[1] = 0; tmp[2] = 1.0;
		CrossProduct(forward, tmp, right);
		VectorNormalize(right);
		CrossProduct(right, forward, up);
		VectorNormalize(up);
	}
}

Vector CalculateAim(const Vector &origin, const Vector &target)
{
	Vector angles;
	Vector deltaPos = target - origin;

	angles.y = atan2(deltaPos.y, deltaPos.x) * 180 / M_PI;
	angles.x = atan2(-(deltaPos.z), sqrt(deltaPos.x * deltaPos.x + deltaPos.y * deltaPos.y)) * 180 / M_PI;
	angles.z = 0.0f;

	AngleNormalize(angles);
	return angles;
}

Vector CalcAngle(const Vector& source, const Vector& destination)
{
	Vector delta(source - destination);
	Vector angles;

	double hypotenuse = sqrt(delta.x * delta.x + delta.y * delta.y);

	angles.x = static_cast<double>(atan(delta.z / hypotenuse) * M_PI);
	angles.y = static_cast<double>(atan(delta.y / delta.z) * M_PI);
	angles.z = 0.0;

	if (delta.x >= 0.0)
		angles.y += 180.0;

	AngleNormalize(angles);
	return angles;
}

inline void VectorTransform(const Vector& in1, const VMatrix &in2, Vector &out)
{
	out[0] = DotProduct(in1, Vector(in2[0][0], in2[0][1], in2[0][2])) + in2[0][3];
	out[1] = DotProduct(in1, Vector(in2[1][0], in2[1][1], in2[1][2])) + in2[1][3];
	out[2] = DotProduct(in1, Vector(in2[2][0], in2[2][1], in2[2][2])) + in2[2][3];
}

float GetAnglesFieldOfView(const Vector& myAngles, const Vector& aimAngles)
{
	static auto getFov = [](float orgViewX, float orgViewY, float ViewX, float ViewY, float& fovX, float& fovY) -> void
	{
		fovX = std::abs(orgViewX - ViewX);
		fovY = std::abs(orgViewY - ViewY);

		if (fovY < -180) fovY += 360;

		if (fovY > 180) fovY -= 360;
	};

	static auto getDistance = [](float Xhere, float Yhere) -> float
	{
		Xhere = std::abs(Xhere);
		Yhere = std::abs(Yhere);

		Xhere *= Xhere;
		Yhere *= Yhere;
		float combined = (Xhere + Yhere);
		return sqrt(combined);
	};

	Vector toHim(0.0f, 0.0f, 0.0f);
	getFov(myAngles.x, myAngles.y, aimAngles.x, aimAngles.y, toHim.x, toHim.y);
	return getDistance(toHim.x, toHim.y);
}

bool WorldToScreen(const Vector &point, Vector &out)
{
	int m_iWidth, m_iHeight;
	g_cInterfaces.Engine->GetScreenSize(m_iWidth, m_iHeight);
	const VMatrix &worldToScreen = g_cInterfaces.Engine->WorldToScreenMatrix();
	float w = worldToScreen[3][0] * point[0] + worldToScreen[3][1] * point[1] + worldToScreen[3][2] * point[2] + worldToScreen[3][3];
	out.z = 0;
	if (w > 0.01)
	{
		float w1 = 1 / w;
		out.x = m_iWidth / 2 + (0.5 * ((worldToScreen[0][0] * point[0] + worldToScreen[0][1] * point[1] + worldToScreen[0][2] * point[2] + worldToScreen[0][3]) * w1) * m_iWidth + 0.5);
		out.y = m_iHeight / 2 - (0.5 * ((worldToScreen[1][0] * point[0] + worldToScreen[1][1] * point[1] + worldToScreen[1][2] * point[2] + worldToScreen[1][3]) * w1) * m_iHeight + 0.5);
		return true;
	}
	return false;
}
