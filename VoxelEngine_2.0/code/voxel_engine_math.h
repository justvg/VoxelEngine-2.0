#pragma once

#define PI 3.14159265358979323846f

#define I32_MIN (-2147483647 - 1)
#define I32_MAX 2147483647
#define U32_MAX 0xFFFFFFFF
#define FLT_MAX 3.402823466e+38F
#define DEG2RAD(Deg) ((Deg)/180.0f*PI)
#define RAD2DEG(Rad) ((Rad)/PI*180.0f)

union vec2
{
    struct
    {
        r32 x, y;
    };

    struct
    {
        r32 u, v;
    };

    r32 E[2];

    vec2() { x = y = 0.0f; }
    vec2(r32 X, r32 Y) { x = X; y = Y; }
};

union vec3
{
    struct
    {
        r32 x, y, z;
    };

    struct
    {
        r32 u, v, w;
    };

    struct
    {
        r32 r, g, b;
    };

    struct 
    {
        vec2 xy;
        r32 Ignored_0;
    };
    struct 
    {
        r32 Ignored_1;
        vec2 yz;
    };
    struct 
    {
        vec2 uv;
        r32 Ignored_2;
    };
    struct 
    {
        r32 Ignored_3;
        vec2 vw;
    };

    r32 E[3];

    vec3() { x = y = z = 0.0f; }
    vec3(r32 X, r32 Y, r32 Z) { x = X; y = Y; z = Z; }
    vec3(vec2 XY, r32 Z) { x = XY.x; y = XY.y; z = Z; }
};

union vec4
{
    struct
    {
        r32 x, y, z, w;
    };

    struct
    {
        r32 r, g, b, a;
    };

    struct 
    {
        vec3 xyz;
        r32 Ignored_0;
    };
    struct
    {
        vec2 xy;
        r32 Ignored_1;
        r32 Ignored_2;
    };
    struct
    {
        r32 Ignored_3;
        vec2 yz;
        r32 Ignored_4;
    };
    struct
    {
        r32 Ignored_5;
        r32 Ignored_6;
        vec2 zw;
    };

    struct
    {
        vec3 rgb;
        r32 Ignored_7;
    };

    r32 E[4];

    vec4() { x = y = z = w = 0.0f; }
    vec4(vec3 V, r32 W) { xyz = V; w = W; }
    vec4(r32 X, r32 Y, r32 Z, r32 W) { x = X; y = Y; z = Z; w = W; }
};

inline vec2
vec2i(i32 X, i32 Y)
{
    vec2 Result = vec2((r32)X, (r32)Y);

    return(Result);
}

inline vec2
vec2i(u32 X, u32 Y)
{
    vec2 Result = vec2((r32)X, (r32)Y);

    return(Result);
}

inline vec3
vec3i(i32 X, i32 Y, i32 Z)
{
    vec3 Result = vec3((r32)X, (r32)Y, (r32)Z);

    return(Result);
}

inline vec3
vec3i(u32 X, u32 Y, u32 Z)
{
    vec3 Result = vec3((r32)X, (r32)Y, (r32)Z);

    return(Result);
}

inline vec4
vec4i(i32 X, i32 Y, i32 Z, i32 W)
{
    vec4 Result = vec4((r32)X, (r32)Y, (r32)Z, (r32)W);

    return(Result);
}

inline vec4
vec4i(u32 X, u32 Y, u32 Z, u32 W)
{
    vec4 Result = vec4((r32)X, (r32)Y, (r32)Z, (r32)W);

    return(Result);
}

union mat3
{
    r32 E[9];
    struct 
    {
        r32 a11, a21, a31;
        r32 a12, a22, a32;
        r32 a13, a23, a33;
    };
};

union mat4
{
    r32 E[16];

    struct 
    {
        r32 a11, a21, a31, a41;
        r32 a12, a22, a32, a42;
        r32 a13, a23, a33, a43;
        r32 a14, a24, a34, a44;
    };
};

inline r32
Radians(r32 Angle)
{
    r32 Result = (Angle / 180.0f) * PI;;

    return(Result);
}

inline r32
Degrees(r32 Rad)
{
    r32 Result = (Rad / PI) * 180.0f;

    return(Result);
}

inline r32
Lerp(r32 A, r32 B, r32 t)
{
    r32 Result = A + (B - A)*t;

    return(Result);
}

inline r32 
Clamp(r32 Value, r32 MinClamp, r32 MaxClamp)
{
    if(Value < MinClamp) Value = MinClamp;
    else if(Value > MaxClamp) Value = MaxClamp;

    return(Value);
}

inline r32
Min(r32 A, r32 B)
{
	if(A < B) return(A);
	else return(B);
}

inline r32
Max(r32 A, r32 B)
{
	if(A > B) return(A);
	else return(B);
}

inline r32 
Real32Modulo(r32 Numerator, r32 Denominator)
{
	r32 Coeff = (r32)(i32)(Numerator/Denominator);
	r32 Result = Numerator - (Coeff*Denominator);

	return(Result);
}

inline r32
QuinticInterpolation(r32 A, r32 B, r32 t)
{
	r32 s = t*t*t*(t*(6.0f*t - 15.0f) + 10.0f);
	r32 Result = A + (B - A)*s;
	return(Result);
}

// 
// NOTE(georgy): vec2
// 

