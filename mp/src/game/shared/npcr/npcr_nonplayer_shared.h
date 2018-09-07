#pragma once


#ifdef CLIENT_DLL
#include "npcr/c_npcr_nonplayer.h"

#define CNPCRNonPlayer C_NPCRNonPlayer
#else
#include "npcr/npcr_nonplayer.h"
#endif

