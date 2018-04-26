#include "cbase.h"
#include "animation.h"

#include "npcr_motor_nonplayer.h"
#include "npcr_nonplayer.h"
#include "npcr_schedule.h"
#include "npcr_senses.h"


NPCR::CBaseNonPlayer::CBaseNonPlayer() : NPCR::CBaseNPC( this )
{
    m_bCurActivityLoops = false;
    m_iCurActivity = ACT_INVALID;
    m_iLastActivity = ACT_INVALID;
    m_iLastLoopActivity = ACT_INVALID;

    m_hEnemy.Set( nullptr );



    m_iHealth = 0; // Set to 0 here, since keyvalues might set this to something else.
}

NPCR::CBaseNonPlayer::~CBaseNonPlayer()
{
}

void NPCR::CBaseNonPlayer::PostConstructor( const char* szClassname )
{
    BaseClass::PostConstructor( szClassname );

    NPCR::CBaseNPC::PostConstructor();
}

NPCR::CBaseMotor* NPCR::CBaseNonPlayer::CreateMotor()
{
    return new CNonPlayerMotor( this );
}

void NPCR::CBaseNonPlayer::Spawn()
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


    SetThink( &NPCR::CBaseNonPlayer::NPCThink );
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

int NPCR::CBaseNonPlayer::OnTakeDamage_Alive( const CTakeDamageInfo& info )
{
    int ret = BaseClass::OnTakeDamage_Alive( info );

    if ( ret )
    {
        OnDamaged( info );
    }

    return ret;
}

void NPCR::CBaseNonPlayer::SetDefaultEyeOffset()
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

void NPCR::CBaseNonPlayer::HandleAnimEvent( animevent_t* pEvent )
{
    m_bHandledAnimEvent = false;

    OnAnimEvent( pEvent );

    if ( !m_bHandledAnimEvent )
        BaseClass::HandleAnimEvent( pEvent );
}

void NPCR::CBaseNonPlayer::NPCThink()
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

void NPCR::CBaseNonPlayer::VPhysicsUpdate( IPhysicsObject* pPhys )
{
    float prevImpactScale = m_impactEnergyScale;

    // Scale the impact scale WAAYY down. It is VERY easy to get killed by other physics objects otherwise.
    m_impactEnergyScale *= 0.1f;
    ApplyStressDamage( pPhys, true );
    m_impactEnergyScale = prevImpactScale;
}

void NPCR::CBaseNonPlayer::PerformCustomPhysics( Vector* pNewPosition, Vector* pNewVelocity, QAngle* pNewAngles, QAngle* pNewAngVelocity )
{
    SetGroundEntity( GetMotor()->GetGroundEntity() );

    if ( VPhysicsGetObject() )
    {
        //VPhysicsGetObject()->UpdateShadow( *pNewPosition, vec3_angle, true, GetUpdateInterval() );
        VPhysicsGetObject()->UpdateShadow( *pNewPosition, vec3_angle, true, 0.0f );
        
        // This will not apply any force to objects nearby.
        // Will get players stuck
        //VPhysicsGetObject()->SetPosition( *pNewPosition, vec3_angle, true );
    }
}

void NPCR::CBaseNonPlayer::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{

}

bool NPCR::CBaseNonPlayer::SetActivity( Activity act )
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

bool NPCR::CBaseNonPlayer::HasActivity( Activity act )
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

float NPCR::CBaseNonPlayer::GetMoveActivityMovementSpeed()
{
    // Just assume walk by default
    int iSeq = SelectWeightedSequence( ACT_WALK );

    return GetSequenceGroundSpeed( iSeq );
}
