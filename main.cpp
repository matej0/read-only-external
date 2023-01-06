#include <iostream>
#include <Windows.h>
#include <memory>
#include <cmath>
#include <vector>
#include <algorithm>
#include <cstdio>
#include <tlhelp32.h>
#include "Vector3D.h"
#include "csgo.h"
#include <aero-overlay/overlay.hpp>
#include <thread>
#include <chrono>
#include <d3dx9.h>
#include <intrin.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include "sdk.h"
constexpr float RADARRADIUS = 125.0f;


bool aimbot;
bool box;
bool radar;
bool headdot;

void keybinds()
{
	if (GetAsyncKeyState(VK_F1) & 1)
	{
		aimbot = !aimbot;
		printf("trigger: %s\n", aimbot ? "ON" : "OFF");
		Beep(aimbot ? 500 : 300, 120); //simple beep, lets you to know if feature is on or off, prolly dont even need to print this crap to console.
	}

	if (GetAsyncKeyState(VK_F2) & 1)
	{
		box = !box;
		printf("Box: %s\n", box ? "ON" : "OFF");
		Beep(box ? 500 : 300, 120);
	}

	if (GetAsyncKeyState(VK_F3) & 1)
	{
		radar = !radar;
		printf("Radar: %s\n", radar ? "ON" : "OFF");
		Beep(radar ? 500 : 300, 120);
	}

	if (GetAsyncKeyState(VK_F4) & 1)
	{
		headdot = !headdot;
		printf("Head dot: %s\n", headdot ? "ON" : "OFF");
		Beep(headdot ? 500 : 300, 120);
	}
}


struct offsets {
	DWORD local_idx;
	D3DMATRIX local_view_matrix;
	Vector3D local_view_angles; // from clientstate.
	Vector3D local_eye_angles; // from netvar.
	Vector3D local_eye_position;
	Vector3D local_origin;
} globals;

DWORD game_process_id;
DWORD client_base;
DWORD engine_base;
HANDLE game_handle; // we only need permission to read memory.
DWORD radar_base;

template<typename T>
T read(HANDLE hProcess, DWORD lpBaseAddress)
{
	T data;
	SIZE_T nBytesRead;
	if (ReadProcessMemory(hProcess, reinterpret_cast<LPVOID>(lpBaseAddress), &data, sizeof(T), &nBytesRead))
	{
		return data;
	}
	else
	{
		// Read failed
		return T();
	}
}

template<typename T>
inline bool read_and_store(DWORD dwAddress, T& Value)
{
	return ReadProcessMemory(game_handle, reinterpret_cast<LPVOID>(dwAddress), &Value, sizeof(T), NULL);
}

DWORD GetProcessId(const char* processName) {
	HANDLE snapshot;
	PROCESSENTRY32 entry;

	snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snapshot == INVALID_HANDLE_VALUE)
		return 0;

	entry.dwSize = sizeof(entry);

	if (!Process32First(snapshot, &entry)) {
		CloseHandle(snapshot);
		return 0;
	}

	do {
		if (!_stricmp(entry.szExeFile, processName)) {
			CloseHandle(snapshot);
			return entry.th32ProcessID;
		}
	} while (Process32Next(snapshot, &entry));

	CloseHandle(snapshot);
	return 0;
}

DWORD GetModuleBaseAddress(DWORD processId, const char* moduleName) {
	HANDLE snapshot;
	MODULEENTRY32 entry;

	snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, processId);
	if (snapshot == INVALID_HANDLE_VALUE)
		return 0;

	entry.dwSize = sizeof(entry);

	if (!Module32First(snapshot, &entry)) {
		CloseHandle(snapshot);
		return 0;
	}

	do {
		if (!_stricmp(entry.szModule, moduleName)) {
			CloseHandle(snapshot);
			return (DWORD)entry.modBaseAddr;
		}
	} while (Module32Next(snapshot, &entry));

	CloseHandle(snapshot);
	return 0;
}

