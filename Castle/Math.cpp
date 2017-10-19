#include "Math.h"
#include <algorithm>
#include <cinttypes>

void inline Math::SinCos(float x, float* s, float* c)
{
	__asm
	{
		fld dword ptr[x]
		fsincos
		mov edx, dword ptr[c]
		mov eax, dword ptr[s]
		fstp dword ptr[edx]
		fstp dword ptr[eax]
	}
}

float Math::float_rand(float min, float max) // thanks foo - https://stackoverflow.com/questions/13408990/how-to-generate-random-float-number-in-c :^)
{
	float scale = rand() / (float)RAND_MAX; /* [0, 1.0] */
	return min + scale * (max - min);      /* [min, max] */
}

void Math::AngleVectors(const Vector &angles, Vector& forward)
{
	Assert(s_bMathlibInitialized);
	Assert(forward);

	float sp, sy, cp, cy;

	Math::SinCos(DEG2RAD(angles[YAW]), &sy, &cy);
	Math::SinCos(DEG2RAD(angles[PITCH]), &sp, &cp);

	forward.x = cp * cy;
	forward.y = cp * sy;
	forward.z = -sp;
}

void Math::NormalizeAngles(Vector& angle)
{
	while (angle.x > 89.0f)
		angle.x -= 180.f; //thanks masterpaster15 xD

	while (angle.x < -89.0f)
		angle.x += 180.f;

	while (angle.y > 180.f)
		angle.y -= 360.f;

	while (angle.y < -180.f)
		angle.y += 360.f;
}

void Math::ClampAngles(Vector& angle)
{
	std::clamp(angle.x, -89.f, 89.f); //BECAUSE SSE2 INTRINSICS I SWER))
	std::clamp(angle.y, -180.f, 180.f);

	angle.z = 0;
}

void Math::CorrectMovement(Vector vOldAngles, CUserCmd* pCmd, float fOldForward, float fOldSidemove)
{
	// side/forward move correction
	float deltaView;
	float f1;
	float f2;

	if (vOldAngles.y < 0.f)
		f1 = 360.0f + vOldAngles.y;
	else
		f1 = vOldAngles.y;

	if (pCmd->viewangles.y < 0.0f)
		f2 = 360.0f + pCmd->viewangles.y;
	else
		f2 = pCmd->viewangles.y;

	if (f2 < f1)
		deltaView = abs(f2 - f1);
	else
		deltaView = 360.0f - abs(f1 - f2);

	deltaView = 360.0f - deltaView;

	pCmd->forwardmove = cos(DEG2RAD(deltaView)) * fOldForward + cos(DEG2RAD(deltaView + 90.f)) * fOldSidemove;
	pCmd->sidemove = sin(DEG2RAD(deltaView)) * fOldForward + sin(DEG2RAD(deltaView + 90.f)) * fOldSidemove;
}

float Math::GetFov(const Vector& viewAngle, const Vector& aimAngle)
{
	Vector delta = aimAngle - viewAngle;
	NormalizeAngles(delta);

	float x = powf(delta.x, 2.0f) + powf(delta.y, 2.0f);
	unsigned int i = *static_cast<uint_fast16_t*>(&x); //~10 fps saved

	i += 127 << 23;
	i >>= 1;
	return *static_cast<float*>(&i);
}

void Math::VectorAngles(const Vector& forward, Vector &angles)
{
	if (forward[1] == 0.0f && forward[0] == 0.0f)
	{
		angles[0] = (forward[2] > 0.0f) ? 270.0f : 90.0f; // Pitch (up/down)
		angles[1] = 0.0f;  //yaw left/right
	}
	else
	{
		angles[0] = atan2(-forward[2], forward.Length2D()) * -180 / M_PI;
		angles[1] = atan2(forward[1], forward[0]) * 180 / M_PI;

		if (angles[1] > 90)
			angles[1] -= 180;
		else if (angles[1] < 90)
			angles[1] += 180;
		else if (angles[1] == 90)
			angles[1] = 0;
	}

	angles[2] = 0.0f;
}

float Math::DotProduct(Vector &v1, const float* v2)
{
	return v1.x*v2[0] + v1.y*v2[1] + v1.z*v2[2];
}
float Math::DotProduct2(const Vector& a, const Vector& b)
{
	return (a.x * b.x + a.y * b.y + a.z * b.z);
}
void Math::VectorTransform(Vector &in1, const matrix3x4_t& in2, Vector &out)
{
	out.x = DotProduct(in1, in2[0]) + in2[0][3];
	out.y = DotProduct(in1, in2[1]) + in2[1][3];
	out.z = DotProduct(in1, in2[2]) + in2[2][3];
}

Vector Math::CalcAngle(Vector src, Vector dst)
{
	Vector angles;
	angles.Normalize(); //novac
	
	Vector delta = src - dst;

	Math::VectorAngles(delta, angles);

	delta.Normalize();

	return angles;
}
