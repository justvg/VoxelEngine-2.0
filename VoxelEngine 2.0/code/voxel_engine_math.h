#pragma once

#include <math.h>
#include <xmmintrin.h>

#define PI 3.14159265358979323846f
#define INT_MIN (-2147483647 - 1)
#define INT_MAX 2147483647
#define FLT_MAX 3.402823466e+38F
#define DEG2RAD(Deg) ((Deg)/180.0f*PI)
#define RAD2DEG(Rad) ((Rad)/PI*180.0f)

#define SHUFFLE3(V, X, Y, Z) (vec3(_mm_shuffle_ps((V).m, (V).m, _MM_SHUFFLE(Z, Z, Y, X))))
#define SHUFFLE4(V, X, Y, Z, W) (vec4(_mm_shuffle_ps((V).m, (V).m, _MM_SHUFFLE(W, Z, Y, X))))

typedef float real32;
typedef double real64;

//
// NOTE(georgy): vec3
//

struct vec3
{
	__m128 m;

	inline vec3() { m = _mm_set1_ps(0.0f); } // TODO(georgy): Do I always want to zero vec3??
	inline explicit vec3(real32 *V) { m = _mm_set_ps(V[2], V[2], V[1], V[0]); }
	inline explicit vec3(real32 X, real32 Y, real32 Z) { m = _mm_set_ps(Z, Z, Y, X); }
	inline explicit vec3(__m128 V) { m = V; }
	inline vec3 __vectorcall vec3i(int32_t X, int32_t Y, int32_t Z) { return(vec3((real32)X, (real32)Y, (real32)Z)); }

	inline real32 __vectorcall x() { return(_mm_cvtss_f32(m)); }
	inline real32 __vectorcall y() { return(_mm_cvtss_f32(_mm_shuffle_ps(m, m, _MM_SHUFFLE(1, 1, 1, 1)))); }
	inline real32 __vectorcall z() { return(_mm_cvtss_f32(_mm_shuffle_ps(m, m, _MM_SHUFFLE(2, 2, 2, 2)))); }

	inline vec3 __vectorcall yzx() { return(SHUFFLE3(*this, 1, 2, 0)); }
	inline vec3 __vectorcall zxy() { return(SHUFFLE3(*this, 2, 0, 1)); }

	void __vectorcall SetX(real32 X)
	{
		m = _mm_move_ss(m, _mm_set_ss(X));
	}

	void __vectorcall SetY(real32 Y)
	{
		__m128 Temp = _mm_move_ss(m, _mm_set_ss(Y));
		Temp = _mm_shuffle_ps(Temp, Temp, _MM_SHUFFLE(3, 2, 0, 0));
		m = _mm_move_ss(Temp, m);
	}

	void __vectorcall SetZ(real32 Z)
	{
		__m128 Temp = _mm_move_ss(m, _mm_set_ss(Z));
		Temp = _mm_shuffle_ps(Temp, Temp, _MM_SHUFFLE(3, 0, 1, 0));
		m = _mm_move_ss(Temp, m);
	}

	inline real32 operator[] (size_t I) { return(m.m128_f32[I]); };
};

inline vec3 __vectorcall
operator+ (vec3 A, vec3 B)
{
	A.m = _mm_add_ps(A.m, B.m);
	return(A);
}

inline vec3 __vectorcall
operator- (vec3 A, vec3 B)
{
	A.m = _mm_sub_ps(A.m, B.m);
	return(A);
}

inline vec3 __vectorcall
Hadamard(vec3 A, vec3 B)
{
	A.m = _mm_mul_ps(A.m, B.m);
	return(A);
}

inline vec3 __vectorcall
operator* (vec3 A, real32 B)
{
	A.m = _mm_mul_ps(A.m, _mm_set1_ps(B));
	return(A);
}

inline vec3 __vectorcall
operator* (real32 B, vec3 A)
{
	A = A * B;
	return(A);
}

inline vec3 __vectorcall
operator/ (vec3 A, real32 B)
{
	A = A * (1.0f / B);
	return(A);
}

inline vec3 __vectorcall
operator/ (real32 B, vec3 A)
{
	A.m = _mm_div_ps(_mm_set1_ps(B), A.m);
	return(A);
}