static bool ScreenTransform(const Vector3D& point, Vector3D& screen)
{
	const D3DXMATRIX& w2sMatrix = globals.local_view_matrix;
	screen.x = w2sMatrix.m[0][0] * point.x + w2sMatrix.m[0][1] * point.y + w2sMatrix.m[0][2] * point.z + w2sMatrix.m[0][3];
	screen.y = w2sMatrix.m[1][0] * point.x + w2sMatrix.m[1][1] * point.y + w2sMatrix.m[1][2] * point.z + w2sMatrix.m[1][3];
	screen.z = 0.0f;

	float w = w2sMatrix.m[3][0] * point.x + w2sMatrix.m[3][1] * point.y + w2sMatrix.m[3][2] * point.z + w2sMatrix.m[3][3];

	if (w < 0.001f) {
		screen.x *= 100000;
		screen.y *= 100000;
		return true;
	}

	float invw = 1.0f / w;
	screen.x *= invw;
	screen.y *= invw;

	return false;
}

static bool WorldToScreen(const Vector3D &origin, Vector3D &screen, int w, int h)
{
	if (!ScreenTransform(origin, screen)) {
		screen.x = (w / 2.0f) + (screen.x * w) / 2;
		screen.y = (h / 2.0f) - (screen.y * h) / 2;

		return true;
	}
	return false;
}





bool calculate_bounds(DWORD entity, int & x, int & y, int & w, int & h)
{
	const D3DMATRIX3 vMatrix = read<D3DMATRIX3>(game_handle, entity + hazedumper::netvars::m_rgflCoordinateFrame);

	ICollision collision = read<ICollision>(game_handle, entity + hazedumper::netvars::m_Collision);
	Vector3D vMin = collision.mins;
	Vector3D vMax = collision.maxs;

	Vector3D vPointList[] = {
		Vector3D(vMin.x, vMin.y, vMin.z),
		Vector3D(vMin.x, vMax.y, vMin.z),
		Vector3D(vMax.x, vMax.y, vMin.z),
		Vector3D(vMax.x, vMin.y, vMin.z),
		Vector3D(vMax.x, vMax.y, vMax.z),
		Vector3D(vMin.x, vMax.y, vMax.z),
		Vector3D(vMin.x, vMin.y, vMax.z),
		Vector3D(vMax.x, vMin.y, vMax.z)
	};

	Vector3D vTransformed[8];

	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			for (int j = 0; j < 3; j++)
				vTransformed[i][j] = vPointList[i].Dot((Vector3D&)vMatrix.m[j]) + vMatrix.m[j][3];
		}
	}
		

	Vector3D flb, brt, blb, frt, frb, brb, blt, flt;

	if (!WorldToScreen(vTransformed[3], flb, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)) ||
		!WorldToScreen(vTransformed[0], blb, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)) ||
		!WorldToScreen(vTransformed[2], frb, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)) ||
		!WorldToScreen(vTransformed[6], blt, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)) ||
		!WorldToScreen(vTransformed[5], brt, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)) ||
		!WorldToScreen(vTransformed[4], frt, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)) ||
		!WorldToScreen(vTransformed[1], brb, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)) ||
		!WorldToScreen(vTransformed[7], flt, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)))
		return false;

	Vector3D arr[] = { flb, brt, blb, frt, frb, brb, blt, flt };

	float left = flb.x;
	float top = flb.y;
	float right = flb.x;
	float bottom = flb.y;

	for (int i = 0; i < 8; i++)
	{
		if (left > arr[i].x)
			left = arr[i].x;
		if (top < arr[i].y)
			top = arr[i].y;
		if (right < arr[i].x)
			right = arr[i].x;
		if (bottom > arr[i].y)
			bottom = arr[i].y;
	}

	x = left;
	y = bottom;
	w = right - left;
	h = top - bottom;

	return true;
}


int health(DWORD entity)
{
	return read<int>(game_handle, entity + hazedumper::netvars::m_iHealth);
}

bool alive(DWORD entity) 
{
	return (health <= 0);
}

int team(DWORD entity)
{
	return read<int>(game_handle, entity + hazedumper::netvars::m_iTeamNum);
}
bool enemy(DWORD entity, DWORD local_entity) 
{
	return (team(entity) != team(local_entity));
}

bool dormant(DWORD entity)
{
	return read<bool>(game_handle, entity + hazedumper::signatures::m_bDormant);
}


