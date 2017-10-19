#include "Features.h"

bool Settings::ThirdPerson::enabled = false;
float Settings::ThirdPerson::distance = 30.f;
bool Settings::ThirdPerson::Fake;
bool Settings::ThirdPerson::Real;
void ThirdPerson::FrameStageNotify(ClientFrameStage_t stage)
{
	C_BasePlayer* localplayer = (C_BasePlayer*)pEntityList->GetClientEntity(pEngine->GetLocalPlayer());
	static bool Yeaman;
	if (pEngine->IsInGame() && localplayer && stage == ClientFrameStage_t::FRAME_RENDER_START)
	{
		ConVar* sv_hack = pCvar->FindVar("sv_cheats");
		SpoofedConvar* sv_hackerman = new SpoofedConvar(sv_hack); //prevent mem leak
		sv_hackerman->SetInt(1);
		
		if (Settings::ThirdPerson::enabled && localplayer->GetAlive())
		{
			if (!Yeaman)
			{
			pEngine->ExecuteClientCmd("thirdperson");
			Yeaman = true;
			}

				
			if(Settings::ThirdPerson::Fake || Settings::ThirdPerson::Real && Settings::AntiAim::Pitch::enabled || Settings::AntiAim::Yaw::enabled)
				*localplayer->GetVAngles() = lastTickViewAngles;
		}
		else
		{
			if (Yeaman)
			{
			pEngine->ExecuteClientCmd("firstperson");
			Yeaman = false;
			}
		}
	}
}
