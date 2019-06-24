#include "cbase.h"
#include "animation.h"

#include "c_npcr_nonplayer.h"


C_NPCRNonPlayer::C_NPCRNonPlayer()
{
    m_bCurActivityLoops = false;
    m_iCurActivity = ACT_INVALID;
    m_iLastActivity = ACT_INVALID;
}

void C_NPCRNonPlayer::PostDataUpdate( DataUpdateType_t updateType )
{
    BaseClass::PostDataUpdate( updateType );

    if ( updateType == DATA_UPDATE_CREATED )
    {
        // Default to idle animation when spawning.
        SetActivity( ACT_IDLE );
    }
}

void C_NPCRNonPlayer::DispatchAnimEvents()
{
    // don't fire events if the framerate is 0
    if ( m_flPlaybackRate == 0.0f )
        return;


    CStudioHdr* hdr = GetModelPtr();

    if ( !hdr || !hdr->SequencesAvailable() )
    {
        return;
    }



    int iSeq = GetSequence();

    // Skip this altogether if there are no events
    if ( hdr->pSeqdesc( iSeq ).numevents == 0 )
    {
        return;
    }
        

    // look from when it last checked to some short time in the future	
    float flCycleRate = GetSequenceCycleRate( hdr, iSeq ) * m_flPlaybackRate;
    float flStart = m_flLastEventCheck;
    float flEnd = GetCycle();

    if ( !SequenceLoops() && m_bSequenceFinished )
    {
        flEnd = 1.01f;
    }


    //
    // FIXME: does not handle negative framerates!
    int index = 0;
    animevent_t	event;
    while ( (index = GetAnimationEvent( hdr, iSeq, &event, flStart, flEnd, index )) != 0 )
    {
        event.pSource = this;
        // calc when this event should happen
        if ( flCycleRate > 0.0f )
        {
            float flCycle = event.cycle;
            if ( flCycle > GetCycle() )
            {
                flCycle = flCycle - 1.0;
            }
            event.eventtime = m_flAnimTime + (flCycle - GetCycle()) / flCycleRate + GetAnimTimeInterval();
        }


        HandleAnimEvent( &event );
    }

    m_flLastEventCheck = flEnd;
}

void C_NPCRNonPlayer::HandleAnimEvent( animevent_t* pEvent )
{
}
