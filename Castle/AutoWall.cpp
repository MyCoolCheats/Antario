#include "autowall.h"
#include "Math.h"
double Autowall::GetHitgroupDamageMultiplier(HitGroups iHitGroup)
{
	switch (iHitGroup)
	{
	case HitGroups::HITGROUP_HEAD:
		return 4.0;
	case HitGroups::HITGROUP_CHEST:
	case HitGroups::HITGROUP_LEFTARM:
	case HitGroups::HITGROUP_RIGHTARM:
		return 1.0;
	case HitGroups::HITGROUP_STOMACH:
		return 1.25;
	case HitGroups::HITGROUP_LEFTLEG:
	case HitGroups::HITGROUP_RIGHTLEG:
		return 0.75;
	default:
		return 1.0;
	}
}

void Autowall::ScaleDamage(HitGroups hitgroup, C_BasePlayer* enemy, double weapon_armor_ratio, double& current_damage)
{
	current_damage *= Autowall::GetHitgroupDamageMultiplier(hitgroup);

	if (enemy->GetArmor() > 0)
	{
		if (hitgroup == HitGroups::HITGROUP_HEAD)
		{
			if (enemy->HasHelmet())
				current_damage *= weapon_armor_ratio * 0.75;
		}
		else
			current_damage *= weapon_armor_ratio * 0.5;
	}
}

bool Autowall::TraceToExit(Vector& end, trace_t* enter_trace, Vector start, Vector dir, trace_t* exit_trace)
{
	double distance = 0.0;

	while (distance <= 90.0)
	{
		distance += 4.0;
		end = start + dir * distance;

		auto point_contents = pTrace->GetPointContents(end, MASK_SHOT_HULL | CONTENTS_HITBOX, NULL);

		if (point_contents & MASK_SHOT_HULL && !(point_contents & CONTENTS_HITBOX))
			continue;

		auto new_end = end - (dir * 4.0);

		Ray_t ray;
		ray.Init(end, new_end);
		pTrace->TraceRay(ray, MASK_SHOT, 0, exit_trace);

		if (exit_trace->startsolid && exit_trace->surface.flags & SURF_HITBOX)
		{
			ray.Init(end, start);

			CTraceFilter filter;
			filter.pSkip = exit_trace->m_pEnt;

			pTrace->TraceRay(ray, 0x600400B, &filter, exit_trace);

			if ((exit_trace->fraction < 1.0 || exit_trace->allsolid) && !exit_trace->startsolid)
			{
				end = exit_trace->endpos;
				return true;
			}

			continue;
		}

		if (!(exit_trace->fraction < 1.0 || exit_trace->allsolid || exit_trace->startsolid) || exit_trace->startsolid)
		{
			if (exit_trace->m_pEnt)
			{
				if (enter_trace->m_pEnt && enter_trace->m_pEnt == pEntityList->GetClientEntity(0))
					return true;
			}

			continue;
		}

		if (exit_trace->surface.flags >> 7 & 1 && !(enter_trace->surface.flags >> 7 & 1))
			continue;

		if (exit_trace->plane.normal.Dot(dir) <= 1.0)
		{
			auto fraction = exit_trace->fraction * 4.0;
			end = end - (dir * fraction);

			return true;
		}
	}

	return false;
}

bool Autowall::HandleBulletPenetration(CCSWeaponInfo* weaponInfo, FireBulletData& data)
{
	surfacedata_t *enter_surface_data = pPhysics->GetSurfaceData(data.enter_trace.surface.surfaceProps);
	int enter_material = enter_surface_data->game.material;
	double enter_surf_penetration_mod = *(float*)((uint8_t*)enter_surface_data + 76);

	data.trace_length += data.enter_trace.fraction * data.trace_length_remaining;
	data.current_damage *= powf(weaponInfo->GetRangeModifier(), data.trace_length * 0.002);

	if (data.trace_length > 3000.0 || enter_surf_penetration_mod < 0.1)
		data.penetrate_count = 0;

	if (data.penetrate_count <= 0)
		return false;

	Vector dummy;
	trace_t trace_exit;

	if (!TraceToExit(dummy, &data.enter_trace, data.enter_trace.endpos, data.direction, &trace_exit))
		return false;

	surfacedata_t *exit_surface_data = pPhysics->GetSurfaceData(trace_exit.surface.surfaceProps);
	int exit_material = exit_surface_data->game.material;

	double exit_surf_penetration_mod = *(float*)((uint8_t*)exit_surface_data + 76);
	double final_damage_modifier = 0.16;
	double combined_penetration_modifier = 0.0;

	if ((data.enter_trace.contents & CONTENTS_GRATE) != 0 || enter_material == 89 || enter_material == 71)
	{
		combined_penetration_modifier = 3.0;
		final_damage_modifier = 0.05;
	}
	else
		combined_penetration_modifier = (enter_surf_penetration_mod + exit_surf_penetration_mod) * 0.5;

	if (enter_material == exit_material)
	{
		if (exit_material == 87 || exit_material == 85)
			combined_penetration_modifier = 3.0;
		else if (exit_material == 76)
			combined_penetration_modifier = 2.0;
	}

	double v34 = fmaxf(0.0, 1.0 / combined_penetration_modifier);
	double v35 = (data.current_damage * final_damage_modifier) + v34 * 3.0 * fmaxf(0.0, (3.0 / weaponInfo->GetPenetration()) * 1.25);
	double thickness = (trace_exit.endpos - data.enter_trace.endpos).Length();

	thickness *= thickness;
	thickness *= v34;
	thickness /= 24.0;

	double lost_damage = fmaxf(0.0f, v35 + thickness);

	if (lost_damage > data.current_damage)
		return false;

	if (lost_damage >= 0.0)
		data.current_damage -= lost_damage;

	if (data.current_damage < 1.0)
		return false;

	data.src = trace_exit.endpos;
	data.penetrate_count--;

	return true;
}

