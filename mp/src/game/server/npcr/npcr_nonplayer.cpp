#include "cbase.h"
#include "animation.h"

#include "npcr_motor_nonplayer.h"
#include "npcr_nonplayer.h"
#include "npcr_schedule.h"
#include "npcr_senses.h"


BEGIN_DATADESC( CNPCRNonPlayer )
    // Outputs
    DEFINE_OUTPUT( m_OnDamaged, "OnDamaged" ),
    DEFINE_OUTPUT( m_OnDamagedByPlayer, "OnDamagedByPlayer" ),
    DEFINE_OUTPUT( m_OnHalfHealth, "OnHalfHealth" ),
    DEFINE_OUTPUT( m_OnDeath, "OnDeath" ),
    DEFINE_OUTPUT( m_OnFoundPlayer, "OnFoundPlayer" ),
    DEFINE_OUTPUT( m_OnFoundEnemy, "OnFoundEnemy" ),
    DEFINE_OUTPUT( m_OnLostPlayerLOS, "OnLostPlayerLOS" ),
    DEFINE_OUTPUT( m_OnLostEnemyLOS, "OnLostEnemyLOS" ),
    DEFINE_OUTPUT( m_OnLostEnemy, "OnLostEnemy" ),
    // Inputs
    DEFINE_INPUTFUNC( FIELD_INTEGER, "SetHealth", InputSetHealth ),
END_DATADESC()


CNPCRNonPlayer::CNPCRNonPlayer() : NPCR::CBaseNPC( this )
{
    m_bCurActivityLoops = false;
    m_iCurActivity = ACT_INVALID;
    m_iLastActivity = ACT_INVALID;
    m_iLastLoopActivity = ACT_INVALID;

    m_hEnemy.Set( nullptr );



    m_iHealth = 0; // Set to 0 here, since keyvalues might set this to something else.




    m_flLastDamageTime = 0.0f;

    m_flNextLOSOutputs = 0.0f;
    m_bDidSeeEnemyLastTime = false;
}

CNPCRNonPlayer::~CNPCRNonPlayer()
{
}

void CNPCRNonPlayer::PostConstructor( const char* szClassname )
{
    BaseClass::PostConstructor( szClassname );

    NPCR::CBaseNPC::PostConstructor();
}

NPCR::CBaseMotor* CNPCRNonPlayer::CreateMotor()
{
    return new NPCR::CNonPlayerMotor( this );
}

void CNPCRNonPlayer::Spawn()
{
    BaseClass::Spawn();

    SetSolid( SOLID_BBOX );
    AddSolidFlags( FSOLID_NOT_STANDABLE );
    SetMoveType( MOVETYPE_CUSTOM );
    SetCollisionGroup( COLLISION_GROUP_NPC );
    SetBlocksLOS( false );

    // This flag makes sure we can touch NPC triggers.
    AddFlag( FL_NPC );
    

    
    InitBoneControllers(); 


    SetThink( &CNPCRNonPlayer::NPCThink );
    SetNextThink( gpGlobals->curtime );

    
    m_takedamage = DAMAGE_YES;
    Assert( m_iMaxHealth > 0 );
    if ( m_iMaxHealth < 1 )
        m_iMaxHealth = 1;
    if ( m_iHealth < 1 )
        m_iHealth = m_iMaxHealth;


    SetDefaultEyeOffset();

    // Set proper collision bounds.
    // Fixes bots getting stuck on other objects. (players, other bots, props(?))
    Vector mins, maxs;
    maxs.x = maxs.y = GetMotor()->GetHullWidth() / 2.0f;
    maxs.z = GetMotor()->GetHullHeight();
    mins.x = mins.y = -maxs.x;
    mins.z = 0.0f;


    SetCollisionBounds( mins, maxs );


    if ( VPhysicsGetObject() )
    {
        VPhysicsGetObject()->EnableCollisions( false );
        VPhysicsDestroyObject();
    }

	IPhysicsObject* pPhysObj = VPhysicsInitShadow( false, false );
	if ( pPhysObj )
	{
        pPhysObj->EnableMotion( false );
        pPhysObj->EnableGravity( false );

        float mass = 0.0f;
        if ( GetModelPtr() )
        {
            mass = GetModelPtr()->mass();
        }

        if ( mass < 1.0f )
            mass = 1.0f;
        pPhysObj->SetMass( mass );


        IPhysicsShadowController* pController = pPhysObj->GetShadowController();
        pController->SetTeleportDistance( 1.0f );
    }
}

int CNPCRNonPlayer::OnTakeDamage_Alive( const CTakeDamageInfo& info )
{
    if ( !BaseClass::OnTakeDamage_Alive( info ) )
        return 0;


    OnDamaged( info );


    //
    // Fire outputs
    //
    CBaseEntity* pAttacker = info.GetAttacker();

    // Don't fire this multiple times a frame.
    if ( m_flLastDamageTime != gpGlobals->curtime )
    {
        m_OnDamaged.FireOutput( pAttacker, this );

        if ( pAttacker )
        {
            if ( pAttacker->IsPlayer() )
            {
                m_OnDamagedByPlayer.FireOutput( pAttacker, this );
            }
        }



        m_flLastDamageTime = gpGlobals->curtime;
    }

    if ( m_iHealth <= (m_iMaxHealth / 2) )
    {
        m_OnHalfHealth.FireOutput( pAttacker, this );
    }


    return 1;
}

