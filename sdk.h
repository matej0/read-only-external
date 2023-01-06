#pragma once
#include "Vector3D.h"
#include <iostream>
#include <Windows.h>
#include <memory>
#include <cmath>
#include <vector>
#include <algorithm>
#include <cstdio>
#include <tlhelp32.h>
#define DEG2RAD( x ) ( ( float )( x ) * ( float )( ( float )( M_PI ) / 180.0f ) )
#define RAD2DEG( x ) ( ( float )( x ) * ( float )( 180.0f / ( float )( M_PI ) ) )

class Vector2D {
public:
	float x, y;

	inline Vector2D& operator=(const Vector2D &vOther)
	{
		x = vOther.x; y = vOther.y;
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
	inline bool operator==(const Vector2D& src) const
	{
		return (src.x == x) && (src.y == y);
	}
	//===============================================
	inline bool operator!=(const Vector2D& src) const
	{
		return (src.x != x) || (src.y != y);
	}
	//===============================================
	__forceinline void VectorCopy(const Vector2D& src, Vector2D& dst)
	{
		dst.x = src.x;
		dst.y = src.y;
	}
	//===============================================
	__forceinline Vector2D& operator+=(const Vector2D& v)
	{

		x += v.x; y += v.y;
		return *this;
	}
	//===============================================
	__forceinline Vector2D& operator-=(const Vector2D& v)
	{
		x -= v.x; y -= v.y;
		return *this;
	}
	//===============================================
	__forceinline Vector2D& operator*=(float fl)
	{
		x *= fl;
		y *= fl;
		return *this;
	}
	//===============================================
	__forceinline  Vector2D& operator*=(const Vector2D& v)
	{
		x *= v.x;
		y *= v.y;
		return *this;
	}
	//===============================================
	__forceinline Vector2D&	operator+=(float fl)
	{
		x += fl;
		y += fl;
		return *this;
	}
	//===============================================
	__forceinline Vector2D&	operator-=(float fl)
	{
		x -= fl;
		y -= fl;
		return *this;
	}
	//===============================================
	__forceinline  Vector2D& operator/=(float fl)
	{
		float oofl = 1.0f / fl;
		x *= oofl;
		y *= oofl;
		return *this;
	}
	//===============================================
	__forceinline Vector2D& operator/=(const Vector2D& v)
	{
		x /= v.x;
		y /= v.y;
		return *this;
	}

	inline float Length(void) const
	{

		float root = 0.0f;

		float sqsr = x * x + y * y;

		__asm
		{
			sqrtss xmm0, sqsr
			movss root, xmm0
		}

		return root;
	}

	float Normalize()
	{
		float flLength = Length();
		float flLengthNormal = 1.f / (FLT_EPSILON + flLength);

		x *= flLengthNormal;
		y *= flLengthNormal;

		return flLength;
	}
};

class ICollision {
public:
	int pad[2];
	Vector3D mins;
	Vector3D maxs;
};

class CHudRadar
{
public:
	char pad_0x0000[0xE0]; //0x0000
	Vector2D dummy;
	Vector2D m_vMapPos; //0x00E0 
	char pad_0x00E8[0xC]; //0x00E8
	float m_flScale; //0x00F4 
	char pad_0x00F8[0x1C]; //0x00F8
	Vector2D m_vLocalPos; //0x0114 
	char pad_0x011C[0x4]; //0x011C
	Vector2D m_vOverviewCoords; //0x0120 
	char pad_0x0128[0x4]; //0x0128
	float m_flYawMinus90; //0x012C 
	char pad_0x0130[0x174]; //0x0130
	Vector2D m_vLocalAngles; //0x02A4 
	char pad_0x02AC[0x28]; //0x02AC
	float m_flTime; //0x02D4 
	char pad_0x02D8[0x164]; //0x02D8

}; //Size=0x043C

struct player_info
{
	__int64         unknown;            //0x0000 
	union
	{
		__int64       steamID64;          //0x0008 - SteamID64
		struct
		{
			__int32     xuid_low;
			__int32     xuid_high;
		};
	};
	char            szName[128];        //0x0010 - Player Name
	int             userId;             //0x0090 - Unique Server Identifier
	char            szSteamID[20];      //0x0094 - STEAM_X:Y:Z
	char            pad_0x00A8[0x10];   //0x00A8
	unsigned long   iSteamID;           //0x00B8 - SteamID 
	char            szFriendsName[128];
	bool            fakeplayer;
	bool            ishltv;
	unsigned int    customfiles[4];
	unsigned char   filesdownloaded;
} var;

#pragma pack(push, 4)
class SFMapOverviewIconPackage
{
public:
	char m_Icons[34][4]; //0x0000
	Vector3D m_Position; //0x0088
	Vector3D m_Angle; //0x0094
	Vector3D m_HudPosition; //0x00A0
	float m_HudRotation; //0x00AC
	float m_HudScale; //0x00B0
	float m_fRoundStartTime; //0x00B4
	float m_fDeadTime; //0x00B8
	float m_fGhostTime; //0x00BC
	float m_fCurrentAlpha; //0x00C0
	float m_fLastColorUpdate; //0x00C4
	uint64_t m_iCurrentVisibilityFlags; //0x00C8
	int32_t m_iIndex; //0x00D0
	int32_t m_iEntityID; //0x00D4
	int32_t m_Health; //0x00D8
	char m_wcName[128]; //0x00DC
	int32_t m_iPlayerType; //0x015C
	int32_t m_nAboveOrBelow; //0x0160
	int32_t m_nGrenadeType; //0x0164
	float m_fGrenExpireTime; //0x0168
	int32_t m_IconPackType; //0x016C
	bool m_bIsActive : 1; //0x0170
	bool m_bOffMap : 1;
	bool m_bIsLowHealth : 1;
	bool m_bIsSelected : 1;
	bool m_bIsFlashed : 1;
	bool m_bIsFiring : 1;
	bool m_bIsPlayer : 1;
	bool m_bIsSpeaking : 1;
	bool m_bIsDead : 1;
	bool m_bIsMovingHostage : 1;
	bool m_bIsSpotted : 1;
	bool m_bIsRescued : 1;
	bool m_bIsOnLocalTeam : 1;
	bool m_bIsDefuser : 1;

}; //Size: 0x0174
static_assert(sizeof(SFMapOverviewIconPackage) == 0x174);
#pragma pack(pop)


typedef struct _D3DMATRIX3 {
	union {
		struct {
			float        _11, _12, _13, _14;
			float        _21, _22, _23, _24;
			float        _31, _32, _33, _34;
			float        _41, _42, _43, _44;

		};
		float m[3][4];
	};
} D3DMATRIX3;


float get_fov(Vector3D point1, Vector3D point2) {
	float dx = point2.x - point1.x;
	float dy = point2.y - point1.y;
	float dz = point2.z - point1.z;

	float distance = sqrt(dx * dx + dy * dy + dz * dz);

	return distance;
}

FORCEINLINE float __vectorcall FastSqrt(float x)
{
	__m128 root = _mm_sqrt_ss(_mm_load_ss(&x));
	return *(reinterpret_cast<float *>(&root));
}

void VectorAngles(const Vector3D& forward, Vector3D &angles)
{
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

		tmp = FastSqrt (forward[0] * forward[0] + forward[1] * forward[1]);
		pitch = (atan2(-forward[2], tmp) * 180 / M_PI);
		if (pitch < 0)
			pitch += 360;
	}

	angles[0] = pitch;
	angles[1] = yaw;
	angles[2] = 0;
}


Vector3D calc_angle(Vector3D src, Vector3D dst)
{
	Vector3D angle;
	Vector3D delta = src - dst;
	delta.Normalize();
	VectorAngles(delta, angle);

	return angle;
}

void clamp_angles(Vector3D& angles)
{
	if (angles.x > 89.0f) angles.x = 89.0f;
	else if (angles.x < -89.0f) angles.x = -89.0f;

	if (angles.y > 180.0f) angles.y = 180.0f;
	else if (angles.y < -180.0f) angles.y = -180.0f;

	angles.z = 0;
}