inline vec3 & __vectorcall
operator+= (vec3 &A, vec3 B)
{
	A = A + B;
	return(A);
}

inline vec3 & __vectorcall
operator-= (vec3 &A, vec3 B)
{
	A = A - B;
	return(A);
}

inline vec3 & __vectorcall
operator*= (vec3 &A, real32 B)
{
	A = A * B;
	return(A);
}

inline vec3 __vectorcall
Min(vec3 A, vec3 B)
{
	A.m = _mm_min_ps(A.m, B.m);
	return(A);
}

inline vec3 __vectorcall
Max(vec3 A, vec3 B)
{
	A.m = _mm_max_ps(A.m, B.m);
	return(A);
}

inline vec3 __vectorcall
Clamp(vec3 A, vec3 MinClamp, vec3 MaxClamp)
{
	return(Min(MaxClamp, Max(MinClamp, A)));
}

inline vec3 __vectorcall
operator- (vec3 A)
{
	A = vec3(_mm_setzero_ps()) - A;
	return(A);
}

inline vec3 __vectorcall
Cross(vec3 A, vec3 B)
{
	vec3 Result = (Hadamard(A.zxy(), B) - Hadamard(A, B.zxy())).zxy();
	return(Result);
}

inline real32 __vectorcall
Dot(vec3 A, vec3 B)
{
	vec3 Temp = Hadamard(A, B);
	real32 Result = Temp.x() + Temp.y() + Temp.z();
	return(Result);
}

inline real32 __vectorcall
LengthSq(vec3 A)
{
	return(Dot(A, A));
}

inline real32 __vectorcall
Length(vec3 A)
{
	return(sqrtf(Dot(A, A)));
}

inline vec3 __vectorcall
Normalize(vec3 A)
{
	return(A * (1.0f / Length(A)));
}

inline vec3 __vectorcall
Lerp(vec3 A, vec3 B, real32 t)
{
	return(A + t*(B - A));
}

//
// NOTE(georgy): vec4
//

struct vec4
{
	__m128 m;

	inline vec4() {}
	inline explicit vec4(real32 *V) { m = _mm_set_ps(V[3], V[2], V[1], V[0]); }
	inline explicit vec4(real32 X, real32 Y, real32 Z, real32 W) { m = _mm_set_ps(W, Z, Y, X); }
	inline explicit vec4(__m128 V) { m = V; }
	inline explicit vec4(vec3 V, real32 W) { m = V.m; SetW(W); }
	inline vec4 __vectorcall vec4i(int32_t X, int32_t Y, int32_t Z, int32_t W) { return(vec4((real32)X, (real32)Y, (real32)Z, (real32)W)); }

	inline real32 __vectorcall x() { return(_mm_cvtss_f32(m)); }
	inline real32 __vectorcall y() { return(_mm_cvtss_f32(_mm_shuffle_ps(m, m, _MM_SHUFFLE(1, 1, 1, 1)))); }
	inline real32 __vectorcall z() { return(_mm_cvtss_f32(_mm_shuffle_ps(m, m, _MM_SHUFFLE(2, 2, 2, 2)))); }
	inline real32 __vectorcall w() { return(_mm_cvtss_f32(_mm_shuffle_ps(m, m, _MM_SHUFFLE(3, 3, 3, 3)))); }

	void __vectorcall SetX(real32 X)
	{
		m = _mm_move_ss(m, _mm_set_ss(X));
	}

	void __vectorcall SetY(real32 Y)
	{
		__m128 Temp = _mm_move_ss(m, _mm_set_ss(Y));
		Temp = _mm_shuffle_ps(Temp, Temp, _MM_SHUFFLE(3, 2, 0, 0));
		m = _mm_move_ss(Temp, m);
	}

	void __vectorcall SetZ(real32 Z)
	{
		__m128 Temp = _mm_move_ss(m, _mm_set_ss(Z));
		Temp = _mm_shuffle_ps(Temp, Temp, _MM_SHUFFLE(3, 0, 1, 0));
		m = _mm_move_ss(Temp, m);
	}

