#pragma once

#include <xmmintrin.h>

#define PI 3.14159265358979323846f
#define I32_MIN (-2147483647 - 1)
#define I32_MAX 2147483647
#define U32_MAX 0xFFFFFFFF
#define FLT_MAX 3.402823466e+38F
#define DEG2RAD(Deg) ((Deg)/180.0f*PI)
#define RAD2DEG(Rad) ((Rad)/PI*180.0f)

#define SHUFFLE3(V, X, Y, Z) (vec3(_mm_shuffle_ps((V).m, (V).m, _MM_SHUFFLE(Z, Z, Y, X))))
#define SHUFFLE4(V, X, Y, Z, W) (vec4(_mm_shuffle_ps((V).m, (V).m, _MM_SHUFFLE(W, Z, Y, X))))

typedef float r32;
typedef double r64;

//
// NOTE(georgy): vec3
//

struct vec3
{
	__m128 m;

	inline vec3() { m = _mm_set1_ps(0.0f); } // TODO(georgy): Do I always want to zero vec3??
	inline explicit vec3(r32 *V) { m = _mm_set_ps(V[2], V[2], V[1], V[0]); }
	inline explicit vec3(r32 X, r32 Y, r32 Z) { m = _mm_set_ps(Z, Z, Y, X); }
	inline explicit vec3(__m128 V) { m = V; }

	inline r32 __vectorcall x() { return(_mm_cvtss_f32(m)); }
	inline r32 __vectorcall y() { return(_mm_cvtss_f32(_mm_shuffle_ps(m, m, _MM_SHUFFLE(1, 1, 1, 1)))); }
	inline r32 __vectorcall z() { return(_mm_cvtss_f32(_mm_shuffle_ps(m, m, _MM_SHUFFLE(2, 2, 2, 2)))); }
	inline r32 __vectorcall w() { return(_mm_cvtss_f32(_mm_shuffle_ps(m, m, _MM_SHUFFLE(3, 3, 3, 3)))); }

	inline vec3 __vectorcall yzx() { return(SHUFFLE3(*this, 1, 2, 0)); }
	inline vec3 __vectorcall zxy() { return(SHUFFLE3(*this, 2, 0, 1)); }

	void __vectorcall SetX(r32 X)
	{
		m = _mm_move_ss(m, _mm_set_ss(X));
	}

	void __vectorcall SetY(r32 Y)
	{
		__m128 Temp = _mm_move_ss(m, _mm_set_ss(Y));
		Temp = _mm_shuffle_ps(Temp, Temp, _MM_SHUFFLE(3, 2, 0, 0));
		m = _mm_move_ss(Temp, m);
	}

	void __vectorcall SetZ(r32 Z)
	{
		__m128 Temp = _mm_move_ss(m, _mm_set_ss(Z));
		Temp = _mm_shuffle_ps(Temp, Temp, _MM_SHUFFLE(3, 0, 1, 0));
		m = _mm_move_ss(Temp, m);
	}

	void __vectorcall SetW(r32 W)
	{
		__m128 Temp = _mm_move_ss(m, _mm_set_ss(W));
		Temp = _mm_shuffle_ps(Temp, Temp, _MM_SHUFFLE(0, 2, 1, 0));
		m = _mm_move_ss(Temp, m);
	}

	inline r32 operator[] (size_t I) { return(m.m128_f32[I]); };
};

inline vec3 __vectorcall 
vec3i(i32 X, i32 Y, i32 Z) 
{ 
	return(vec3((r32)X, (r32)Y, (r32)Z)); 
}

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
operator* (vec3 A, r32 B)
{
	A.m = _mm_mul_ps(A.m, _mm_set1_ps(B));
	return(A);
}

inline vec3 __vectorcall
operator* (r32 B, vec3 A)
{
	A = A * B;
	return(A);
}

inline vec3 __vectorcall
operator/ (vec3 A, r32 B)
{
	A = A * (1.0f / B);
	return(A);
}

inline vec3 __vectorcall
operator/ (r32 B, vec3 A)
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
operator*= (vec3 &A, r32 B)
{
	A = A * B;
	return(A);
}

inline vec3 __vectorcall
operator< (vec3 A, vec3 B)
{
	A.m = _mm_cmplt_ps(A.m, B.m);
	return(A);
}

inline vec3 __vectorcall
operator> (vec3 A, vec3 B)
{
	A.m = _mm_cmpgt_ps(A.m, B.m);
	return(A);
}

