#include "cbase.h"

#include "team.h"
#include "items.h"
#include "props.h"
#include "IEffects.h"
#include "envspark.h"
#include "precipitation_shared.h"


#include "zmr_gamerules.h"
#include "zmr_player.h"
#include "zmr_shareddefs.h"
#include "npcs/zmr_zombiebase_shared.h"
#include "weapons/zmr_base.h"
#include "zmr_mapitemaction.h"

#include "zmr_entities.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



#define MODEL_MANIPULATE        "models/manipulatable.mdl"
#define MODEL_ZOMBIESPAWN       "models/zombiespawner.mdl"
#define MODEL_RALLYPOINT        "models/rallypoint.mdl"
#define MODEL_SPAWNNODE         "models/spawnnode.mdl"
#define MODEL_AMBUSHTRIGGER     "models/trap.mdl"
#define MODEL_MANITRIGGER       "models/trap.mdl"


/*
    func_win
*/
BEGIN_DATADESC( CZMEntWin )
    DEFINE_INPUTFUNC( FIELD_VOID, "Win", InputHumanWin ),
    DEFINE_INPUTFUNC( FIELD_VOID, "Lose", InputHumanLose ),

    DEFINE_OUTPUT( m_OnWin, "OnWin" ),
    DEFINE_OUTPUT( m_OnLose, "OnLose" ),
    DEFINE_OUTPUT( m_OnSubmit, "OnSubmit" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( func_win, CZMEntWin );


void CZMEntWin::OnRoundEnd( ZMRoundEndReason_t reason )
{
    // Fire outputs for all func_wins.
    bool output = false;

    switch ( reason )
    {
    case ZMROUND_HUMANLOSE :
    case ZMROUND_HUMANDEAD :
    case ZMROUND_HUMANWIN :
    case ZMROUND_ZMSUBMIT : output = true; break;
    default : break;
    }

    if ( !output ) return;


    CZMEntWin* pWin;
    COutputEvent* pEvent;
    CBaseEntity* pEnt = nullptr;
    
    while ( (pEnt = gEntList.FindEntityByClassname( pEnt, "func_win" )) != nullptr )
    {
        pWin = dynamic_cast<CZMEntWin*>( pEnt );
        if ( !pWin ) continue;


        switch ( reason )
        {
        case ZMROUND_HUMANLOSE :
        case ZMROUND_HUMANDEAD : pEvent = &(pWin->m_OnLose); break;
        case ZMROUND_HUMANWIN : pEvent = &(pWin->m_OnWin); break;
        case ZMROUND_ZMSUBMIT :
        default : pEvent = &(pWin->m_OnSubmit); break;
        }

        pEvent->FireOutput( nullptr, nullptr );
    }
}

void CZMEntWin::Spawn()
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

void CZMEntBaseUsable::Spawn()
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

int CZMEntBaseSimple::ShouldTransmit( const CCheckTransmitInfo* pInfo )
{
    CZMPlayer* pPlayer = static_cast<CZMPlayer*>( CBaseEntity::Instance( pInfo->m_pClientEnt ) );

    if ( pPlayer && pPlayer->IsZM() )
    {
        return FL_EDICT_ALWAYS;
    }

    return FL_EDICT_DONTSEND;
}

int CZMEntBaseSimple::UpdateTransmitState()
{
    return FL_EDICT_FULLCHECK;
}


/*
    Zombie spawn
*/
IMPLEMENT_SERVERCLASS_ST( CZMEntZombieSpawn, DT_ZM_EntZombieSpawn )
    SendPropInt( SENDINFO( m_fZombieFlags ) ),
    SendPropArray3( SENDINFO_ARRAY3(m_iZombieCosts), SendPropInt( SENDINFO_ARRAY(m_iZombieCosts), 16 ) ), // Resource cost is not likely to surpass 16 bits
END_SEND_TABLE()

BEGIN_DATADESC( CZMEntZombieSpawn )
    //DEFINE_KEYFIELD( m_bActive, FIELD_BOOLEAN, "Active" ),

    DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),
    DEFINE_INPUTFUNC( FIELD_VOID, "Hide", InputHide ),
    DEFINE_INPUTFUNC( FIELD_VOID, "Unhide", InputUnhide ),

    DEFINE_INPUTFUNC( FIELD_INTEGER, "SetZombieFlags", InputSetZombieFlags ),
    DEFINE_INPUTFUNC( FIELD_INTEGER, "AddZombieFlags", InputAddZombieFlags ),
    DEFINE_INPUTFUNC( FIELD_INTEGER, "RemoveZombieFlags", InputRemoveZombieFlags ),

    DEFINE_INPUTFUNC( FIELD_VOID, "EnableShamblerSpawn", InputEnableZombie0 ),
    DEFINE_INPUTFUNC( FIELD_VOID, "EnableBansheeSpawn", InputEnableZombie1 ),
    DEFINE_INPUTFUNC( FIELD_VOID, "EnableHulkSpawn", InputEnableZombie2 ),
    DEFINE_INPUTFUNC( FIELD_VOID, "EnableDrifterSpawn", InputEnableZombie3 ),
    DEFINE_INPUTFUNC( FIELD_VOID, "EnableImmolatorSpawn", InputEnableZombie4 ),

    DEFINE_INPUTFUNC( FIELD_VOID, "DisableShamblerSpawn", InputDisableZombie0 ),
    DEFINE_INPUTFUNC( FIELD_VOID, "DisableBansheeSpawn", InputDisableZombie1 ),
    DEFINE_INPUTFUNC( FIELD_VOID, "DisableHulkSpawn", InputDisableZombie2 ),
    DEFINE_INPUTFUNC( FIELD_VOID, "DisableDrifterSpawn", InputDisableZombie3 ),
    DEFINE_INPUTFUNC( FIELD_VOID, "DisableImmolatorSpawn", InputDisableZombie4 ),

    DEFINE_INPUTFUNC( FIELD_INTEGER, "SetShamblerCost", InputSetZombieCost0 ),
    DEFINE_INPUTFUNC( FIELD_INTEGER, "SetBansheeCost", InputSetZombieCost1 ),
    DEFINE_INPUTFUNC( FIELD_INTEGER, "SetHulkCost", InputSetZombieCost2 ),
    DEFINE_INPUTFUNC( FIELD_INTEGER, "SetDrifterCost", InputSetZombieCost3 ),
    DEFINE_INPUTFUNC( FIELD_INTEGER, "SetImmolatorCost", InputSetZombieCost4 ),


    DEFINE_KEYFIELD( m_fZombieFlags, FIELD_INTEGER, "zombieflags" ),
    //DEFINE_KEYFIELD( m_nSpawnQueueCapacity, FIELD_INTEGER, "spawnqueuecapacity" ),
    DEFINE_KEYFIELD( m_sRallyName, FIELD_STRING, "rallyname" ),
    DEFINE_KEYFIELD( m_sFirstNodeName, FIELD_STRING, "nodename" ),

    DEFINE_KEYFIELD( m_sZombieModelGroup, FIELD_STRING, "modelgroup" ),

    DEFINE_KEYFIELD( m_iZombieCosts[0], FIELD_INTEGER, "shamblercost" ),
    DEFINE_KEYFIELD( m_iZombieCosts[1], FIELD_INTEGER, "bansheecost" ),
    DEFINE_KEYFIELD( m_iZombieCosts[2], FIELD_INTEGER, "hulkcost" ),
    DEFINE_KEYFIELD( m_iZombieCosts[3], FIELD_INTEGER, "driftercost" ),
    DEFINE_KEYFIELD( m_iZombieCosts[4], FIELD_INTEGER, "immolatorcost" ),

    DEFINE_OUTPUT( m_OnSpawnZMClass[0], "OnSpawnShambler" ),
    DEFINE_OUTPUT( m_OnSpawnZMClass[1], "OnSpawnBanshee" ),
    DEFINE_OUTPUT( m_OnSpawnZMClass[2], "OnSpawnHulk" ),
    DEFINE_OUTPUT( m_OnSpawnZMClass[3], "OnSpawnDrifter" ),
    DEFINE_OUTPUT( m_OnSpawnZMClass[4], "OnSpawnImmolator" ),

    DEFINE_OUTPUT( m_OnSpawnNPC, "OnSpawnNPC" ),

    DEFINE_THINKFUNC( SpawnThink ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( info_zombiespawn, CZMEntZombieSpawn );
PRECACHE_REGISTER( info_zombiespawn );


CZMEntZombieSpawn::CZMEntZombieSpawn()
{
    m_vSpawnNodes.Purge();
    m_vSpawnQueue.Purge();

    for (int i = 0; i < ZMCLASS_MAX; i++)
    {
        m_iZombieCosts.Set(i, -1);
    }

    m_iZombieNodeCounter = 0;
}

void CZMEntZombieSpawn::Precache()
{
    PrecacheModel( MODEL_ZOMBIESPAWN );
}

void CZMEntZombieSpawn::Spawn()
{
    BaseClass::Spawn();

    SetModel( MODEL_ZOMBIESPAWN );

    InitSpawnNodes();


    //if ( m_nSpawnQueueCapacity < 1 )
    //    m_nSpawnQueueCapacity = 10;

    // Spawn flags to zombie flags.
    if ( m_fZombieFlags <= 0 ) // Only if the author isn't using the deprecated method.
    {
        for ( int i = 0; i < ZMCLASS_MAX; i++ )
        {
            if ( HasSpawnFlags( 1 << i ) )
            {
                m_fZombieFlags |= ( 1 << i );
            }
        }
    }



    m_vSpawnQueue.EnsureCapacity( 10 );

    StopSpawning();

    
    CBaseEntity* pEnt = nullptr;
    while ( !m_hRallyPoint.Get() && (pEnt = gEntList.FindEntityByName( pEnt, m_sRallyName )) != nullptr )
    {
        m_hRallyPoint.Set( dynamic_cast<CZMEntRallyPoint*>( pEnt ) );
    }
}

bool CZMEntZombieSpawn::AcceptInput( const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t Value, int outputID )
{
    bool base = BaseClass::AcceptInput( szInputName, pActivator, pCaller, Value, outputID );

    // Update the menu in response to any valid input
    if ( base ) SendMenuUpdate();

    return base;
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

bool CZMEntZombieSpawn::CanSpawn( ZombieClass_t zclass ) const
{
    return (GetZombieFlags() == 0 || (GetZombieFlags() & (1 << zclass)) );
}

bool CZMEntZombieSpawn::QueueUnit( CZMPlayer* pPlayer, ZombieClass_t zclass, int amount )
{
    if ( !CZMBaseZombie::IsValidClass( zclass ) )
    {
        Warning( "Attempted to spawn an invalid unit in queue (%i)!\n", zclass );
        return false;
    }


    if ( !CanSpawn( zclass ) ) return false;


    int plyindex = pPlayer ? pPlayer->entindex() : 0;
    bool added = false;

    if ( m_vSpawnQueue.Count() )
    {
        queue_info_t& queue = m_vSpawnQueue.Element( m_vSpawnQueue.Count() - 1 );
        if ( queue.m_zclass == zclass && queue.m_iSpawnerIndex == plyindex )
        {
            queue.m_nCount += amount;
            if ( queue.m_nCount > 99 )
                queue.m_nCount = 99;

            added = true;
        }
    }


    if ( !added && m_vSpawnQueue.Count() < 10 )
    {
        queue_info_t queue;

        queue.m_zclass = zclass;
        queue.m_nCount = (uint8)amount;
        queue.m_iSpawnerIndex = plyindex;

        m_vSpawnQueue.AddToTail( queue );
    }


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

float CZMEntZombieSpawn::GetSpawnDelay() const
{
    return CZMBaseZombie::GetSpawnDelay( !m_vSpawnQueue.Count() ? ZMCLASS_INVALID : m_vSpawnQueue[0].m_zclass );
}

void CZMEntZombieSpawn::SetNextSpawnThink()
{
    // ZMRTODO: See if this causes any problems.
    // Only set next spawn think if we're not already thinking.
    float delay = GetSpawnDelay();

    if ( GetNextThink() == TICK_NEVER_THINK || (GetNextThink() - gpGlobals->curtime) > delay )
    {
        SetNextThink( gpGlobals->curtime + delay );
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

        if ( pZM && pZM->IsZM() && pZM->GetMenuEnt() == this )
        {
            filter.AddRecipient( pZM );
        }
    }

    if ( !filter.GetRecipientCount() ) return;


	UserMessageBegin( filter, "ZMBuildMenuUpdate" );
    {
		WRITE_SHORT( entindex() );

		WRITE_BOOL( IsActive() );

        int count = m_vSpawnQueue.Count();
        WRITE_BYTE( count );

		for ( int i = 0; i < count; i++ )
		{
            Assert( m_vSpawnQueue[i].m_zclass != ZMCLASS_INVALID );

			WRITE_BYTE( m_vSpawnQueue[i].m_zclass );
			WRITE_BYTE( m_vSpawnQueue[i].m_nCount );
		}
    }
	MessageEnd();
}

void CZMEntZombieSpawn::SetRallyPoint( const Vector& pos )
{
    auto* pRallypoint = m_hRallyPoint.Get();
    if ( !pRallypoint )
    {
        pRallypoint = dynamic_cast<CZMEntRallyPoint*>( CBaseEntity::Create( "info_rallypoint", pos, vec3_angle, this ) );
        m_hRallyPoint.Set( pRallypoint );

        if ( !pRallypoint )
        {
            Warning( "Unable to create rally point for zombie spawn %i!\n", entindex() );
        }

        return;
    }

    pRallypoint->Teleport( &pos, nullptr, nullptr );
}

void CZMEntZombieSpawn::SpawnThink()
{
    if ( m_vSpawnQueue.Count() < 1 || !IsActive() )
    {
        StopSpawning();
        return;
    }


    queue_info_t& queue = m_vSpawnQueue.Element( 0 );

    ZombieClass_t zclass = queue.m_zclass;


    int cost = m_iZombieCosts[zclass] == -1 ? CZMBaseZombie::GetCost( zclass ) : m_iZombieCosts[zclass];
    bool bCreate = true;
    CZMPlayer* pPlayer = nullptr;

    if ( queue.m_iSpawnerIndex > 0 )
    {
        pPlayer = ToZMPlayer( UTIL_PlayerByIndex( queue.m_iSpawnerIndex ) );


        if ( pPlayer )
        {
            bCreate = pPlayer->HasEnoughRes( cost );
        }
        else
        {
            // That player no longer exists, just remove his shit from queue.
            bCreate = false;
            m_vSpawnQueue.Remove( 0 );
        }
    }

    if ( bCreate && !CZMBaseZombie::HasEnoughPopToSpawn( zclass ) )
        bCreate = false;

    
    if ( bCreate && CreateZombie( zclass, pPlayer ) )
    {
        queue.m_nCount--;


        // We no longer have zombies in this slot, remove us.
        if ( !queue.m_nCount )
        {
            m_vSpawnQueue.Remove( 0 );
        }
        

        if ( pPlayer )
        {
            pPlayer->IncResources( -cost );
        }


        // Tell everyone that has this menu open that a zombie has been spawned.
        SendMenuUpdate();
    }


    
    if ( m_vSpawnQueue.Count() )
    {
        SetNextSpawnThink();
    }
    else
    {
        // Nothing more in queue, stop spawning think.
        StopSpawning();
    }
}

CZMBaseZombie* CZMEntZombieSpawn::CreateZombie( ZombieClass_t zclass, CZMPlayer* pSpawner )
{
    const char* classname = CZMBaseZombie::ClassToName( zclass );

    if ( !classname )
    {
        Warning( "Can't create zombie of class %i!\n", zclass );
        return false;
    }


    CZMBaseZombie* pZombie = static_cast<CZMBaseZombie*>( CreateEntityByName( classname ) );

    if ( !pZombie ) return nullptr;


    Vector spawnpos;
    QAngle ang = vec3_angle;

    auto* pSpawnPoint = FindSpawnPoint( pZombie, spawnpos, ang );
    if ( !pSpawnPoint )
    {
        UTIL_RemoveImmediate( pZombie );
        return nullptr;
    }


    pZombie->Teleport( &spawnpos, nullptr, &vec3_origin );
    ang.x = ang.z = 0.0f;
    pZombie->SetAbsAngles( ang );

    // Don't drop to floor since npcs seem to get stuck on displacements.
    //UTIL_DropToFloor( pZombie, MASK_NPCSOLID );

    pZombie->SetZombieModelGroupName( m_sZombieModelGroup );


    // We can be marked for deletion...
    if ( DispatchSpawn( pZombie ) != 0 )
    {
        UTIL_RemoveImmediate( pZombie );
        return nullptr;
    }

    pZombie->SetMyCommandStyle( pSpawner );

    pZombie->Activate();


    // Fire outputs in case the mapper wants to do something with the zombies spawned here
    m_OnSpawnNPC.Set( pZombie, pZombie, this );
	m_OnSpawnZMClass[pZombie->GetZombieClass()].Set( pZombie, pZombie, this );

    if ( pSpawnPoint != this )
    {
        // Also fire an output on what could be either a spawn node or a spawn volume.
        // We could do classname check + dynamic_casting instead, but doing it this way is more flexible.
        variant_t variant;
        variant.SetEntity( pZombie );
        pSpawnPoint->FireNamedOutput( "OnSpawnNPC", variant, pZombie, pSpawnPoint );
    }


    auto* pRallypoint = m_hRallyPoint.Get();
    if ( pRallypoint )
    {
        DevMsg( "Commanding zombie to rallypoint...\n" );

        // Face the rallypoint instead.
        Vector dir = pRallypoint->GetAbsOrigin() - pZombie->GetAbsOrigin();
        ang.x = ang.z = 0.f;
        ang.y = RAD2DEG( atan2f( dir.y, dir.x ) );
        pZombie->SetAbsAngles( ang );

        pZombie->Command( nullptr, pRallypoint->GetAbsOrigin(), 256.0f ); // Some additional tolerance.
    }
    

    return pZombie;
}

void CZMEntZombieSpawn::InitSpawnNodes()
{
    if ( m_vSpawnNodes.Count() )
        return;

 
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
                    Warning( "Detected recursion in spawn nodes!\n" );
                    break;
                }

                pEnt = nullptr;
                next = temp;

                m_vSpawnNodes.AddToTail( next );
            }
        }
    }
}