	void __vectorcall SetW(real32 W)
	{
		__m128 Temp = _mm_move_ss(m, _mm_set_ss(W));
		Temp = _mm_shuffle_ps(Temp, Temp, _MM_SHUFFLE(0, 2, 1, 0));
		m = _mm_move_ss(Temp, m);
	}

	inline real32 operator[] (size_t I) { return(m.m128_f32[I]); };
};

inline vec4 __vectorcall
operator+ (vec4 A, vec4 B)
{
	A.m = _mm_add_ps(A.m, B.m);
	return(A);
}

inline vec4 __vectorcall
operator- (vec4 A, vec4 B)
{
	A.m = _mm_sub_ps(A.m, B.m);
	return(A);
}

inline vec4 __vectorcall
Hadamard(vec4 A, vec4 B)
{
	A.m = _mm_mul_ps(A.m, B.m);
	return(A);
}

inline vec4 __vectorcall
operator* (vec4 A, real32 B)
{
	A.m = _mm_mul_ps(A.m, _mm_set1_ps(B));
	return(A);
}

inline vec4 __vectorcall
operator* (real32 B, vec4 A)
{
	A = A * B;
	return(A);
}

inline vec4 & __vectorcall
operator+= (vec4 &A, vec4 B)
{
	A = A + B;
	return(A);
}

inline vec4 & __vectorcall
operator-= (vec4 &A, vec4 B)
{
	A = A - B;
	return(A);
}

inline vec4 & __vectorcall
operator*= (vec4 &A, real32 B)
{
	A = A * B;
	return(A);
}

inline vec4 __vectorcall
Min(vec4 A, vec4 B)
{
	A.m = _mm_min_ps(A.m, B.m);
	return(A);
}

inline vec4 __vectorcall
Max(vec4 A, vec4 B)
{
	A.m = _mm_max_ps(A.m, B.m);
	return(A);
}

inline vec4 __vectorcall
Clamp(vec4 A, vec4 MinClamp, vec4 MaxClamp)
{
	return(Min(MaxClamp, Max(MinClamp, A)));
}

inline vec4 __vectorcall
operator- (vec4 A)
{
	A = vec4(_mm_setzero_ps()) - A;
	return(A);
}

inline real32 __vectorcall
Dot(vec4 A, vec4 B)
{
	vec4 Temp = Hadamard(A, B);
	real32 Result = Temp.x() + Temp.y() + Temp.z();
	return(Result);
}

inline real32 __vectorcall
LengthSq(vec4 A)
{
	return(Dot(A, A));
}

inline real32 __vectorcall
Length(vec4 A)
{
	return(sqrtf(Dot(A, A)));
}

inline vec4 __vectorcall
Normalize(vec4 A)
{
	return(A * (1.0f / Length(A)));
}

inline vec4 __vectorcall
Lerp(vec4 A, vec4 B, real32 t)
{
	return(A + t*(B - A));
}

//
// NOTE(georgy): mat4
//

struct mat4
{
	vec4 FirstColumn;
	vec4 SecondColumn;
	vec4 ThirdColumn;
	vec4 FourthColumn;
};