inline vec2
operator+ (vec2 A, vec2 B)
{
    vec2 Result;

    Result.x = A.x + B.x;
    Result.y = A.y + B.y;

    return(Result);
}

inline vec2
operator- (vec2 A, vec2 B)
{
    vec2 Result;

    Result.x = A.x - B.x;
    Result.y = A.y - B.y;

    return(Result);
}

inline vec2
Hadamard(vec2 A, vec2 B)
{
    vec2 Result;

    Result.x = A.x * B.x;
    Result.y = A.y * B.y;

    return(Result);
}

inline vec2
operator* (vec2 A, r32 B)
{
    vec2 Result;

    Result.x = A.x * B;
    Result.y = A.y * B;

    return(Result);
}

inline vec2
operator* (r32 B, vec2 A)
{
    vec2 Result = A * B;

    return(Result);
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
    vec2 Result;

    Result.x = -A.x;
    Result.y = -A.y;
    
    return(Result);
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
    r32 Result = Dot(A, A);

    return(Result);
}

inline r32
Length(vec2 A)
{
    r32 Result = SquareRoot(LengthSq(A));

    return(Result);
}

inline vec2
Normalize(vec2 A)
{
    vec2 Result = A * (1.0f / Length(A));

    return(Result);
}

inline vec2
Perp(vec2 A)
{
    vec2 Result = vec2(-A.y, A.x);
    
    return(Result);
}

inline r32
Cross2D(vec2 A, vec2 B)
{
    r32 Result = Dot(Perp(A), B);

    return(Result);
}

inline vec2
Lerp(vec2 A, vec2 B, r32 t)
{
    vec2 Result = A + (B - A)*t;

    return(Result);
}

// 
// NOTE(georgy): vec3
// 

inline vec3
operator+ (vec3 A, vec3 B)
{
    vec3 Result;

    Result.x = A.x + B.x;
    Result.y = A.y + B.y;
    Result.z = A.z + B.z;

    return(Result);
}

inline vec3
operator- (vec3 A, vec3 B)
{
    vec3 Result;

    Result.x = A.x - B.x;
    Result.y = A.y - B.y;
    Result.z = A.z - B.z;

    return(Result);
}

inline vec3
Hadamard(vec3 A, vec3 B)
{
    vec3 Result;

    Result.x = A.x * B.x;
    Result.y = A.y * B.y;
    Result.z = A.z * B.z;

    return(Result);
}

inline vec3
operator* (vec3 A, r32 B)
{
    vec3 Result;

    Result.x = A.x * B;
    Result.y = A.y * B;
    Result.z = A.z * B;

    return(Result);
}

inline vec3
operator* (r32 B, vec3 A)
{
    vec3 Result = A * B;

    return(Result);
}

inline vec3 &
operator+= (vec3 &A, vec3 B)
{
    A = A + B;
    
    return(A);
}

inline vec3 &
operator-= (vec3 &A, vec3 B)
{
    A = A - B;
    
    return(A);
}

inline vec3 &
operator*= (vec3 &A, r32 B)
{
    A = A * B;
    
    return(A);
}

inline vec3
operator- (vec3 A)
{
    vec3 Result;

    Result.x = -A.x;
    Result.y = -A.y;
    Result.z = -A.z;
    
    return(Result);
}

inline r32
Dot(vec3 A, vec3 B)
{
    r32 Result = A.x*B.x + A.y*B.y + A.z*B.z;

    return(Result);
}

inline r32
LengthSq(vec3 A)
{
    r32 Result = Dot(A, A);

    return(Result);
}

inline r32
Length(vec3 A)
{
    r32 Result = SquareRoot(LengthSq(A));

    return(Result);
}

inline vec3
Normalize(vec3 A)
{
    vec3 Result = A * (1.0f / Length(A));

    return(Result);
}

inline vec3
Cross(vec3 A, vec3 B)
{
    vec3 Result;

    Result.x = A.y*B.z - B.y*A.z;
    Result.y = A.z*B.x - B.z*A.x;
    Result.z = A.x*B.y - B.x*A.y;

    return(Result);    
}

inline vec3
Lerp(vec3 A, vec3 B, r32 t)
{
    vec3 Result = A + (B - A)*t;

    return(Result);
}

internal bool32
PointInTriangle(vec3 P, vec3 A, vec3 B, vec3 C)
{
    A -= P; B -= P; C -= P;
    vec3 U = Cross(B, C);
    vec3 V = Cross(C, A);
    vec3 W = Cross(A, B);
    bool32 Result = (Dot(U, V) >= 0.0f) && (Dot(U, W) >= 0.0f);

    return(Result);
}

inline vec3 
Clamp(vec3 A, vec3 MinClamp, vec3 MaxClamp)
{
	vec3 Result;

    Result.x = Clamp(A.x, MinClamp.x, MaxClamp.x);
    Result.y = Clamp(A.y, MinClamp.y, MaxClamp.y);
    Result.z = Clamp(A.z, MinClamp.z, MaxClamp.z);

    return(Result);
}

internal vec3
Min(vec3 A, vec3 B)
{
	vec3 Result;
	Result.x = Min(A.x, B.x);
	Result.y = Min(A.y, B.y);
	Result.z = Min(A.z, B.z);

	return(Result);
}

