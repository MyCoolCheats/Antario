#include "Features.h"

bool Settings::BHop::enabled = false;

void BHop::CreateMove(CUserCmd* cmd)
{
	if (!Settings::BHop::enabled)
		return;

	static bool last_jumped = false;
	static bool should_fake = false;

	C_BasePlayer* localplayer = (C_BasePlayer*)pEntityList->GetClientEntity(pEngine->GetLocalPlayer());

	if (!localplayer)
		return;

	if (localplayer->GetMoveType() == MOVETYPE_LADDER || localplayer->GetMoveType() == MOVETYPE_NOCLIP)
		return;

	if (!last_jumped && should_fake)
	{
		should_fake = false;
		cmd->buttons |= IN_JUMP;
	}
	else if (cmd->buttons & IN_JUMP)
	{
		if (localplayer->GetFlags() & FL_ONGROUND)
		{
			last_jumped = true;
			should_fake = true;
		}
		else
		{
			cmd->buttons &= ~IN_JUMP;
			last_jumped = false;
		}
	}
	else
	{
		last_jumped = false;
		should_fake = false;
	}
}