Vector3D rotate_point(const Vector3D& point, const Vector3D& center, float angle)
{
	/*float _cos = std::cosf(DEG2RAD(angle));
	float _sin = std::sinf(DEG2RAD(angle));

	Vector3D relative =
	{
		point.x - center.x,
		point.y - center.y
	};

	Vector3D rotated =
	{
		_cos * (relative.x) - _sin * (relative.y),
		_sin * (relative.x) + _cos * (relative.y)
	};

	rotated += center;

	return rotated;*/

	float costheta = (float)std::cosf(DEG2RAD(angle));
	float sintheta = (float)std::sinf(DEG2RAD(angle));

	Vector3D out = {
		costheta * (point.x - center.y) - sintheta * (point.y - center.y),
		sintheta * (point.x - center.y) + costheta * (point.y - center.y)
	};

	out += center;
	return out;
		
};


void get_radar_point(Vector3D enemy, Vector3D local, Vector3D local_angle, float &d, float &p, float hud_scale)
{
	float delta_x = enemy.x - local.x;
	float delta_y = enemy.y - local.y;

	float radar_scale = 0.7f / 10.f;
	const float radar_radius = 125.f;

	delta_x *= radar_scale;
	delta_y *= radar_scale;


	Vector3D point = { delta_x, delta_y };

	/*if (point.Length() >= radar_radius)
	{
		auto clip = point.Normalize();
		point.x = clip * radar_radius;
		point.y = clip * radar_radius;
	}*/

	point.y *= -1.f;

	Vector3D center = { 145.f, 195.f };
	point += center;

	Vector3D pos = rotate_point(point, center, local_angle.y - 90.f);

	d = pos.x;
	p = pos.y;
}

void WorldToRadar(const Vector3D& ptin, Vector3D& ptout, Vector2D map)
{
	const float pixel_to_radar_scale = 0.586f; // always constant.
	//float world_to_pixel_scale = 1.0f / 5.f;
	const float world_to_pixel_scale = read<float>(game_handle, radar_base + 0x108);
	float world_to_radar_scale = world_to_pixel_scale * pixel_to_radar_scale;

	float fWorldScale = world_to_radar_scale;//world_to_radar_scale;
	float flMapScaler = 0.f;

	float fScale = 0.7f + flMapScaler;

	fWorldScale *= fScale;

	ptout.x = ((ptin.x - (map.x)) * fWorldScale);
	ptout.y = (((map.y) - ptin.y) * fWorldScale);
	ptout.z = 0.f;
}

//void Vector3DMultiply(const D3DMATRIX3 &src1, const Vector3D &src2, Vector3D &dst)
//{
//	// Make sure it works if src2 == dst
//	Vector3D tmp;
//	const Vector3D &v = (&src2 == &dst) ? static_cast<const Vector3D>(tmp) : src2;
//
//	if (&src2 == &dst)
//	{
//		VectorCopy(src2, tmp);
//	}
//
//	dst[0] = src1.m[0][0] * v[0] + src1.m[0][1] * v[1] + src1.m[0][2] * v[2];
//	dst[1] = src1.m[1][0] * v[0] + src1.m[1][1] * v[1] + src1.m[1][2] * v[2];
//	dst[2] = src1.m[2][0] * v[0] + src1.m[2][1] * v[1] + src1.m[2][2] * v[2];
//}
//
//
//
//void MatrixBuildRotateZ(D3DMATRIX3 &dst, float angleDegrees)
//{
//	float radians = angleDegrees * (M_PI / 180.0f);
//
//	float fSin = (float)sin(radians);
//	float fCos = (float)cos(radians);
//
//	dst.m[0][0] = fCos; dst.m[0][1] = -fSin; dst.m[0][2] = 0.0f; dst.m[0][3] = 0.0f;
//	dst.m[1][0] = fSin; dst.m[1][1] = fCos; dst.m[1][2] = 0.0f; dst.m[1][3] = 0.0f;
//	dst.m[2][0] = 0.0f; dst.m[2][1] = 0.0f; dst.m[2][2] = 1.0f; dst.m[2][3] = 0.0f;
//	dst.m[3][0] = 0.0f; dst.m[3][1] = 0.0f; dst.m[3][2] = 0.0f; dst.m[3][3] = 1.0f;
//}
//
//
//void RadarToHud(const Vector3D& ptin, Vector3D& ptout, float rotation, Vector3D local_radar)
//{
//	D3DMATRIX3 rot;
//	MatrixBuildRotateZ(rot, rotation);
//
//	Vector3D multed;
//	multed.x = ptin.x - local_radar.x;
//	multed.y = ptin.y - local_radar.y;
//
//	Vector3DMultiply(rot, multed, ptout);
//}

