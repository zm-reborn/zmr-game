#include "cbase.h"

#include "c_zmr_player.h"

#include "c_zmr_flashlightsystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


ConVar zm_cl_flashlight_expensive_max( "zm_cl_flashlight_expensive_max", "0", FCVAR_ARCHIVE );
ConVar zm_cl_flashlight_expensive_dist( "zm_cl_flashlight_expensive_dist", "300" );



//
//
//
CZMFlashlightSystem* GetZMFlashlightSystem()
{
    static CZMFlashlightSystem s;
    return &s;
}


//
//
//
C_ZMPlayer* ZMPotentialPlayer_t::GetPlayer() const
{
    return static_cast<C_ZMPlayer*>( UTIL_PlayerByIndex( entindex ) );
}


//
//
//
CZMFlashlightSystem::CZMFlashlightSystem() : CAutoGameSystemPerFrame( "ZMFlashlightSystem" ), m_vCurActiveExpensive( MAX_PLAYERS )
{
    m_flNextUpdate = 0.0f;
}

CZMFlashlightSystem::~CZMFlashlightSystem()
{
}

void CZMFlashlightSystem::LevelInitPostEntity()
{
    m_flNextUpdate = 0.0f;
    m_vCurActiveExpensive.RemoveAll();
}

void CZMFlashlightSystem::LevelShutdownPostEntity()
{
}

void CZMFlashlightSystem::Update( float delta )
{
    if ( m_flNextUpdate > gpGlobals->curtime )
        return;

    m_flNextUpdate = gpGlobals->curtime + (1.0f/10.0f);


    int numwanted = zm_cl_flashlight_expensive_max.GetInt();
    if ( numwanted < 1 )
    {
        Disable();
        return;
    }


    auto* pLocal = C_ZMPlayer::GetLocalPlayer();
    if ( !pLocal ) return;


    auto& mypos = pLocal->GetAbsOrigin();
    Vector myfwd;
    AngleVectors( pLocal->EyeAngles(), &myfwd );


    //
    // Find all potentially viable players that should get a flashlight.
    //
    float flMaxDist = zm_cl_flashlight_expensive_dist.GetFloat();
    flMaxDist *= flMaxDist;

    CUtlVector<ZMPotentialPlayer_t> newset;

    for ( int i = 1; i <= gpGlobals->maxClients; i++ )
    {
        auto* pPlayer = static_cast<C_ZMPlayer*>( UTIL_PlayerByIndex( i ) );

        if ( !pPlayer ) continue;

        if ( pPlayer == pLocal ) continue;

        if ( pPlayer->IsDormant() ) continue;

        // Is flashlight not on?
        if ( !pPlayer->IsEffectActive( EF_DIMLIGHT ) ) continue;


        if ( mypos.DistToSqr( pPlayer->GetAbsOrigin() ) > flMaxDist ) continue;


        Vector fwd;
        AngleVectors( pPlayer->EyeAngles(), &fwd );

        newset.AddToTail( { i, myfwd.Dot( fwd ) } );
    }

    // Sort them based on how close they are looking at our direction.
    newset.Sort([]( const ZMPotentialPlayer_t* p1, const ZMPotentialPlayer_t* p2 )
    {
        return p1->dotToLocalView > p2->dotToLocalView ? -1 : 1;
    });



    newset.SetCountNonDestructively( MIN( newset.Count(), numwanted ) );


    // Update old ones.
    for ( int i = 0; i < m_vCurActiveExpensive.Count(); i++ )
    {
        auto& active = m_vCurActiveExpensive[i];

        bool exists = false;

        int j = 0;
        for ( ; j < newset.Count(); j++ )
        {
            if ( newset[j].entindex == active.entindex )
            {
                exists = true;
                break;
            }
        }

        // This one shouldn't be here anymore.
        if ( !exists )
        {
            auto* pPlayer = active.GetPlayer();

            // Wait until we are completely done before opening our slot.
            if ( !pPlayer || !pPlayer->HasExpensiveFlashlightOn() )
            {
                m_vCurActiveExpensive.Remove( i );
                --i;
            }
            else
            {
                pPlayer->PreferExpensiveFlashlight( false );
            }
        }
        // We're in the set, just update.
        else
        {
            active.dotToLocalView = newset[j].dotToLocalView;

            newset.Remove( j );
        }
    }


    int freeslots = numwanted - m_vCurActiveExpensive.Count();

    int num = MIN( newset.Count(), freeslots );

    // Tell the new ones to prefer expensive flashlight.
    for ( int i = 0; i < num; i++ )
    {
        auto* pPlayer = newset[i].GetPlayer();
        pPlayer->PreferExpensiveFlashlight( true );
    }

    m_vCurActiveExpensive.AddMultipleToTail( num, newset.Base() );
}

void CZMFlashlightSystem::Disable()
{
    int len = m_vCurActiveExpensive.Count();
    if ( len > 0 )
    {
        for ( int i = 0; i < len; i++ )
        {
            auto* pPlayer = m_vCurActiveExpensive[i].GetPlayer();
            if ( pPlayer )
            {
                pPlayer->PreferExpensiveFlashlight( false );
            }
        }

        m_vCurActiveExpensive.RemoveAll();
    }
}

bool CZMFlashlightSystem::AddPlayerToExpensiveList( C_ZMPlayer* pPlayer )
{
    Assert( pPlayer );


    if ( FindPlayer( pPlayer->entindex() ) == -1 && GetNumExpensiveFlashlights() < GetExpensiveFlashlightLimit() )
    {
        m_vCurActiveExpensive.AddToTail( { pPlayer->entindex(), 0.0f } );

        return true;
    }


    return false;
}

int CZMFlashlightSystem::GetNumExpensiveFlashlights() const
{
    return m_vCurActiveExpensive.Count();
}

int CZMFlashlightSystem::GetExpensiveFlashlightLimit()
{
    return zm_cl_flashlight_expensive_max.GetInt();
}

int CZMFlashlightSystem::FindPlayer( int entindex ) const
{
    FOR_EACH_VEC( m_vCurActiveExpensive, i )
    {
        if ( m_vCurActiveExpensive[i].entindex == entindex )
            return i;
    }

    return -1;
}