CBaseEntity* CZMEntZombieSpawn::FindSpawnPoint( CZMBaseZombie* pZombie, Vector& outpos, QAngle& outang )
{
    if ( !pZombie ) return nullptr;


    const int nNodes =  m_vSpawnNodes.Count();

    //
    // We have spawn nodes.
    //
    if ( nNodes )
    {
        CZMEntSpawnNode* pNode = nullptr;

        ++m_iZombieNodeCounter;
        if ( m_iZombieNodeCounter >= nNodes )
            m_iZombieNodeCounter = 0;


        int startindex = m_iZombieNodeCounter;

        int endindex = startindex - 1;
        if ( endindex < 0 )
            endindex = nNodes - 1;

        for ( int i = startindex; ; i++ )
        {
            if ( i >= nNodes )
                i = 0;

            pNode = m_vSpawnNodes.Element( i );

            if ( pNode && pZombie->CanSpawn( pNode->GetAbsOrigin() ) )
            {
                outpos = pNode->GetAbsOrigin();
                outang = pNode->GetAbsAngles();
                return pNode;
            }

            if ( i == endindex )
                break;
        }

        // No open nodes found, just pick the next node
        pNode = m_vSpawnNodes.Element( startindex );

        if ( pNode )
        {
            outpos = pNode->GetAbsOrigin();
            outang = pNode->GetAbsAngles();
            return pNode;
        }
    }
    //
    // No nodes, check spawn volume.
    //
    else
    {
        auto* pBrush = dynamic_cast<CZMEntTriggerSpawnVolume*>( gEntList.FindEntityByName( nullptr, m_sFirstNodeName ) );

        if ( pBrush )
        {
            pBrush->GetPositionWithin( outpos );
            return pBrush;
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
            Vector dir = pos - GetAbsOrigin(); // Face away from the zombie spawn.
            outang.y = RAD2DEG( atan2f( dir.y, dir.x ) );

            return this;
        }
    }

    return nullptr;
}


