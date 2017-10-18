#include "Features.h"

bool Settings::Autoblock::enabled = false;
ButtonCode_t Settings::Autoblock::key = ButtonCode_t::KEY_6;

void Autoblock::CreateMove(CUserCmd* cmd)
{
	if (!Settings::Autoblock::enabled)
		return;
	
	if (Settings::Autoblock::enabled)
		return;
	
	/*
	http://www.nationalbullyinghelpline.co.uk/
	BULLYING IS DISCRIMINATION
	BULLYING DOES NOT DISCRIMINATE
	*/
}