internal mat4
operator* (mat4 A, mat4 B)
{
	mat4 Result;

	Result.FirstColumn = vec4(_mm_setzero_ps());
	Result.FirstColumn += Hadamard(A.FirstColumn, SHUFFLE4(B.FirstColumn, 0, 0, 0, 0));
	Result.FirstColumn += Hadamard(A.SecondColumn, SHUFFLE4(B.FirstColumn, 1, 1, 1, 1));
	Result.FirstColumn += Hadamard(A.ThirdColumn, SHUFFLE4(B.FirstColumn, 2, 2, 2, 2));
	Result.FirstColumn += Hadamard(A.FourthColumn, SHUFFLE4(B.FirstColumn, 3, 3, 3, 3));

	Result.SecondColumn = vec4(_mm_setzero_ps());
	Result.SecondColumn += Hadamard(A.FirstColumn, SHUFFLE4(B.SecondColumn, 0, 0, 0, 0));
	Result.SecondColumn += Hadamard(A.SecondColumn, SHUFFLE4(B.SecondColumn, 1, 1, 1, 1));
	Result.SecondColumn += Hadamard(A.ThirdColumn, SHUFFLE4(B.SecondColumn, 2, 2, 2, 2));
	Result.SecondColumn += Hadamard(A.FourthColumn, SHUFFLE4(B.SecondColumn, 3, 3, 3, 3));

	Result.ThirdColumn = vec4(_mm_setzero_ps());
	Result.ThirdColumn += Hadamard(A.FirstColumn, SHUFFLE4(B.ThirdColumn, 0, 0, 0, 0));
	Result.ThirdColumn += Hadamard(A.SecondColumn, SHUFFLE4(B.ThirdColumn, 1, 1, 1, 1));
	Result.ThirdColumn += Hadamard(A.ThirdColumn, SHUFFLE4(B.ThirdColumn, 2, 2, 2, 2));
	Result.ThirdColumn += Hadamard(A.FourthColumn, SHUFFLE4(B.ThirdColumn, 3, 3, 3, 3));

	Result.FourthColumn = vec4(_mm_setzero_ps());
	Result.FourthColumn += Hadamard(A.FirstColumn, SHUFFLE4(B.FourthColumn, 0, 0, 0, 0));
	Result.FourthColumn += Hadamard(A.SecondColumn, SHUFFLE4(B.FourthColumn, 1, 1, 1, 1));
	Result.FourthColumn += Hadamard(A.ThirdColumn, SHUFFLE4(B.FourthColumn, 2, 2, 2, 2));
	Result.FourthColumn += Hadamard(A.FourthColumn, SHUFFLE4(B.FourthColumn, 3, 3, 3, 3));

	return(Result);
}

inline mat4 __vectorcall
Identity(real32 Diagonal = 1.0f)
{
	mat4 Result;

	Result.FirstColumn = vec4(Diagonal, 0.0f, 0.0f, 0.0f);
	Result.SecondColumn = vec4(0.0f, Diagonal, 0.0f, 0.0f);
	Result.ThirdColumn = vec4(0.0f, 0.0f, Diagonal, 0.0f);
	Result.FourthColumn = vec4(0.0f, 0.0f, 0.0f, Diagonal);

	return(Result);
}

inline mat4 __vectorcall
Translate(vec3 Translation)
{
	mat4 Result;

	Result.FirstColumn = vec4(1.0f, 0.0f, 0.0f, 0.0f);
	Result.SecondColumn = vec4(0.0f, 1.0f, 0.0f, 0.0f);
	Result.ThirdColumn = vec4(0.0f, 0.0f, 1.0f, 0.0f);
	Result.FourthColumn = vec4(Translation, 1.0f);

	return(Result);
}

inline mat4 __vectorcall
Scale(real32 ScaleFactor)
{
	mat4 Result;

	Result.FirstColumn = vec4(ScaleFactor, 0.0f, 0.0f, 0.0f);
	Result.SecondColumn = vec4(0.0f, ScaleFactor, 0.0f, 0.0f);
	Result.ThirdColumn = vec4(0.0f, 0.0f, ScaleFactor, 0.0f);
	Result.FourthColumn = vec4(0.0f, 0.0f, 0.0f, 1.0f);

	return(Result);
}

inline mat4 __vectorcall
Scale(vec3 ScaleFactor)
{
	mat4 Result;

	Result.FirstColumn = vec4(ScaleFactor.x(), 0.0f, 0.0f, 0.0f);
	Result.SecondColumn = vec4(0.0f, ScaleFactor.y(), 0.0f, 0.0f);
	Result.ThirdColumn = vec4(0.0f, 0.0f, ScaleFactor.z(), 0.0f);
	Result.FourthColumn = vec4(0.0f, 0.0f, 0.0f, 1.0f);

	return(Result);
}

