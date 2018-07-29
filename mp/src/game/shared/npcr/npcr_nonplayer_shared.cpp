#include "cbase.h"
#include "animation.h"

#include "npcr_nonplayer_shared.h"


bool CNPCRNonPlayer::HasActivity( Activity act )
{
    CStudioHdr* hdr = GetModelPtr();
    if ( !hdr || !hdr->SequencesAvailable() )
    {
        return false;
    }

    VerifySequenceIndex( hdr );
    int iNewSeq = hdr->SelectWeightedSequence( act, GetSequence() );

    if ( iNewSeq <= -1 )
    {
        DevMsg( "Couldn't find sequence for activity %i!\n", act );
        return false;
    }

    return true;
}

bool CNPCRNonPlayer::SetActivity( Activity act )
{
    if ( act == m_iCurActivity && m_bCurActivityLoops )
        return true;


    CStudioHdr* hdr = GetModelPtr();
    if ( !hdr || !hdr->SequencesAvailable() )
    {
        DevMsg( "NPC doesn't have any sequences!\n" );
        return false;
    }

    
    RandomSeed( 1 );
    VerifySequenceIndex( hdr );
    int iNewSeq = hdr->SelectWeightedSequence( act, GetSequence() );

    if ( iNewSeq <= -1 )
    {
        DevMsg( "Couldn't find sequence for activity %i!\n", act );
        return false;
    }


    Activity last = m_iCurActivity;

#ifdef GAME_DLL
    if ( !IsSequenceFinished() )
        OnAnimActivityInterrupted( act );
#endif


    SetCycle( 0.0f );

    ResetSequence( iNewSeq );
    m_bCurActivityLoops = SequenceLoops();

    m_iCurActivity = act;
    m_iLastActivity = last;
#ifndef CLIENT_DLL
    m_iLastLoopActivity = ACT_INVALID;
#endif

    return true;
}