/*
    Spawnnode
*/
BEGIN_DATADESC( CZMEntSpawnNode )
    DEFINE_KEYFIELD( m_sNextNodeName, FIELD_STRING, "nodename" ),
    DEFINE_OUTPUT( m_OnSpawnNPC, "OnSpawnNPC" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( info_spawnnode, CZMEntSpawnNode );
PRECACHE_REGISTER( info_spawnnode );


void CZMEntSpawnNode::Precache()
{
    PrecacheModel( MODEL_SPAWNNODE );
}

void CZMEntSpawnNode::Spawn()
{
    SetModel( MODEL_SPAWNNODE );

    SetSolid( SOLID_NONE );
}


/*
    Rallypoint
*/
BEGIN_DATADESC( CZMEntRallyPoint )
END_DATADESC()

LINK_ENTITY_TO_CLASS( info_rallypoint, CZMEntRallyPoint );
PRECACHE_REGISTER( info_rallypoint );


void CZMEntRallyPoint::Precache()
{
    PrecacheModel( MODEL_RALLYPOINT );
}

void CZMEntRallyPoint::Spawn()
{ 
    SetModel( MODEL_RALLYPOINT );

    SetSolid( SOLID_NONE );
}


/*
    Manipulate trigger
*/
//IMPLEMENT_SERVERCLASS_ST( CZMEntManipulateTrigger, DT_ZM_EntManipulateTrigger )
//END_SEND_TABLE()

BEGIN_DATADESC( CZMEntManipulateTrigger )
    DEFINE_THINKFUNC( ScanThink ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( info_manipulate_trigger, CZMEntManipulateTrigger );
PRECACHE_REGISTER( info_manipulate_trigger );


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

void CZMEntManipulateTrigger::Precache()
{
    PrecacheModel( MODEL_MANITRIGGER );
}

void CZMEntManipulateTrigger::Spawn()
{ 
    SetModel( MODEL_MANITRIGGER );

    SetSolid( SOLID_NONE );

    SetThink( &CZMEntManipulateTrigger::ScanThink );
    SetNextThink( gpGlobals->curtime + 0.5f );
}

ConVar zm_sv_trap_triggerrange( "zm_sv_trap_triggerrange", "100", FCVAR_NOTIFY, "How close survivors have to get to a trap to trigger it." );

void CZMEntManipulateTrigger::ScanThink()
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
    Ambush trigger
*/

ConVar zm_sv_ambush_triggerrange( "zm_sv_ambush_triggerrange", "100", FCVAR_NOTIFY, "How close survivors have to get to an ambush to trigger it." );


//IMPLEMENT_SERVERCLASS_ST( CZMEntManipulateTrigger, DT_ZM_EntManipulateTrigger )
//END_SEND_TABLE()

BEGIN_DATADESC( CZMEntAmbushTrigger )
    DEFINE_THINKFUNC( ScanThink ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( info_ambush_trigger, CZMEntAmbushTrigger );
PRECACHE_REGISTER( info_ambush_trigger );


CZMEntAmbushTrigger::CZMEntAmbushTrigger()
{
    m_nAmbushZombies = 0;
}

CZMEntAmbushTrigger::~CZMEntAmbushTrigger()
{

}

void CZMEntAmbushTrigger::Precache()
{
    PrecacheModel( MODEL_AMBUSHTRIGGER );
}

void CZMEntAmbushTrigger::Spawn()
{ 
    SetModel( MODEL_AMBUSHTRIGGER );

    SetSolid( SOLID_NONE );

    SetThink( &CZMEntAmbushTrigger::ScanThink );
    SetNextThink( gpGlobals->curtime + 0.5f );
}

void CZMEntAmbushTrigger::SetAmbushZombies( int count )
{
    if ( count < 1 )
    {
        Warning( "Can't create an ambush trigger with 0 zombies!\n" );
        return;
    }


    m_nAmbushZombies = count;
}

void CZMEntAmbushTrigger::RemoveZombieFromAmbush()
{
    if ( --m_nAmbushZombies <= 0 )
    {
        UTIL_Remove( this );
    }
}

void CZMEntAmbushTrigger::Trigger( CBaseEntity* pActivator )
{
    g_ZombieManager.ForEachAliveZombie( [ this, pActivator ]( CZMBaseZombie* pZombie )
    {
        if ( pZombie->GetAmbushTrigger() == this )
        {
            pZombie->RemoveFromAmbush( false );

            if ( !pZombie->GetEnemy() )
            {
                pZombie->AcquireEnemy( pActivator );
            }
        }
    } );
}

void CZMEntAmbushTrigger::ScanThink()
{
    CBaseEntity* pEnt = nullptr;
    while ( (pEnt = gEntList.FindEntityInSphere( pEnt, GetAbsOrigin(), zm_sv_ambush_triggerrange.GetFloat() )) != nullptr )
    {
        CZMPlayer* pPlayer = ToZMPlayer( pEnt );

        if ( pPlayer && pPlayer->IsHuman() && pPlayer->IsAlive() )
        {
            Trigger( pPlayer );


            UTIL_Remove( this );
            return;
        }
    }

    SetNextThink( gpGlobals->curtime + 0.3f );
}


/*
    Manipulate orb
*/
IMPLEMENT_SERVERCLASS_ST( CZMEntManipulate, DT_ZM_EntManipulate )
    SendPropInt( SENDINFO( m_nCost ) ),
    SendPropInt( SENDINFO( m_nTrapCost ) ),
    SendPropStringT( SENDINFO( m_sDescription ) ),
END_SEND_TABLE()

BEGIN_DATADESC( CZMEntManipulate )
    DEFINE_KEYFIELD( m_sDescription, FIELD_STRING, "description" ),
    DEFINE_KEYFIELD( m_nCost, FIELD_INTEGER, "Cost" ),
    DEFINE_KEYFIELD( m_nTrapCost, FIELD_INTEGER, "TrapCost" ),
    DEFINE_KEYFIELD( m_bRemoveOnTrigger, FIELD_BOOLEAN, "RemoveOnTrigger" ),

    DEFINE_OUTPUT( m_OnPressed, "OnPressed" ),

    DEFINE_INPUTFUNC( FIELD_VOID, "Press", InputPress ),
    DEFINE_INPUTFUNC( FIELD_STRING, "SetDescription", InputSetDescription ),
    DEFINE_INPUTFUNC( FIELD_INTEGER, "SetTrapCost", InputSetTrapCost ),
    DEFINE_INPUTFUNC( FIELD_INTEGER, "SetCost", InputSetCost ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( info_manipulate, CZMEntManipulate );
PRECACHE_REGISTER( info_manipulate );


CZMEntManipulate::CZMEntManipulate()
{
    m_vTriggers.Purge();
}

CZMEntManipulate::~CZMEntManipulate()
{
    RemoveTriggers();
}

void CZMEntManipulate::Precache()
{
    PrecacheModel( MODEL_MANIPULATE );
}

void CZMEntManipulate::Spawn()
{
    BaseClass::Spawn();


    if ( m_nCost <= 0 ) m_nCost = 10;
    if ( m_nTrapCost <= 0 ) m_nTrapCost = m_nCost * 1.5;


    SetModel( MODEL_MANIPULATE );
}

bool CZMEntManipulate::AcceptInput( const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t Value, int outputID )
{
    bool base = BaseClass::AcceptInput( szInputName, pActivator, pCaller, Value, outputID );

    // Update the menu in response to any valid input
    if ( base ) SendMenuUpdate();

    return base;
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

void CZMEntManipulate::InputPress( inputdata_t &inputdata )
{
    Trigger( nullptr );
}

void CZMEntManipulate::InputSetTrapCost( inputdata_t &inputdata )
{
    if ( inputdata.value.Int() <= 0 )
        m_nTrapCost = m_nCost * 1.5;
    else
        m_nTrapCost = inputdata.value.Int();
}

void CZMEntManipulate::InputSetCost( inputdata_t &inputdata )
{
    if ( inputdata.value.Int() <= 0 )
        m_nCost = 10;
    else
        m_nCost = inputdata.value.Int();
}

void CZMEntManipulate::Trigger( CBaseEntity* pActivator )
{
    if ( !IsActive() ) return;


    if ( pActivator && pActivator->IsPlayer() )
    {
        CZMPlayer* pPlayer = ToZMPlayer( pActivator );

        if ( pPlayer )
        {
            if ( pPlayer->HasEnoughRes( GetCost() ) )
            {
                pPlayer->IncResources( -GetCost() );
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

void CZMEntManipulate::SendMenuUpdate()
{
	CRecipientFilter filter;
	filter.MakeReliable();

    CZMPlayer* pZM;
    for ( int i = 1; i <= gpGlobals->maxClients; i++ )
    {
        pZM = ToZMPlayer( UTIL_PlayerByIndex( i ) );

        if ( pZM && pZM->IsZM() && pZM->GetMenuEnt() == this )
        {
            filter.AddRecipient( pZM );
        }
    }

    if ( !filter.GetRecipientCount() ) return;


	UserMessageBegin( filter, "ZMManiMenuUpdate" );
    {
		WRITE_SHORT( entindex() );

		WRITE_BOOL( IsActive() );
    }
	MessageEnd();
}


/*
    Player count trigger
*/
class CZMEntTriggerPlayerCount : public CBaseTrigger
{
public:
    DECLARE_CLASS( CZMEntTriggerPlayerCount, CBaseTrigger )
    DECLARE_DATADESC()


    void Spawn();

    void CountThink();


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
    DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),

    DEFINE_OUTPUT( m_OnCount, "OnCount" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_playercount, CZMEntTriggerPlayerCount );


void CZMEntTriggerPlayerCount::Spawn()
{
    BaseClass::Spawn();

    InitTrigger();

    // Don't think instantly since Active-keyvalue still isn't set for some reason.
    SetThink( &CZMEntTriggerPlayerCount::CountThink );
    SetNextThink( gpGlobals->curtime + 1.0f );
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

void CZMEntTriggerPlayerCount::CountThink()
{
    if ( !IsActive() ) return;


    int count = 0;

    touchlink_t* root = static_cast<touchlink_t*>( GetDataObject( TOUCHLINK ) );

    if ( !root )
    {
        //DevMsg( "No touch link found for trigger_playercount!\n" );

        SetNextThink( gpGlobals->curtime + 1.0f );
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

        SetNextThink( gpGlobals->curtime + 1.0f );
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


    void Spawn();

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
    DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),

    DEFINE_INPUTFUNC( FIELD_VOID, "Count", InputCount ),

    DEFINE_OUTPUT( m_OnCount, "OnCount" ),
    DEFINE_OUTPUT( m_OnNotCount, "OnNotCount" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_entitycount, CZMEntTriggerEntityCount );


void CZMEntTriggerEntityCount::Spawn()
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
        DevMsg( "No touch link found for trigger_entitycount!\n" );
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
            CZMBaseZombie* pNPC = ToZMBaseZombie( pEnt );

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

void CZMEntLoadout::Spawn()
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

// Hack to fix some old maps using this shit wrong.
CZMEntLoadout::LoadOutMethod_t CZMEntLoadout::GetMethod()
{
    if ( m_iMethod <= LOMETHOD_INVALID || m_iMethod >= LOMETHOD_MAX )
        return LOMETHOD_RANDOM;

    return m_iMethod;
}

void CZMEntLoadout::Reset()
{
    switch ( GetMethod() )
    {
    case LOMETHOD_RANDOM :
    {
        for ( int i = 0; i < LO_MAX; i++ )
        {
            m_iCurRandom[i] = m_iCounts[i];
        }
        break;

    }
    case LOMETHOD_CATEGORY : // My favorite...
    default :
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
    }

}

void CZMEntLoadout::DistributeToPlayer( CZMPlayer* pPlayer )
{
    if ( !pPlayer ) return;

    if ( !pPlayer->IsHuman() || !pPlayer->IsAlive() ) return;
    

    switch ( GetMethod() )
    {
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
    
    case LOMETHOD_CATEGORY :
    default :
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
    }
}

void CZMEntLoadout::GiveWeapon( CZMPlayer* pPlayer, int loadout_wep )
{
    // Loadout is a mix of items and item classes.
    const char* loadouts[LO_MAX] = {
        "Pistol",
        "Shotgun",
        "Rifle",
        "SMG",
        "weapon_zm_molotov",
        "weapon_zm_sledge",
        "weapon_zm_improvised",
        "BigPistol",
    };

    COMPILE_TIME_ASSERT( ARRAYSIZE( loadouts ) >= LO_MAX );

    Assert( loadout_wep >= 0 && loadout_wep < LO_MAX );

    const char* loadoutClass = loadouts[loadout_wep];


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


    CUtlVector<const ZMItemAction::ItemBaseData_t*> items;

    // See if it's a single item.
    auto* pData = ZMItemAction::g_ZMMapItemSystem.GetItemData( loadoutClass );
    
    if ( pData )
    {
        items.AddToTail( pData );
    }
    else
    {
        // It's a class. Get all the items.
        auto flag = ZMItemAction::g_ZMMapItemSystem.GetClassFlagByName( loadoutClass );
        ZMItemAction::g_ZMMapItemSystem.GetMapItemsByClass( flag, items );
    }


    // Get a random weapon from the items.
    const char* wepname = items.Count() > 0 ? items[random->RandomInt( 0, items.Count() - 1 )]->m_pszClassname : "";
    auto* pWeapon = ToZMBaseWeapon( pPlayer->Weapon_Create( wepname ) );

    if ( !pWeapon )
    {
        Warning( "Failed to give player loadout '%s'!\n", loadoutClass );
        return;
    }


    pPlayer->Weapon_Equip( pWeapon );


    if ( ammo )
    {
        pPlayer->GiveAmmo( pWeapon->GetMaxClip1(), pWeapon->GetPrimaryAmmoType(), true );
    }

    DevMsg( "Gave %s weapon %s!\n", pPlayer->GetPlayerName(), wepname );
}


/*
    Block hidden trigger
*/
extern CUtlVector<CZMEntTriggerBlockHidden*> g_ZMBlockHidden;

BEGIN_DATADESC( CZMEntTriggerBlockHidden )
    DEFINE_KEYFIELD( m_bActive, FIELD_BOOLEAN, "Active" ),

    DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),
    DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
    DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_blockspotcreate, CZMEntTriggerBlockHidden );


CZMEntTriggerBlockHidden::CZMEntTriggerBlockHidden()
{
    g_ZMBlockHidden.AddToTail( this );
}

CZMEntTriggerBlockHidden::~CZMEntTriggerBlockHidden()
{
    g_ZMBlockHidden.FindAndRemove( this );
}

void CZMEntTriggerBlockHidden::Spawn()
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


/*
    Block phys explosion
*/
CUtlVector<CZMEntTriggerBlockPhysExp*> g_ZMBlockPhysExp;

BEGIN_DATADESC( CZMEntTriggerBlockPhysExp )
    DEFINE_KEYFIELD( m_bActive, FIELD_BOOLEAN, "Active" ),

    DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),
    DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
    DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_blockphysexplosion, CZMEntTriggerBlockPhysExp );


CZMEntTriggerBlockPhysExp::CZMEntTriggerBlockPhysExp()
{
    g_ZMBlockPhysExp.AddToTail( this );
}

CZMEntTriggerBlockPhysExp::~CZMEntTriggerBlockPhysExp()
{
    g_ZMBlockPhysExp.FindAndRemove( this );
}

void CZMEntTriggerBlockPhysExp::Spawn()
{
    BaseClass::Spawn();

    InitTrigger();
}

void CZMEntTriggerBlockPhysExp::InputToggle( inputdata_t &inputData )
{
    m_bActive = !m_bActive;
}

void CZMEntTriggerBlockPhysExp::InputEnable( inputdata_t &inputData )
{
    m_bActive = true;
}

void CZMEntTriggerBlockPhysExp::InputDisable( inputdata_t &inputData )
{
    m_bActive = false;
}


/*
    Give resources
*/
#define SF_GIVERES_LIMIT            ( 1 << 0 )

class CZMEntGiveResources : public CServerOnlyPointEntity
{
public:
    DECLARE_CLASS( CZMEntGiveResources, CServerOnlyPointEntity )
    DECLARE_DATADESC()


    void Spawn();

    void InputGiveResources( inputdata_t &inputData );
};

BEGIN_DATADESC( CZMEntGiveResources )
    DEFINE_INPUTFUNC( FIELD_INTEGER, "GiveResources", InputGiveResources ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( func_giveresources, CZMEntGiveResources );


void CZMEntGiveResources::Spawn()
{
    SetSolid( SOLID_NONE );
    AddEffects( EF_NODRAW );
    SetMoveType( MOVETYPE_NONE );
}

void CZMEntGiveResources::InputGiveResources( inputdata_t& inputData )
{
    CZMRules::RewardResources( inputData.value.Int(), HasSpawnFlags( SF_GIVERES_LIMIT ) );
}


/*
    Trigger give points
*/
class CZMEntTriggerGivePoints : public CBaseTrigger
{
public:
    DECLARE_CLASS( CZMEntTriggerGivePoints, CBaseTrigger )
    DECLARE_DATADESC()


    void Spawn();

    void InputAward( inputdata_t &inputData );
};

BEGIN_DATADESC( CZMEntTriggerGivePoints )
    DEFINE_INPUTFUNC( FIELD_INTEGER, "Award", InputAward ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_givepoints, CZMEntTriggerGivePoints );


void CZMEntTriggerGivePoints::Spawn()
{
    BaseClass::Spawn();

    InitTrigger();
}

void CZMEntTriggerGivePoints::InputAward( inputdata_t &inputData )
{
    touchlink_t* root = static_cast<touchlink_t*>( GetDataObject( TOUCHLINK ) );

    if ( !root )
    {
        DevMsg( "No touch link found for trigger_givepoints!\n" );
        return;
    }


    int award = inputData.value.Int();

    for ( touchlink_t* link = root->nextLink; link != root; link = link->nextLink )
    {
        CZMPlayer* pPlayer = ToZMPlayer( link->entityTouched.Get() );

        if ( !pPlayer ) continue;


        if ( pPlayer->IsHuman() && pPlayer->IsAlive() )
        {
            pPlayer->IncrementFragCount( award );
        }
    }
}

/*
    Phys explosion
*/
BEGIN_DATADESC( CZMPhysExplosion )
    DEFINE_THINKFUNC( DelayThink ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( env_delayed_physexplosion, CZMPhysExplosion );
PRECACHE_REGISTER( env_delayed_physexplosion );


ConVar zm_sv_physexp_debug( "zm_sv_physexp_debug", "0" );
ConVar zm_sv_physexp_disorientateplayer( "zm_sv_physexp_disorientateplayer", "10", FCVAR_NOTIFY | FCVAR_ARCHIVE, "Maximum angular disorientation applied to players." );
ConVar zm_sv_physexp_player_mult( "zm_sv_physexp_player_mult", "0.05", FCVAR_NOTIFY | FCVAR_ARCHIVE, "How much players are pushed." );


CZMPhysExplosion::CZMPhysExplosion()
{
    m_hSpark = nullptr;
}

CZMPhysExplosion::~CZMPhysExplosion()
{
    if ( m_hSpark.Get() )
    {
        UTIL_Remove( m_hSpark );
    }
}

CZMPhysExplosion* CZMPhysExplosion::CreatePhysExplosion( const Vector& pos, float delay, float magnitude, float radius )
{
    auto* pExp = dynamic_cast<CZMPhysExplosion*>( CreateEntityByName( "env_delayed_physexplosion" ) );

    if ( !pExp )
    {
        return nullptr;
    }



    pExp->SetMagnitude( magnitude );
    pExp->SetRadius( radius );


    if ( DispatchSpawn( pExp ) != 0 )
    {
        UTIL_RemoveImmediate( pExp );
        return nullptr;
    }


    pExp->Teleport( &pos, nullptr, nullptr );
    pExp->Activate();
    pExp->DelayedExplode( delay );

    return pExp;
}

void CZMPhysExplosion::Spawn()
{
    BaseClass::Spawn();
}

void CZMPhysExplosion::Precache()
{
    PrecacheScriptSound( "ZMPower.PhysExplode_Buildup" );
    PrecacheScriptSound( "ZMPower.PhysExplode_Boom" );
}

void CZMPhysExplosion::DelayThink()
{
    CPASAttenuationFilter filter( this, 1.0f );
    
    EmitSound_t snd;
    snd.m_pSoundName = "ZMPower.PhysExplode_Boom";
    snd.m_pOrigin = &GetAbsOrigin();
    EmitSound( filter, entindex(), snd );

    g_pEffects->Sparks( GetAbsOrigin(), 10, 5 );
    g_pEffects->Sparks( GetAbsOrigin(), 15, 3 );


    Push();

    UTIL_Remove( this );
}

void CZMPhysExplosion::Push()
{
    CBaseEntity* pEnt = nullptr;
    Vector src = GetAbsOrigin();
    Vector end;

    float flRadius = MAX( GetRadius(), 1.0f );


    while ( (pEnt = gEntList.FindEntityInSphere( pEnt, src, flRadius )) != nullptr )
    {
        // Physics objects
        bool bValid = pEnt->GetMoveType() == MOVETYPE_VPHYSICS;

        if ( !bValid )
        {
            // Alive players
            bValid = pEnt->IsPlayer() && pEnt->GetTeamNumber() == ZMTEAM_HUMAN && pEnt->IsAlive();

            if ( !bValid )
                continue;
        }


        end = pEnt->BodyTarget( src );



        Vector dir = (end - src);
        float dist = dir.NormalizeInPlace();


        float ratio = 1.0f - dist / flRadius;
        if ( ratio < 0.0f )
            ratio = 0.0f;



        IPhysicsObject* pPhys = pEnt->VPhysicsGetObject();


        //
        // Player
        //
        if ( pEnt->IsPlayer() )
        {
            auto* pPlayer = ToZMPlayer( pEnt );

            //
            // Disorient the player
            //
            if ( zm_sv_physexp_disorientateplayer.GetFloat() > 0.0f )
            {
                float f = zm_sv_physexp_disorientateplayer.GetFloat();

                QAngle ang;
                ang.x = random->RandomInt( -f, f );
                ang.y = random->RandomInt( -f, f );
                ang.z = 0.0f;

                pPlayer->SnapEyeAngles( pPlayer->EyeAngles() + ang );
                pPlayer->ViewPunch( ang );
            }


            //
            // Push em.
            //
            if ( zm_sv_physexp_player_mult.GetFloat() > 0.0f )
            {
                float force = ratio * GetMagnitude() * zm_sv_physexp_player_mult.GetFloat();
                force *= phys_pushscale.GetFloat();


                Vector push = dir * force;

                if ( pEnt->GetFlags() & FL_BASEVELOCITY )
                {
                    push += pEnt->GetBaseVelocity();
                }

                pEnt->SetGroundEntity( nullptr );
                pEnt->SetBaseVelocity( push );
                pEnt->AddFlag( FL_BASEVELOCITY );
            }


            pPlayer->ForceDropOfCarriedPhysObjects( nullptr );
        }
        //
        // Physics object
        //
        else if ( pPhys )
        {
            float force = GetMagnitude() * ratio;
            if ( force < 1.0f )
                force = 1.0f;


            CTakeDamageInfo info( this, this, force, DMG_BLAST );
            CalculateExplosiveDamageForce( &info, dir, src );

            //force *= phys_pushscale.GetFloat();
            //info.SetDamagePosition( src );
            //info.SetDamageForce( dir * force );

            if ( pPhys->GetGameFlags() & FVPHYSICS_PLAYER_HELD )
            {
                auto* pOwner = pEnt->GetOwnerEntity();
                if ( pOwner && pOwner->IsPlayer() )
                {
                    auto* pPlayer = ToZMPlayer( pOwner );

                    pPlayer->ForceDropOfCarriedPhysObjects( pEnt );
                }
            }

            pEnt->VPhysicsTakeDamage( info );
        }
    }
}

void CZMPhysExplosion::CreateEffects( float delay )
{
    CPASAttenuationFilter filter( this, 1.0f );

    EmitSound_t snd;
    snd.m_pSoundName = "ZMPower.PhysExplode_Buildup";
    snd.m_pOrigin = &GetAbsOrigin();
    EmitSound( filter, entindex(), snd );



    auto* pSpark = dynamic_cast<CEnvSpark*>( CreateEntityByName( "env_spark" ) );

    if ( !pSpark ) return;


    const int SF_SPARK_START_ON = 64;
    const int SF_SPARK_GLOW = 128;
    const int SF_SPARK_SILENT = 256;

    pSpark->AddSpawnFlags( SF_SPARK_START_ON );
    pSpark->AddSpawnFlags( SF_SPARK_GLOW );
    pSpark->AddSpawnFlags( SF_SPARK_SILENT );

    pSpark->CBaseEntity::KeyValue( "MaxDelay", 0.0f );
    pSpark->CBaseEntity::KeyValue( "Magnitude" , 2 );
    pSpark->CBaseEntity::KeyValue( "TrailLength" , 1.5 );
    //pSpark->CBaseEntity::KeyValue( "DeathTime" , (gpGlobals->curtime + delay) );

    if ( DispatchSpawn( pSpark ) != 0 )
    {
        UTIL_RemoveImmediate( pSpark );
        return;
    }

    pSpark->Teleport( &GetAbsOrigin(), nullptr, nullptr );

    m_hSpark.Set( pSpark );
}

void CZMPhysExplosion::DelayedExplode( float delay )
{
    if ( delay < 0.0f ) delay = 0.0f;

    if ( delay > 0.0f )
        CreateEffects( delay );


    SetThink( &CZMPhysExplosion::DelayThink );
    SetNextThink( gpGlobals->curtime + delay );


    if ( zm_sv_physexp_debug.GetBool() )
    {
        NDebugOverlay::Sphere( GetAbsOrigin(), GetRadius(), 255, 0, 0, false, delay );
    }
}

/*
    Score Entity
*/
#define SF_SCORE_NEGATIVE       0x0001
#define SF_SCORE_TEAM           0x0002

class CZMEntTeamScore : public CServerOnlyPointEntity
{
public:
    DECLARE_CLASS( CZMEntTeamScore, CServerOnlyPointEntity )
    DECLARE_DATADESC()


    void Spawn() OVERRIDE;


    void InputApplyScore( inputdata_t& inputData );
    void InputApplyScoreZM( inputdata_t& inputData );
    void InputApplyScoreSurvivors( inputdata_t& inputData );


    inline bool AllowNegative() { return ( m_spawnflags & SF_SCORE_NEGATIVE ) ? true : false; };
    inline bool ApplyToTeam() { return ( m_spawnflags & SF_SCORE_TEAM ) ? true : false; };

private:
    void ApplyScore( CTeam* team );
    void ApplyScoreActivator( CBaseEntity* pEnt );


    int m_nPoints;
};

BEGIN_DATADESC( CZMEntTeamScore )
    DEFINE_KEYFIELD( m_nPoints, FIELD_INTEGER, "points" ),

    DEFINE_INPUTFUNC( FIELD_VOID, "ApplyScore", InputApplyScore ),
    DEFINE_INPUTFUNC( FIELD_VOID, "ApplyScoreZM", InputApplyScoreZM ),
    DEFINE_INPUTFUNC( FIELD_VOID, "ApplyScoreSurvivors", InputApplyScoreSurvivors ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( game_score_team, CZMEntTeamScore );

void CZMEntTeamScore::Spawn()
{
}

void CZMEntTeamScore::ApplyScore( CTeam* team )
{
    if ( !team ) return;


    CZMPlayer* pPlayer;
    for ( int i = 1; i <= gpGlobals->maxClients; i++ )
    {
        pPlayer = ToZMPlayer( UTIL_PlayerByIndex( i ) );

        if ( !pPlayer ) continue;


        if ( pPlayer->GetTeam() == team )
        {
            pPlayer->AddPoints( m_nPoints, AllowNegative() );
        }
    }
}

void CZMEntTeamScore::ApplyScoreActivator( CBaseEntity* pEnt )
{
    if ( !pEnt ) return;


    if ( !pEnt->IsPlayer() )
    {
        IPhysicsObject* pObj = pEnt->VPhysicsGetObject();

        if ( !pObj || !(pObj->GetGameFlags() & FVPHYSICS_PLAYER_HELD) ) return;

        if ( !pEnt->GetOwnerEntity() ) return;


        pEnt = pEnt->GetOwnerEntity();
    }

    if ( ApplyToTeam() )
    {
        ApplyScore( pEnt->GetTeam() );
    }
    else
    {
        pEnt->AddPoints( m_nPoints, AllowNegative() );
    }
    
}

void CZMEntTeamScore::InputApplyScore( inputdata_t& inputData )
{
    ApplyScoreActivator( inputData.pActivator );
}

void CZMEntTeamScore::InputApplyScoreZM( inputdata_t& inputData )
{
    ApplyScore( GetGlobalTeam( ZMTEAM_ZM ) );
}

void CZMEntTeamScore::InputApplyScoreSurvivors( inputdata_t& inputData )
{
    ApplyScore( GetGlobalTeam( ZMTEAM_HUMAN ) );
}


/*
    Spawn points
*/
static const char* g_szSpawnpoints[] =
{
    "info_player_deathmatch",
    "info_player_survivor",
    "info_player_zombiemaster",
    "info_player_start"
};

BEGIN_DATADESC( CZMEntSpawnPoint )
    DEFINE_KEYFIELD( m_bIsEnabled, FIELD_BOOLEAN, "enabled" ),

    DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
    DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
    DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),
END_DATADESC()

CZMEntSpawnPoint::CZMEntSpawnPoint()
{
    m_bIsEnabled = true;
}

void CZMEntSpawnPoint::InputEnable( inputdata_t& inputData )
{
    m_bIsEnabled = true;
}

void CZMEntSpawnPoint::InputDisable( inputdata_t& inputData )
{
    m_bIsEnabled = false;
}

void CZMEntSpawnPoint::InputToggle( inputdata_t& inputData )
{
    m_bIsEnabled = !m_bIsEnabled;
}

LINK_ENTITY_TO_CLASS( info_player_start, CZMEntSpawnPoint );
LINK_ENTITY_TO_CLASS( info_player_deathmatch, CZMEntSpawnPoint );
LINK_ENTITY_TO_CLASS( info_player_survivor, CZMEntSpawnPoint );
LINK_ENTITY_TO_CLASS( info_player_zombiemaster, CZMEntSpawnPoint );



/*
    ZM fog controller
*/
static ConVar zm_sv_zmfog_enabled( "zm_sv_zmfog_enabled", "0", 0 );
static ConVar zm_sv_zmfog_color( "zm_sv_zmfog_color", "14 0 0", 0 );
static ConVar zm_sv_zmfog_density( "zm_sv_zmfog_density", "1", 0 );
static ConVar zm_sv_zmfog_start( "zm_sv_zmfog_start", "1200", 0 );
static ConVar zm_sv_zmfog_end( "zm_sv_zmfog_end", "2000", 0 );
static ConVar zm_sv_zmfog_farz( "zm_sv_zmfog_farz", "-1", 0 );
static ConVar zm_sv_zmfog_farz_skybox( "zm_sv_zmfog_farz_skybox", "-1", 0 );

BEGIN_DATADESC( CZMEntFogController )
END_DATADESC()

LINK_ENTITY_TO_CLASS( env_fog_controller_zm, CZMEntFogController );


CZMEntFogController::CZMEntFogController()
{
    m_bNeedsInit = false;
}

bool CZMEntFogController::IsEnabled()
{
    return zm_sv_zmfog_enabled.GetBool();
}

void CZMEntFogController::InitFog()
{
    if ( !m_bNeedsInit )
        return;


    color32 clr;
    int tempclr[3];
    sscanf( zm_sv_zmfog_color.GetString(), "%i %i %i", &tempclr[0], &tempclr[1], &tempclr[2] );
    clr.r = tempclr[0]; clr.g = tempclr[1]; clr.b = tempclr[2]; clr.a = 255;


    m_fog.enable = true;
    m_fog.blend = false;

    m_fog.colorPrimary = m_fog.colorSecondary = clr;

    m_fog.start = zm_sv_zmfog_start.GetFloat();
    m_fog.end = zm_sv_zmfog_end.GetFloat();

    m_fog.maxdensity = zm_sv_zmfog_density.GetFloat();
    m_fog.farz = zm_sv_zmfog_farz.GetFloat();

    m_fog.lerptime = -1.0f;

    m_flSkyboxFarZ = zm_sv_zmfog_farz_skybox.GetFloat();
}

/*
    Brush spawn volume
*/
BEGIN_DATADESC( CZMEntTriggerSpawnVolume )
    DEFINE_KEYFIELD( m_bActive, FIELD_BOOLEAN, "Active" ),

    DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),
    DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
    DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),

    DEFINE_OUTPUT( m_OnSpawnNPC, "OnSpawnNPC" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( trigger_zombiespawnvolume, CZMEntTriggerSpawnVolume );


CZMEntTriggerSpawnVolume::CZMEntTriggerSpawnVolume()
{

}

CZMEntTriggerSpawnVolume::~CZMEntTriggerSpawnVolume()
{

}

void CZMEntTriggerSpawnVolume::Spawn()
{
    BaseClass::Spawn();

    InitTrigger();
}

void CZMEntTriggerSpawnVolume::InputToggle( inputdata_t &inputData )
{
    m_bActive = !m_bActive;
}

void CZMEntTriggerSpawnVolume::InputEnable( inputdata_t &inputData )
{
    m_bActive = true;
}

void CZMEntTriggerSpawnVolume::InputDisable( inputdata_t &inputData )
{
    m_bActive = false;
}

void CZMEntTriggerSpawnVolume::GetPositionWithin( const CBaseEntity* pEnt, Vector& pos )
{
    auto* pNonConst = const_cast<CBaseEntity*>( pEnt );

    auto* pModel = pNonConst->GetModel();
    if ( !pModel )
    {
        Assert( 0 );
        return;
    }


    float size = modelinfo->GetModelRadius( pModel ) + 10.0f;


    Vector start, dir;
    QAngle ang;
    Ray_t ray;
    trace_t tr;

    
    Vector origin = pEnt->GetAbsOrigin();
    
    // Try a few times.
    for ( int i = 0; i < 3; i++ )
    {
        ang.x = random->RandomFloat( -20.0f, 20.0f );
        ang.y = random->RandomFloat( -180.0f, 180.0f );
        ang.z = 0.0f;

        AngleVectors( ang, &dir );



        start = origin + dir * size;

        // Trace inwards to find a position on the surface.
        ray.Init( start, origin );

        enginetrace->ClipRayToEntity( ray, MASK_SOLID, pNonConst, &tr );
        if ( tr.fraction == 1.0f )
            continue;

        Assert( tr.m_pEnt == pEnt );


        // Get a random position between the surface and the origin.
        // This assumes we're convex.
        float dist = (tr.endpos - origin).Length();

        float mult = random->RandomFloat( 0.1f, 0.95f );

        pos = origin + dir * dist * mult;



        // Trace to find a valid spot. In case the brush is enveloped by world brushes.
        const Vector mins( -16, -16, 0 );
        const Vector maxs( 16, 16, 72 );

        trace_t tr;
        CTraceFilterNoNPCsOrPlayer filter( pEnt, COLLISION_GROUP_NONE );
        UTIL_TraceHull( origin, pos, mins, maxs, MASK_NPCSOLID, &filter, &tr );

        pos = tr.endpos;

        break;
    }
}

void CZMEntTriggerSpawnVolume::GetPositionWithin( Vector& pos ) const
{
    GetPositionWithin( this, pos );
}

/*
    Precipitation
*/
class CZMEntPrecipitation : public CBaseEntity
{
public:
    DECLARE_CLASS( CZMEntPrecipitation, CBaseEntity );
    DECLARE_DATADESC();
    DECLARE_SERVERCLASS();

    CZMEntPrecipitation();
    ~CZMEntPrecipitation();


    virtual void Spawn() OVERRIDE;
    virtual int UpdateTransmitState() OVERRIDE;

private:
    CNetworkVar( PrecipitationType_t, m_nPrecipType );
};

LINK_ENTITY_TO_CLASS( func_precipitation, CZMEntPrecipitation );

BEGIN_DATADESC( CZMEntPrecipitation )
    DEFINE_KEYFIELD( m_nPrecipType, FIELD_INTEGER, "preciptype" ),
END_DATADESC()

// Just send the normal entity crap
IMPLEMENT_SERVERCLASS_ST( CZMEntPrecipitation, DT_ZM_EntPrecipitation)
    SendPropInt( SENDINFO( m_nPrecipType ), Q_log2( NUM_PRECIPITATION_TYPES ) + 1, SPROP_UNSIGNED ),
END_SEND_TABLE()


CZMEntPrecipitation::CZMEntPrecipitation()
{
    m_nPrecipType = PRECIPITATION_TYPE_RAIN;
}

CZMEntPrecipitation::~CZMEntPrecipitation()
{
}

void CZMEntPrecipitation::Spawn()
{
    //SetTransmitState( FL_EDICT_ALWAYS );
    SetTransmitState( FL_EDICT_PVSCHECK );

    PrecacheMaterial( "effects/fleck_ash1" );
    PrecacheMaterial( "effects/fleck_ash2" );
    PrecacheMaterial( "effects/fleck_ash3" );
    PrecacheMaterial( "effects/ember_swirling001" );
    Precache();


    // Default to rain.
    if ( m_nPrecipType < 0 || m_nPrecipType > NUM_PRECIPITATION_TYPES )
        m_nPrecipType = PRECIPITATION_TYPE_RAIN;


    SetMoveType( MOVETYPE_NONE );
    SetModel( STRING( GetModelName() ) );		// Set size
    //if ( m_nPrecipType == PRECIPITATION_TYPE_PARTICLERAIN )
    {
        SetSolid( SOLID_VPHYSICS );
        AddSolidFlags( FSOLID_NOT_SOLID );
        AddSolidFlags( FSOLID_FORCE_WORLD_ALIGNED );
        VPhysicsInitStatic();
    }
    //else
    //{
    //    SetSolid( SOLID_NONE );							// Remove model & collisions
    //}

    m_nRenderMode = kRenderEnvironmental;

    // This fixes us casting shadows on client.
    // There may be many different models which would then attempt to cast shadows costing us precious shadow slots.
    AddEffects( EF_NODRAW );
}

int CZMEntPrecipitation::UpdateTransmitState()
{
    return SetTransmitState( FL_EDICT_ALWAYS );
}