internal mat4 __vectorcall
Rotate(r32 Angle, vec3 Axis)
{
	mat4 Result;

	r32 Radians = DEG2RAD(Angle);
	r32 Cos = cosf(Radians);
	r32 Sin = sinf(Radians);

	Axis = Normalize(Axis);
	r32 AxisX = Axis.x();
	r32 AxisY = Axis.y();
	r32 AxisZ = Axis.z();

	Result.FirstColumn = vec4(AxisX*AxisX*(1.0f - Cos) + Cos,
							  AxisX*AxisY*(1.0f - Cos) + AxisZ*Sin,
							  AxisX*AxisZ*(1.0f - Cos) - AxisY*Sin,
							  0.0f);
	Result.SecondColumn = vec4(AxisX*AxisY*(1.0f - Cos) - AxisZ*Sin,
							   AxisY*AxisY*(1.0f - Cos) + Cos,
							   AxisY*AxisZ*(1.0f - Cos) + AxisX*Sin,
							   0.0f);

	Result.ThirdColumn = vec4(AxisX*AxisZ*(1.0f - Cos) + AxisY*Sin,
							  AxisY*AxisZ*(1.0f - Cos) - AxisX*Sin,
							  AxisZ*AxisZ*(1.0f - Cos) + Cos,
							  0.0f);		
	Result.FourthColumn = vec4(0.0f, 0.0f, 0.0f, 1.0f);

	return(Result);
}

internal mat4 __vectorcall
LookAt(vec3 From, vec3 Target, vec3 UpAxis = vec3(0.0f, 1.0f, 0.0f))
{
	vec3 Forward = Normalize(From - Target);
	vec3 Right = Normalize(Cross(UpAxis, Forward));
	vec3 Up = Cross(Forward, Right);

	mat4 Result;

	Result.FirstColumn = vec4(Right.x(), Up.x(), Forward.x(), 0.0f);
	Result.SecondColumn = vec4(Right.y(), Up.y(), Forward.y(), 0.0f);
	Result.ThirdColumn = vec4(Right.z(), Up.z(), Forward.z(), 0.0f);
	Result.FourthColumn = vec4(-Dot(From, Right), -Dot(From, Up), -Dot(From, Forward), 1.0f);

	return(Result);
}

internal mat4 _vectorcall
RotationMatrixFromDirection(vec3 Dir)
{
	vec3 UpAxis = vec3(0.0f, 1.0f, 0.0f);

	vec3 Forward = Normalize(Dir);
	vec3 Right = Normalize(Cross(UpAxis, Forward));
	vec3 Up = Cross(Forward, Right);

	mat4 Result;
	Result.FirstColumn = vec4(Right.x(), Up.x(), Forward.x(), 0.0f);
	Result.SecondColumn = vec4(Right.y(), Up.y(), Forward.y(), 0.0f);
	Result.ThirdColumn = vec4(Right.z(), Up.z(), Forward.z(), 0.0f);
	Result.FourthColumn = vec4(0.0f, 0.0f, 0.0f, 1.0f);

	return(Result);
}

internal mat4 __vectorcall
Perspective(real32 FoV, real32 AspectRatio, real32 Near, real32 Far)
{
	real32 Scale = tanf(DEG2RAD(FoV)*0.5f) * Near;
	real32 Top = Scale;
	real32 Bottom = -Top;
	real32 Right = AspectRatio * Top;
	real32 Left = -Right;

	mat4 Result;

	Result.FirstColumn = vec4(2.0f * Near / (Right - Left), 0.0f, 0.0f, 0.0f);
	Result.SecondColumn = vec4(0.0f, 2.0f * Near / (Top - Bottom), 0.0f, 0.0f);
	Result.ThirdColumn = vec4((Right + Left) / (Right - Left), 
							  (Top + Bottom) / (Top - Bottom), 
							  -(Far + Near) / (Far - Near), 
							  -1.0f);
	Result.FourthColumn = vec4(0.0f, 0.0f, -(2.0f * Far * Near) / (Far - Near), 0.0f);

	return(Result);
}

//
// NOTE(georgy): mat4
//
struct mat3
{
	vec3 FirstColumn;
	vec3 SecondColumn;
	vec3 ThirdColumn;
};

