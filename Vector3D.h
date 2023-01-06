#pragma once
typedef float matrix3x4[3][4];

inline float DotProduct(const float* v1, const float* v2)
{
	return(v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2]);
}


class Vector3D {
public:
	float x, y, z;

	Vector3D(float X = 0.0f, float Y = 0.0f, float Z = 0.0f) {
		x = X;
		y = Y;
		z = Z;
	};

	inline float Dot(const Vector3D& vOther) const
	{
		const Vector3D& a = *this;

		return(a.x*vOther.x + a.y*vOther.y + a.z*vOther.z);
	}

	
	Vector3D operator-(const Vector3D& other) {
		Vector3D result;
		result.x = x - other.x;
		result.y = y - other.y;
		result.z = z - other.z;
		return result;
	}

	// Implement the + operator that adds two Vector3D objects
	Vector3D operator+(const Vector3D& other) {
		Vector3D result;
		result.x = x + other.x;
		result.y = y + other.y;
		result.z = z + other.z;
		return result;
	}
	inline Vector3D& operator=(const Vector3D &vOther)
	{
		x = vOther.x; y = vOther.y; z = vOther.z;
		return *this;
	}
	//===============================================
	inline float& operator[](int i)
	{
		return ((float*)this)[i];
	}
	//===============================================
	inline float operator[](int i) const
	{
		return ((float*)this)[i];
	}
	//===============================================
	inline bool operator==(const Vector3D& src) const
	{
		return (src.x == x) && (src.y == y) && (src.z == z);
	}
	//===============================================
	inline bool operator!=(const Vector3D& src) const
	{
		return (src.x != x) || (src.y != y) || (src.z != z);
	}
	//===============================================
	
	//===============================================
	__forceinline  Vector3D& operator+=(const Vector3D& v)
	{
		x += v.x; y += v.y; z += v.z;
		return *this;
	}
	//===============================================
	__forceinline  Vector3D& operator-=(const Vector3D& v)
	{
		x -= v.x; y -= v.y; z -= v.z;
		return *this;
	}
	//===============================================
	__forceinline  Vector3D& operator*=(float fl)
	{
		x *= fl;
		y *= fl;
		z *= fl;
		return *this;
	}
	//===============================================
	__forceinline  Vector3D& operator*=(const Vector3D& v)
	{
		x *= v.x;
		y *= v.y;
		z *= v.z;
		return *this;
	}
	//===============================================
	__forceinline Vector3D& operator+=(float fl)
	{
		x += fl;
		y += fl;
		z += fl;
		return *this;
	}
	//===============================================
	__forceinline Vector3D& operator-=(float fl)
	{
		x -= fl;
		y -= fl;
		z -= fl;
		return *this;
	}
	//===============================================
	__forceinline  Vector3D& operator/=(float fl)
	{
		float oofl = 1.0f / fl;
		x *= oofl;
		y *= oofl;
		z *= oofl;
		return *this;
	}
	//===============================================
	__forceinline  Vector3D& operator/=(const Vector3D& v)
	{
		x /= v.x;
		y /= v.y;
		z /= v.z;
		return *this;
	}

	inline float Length(void) const
	{
		float root = 0.0f;

		float sqsr = x * x + y * y + z * z;

		__asm
		{
			sqrtss xmm0, sqsr
			movss root, xmm0
		}

		return root;
	}

	__forceinline float LengthSqr(void) const
	{
		return (x*x + y * y + z * z);
	}

	bool IsZero(float tolerance = 0.01f) const
	{
		return (x > -tolerance && x < tolerance &&
			y > -tolerance && y < tolerance &&
			z > -tolerance && z < tolerance);
	}

	float Normalize()
	{
		float flLength = Length();
		float flLengthNormal = 1.f / (FLT_EPSILON + flLength);

		x *= flLengthNormal;
		y *= flLengthNormal;
		z *= flLengthNormal;

		return flLength;
	}
};

__forceinline void VectorCopy(const Vector3D& src, Vector3D& dst)
{
	dst.x = src.x;
	dst.y = src.y;
	dst.z = src.z;
}