internal vec3
Max(vec3 A, vec3 B)
{
	vec3 Result;
	Result.x = Max(A.x, B.x);
	Result.y = Max(A.y, B.y);
	Result.z = Max(A.z, B.z);

	return(Result);
}

// 
// NOTE(georgy): vec4
// 

inline vec4
operator+ (vec4 A, vec4 B)
{
    vec4 Result;

    Result.x = A.x + B.x;
    Result.y = A.y + B.y;
    Result.z = A.z + B.z;
    Result.w = A.w + B.w;

    return(Result);
}

inline vec4
operator- (vec4 A, vec4 B)
{
    vec4 Result;

    Result.x = A.x - B.x;
    Result.y = A.y - B.y;
    Result.z = A.z - B.z;
    Result.w = A.w - B.w;

    return(Result);
}

inline vec4
Hadamard(vec4 A, vec4 B)
{
    vec4 Result;

    Result.x = A.x * B.x;
    Result.y = A.y * B.y;
    Result.z = A.z * B.z;
    Result.w = A.w * B.w;

    return(Result);
}

inline vec4
operator* (vec4 A, r32 B)
{
    vec4 Result;

    Result.x = A.x * B;
    Result.y = A.y * B;
    Result.z = A.z * B;
    Result.w = A.w * B;

    return(Result);
}

inline vec4
operator* (r32 B, vec4 A)
{
    vec4 Result = A * B;

    return(Result);
}

inline vec4
operator* (mat4 M, vec4 V)
{
	vec4 Result;

	Result.x = M.a11*V.x + M.a12*V.y + M.a13*V.z + M.a14*V.w;
	Result.y = M.a21*V.x + M.a22*V.y + M.a23*V.z + M.a24*V.w;
	Result.z = M.a31*V.x + M.a32*V.y + M.a33*V.z + M.a34*V.w;
	Result.w = M.a41*V.x + M.a42*V.y + M.a43*V.z + M.a44*V.w;

	return(Result);
}

inline vec4 &
operator+= (vec4 &A, vec4 B)
{
    A = A + B;
    
    return(A);
}

inline vec4 &
operator-= (vec4 &A, vec4 B)
{
    A = A - B;
    
    return(A);
}

inline vec4 &
operator*= (vec4 &A, r32 B)
{
    A = A * B;
    
    return(A);
}

inline vec4
operator- (vec4 A)
{
    vec4 Result;

    Result.x = -A.x;
    Result.y = -A.y;
    Result.z = -A.z;
    Result.w = -A.w;
    
    return(Result);
}

inline r32
Dot(vec4 A, vec4 B)
{
    r32 Result = A.x*B.x + A.y*B.y + A.z*B.z + A.w*B.w;

    return(Result);
}

inline r32
LengthSq(vec4 A)
{
    r32 Result = Dot(A, A);

    return(Result);
}

inline r32
Length(vec4 A)
{
    r32 Result = SquareRoot(LengthSq(A));

    return(Result);
}

inline vec4
Normalize(vec4 A)
{
    vec4 Result = A * (1.0f / Length(A));

    return(Result);
}

inline vec4
Lerp(vec4 A, vec4 B, r32 t)
{
    vec4 Result = A + (B - A)*t;

    return(Result);
}

inline vec4 
Clamp(vec4 A, vec4 MinClamp, vec4 MaxClamp)
{
	vec4 Result;

    Result.x = Clamp(A.x, MinClamp.x, MaxClamp.x);
    Result.y = Clamp(A.y, MinClamp.y, MaxClamp.y);
    Result.z = Clamp(A.z, MinClamp.z, MaxClamp.z);
    Result.w = Clamp(A.w, MinClamp.w, MaxClamp.w);

    return(Result);
}

internal vec4
Min(vec4 A, vec4 B)
{
	vec4 Result;
	Result.x = Min(A.x, B.x);
	Result.y = Min(A.y, B.y);
	Result.z = Min(A.z, B.z);
	Result.w = Min(A.w, B.w);

	return(Result);
}

internal vec4
Max(vec4 A, vec4 B)
{
	vec4 Result;
	Result.x = Max(A.x, B.x);
	Result.y = Max(A.y, B.y);
	Result.z = Max(A.z, B.z);
	Result.w = Max(A.w, B.w);

	return(Result);
}

// 
// NOTE(georgy): mat3
// 

inline mat3
Identity3x3(r32 Diagonal = 1.0f)
{
    mat3 Result = {};

    Result.a11 = Diagonal;
    Result.a22 = Diagonal;
    Result.a33 = Diagonal;

    return(Result);
}

inline mat3
Scale3x3(r32 Scaling)
{
    mat3 Result = {};

    Result.a11 = Scaling;
    Result.a22 = Scaling;
    Result.a33 = Scaling;

    return(Result);
}

inline mat3
Scale3x3(vec3 Scaling)
{
    mat3 Result = {};

    Result.a11 = Scaling.x;
    Result.a22 = Scaling.y;
    Result.a33 = Scaling.z;

    return(Result);
}

