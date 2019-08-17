#include "cbase.h"
#include "animation.h"

#include "npcr_nonplayer_shared.h"


int CNPCRNonPlayer::GetAnimationRandomSeed()
{
    static int i = 0;
    return ( i = (++i % 50) );
}

bool CNPCRNonPlayer::HasActivity( Activity act )
{
    CStudioHdr* hdr = GetModelPtr();
    if ( !hdr || !hdr->SequencesAvailable() )
    {
        return false;
    }

    //VerifySequenceIndex( hdr );
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


    VerifySequenceIndex( hdr );
    
    RandomSeed( GetAnimationRandomSeed() ); // Don't use random->SetSeed, SelectWeightedSequence does not use it.
    int iNewSeq = hdr->SelectWeightedSequence( act, GetSequence() );

    if ( iNewSeq <= -1 )
    {
        DevMsg( "Couldn't find sequence for activity %i!\n", act );
        return false;
    }


    Activity last = m_iCurActivity;

#ifdef GAME_DLL
    // Interruptions should always be called
    // because it's possible for the activity to be interrupted
    // but then never finished
    //if ( !IsSequenceFinished() )
        OnAnimActivityInterrupted( act );
#endif


    SetCycle( 0.0f );

    ResetSequence( iNewSeq );
    m_bCurActivityLoops = SequenceLoops();

    m_iCurActivity = act;
    m_iLastActivity = last;

    return true;
}