void Vector3DMultiply(const D3DMATRIX3 &src1, const Vector3D &src2, Vector3D &dst)
{
	dst[0] = src1.m[0][0] * src2[0] + src1.m[0][1] * src2[1] + src1.m[0][2] * src2[2];
	dst[1] = src1.m[1][0] * src2[0] + src1.m[1][1] * src2[1] + src1.m[1][2] * src2[2];
	dst[2] = src1.m[2][0] * src2[0] + src1.m[2][1] * src2[1] + src1.m[2][2] * src2[2];
}

void MatrixBuildRotateZ(D3DMATRIX3 &dst, float angleDegrees)
{
	float radians = DEG2RAD(angleDegrees);//angleDegrees * (M_PI / 180.0f);

	float fSin = sinf(radians);
	float fCos = cosf(radians);

	dst.m[0][0] = fCos; dst.m[0][1] = -fSin; dst.m[0][2] = 0.0f; dst.m[0][3] = 0.0f;
	dst.m[1][0] = fSin; dst.m[1][1] = fCos; dst.m[1][2] = 0.0f; dst.m[1][3] = 0.0f;
	dst.m[2][0] = 0.0f; dst.m[2][1] = 0.0f; dst.m[2][2] = 1.0f; dst.m[2][3] = 0.0f;
	dst.m[3][0] = 0.0f; dst.m[3][1] = 0.0f; dst.m[3][2] = 0.0f; dst.m[3][3] = 1.0f;
}

void RadarToHud(const Vector3D& ptin, Vector3D& ptout, float rotation, Vector3D local_radar)
{
	D3DMATRIX3 rot;
	MatrixBuildRotateZ(rot, rotation);

	Vector3D multed;
	multed.x = ptin.x - local_radar.x;
	multed.y = ptin.y - local_radar.y;

	Vector3DMultiply(rot, multed, ptout);
}