internal mat3
Rotate3x3(r32 Angle, vec3 Axis)
{
    mat3 Result;

    r32 Rad = Radians(Angle);
    r32 Sine = Sin(Rad);
    r32 Cosine = Cos(Rad);

	Result.a11 = Axis.x*Axis.x*(1.0f - Cosine) + Cosine;
	Result.a21 = Axis.x*Axis.y*(1.0f - Cosine) + Axis.z*Sine;
	Result.a31 = Axis.x*Axis.z*(1.0f - Cosine) - Axis.y*Sine;

	Result.a12 = Axis.x*Axis.y*(1.0f - Cosine) - Axis.z*Sine;
	Result.a22 = Axis.y*Axis.y*(1.0f - Cosine) + Cosine;
	Result.a32 = Axis.y*Axis.z*(1.0f - Cosine) + Axis.x*Sine;

	Result.a13 = Axis.x*Axis.z*(1.0f - Cosine) + Axis.y*Sine;
	Result.a23 = Axis.y*Axis.z*(1.0f - Cosine) - Axis.x*Sine;
	Result.a33 = Axis.z*Axis.z*(1.0f - Cosine) + Cosine;

    return(Result);
}

internal mat3
Transpose3x3(const mat3 &M)
{
    mat3 Result;

    Result.a11 = M.a11;
	Result.a21 = M.a12;
	Result.a31 = M.a13;

	Result.a12 = M.a21;
	Result.a22 = M.a22;
	Result.a32 = M.a23;

	Result.a13 = M.a31;
	Result.a23 = M.a32;
	Result.a33 = M.a33;

    return(Result);
}

internal mat3
Inverse3x3(const mat3 &M)
{
    mat3 InverseMatrix = {};
    
    r32 Determinant = M.a11*M.a22*M.a33 + M.a12*M.a23*M.a31 + M.a13*M.a21*M.a32 - 
                      (M.a31*M.a22*M.a13 + M.a32*M.a23*M.a11 + M.a33*M.a21*M.a12);
    if(Determinant > 0.00001f)
    {
        r32 OneOverDeterminant = 1.0f / Determinant;

        InverseMatrix.a11 = (M.a22*M.a33 - M.a32*M.a23)*OneOverDeterminant;
        InverseMatrix.a12 = (-(M.a21*M.a33 - M.a31*M.a23))*OneOverDeterminant;
        InverseMatrix.a13 = (M.a21*M.a32 - M.a31*M.a22)*OneOverDeterminant;
        InverseMatrix.a21 = (-(M.a12*M.a33 - M.a32*M.a13))*OneOverDeterminant;
        InverseMatrix.a22 = (M.a11*M.a33 - M.a31*M.a13)*OneOverDeterminant;
        InverseMatrix.a23 = (-(M.a11*M.a32 - M.a31*M.a12))*OneOverDeterminant;
        InverseMatrix.a31 = (M.a12*M.a23 - M.a22*M.a13)*OneOverDeterminant;
        InverseMatrix.a32 = (-(M.a11*M.a23 - M.a21*M.a13))*OneOverDeterminant;
        InverseMatrix.a33 = (M.a11*M.a22 - M.a21*M.a12)*OneOverDeterminant;

        InverseMatrix = Transpose3x3(InverseMatrix);
    }

    return(InverseMatrix);
}

internal mat3
operator*(mat3 A, mat3 B)
{
    mat3 Result;

    for(u32 Row = 0;
        Row < 3;
        Row++)
    {
        for(u32 Column = 0;
            Column < 3;
            Column++)
        {
            r32 Sum = 0.0f;
            for(u32 E = 0;
                E < 3;
                E++)
            {
                Sum += A.E[Row + E*3] * B.E[Column*3 + E];
            }
            Result.E[Row + Column*3] = Sum;
        }
    }

    return(Result);
}

internal vec3
operator*(mat3 A, vec3 B)
{
    vec3 Result;

    Result.x = A.a11*B.x + A.a12*B.y + A.a13*B.z;
    Result.y = A.a21*B.x + A.a22*B.y + A.a23*B.z;
    Result.z = A.a31*B.x + A.a32*B.y + A.a33*B.z;

    return(Result);
}

// 
// NOTE(georgy): mat4
// 

inline mat4
Identity(r32 Diagonal = 1.0f)
{
    mat4 Result = {};

    Result.a11 = Diagonal;
    Result.a22 = Diagonal;
    Result.a33 = Diagonal;
    Result.a44 = Diagonal;

    return(Result);
}

inline mat4
Translate(vec3 Translation)
{
    mat4 Result = Identity(1.0f);

    Result.a14 = Translation.x;
    Result.a24 = Translation.y;
    Result.a34 = Translation.z;

    return(Result);
}

inline mat4
Scale(r32 Scaling)
{
    mat4 Result = {};

    Result.a11 = Scaling;
    Result.a22 = Scaling;
    Result.a33 = Scaling;
    Result.a44 = 1.0f;

    return(Result);
}

inline mat4
Scale(vec3 Scaling)
{
    mat4 Result = {};

    Result.a11 = Scaling.x;
    Result.a22 = Scaling.y;
    Result.a33 = Scaling.z;
    Result.a44 = 1.0f;

    return(Result);
}