internal mat3
operator* (mat3 A, mat3 B)
{
	mat3 Result;

	Result.FirstColumn = vec3(_mm_setzero_ps());
	Result.FirstColumn += Hadamard(A.FirstColumn, SHUFFLE3(B.FirstColumn, 0, 0, 0));
	Result.FirstColumn += Hadamard(A.SecondColumn, SHUFFLE3(B.FirstColumn, 1, 1, 1));
	Result.FirstColumn += Hadamard(A.ThirdColumn, SHUFFLE3(B.FirstColumn, 2, 2, 2));

	Result.SecondColumn = vec3(_mm_setzero_ps());
	Result.SecondColumn += Hadamard(A.FirstColumn, SHUFFLE3(B.SecondColumn, 0, 0, 0));
	Result.SecondColumn += Hadamard(A.SecondColumn, SHUFFLE3(B.SecondColumn, 1, 1, 1));
	Result.SecondColumn += Hadamard(A.ThirdColumn, SHUFFLE3(B.SecondColumn, 2, 2, 2));

	Result.ThirdColumn = vec3(_mm_setzero_ps());
	Result.ThirdColumn += Hadamard(A.FirstColumn, SHUFFLE3(B.ThirdColumn, 0, 0, 0));
	Result.ThirdColumn += Hadamard(A.SecondColumn, SHUFFLE3(B.ThirdColumn, 1, 1, 1));
	Result.ThirdColumn += Hadamard(A.ThirdColumn, SHUFFLE3(B.ThirdColumn, 2, 2, 2));

	return(Result);
}

inline vec3 __vectorcall
operator* (mat3 A, vec3 V)
{
	vec3 Result = V.x() * A.FirstColumn + V.y() * A.SecondColumn + V.z() * A.ThirdColumn;

	return(Result);
}

inline mat3 __vectorcall
Identity3x3(real32 Diagonal = 1.0f)
{
	mat3 Result;

	Result.FirstColumn = vec3(Diagonal, 0.0f, 0.0f);
	Result.SecondColumn = vec3(0.0f, Diagonal, 0.0f);
	Result.ThirdColumn = vec3(0.0f, 0.0f, Diagonal);

	return(Result);
}

inline mat3 __vectorcall
Scale3x3(real32 ScaleFactor)
{
	mat3 Result;

	Result.FirstColumn = vec3(ScaleFactor, 0.0f, 0.0f);
	Result.SecondColumn = vec3(0.0f, ScaleFactor, 0.0f);
	Result.ThirdColumn = vec3(0.0f, 0.0f, ScaleFactor);

	return(Result);
}

inline mat3 __vectorcall
Scale3x3(vec3 ScaleFactor)
{
	mat3 Result;

	Result.FirstColumn = vec3(ScaleFactor.x(), 0.0f, 0.0f);
	Result.SecondColumn = vec3(0.0f, ScaleFactor.y(), 0.0f);
	Result.ThirdColumn = vec3(0.0f, 0.0f, ScaleFactor.z());

	return(Result);
}

internal mat3 __vectorcall
Rotate3x3(r32 Angle, vec3 Axis)
{
	mat3 Result;

	r32 Radians = DEG2RAD(Angle);
	r32 Cos = cosf(Radians);
	r32 Sin = sinf(Radians);

	Axis = Normalize(Axis);
	r32 AxisX = Axis.x();
	r32 AxisY = Axis.y();
	r32 AxisZ = Axis.z();

	Result.FirstColumn = vec3(AxisX*AxisX*(1.0f - Cos) + Cos,
							  AxisX*AxisY*(1.0f - Cos) + AxisZ*Sin,
							  AxisX*AxisZ*(1.0f - Cos) - AxisY*Sin);
	Result.SecondColumn = vec3(AxisX*AxisY*(1.0f - Cos) - AxisZ*Sin,
							  AxisY*AxisY*(1.0f - Cos) + Cos,
							  AxisY*AxisZ*(1.0f - Cos) + AxisX*Sin);

	Result.ThirdColumn = vec3(AxisX*AxisZ*(1.0f - Cos) + AxisY*Sin,
							  AxisY*AxisZ*(1.0f - Cos) - AxisX*Sin,
							  AxisZ*AxisZ*(1.0f - Cos) + Cos);

	return(Result);
}

//
// NOTE(georgy): vec2
//

union vec2
{
	vec2() {}
	explicit vec2(real32 X, real32 Y) { x = X; y = Y; }