void aim(DWORD local_entity)
{
	if (!aimbot)
		return;

	DWORD best_entity = NULL;
	float dist_to_best = (std::numeric_limits<float>::max)();

	if (GetAsyncKeyState(VK_XBUTTON1) || GetAsyncKeyState(VK_LBUTTON))
	{
		for (unsigned int i = 1; i < 64; i++)
		{
			DWORD entity = read<DWORD>(game_handle, client_base + hazedumper::signatures::dwEntityList + i * 0x10);

			if (!entity || health(entity) < 1 || !enemy(entity, local_entity) || dormant(entity))
				continue;

			DWORD bone_matrix = read<DWORD>(game_handle, entity + hazedumper::netvars::m_dwBoneMatrix);
			D3DMATRIX3 matrix = read<D3DMATRIX3>(game_handle, bone_matrix + 8 * 0x30);

			Vector3D head = Vector3D(matrix.m[0][3], matrix.m[1][3], matrix.m[2][3]);

			Vector3D angle = calc_angle(globals.local_eye_position, head);
			angle.Normalize();
			clamp_angles(angle);

			Vector3D temp = (angle - globals.local_eye_position);
			temp.Normalize();

			float fov = temp.Length();

			if (fov < dist_to_best)
			{
				dist_to_best = fov;
				best_entity = entity;
			}
		}

		if (best_entity && best_entity != local_entity)
		{
			DWORD bone_matrix_target = read<DWORD>(game_handle, best_entity + hazedumper::netvars::m_dwBoneMatrix);
			D3DMATRIX3 matrix_target = read<D3DMATRIX3>(game_handle, bone_matrix_target + 8 * 0x30);
			Vector3D head_target = Vector3D(matrix_target.m[0][3], matrix_target.m[1][3], matrix_target.m[2][3]);
			
			if (!head_target.IsZero())
			{
				Vector3D vecTransformedPos;
				WorldToScreen(head_target, vecTransformedPos, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
			}
		}
	}
}

void trigger(DWORD local_entity)
{
	if (!aimbot)
		return;

	if (GetAsyncKeyState(VK_XBUTTON1))
	{
		int idx = read<int>(game_handle, local_entity + hazedumper::netvars::m_iCrosshairId);
		DWORD entity = read<DWORD>(game_handle, client_base + hazedumper::signatures::dwEntityList + idx * 0x10);

		if (idx > 0 && idx < 65)
		{
			INPUT Input = { 0 };
			Input.type = INPUT_MOUSE;
			Input.mi.mouseData = 0;
			Input.mi.time = 0;

			Input.mi.dwFlags = (MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE | MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP);;
			SendInput(1, &Input, sizeof(INPUT));
		}
	}
}

char current_map[256];
void esp(aero::surface_ptr surface, aero::font_ptr font, DWORD local_entity)
{
	for (unsigned int i = 0; i < 65; i++)
	{
		DWORD entity = read<DWORD>(game_handle, client_base + hazedumper::signatures::dwEntityList + i * 0x10);

		if (!entity || health(entity) < 1 || !enemy(entity, local_entity))
			continue;
		
		//std::cout << i << ": " << health(entity) << std::endl;

		CHudRadar hud_radar = read<CHudRadar>(game_handle, radar_base); // read it as a class so we can access the radar information
		const auto radar_player = read<SFMapOverviewIconPackage>(game_handle, radar_base + 0xB0 + (sizeof(SFMapOverviewIconPackage) * (i * 0x10)));

		if (!dormant(entity))
		{
			if (box)
			{
				int x, y, w, h;
				calculate_bounds(entity, x, y, w, h);

				/*if (radar_player.m_bIsDead && dormant(entity))
				{

					Vector3D radar_origin = radar_player.m_Position;
					Vector3D radar_origin_2d = Vector3D();

					if (WorldToScreen(radar_origin, radar_origin_2d, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)))
					{
						surface->border_box(x, y, w, h, 1.f, aero::color(255, 255, 255, 180), 0.f, 0.f);
					}

					continue;
				}*/

				surface->border_box(x, y, w, h, 1.f, aero::color(255, 255, 255, 200), 1.f, 1.f, aero::color(0, 0, 0, 150), aero::color(0, 0, 0, 150));
			}

			if (headdot)
			{
				DWORD bone_matrix_target = read<DWORD>(game_handle, entity + hazedumper::netvars::m_dwBoneMatrix);
				D3DMATRIX3 matrix_target = read<D3DMATRIX3>(game_handle, bone_matrix_target + 8 * 0x30);
				Vector3D head_target = Vector3D(matrix_target.m[0][3], matrix_target.m[1][3], matrix_target.m[2][3]);
				Vector3D head_transformed;

				if (WorldToScreen(head_target, head_transformed, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)))
				{
					surface->text(head_transformed.x - 4, head_transformed.y - 6, font, 0xFFFFFFFF, "+");
				}
			}
		}
		

		if (radar)
		{
			if (/*!dormant(entity) && */health(local_entity) > 0)
			{
				Vector3D my_origin = read<Vector3D>(game_handle, local_entity + hazedumper::netvars::m_vecOrigin);
				Vector3D enemy_origin = read<Vector3D>(game_handle, entity + hazedumper::netvars::m_vecOrigin);

				//if (dormant(entity) || radar_player.m_bIsDead)
				//	enemy_origin = radar_player.m_Position;

				//printf("scale: %.f, %.f\n", hud_radar.m_flScale, radar_player.m_HudScale);
				//printf("hud rot: %.f\n", radar_player.m_HudRotation);

				//float rx, ry, z, scale;
				//get_radar_point(enemy_origin, my_origin, globals.local_view_angles, rx, ry, radar_player.m_HudScale);
				//surface->text(rx - 4, ry - 4, font, 0xFFFFFFFF, "X");

				const float mapSize = 320.f;
				Vector3D mapPosition;
				Vector3D mapCenter = { mapSize / 2.f, mapSize / 2.f };

				float mapAngle;

				WorldToRadar(enemy_origin, mapPosition, hud_radar.m_vMapPos);

				//printf("%.f : %.f\n", hud_radar.m_vOverviewCoords.x, hud_radar.m_vOverviewCoords.y);
				//printf("%.f\n", hud_radar.m_flScale);

				Vector3D m_RadarViewpointWorld = my_origin;
				m_RadarViewpointWorld.z = 0.f;

				Vector3D m_RadarViewpointMap;
				WorldToRadar(m_RadarViewpointWorld, m_RadarViewpointMap, hud_radar.m_vMapPos);

				Vector3D newMapPosition = mapPosition;
				newMapPosition -= m_RadarViewpointMap;
				float dist = newMapPosition.LengthSqr();
				if (dist > RADARRADIUS*RADARRADIUS)
				{
					newMapPosition *= std::sqrt(RADARRADIUS*RADARRADIUS / dist);
					mapPosition = m_RadarViewpointMap;
					mapPosition += newMapPosition;
					//mapAngle = 180.0f*atan2(newMapPosition.y, newMapPosition.x) / 3.141592f + 90;
				}

			//	else
				{
					mapAngle = globals.local_view_angles.y - 90.f;
				}

				Vector3D hudpos;
				RadarToHud(mapPosition, hudpos, mapAngle, m_RadarViewpointMap);
				hudpos += { 145, 195 };

				surface->text(hudpos.x, hudpos.y, font, 0xFFFFFFFF, "x");

				// non-rotating
				//surface->text(mapPosition.x - 4, mapPosition.y - 4, font, 0xFFFFFFFF, "x");
				//surface->text(m_RadarViewpointMap.x - 4, m_RadarViewpointMap.y - 4, font, 0xFFFFFFFF, "+");
			}
			surface->text(145, 195, font, 0xFFFFFFFF, "+");

		}


	}
}