internal mat4
Rotate(r32 Angle, vec3 Axis)
{
    mat4 Result;

    r32 Rad = Radians(Angle);
    r32 Sine = Sin(Rad);
    r32 Cosine = Cos(Rad);

    Axis = Normalize(Axis);

    Result.a11 = Axis.x*Axis.x*(1.0f - Cosine) + Cosine;
	Result.a21 = Axis.x*Axis.y*(1.0f - Cosine) + Axis.z*Sine;
	Result.a31 = Axis.x*Axis.z*(1.0f - Cosine) - Axis.y*Sine;
	Result.a41 = 0.0f;

    Result.a12 = Axis.x*Axis.y*(1.0f - Cosine) - Axis.z*Sine;
	Result.a22 = Axis.y*Axis.y*(1.0f - Cosine) + Cosine;
	Result.a32 = Axis.y*Axis.z*(1.0f - Cosine) + Axis.x*Sine;
	Result.a42 = 0.0f;

    Result.a13 = Axis.x*Axis.z*(1.0f - Cosine) + Axis.y*Sine;
	Result.a23 = Axis.y*Axis.z*(1.0f - Cosine) - Axis.x*Sine;
	Result.a33 = Axis.z*Axis.z*(1.0f - Cosine) + Cosine;
	Result.a43 = 0.0f;

	Result.a14 = 0.0f;
	Result.a24 = 0.0f;
	Result.a34 = 0.0f;
	Result.a44 = 1.0f;

    return(Result);
}

internal mat4
Mat4(const mat3 &M)
{
    mat4 Result;

    Result.a11 = M.a11;
    Result.a21 = M.a21;
    Result.a31 = M.a31;
    Result.a12 = M.a12;
    Result.a22 = M.a22;
    Result.a32 = M.a32;
    Result.a13 = M.a13;
    Result.a23 = M.a23;
    Result.a33 = M.a33;
    Result.a41 = Result.a42 = Result.a43 = Result.a14 = Result.a24 = Result.a34 = 0.0f;
    Result.a44 = 1.0f;

    return(Result);
}

internal mat4
LookAt(vec3 From, vec3 Target, vec3 UpAxis = vec3(0.0f, 1.0f, 0.0f))
{
    vec3 Forward = Normalize(From - Target);
    vec3 Right = Normalize(Cross(UpAxis, Forward));
    vec3 Up = Cross(Forward, Right);

    mat4 Result;

    Result.a11 = Right.x;
    Result.a21 = Up.x;
    Result.a31 = Forward.x;
    Result.a41 = 0.0f;

    Result.a12 = Right.y;
    Result.a22 = Up.y;
    Result.a32 = Forward.y;
    Result.a42 = 0.0f;

    Result.a13 = Right.z;
    Result.a23 = Up.z;
    Result.a33 = Forward.z;
    Result.a43 = 0.0f;

    Result.a14 = -Dot(Right, From);
    Result.a24 = -Dot(Up, From);
    Result.a34 = -Dot(Forward, From);
    Result.a44 = 1.0f;

    return(Result);
}

internal mat4
ViewRotationMatrixFromDirection(vec3 Dir, vec3 UpAxis = vec3(0.0f, 1.0f, 0.0f))
{
    vec3 Forward = Normalize(Dir);
    vec3 Right = Normalize(Cross(UpAxis, Forward));
    vec3 Up = Cross(Forward, Right);

    mat4 Result;

    Result.a11 = Right.x;
    Result.a21 = Up.x;
    Result.a31 = Forward.x;
    Result.a41 = 0.0f;

    Result.a12 = Right.y;
    Result.a22 = Up.y;
    Result.a32 = Forward.y;
    Result.a42 = 0.0f;

    Result.a13 = Right.z;
    Result.a23 = Up.z;
    Result.a33 = Forward.z;
    Result.a43 = 0.0f;

    Result.a14 = 0.0f;
    Result.a24 = 0.0f;
    Result.a34 = 0.0f;
    Result.a44 = 1.0f;

    return(Result);
}

internal mat4 
Ortho(r32 Bottom, r32 Top, r32 Left, r32 Right, r32 Near, r32 Far)
{
    mat4 Result = {};

    Result.a11 = 2.0f / (Right - Left);
    Result.a22 = 2.0f / (Top - Bottom);
    Result.a33 = -2.0f / (Far - Near);
    Result.a14 = -(Right + Left) / (Right - Left);
    Result.a24 = -(Top + Bottom) / (Top - Bottom);
	Result.a34 = -(Far + Near) / (Far - Near);
	Result.a44 = 1.0f;

    return(Result);
}

internal mat4
Perspective(r32 FoV, r32 AspectRatio, r32 Near, r32 Far)
{
    r32 Scale = Tan(Radians(FoV) * 0.5f) * Near;
    r32 Top = Scale;
    r32 Bottom = -Top;
    r32 Right = Scale * AspectRatio;
    r32 Left = -Right;

    mat4 Result = {};

    Result.a11 = 2.0f * Near / (Right - Left);
	Result.a22 = 2.0f * Near / (Top - Bottom);
	Result.a13 = (Right + Left) / (Right - Left);
	Result.a23 = (Top + Bottom) / (Top - Bottom);
	Result.a33 = -(Far + Near) / (Far - Near);
	Result.a43 = -1.0f;
	Result.a34 = -(2.0f * Far*Near) / (Far - Near);

    return(Result);
}