	struct
	{
		real32 x, y;
	};
	real32 E[2];
};

inline vec2
operator+ (vec2 A, vec2 B)
{
	A.x = A.x + B.x;
	A.y = A.y + B.y;
	return(A);
}

inline vec2
operator- (vec2 A, vec2 B)
{
	A.x = A.x - B.x;
	A.y = A.y - B.y;
	return(A);
}

inline vec2
Hadamard(vec2 A, vec2 B)
{
	A.x = A.x * B.x;
	A.y = A.y * B.y;
	return(A);
}

inline vec2
operator* (vec2 A, real32 B)
{
	A.x = A.x * B;
	A.y = A.y * B;
	return(A);
}

inline vec2
operator* (real32 B, vec2 A)
{
	A = A * B;
	return(A);
}

inline vec2 &
operator+= (vec2 &A, vec2 B)
{
	A = A + B;
	return(A);
}

inline vec2 &
operator-= (vec2 &A, vec2 B)
{
	A = A - B;
	return(A);
}

inline vec2 &
operator*= (vec2 &A, real32 B)
{
	A = A * B;
	return(A);
}

inline vec2
operator- (vec2 A)
{
	A.x = -A.x;
	A.y = -A.y;
	return(A);
}

inline real32
Dot(vec2 A, vec2 B)
{
	real32 Result = A.x*B.x + A.y*B.y;
	return(Result);
}

inline real32
LengthSq(vec2 A)
{
	return (Dot(A, A));
}

inline real32
Length(vec2 A)
{
	return (sqrtf(Dot(A, A)));
}

inline vec2
Normalize(vec2 A)
{
	return (A * (1.0f / Length(A)));
}

inline vec2
Lerp(vec2 A, vec2 B, float t)
{
	return (A + (B - A)*t);
}

//
// NOTE(georgy): Scalar
//

inline real32
Clamp(real32 Value, real32 Min, real32 Max)
{
	if (Value < Min) Value = Min;
	if (Value > Max) Value = Max;

	return(Value);
}

// 
// NOTE(georgy): box (rect3 but with 8 vertices)
// 

struct box
{
	vec3 Points[8];
};

internal box
ConstructBoxDim(vec3 Dim)
{
	box Result;

	Result.Points[0] = -0.5f*Dim;
	Result.Points[1] = Result.Points[0] + vec3(0.0f, Dim.y(), 0.0f);
	Result.Points[2] = Result.Points[0] + vec3(0.0f, 0.0f, Dim.z());
	Result.Points[3] = Result.Points[0] + vec3(0.0f, Dim.y(), Dim.z());
	Result.Points[4] = Result.Points[0] + vec3(Dim.x(), 0.0f, 0.0f);
	Result.Points[5] = Result.Points[0] + vec3(Dim.x(), Dim.y(), 0.0f);
	Result.Points[6] = Result.Points[0] + vec3(Dim.x(), 0.0f, Dim.z());
	Result.Points[7] = Result.Points[0] + vec3(Dim.x(), Dim.y(), Dim.z());

	return(Result);
}

internal box __vectorcall
ConstructBox(box *Box, mat3 Transformation, vec3 Translation = vec3(0.0f, 0.0f, 0.0f))
{
	box Result;

	Result.Points[0] = Translation + Transformation * Box->Points[0];
	Result.Points[1] = Translation + Transformation * Box->Points[1];
	Result.Points[2] = Translation + Transformation * Box->Points[2];
	Result.Points[3] = Translation + Transformation * Box->Points[3];
	Result.Points[4] = Translation + Transformation * Box->Points[4];
	Result.Points[5] = Translation + Transformation * Box->Points[5];
	Result.Points[6] = Translation + Transformation * Box->Points[6];
	Result.Points[7] = Translation + Transformation * Box->Points[7];

	return(Result);
}

