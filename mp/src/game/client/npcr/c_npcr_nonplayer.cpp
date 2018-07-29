#include "cbase.h"

#include "c_npcr_nonplayer.h"


C_NPCRNonPlayer::C_NPCRNonPlayer()
{
    m_bCurActivityLoops = false;
    m_iCurActivity = ACT_INVALID;
    m_iLastActivity = ACT_INVALID;
}
