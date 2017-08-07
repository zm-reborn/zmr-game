#include "cbase.h"

#include "team.h"
#include "items.h"
#include "props.h"
#include "physobj.h"


#include "zmr/zmr_gamerules.h"
#include "zmr/zmr_player.h"
#include "zmr/zmr_shareddefs.h"
#include "zmr/npcs/zmr_zombiebase.h"
#include "zmr/zmr_global_shared.h"
#include "zmr/weapons/zmr_base.h"

#include "zmr_entities.h"


static ConVar zm_sv_spawndelay( "zm_sv_spawndelay", "0.6", FCVAR_NOTIFY );


/*
    func_win
*/
class CZMEntWin : public CPointEntity
{
public:
	DECLARE_CLASS( CZMEntWin, CPointEntity )
    DECLARE_DATADESC()

    void Spawn( void ) OVERRIDE;

    void InputHumanWin( inputdata_t &inputdata );
    void InputHumanLose( inputdata_t &inputdata );
};

BEGIN_DATADESC( CZMEntWin )
    DEFINE_INPUTFUNC( FIELD_VOID, "Win", InputHumanWin ),
    DEFINE_INPUTFUNC( FIELD_VOID, "Lose", InputHumanLose ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( func_win, CZMEntWin );

void CZMEntWin::Spawn( void )
{ 
    SetSolid( SOLID_NONE );
    AddEffects( EF_NODRAW );
    
    SetMoveType( MOVETYPE_NONE );
}

void CZMEntWin::InputHumanWin( inputdata_t &inputdata )
{
    ZMRules()->EndRound( ZMROUND_HUMANWIN );
}

void CZMEntWin::InputHumanLose( inputdata_t &inputdata )
{
    ZMRules()->EndRound( ZMROUND_HUMANLOSE );
}


/*
    BaseUsable
*/
IMPLEMENT_SERVERCLASS_ST( CZMEntBaseUsable, DT_ZM_EntBaseUsable )
END_SEND_TABLE()

BEGIN_DATADESC( CZMEntBaseUsable )
    DEFINE_KEYFIELD( m_bActive, FIELD_BOOLEAN, "Active" ),

    DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),
    DEFINE_INPUTFUNC( FIELD_VOID, "Hide", InputHide ),
    DEFINE_INPUTFUNC( FIELD_VOID, "Unhide", InputUnhide ),
END_DATADESC()

void CZMEntBaseUsable::InputToggle( inputdata_t &inputdata )
{
    m_bActive = !m_bActive;
    UpdateUsable();
}

void CZMEntBaseUsable::InputHide( inputdata_t &inputdata )
{
    m_bActive = false;
    UpdateUsable();
}

void CZMEntBaseUsable::InputUnhide( inputdata_t &inputdata )
{
    m_bActive = true;
    UpdateUsable();
}

void CZMEntBaseUsable::Spawn( void )
{
    SetSolid( SOLID_NONE );
    AddSolidFlags( FSOLID_NOT_STANDABLE );

    UpdateUsable();
}

void CZMEntBaseUsable::UpdateUsable()
{
    if ( !m_bActive )
    {
        AddSolidFlags( FSOLID_NOT_SOLID );
        AddEffects( EF_NODRAW );
    }
    else
    {
        RemoveSolidFlags( FSOLID_NOT_SOLID );
        RemoveEffects( EF_NODRAW );
    }
}

/*
    BaseUsable
*/
IMPLEMENT_SERVERCLASS_ST( CZMEntBaseSimple, DT_ZM_EntBaseSimple )
END_SEND_TABLE()

BEGIN_DATADESC( CZMEntBaseSimple )
END_DATADESC()

/*
    Zombie spawn
*/
IMPLEMENT_SERVERCLASS_ST( CZMEntZombieSpawn, DT_ZM_EntZombieSpawn )
    SendPropInt( SENDINFO( m_fZombieFlags ) ),
END_SEND_TABLE()