internal void __vectorcall
TransformBox(box *Box, mat3 Transformation, vec3 Translation = vec3(0.0f, 0.0f, 0.0f))
{
	Box->Points[0] = Translation + Transformation * Box->Points[0];
	Box->Points[1] = Translation + Transformation * Box->Points[1];
	Box->Points[2] = Translation + Transformation * Box->Points[2];
	Box->Points[3] = Translation + Transformation * Box->Points[3];
	Box->Points[4] = Translation + Transformation * Box->Points[4];
	Box->Points[5] = Translation + Transformation * Box->Points[5];
	Box->Points[6] = Translation + Transformation * Box->Points[6];
	Box->Points[7] = Translation + Transformation * Box->Points[7];
}

//
// NOTE(georgy): rect3
//

struct rect3
{
	vec3 Min;
	vec3 Max;
};

inline rect3 __vectorcall
RectMinMax(vec3 Min, vec3 Max)
{
	rect3 Result;
	
	Result.Min = Min;
	Result.Max = Max;

	return(Result);
}

inline rect3 __vectorcall
RectCenterHalfDim(vec3 Center, vec3 HalfDim)
{
	rect3 Result;

	Result.Min = Center - HalfDim;
	Result.Max = Center + HalfDim;

	return(Result);
}

inline rect3 __vectorcall
RectCenterDim(vec3 Center, vec3 Dim)
{
	rect3 Result = RectCenterHalfDim(Center, 0.5f*Dim);

	return(Result);
}

inline rect3 __vectorcall
RectBottomFaceCenterDim(vec3 BottomFaceCenter, vec3 Dim)
{
	vec3 Center = BottomFaceCenter + 0.5f*vec3(0.0f, Dim.y(), 0.0f);
	rect3 Result = RectCenterDim(Center, Dim);

	return(Result);		
}

inline rect3 
AddRadiusTo(rect3 A, vec3 Radius)
{
	rect3 Result;

	Result.Min = A.Min - Radius;
	Result.Max = A.Max + Radius;

	return(Result);
}

inline bool32
IsInRect(rect3 A, vec3 Point)
{
	bool32 Result = ((Point.x() >= A.Min.x()) &&
					 (Point.y() >= A.Min.y()) &&
					 (Point.z() >= A.Min.z()) &&
					 (Point.x() < A.Max.x()) &&
					 (Point.y() < A.Max.y()) &&
			         (Point.z() < A.Max.z()));

	return(Result);
}

inline bool32
RectIntersect(rect3 A, rect3 B)
{
	bool32 Result = !((B.Max.x() <= A.Min.x()) ||
					 (B.Max.y() <= A.Min.y()) ||
					 (B.Max.z() <= A.Min.z()) ||
					 (B.Min.x() >= A.Max.x()) ||
					 (B.Min.y() >= A.Max.y()) ||
					 (B.Min.z() >= A.Max.z()));

	return(Result);
}

inline rect3
RectFromBox(box *Box)
{
	rect3 Result;

	Result.Min = vec3(FLT_MAX, FLT_MAX, FLT_MAX);
	Result.Max = vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
	for(u32 PointIndex = 0;
		PointIndex < 8;
		PointIndex++)
	{
		Result.Min = Min(Result.Min, Box->Points[PointIndex]);
		Result.Max = Max(Result.Max, Box->Points[PointIndex]);
	}

	return(Result);
}

//
// NOTE(georgy): plane
// 

struct plane
{
	vec3 Normal;
	r32 D;
};

inline plane __vectorcall
PlaneFromTriangle(vec3 P1, vec3 P2, vec3 P3)
{
	plane Result;

	Result.Normal = Normalize(Cross(P2 - P1, P3 - P1));
	Result.D = Dot(P1, Result.Normal);

	return(Result);
}

inline r32 
SignedDistance(plane Plane, vec3 P)
{
	r32 Result = Dot(P, Plane.Normal) - Plane.D;

	return(Result);
}


inline bool32 __vectorcall
IsPointInTriangle(vec3 P, vec3 A, vec3 B, vec3 C)
{
	bool32 Result = ((Dot(Cross(B - A, P - A), Cross(B - A, C - A)) >= 0.0f) &&
					 (Dot(Cross(C - B, P - B), Cross(C - B, A - B)) >= 0.0f) &&
					 (Dot(Cross(A - C, P - C), Cross(A - C, B - C)) >= 0.0f));

	return(Result);
}