void CNPCRNonPlayer::Event_Killed( const CTakeDamageInfo& info )
{
    m_OnDeath.FireOutput( info.GetAttacker(), this );



    BaseClass::Event_Killed( info );


    // IMPORTANT: We need this check or the entity will never be removed.
    // The ragdolling process will mark us for removal.
    // If you don't get a ragdoll, you never get removed! :(
    if ( info.GetDamageType() & DMG_REMOVENORAGDOLL )
    {
        RemoveDeferred();
    }
}

void CNPCRNonPlayer::SetDefaultEyeOffset()
{
    if  ( GetModelPtr() )
    {
        Vector offset = vec3_origin;
        GetEyePosition( GetModelPtr(), offset );

        if ( offset == vec3_origin )
        {
            VectorAdd( WorldAlignMins(), WorldAlignMaxs(), offset );

            offset *= 0.75;
        }

        SetViewOffset( offset );
    }
}

void CNPCRNonPlayer::HandleAnimEvent( animevent_t* pEvent )
{
    m_bHandledAnimEvent = false;

    OnAnimEvent( pEvent );

    if ( !m_bHandledAnimEvent )
        BaseClass::HandleAnimEvent( pEvent );
}

void CNPCRNonPlayer::PostUpdate()
{
    NPCR::CBaseNPC::PostUpdate();


    CBaseEntity* pCurEnemy = GetEnemy();

    if ( pCurEnemy )
    {
        //
        // Fire sight outputs
        //
        if ( m_flNextLOSOutputs <= gpGlobals->curtime )
        {
            const bool bCanSee = GetSenses()->GetEntityOf( pCurEnemy ) != nullptr;
            
            // Player varients are kept for backwards compatibility. (damn you, zm_kink)
            if ( !bCanSee && m_bDidSeeEnemyLastTime )
            {
                if ( pCurEnemy->IsPlayer() )
                    m_OnLostPlayerLOS.FireOutput( pCurEnemy, this );

                m_OnLostEnemyLOS.FireOutput( pCurEnemy, this );

            }
            else if ( bCanSee && !m_bDidSeeEnemyLastTime )
            {
                if ( pCurEnemy->IsPlayer() )
                    m_OnFoundPlayer.Set( m_hEnemy, this, this );

                m_OnFoundEnemy.Set( m_hEnemy, this, this );
            }

            m_flNextLOSOutputs = gpGlobals->curtime + 0.1f;
            m_bDidSeeEnemyLastTime = bCanSee;
        }
    }
}

void CNPCRNonPlayer::NPCThink()
{
    SetNextThink( gpGlobals->curtime );


    UpdateLastKnownArea();


    CBaseNPC::Update();

    
    // Make sure we don't send multiple OnAnimActivityFinished() to schedules in one "update".
    if ( IsSequenceFinished() && (SequenceLoops() || m_iCurActivity != m_iLastLoopActivity) )
    {
        Activity last = m_iCurActivity;
        OnAnimActivityFinished( m_iCurActivity );
        m_iLastLoopActivity = last;
    }
}

void CNPCRNonPlayer::VPhysicsUpdate( IPhysicsObject* pPhys )
{
    float prevImpactScale = m_impactEnergyScale;

    // Scale the impact scale WAAYY down. It is VERY easy to get killed by other physics objects otherwise.
    m_impactEnergyScale *= 0.1f;
    ApplyStressDamage( pPhys, true );
    m_impactEnergyScale = prevImpactScale;
}

void CNPCRNonPlayer::PerformCustomPhysics( Vector* pNewPosition, Vector* pNewVelocity, QAngle* pNewAngles, QAngle* pNewAngVelocity )
{
    SetGroundEntity( GetMotor()->GetGroundEntity() );

    if ( VPhysicsGetObject() )
    {
        //VPhysicsGetObject()->UpdateShadow( *pNewPosition, vec3_angle, true, 0.0f );
        VPhysicsGetObject()->UpdateShadow( *pNewPosition, vec3_angle, false, GetUpdateInterval() );
        
        // This will not apply any force to objects nearby.
        // Will get players stuck (if called alone?)
        VPhysicsGetObject()->SetPosition( *pNewPosition, vec3_angle, true );
    }
}

float CNPCRNonPlayer::GetMoveActivityMovementSpeed()
{
    // Just assume walk by default
    int iSeq = SelectWeightedSequence( ACT_WALK );

    return GetSequenceGroundSpeed( iSeq );
}

void CNPCRNonPlayer::AcquireEnemy( CBaseEntity* pEnemy )
{
    Assert( pEnemy != nullptr );

    SetEnemy( pEnemy );


    // Perform sight outputs.
    m_bDidSeeEnemyLastTime = false;
}

void CNPCRNonPlayer::LostEnemy()
{
    CBaseEntity* pOldEnemy = GetEnemy();

    SetEnemy( nullptr );

    m_OnLostEnemy.FireOutput( pOldEnemy, this );
}

void CNPCRNonPlayer::InputSetHealth( inputdata_t& inputdata )
{
    int iOldHealth = GetHealth();
    int iNewHealth = inputdata.value.Int();
    int iDelta = abs( iOldHealth - iNewHealth );
    if ( iNewHealth > iOldHealth )
    {
        TakeHealth( iDelta, DMG_GENERIC );
    }
    else if ( iNewHealth < iOldHealth )
    {
        TakeDamage( CTakeDamageInfo( this, this, iDelta, DMG_GENERIC ) );
    }
}
