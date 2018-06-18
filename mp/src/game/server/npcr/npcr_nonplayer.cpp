#include "cbase.h"
#include "animation.h"

#include "npcr_motor_nonplayer.h"
#include "npcr_nonplayer.h"
#include "npcr_schedule.h"
#include "npcr_senses.h"


BEGIN_DATADESC( CNPCRNonPlayer )
    DEFINE_OUTPUT( m_OnDamaged, "OnDamaged" ),
    DEFINE_OUTPUT( m_OnDamagedByPlayer, "OnDamagedByPlayer" ),
    DEFINE_OUTPUT( m_OnDeath, "OnDeath" ),
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

    // This flag makes sure we can touch NPC triggers.
    AddFlag( FL_NPC );
    

    
    InitBoneControllers(); 
    SetActivity( ACT_IDLE );


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

    // Don't fire this multiple times a frame.
    if ( m_flLastDamageTime != gpGlobals->curtime )
    {
        CBaseEntity* pAttacker = info.GetAttacker();

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


    return 1;
}

void CNPCRNonPlayer::Event_Killed( const CTakeDamageInfo& info )
{
    m_OnDeath.FireOutput( info.GetAttacker(), this );



    BaseClass::Event_Killed( info );
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
        //VPhysicsGetObject()->UpdateShadow( *pNewPosition, vec3_angle, true, GetUpdateInterval() );
        VPhysicsGetObject()->UpdateShadow( *pNewPosition, vec3_angle, true, 0.0f );
        
        // This will not apply any force to objects nearby.
        // Will get players stuck (if called alone?)
        VPhysicsGetObject()->SetPosition( *pNewPosition, vec3_angle, true );
    }
}

void CNPCRNonPlayer::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{

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
    int iNewSeq = hdr->SelectWeightedSequence( act, GetSequence() );

    if ( iNewSeq <= -1 )
    {
        DevMsg( "Couldn't find sequence for activity %i!\n", act );
        return false;
    }


    Activity last = m_iCurActivity;

    if ( !IsSequenceFinished() )
        OnAnimActivityInterrupted( act );


    //if ( GetSequence() != iNewSeq )
    SetCycle( 0.0f );

    ResetSequence( iNewSeq );
    m_bCurActivityLoops = SequenceLoops();

    m_iCurActivity = act;
    m_iLastActivity = last;
    m_iLastLoopActivity = ACT_INVALID;

    return true;
}

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

float CNPCRNonPlayer::GetMoveActivityMovementSpeed()
{
    // Just assume walk by default
    int iSeq = SelectWeightedSequence( ACT_WALK );

    return GetSequenceGroundSpeed( iSeq );
}