internal mat4
operator*(mat4 A, mat4 B)
{
    mat4 Result;

    for(u32 Row = 0;
        Row < 4;
        Row++)
    {
        for(u32 Column = 0;
            Column < 4;
            Column++)
        {
            r32 Sum = 0.0f;
            for(u32 E = 0;
                E < 4;
                E++)
            {
                Sum += A.E[Row + E*4] * B.E[Column*4 + E];
            }
            Result.E[Row + Column*4] = Sum;
        }
    }

    return(Result);
}

// 
// NOTE(georgy): Quaternion 
// 

struct quaternion
{
	r32 w;
	vec3 v;
};

inline quaternion 
Quaternion(r32 W, vec3 V)
{
	quaternion Result = { W, V };
	return(Result);
}

inline quaternion 
QuaternionAngleAxis(r32 Angle, vec3 Axis)
{
	quaternion Result;

	r32 AngleInRadians = Radians(Angle);
	r32 W = Cos(0.5f*AngleInRadians);
	vec3 V = Sin(0.5f*AngleInRadians)*Axis;

	Result = Quaternion(W, V);

	return(Result);
}

inline quaternion 
operator+ (quaternion A, quaternion B)
{
	quaternion Result;
	Result.w = A.w + B.w;
	Result.v = A.v + B.v;

	return(Result);
}

inline quaternion 
operator- (quaternion A)
{
	quaternion Result;
	Result.w = -A.w;
	Result.v = -A.v;

	return(Result);
}

inline quaternion 
operator* (quaternion A, r32 B)
{
	quaternion Result;

	Result.w = B*A.w;
	Result.v = B*A.v;

	return(Result);
}

inline quaternion 
operator* (r32 B, quaternion A)
{
	quaternion Result = A * B;
	return(Result);
}

inline quaternion 
operator* (quaternion A, quaternion B)
{
	quaternion Result;

	Result.w = A.w*B.w - Dot(A.v, B.v);
	Result.v = A.w*B.v + B.w*A.v + Cross(A.v, B.v);

	return(Result);
}

inline r32 
Length(quaternion A)
{
	r32 Result = SquareRoot(A.w*A.w + LengthSq(A.v));
	return(Result);
}

inline r32 
Dot(quaternion A, quaternion B)
{
	r32 Result = A.w*B.w + Dot(A.v, B.v);
	return(Result);
}

internal quaternion 
Slerp(quaternion A, quaternion B, r32 t)
{
	r32 Omega = ArcCos(Dot(A, B));
	quaternion Result = ((Sin((1.0f - t)*Omega)/Sin(Omega))*A) +
						((Sin(t*Omega)/Sin(Omega))*B);

	return(Result);
}

internal mat4 
QuaternionToMatrix(quaternion A)
{
	mat4 Result;

	r32 W = A.w;
	r32 X = A.v.x;
	r32 Y = A.v.y;
	r32 Z = A.v.z;

    Result.a11 = 1.0f - 2.0f*Y*Y - 2.0f*Z*Z;
    Result.a21 = 2.0f*X*Y + 2.0f*W*Z;
    Result.a31 = 2.0f*X*Z - 2.0f*W*Y;
    Result.a41 = 0.0f;

    Result.a12 = 2.0f*X*Y - 2.0f*W*Z;
    Result.a22 = 1.0f - 2.0f*X*X - 2.0f*Z*Z;
    Result.a32 = 2.0f*Y*Z + 2.0f*W*X;
    Result.a42 = 0.0f;

    Result.a13 = 2.0f*X*Z + 2.0f*W*Y;
    Result.a23 = 2.0f*Y*Z - 2.0f*W*X;
    Result.a33 = 1.0f - 2.0f*X*X - 2.0f*Y*Y;
    Result.a43 = 0.0f;

    Result.a14 = 0.0f;
    Result.a24 = 0.0f;
    Result.a34 = 0.0f;
    Result.a44 = 1.0f;

	return(Result);
}

internal quaternion 
RotationMatrixToQuaternion(mat4 A)
{
	quaternion Result;

	r32 m11 = A.a11;
	r32 m22 = A.a22;
	r32 m33 = A.a33;

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
			X = (A.a32 - A.a23) * Mult;
			Y = (A.a13 - A.a31) * Mult;
			Z = (A.a21 - A.a12) * Mult;
		} break;

		case 1:
		{
			W = (A.a32 - A.a23) * Mult;
			X = BiggestValue;
			Y = (A.a21 + A.a12) * Mult;
			Z = (A.a13 + A.a31) * Mult;
		} break;

		case 2:
		{
			W = (A.a13 - A.a31) * Mult;
			X = (A.a21 + A.a12) * Mult;
			Y = BiggestValue;
			Z = (A.a32 + A.a23) * Mult;
		} break;

		case 3:
		{
			W = (A.a21 - A.a12) * Mult;
			X = (A.a13 + A.a31) * Mult;
			Y = (A.a32 + A.a23) * Mult;
			Z = BiggestValue;
		} break;
	}

	Result = Quaternion(W, vec3(X, Y, Z));

	return(Result);
}

inline quaternion 
Conjugate(quaternion A)
{
	quaternion Result;

	Result.w = A.w;
	Result.v = -A.v;

	return(Result);
}

//
// NOTE(georgy): rect2
//

struct rect2
{
	vec2 Min;
	vec2 Max;
};

inline rect2 
RectMinMax(vec2 Min, vec2 Max)
{
	rect2 Result;
	
	Result.Min = Min;
	Result.Max = Max;

	return(Result);
}