int main()
{
	game_process_id = GetProcessId("csgo.exe");
	client_base = GetModuleBaseAddress(game_process_id, "client.dll");
	engine_base = GetModuleBaseAddress(game_process_id, "engine.dll");
	game_handle = OpenProcess(PROCESS_VM_READ, 0, game_process_id); // we only need permission to read memory.

	const auto overlay = std::make_unique<aero::overlay>();
	const auto status = overlay->attach(game_process_id);

	if (status != aero::api_status::success) 
	{
		printf("[>] failed to create overlay: %d\n", status);
		return -1;
	}

	const auto surface = overlay->get_surface();
	const auto font = surface->add_font("test", "Verdana", 12.f);

	/*surface->add_callback([&surface, &font]
	{
		surface->text(5.f, 5.f, font, 0xFFFFFFFF, "this is an example");
	});*/





	while (overlay->message_loop()) 
	{
		keybinds();

		DWORD client_state = read<DWORD>(game_handle, engine_base + hazedumper::signatures::dwClientState);
		globals.local_idx = read<DWORD>(game_handle, client_state + hazedumper::signatures::dwClientState_GetLocalPlayer) + 1;

		DWORD local_entity = read<DWORD>(game_handle, client_base + hazedumper::signatures::dwLocalPlayer);

		globals.local_view_matrix = read<D3DMATRIX>(game_handle, client_base + hazedumper::signatures::dwViewMatrix);
		globals.local_eye_angles = Vector3D(read<float>(game_handle, local_entity + hazedumper::netvars::m_angEyeAnglesX), read<float>(game_handle, local_entity + hazedumper::netvars::m_angEyeAnglesY), 0.0f);
		globals.local_view_angles = read<Vector3D>(game_handle, client_state + hazedumper::signatures::dwClientState_ViewAngles);
		globals.local_eye_position = read<Vector3D>(game_handle, local_entity + hazedumper::netvars::m_vecOrigin) + read<Vector3D>(game_handle, local_entity + hazedumper::netvars::m_vecViewOffset);
		//read_and_store<char[256]>(client_state + hazedumper::signatures::dwClientState_Map, current_map);

		radar_base = read<DWORD>(game_handle, client_base + hazedumper::signatures::dwRadarBase);
		radar_base = read<DWORD>(game_handle, radar_base + 0x78); // getting CCSGO_HudRadar

		if (surface->begin_scene()) {

			
			//aim(local_entity);
			trigger(local_entity);
			esp(surface, font, local_entity);

			surface->end_scene();
		}


		std::this_thread::sleep_for(std::chrono::milliseconds(2));
		system("cls");
	}

	
	CloseHandle(game_handle);
	return 0;
}