void TraceLine(Vector vecAbsStart, Vector vecAbsEnd, unsigned int mask, C_BasePlayer* ignore, trace_t* ptr)
{
	Ray_t ray;
	ray.Init(vecAbsStart, vecAbsEnd);
	CTraceFilter filter;
	filter.pSkip = ignore;

	pTrace->TraceRay(ray, mask, &filter, ptr);
}

bool Autowall::SimulateFireBullet(C_BaseCombatWeapon* pWeapon, bool teamCheck, FireBulletData& data)
{
	C_BasePlayer* localplayer = (C_BasePlayer*)pEntityList->GetClientEntity(pEngine->GetLocalPlayer());
	CCSWeaponInfo* weaponInfo = pWeapon->GetCSWpnData();

	data.penetrate_count = 4;
	data.trace_length = 0.0;
	data.current_damage = (float)weaponInfo->GetDamage();

	while (data.penetrate_count > 0 && data.current_damage >= 1.0)
	{
		data.trace_length_remaining = weaponInfo->GetRange() - data.trace_length;

		Vector end = data.src + data.direction * data.trace_length_remaining;

		// data.enter_trace
		// initial trace
		TraceLine(data.src, end, MASK_SHOT, localplayer, &data.enter_trace);

		Ray_t ray;
		ray.Init(data.src, end + data.direction * 40.0);

		pTrace->TraceRay(ray, MASK_SHOT, &data.filter, &data.enter_trace);
		
		// real man traces
		TraceLine(data.src, end, MASK_SHOT, localplayer, &data.enter_trace);
		TraceLine(data.src, end + data.direction * 40.0, MASK_SHOT, localplayer, &data.enter_trace);

		if (data.enter_trace.fraction == 1.0)
			break;

		if (data.enter_trace.hitgroup <= HitGroups::HITGROUP_RIGHTLEG && data.enter_trace.hitgroup > HitGroups::HITGROUP_GENERIC)
		{
			data.trace_length += data.enter_trace.fraction * data.trace_length_remaining;
			data.current_damage *= powf(weaponInfo->GetRangeModifier(), data.trace_length * 0.002);

			C_BasePlayer* player = (C_BasePlayer*)data.enter_trace.m_pEnt;
			if (teamCheck && player->GetTeam() == localplayer->GetTeam())
				return false;

			ScaleDamage(data.enter_trace.hitgroup, player, weaponInfo->GetWeaponArmorRatio(), data.current_damage);

			return true;
		}

		if (!HandleBulletPenetration(weaponInfo, data))
			break;
	}

	return false;
}

double Autowall::GetDamage(const Vector& point, bool teamCheck, FireBulletData& fData)
{
	double damage = 0.0;
	Vector dst = point;
	C_BasePlayer* localplayer = (C_BasePlayer*)pEntityList->GetClientEntity(pEngine->GetLocalPlayer());
	FireBulletData data;
	data.src = localplayer->GetEyePosition();
	data.filter.pSkip = localplayer;

	Vector angles = Math::CalcAngle(data.src, dst);
	Math::AngleVectors(angles, data.direction);

	data.direction.NormalizeInPlace();

	C_BaseCombatWeapon* activeWeapon = (C_BaseCombatWeapon*)pEntityList->GetClientEntityFromHandle(localplayer->GetActiveWeapon());
	if (!activeWeapon)
		return -1.0f;

	if (SimulateFireBullet(activeWeapon, teamCheck, data))
		damage = data.current_damage;

	fData = data;

	return damage;
}