inline rect2 
RectCenterHalfDim(vec2 Center, vec2 HalfDim)
{
	rect2 Result;

	Result.Min = Center - HalfDim;
	Result.Max = Center + HalfDim;

	return(Result);
}

inline rect2 
RectCenterDim(vec2 Center, vec2 Dim)
{
	rect2 Result = RectCenterHalfDim(Center, 0.5f*Dim);

	return(Result);
}

inline rect2 
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

inline rect3 
RectMinMax(vec3 Min, vec3 Max)
{
	rect3 Result;
	
	Result.Min = Min;
	Result.Max = Max;

	return(Result);
}

inline rect3 
RectCenterHalfDim(vec3 Center, vec3 HalfDim)
{
	rect3 Result;

	Result.Min = Center - HalfDim;
	Result.Max = Center + HalfDim;

	return(Result);
}

inline rect3 
RectCenterDim(vec3 Center, vec3 Dim)
{
	rect3 Result = RectCenterHalfDim(Center, 0.5f*Dim);

	return(Result);
}

inline rect3 
RectBottomFaceCenterDim(vec3 BottomFaceCenter, vec3 Dim)
{
	vec3 Center = BottomFaceCenter + 0.5f*vec3(0.0f, Dim.y, 0.0f);
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
	bool32 Result = ((Point.x >= A.Min.x) &&
					 (Point.y >= A.Min.y) &&
					 (Point.z >= A.Min.z) &&
					 (Point.x < A.Max.x) &&
					 (Point.y < A.Max.y) &&
			         (Point.z < A.Max.z));

	return(Result);
}

inline bool32
RectIntersect(rect3 A, rect3 B)
{
	bool32 Result = !((B.Max.x <= A.Min.x) ||
					 (B.Max.y <= A.Min.y) ||
					 (B.Max.z <= A.Min.z) ||
					 (B.Min.x >= A.Max.x) ||
					 (B.Min.y >= A.Max.y) ||
					 (B.Min.z >= A.Max.z));

	return(Result);
}