inline u32 __vectorcall
Mask(vec3 V)
{
	u32 Result = _mm_movemask_ps(V.m) & 7;
	return(Result);
}

inline bool32 __vectorcall
Any(vec3 V)
{
	bool32 Result = (Mask(V) != 0);
	return(Result);
}

inline bool32 __vectorcall
All(vec3 V)
{
	bool32 Result = (Mask(V) == 7);
	return(Result);
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

inline r32 __vectorcall
HMin(vec3 A)
{
	A = Min(A, SHUFFLE3(A, 1, 0, 2));
	r32 Result = Min(A, SHUFFLE3(A, 2, 0, 1)).x();
	return(Result);
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

inline r32 __vectorcall
Dot(vec3 A, vec3 B)
{
	vec3 Temp = Hadamard(A, B);
	r32 Result = Temp.x() + Temp.y() + Temp.z();
	return(Result);
}

inline r32 __vectorcall
LengthSq(vec3 A)
{
	return(Dot(A, A));
}

inline r32 __vectorcall
Length(vec3 A)
{
	return(SquareRoot(Dot(A, A)));
}

inline vec3 __vectorcall
Normalize(vec3 A)
{
	return(A * (1.0f / Length(A)));
}

inline vec3 __vectorcall
Lerp(vec3 A, vec3 B, r32 t)
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
	inline explicit vec4(r32 *V) { m = _mm_set_ps(V[3], V[2], V[1], V[0]); }
	inline explicit vec4(r32 X, r32 Y, r32 Z, r32 W) { m = _mm_set_ps(W, Z, Y, X); }
	inline explicit vec4(__m128 V) { m = V; }
	inline explicit vec4(vec3 V, r32 W) { m = V.m; SetW(W); }

	inline r32 __vectorcall x() { return(_mm_cvtss_f32(m)); }
	inline r32 __vectorcall y() { return(_mm_cvtss_f32(_mm_shuffle_ps(m, m, _MM_SHUFFLE(1, 1, 1, 1)))); }
	inline r32 __vectorcall z() { return(_mm_cvtss_f32(_mm_shuffle_ps(m, m, _MM_SHUFFLE(2, 2, 2, 2)))); }
	inline r32 __vectorcall w() { return(_mm_cvtss_f32(_mm_shuffle_ps(m, m, _MM_SHUFFLE(3, 3, 3, 3)))); }

	void __vectorcall SetX(r32 X)
	{
		m = _mm_move_ss(m, _mm_set_ss(X));
	}

	void __vectorcall SetY(r32 Y)
	{
		__m128 Temp = _mm_move_ss(m, _mm_set_ss(Y));
		Temp = _mm_shuffle_ps(Temp, Temp, _MM_SHUFFLE(3, 2, 0, 0));
		m = _mm_move_ss(Temp, m);
	}

	void __vectorcall SetZ(r32 Z)
	{
		__m128 Temp = _mm_move_ss(m, _mm_set_ss(Z));
		Temp = _mm_shuffle_ps(Temp, Temp, _MM_SHUFFLE(3, 0, 1, 0));
		m = _mm_move_ss(Temp, m);
	}

	void __vectorcall SetW(r32 W)
	{
		__m128 Temp = _mm_move_ss(m, _mm_set_ss(W));
		Temp = _mm_shuffle_ps(Temp, Temp, _MM_SHUFFLE(0, 2, 1, 0));
		m = _mm_move_ss(Temp, m);
	}

	inline r32 operator[] (size_t I) { return(m.m128_f32[I]); };
};

inline vec4 __vectorcall 
vec4i(i32 X, i32 Y, i32 Z, i32 W) 
{ 
	return(vec4((r32)X, (r32)Y, (r32)Z, (r32)W)); 
}

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
operator* (vec4 A, r32 B)
{
	A.m = _mm_mul_ps(A.m, _mm_set1_ps(B));
	return(A);
}

inline vec4 __vectorcall
operator* (r32 B, vec4 A)
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
operator*= (vec4 &A, r32 B)
{
	A = A * B;
	return(A);
}

inline vec4 __vectorcall
operator< (vec4 A, vec4 B)
{
	A.m = _mm_cmplt_ps(A.m, B.m);
	return(A);
}

inline vec4 __vectorcall
operator> (vec4 A, vec4 B)
{
	A.m = _mm_cmpgt_ps(A.m, B.m);
	return(A);
}

inline u32 __vectorcall
Mask(vec4 V)
{
	u32 Result = _mm_movemask_ps(V.m) & 15;
	return(Result);
}

inline bool32 __vectorcall
Any(vec4 V)
{
	bool32 Result = (Mask(V) != 0);
	return(Result);
}

inline bool32 __vectorcall
All(vec4 V)
{
	bool32 Result = (Mask(V) == 15);
	return(Result);
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

inline r32 __vectorcall
Dot(vec4 A, vec4 B)
{
	vec4 Temp = Hadamard(A, B);
	r32 Result = Temp.x() + Temp.y() + Temp.z();
	return(Result);
}

inline r32 __vectorcall
LengthSq(vec4 A)
{
	return(Dot(A, A));
}

inline r32 __vectorcall
Length(vec4 A)
{
	return(SquareRoot(Dot(A, A)));
}

inline vec4 __vectorcall
Normalize(vec4 A)
{
	return(A * (1.0f / Length(A)));
}

inline vec4 __vectorcall
Lerp(vec4 A, vec4 B, r32 t)
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

inline vec4 __vectorcall
operator* (mat4 A, vec4 V)
{
	vec4 Result = V.x() * A.FirstColumn + V.y() * A.SecondColumn + V.z() * A.ThirdColumn + V.w() * A.FourthColumn;

	return(Result);
}

inline mat4 __vectorcall
Identity(r32 Diagonal = 1.0f)
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
Scale(r32 ScaleFactor)
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
	r32 Cosine = Cos(Radians);
	r32 Sine = Sin(Radians);

	Axis = Normalize(Axis);
	r32 AxisX = Axis.x();
	r32 AxisY = Axis.y();
	r32 AxisZ = Axis.z();

	Result.FirstColumn = vec4(AxisX*AxisX*(1.0f - Cosine) + Cosine,
							  AxisX*AxisY*(1.0f - Cosine) + AxisZ*Sine,
							  AxisX*AxisZ*(1.0f - Cosine) - AxisY*Sine,
							  0.0f);
	Result.SecondColumn = vec4(AxisX*AxisY*(1.0f - Cosine) - AxisZ*Sine,
							   AxisY*AxisY*(1.0f - Cosine) + Cosine,
							   AxisY*AxisZ*(1.0f - Cosine) + AxisX*Sine,
							   0.0f);

	Result.ThirdColumn = vec4(AxisX*AxisZ*(1.0f - Cosine) + AxisY*Sine,
							  AxisY*AxisZ*(1.0f - Cosine) - AxisX*Sine,
							  AxisZ*AxisZ*(1.0f - Cosine) + Cosine,
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
Perspective(r32 FoV, r32 AspectRatio, r32 Near, r32 Far)
{
	r32 Scale = Tan(DEG2RAD(FoV)*0.5f) * Near;
	r32 Top = Scale;
	r32 Bottom = -Top;
	r32 Right = AspectRatio * Top;
	r32 Left = -Right;

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

internal mat4 __vectorcall
Ortho(r32 Bot, r32 Top, r32 Left, r32 Right, r32 Near, r32 Far)
{
	mat4 Result;

	Result.FirstColumn = vec4(2.0f / (Right - Left), 0.0f, 0.0f, 0.0f);
	Result.SecondColumn = vec4(0.0f, 2.0f / (Top - Bot), 0.0f, 0.0f);
	Result.ThirdColumn = vec4(0.0f, 0.0f, -2.0f / (Far - Near), 0.0f);
	Result.FourthColumn = vec4(-(Right + Left) / (Right - Left), 
							  -(Top + Bot) / (Top - Bot),
							  -(Far + Near) / (Far - Near), 1.0f);

	return(Result);
}

//
// NOTE(georgy): mat3
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
Identity3x3(r32 Diagonal = 1.0f)
{
	mat3 Result;

	Result.FirstColumn = vec3(Diagonal, 0.0f, 0.0f);
	Result.SecondColumn = vec3(0.0f, Diagonal, 0.0f);
	Result.ThirdColumn = vec3(0.0f, 0.0f, Diagonal);

	return(Result);
}

inline mat3 __vectorcall
Scale3x3(r32 ScaleFactor)
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
	r32 Cosine = Cos(Radians);
	r32 Sine = Sin(Radians);
	Axis = Normalize(Axis);
	r32 AxisX = Axis.x();
	r32 AxisY = Axis.y();
	r32 AxisZ = Axis.z();

	Result.FirstColumn = vec3(AxisX*AxisX*(1.0f - Cosine) + Cosine,
							  AxisX*AxisY*(1.0f - Cosine) + AxisZ*Sine,
							  AxisX*AxisZ*(1.0f - Cosine) - AxisY*Sine);
	Result.SecondColumn = vec3(AxisX*AxisY*(1.0f - Cosine) - AxisZ*Sine,
							  AxisY*AxisY*(1.0f - Cosine) + Cosine,
							  AxisY*AxisZ*(1.0f - Cosine) + AxisX*Sine);

	Result.ThirdColumn = vec3(AxisX*AxisZ*(1.0f - Cosine) + AxisY*Sine,
							  AxisY*AxisZ*(1.0f - Cosine) - AxisX*Sine,
							  AxisZ*AxisZ*(1.0f - Cosine) + Cosine);

	return(Result);
}

//
// NOTE(georgy): vec2
//

union vec2
{
	vec2() {}
	explicit vec2(r32 X, r32 Y) { x = X; y = Y; }

	struct
	{
		r32 x, y;
	};
	r32 E[2];
};

inline vec2
vec2i(i32 X, i32 Y) 
{ 
	vec2 Result = vec2((r32)X, (r32)Y);
	return(Result);
}

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
operator* (vec2 A, r32 B)
{
	A.x = A.x * B;
	A.y = A.y * B;
	return(A);
}

inline vec2
operator* (r32 B, vec2 A)
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
operator*= (vec2 &A, r32 B)
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

inline r32
Dot(vec2 A, vec2 B)
{
	r32 Result = A.x*B.x + A.y*B.y;
	return(Result);
}

inline r32
LengthSq(vec2 A)
{
	return (Dot(A, A));
}

inline r32
Length(vec2 A)
{
	return (SquareRoot(Dot(A, A)));
}

inline vec2
Normalize(vec2 A)
{
	return (A * (1.0f / Length(A)));
}

inline vec2
Lerp(vec2 A, vec2 B, r32 t)
{
	return (A + (B - A)*t);
}

// 
// NOTE(georgy): Quaternion 
// 

struct quaternion
{
	r32 w;
	vec3 v;
};

inline quaternion __vectorcall
Quaternion(r32 W, vec3 V)
{
	quaternion Result = { W, V };
	return(Result);
}

inline quaternion __vectorcall
QuaternionAngleAxis(r32 Angle, vec3 Axis)
{
	quaternion Result;

	r32 AngleInRadians = DEG2RAD(Angle);
	r32 W = Cos(0.5f*AngleInRadians);
	vec3 V = Sin(0.5f*AngleInRadians)*Axis;

	Result = Quaternion(W, V);

	return(Result);
}

inline quaternion __vectorcall
operator+ (quaternion A, quaternion B)
{
	quaternion Result;
	Result.w = A.w + B.w;
	Result.v = A.v + B.v;

	return(Result);
}

inline quaternion __vectorcall
operator- (quaternion A)
{
	quaternion Result;
	Result.w = -A.w;
	Result.v = -A.v;

	return(Result);
}

inline quaternion __vectorcall
operator* (quaternion A, r32 B)
{
	quaternion Result;

	Result.w = B*A.w;
	Result.v = B*A.v;

	return(Result);
}

inline quaternion __vectorcall
operator* (r32 B, quaternion A)
{
	quaternion Result = A * B;
	return(Result);
}

inline quaternion __vectorcall
operator* (quaternion A, quaternion B)
{
	quaternion Result;

	Result.w = A.w*B.w - Dot(A.v, B.v);
	Result.v = A.w*B.v + B.w*A.v + Cross(A.v, B.v);

	return(Result);
}

inline r32 __vectorcall
Length(quaternion A)
{
	r32 Result = SquareRoot(A.w*A.w + LengthSq(A.v));
	return(Result);
}

inline r32 __vectorcall
Dot(quaternion A, quaternion B)
{
	r32 Result = A.w*B.w + Dot(A.v, B.v);
	return(Result);
}

internal quaternion __vectorcall
Slerp(quaternion A, quaternion B, r32 t)
{
	r32 Omega = ArcCos(Dot(A, B));
	quaternion Result = ((Sin((1.0f - t)*Omega)/Sin(Omega))*A) +
						((Sin(t*Omega)/Sin(Omega))*B);

	return(Result);
}

internal mat4 __vectorcall
QuaternionToMatrix(quaternion A)
{
	mat4 Result;

	r32 W = A.w;
	r32 X = A.v.x();
	r32 Y = A.v.y();
	r32 Z = A.v.z();

	Result.FirstColumn = vec4(1.0f - 2.0f*Y*Y - 2.0f*Z*Z, 2.0f*X*Y + 2.0f*W*Z, 2.0f*X*Z - 2.0f*W*Y, 0.0f);
	Result.SecondColumn = vec4(2.0f*X*Y - 2.0f*W*Z, 1.0f - 2.0f*X*X - 2.0f*Z*Z, 2.0f*Y*Z + 2.0f*W*X, 0.0f);
	Result.ThirdColumn = vec4(2.0f*X*Z + 2.0f*W*Y, 2.0f*Y*Z - 2.0f*W*X, 1.0f - 2.0f*X*X - 2.0f*Y*Y, 0.0f);
	Result.FourthColumn = vec4(0.0f, 0.0f, 0.0f, 1.0f);

	return(Result);
}

internal quaternion __vectorcall
RotationMatrixToQuaternion(mat4 A)
{
	quaternion Result;

	r32 m11 = A.FirstColumn.x();
	r32 m22 = A.SecondColumn.y();
	r32 m33 = A.ThirdColumn.z();

	r32 FourWSquaredMinus1 = m11 + m22 + m33;
	r32 FourXSquaredMinus1 = m11 - m22 - m33;
	r32 FourYSquaredMinus1 = m22 - m11 - m33;
	r32 FourZSquaredMinus1 = m33 - m11 - m22;

	u32 BiggestIndex = 0;
	r32 FourBiggestSquaredMinus1 = FourWSquaredMinus1;
	if(FourXSquaredMinus1 > FourBiggestSquaredMinus1)
	{
		FourBiggestSquaredMinus1 = FourXSquaredMinus1;
		BiggestIndex = 1;
	}
	if(FourYSquaredMinus1 > FourBiggestSquaredMinus1)
	{
		FourBiggestSquaredMinus1 = FourYSquaredMinus1;
		BiggestIndex = 2;
	}
	if(FourZSquaredMinus1 > FourBiggestSquaredMinus1)
	{
		FourBiggestSquaredMinus1 = FourZSquaredMinus1;
		BiggestIndex = 3;
	}

	r32 BiggestValue = SquareRoot(FourBiggestSquaredMinus1 + 1.0f) * 0.5f;
	r32 Mult = 0.25f / BiggestValue;

	r32 W, X, Y, Z;
	switch (BiggestIndex)
	{
		case 0:
		{
			W = BiggestValue;
			X = (A.SecondColumn.z() - A.ThirdColumn.y()) * Mult;
			Y = (A.ThirdColumn.x() - A.FirstColumn.z()) * Mult;
			Z = (A.FirstColumn.y() - A.SecondColumn.x()) * Mult;
		} break;

		case 1:
		{
			W = (A.SecondColumn.z() - A.ThirdColumn.y()) * Mult;
			X = BiggestValue;
			Y = (A.FirstColumn.y() + A.SecondColumn.x()) * Mult;
			Z = (A.ThirdColumn.x() + A.FirstColumn.z()) * Mult;
		} break;

		case 2:
		{
			W = (A.ThirdColumn.x() - A.FirstColumn.z()) * Mult;
			X = (A.FirstColumn.y() + A.SecondColumn.x()) * Mult;
			Y = BiggestValue;
			Z = (A.SecondColumn.z() + A.ThirdColumn.y()) * Mult;
		} break;

		case 3:
		{
			W = (A.FirstColumn.y() - A.SecondColumn.x()) * Mult;
			X = (A.ThirdColumn.x() + A.FirstColumn.z()) * Mult;
			Y = (A.SecondColumn.z() + A.ThirdColumn.y()) * Mult;
			Z = BiggestValue;
		} break;
	}

	Result = Quaternion(W, vec3(X, Y, Z));

	return(Result);
}

inline quaternion __vectorcall
Conjugate(quaternion A)
{
	quaternion Result;

	Result.w = A.w;
	Result.v = -A.v;

	return(Result);
}

//
// NOTE(georgy): Scalar
//

inline r32
Lerp(r32 A, r32 B, r32 t)
{
	r32 Result = A + (B - A)*t;
	return(Result);
}

inline r32
QuinticInterpolation(r32 A, r32 B, r32 t)
{
	r32 s = t*t*t*(t*(6.0f*t - 15.0f) + 10.0f);
	r32 Result = A + (B - A)*s;
	return(Result);
}

inline r32
Clamp(r32 Value, r32 Min, r32 Max)
{
	if (Value < Min) Value = Min;
	if (Value > Max) Value = Max;

	return(Value);
}

inline r32 
Real32Modulo(r32 Numerator, r32 Denominator)
{
	r32 Coeff = (r32)(i32)(Numerator/Denominator);
	r32 Result = Numerator - (Coeff*Denominator);

	return(Result);
}

// 
// NOTE(georgy): box (rect3 but with 8 vertices)
// 

struct box
{
	vec3 Points[8];
};

internal box __vectorcall
ConstructBoxMinDim(vec3 Min, vec3 Dim)
{
	box Result;

	Result.Points[0] = Min;
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
ConstructBoxDim(vec3 Dim)
{
	box Result = ConstructBoxMinDim(-0.5f*Dim, Dim);

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
	// TODO(georgy): This is stupid, I dont want to copy box from ConstructBox. Can do this with pointers!
	*Box = ConstructBox(Box, Transformation, Translation);
}

//
// NOTE(georgy): rect2
//

struct rect2
{
	vec2 Min;
	vec2 Max;
};

inline rect2 __vectorcall
RectMinMax(vec2 Min, vec2 Max)
{
	rect2 Result;
	
	Result.Min = Min;
	Result.Max = Max;

	return(Result);
}

inline rect2 __vectorcall
RectCenterHalfDim(vec2 Center, vec2 HalfDim)
{
	rect2 Result;

	Result.Min = Center - HalfDim;
	Result.Max = Center + HalfDim;

	return(Result);
}

inline rect2 __vectorcall
RectCenterDim(vec2 Center, vec2 Dim)
{
	rect2 Result = RectCenterHalfDim(Center, 0.5f*Dim);

	return(Result);
}

inline rect2 __vectorcall
RectBottomFaceCenterDim(vec2 BottomFaceCenter, vec2 Dim)
{
	vec2 Center = BottomFaceCenter + 0.5f*vec2(0.0f, Dim.y);
	rect2 Result = RectCenterDim(Center, Dim);

	return(Result);		
}

inline vec2
GetDim(rect2 A)
{
	vec2 Result;

	Result.x = A.Max.x - A.Min.x;
	Result.y = A.Max.y - A.Min.y;

	return(Result);
}

inline rect2 
AddRadiusTo(rect2 A, vec2 Radius)
{
	rect2 Result;

	Result.Min = A.Min - Radius;
	Result.Max = A.Max + Radius;

	return(Result);
}

inline bool32
IsInRect(rect2 A, vec2 Point)
{
	bool32 Result = ((Point.x >= A.Min.x) &&
					 (Point.y >= A.Min.y) &&
					 (Point.x < A.Max.x) &&
					 (Point.y < A.Max.y));

	return(Result);
}

inline bool32
RectIntersect(rect2 A, rect2 B)
{
	bool32 Result = !((B.Max.x <= A.Min.x) ||
					  (B.Max.y <= A.Min.y) ||
					  (B.Min.x >= A.Max.x) ||
					  (B.Min.y >= A.Max.y));

	return(Result);
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

//
// NOTE(georgy): Perlin noise
//

// NOTE(georgy): Must be a power of 2!
global_variable vec2 Gradients2D[8] = 
{
	vec2(0.0f, 1.0f),
	vec2(-0.7071f, 0.7071f),
	vec2(-1.0f, 0.0f),
	vec2(-0.7071f, -0.7071f),
	vec2(0.0f, -1.0f),
	vec2(0.7071f, -0.7071f),
	vec2(1.0f, 0.0f),
	vec2(0.7071f, 0.7071f)
};

// NOTE(georgy): Must be a power of 2!
global_variable vec3 Gradients3D[16] = 
{
	vec3(0.7071f,0.7071f, 0.0f),
	vec3(-0.7071f, 0.7071f, 0.0f),
	vec3(0.7071f, -0.7071f, 0.0f),
	vec3(-0.7071f, -0.7071f, 0.0f),
	vec3(0.7071f, 0.0f, 0.7071f),
	vec3(-0.7071f, 0.0f, 0.7071f),
	vec3(0.7071f, 0.0f, -0.7071f),
	vec3(-0.7071f, 0.0f, -0.7071f),
	vec3(0.0f, 0.7071f, 0.7071f),
	vec3(0.0f, -0.7071f, 0.7071f),
	vec3(0.0f, 0.7071f, -0.7071f),
	vec3(0.0f, -0.7071f, -0.7071f),
	vec3(0.7071f, 0.7071f, 0.0f),
	vec3(-0.7071f, 0.7071f, 0.0f),
	vec3(0.0f, -0.7071f, 0.7071f),
	vec3(0.0f, -0.7071f, -0.7071f)
};

// NOTE(georgy): Must be a power of 2!
global_variable u32 PermutationTable[512] = 
{
	151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,
	8,99,37,240,21,10,23,190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,
	35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,71,
	134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,
	55,46,245,40,244,102,143,54,65,25,63,161,1,216,80,73,209,76,132,187,208, 89,
	18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,
	250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,
	189,28,42,223,183,170,213,119,248,152,2,44,154,163,70,221,153,101,155,167,43,
	172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,218,246,97,
	228,251,34,242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,
	107,49,192,214,31,181,199,106,157,184,84,204,176,115,121,50,45,127,4,150,254,
	138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180,

	151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,
	8,99,37,240,21,10,23,190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,
	35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,71,
	134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,
	55,46,245,40,244,102,143,54,65,25,63,161,1,216,80,73,209,76,132,187,208, 89,
	18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,
	250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,
	189,28,42,223,183,170,213,119,248,152,2,44,154,163,70,221,153,101,155,167,43,
	172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,218,246,97,
	228,251,34,242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,
	107,49,192,214,31,181,199,106,157,184,84,204,176,115,121,50,45,127,4,150,254,
	138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
};

// NOTE(georgy): These return ~[0, 1]
internal r32 
PerlinNoise2D(vec2 P, u32 KindOfSeed = 0)
{
	i32 I = FloorReal32ToInt32(P.x);
	i32 J = FloorReal32ToInt32(P.y);

	u32 PermutationForI = PermutationTable[(I + KindOfSeed) & (ArrayCount(PermutationTable) - 1)];
	u32 PermutationForI1 = PermutationTable[(I + 1 + KindOfSeed) & (ArrayCount(PermutationTable) - 1)];

	u32 Gradient00Index = (PermutationTable[(J + PermutationForI + KindOfSeed) & (ArrayCount(PermutationTable) - 1)]) &
						   (ArrayCount(Gradients2D) - 1);
	vec2 Gradient00 = Gradients2D[Gradient00Index];
	u32 Gradient10Index = (PermutationTable[(J + PermutationForI1 + KindOfSeed) & (ArrayCount(PermutationTable) - 1)]) &
						   (ArrayCount(Gradients2D) - 1);
	vec2 Gradient10 = Gradients2D[Gradient10Index];
	u32 Gradient01Index = (PermutationTable[(J + 1 + PermutationForI + KindOfSeed) & (ArrayCount(PermutationTable) - 1)]) &
						   (ArrayCount(Gradients2D) - 1);
	vec2 Gradient01 = Gradients2D[Gradient01Index];
	u32 Gradient11Index = (PermutationTable[(J + 1 + PermutationForI1 + KindOfSeed) & (ArrayCount(PermutationTable) - 1)]) &
						   (ArrayCount(Gradients2D) - 1);
	vec2 Gradient11 = Gradients2D[Gradient11Index];

	r32 U = P.x - I;
	r32 V = P.y - J;

	r32 GradientRamp00 = Dot(Gradient00, vec2(U, V));
	r32 GradientRamp10 = Dot(Gradient10, vec2(U - 1.0f, V));
	r32 GradientRamp01 = Dot(Gradient01, vec2(U, V - 1.0f));
	r32 GradientRamp11 = Dot(Gradient11, vec2(U - 1.0f, V - 1.0f));

	r32 QuinticFactorForU = U*U*U*(U*(6.0f*U - 15.0f) + 10.0f);
	r32 InterpolatedX0 = Lerp(GradientRamp00, GradientRamp10, QuinticFactorForU);
	r32 InterpolatedX1 = Lerp(GradientRamp01, GradientRamp11, QuinticFactorForU);
	r32 Result = QuinticInterpolation(InterpolatedX0, InterpolatedX1, V);

	Result = (Result + 0.7f) * 0.71428571428571428571428571428571f;

	return(Result);
}

internal r32 
PerlinNoise3D(vec3 P)
{
	i32 I = FloorReal32ToInt32(P.x());
	i32 J = FloorReal32ToInt32(P.y());
	i32 K = FloorReal32ToInt32(P.z());

	u32 PermutationForI = PermutationTable[I & (ArrayCount(PermutationTable) - 1)];
	u32 PermutationForI1 = PermutationTable[(I + 1) & (ArrayCount(PermutationTable) - 1)];
	u32 PermutationForJI = PermutationTable[(J + PermutationForI) & (ArrayCount(PermutationTable) - 1)];
	u32 PermutationForJ1I = PermutationTable[((J + 1) + PermutationForI) & (ArrayCount(PermutationTable) - 1)];
	u32 PermutationForJI1 = PermutationTable[(J + PermutationForI1) & (ArrayCount(PermutationTable) - 1)];
	u32 PermulationForJ1I1 = PermutationTable[((J + 1) + PermutationForI1) & (ArrayCount(PermutationTable) - 1)];

	u32 Gradient000Index = PermutationTable[(K + PermutationForJI) & (ArrayCount(PermutationTable) - 1)] &
						   (ArrayCount(Gradients3D) - 1);
	vec3 Gradient000 = Gradients3D[Gradient000Index];

	u32 Gradient100Index = PermutationTable[(K + PermutationForJI1) & (ArrayCount(PermutationTable) - 1)] &
						   (ArrayCount(Gradients3D) - 1);
	vec3 Gradient100 = Gradients3D[Gradient100Index];

	u32 Gradient010Index = PermutationTable[(K + PermutationForJ1I) & (ArrayCount(PermutationTable) - 1)] &
						   (ArrayCount(Gradients3D) - 1);
	vec3 Gradient010 = Gradients3D[Gradient010Index];

	u32 Gradient110Index = PermutationTable[(K + PermulationForJ1I1) & (ArrayCount(PermutationTable) - 1)] &
						   (ArrayCount(Gradients3D) - 1);
	vec3 Gradient110 = Gradients3D[Gradient110Index];

	u32 Gradient001Index = PermutationTable[((K + 1) + PermutationForJI) & (ArrayCount(PermutationTable) - 1)] &
						   (ArrayCount(Gradients3D) - 1);
	vec3 Gradient001 = Gradients3D[Gradient001Index];

	u32 Gradient101Index = PermutationTable[((K + 1) + PermutationForJI1) & (ArrayCount(PermutationTable) - 1)] &
						   (ArrayCount(Gradients3D) - 1);
	vec3 Gradient101 = Gradients3D[Gradient101Index];

	u32 Gradient011Index = PermutationTable[((K + 1) + PermutationForJ1I) & (ArrayCount(PermutationTable) - 1)] &
						   (ArrayCount(Gradients3D) - 1);
	vec3 Gradient011 = Gradients3D[Gradient011Index];

	u32 Gradient111Index = PermutationTable[((K + 1) + PermulationForJ1I1) & (ArrayCount(PermutationTable) - 1)] &
						   (ArrayCount(Gradients3D) - 1);
	vec3 Gradient111 = Gradients3D[Gradient111Index];

	r32 U = P.x() - I;
	r32 V = P.y() - J;
	r32 T = P.z() - K;

	r32 GradientRamp000 = Dot(Gradient000, vec3(U, V, T));
	r32 GradientRamp100 = Dot(Gradient100, vec3(U - 1.0f, V, T));
	r32 GradientRamp010 = Dot(Gradient010, vec3(U, V - 1.0f, T));
	r32 GradientRamp110 = Dot(Gradient110, vec3(U - 1.0f, V - 1.0f, T));
	r32 GradientRamp001 = Dot(Gradient001, vec3(U, V, 1.0f - T));
	r32 GradientRamp101 = Dot(Gradient101, vec3(U - 1.0f, V, 1.0f - T));
	r32 GradientRamp011 = Dot(Gradient011, vec3(U, V - 1.0f, 1.0f - T));
	r32 GradientRamp111 = Dot(Gradient111, vec3(U - 1.0f, V - 1.0f, 1.0f - T));

	r32 QuinticFactorForU = U*U*U*(U*(6.0f*U - 15.0f) + 10.0f);
	r32 QuinticFactorForV = V*V*V*(V*(6.0f*V - 15.0f) + 10.0f);
	r32 InterpolatedX00 = Lerp(GradientRamp000, GradientRamp100, QuinticFactorForU);
	r32 InterpolatedX10 = Lerp(GradientRamp010, GradientRamp110, QuinticFactorForU);
	r32 InterpolatedY0 = Lerp(InterpolatedX00, InterpolatedX10, QuinticFactorForV);

	r32 InterpolatedX01 = Lerp(GradientRamp001, GradientRamp101, QuinticFactorForU);
	r32 InterpolatedX11 = Lerp(GradientRamp011, GradientRamp111, QuinticFactorForU);
	r32 InterpolatedY1 = Lerp(InterpolatedX01, InterpolatedX11, QuinticFactorForV);

	r32 Result = QuinticInterpolation(InterpolatedY0, InterpolatedY1, T);

	Result = (Result + 0.7f) * 0.71428571428571428571428571428571f;

	return(Result);
}