BEGIN_DATADESC( CZMEntZombieSpawn )
    //DEFINE_KEYFIELD( m_bActive, FIELD_BOOLEAN, "Active" ),

    //DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),
    //DEFINE_INPUTFUNC( FIELD_VOID, "Hide", InputHide ),
    //DEFINE_INPUTFUNC( FIELD_VOID, "Unhide", InputUnhide ),


    DEFINE_KEYFIELD( m_fZombieFlags, FIELD_INTEGER, "zombieflags" ),
    //DEFINE_KEYFIELD( m_nSpawnQueueCapacity, FIELD_INTEGER, "spawnqueuecapacity" ),
    DEFINE_KEYFIELD( m_sRallyName, FIELD_STRING, "rallyname" ),
    DEFINE_KEYFIELD( m_sFirstNodeName, FIELD_STRING, "nodename" ),

    DEFINE_THINKFUNC( SpawnThink ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( info_zombiespawn, CZMEntZombieSpawn );


CZMEntZombieSpawn::CZMEntZombieSpawn()
{
    m_vSpawnNodes.Purge();
    m_vSpawnQueue.Purge();
}

void CZMEntZombieSpawn::Precache( void )
{
    PrecacheModel( "models/zombiespawner.mdl" );
}

void CZMEntZombieSpawn::Spawn( void )
{
    BaseClass::Spawn();

    Precache();
    SetModel( "models/zombiespawner.mdl" );


    //if ( m_nSpawnQueueCapacity < 1 )
    //    m_nSpawnQueueCapacity = 10;


    m_vSpawnQueue.EnsureCapacity( 10 );

    StopSpawning();

    
    CBaseEntity* pEnt = nullptr;
    while ( m_pRallyPoint == nullptr && (pEnt = gEntList.FindEntityByName( pEnt, m_sRallyName )) != nullptr )
    {
        m_pRallyPoint = dynamic_cast<CZMEntRallyPoint*>( pEnt );
    }
}

void CZMEntZombieSpawn::InputToggle( inputdata_t &inputdata )
{
    BaseClass::InputToggle( inputdata );

    if ( IsActive() ) StartSpawning();
    else StopSpawning();
}

void CZMEntZombieSpawn::InputHide( inputdata_t &inputdata )
{
    BaseClass::InputHide( inputdata );

    StopSpawning();
}

void CZMEntZombieSpawn::InputUnhide( inputdata_t &inputdata )
{
    BaseClass::InputUnhide( inputdata );

    StartSpawning();
}

bool CZMEntZombieSpawn::CanSpawn( ZombieClass_t zclass )
{
    return (GetZombieFlags() == 0 || (GetZombieFlags() & (1 < zclass)) );
}

bool CZMEntZombieSpawn::QueueUnit( CZMPlayer* pPlayer, ZombieClass_t zclass, int amount )
{
    if ( !CZMBaseZombie::IsValidClass( zclass ) )
    {
        Warning( "Attempted to spawn an invalid unit in queue (%i)!\n", zclass );
        return false;
    }


    if ( m_vSpawnQueue.Count() >= 10 )
        return false;

    if ( !CanSpawn( zclass ) ) return false;


    queue_info_t queue;

    queue.m_zclass = zclass;
    queue.m_iSpawnerIndex = ( pPlayer ) ? pPlayer->entindex() : 0;


    int left = 10 - m_vSpawnQueue.Count();
    
    if ( amount > left ) amount = left;
    

    for ( int i = 0; i < amount; i++ )
        m_vSpawnQueue.AddToTail( queue );


    StartSpawning();


    SendMenuUpdate();

    return true;
}

void CZMEntZombieSpawn::QueueClear( int inamount, int inpos )
{
    if ( !m_vSpawnQueue.Count() ) return;


    int pos = inpos;
    if ( pos < 0 || pos >= m_vSpawnQueue.Count() ) pos = m_vSpawnQueue.Count() - 1;


    int amount = inamount;
    if ( amount < 1 || amount > m_vSpawnQueue.Count() ) amount = m_vSpawnQueue.Count();


    if ( (pos + amount) > m_vSpawnQueue.Count() )
    {
        amount = m_vSpawnQueue.Count() - pos;
    }

    m_vSpawnQueue.RemoveMultiple( pos, amount );

    SendMenuUpdate();
}

void CZMEntZombieSpawn::StartSpawning()
{
    SetThink( &CZMEntZombieSpawn::SpawnThink );
    SetNextSpawnThink();
}

void CZMEntZombieSpawn::SetNextSpawnThink()
{
    // ZMRTODO: See if this causes any problems.
    // Only set next spawn think if we're not already thinking.
    if ( GetNextThink() == TICK_NEVER_THINK || (GetNextThink() - gpGlobals->curtime) > zm_sv_spawndelay.GetFloat() )
    {
        SetNextThink( gpGlobals->curtime + zm_sv_spawndelay.GetFloat() );
    }
}

void CZMEntZombieSpawn::StopSpawning()
{
    SetThink( nullptr );
    SetNextThink( TICK_NEVER_THINK );
}

void CZMEntZombieSpawn::SendMenuUpdate()
{
	CRecipientFilter filter;
	filter.MakeReliable();

    CZMPlayer* pZM;
    for ( int i = 1; i <= gpGlobals->maxClients; i++ )
    {
        pZM = ToZMPlayer( UTIL_PlayerByIndex( i ) );

        if ( pZM && pZM->IsZM() && pZM->GetBuildSpawn() == this )
        {
            filter.AddRecipient( pZM );
        }
    }

    if ( !filter.GetRecipientCount() ) return;


	UserMessageBegin( filter, "ZMBuildMenuUpdate" );
		WRITE_SHORT( entindex() );

		WRITE_BOOL( false ); // Force open

		for ( int i=0; i < 10; i++ )
		{
			//have to increment by 1 so that type_invalid fits into the unsigned byte
			if ( m_vSpawnQueue.IsValidIndex( i ) )
			{
				WRITE_BYTE( m_vSpawnQueue[i].m_zclass + 1 );
			}
			else
			{
				WRITE_BYTE( ZMCLASS_INVALID + 1 );
			}
		}
	MessageEnd();
}

void CZMEntZombieSpawn::SetRallyPoint( const Vector& pos )
{
    if ( !m_pRallyPoint )
    {
        m_pRallyPoint = dynamic_cast<CZMEntRallyPoint*>( CBaseEntity::Create( "info_rallypoint", pos, vec3_angle, this ) );

        if ( !m_pRallyPoint )
        {
            Warning( "Unable to create rally point for zombie spawn %i!\n", entindex() );
        }

        return;
    }

    m_pRallyPoint->Teleport( &pos, nullptr, nullptr );
}

void CZMEntZombieSpawn::SpawnThink()
{
    if ( m_vSpawnQueue.Count() < 1 || !IsActive() )
    {
        StopSpawning();
        return;
    }

    queue_info_t queue = m_vSpawnQueue.Element( 0 );

    ZombieClass_t zclass = queue.m_zclass;


    int cost = CZMBaseZombie::GetCost( zclass );
    bool bCreate = true;
    CZMPlayer* pPlayer = nullptr;

    if ( queue.m_iSpawnerIndex > 0 )
    {
        pPlayer = ToZMPlayer( UTIL_PlayerByIndex( queue.m_iSpawnerIndex ) );

        if ( pPlayer && !pPlayer->HasEnoughRes( cost ) )
            bCreate = false;
    }

    if ( bCreate && !CZMBaseZombie::HasEnoughPopToSpawn( zclass ) )
        bCreate = false;

    
    if ( bCreate && CreateZombie( zclass ) )
    {
        m_vSpawnQueue.Remove( 0 );

        if ( pPlayer )
        {
            pPlayer->SetResources( pPlayer->GetResources() - cost );
        }


        // Tell everyone that has this menu open that a zombie has been spawned.
        SendMenuUpdate();
    }


    SetNextSpawnThink();
}

bool CZMEntZombieSpawn::CreateZombie( ZombieClass_t zclass )
{
    const char* classname = CZMBaseZombie::ClassToName( zclass );

    if ( !classname )
    {
        Warning( "Can't create zombie of class %i!\n", zclass );
        return false;
    }


    CZMBaseZombie* pZombie = static_cast<CZMBaseZombie*>( CreateEntityByName( classname ) );

    if ( !pZombie ) return false;


    Vector spawnpos;
    QAngle ang = vec3_angle;

    if ( !FindSpawnPoint( pZombie, spawnpos, ang ) )
    {
        pZombie->SUB_Remove();
        return false;
    }


    pZombie->Teleport( &spawnpos, &ang, &vec3_origin );

    UTIL_DropToFloor( pZombie, MASK_NPCSOLID );


    // We can be marked for deletion...
    if ( DispatchSpawn( pZombie ) != 0 )
    {
        pZombie->SUB_Remove();
        return false;
    }

    pZombie->Activate();


    if ( m_pRallyPoint )
    {
        Msg( "Commanding zombie to rallypoint...\n" );
        pZombie->Command( m_pRallyPoint->GetAbsOrigin() );
    }
    

    return true;
}

bool CZMEntZombieSpawn::FindSpawnPoint( CZMBaseZombie* pZombie, Vector& outpos, QAngle& outang )
{
    if ( !pZombie ) return false;


    // We have no spawn nodes, so find em.
    if ( !m_vSpawnNodes.Count() )
    {
        CZMEntSpawnNode* temp;
        bool recursive = false;

        CZMEntSpawnNode* start = nullptr;

        CBaseEntity* pEnt = nullptr;
        while ( start == nullptr && (pEnt = gEntList.FindEntityByName( pEnt, m_sFirstNodeName )) != nullptr )
        {
            start = dynamic_cast<CZMEntSpawnNode*>( pEnt );
        }

        if ( start )
        {
            pEnt = nullptr;

            CZMEntSpawnNode* next = start;

            m_vSpawnNodes.AddToTail( next );

            while ( (pEnt = gEntList.FindEntityByName( pEnt, next->m_sNextNodeName )) != nullptr )
            {
                temp = dynamic_cast<CZMEntSpawnNode*>( pEnt );

                if ( temp )
                {
                    // Check for recursion.
                    for ( int i = 0; i < m_vSpawnNodes.Count(); i++ )
                    {
                            if ( temp == m_vSpawnNodes.Element( i ) )
                            {
                                recursive = true;
                                break;
                            }
                    }

                    if ( recursive )
                    {
                        Warning( "Detected recursion in spawn nodes!" );
                        break;
                    }

                    pEnt = nullptr;
                    next = temp;

                    m_vSpawnNodes.AddToTail( next );
                }
            }
        }
    }

    
    if ( m_vSpawnNodes.Count() )
    {
        CZMEntSpawnNode* pNode = nullptr;

        // Try random node.
        pNode = m_vSpawnNodes.Element( random->RandomInt( 0, m_vSpawnNodes.Count() - 1 ) );

        if ( pZombie->CanSpawn( pNode->GetAbsOrigin() ) )
        {
            outpos = pNode->GetAbsOrigin();
            outang = pNode->GetAbsAngles();
            return true;
        }

        // Try any node.
        for ( int i = 0; i < m_vSpawnNodes.Count(); i++ )
        {
            pNode = m_vSpawnNodes.Element( i );

            if ( pZombie->CanSpawn( pNode->GetAbsOrigin() ) )
            {
                outpos = pNode->GetAbsOrigin();
                outang = pNode->GetAbsAngles();
                return true;
            }
        }
    }

    // Isn't working, spawn in a radius.
    // Have to do this since some maps designed their spawning around it.
    Vector pos;

    float radius = 128.0f;

    for ( int i = 0; i < 25; i++ )
    {
        pos = GetAbsOrigin();
        pos.x += random->RandomFloat( -radius, radius );
        pos.y += random->RandomFloat( -radius, radius );

        if ( pZombie->CanSpawn( pos ) )
        {
            outpos = pos;
            return true;
        }
    }

    return false;
}


/*
    Spawnnode
*/
BEGIN_DATADESC( CZMEntSpawnNode )
    DEFINE_KEYFIELD( m_sNextNodeName, FIELD_STRING, "nodename" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( info_spawnnode, CZMEntSpawnNode );

void CZMEntSpawnNode::Precache( void )
{
    PrecacheModel( "models/spawnnode.mdl" );
}

void CZMEntSpawnNode::Spawn( void )
{
    Precache();
    SetModel( "models/spawnnode.mdl" );

    SetSolid( SOLID_NONE );
}


/*
    Rallypoint
*/
BEGIN_DATADESC( CZMEntRallyPoint )
END_DATADESC()

LINK_ENTITY_TO_CLASS( info_rallypoint, CZMEntRallyPoint );

void CZMEntRallyPoint::Precache( void )
{
    PrecacheModel( "models/rallypoint.mdl" );
}

void CZMEntRallyPoint::Spawn( void )
{ 
    Precache();
    SetModel( "models/rallypoint.mdl" );

    SetSolid( SOLID_NONE );
}


/*
    Trap trigger
*/
//IMPLEMENT_SERVERCLASS_ST( CZMEntManipulateTrigger, DT_ZM_EntManipulateTrigger )
//END_SEND_TABLE()

BEGIN_DATADESC( CZMEntManipulateTrigger )
    DEFINE_THINKFUNC( ScanThink ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( info_manipulate_trigger, CZMEntManipulateTrigger );

CZMEntManipulateTrigger::CZMEntManipulateTrigger()
{

}

CZMEntManipulateTrigger::~CZMEntManipulateTrigger()
{
    CZMEntManipulate* pOwner = dynamic_cast<CZMEntManipulate*>( GetOwnerEntity() );

    if ( pOwner )
    {
        pOwner->RemoveTrigger( this );
    }
}

void CZMEntManipulateTrigger::Precache( void )
{
    PrecacheModel( "models/trap.mdl" );
}

void CZMEntManipulateTrigger::Spawn( void )
{ 
    Precache();
    SetModel( "models/trap.mdl" );

    SetSolid( SOLID_NONE );

    SetThink( &CZMEntManipulateTrigger::ScanThink );
    SetNextThink( gpGlobals->curtime + 0.5f );
}

ConVar zm_sv_trap_triggerrange( "zm_sv_trap_triggerrange", "100", FCVAR_NOTIFY );

void CZMEntManipulateTrigger::ScanThink( void )
{
    CBaseEntity* pEnt = nullptr;
    while ( (pEnt = gEntList.FindEntityInSphere( pEnt, GetAbsOrigin(), zm_sv_trap_triggerrange.GetFloat() )) != nullptr )
    {
        CZMPlayer* pPlayer = ToZMPlayer( pEnt );

        if ( pPlayer && pPlayer->IsHuman() && pPlayer->IsAlive() )
        {
            CZMEntManipulate* pOwner = dynamic_cast<CZMEntManipulate*>( GetOwnerEntity() );

            if ( pOwner )
            {
                pOwner->Trigger( this );

                UTIL_Remove( this );
            }
        }
    }

    SetNextThink( gpGlobals->curtime + 0.5f );
}

/*
    Trap
*/
IMPLEMENT_SERVERCLASS_ST( CZMEntManipulate, DT_ZM_EntManipulate )
    SendPropInt( SENDINFO( m_nCost ) ),
    SendPropInt( SENDINFO( m_nTrapCost ) ),
END_SEND_TABLE()

BEGIN_DATADESC( CZMEntManipulate )
    DEFINE_KEYFIELD( m_sDescription, FIELD_STRING, "description" ),
    DEFINE_KEYFIELD( m_nCost, FIELD_INTEGER, "Cost" ),
    DEFINE_KEYFIELD( m_nTrapCost, FIELD_INTEGER, "TrapCost" ),
    DEFINE_KEYFIELD( m_bRemoveOnTrigger, FIELD_BOOLEAN, "RemoveOnTrigger" ),

    DEFINE_OUTPUT( m_OnPressed, "OnPressed" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( info_manipulate, CZMEntManipulate );


CZMEntManipulate::CZMEntManipulate()
{
    m_vTriggers.Purge();
    //m_sDescription[0] = 0;
}

CZMEntManipulate::~CZMEntManipulate()
{
    RemoveTriggers();
}

void CZMEntManipulate::Precache( void )
{
    PrecacheModel( "models/manipulatable.mdl" );

    UTIL_PrecacheOther( "info_manipulate_trigger" );
}

void CZMEntManipulate::Spawn( void )
{
    BaseClass::Spawn();


    if ( m_nCost <= 0 ) m_nCost = 10;
    if ( m_nTrapCost <= 0 ) m_nTrapCost = m_nCost * 1.5;


    Precache();
    SetModel( "models/manipulatable.mdl" );
}

void CZMEntManipulate::InputToggle( inputdata_t &inputdata )
{
    BaseClass::InputToggle( inputdata );

    if ( !IsActive() )
    {
        RemoveTriggers();
    }
}

void CZMEntManipulate::InputHide( inputdata_t &inputdata )
{
    BaseClass::InputHide( inputdata );

    RemoveTriggers();
}

void CZMEntManipulate::InputUnhide( inputdata_t &inputdata )
{
    BaseClass::InputUnhide( inputdata );
}

void CZMEntManipulate::Trigger( CBaseEntity* pActivator )
{
    if ( !IsActive() ) return;


    CZMEntManipulateTrigger* pTrigger = dynamic_cast<CZMEntManipulateTrigger*>( pActivator );
    if ( !pTrigger && pActivator->IsPlayer() )
    {
        CZMPlayer* pPlayer = ToZMPlayer( pActivator );

        if ( pPlayer )
        {
            if ( pPlayer->GetResources() >= GetCost() )
            {
                pPlayer->SetResources( pPlayer->GetResources() - GetCost() );
            }
            else // Not enough res!
            {
                return;
            }
        }
    }


    m_OnPressed.FireOutput( pActivator, this );




    RemoveTriggers();

    if ( m_bRemoveOnTrigger )
    {
        UTIL_Remove( this );
    }
}

void CZMEntManipulate::CreateTrigger( const Vector& pos )
{
    CZMEntManipulateTrigger* pTrigger = dynamic_cast<CZMEntManipulateTrigger*>( CBaseEntity::Create( "info_manipulate_trigger", pos, vec3_angle, nullptr ) );

    if ( !pTrigger ) return;


    pTrigger->SetOwnerEntity( this );

    m_vTriggers.AddToTail( pTrigger );
}

void CZMEntManipulate::RemoveTrigger( CZMEntManipulateTrigger* pTrigger )
{
    int i = m_vTriggers.Find( pTrigger );
    if ( i < 0 ) return;


    m_vTriggers.Remove( i );

    UTIL_Remove( pTrigger );
}

void CZMEntManipulate::RemoveTriggers()
{
    CZMEntManipulateTrigger* trigger;

    for ( int i = 0; i < m_vTriggers.Count(); i++ )
    {
        trigger = m_vTriggers[i];
        m_vTriggers.Remove( i );
        --i;

        UTIL_Remove( trigger );
    }

    m_vTriggers.Purge();
}


/*
    Player count trigger
*/
class CZMEntTriggerPlayerCount : public CBaseTrigger
{
public:
    DECLARE_CLASS( CZMEntTriggerPlayerCount, CBaseTrigger )
    DECLARE_DATADESC()


    void Spawn( void );

    void CountThink( void );


    void InputToggle( inputdata_t &inputData );
    void InputDisable( inputdata_t &inputData );
    void InputEnable( inputdata_t &inputData );

    COutputEvent m_OnCount;


    void UpdateState( bool );
    inline bool IsActive() { return m_bActive; };

private:
    int m_iPercentageToFire;
    bool m_bActive;
};

BEGIN_DATADESC( CZMEntTriggerPlayerCount )
    DEFINE_KEYFIELD( m_iPercentageToFire, FIELD_INTEGER, "percentagetofire" ), // It just has to be an integer...
    DEFINE_KEYFIELD( m_bActive, FIELD_BOOLEAN, "Active" ),

    DEFINE_THINKFUNC( CountThink ),


    DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),
    DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
    DEFINE_INPUTFUNC( FIELD_VOID, "Enabled", InputEnable ),

    DEFINE_OUTPUT( m_OnCount, "OnCount" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_playercount, CZMEntTriggerPlayerCount );


void CZMEntTriggerPlayerCount::Spawn( void )
{
    BaseClass::Spawn();

    InitTrigger();

    SetThink( &CZMEntTriggerPlayerCount::CountThink );
    SetNextThink( gpGlobals->curtime );
}

void CZMEntTriggerPlayerCount::InputToggle( inputdata_t &inputData )
{
    UpdateState( !m_bActive );
}

void CZMEntTriggerPlayerCount::InputEnable( inputdata_t &inputData )
{
    UpdateState( true );
}

void CZMEntTriggerPlayerCount::InputDisable( inputdata_t &inputData )
{
    UpdateState( false );
}

void CZMEntTriggerPlayerCount::UpdateState( bool newstate )
{
    if ( m_bActive == newstate ) return;


    m_bActive = newstate;

    if ( m_bActive )
    {
        SetThink( &CZMEntTriggerPlayerCount::CountThink );
        SetNextThink( gpGlobals->curtime + 1.0f );
    }
    else
    {
        SetNextThink( TICK_NEVER_THINK );
    }
}

void CZMEntTriggerPlayerCount::CountThink( void )
{
    if ( !IsActive() ) return;


    int count = 0;

    touchlink_t* root = static_cast<touchlink_t*>( GetDataObject( TOUCHLINK ) );

    if ( !root )
    {
        DevMsg( "No root found for trigger_playercount!\n" );
        return;
    }


    for ( touchlink_t* link = root->nextLink; link != root; link = link->nextLink )
    {
        CZMPlayer* pPlayer = dynamic_cast<CZMPlayer*>( link->entityTouched.Get() );

        if ( pPlayer && pPlayer->IsHuman() && pPlayer->IsAlive() ) ++count;
    }


    CTeam* team = GetGlobalTeam( ZMTEAM_HUMAN );

    if ( !team )
    {
        DevMsg( "No human team found! Can't count players! (trigger_playercount)\n" );
        return;
    }


    int totalplayers = team->GetNumPlayers();

    if ( totalplayers > 0 )
    {
        float p = count / (float)totalplayers * 100.0f;

        if ( p >= m_iPercentageToFire )
        {
            m_OnCount.FireOutput( nullptr, this );
        }
    }

    SetNextThink( gpGlobals->curtime + 1.0f );
}

/*
    Entity count trigger
*/
class CZMEntTriggerEntityCount : public CBaseTrigger
{
public:
    DECLARE_CLASS( CZMEntTriggerEntityCount, CBaseTrigger )
    DECLARE_DATADESC()


    void Spawn( void );

    void InputCount( inputdata_t &inputData );
    void InputToggle( inputdata_t &inputData );
    void InputDisable( inputdata_t &inputData );
    void InputEnable( inputdata_t &inputData );


    COutputEvent m_OnCount;
    COutputEvent m_OnNotCount;


    void UpdateState( bool );
    inline bool IsActive() { return m_bActive; };

private:
    int m_iCountToFire;
    int m_iTriggerFlags;
    bool m_bActive;
};

#define SF_ENTCOUNT_PLAYERS     1 // Humans, in our case.
#define SF_ENTCOUNT_NPCS        2
#define SF_ENTCOUNT_PROPS       3

BEGIN_DATADESC( CZMEntTriggerEntityCount )
    DEFINE_KEYFIELD( m_iCountToFire, FIELD_INTEGER, "counttofire" ), // It just has to be an integer...
    DEFINE_KEYFIELD( m_iTriggerFlags, FIELD_INTEGER, "triggerflags" ),
    DEFINE_KEYFIELD( m_bActive, FIELD_BOOLEAN, "Active" ),

    DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),
    DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
    DEFINE_INPUTFUNC( FIELD_VOID, "Enabled", InputEnable ),

    DEFINE_INPUTFUNC( FIELD_VOID, "Count", InputCount ),

    DEFINE_OUTPUT( m_OnCount, "OnCount" ),
    DEFINE_OUTPUT( m_OnNotCount, "OnNotCount" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_entitycount, CZMEntTriggerEntityCount );


void CZMEntTriggerEntityCount::Spawn( void )
{
    BaseClass::Spawn();

    if ( m_iTriggerFlags < SF_ENTCOUNT_PLAYERS || m_iTriggerFlags > SF_ENTCOUNT_PROPS )
    {
        Warning( "trigger_entitycount has an invalid trigger flag %i!\n", m_iTriggerFlags );
    }

    InitTrigger();
}

void CZMEntTriggerEntityCount::InputToggle( inputdata_t &inputData )
{
    m_bActive = !m_bActive;
}

void CZMEntTriggerEntityCount::InputEnable( inputdata_t &inputData )
{
    m_bActive = true;
}

void CZMEntTriggerEntityCount::InputDisable( inputdata_t &inputData )
{
    m_bActive = false;
}

void CZMEntTriggerEntityCount::InputCount( inputdata_t &inputData )
{
    if ( !IsActive() ) return;


    int count = 0;

    touchlink_t* root = static_cast<touchlink_t*>( GetDataObject( TOUCHLINK ) );

    if ( !root )
    {
        DevMsg( "No root found for trigger_entitycount!\n" );
        return;
    }


    for ( touchlink_t* link = root->nextLink; link != root; link = link->nextLink )
    {
        CBaseEntity* pEnt = link->entityTouched.Get();

        if ( !pEnt ) continue;

        
        switch ( m_iTriggerFlags )
        {
        case SF_ENTCOUNT_PLAYERS :
        {
            CZMPlayer* pPlayer = ToZMPlayer( pEnt );

            if ( pPlayer && pPlayer->IsHuman() )
            {
                ++count;
            }

            break;
        }
        case SF_ENTCOUNT_NPCS :
        {
            CAI_BaseNPC* pNPC = dynamic_cast<CAI_BaseNPC*>( pEnt );

            if ( pNPC && pNPC->IsAlive() )
            {
                ++count;
            }

            break;
        }
        case SF_ENTCOUNT_PROPS :
        {
            CPhysicsProp* pProp = dynamic_cast<CPhysicsProp*>( pEnt );

            if ( pProp )
            {
                ++count;
                break;
            }

            CPhysBox* pBrush = dynamic_cast<CPhysBox*>( pEnt );

            if ( pBrush ) ++count;

            break;
        }
        default : break;
        }
    }


    if ( count >= m_iCountToFire )
    {
        m_OnCount.FireOutput( inputData.pActivator, this );
    }
    else
    {
        m_OnNotCount.FireOutput( inputData.pActivator, this );
    }
}

/*
    Loadout
*/
BEGIN_DATADESC( CZMEntLoadout )
    DEFINE_KEYFIELD( m_iMethod, FIELD_INTEGER, "Method" ),
    DEFINE_KEYFIELD( m_iCounts[CZMEntLoadout::LO_PISTOL], FIELD_INTEGER, "Pistols" ),
    DEFINE_KEYFIELD( m_iCounts[CZMEntLoadout::LO_SHOTGUN], FIELD_INTEGER, "Shotguns" ),
    DEFINE_KEYFIELD( m_iCounts[CZMEntLoadout::LO_RIFLE], FIELD_INTEGER, "Rifles" ),
    DEFINE_KEYFIELD( m_iCounts[CZMEntLoadout::LO_MAC10], FIELD_INTEGER, "Mac10s" ),
    DEFINE_KEYFIELD( m_iCounts[CZMEntLoadout::LO_MOLOTOV], FIELD_INTEGER, "Molotovs" ),
    DEFINE_KEYFIELD( m_iCounts[CZMEntLoadout::LO_SLEDGE], FIELD_INTEGER, "Sledgehammers" ),
    DEFINE_KEYFIELD( m_iCounts[CZMEntLoadout::LO_IMPROVISED], FIELD_INTEGER, "Improvised" ),
    DEFINE_KEYFIELD( m_iCounts[CZMEntLoadout::LO_REVOLVER], FIELD_INTEGER, "Revolvers" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( info_loadout, CZMEntLoadout );

CZMEntLoadout::CZMEntLoadout()
{
}

CZMEntLoadout::~CZMEntLoadout()
{
    CZMRules* pRules = ZMRules();

    if ( pRules )
    {
        pRules->SetLoadoutEnt( nullptr );
    }
}

void CZMEntLoadout::Spawn( void )
{
    SetSolid( SOLID_NONE );
    AddEffects( EF_NODRAW );
    SetMoveType( MOVETYPE_NONE );


    // Make sure we don't give out negative weapons, lul.
    for ( int i = 0; i < LO_MAX; i++ )
    {
        if ( m_iCounts[i] < 0 ) m_iCounts[i] = 0;
    }


    Reset();


    CZMRules* pRules = ZMRules();

    if ( pRules )
    {
        pRules->SetLoadoutEnt( this );
    }
}

void CZMEntLoadout::Reset()
{
    switch ( GetMethod() )
    {
    case LOMETHOD_CATEGORY : // My favorite...
    {
        for ( int i = 0; i < LOCAT_MAX; i++ )
        {
            m_vCurCat[i].Purge();
        }

        for ( int i = 0; i < m_iCounts[LO_IMPROVISED]; i++)
            m_vCurCat[LOCAT_MELEE].AddToTail( LO_IMPROVISED );

        for ( int i = 0; i < m_iCounts[LO_SLEDGE]; i++)
            m_vCurCat[LOCAT_MELEE].AddToTail( LO_SLEDGE );


        
        for ( int i = 0; i < m_iCounts[LO_PISTOL]; i++)
            m_vCurCat[LOCAT_PISTOL].AddToTail( LO_PISTOL );

        for ( int i = 0; i < m_iCounts[LO_REVOLVER]; i++)
            m_vCurCat[LOCAT_PISTOL].AddToTail( LO_REVOLVER );



        for ( int i = 0; i < m_iCounts[LO_MAC10]; i++)
            m_vCurCat[LOCAT_LARGE].AddToTail( LO_MAC10 );

        for ( int i = 0; i < m_iCounts[LO_SHOTGUN]; i++)
            m_vCurCat[LOCAT_LARGE].AddToTail( LO_SHOTGUN );

        for ( int i = 0; i < m_iCounts[LO_RIFLE]; i++)
            m_vCurCat[LOCAT_LARGE].AddToTail( LO_RIFLE );



        for ( int i = 0; i < m_iCounts[LO_MOLOTOV]; i++)
            m_vCurCat[LOCAT_EQUIPMENT].AddToTail( LO_MOLOTOV );

        break;

    }
    case LOMETHOD_RANDOM :
    {
        for ( int i = 0; i < LO_MAX; i++ )
        {
            m_iCurRandom[i] = m_iCounts[i];
        }
        break;

    }
    default :
        break;
    }

}

void CZMEntLoadout::DistributeToPlayer( CZMPlayer* pPlayer )
{
    if ( !pPlayer ) return;

    if ( !pPlayer->IsHuman() || !pPlayer->IsAlive() ) return;
    

    switch ( GetMethod() )
    {
    case LOMETHOD_CATEGORY :
    {
        for ( int i = 0; i < LOCAT_MAX; i++ )
        {
            if ( m_vCurCat[i].Count() > 0 )
            {
                int j = random->RandomInt( 0, m_vCurCat[i].Count() - 1 );

                GiveWeapon( pPlayer, m_vCurCat[i].Element( j ) );

                m_vCurCat[i].Remove( j );
            }
        }
        break;
    }

    case LOMETHOD_RANDOM :
    {
        CUtlVector<int> vRemaining;
        vRemaining.Purge();

        for ( int i = 0; i < LO_MAX; i++ )
        {
            if ( m_iCurRandom[i] > 0 )
                vRemaining.AddToTail( i );
        }

        if ( vRemaining.Count() > 0 )
        {
            int j = vRemaining[random->RandomInt( 0, vRemaining.Count() - 1 )];

            GiveWeapon( pPlayer, j );

            --m_iCurRandom[j];
        }
        break;
    }

    default :
        break;
    }
}

void CZMEntLoadout::GiveWeapon( CZMPlayer* pPlayer, int loadout_wep )
{
    const char* weps[] = {
        "weapon_zm_pistol",
        "weapon_zm_shotgun",
        "weapon_zm_rifle",
        "weapon_zm_mac10",
        "weapon_zm_molotov",
        "weapon_zm_sledge",
        "weapon_zm_improvised",
        "weapon_zm_revolver",
    };

    COMPILE_TIME_ASSERT( ARRAYSIZE( weps ) >= LO_MAX );

    Assert( loadout_wep >= 0 && loadout_wep < LO_MAX );

    const char* wepname = weps[loadout_wep];


    bool ammo = false;

    switch ( loadout_wep )
    {
    case LO_PISTOL:
    case LO_REVOLVER:
    case LO_RIFLE:
    case LO_SHOTGUN:
    case LO_MAC10:
        ammo = true;
        break;
    default :
        break;
    }


    CZMBaseWeapon* pWeapon = static_cast<CZMBaseWeapon*>( pPlayer->Weapon_Create( wepname ) );

    if ( !pWeapon )
    {
        Warning( "Failed to give player loadout weapon %s!\n", wepname );
        return;
    }


    pPlayer->Weapon_Equip( pWeapon );


    if ( ammo )
    {
        pPlayer->GiveAmmo( pWeapon->GetMaxClip1(), pWeapon->GetPrimaryAmmoType(), true );
    }

    DevMsg( "Gave %s weapon %s!\n", pPlayer->GetPlayerName(), wepname );
}


int ITEM_GiveAmmo( CBasePlayer *pPlayer, float flCount, const char *pszAmmoName, bool bSuppressSound = false );

/*
    Ammo - revolver
*/
class CZMItemRevolverAmmo : public CItem
{
public:
	DECLARE_CLASS( CZMItemRevolverAmmo, CItem );

	void Spawn( void )
	{ 
		Precache();
		SetModel( "models/items/revolverammo.mdl" );
		BaseClass::Spawn();
	}
	void Precache( void )
	{
		PrecacheModel( "models/items/revolverammo.mdl" );
	}
	bool MyTouch( CBasePlayer *pPlayer )
	{
		if ( ITEM_GiveAmmo( pPlayer, SIZE_ZMAMMO_REVOLVER, "Revolver" ) )
		{
			if ( g_pGameRules->ItemShouldRespawn( this ) == GR_ITEM_RESPAWN_NO )
			{
				UTIL_Remove( this );
			}

			return true;
		}
		return false;
	}
};

LINK_ENTITY_TO_CLASS( item_ammo_revolver, CZMItemRevolverAmmo );


/*
    Block hidden trigger
*/
BEGIN_DATADESC( CZMEntTriggerBlockHidden )
    DEFINE_KEYFIELD( m_bActive, FIELD_BOOLEAN, "Active" ),

    DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),
    DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
    DEFINE_INPUTFUNC( FIELD_VOID, "Enabled", InputEnable ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_blockspotcreate, CZMEntTriggerBlockHidden );


CZMEntTriggerBlockHidden::CZMEntTriggerBlockHidden()
{
    g_pBlockHidden->AddToTail( this );
}

CZMEntTriggerBlockHidden::~CZMEntTriggerBlockHidden()
{
    g_pBlockHidden->FindAndRemove( this );
}

void CZMEntTriggerBlockHidden::Spawn( void )
{
    BaseClass::Spawn();

    InitTrigger();
}

void CZMEntTriggerBlockHidden::InputToggle( inputdata_t &inputData )
{
    m_bActive = !m_bActive;
}

void CZMEntTriggerBlockHidden::InputEnable( inputdata_t &inputData )
{
    m_bActive = true;
}

void CZMEntTriggerBlockHidden::InputDisable( inputdata_t &inputData )
{
    m_bActive = false;
}