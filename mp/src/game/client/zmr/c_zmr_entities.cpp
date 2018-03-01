#include "cbase.h"

#include "c_zmr_entities.h"
#include "zmr/zmr_player_shared.h"


/*
    Base simple
*/
IMPLEMENT_CLIENTCLASS_DT( C_ZMEntBaseSimple, DT_ZM_EntBaseSimple, CZMEntBaseSimple )
END_RECV_TABLE()

bool C_ZMEntBaseSimple::ShouldDraw()
{
    C_ZMPlayer* pPlayer = C_ZMPlayer::GetLocalPlayer();

    if ( !pPlayer ) return false;

    if ( !pPlayer->IsZM() )
    {
        return false;
    }

    return BaseClass::ShouldDraw();
}

#ifdef _DEBUG
void C_ZMEntBaseSimple::OnDataChanged( DataUpdateType_t type )
{
    BaseClass::OnDataChanged( type );

    if ( type == DATA_UPDATE_CREATED )
    {
        DevMsg( "ZM ent %i created!\n", entindex() );
    }
    else
    {
        DevMsg( "ZM ent %i changed! (%i)\n", entindex(), type );
    }
}
#endif

/*
    Base usable
*/
#define MAT_SPAWNSPRITE         "zmr_effects/orb_red"
#define MAT_MANISPRITE          "zmr_effects/orb_orange"


IMPLEMENT_CLIENTCLASS_DT( C_ZMEntBaseUsable, DT_ZM_EntBaseUsable, CZMEntBaseUsable )
END_RECV_TABLE()


C_ZMEntBaseUsable::C_ZMEntBaseUsable()
{
}

bool C_ZMEntBaseUsable::ShouldDraw()
{
    C_ZMPlayer* pPlayer = C_ZMPlayer::GetLocalPlayer();

    if ( !pPlayer ) return false;

    if ( !pPlayer->IsZM() )
    {
        return false;
    }

    return BaseClass::ShouldDraw();
}

int C_ZMEntBaseUsable::DrawModel( int flags )
{
    InitSpriteMat();

    
    static color32 clr = { 255, 255, 255, 255 };

	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->Bind( m_SpriteMat, this );

    DrawSprite( GetAbsOrigin(), 128, 128, clr );


    return BaseClass::DrawModel( flags );
}

/*
    Zombie spawn
*/
IMPLEMENT_CLIENTCLASS_DT( C_ZMEntZombieSpawn, DT_ZM_EntZombieSpawn, CZMEntZombieSpawn )
    RecvPropInt( RECVINFO( m_fZombieFlags ) ),
END_RECV_TABLE()

BEGIN_DATADESC( C_ZMEntZombieSpawn )
END_DATADESC()


C_ZMEntZombieSpawn::C_ZMEntZombieSpawn()
{
    m_fZombieFlags = 0;
}

void C_ZMEntZombieSpawn::Precache()
{
    BaseClass::Precache();

    PrecacheModel( MAT_SPAWNSPRITE );
}

void C_ZMEntZombieSpawn::InitSpriteMat()
{
    if ( m_SpriteMat == nullptr )
    {
        m_SpriteMat.Init( MAT_SPAWNSPRITE, TEXTURE_GROUP_CLIENT_EFFECTS );
    }
}


/*
    Manipulate
*/
IMPLEMENT_CLIENTCLASS_DT( C_ZMEntManipulate, DT_ZM_EntManipulate, CZMEntManipulate )
    RecvPropInt( RECVINFO( m_nCost ) ),
    RecvPropInt( RECVINFO( m_nTrapCost ) ),
    RecvPropString( RECVINFO( m_sDescription ) ),
END_RECV_TABLE()

BEGIN_DATADESC( C_ZMEntManipulate )
END_DATADESC()


C_ZMEntManipulate::C_ZMEntManipulate()
{
    m_nCost = 10;
    m_nTrapCost = 15;
    m_sDescription[0] = 0;
}

void C_ZMEntManipulate::Precache()
{
    BaseClass::Precache();

    PrecacheModel( MAT_MANISPRITE );
}

void C_ZMEntManipulate::InitSpriteMat()
{
    if ( m_SpriteMat == nullptr )
    {
        m_SpriteMat.Init( MAT_MANISPRITE, TEXTURE_GROUP_CLIENT_EFFECTS );
    }
}