internal rect3
RectForTransformedRect(rect3 OldRect, mat3 Transformation, vec3 Translation = vec3(0.0f, 0.0f, 0.0f))
{
	rect3 Result = {Translation, Translation};
	r32 m11 = Transformation.a11;
	r32 m12 = Transformation.a21;
	r32 m13 = Transformation.a31;
	r32 m21 = Transformation.a21;
	r32 m22 = Transformation.a22;
	r32 m23 = Transformation.a23;
	r32 m31 = Transformation.a31;
	r32 m32 = Transformation.a32;
	r32 m33 = Transformation.a33;

	r32 OldRectMaxX = OldRect.Max.x;
	r32 OldRectMinX = OldRect.Min.x;
	r32 OldRectMaxY = OldRect.Max.y;
	r32 OldRectMinY = OldRect.Min.y;
	r32 OldRectMaxZ = OldRect.Max.z;
	r32 OldRectMinZ = OldRect.Min.z;

	if(m11 > 0.0f)
	{
		Result.Max.x = Result.Max.x + m11*OldRectMaxX;
		Result.Min.x = Result.Min.x + m11*OldRectMinX;
	}
	else
	{
		Result.Max.x = Result.Max.x + m11*OldRectMinX;
		Result.Min.x = Result.Min.x + m11*OldRectMaxX;
	}

	if(m12 > 0.0f)
	{
		Result.Max.x = Result.Max.x + m12*OldRectMaxY;
		Result.Min.x = Result.Min.x + m12*OldRectMinY;
	}
	else
	{
		Result.Max.x = Result.Max.x + m12*OldRectMinY;
		Result.Min.x = Result.Min.x + m12*OldRectMaxY;
	}

	if(m13 > 0.0f)
	{
		Result.Max.x = Result.Max.x + m13*OldRectMaxZ;
		Result.Min.x = Result.Min.x + m13*OldRectMinZ;
	}
	else
	{
		Result.Max.x = Result.Max.x + m13*OldRectMinZ;
		Result.Min.x = Result.Min.x + m13*OldRectMaxZ;
	}


	if(m21 > 0.0f)
	{
		Result.Max.y = Result.Max.y + m21*OldRectMaxX;
		Result.Min.y = Result.Min.y + m21*OldRectMinX;
	}
	else
	{
		Result.Max.y = Result.Max.y + m21*OldRectMinX;
		Result.Min.y = Result.Min.y + m21*OldRectMaxX;
	}

	if(m22 > 0.0f)
	{
		Result.Max.y = Result.Max.y + m22*OldRectMaxY;
		Result.Min.y = Result.Min.y + m22*OldRectMinY;
	}
	else
	{
		Result.Max.y = Result.Max.y + m22*OldRectMinY;
		Result.Min.y = Result.Min.y + m22*OldRectMaxY;
	}

	if(m23 > 0.0f)
	{
		Result.Max.y = Result.Max.y + m23*OldRectMaxZ;
		Result.Min.y = Result.Min.y + m23*OldRectMinZ;
	}
	else
	{
		Result.Max.y = Result.Max.y + m23*OldRectMinZ;
		Result.Min.y = Result.Min.y + m23*OldRectMaxZ;
	}

	
	if(m31 > 0.0f)
	{
		Result.Max.z = Result.Max.z + m31*OldRectMaxX;
		Result.Min.z = Result.Min.z + m31*OldRectMinX;
	}
	else
	{
		Result.Max.z = Result.Max.z + m31*OldRectMinX;
		Result.Min.z = Result.Min.z + m31*OldRectMaxX;
	}

	if(m32 > 0.0f)
	{
		Result.Max.z = Result.Max.z + m32*OldRectMaxY;
		Result.Min.z = Result.Min.z + m32*OldRectMinY;
	}
	else
	{
		Result.Max.z = Result.Max.z + m32*OldRectMinY;
		Result.Min.z = Result.Min.z + m32*OldRectMaxY;
	}

	if(m33 > 0.0f)
	{
		Result.Max.z = Result.Max.z + m33*OldRectMaxZ;
		Result.Min.z = Result.Min.z + m33*OldRectMinZ;
	}
	else
	{
		Result.Max.z = Result.Max.z + m33*OldRectMinZ;
		Result.Min.z = Result.Min.z + m33*OldRectMaxZ;
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

inline plane 
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


inline bool32 
IsPointInTriangle(vec3 P, vec3 A, vec3 B, vec3 C)
{
	A -= P; B -= P; C -= P;
	vec3 U = Cross(B, C);
	vec3 V = Cross(C, A);
	vec3 W = Cross(A, B);
	bool32 Result = (Dot(U, V) >= 0.0f) && (Dot(U, W) >= 0.0f);

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
	i32 I = FloorReal32ToInt32(P.x);
	i32 J = FloorReal32ToInt32(P.y);
	i32 K = FloorReal32ToInt32(P.z);

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

	r32 U = P.x - I;
	r32 V = P.y - J;
	r32 T = P.z - K;

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

// 
// NOTE(georgy): Sphere
// 

struct sphere
{
	vec3 P;
	r32 Radius;
};

inline void
AddPointToSphere(sphere *Sphere, vec3 Point)
{
	vec3 FromCenter = Point - Sphere->P;
	r32 DistanceToPoint = LengthSq(FromCenter);
	if(DistanceToPoint > (Sphere->Radius * Sphere->Radius))
	{
		DistanceToPoint = SquareRoot(DistanceToPoint);
		Sphere->P += (0.5f*(DistanceToPoint - Sphere->Radius))*Normalize(FromCenter);
		Sphere->Radius = 0.5f*(DistanceToPoint + Sphere->Radius);
	}
}

internal sphere
SphereFromPoints(vec3 *Points, u32 Count, u32 Iterations = 0)
{
	sphere Result = {};

	r32 MinX = FLT_MAX, MinY = FLT_MAX, MinZ = FLT_MAX, MaxX = -FLT_MAX, MaxY = -FLT_MAX, MaxZ = -FLT_MAX;
	u32 MinXIndex = Count - 1, MinYIndex = Count - 1, MinZIndex = Count - 1, MaxXIndex = 0, MaxYIndex = 0, MaxZIndex = 0;
	for(u32 PointIndex = 0;
		PointIndex < Count;
		PointIndex++)
	{
		if(MinX > Points[PointIndex].x) { MinX = Points[PointIndex].x; MinXIndex = PointIndex; }
		if(MinY > Points[PointIndex].y) { MinY = Points[PointIndex].y; MinYIndex = PointIndex; }
		if(MinZ > Points[PointIndex].z) { MinZ = Points[PointIndex].z; MinZIndex = PointIndex; }
		if(MaxX < Points[PointIndex].x) { MaxX = Points[PointIndex].x; MaxXIndex = PointIndex; }
		if(MaxY < Points[PointIndex].y) { MaxY = Points[PointIndex].y; MaxYIndex = PointIndex; }
		if(MaxZ < Points[PointIndex].z) { MaxZ = Points[PointIndex].z; MaxZIndex = PointIndex; }
	}

	r32 XDiff = MaxX - MinX;
	r32 YDiff = MaxY - MinY;
	r32 ZDiff = MaxZ - MinZ;
	u32 MinIndex = MinXIndex;
	u32 MaxIndex = MaxXIndex;
	if((YDiff > XDiff) && (YDiff > ZDiff))
	{
		MinIndex = MinYIndex;
		MaxIndex = MaxYIndex;
	}
	else if((ZDiff > XDiff) && (ZDiff > YDiff))
	{
		MinIndex = MinZIndex;
		MaxIndex = MaxZIndex;
	}

	Result.P = Lerp(Points[MinIndex], Points[MaxIndex], 0.5f);
	Result.Radius = Length(Points[MaxIndex] - Result.P);

	for(u32 PointIndex = 0;
		PointIndex < Count;
		PointIndex++)
	{
		AddPointToSphere(&Result, Points[PointIndex]);
	}
	
	for(u32 Iteration = 0;
		Iteration < Iterations;
		Iteration++)
	{
		sphere NewSphere = {Result.P, 0.8f*Result.Radius};
				
		for(u32 PointIndex = 0;
			PointIndex < Count;
			PointIndex++)
		{
			AddPointToSphere(&NewSphere, Points[PointIndex]);
		}

		if(NewSphere.Radius < Result.Radius)
		{
			Result = NewSphere;
		}
	}

	return(Result);
}