#include "cbase.h"

#ifdef CLIENT_DLL
    //#include "c_hl2mp_player.h"
    #include "vcollide_parse.h"
    //#include "engine/ivdebugoverlay.h"
    #include "iviewrender_beams.h"
    //#include "beamdraw.h"
    //#include "c_te_effect_dispatch.h"
    #include "model_types.h"
    #include "clienteffectprecachesystem.h"
    #include "fx_interpvalue.h"
#else
    #include "soundent.h"
    #include "ndebugoverlay.h"
    #include "ai_basenpc.h"
    #include "player_pickup.h"
    #include "physics_prop_ragdoll.h"
    #include "globalstate.h"
    #include "props.h"
    #include "te_effect_dispatch.h"
    #include "util.h"
#endif

//#include "gamerules.h"
//#include "soundenvelope.h"
//#include "engine/IEngineSound.h"
//#include "physics.h"
#include "in_buttons.h"
//#include "IEffects.h"
//#include "shake.h"
//#include "beam_shared.h"
//#include "Sprite.h"
//#include "physics_saverestore.h"
//#include "movevars_shared.h"
//#include "weapon_hl2mpbasehlmpcombatweapon.h"
#include "vphysics/friction.h"
#include "debugoverlay_shared.h"


#include "zmr_base.h"
#include "zmr_carry.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



static const char *s_pWaitForUpgradeContext = "WaitForUpgrade";


ConVar	g_debug_physcannon( "g_debug_physcannon", "0", FCVAR_REPLICATED | FCVAR_CHEAT );

ConVar physcannon_minforce( "physcannon_minforce", "50", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar physcannon_maxforce( "physcannon_maxforce", "300", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar physcannon_maxmass( "physcannon_maxmass", "40", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar physcannon_tracelength( "physcannon_tracelength", "75", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar physcannon_chargetime("physcannon_chargetime", "2", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar physcannon_pullforce( "physcannon_pullforce", "4000", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar physcannon_cone( "physcannon_cone", "0.97", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar physcannon_ball_cone( "physcannon_ball_cone", "0.997", FCVAR_REPLICATED | FCVAR_CHEAT );
ConVar player_throwforce( "player_throwforce", "2000", FCVAR_REPLICATED | FCVAR_CHEAT );

#ifndef CLIENT_DLL
extern ConVar hl2_normspeed;
extern ConVar hl2_walkspeed;
#endif

#define PHYSCANNON_BEAM_SPRITE "sprites/orangelight1.vmt"
#define PHYSCANNON_BEAM_SPRITE_NOZ "sprites/orangelight1_noz.vmt"
#define PHYSCANNON_GLOW_SPRITE "sprites/glow04_noz"
#define PHYSCANNON_ENDCAP_SPRITE "sprites/orangeflare1"
#define PHYSCANNON_CENTER_GLOW "sprites/orangecore1"
#define PHYSCANNON_BLAST_SPRITE "sprites/orangecore2"

#ifdef CLIENT_DLL

    //Precahce the effects
    CLIENTEFFECT_REGISTER_BEGIN( PrecacheEffectPhysCannon )
    CLIENTEFFECT_MATERIAL( "sprites/orangelight1" )
    CLIENTEFFECT_MATERIAL( "sprites/orangelight1_noz" )
    CLIENTEFFECT_MATERIAL( PHYSCANNON_GLOW_SPRITE )
    CLIENTEFFECT_MATERIAL( PHYSCANNON_ENDCAP_SPRITE )
    CLIENTEFFECT_MATERIAL( PHYSCANNON_CENTER_GLOW )
    CLIENTEFFECT_MATERIAL( PHYSCANNON_BLAST_SPRITE )
    CLIENTEFFECT_REGISTER_END()

#endif	// CLIENT_DLL

#ifndef CLIENT_DLL

void PhysCannonBeginUpgrade( CBaseAnimating *pAnim )
{

}

bool PlayerHasMegaPhysCannon( void )
{
    return false;
}

bool PhysCannonAccountableForObject( CBaseCombatWeapon *pPhysCannon, CBaseEntity *pObject )
{
    // BRJ: FIXME! This can't be implemented trivially, so I'm leaving it to Steve or Adrian
    Assert( 0 );
    return false;
}

#endif

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// this will hit skip the pass entity, but not anything it owns 
// (lets player grab own grenades)
class CTraceFilterNoOwnerTest : public CTraceFilterSimple
{
public:
    DECLARE_CLASS( CTraceFilterNoOwnerTest, CTraceFilterSimple );
    
    CTraceFilterNoOwnerTest( const IHandleEntity *passentity, int collisionGroup )
        : CTraceFilterSimple( NULL, collisionGroup ), m_pPassNotOwner(passentity)
    {
    }
    
    virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
    {
        if ( pHandleEntity != m_pPassNotOwner )
            return BaseClass::ShouldHitEntity( pHandleEntity, contentsMask );

        return false;
    }

protected:
    const IHandleEntity *m_pPassNotOwner;
};

static void MatrixOrthogonalize( matrix3x4_t &matrix, int column )
{
    Vector columns[3];
    int i;

    for ( i = 0; i < 3; i++ )
    {
        MatrixGetColumn( matrix, i, columns[i] );
    }

    int index0 = column;
    int index1 = (column+1)%3;
    int index2 = (column+2)%3;

    columns[index2] = CrossProduct( columns[index0], columns[index1] );
    columns[index1] = CrossProduct( columns[index2], columns[index0] );
    VectorNormalize( columns[index2] );
    VectorNormalize( columns[index1] );
    MatrixSetColumn( columns[index1], index1, matrix );
    MatrixSetColumn( columns[index2], index2, matrix );
}

#define SIGN(x) ( (x) < 0 ? -1 : 1 )

static QAngle AlignAngles( const QAngle &angles, float cosineAlignAngle )
{
    matrix3x4_t alignMatrix;
    AngleMatrix( angles, alignMatrix );

    // NOTE: Must align z first
    for ( int j = 3; --j >= 0; )
    {
        Vector vec;
        MatrixGetColumn( alignMatrix, j, vec );
        for ( int i = 0; i < 3; i++ )
        {
            if ( fabs(vec[i]) > cosineAlignAngle )
            {
                vec[i] = SIGN(vec[i]);
                vec[(i+1)%3] = 0;
                vec[(i+2)%3] = 0;
                MatrixSetColumn( vec, j, alignMatrix );
                MatrixOrthogonalize( alignMatrix, j );
                break;
            }
        }
    }

    QAngle out;
    MatrixAngles( alignMatrix, out );
    return out;
}


static void TraceCollideAgainstBBox( const CPhysCollide *pCollide, const Vector &start, const Vector &end, const QAngle &angles, const Vector &boxOrigin, const Vector &mins, const Vector &maxs, trace_t *ptr )
{
    physcollision->TraceBox( boxOrigin, boxOrigin + (start-end), mins, maxs, pCollide, start, angles, ptr );

    if ( ptr->DidHit() )
    {
        ptr->endpos = start * (1-ptr->fraction) + end * ptr->fraction;
        ptr->startpos = start;
        ptr->plane.dist = -ptr->plane.dist;
        ptr->plane.normal *= -1;
    }
}

//-----------------------------------------------------------------------------
// Purpose: Computes a local matrix for the player clamped to valid carry ranges
//-----------------------------------------------------------------------------
// when looking level, hold bottom of object 8 inches below eye level
#define PLAYER_HOLD_LEVEL_EYES	-8

// when looking down, hold bottom of object 0 inches from feet
#define PLAYER_HOLD_DOWN_FEET	2

// when looking up, hold bottom of object 24 inches above eye level
#define PLAYER_HOLD_UP_EYES		24

// use a +/-30 degree range for the entire range of motion of pitch
#define PLAYER_LOOK_PITCH_RANGE	30

// player can reach down 2ft below his feet (otherwise he'll hold the object above the bottom)
#define PLAYER_REACH_DOWN_DISTANCE	24

static void ComputePlayerMatrix( CBasePlayer *pPlayer, matrix3x4_t &out )
{
    if ( !pPlayer )
        return;

    QAngle angles = pPlayer->EyeAngles();
    Vector origin = pPlayer->EyePosition();
    
    // 0-360 / -180-180
    //angles.x = init ? 0 : AngleDistance( angles.x, 0 );
    //angles.x = clamp( angles.x, -PLAYER_LOOK_PITCH_RANGE, PLAYER_LOOK_PITCH_RANGE );
    angles.x = 0;

    float feet = pPlayer->GetAbsOrigin().z + pPlayer->WorldAlignMins().z;
    float eyes = origin.z;
    float zoffset = 0;
    // moving up (negative pitch is up)
    if ( angles.x < 0 )
    {
        zoffset = RemapVal( angles.x, 0, -PLAYER_LOOK_PITCH_RANGE, PLAYER_HOLD_LEVEL_EYES, PLAYER_HOLD_UP_EYES );
    }
    else
    {
        zoffset = RemapVal( angles.x, 0, PLAYER_LOOK_PITCH_RANGE, PLAYER_HOLD_LEVEL_EYES, PLAYER_HOLD_DOWN_FEET + (feet - eyes) );
    }
    origin.z += zoffset;
    angles.x = 0;
    AngleMatrix( angles, origin, out );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

// derive from this so we can add save/load data to it
struct game_shadowcontrol_params_t : public hlshadowcontrol_params_t
{
    DECLARE_SIMPLE_DATADESC();
};

BEGIN_SIMPLE_DATADESC( game_shadowcontrol_params_t )
    
    DEFINE_FIELD( targetPosition,		FIELD_POSITION_VECTOR ),
    DEFINE_FIELD( targetRotation,		FIELD_VECTOR ),
    DEFINE_FIELD( maxAngular, FIELD_FLOAT ),
    DEFINE_FIELD( maxDampAngular, FIELD_FLOAT ),
    DEFINE_FIELD( maxSpeed, FIELD_FLOAT ),
    DEFINE_FIELD( maxDampSpeed, FIELD_FLOAT ),
    DEFINE_FIELD( dampFactor, FIELD_FLOAT ),
    DEFINE_FIELD( teleportDistance,	FIELD_FLOAT ),

END_DATADESC()

//-----------------------------------------------------------------------------
class CGrabController : public IMotionEvent
{
public:

    CGrabController( void );
    ~CGrabController( void );
    void AttachEntity( CBasePlayer *pPlayer, CBaseEntity *pEntity, IPhysicsObject *pPhys, bool bIsMegaPhysCannon, const Vector &vGrabPosition, bool bUseGrabPosition );
    void DetachEntity( bool bClearVelocity );
    void OnRestore();

    bool UpdateObject( CBasePlayer *pPlayer, float flError );

    void SetTargetPosition( const Vector &target, const QAngle &targetOrientation );
    float ComputeError();
    float GetLoadWeight( void ) const { return m_flLoadWeight; }
    void SetAngleAlignment( float alignAngleCosine ) { m_angleAlignment = alignAngleCosine; }
    void SetIgnorePitch( bool bIgnore ) { m_bIgnoreRelativePitch = bIgnore; }
    QAngle TransformAnglesToPlayerSpace( const QAngle &anglesIn, CBasePlayer *pPlayer );
    QAngle TransformAnglesFromPlayerSpace( const QAngle &anglesIn, CBasePlayer *pPlayer );

    CBaseEntity *GetAttached() { return (CBaseEntity *)m_attachedEntity; }

    IMotionEvent::simresult_e Simulate( IPhysicsMotionController *pController, IPhysicsObject *pObject, float deltaTime, Vector &linear, AngularImpulse &angular );
    float GetSavedMass( IPhysicsObject *pObject );

    QAngle			m_attachedAnglesPlayerSpace;
    Vector			m_attachedPositionObjectSpace;

private:
    // Compute the max speed for an attached object
    void ComputeMaxSpeed( CBaseEntity *pEntity, IPhysicsObject *pPhysics );

    game_shadowcontrol_params_t	m_shadow;
    float			m_timeToArrive;
    float			m_errorTime;
    float			m_error;
    float			m_contactAmount;
    float			m_angleAlignment;
    bool			m_bCarriedEntityBlocksLOS;
    bool			m_bIgnoreRelativePitch;

    float			m_flLoadWeight;
    float			m_savedRotDamping[VPHYSICS_MAX_OBJECT_LIST_COUNT];
    float			m_savedMass[VPHYSICS_MAX_OBJECT_LIST_COUNT];
    EHANDLE			m_attachedEntity;
    QAngle			m_vecPreferredCarryAngles;
    bool			m_bHasPreferredCarryAngles;


    IPhysicsMotionController *m_controller;
    int				m_frameCount;
    friend class CZMWeaponCarry;
};

const float DEFAULT_MAX_ANGULAR = 360.0f * 10.0f;
const float REDUCED_CARRY_MASS = 1.0f;

CGrabController::CGrabController( void )
{
    m_shadow.dampFactor = 1.0;
    m_shadow.teleportDistance = 0;
    m_errorTime = 0;
    m_error = 0;
    // make this controller really stiff!
    m_shadow.maxSpeed = 1000;
    m_shadow.maxAngular = DEFAULT_MAX_ANGULAR;
    m_shadow.maxDampSpeed = m_shadow.maxSpeed*2;
    m_shadow.maxDampAngular = m_shadow.maxAngular;
    m_attachedEntity = NULL;
    m_vecPreferredCarryAngles = vec3_angle;
    m_bHasPreferredCarryAngles = false;
}

CGrabController::~CGrabController( void )
{
    DetachEntity( false );
}

void CGrabController::OnRestore()
{
    if ( m_controller )
    {
        m_controller->SetEventHandler( this );
    }
}

void CGrabController::SetTargetPosition( const Vector &target, const QAngle &targetOrientation )
{
    m_shadow.targetPosition = target;
    m_shadow.targetRotation = targetOrientation;

    m_timeToArrive = gpGlobals->frametime;

    CBaseEntity *pAttached = GetAttached();
    if ( pAttached )
    {
        IPhysicsObject *pObj = pAttached->VPhysicsGetObject();
        
        if ( pObj != NULL )
        {
            pObj->Wake();
        }
        else
        {
            DetachEntity( false );
        }
    }
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CGrabController::ComputeError()
{
    if ( m_errorTime <= 0 )
        return 0;

    CBaseEntity *pAttached = GetAttached();
    if ( pAttached )
    {
        Vector pos;
        IPhysicsObject *pObj = pAttached->VPhysicsGetObject();
        
        if ( pObj )
        {	
            pObj->GetShadowPosition( &pos, NULL );

            float error = (m_shadow.targetPosition - pos).Length();
            if ( m_errorTime > 0 )
            {
                if ( m_errorTime > 1 )
                {
                    m_errorTime = 1;
                }
                float speed = error / m_errorTime;
                if ( speed > m_shadow.maxSpeed )
                {
                    error *= 0.5;
                }
                m_error = (1-m_errorTime) * m_error + error * m_errorTime;
            }
        }
        else
        {
            DevMsg( "Object attached to Physcannon has no physics object\n" );
            DetachEntity( false );
            return 9999; // force detach
        }
    }
    
    if ( pAttached->IsEFlagSet( EFL_IS_BEING_LIFTED_BY_BARNACLE ) )
    {
        m_error *= 3.0f;
    }

    m_errorTime = 0;

    return m_error;
}


#define MASS_SPEED_SCALE	60
#define MAX_MASS			40

void CGrabController::ComputeMaxSpeed( CBaseEntity *pEntity, IPhysicsObject *pPhysics )
{
#ifndef CLIENT_DLL
    m_shadow.maxSpeed = 1000;
    m_shadow.maxAngular = DEFAULT_MAX_ANGULAR;

    // Compute total mass...
    float flMass = PhysGetEntityMass( pEntity );
    float flMaxMass = physcannon_maxmass.GetFloat();
    if ( flMass <= flMaxMass )
        return;

    float flLerpFactor = clamp( flMass, flMaxMass, 500.0f );
    flLerpFactor = SimpleSplineRemapVal( flLerpFactor, flMaxMass, 500.0f, 0.0f, 1.0f );

    float invMass = pPhysics->GetInvMass();
    float invInertia = pPhysics->GetInvInertia().Length();

    float invMaxMass = 1.0f / MAX_MASS;
    float ratio = invMaxMass / invMass;
    invMass = invMaxMass;
    invInertia *= ratio;

    float maxSpeed = invMass * MASS_SPEED_SCALE * 200;
    float maxAngular = invInertia * MASS_SPEED_SCALE * 360;

    m_shadow.maxSpeed = Lerp( flLerpFactor, m_shadow.maxSpeed, maxSpeed );
    m_shadow.maxAngular = Lerp( flLerpFactor, m_shadow.maxAngular, maxAngular );
#endif
}


QAngle CGrabController::TransformAnglesToPlayerSpace( const QAngle &anglesIn, CBasePlayer *pPlayer )
{
    if ( m_bIgnoreRelativePitch )
    {
        matrix3x4_t test;
        QAngle angleTest = pPlayer->EyeAngles();
        angleTest.x = 0;
        AngleMatrix( angleTest, test );
        return TransformAnglesToLocalSpace( anglesIn, test );
    }
    return TransformAnglesToLocalSpace( anglesIn, pPlayer->EntityToWorldTransform() );
}

QAngle CGrabController::TransformAnglesFromPlayerSpace( const QAngle &anglesIn, CBasePlayer *pPlayer )
{
    if ( m_bIgnoreRelativePitch )
    {
        matrix3x4_t test;
        QAngle angleTest = pPlayer->EyeAngles();
        angleTest.x = 0;
        AngleMatrix( angleTest, test );
        return TransformAnglesToWorldSpace( anglesIn, test );
    }
    return TransformAnglesToWorldSpace( anglesIn, pPlayer->EntityToWorldTransform() );
}


void CGrabController::AttachEntity( CBasePlayer *pPlayer, CBaseEntity *pEntity, IPhysicsObject *pPhys, bool bIsMegaPhysCannon, const Vector &vGrabPosition, bool bUseGrabPosition )
{
    // play the impact sound of the object hitting the player
    // used as feedback to let the player know he picked up the object
#ifndef CLIENT_DLL
    PhysicsImpactSound( pPlayer, pPhys, CHAN_STATIC, pPhys->GetMaterialIndex(), pPlayer->VPhysicsGetObject()->GetMaterialIndex(), 1.0, 64 );
#endif
    Vector position;
    QAngle angles;
    pPhys->GetPosition( &position, &angles );
    // If it has a preferred orientation, use that instead.
#ifndef CLIENT_DLL
    Pickup_GetPreferredCarryAngles( pEntity, pPlayer, pPlayer->EntityToWorldTransform(), angles );
#endif

//	ComputeMaxSpeed( pEntity, pPhys );

    // Carried entities can never block LOS
    m_bCarriedEntityBlocksLOS = pEntity->BlocksLOS();
    pEntity->SetBlocksLOS( false );
    m_controller = physenv->CreateMotionController( this );
    m_controller->AttachObject( pPhys, true );
    // Don't do this, it's causing trouble with constraint solvers.
    //m_controller->SetPriority( IPhysicsMotionController::HIGH_PRIORITY );

    pPhys->Wake();
    PhysSetGameFlags( pPhys, FVPHYSICS_PLAYER_HELD );
    SetTargetPosition( position, angles );
    m_attachedEntity = pEntity;
    IPhysicsObject *pList[VPHYSICS_MAX_OBJECT_LIST_COUNT];
    int count = pEntity->VPhysicsGetObjectList( pList, ARRAYSIZE(pList) );
    m_flLoadWeight = 0;
    float damping = 10;
    float flFactor = count / 7.5f;
    if ( flFactor < 1.0f )
    {
        flFactor = 1.0f;
    }
    for ( int i = 0; i < count; i++ )
    {
        float mass = pList[i]->GetMass();
        pList[i]->GetDamping( NULL, &m_savedRotDamping[i] );
        m_flLoadWeight += mass;
        m_savedMass[i] = mass;

        // reduce the mass to prevent the player from adding crazy amounts of energy to the system
        pList[i]->SetMass( REDUCED_CARRY_MASS / flFactor );
        pList[i]->SetDamping( NULL, &damping );
    }
    
    // Give extra mass to the phys object we're actually picking up
    pPhys->SetMass( REDUCED_CARRY_MASS );
    pPhys->EnableDrag( false );

    m_errorTime = -1.0f; // 1 seconds until error starts accumulating
    m_error = 0;
    m_contactAmount = 0;

    m_attachedAnglesPlayerSpace = TransformAnglesToPlayerSpace( angles, pPlayer );
    if ( m_angleAlignment != 0 )
    {
        m_attachedAnglesPlayerSpace = AlignAngles( m_attachedAnglesPlayerSpace, m_angleAlignment );
    }

    VectorITransform( pEntity->WorldSpaceCenter(), pEntity->EntityToWorldTransform(), m_attachedPositionObjectSpace );

#ifndef CLIENT_DLL
    // If it's a prop, see if it has desired carry angles
    CPhysicsProp *pProp = dynamic_cast<CPhysicsProp *>(pEntity);
    if ( pProp )
    {
        m_bHasPreferredCarryAngles = pProp->GetPropDataAngles( "preferred_carryangles", m_vecPreferredCarryAngles );
    }
    else
    {
        m_bHasPreferredCarryAngles = false;
    }
#else

    m_bHasPreferredCarryAngles = false;
#endif

}

static void ClampPhysicsVelocity( IPhysicsObject *pPhys, float linearLimit, float angularLimit )
{
    Vector vel;
    AngularImpulse angVel;
    pPhys->GetVelocity( &vel, &angVel );
    float speed = VectorNormalize(vel) - linearLimit;
    float angSpeed = VectorNormalize(angVel) - angularLimit;
    speed = speed < 0 ? 0 : -speed;
    angSpeed = angSpeed < 0 ? 0 : -angSpeed;
    vel *= speed;
    angVel *= angSpeed;
    pPhys->AddVelocity( &vel, &angVel );
}

void CGrabController::DetachEntity( bool bClearVelocity )
{
    CBaseEntity *pEntity = GetAttached();
    if ( pEntity )
    {
        // Restore the LS blocking state
        pEntity->SetBlocksLOS( m_bCarriedEntityBlocksLOS );
        IPhysicsObject *pList[VPHYSICS_MAX_OBJECT_LIST_COUNT];
        int count = pEntity->VPhysicsGetObjectList( pList, ARRAYSIZE(pList) );

        for ( int i = 0; i < count; i++ )
        {
            IPhysicsObject *pPhys = pList[i];
            if ( !pPhys )
                continue;

            // on the odd chance that it's gone to sleep while under anti-gravity
            pPhys->EnableDrag( true );
            pPhys->Wake();
            pPhys->SetMass( m_savedMass[i] );
            pPhys->SetDamping( NULL, &m_savedRotDamping[i] );
            PhysClearGameFlags( pPhys, FVPHYSICS_PLAYER_HELD );
            if ( bClearVelocity )
            {
                PhysForceClearVelocity( pPhys );
            }
            else
            {
#ifndef CLIENT_DLL
                ClampPhysicsVelocity( pPhys, hl2_normspeed.GetFloat() * 1.5f, 2.0f * 360.0f );
#endif
            }

        }
    }

    m_attachedEntity = NULL;
    if ( physenv )
    {
        physenv->DestroyMotionController( m_controller );
    }
    m_controller = NULL;
}

static bool InContactWithHeavyObject( IPhysicsObject *pObject, float heavyMass )
{
    bool contact = false;
    IPhysicsFrictionSnapshot *pSnapshot = pObject->CreateFrictionSnapshot();
    while ( pSnapshot->IsValid() )
    {
        IPhysicsObject *pOther = pSnapshot->GetObject( 1 );
        if ( !pOther->IsMoveable() || pOther->GetMass() > heavyMass )
        {
            contact = true;
            break;
        }
        pSnapshot->NextFrictionData();
    }
    pObject->DestroyFrictionSnapshot( pSnapshot );
    return contact;
}

IMotionEvent::simresult_e CGrabController::Simulate( IPhysicsMotionController *pController, IPhysicsObject *pObject, float deltaTime, Vector &linear, AngularImpulse &angular )
{
    game_shadowcontrol_params_t shadowParams = m_shadow;
    if ( InContactWithHeavyObject( pObject, GetLoadWeight() ) )
    {
        m_contactAmount = Approach( 0.1f, m_contactAmount, deltaTime*2.0f );
    }
    else
    {
        m_contactAmount = Approach( 1.0f, m_contactAmount, deltaTime*2.0f );
    }
    shadowParams.maxAngular = m_shadow.maxAngular * m_contactAmount * m_contactAmount * m_contactAmount;
#ifndef CLIENT_DLL
    m_timeToArrive = pObject->ComputeShadowControl( shadowParams, m_timeToArrive, deltaTime );
#else
    m_timeToArrive = pObject->ComputeShadowControl( shadowParams, (TICK_INTERVAL*2), deltaTime );
#endif
    
    // Slide along the current contact points to fix bouncing problems
    Vector velocity;
    AngularImpulse angVel;
    pObject->GetVelocity( &velocity, &angVel );
    PhysComputeSlideDirection( pObject, velocity, angVel, &velocity, &angVel, GetLoadWeight() );
    pObject->SetVelocityInstantaneous( &velocity, NULL );

    linear.Init();
    angular.Init();
    m_errorTime += deltaTime;

    return SIM_LOCAL_ACCELERATION;
}

float CGrabController::GetSavedMass( IPhysicsObject *pObject )
{
    CBaseEntity *pHeld = m_attachedEntity;
    if ( pHeld )
    {
        if ( pObject->GetGameData() == (void*)pHeld )
        {
            IPhysicsObject *pList[VPHYSICS_MAX_OBJECT_LIST_COUNT];
            int count = pHeld->VPhysicsGetObjectList( pList, ARRAYSIZE(pList) );
            for ( int i = 0; i < count; i++ )
            {
                if ( pList[i] == pObject )
                    return m_savedMass[i];
            }
        }
    }
    return 0.0f;
}

//-----------------------------------------------------------------------------
// Player pickup controller
//-----------------------------------------------------------------------------

class CPlayerPickupController : public CBaseEntity
{
    DECLARE_CLASS( CPlayerPickupController, CBaseEntity );
public:
    void Init( CBasePlayer *pPlayer, CBaseEntity *pObject );
    void Shutdown( bool bThrown = false );
    bool OnControls( CBaseEntity *pControls ) { return true; }
    void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
    void OnRestore()
    {
        m_grabController.OnRestore();
    }
    void VPhysicsUpdate( IPhysicsObject *pPhysics ){}
    void VPhysicsShadowUpdate( IPhysicsObject *pPhysics ) {}

    bool IsHoldingEntity( CBaseEntity *pEnt );
    CGrabController &GetGrabController() { return m_grabController; }

private:
    CGrabController		m_grabController;
    CBasePlayer			*m_pPlayer;
};

LINK_ENTITY_TO_CLASS( player_pickup, CPlayerPickupController );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//			*pObject - 
//-----------------------------------------------------------------------------
void CPlayerPickupController::Init( CBasePlayer *pPlayer, CBaseEntity *pObject )
{
#ifndef CLIENT_DLL
    // Holster player's weapon
    if ( pPlayer->GetActiveWeapon() )
    {
        if ( !pPlayer->GetActiveWeapon()->Holster() )
        {
            Shutdown();
            return;
        }
    }


    CHL2MP_Player *pOwner = (CHL2MP_Player *)ToBasePlayer( pPlayer );
    if ( pOwner )
    {
        pOwner->EnableSprint( false );
    }

    // If the target is debris, convert it to non-debris
    if ( pObject->GetCollisionGroup() == COLLISION_GROUP_DEBRIS )
    {
        // Interactive debris converts back to debris when it comes to rest
        pObject->SetCollisionGroup( COLLISION_GROUP_INTERACTIVE_DEBRIS );
    }

    // done so I'll go across level transitions with the player
    SetParent( pPlayer );
    m_grabController.SetIgnorePitch( true );
    m_grabController.SetAngleAlignment( DOT_30DEGREE );
    m_pPlayer = pPlayer;
    IPhysicsObject *pPhysics = pObject->VPhysicsGetObject();
    Pickup_OnPhysGunPickup( pObject, m_pPlayer );
    
    m_grabController.AttachEntity( pPlayer, pObject, pPhysics, false, vec3_origin, false );
    
    m_pPlayer->m_Local.m_iHideHUD |= HIDEHUD_WEAPONSELECTION;
    m_pPlayer->SetUseEntity( this );
#endif
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bool - 
//-----------------------------------------------------------------------------
void CPlayerPickupController::Shutdown( bool bThrown )
{
#ifndef CLIENT_DLL
    CBaseEntity *pObject = m_grabController.GetAttached();

    bool bClearVelocity = false;
    if ( !bThrown && pObject && pObject->VPhysicsGetObject() && pObject->VPhysicsGetObject()->GetContactPoint(NULL,NULL) )
    {
        bClearVelocity = true;
    }

    m_grabController.DetachEntity( bClearVelocity );

    if ( pObject != NULL )
    {
        Pickup_OnPhysGunDrop( pObject, m_pPlayer, bThrown ? THROWN_BY_PLAYER : DROPPED_BY_PLAYER );
    }

    if ( m_pPlayer )
    {
        CHL2MP_Player *pOwner = (CHL2MP_Player *)ToBasePlayer( m_pPlayer );
        if ( pOwner )
        {
            pOwner->EnableSprint( true );
        }

        m_pPlayer->SetUseEntity( NULL );
        if ( m_pPlayer->GetActiveWeapon() )
        {
            if ( !m_pPlayer->GetActiveWeapon()->Deploy() )
            {
                // We tried to restore the player's weapon, but we couldn't.
                // This usually happens when they're holding an empty weapon that doesn't
                // autoswitch away when out of ammo. Switch to next best weapon.
                m_pPlayer->SwitchToNextBestWeapon( NULL );
            }
        }

        m_pPlayer->m_Local.m_iHideHUD &= ~HIDEHUD_WEAPONSELECTION;
    }
    Remove();

#endif
    
}


void CPlayerPickupController::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
    if ( ToBasePlayer(pActivator) == m_pPlayer )
    {
        CBaseEntity *pAttached = m_grabController.GetAttached();

        // UNDONE: Use vphysics stress to decide to drop objects
        // UNDONE: Must fix case of forcing objects into the ground you're standing on (causes stress) before that will work
        if ( !pAttached || useType == USE_OFF || (m_pPlayer->m_nButtons & IN_ATTACK2) || m_grabController.ComputeError() > 12 )
        {
            Shutdown();
            return;
        }
        
        //Adrian: Oops, our object became motion disabled, let go!
        IPhysicsObject *pPhys = pAttached->VPhysicsGetObject();
        if ( pPhys && pPhys->IsMoveable() == false )
        {
            Shutdown();
            return;
        }

#if STRESS_TEST
        vphysics_objectstress_t stress;
        CalculateObjectStress( pPhys, pAttached, &stress );
        if ( stress.exertedStress > 250 )
        {
            Shutdown();
            return;
        }
#endif
        // +ATTACK will throw phys objects
        if ( m_pPlayer->m_nButtons & IN_ATTACK )
        {
            Shutdown( true );
            Vector vecLaunch;
            m_pPlayer->EyeVectors( &vecLaunch );
            // JAY: Scale this with mass because some small objects really go flying
            float massFactor = clamp( pPhys->GetMass(), 0.5, 15 );
            massFactor = RemapVal( massFactor, 0.5, 15, 0.5, 4 );
            vecLaunch *= player_throwforce.GetFloat() * massFactor;

            pPhys->ApplyForceCenter( vecLaunch );
            AngularImpulse aVel = RandomAngularImpulse( -10, 10 ) * massFactor;
            pPhys->ApplyTorqueCenter( aVel );
            return;
        }

        if ( useType == USE_SET )
        {
            // update position
            m_grabController.UpdateObject( m_pPlayer, 12 );
        }
    }
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEnt - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CPlayerPickupController::IsHoldingEntity( CBaseEntity *pEnt )
{
    return ( m_grabController.GetAttached() == pEnt );
}

void PlayerPickupObject( CBasePlayer *pPlayer, CBaseEntity *pObject )
{
    
#ifndef CLIENT_DLL
    
    //Don't pick up if we don't have a phys object.
    if ( pObject->VPhysicsGetObject() == NULL )
         return;

    CPlayerPickupController *pController = (CPlayerPickupController *)CBaseEntity::Create( "player_pickup", pObject->GetAbsOrigin(), vec3_angle, pPlayer );
    
    if ( !pController )
        return;

    pController->Init( pPlayer, pObject );

#endif

}

//----------------------------------------------------------------------------------------------------------------------------------------------------------
//  CZMWeaponCarry class
//----------------------------------------------------------------------------------------------------------------------------------------------------------

#ifdef CLIENT_DLL
#define CZMWeaponCarry C_ZMWeaponCarry
#endif

class CZMWeaponCarry : public CZMBaseWeapon
{
public:
    DECLARE_CLASS( CZMWeaponCarry, CZMBaseWeapon );
    DECLARE_NETWORKCLASS();
    DECLARE_PREDICTABLE(); // Requires semicolon, thanks Valve.
#ifndef CLIENT_DLL
    DECLARE_ACTTABLE();
#endif

    CZMWeaponCarry();

#ifdef CLIENT_DLL
    bool ShouldDrawPickup() OVERRIDE { return false; };
    bool ShouldDraw() OVERRIDE { return false; };
#endif
    
    virtual void	OnRestore();
    virtual void	UpdateOnRemove(void);
    void	PrimaryAttack();
    void	SecondaryAttack();
    void	WeaponIdle();
    void	ItemPreFrame();
    void	ItemPostFrame();

    void    Drop( const Vector& ) OVERRIDE;
    void	ForceDrop( void );
    bool	DropIfEntityHeld( CBaseEntity *pTarget );	// Drops its held entity if it matches the entity passed in
    CGrabController &GetGrabController() { return m_grabController; }

    bool	CanHolster( void );
    bool	Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
    bool	Deploy( void );

    bool	HasAnyAmmo( void ) { return true; }

    virtual void SetViewModel( void );
    //virtual const char *GetShootSound( int iIndex ) const;
    
    // Move this to public.
    bool	CanPickupObject( CBaseEntity *pTarget );

#ifndef CLIENT_DLL
    CNetworkQAngle	( m_attachedAnglesPlayerSpace );
#else
    QAngle m_attachedAnglesPlayerSpace;
#endif

    CNetworkVector	( m_attachedPositionObjectSpace );

    CNetworkHandle( CBaseEntity, m_hAttachedObject );

    EHANDLE m_hOldAttachedObject;

protected:
    enum FindObjectResult_t
    {
        OBJECT_FOUND = 0,
        OBJECT_NOT_FOUND,
        OBJECT_BEING_DETACHED,
    };

    // Pickup and throw objects.
    void	CheckForTarget( void );
    
#ifndef CLIENT_DLL
    bool	AttachObject( CBaseEntity *pObject, const Vector &vPosition );
    FindObjectResult_t		FindObject( void );
    CBaseEntity *FindObjectInCone( const Vector &vecOrigin, const Vector &vecDir, float flCone );
#endif	// !CLIENT_DLL

    void	UpdateObject( void );
    void	DetachObject( bool playSound = true, bool wasLaunched = false );
    void	LaunchObject( const Vector &vecDir, float flForce );

    // Punt objects - this is pointing at an object in the world and applying a force to it.
    void	PuntNonVPhysics( CBaseEntity *pEntity, const Vector &forward, trace_t &tr );
    void	PuntVPhysics( CBaseEntity *pEntity, const Vector &forward, trace_t &tr );

    // Velocity-based throw common to punt and launch code.
    void	ApplyVelocityBasedForce( CBaseEntity *pEntity, const Vector &forward );


    // Trace length
    float	TraceLength();

    // Sprite scale factor 
    float	SpriteScaleFactor();

    float			GetLoadPercentage();

    void	DryFire( void );
    void	PrimaryFireEffect( void );

#ifndef CLIENT_DLL
    // What happens when the physgun picks up something 
    void	Physgun_OnPhysGunPickup( CBaseEntity *pEntity, CBasePlayer *pOwner, PhysGunPickup_t reason );
#endif	// !CLIENT_DLL

#ifdef CLIENT_DLL

    //virtual void	ViewModelDrawn( C_BaseViewModel *pBaseViewModel );
    virtual void	OnDataChanged( DataUpdateType_t type );
    
    void			ManagePredictedObject( void );

    bool			m_bOldOpen;			// Used for parity checks

    //void			NotifyShouldTransmit( ShouldTransmitState_t state );

#endif	// CLIENT_DLL

    int		m_nChangeState;				// For delayed state change of elements
    float	m_flCheckSuppressTime;		// Amount of time to suppress the checking for targets
    bool	m_flLastDenySoundPlayed;	// Debounce for deny sound
    int		m_nAttack2Debounce;

    CNetworkVar( bool,	m_bActive );
    CNetworkVar( bool,	m_bOpen );

    bool	m_bResetOwnerEntity;
    
    float	m_flElementDebounce;

    CGrabController		m_grabController;

    float	m_flRepuntObjectTime;
    EHANDLE m_hLastPuntedObject;

private:
    CZMWeaponCarry( const CZMWeaponCarry & );


    // Our stuff
public:
    bool CanBeDropped() OVERRIDE { return false; };
};

IMPLEMENT_NETWORKCLASS_ALIASED( ZMWeaponCarry, DT_ZM_WeaponCarry )

BEGIN_NETWORK_TABLE( CZMWeaponCarry, DT_ZM_WeaponCarry )
#ifdef CLIENT_DLL
    RecvPropBool( RECVINFO( m_bActive ) ),
    RecvPropEHandle( RECVINFO( m_hAttachedObject ) ),
    RecvPropVector( RECVINFO( m_attachedPositionObjectSpace ) ),
    RecvPropFloat( RECVINFO( m_attachedAnglesPlayerSpace[0] ) ),
    RecvPropFloat( RECVINFO( m_attachedAnglesPlayerSpace[1] ) ),
    RecvPropFloat( RECVINFO( m_attachedAnglesPlayerSpace[2] ) ),
    RecvPropBool( RECVINFO( m_bOpen ) ),
#else
    SendPropBool( SENDINFO( m_bActive ) ),
    SendPropEHandle( SENDINFO( m_hAttachedObject ) ),
    SendPropVector(SENDINFO( m_attachedPositionObjectSpace ), -1, SPROP_COORD),
    SendPropAngle( SENDINFO_VECTORELEM(m_attachedAnglesPlayerSpace, 0 ), 11 ),
    SendPropAngle( SENDINFO_VECTORELEM(m_attachedAnglesPlayerSpace, 1 ), 11 ),
    SendPropAngle( SENDINFO_VECTORELEM(m_attachedAnglesPlayerSpace, 2 ), 11 ),
    SendPropBool( SENDINFO( m_bOpen ) ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CZMWeaponCarry )
    DEFINE_PRED_FIELD( m_bOpen,			FIELD_BOOLEAN,	FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_zm_carry, CZMWeaponCarry );
PRECACHE_WEAPON_REGISTER( weapon_zm_carry );

#ifndef CLIENT_DLL
acttable_t CZMWeaponCarry::m_acttable[] = 
{
    { ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_PHYSGUN,					false },
    { ACT_HL2MP_RUN,					ACT_HL2MP_RUN_PHYSGUN,					false },
    { ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_PHYSGUN,			false },
    { ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_PHYSGUN,			false },
    { ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_PHYSGUN,	false },
    { ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_PHYSGUN,		false },
    { ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_PHYSGUN,					false },
};
IMPLEMENT_ACTTABLE( CZMWeaponCarry );
#endif


enum
{
    ELEMENT_STATE_NONE = -1,
    ELEMENT_STATE_OPEN,
    ELEMENT_STATE_CLOSED,
};

enum
{
    EFFECT_NONE,
    EFFECT_CLOSED,
    EFFECT_READY,
    EFFECT_HOLDING,
    EFFECT_LAUNCH,
};

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CZMWeaponCarry::CZMWeaponCarry()
{
    m_bOpen					= false;
    m_nChangeState			= ELEMENT_STATE_NONE;
    m_flCheckSuppressTime	= 0.0f;
    m_flLastDenySoundPlayed	= false;

#ifdef CLIENT_DLL
    m_bOldOpen				= false;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Restore
//-----------------------------------------------------------------------------
void CZMWeaponCarry::OnRestore()
{
    BaseClass::OnRestore();
    m_grabController.OnRestore();
}


//-----------------------------------------------------------------------------
// On Remove
//-----------------------------------------------------------------------------
void CZMWeaponCarry::UpdateOnRemove(void)
{
    BaseClass::UpdateOnRemove();
}

#ifdef CLIENT_DLL
void CZMWeaponCarry::OnDataChanged( DataUpdateType_t type )
{
    BaseClass::OnDataChanged( type );

    if ( type == DATA_UPDATE_CREATED )
    {
        SetNextClientThink( CLIENT_THINK_ALWAYS );

        C_BaseAnimating::AutoAllowBoneAccess boneaccess( true, false );
    }

    if ( GetOwner() == NULL )
    {
        if ( m_hAttachedObject )
        {
            m_hAttachedObject->VPhysicsDestroyObject();
        }

        if ( m_hOldAttachedObject )
        {
            m_hOldAttachedObject->VPhysicsDestroyObject();
        }
    }


    // Update element state when out of parity
    if ( m_bOldOpen != m_bOpen )
    {
        m_bOldOpen = (bool) m_bOpen;
    }
}
#endif

//-----------------------------------------------------------------------------
// Sprite scale factor 
//-----------------------------------------------------------------------------
inline float CZMWeaponCarry::SpriteScaleFactor() 
{
    return 1.0f;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CZMWeaponCarry::Deploy( void )
{
    bool bReturn = BaseClass::Deploy();

    m_flNextSecondaryAttack = m_flNextPrimaryAttack = gpGlobals->curtime;

    CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

    if ( pOwner )
    {
        pOwner->SetNextAttack( gpGlobals->curtime );
    }

    return bReturn;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CZMWeaponCarry::SetViewModel( void )
{
    BaseClass::SetViewModel();
}

//-----------------------------------------------------------------------------
// Purpose: Force the cannon to drop anything it's carrying
//-----------------------------------------------------------------------------
void CZMWeaponCarry::ForceDrop( void )
{
    DetachObject();
}


//-----------------------------------------------------------------------------
// Purpose: Drops its held entity if it matches the entity passed in
// Input  : *pTarget - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CZMWeaponCarry::DropIfEntityHeld( CBaseEntity *pTarget )
{
    if ( pTarget == NULL )
        return false;

    CBaseEntity *pHeld = m_grabController.GetAttached();
    
    if ( pHeld == NULL )
        return false;

    if ( pHeld == pTarget )
    {
        ForceDrop();
        return true;
    }

    return false;
}


void CZMWeaponCarry::Drop( const Vector &vecVelocity )
{
    ForceDrop();

    BaseClass::Drop( vecVelocity );
}

bool CZMWeaponCarry::CanHolster( void ) 
{ 
    //Don't holster this weapon if we're holding onto something
    if ( m_bActive )
        return false;

    return BaseClass::CanHolster();
};

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CZMWeaponCarry::Holster( CBaseCombatWeapon *pSwitchingTo )
{
    //Don't holster this weapon if we're holding onto something
    if ( m_bActive )
        return false;

    ForceDrop();

    return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CZMWeaponCarry::DryFire( void )
{
    SendWeaponAnim( ACT_VM_PRIMARYATTACK );

    //WeaponSound( EMPTY );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CZMWeaponCarry::PrimaryFireEffect( void )
{
    CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
    
    if ( pOwner == NULL )
        return;

    pOwner->ViewPunch( QAngle( -0.2f, SharedRandomInt( "physcannonfire", -0.5f, 0.5f ), 0.0f ) );

    //WeaponSound( SINGLE );
}

void CZMWeaponCarry::PuntNonVPhysics( CBaseEntity *pEntity, const Vector &forward, trace_t &tr )
{
    //if ( m_hLastPuntedObject == pEntity && gpGlobals->curtime < m_flRepuntObjectTime )
    //    return;
    /*
#ifndef CLIENT_DLL
    CTakeDamageInfo	info;
    
    info.SetAttacker( GetOwner() );
    info.SetInflictor( this );
    info.SetDamage( 1.0f );
    info.SetDamageType( DMG_CRUSH | DMG_PHYSGUN );
    info.SetDamageForce( forward );	// Scale?
    info.SetDamagePosition( tr.endpos );

    m_hLastPuntedObject = pEntity;
    m_flRepuntObjectTime = gpGlobals->curtime + 0.5f;

    pEntity->DispatchTraceAttack( info, forward, &tr );

    ApplyMultiDamage();

#endif
    
    PrimaryFireEffect();
    SendWeaponAnim( ACT_VM_SECONDARYATTACK );

    m_nChangeState = ELEMENT_STATE_CLOSED;
    m_flElementDebounce = gpGlobals->curtime + 0.5f;
    m_flCheckSuppressTime = gpGlobals->curtime + 0.25f;
    */
}


#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// What happens when the physgun picks up something 
//-----------------------------------------------------------------------------
void CZMWeaponCarry::Physgun_OnPhysGunPickup( CBaseEntity *pEntity, CBasePlayer *pOwner, PhysGunPickup_t reason )
{
    // If the target is debris, convert it to non-debris
    if ( pEntity->GetCollisionGroup() == COLLISION_GROUP_DEBRIS )
    {
        // Interactive debris converts back to debris when it comes to rest
        pEntity->SetCollisionGroup( COLLISION_GROUP_INTERACTIVE_DEBRIS );
    }

    Pickup_OnPhysGunPickup( pEntity, pOwner, reason );
}
#endif

//-----------------------------------------------------------------------------
// Punt vphysics
//-----------------------------------------------------------------------------
void CZMWeaponCarry::PuntVPhysics( CBaseEntity *pEntity, const Vector &vecForward, trace_t &tr )
{
    CBasePlayer *pOwner = ToBasePlayer( GetOwner() );


    if ( m_hLastPuntedObject == pEntity && gpGlobals->curtime < m_flRepuntObjectTime )
        return;

    m_hLastPuntedObject = pEntity;
    m_flRepuntObjectTime = gpGlobals->curtime + 0.5f;

#ifndef CLIENT_DLL
    CTakeDamageInfo	info;

    Vector forward = vecForward;

    info.SetAttacker( GetOwner() );
    info.SetInflictor( this );
    info.SetDamage( 0.0f );
    info.SetDamageType( DMG_PHYSGUN );
    pEntity->DispatchTraceAttack( info, forward, &tr );
    ApplyMultiDamage();


    if ( Pickup_OnAttemptPhysGunPickup( pEntity, pOwner, PUNTED_BY_CANNON ) )
    {
        IPhysicsObject *pList[VPHYSICS_MAX_OBJECT_LIST_COUNT];
        int listCount = pEntity->VPhysicsGetObjectList( pList, ARRAYSIZE(pList) );
        if ( !listCount )
        {
            //FIXME: Do we want to do this if there's no physics object?
            Physgun_OnPhysGunPickup( pEntity, pOwner, PUNTED_BY_CANNON );
            DryFire();
            return;
        }
                
        if( forward.z < 0 )
        {
            //reflect, but flatten the trajectory out a bit so it's easier to hit standing targets
            forward.z *= -0.65f;
        }
                
        // NOTE: Do this first to enable motion (if disabled) - so forces will work
        // Tell the object it's been punted
        Physgun_OnPhysGunPickup( pEntity, pOwner, PUNTED_BY_CANNON );

        // don't push vehicles that are attached to the world via fixed constraints
        // they will just wiggle...
        if ( (pList[0]->GetGameFlags() & FVPHYSICS_CONSTRAINT_STATIC) && pEntity->GetServerVehicle() )
        {
            forward.Init();
        }

        if ( !Pickup_ShouldPuntUseLaunchForces( pEntity, PHYSGUN_FORCE_PUNTED ) )
        {
            int i;

            // limit mass to avoid punting REALLY huge things
            float totalMass = 0;
            for ( i = 0; i < listCount; i++ )
            {
                totalMass += pList[i]->GetMass();
            }

            // ZMRCHANGES
            /*
            float maxMass = 250;
            IServerVehicle *pVehicle = pEntity->GetServerVehicle();
            if ( pVehicle )
            {
                maxMass *= 2.5;	// 625 for vehicles
            }
            float mass = MIN(totalMass, maxMass); // max 250kg of additional force
            */
            // Put some spin on the object
            for ( i = 0; i < listCount; i++ )
            {
                const float hitObjectFactor = 0.5f;
                const float otherObjectFactor = 1.0f - hitObjectFactor;
                // Must be light enough
                float ratio = pList[i]->GetMass() / totalMass;
                if ( pList[i] == pEntity->VPhysicsGetObject() )
                {
                    ratio += hitObjectFactor;
                    ratio = MIN(ratio,1.0f);
                }
                else
                {
                    ratio *= otherObjectFactor;
                }

                // ZMRCHANGES
                pList[i]->ApplyForceCenter( forward * 3500.0f * ratio ); // Was 15000.0f
                // forward * mass * 600.0f * ratio
                pList[i]->ApplyForceOffset( forward * 30.0f * ratio, tr.endpos );
            }
        }
        else
        {
            ApplyVelocityBasedForce( pEntity, vecForward );
        }
    }

#endif
    // Add recoil
    QAngle	recoil = QAngle( random->RandomFloat( 1.0f, 2.0f ), random->RandomFloat( -1.0f, 1.0f ), 0 );
    pOwner->ViewPunch( recoil );



    PrimaryFireEffect();
    SendWeaponAnim( ACT_VM_SECONDARYATTACK );

    m_nChangeState = ELEMENT_STATE_CLOSED;
    m_flElementDebounce = gpGlobals->curtime + 0.5f;
    m_flCheckSuppressTime = gpGlobals->curtime + 0.25f;

    // Don't allow the gun to regrab a thrown object!!
    m_flNextSecondaryAttack = m_flNextPrimaryAttack = gpGlobals->curtime + 0.5f;
}

//-----------------------------------------------------------------------------
// Purpose: Applies velocity-based forces to throw the entity. This code is
//			called from both punt and launch carried code.
//			ASSUMES: that pEntity is a vphysics entity.
// Input  : - 
//-----------------------------------------------------------------------------
void CZMWeaponCarry::ApplyVelocityBasedForce( CBaseEntity *pEntity, const Vector &forward )
{
#ifndef CLIENT_DLL
    IPhysicsObject *pPhysicsObject = pEntity->VPhysicsGetObject();
    Assert(pPhysicsObject); // Shouldn't ever get here with a non-vphysics object.
    if (!pPhysicsObject)
        return;

    float flForceMax = physcannon_maxforce.GetFloat();
    float flForce = flForceMax;

    float mass = pPhysicsObject->GetMass();
    if (mass > 100)
    {
        // ZMRCHANGES
        mass = MIN(mass, 150); // Was 1000
        float flForceMin = physcannon_minforce.GetFloat();
        flForce = SimpleSplineRemapVal(mass, 50, 150, flForceMax, flForceMin); // Was 100, 600
    }

    Vector vVel = forward * flForce;
    // FIXME: Josh needs to put a real value in for PHYSGUN_FORCE_PUNTED
    AngularImpulse aVel = Pickup_PhysGunLaunchAngularImpulse( pEntity, PHYSGUN_FORCE_PUNTED );
        
    pPhysicsObject->AddVelocity( &vVel, &aVel );

#endif

}


//-----------------------------------------------------------------------------
// Trace length
//-----------------------------------------------------------------------------
float CZMWeaponCarry::TraceLength()
{
    return physcannon_tracelength.GetFloat();
}


//-----------------------------------------------------------------------------
// Purpose: 
//
// This mode is a toggle. Primary fire one time to pick up a physics object.
// With an object held, click primary fire again to drop object.
//-----------------------------------------------------------------------------
void CZMWeaponCarry::PrimaryAttack( void )
{
    if( m_flNextPrimaryAttack > gpGlobals->curtime )
        return;

    CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
    
    if ( pOwner == NULL )
        return;

    if( m_bActive )
    {
        // Punch the object being held!!
        Vector forward;
        pOwner->EyeVectors( &forward );

        // Validate the item is within punt range
        // ZMRCHANGES: No reason for this.
        /*CBaseEntity *pHeld = m_grabController.GetAttached();
        Assert( pHeld != NULL );

        if ( pHeld != NULL )
        {
            float heldDist = ( pHeld->WorldSpaceCenter() - pOwner->WorldSpaceCenter() ).Length();

            if ( heldDist > physcannon_tracelength.GetFloat() )
            {
                // We can't punt this yet
                DryFire();
                return;
            }
        }*/

        LaunchObject( forward, 1337.0f );

        PrimaryFireEffect();
        SendWeaponAnim( ACT_VM_SECONDARYATTACK );
        return;
    }

    // If not active, just issue a physics punch in the world.
    m_flNextPrimaryAttack = gpGlobals->curtime + 0.5f;

    Vector forward;
    pOwner->EyeVectors( &forward );

    // NOTE: Notice we're *not* using the mega tracelength here
    // when you have the mega cannon. Punting has shorter range.
    Vector start, end;
    start = pOwner->Weapon_ShootPosition();
    float flPuntDistance = TraceLength();
    VectorMA( start, flPuntDistance, forward, end );

    CTraceFilterNoOwnerTest filter( pOwner, COLLISION_GROUP_NONE );
    trace_t tr;
    UTIL_TraceHull( start, end, -Vector(8,8,8), Vector(8,8,8), MASK_SHOT|CONTENTS_GRATE, &filter, &tr );
    bool bValid = true;
    CBaseEntity *pEntity = tr.m_pEnt;
    if ( tr.fraction == 1 || !tr.m_pEnt || tr.m_pEnt->IsEFlagSet( EFL_NO_PHYSCANNON_INTERACTION ) )
    {
        bValid = false;
    }
    else if ( (pEntity->GetMoveType() != MOVETYPE_VPHYSICS) && ( pEntity->m_takedamage == DAMAGE_NO ) )
    {
        bValid = false;
    }

    // If the entity we've hit is invalid, try a traceline instead
    if ( !bValid )
    {
        UTIL_TraceLine( start, end, MASK_SHOT|CONTENTS_GRATE, &filter, &tr );
        if ( tr.fraction == 1 || !tr.m_pEnt || tr.m_pEnt->IsEFlagSet( EFL_NO_PHYSCANNON_INTERACTION ) )
        {
            // Play dry-fire sequence
            DryFire();
            return;
        }

        pEntity = tr.m_pEnt;
    }

    // See if we hit something
    if ( pEntity->GetMoveType() != MOVETYPE_VPHYSICS )
    {
        if ( pEntity->m_takedamage == DAMAGE_NO )
        {
            DryFire();
            return;
        }

        if( GetOwner()->IsPlayer() )
        {
            // Don't let the player zap any NPC's except regular antlions and headcrabs.
            if( pEntity->IsPlayer() )
            {
                DryFire();
                return;
            }
        }

        PuntNonVPhysics( pEntity, forward, tr );
    }
    else
    {
        if ( pEntity->VPhysicsIsFlesh( ) )
        {
            DryFire();
            return;
        }
        PuntVPhysics( pEntity, forward, tr );
    }
}


//-----------------------------------------------------------------------------
// Purpose: Click secondary attack whilst holding an object to hurl it.
//-----------------------------------------------------------------------------
void CZMWeaponCarry::SecondaryAttack( void )
{
#ifndef CLIENT_DLL
    if ( m_flNextSecondaryAttack > gpGlobals->curtime )
        return;

    CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
    
    if ( pOwner == NULL )
        return;

    // See if we should drop a held item
    if ( ( m_bActive ) && ( pOwner->m_afButtonPressed & IN_ATTACK2 ) )
    {
        // Drop the held object
        m_flNextPrimaryAttack = gpGlobals->curtime + 0.5;
        m_flNextSecondaryAttack = gpGlobals->curtime + 0.5;

        DetachObject();


        SendWeaponAnim( ACT_VM_PRIMARYATTACK );
    }
    else
    {
        // Otherwise pick it up
        FindObjectResult_t result = FindObject();
        switch ( result )
        {
        case OBJECT_FOUND:
            //WeaponSound( SPECIAL1 );
            SendWeaponAnim( ACT_VM_PRIMARYATTACK );
            m_flNextSecondaryAttack = gpGlobals->curtime + 0.5f;

            // We found an object. Debounce the button
            m_nAttack2Debounce |= pOwner->m_nButtons;
            break;

        case OBJECT_NOT_FOUND:
            m_flNextSecondaryAttack = gpGlobals->curtime + 0.1f;
            break;

        case OBJECT_BEING_DETACHED:
            m_flNextSecondaryAttack = gpGlobals->curtime + 0.01f;
            break;
        }

    }
#endif
}	

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CZMWeaponCarry::WeaponIdle( void )
{
    if ( HasWeaponIdleTimeElapsed() )
    {
        if ( m_bActive )
        {
            //Shake when holding an item
            SendWeaponAnim( ACT_VM_RELOAD );
        }
        else
        {
            //Otherwise idle simply
            SendWeaponAnim( ACT_VM_IDLE );
        }
    }
}

#ifndef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pObject - 
//-----------------------------------------------------------------------------
bool CZMWeaponCarry::AttachObject( CBaseEntity *pObject, const Vector &vPosition )
{

    if ( m_bActive )
        return false;

    if ( CanPickupObject( pObject ) == false )
        return false;

    m_grabController.SetIgnorePitch( false );
    m_grabController.SetAngleAlignment( 0 );

    IPhysicsObject *pPhysics = pObject->VPhysicsGetObject();

    // Must be valid
    if ( !pPhysics )
        return false;

    CHL2MP_Player *pOwner = (CHL2MP_Player *)ToBasePlayer( GetOwner() );

    m_bActive = true;
    if( pOwner )
    {
        // NOTE: This can change the mass; so it must be done before max speed setting
        Physgun_OnPhysGunPickup( pObject, pOwner, PICKED_UP_BY_CANNON );
    }

    // NOTE :This must happen after OnPhysGunPickup because that can change the mass
    m_grabController.AttachEntity( pOwner, pObject, pPhysics, false, vPosition, false );
    m_hAttachedObject = pObject;
    m_attachedPositionObjectSpace = m_grabController.m_attachedPositionObjectSpace;
    m_attachedAnglesPlayerSpace = m_grabController.m_attachedAnglesPlayerSpace;

    m_bResetOwnerEntity = false;

    if ( m_hAttachedObject->GetOwnerEntity() == NULL )
    {
        m_hAttachedObject->SetOwnerEntity( pOwner );
        m_bResetOwnerEntity = true;
    }

    // Don't drop again for a slight delay, in case they were pulling objects near them
    m_flNextSecondaryAttack = gpGlobals->curtime + 0.4f;

    return true;
}

CZMWeaponCarry::FindObjectResult_t CZMWeaponCarry::FindObject( void )
{
    // ZMRCHANGES
    CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
    
    Assert( pPlayer );
    if ( pPlayer == NULL )
        return OBJECT_NOT_FOUND;
    
    Vector forward;
    pPlayer->EyeVectors( &forward );

    // Setup our positions
    Vector	start = pPlayer->Weapon_ShootPosition();
    float	testLength = TraceLength() * 4.0f;
    Vector	end = start + forward * testLength;

    // Try to find an object by looking straight ahead
    trace_t tr;
    CTraceFilterNoOwnerTest filter( pPlayer, COLLISION_GROUP_NONE );
    UTIL_TraceLine( start, end, MASK_SHOT|CONTENTS_GRATE, &filter, &tr );
    
    // Try again with a hull trace
    if ( ( tr.fraction == 1.0 ) || ( tr.m_pEnt == NULL ) || ( tr.m_pEnt->IsWorld() ) )
    {
        UTIL_TraceHull( start, end, -Vector(4,4,4), Vector(4,4,4), MASK_SHOT|CONTENTS_GRATE, &filter, &tr );
    }

    CBaseEntity *pEntity = tr.m_pEnt ? tr.m_pEnt->GetRootMoveParent() : NULL;
    bool	bAttach = false;
    bool	bPull = false;

    // If we hit something, pick it up or pull it
    if ( ( tr.fraction != 1.0f ) && ( tr.m_pEnt ) && ( tr.m_pEnt->IsWorld() == false ) )
    {
        if ( CanPickupObject( pEntity ) )
        {
            bAttach = true;
        }
        else
        {
            bPull = true;
        }
    }

    if ( !bAttach && !bPull ) return OBJECT_NOT_FOUND;


    /*if ( CanPickupObject( pEntity ) == false )
    {
        // Make a noise to signify we can't pick this up
        if ( !m_flLastDenySoundPlayed )
        {
            m_flLastDenySoundPlayed = true;
            //WeaponSound( SPECIAL3 );
        }

        return OBJECT_NOT_FOUND;
    }*/

    // Check to see if the object is constrained + needs to be ripped off...
    CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
    if ( !Pickup_OnAttemptPhysGunPickup( pEntity, pOwner, PICKED_UP_BY_CANNON ) )
        return OBJECT_BEING_DETACHED;


    if ( bAttach )
    {
        return AttachObject( pEntity, tr.endpos ) ? OBJECT_FOUND : OBJECT_NOT_FOUND;
    }


    //if ( !bPull )
    //    return OBJECT_NOT_FOUND;

    // FIXME: This needs to be run through the CanPickupObject logic
    IPhysicsObject *pObj = pEntity->VPhysicsGetObject();
    if ( !pObj )
        return OBJECT_NOT_FOUND;

    // If we're too far, simply start to pull the object towards us
    Vector	pullDir = start - pEntity->WorldSpaceCenter();
    VectorNormalize( pullDir );
    pullDir *= physcannon_pullforce.GetFloat();
    
    float mass = PhysGetEntityMass( pEntity );
    if ( mass < 50.0f )
    {
        pullDir *= (mass + 0.5) * (1/50.0f);
    }

    // Nudge it towards us
    pObj->ApplyForceCenter( pullDir );
    return OBJECT_NOT_FOUND;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CBaseEntity *CZMWeaponCarry::FindObjectInCone( const Vector &vecOrigin, const Vector &vecDir, float flCone )
{
    // Find the nearest physics-based item in a cone in front of me.
    CBaseEntity *list[256];
    float flNearestDist = TraceLength() + 1.0;
    Vector mins = vecOrigin - Vector( flNearestDist, flNearestDist, flNearestDist );
    Vector maxs = vecOrigin + Vector( flNearestDist, flNearestDist, flNearestDist );

    CBaseEntity *pNearest = NULL;

    int count = UTIL_EntitiesInBox( list, 256, mins, maxs, 0 );
    for( int i = 0 ; i < count ; i++ )
    {
        if ( !list[ i ]->VPhysicsGetObject() )
            continue;

        // Closer than other objects
        Vector los = ( list[ i ]->WorldSpaceCenter() - vecOrigin );
        float flDist = VectorNormalize( los );
        if( flDist >= flNearestDist )
            continue;

        // Cull to the cone
        if ( DotProduct( los, vecDir ) <= flCone )
            continue;

        // Make sure it isn't occluded!
        trace_t tr;
        CTraceFilterNoOwnerTest filter( GetOwner(), COLLISION_GROUP_NONE );
        UTIL_TraceLine( vecOrigin, list[ i ]->WorldSpaceCenter(), MASK_SHOT|CONTENTS_GRATE, &filter, &tr );
        if( tr.m_pEnt == list[ i ] )
        {
            flNearestDist = flDist;
            pNearest = list[ i ];
        }
    }

    return pNearest;
}

#endif

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CGrabController::UpdateObject( CBasePlayer *pPlayer, float flError )
{
    CBaseEntity *pEntity = GetAttached();
    if ( !pEntity )
        return false;
    if ( ComputeError() > flError )
        return false;
    if ( pPlayer->GetGroundEntity() == pEntity )
        return false;
    if (!pEntity->VPhysicsGetObject() )
        return false;    

    //Adrian: Oops, our object became motion disabled, let go!
    IPhysicsObject *pPhys = pEntity->VPhysicsGetObject();
    if ( pPhys && pPhys->IsMoveable() == false )
    {
        return false;
    }

    if ( m_frameCount == gpGlobals->framecount )
    {
        return true;
    }
    m_frameCount = gpGlobals->framecount;
    Vector forward, right, up;
    QAngle playerAngles = pPlayer->EyeAngles();

    float pitch = AngleDistance(playerAngles.x,0);
    playerAngles.x = clamp( pitch, -75, 75 );
    AngleVectors( playerAngles, &forward, &right, &up );

    // Now clamp a sphere of object radius at end to the player's bbox
    Vector radial = physcollision->CollideGetExtent( pPhys->GetCollide(), vec3_origin, pEntity->GetAbsAngles(), -forward );
    Vector player2d = pPlayer->CollisionProp()->OBBMaxs();
    float playerRadius = player2d.Length2D();
    float flDot = DotProduct( forward, radial );

    float radius = playerRadius + fabs( flDot );

    float distance = 24 + ( radius * 2.0f );

    Vector start = pPlayer->Weapon_ShootPosition();
    Vector end = start + ( forward * distance );

    trace_t	tr;
    CTraceFilterSkipTwoEntities traceFilter( pPlayer, pEntity, COLLISION_GROUP_NONE );
    Ray_t ray;
    ray.Init( start, end );
    enginetrace->TraceRay( ray, MASK_SOLID_BRUSHONLY, &traceFilter, &tr );

    if ( tr.fraction < 0.5 )
    {
        end = start + forward * (radius*0.5f);
    }
    else if ( tr.fraction <= 1.0f )
    {
        end = start + forward * ( distance - radius );
    }

    Vector playerMins, playerMaxs, nearest;
    pPlayer->CollisionProp()->WorldSpaceAABB( &playerMins, &playerMaxs );
    Vector playerLine = pPlayer->CollisionProp()->WorldSpaceCenter();
    CalcClosestPointOnLine( end, playerLine+Vector(0,0,playerMins.z), playerLine+Vector(0,0,playerMaxs.z), nearest, NULL );

    Vector delta = end - nearest;
    float len = VectorNormalize(delta);
    if ( len < radius )
    {
        end = nearest + radius * delta;
    }

    QAngle angles = TransformAnglesFromPlayerSpace( m_attachedAnglesPlayerSpace, pPlayer );

    //Show overlays of radius
    if ( g_debug_physcannon.GetBool() )
    {

#ifdef CLIENT_DLL

        debugoverlay->AddBoxOverlay( end, -Vector( 2,2,2 ), Vector(2,2,2), angles, 0, 255, 255, true, 0 );

        debugoverlay->AddBoxOverlay( GetAttached()->WorldSpaceCenter(), 
                            -Vector( radius, radius, radius), 
                            Vector( radius, radius, radius ),
                            angles,
                            255, 255, 0,
                            true,
                            0.0f );

#else

        NDebugOverlay::Box( end, -Vector( 2,2,2 ), Vector(2,2,2), 0, 255, 0, true, 0 );

        NDebugOverlay::Box( GetAttached()->WorldSpaceCenter(), 
                            -Vector( radius+5, radius+5, radius+5), 
                            Vector( radius+5, radius+5, radius+5 ),
                            255, 0, 0,
                            true,
                            0.0f );
#endif
    }
    
#ifndef CLIENT_DLL
    // If it has a preferred orientation, update to ensure we're still oriented correctly.
    Pickup_GetPreferredCarryAngles( pEntity, pPlayer, pPlayer->EntityToWorldTransform(), angles );


    // We may be holding a prop that has preferred carry angles
    if ( m_bHasPreferredCarryAngles )
    {
        matrix3x4_t tmp;
        ComputePlayerMatrix( pPlayer, tmp );
        angles = TransformAnglesToWorldSpace( m_vecPreferredCarryAngles, tmp );
    }

#endif

    matrix3x4_t attachedToWorld;
    Vector offset;
    AngleMatrix( angles, attachedToWorld );
    VectorRotate( m_attachedPositionObjectSpace, attachedToWorld, offset );

    SetTargetPosition( end - offset, angles );

    return true;
}

void CZMWeaponCarry::UpdateObject( void )
{
    CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
    Assert( pPlayer );

    float flError = 12;
    if ( !m_grabController.UpdateObject( pPlayer, flError ) )
    {
        DetachObject();
        return;
    }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CZMWeaponCarry::DetachObject( bool playSound, bool wasLaunched )
{
#ifndef CLIENT_DLL
    if ( m_bActive == false )
        return;

    CHL2MP_Player *pOwner = (CHL2MP_Player *)ToBasePlayer( GetOwner() );
    if( pOwner != NULL )
    {
        pOwner->EnableSprint( true );
        pOwner->SetMaxSpeed( hl2_normspeed.GetFloat() );
    }

    CBaseEntity *pObject = m_grabController.GetAttached();

    m_grabController.DetachEntity( wasLaunched );

    if ( pObject != NULL )
    {
        Pickup_OnPhysGunDrop( pObject, pOwner, wasLaunched ? LAUNCHED_BY_CANNON : DROPPED_BY_CANNON );
    }


    if ( pObject && m_bResetOwnerEntity == true )
    {
        pObject->SetOwnerEntity( NULL );
    }

    m_bActive = false;
    m_hAttachedObject = NULL;

    
    if ( playSound )
    {
        //Play the detach sound
        //WeaponSound( MELEE_MISS );
    }
    
#else

    m_grabController.DetachEntity( wasLaunched );

    if ( m_hAttachedObject )
    {
        m_hAttachedObject->VPhysicsDestroyObject();
    }
#endif
}


#ifdef CLIENT_DLL
void CZMWeaponCarry::ManagePredictedObject( void )
{
    CBaseEntity *pAttachedObject = m_hAttachedObject.Get();

    if ( m_hAttachedObject )
    {
        // NOTE :This must happen after OnPhysGunPickup because that can change the mass
        if ( pAttachedObject != GetGrabController().GetAttached() )
        {
            IPhysicsObject *pPhysics = pAttachedObject->VPhysicsGetObject();

            if ( pPhysics == NULL )
            {
                solid_t tmpSolid;
                PhysModelParseSolid( tmpSolid, m_hAttachedObject, pAttachedObject->GetModelIndex() );

                pAttachedObject->VPhysicsInitNormal( SOLID_VPHYSICS, 0, false, &tmpSolid );
            }

            pPhysics = pAttachedObject->VPhysicsGetObject();

            if ( pPhysics )
            {
                m_grabController.SetIgnorePitch( false );
                m_grabController.SetAngleAlignment( 0 );

                GetGrabController().AttachEntity( ToBasePlayer( GetOwner() ), pAttachedObject, pPhysics, false, vec3_origin, false );
                GetGrabController().m_attachedPositionObjectSpace = m_attachedPositionObjectSpace;
                GetGrabController().m_attachedAnglesPlayerSpace = m_attachedAnglesPlayerSpace;
            }
        }
    }
    else
    {
        if ( m_hOldAttachedObject && m_hOldAttachedObject->VPhysicsGetObject() )
        {
            GetGrabController().DetachEntity( false );

            m_hOldAttachedObject->VPhysicsDestroyObject();
        }
    }

    m_hOldAttachedObject = m_hAttachedObject;
}

#endif

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CZMWeaponCarry::ItemPreFrame()
{
    BaseClass::ItemPreFrame();

#ifdef CLIENT_DLL
    C_BasePlayer *localplayer = C_BasePlayer::GetLocalPlayer();

    if ( localplayer && !localplayer->IsObserver() )
        ManagePredictedObject();
#endif

    // Update the object if the weapon is switched on.
    if( m_bActive )
    {
        UpdateObject();
    }
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CZMWeaponCarry::CheckForTarget( void )
{
#ifndef CLIENT_DLL
    //See if we're suppressing this
    if ( m_flCheckSuppressTime > gpGlobals->curtime )
        return;

    // holstered
    if ( IsEffectActive( EF_NODRAW ) )
        return;

    CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

    if ( pOwner == NULL )
        return;

    if ( m_bActive )
        return;

    Vector	aimDir;
    pOwner->EyeVectors( &aimDir );

    Vector	startPos	= pOwner->Weapon_ShootPosition();
    Vector	endPos;
    VectorMA( startPos, TraceLength(), aimDir, endPos );

    trace_t	tr;
    UTIL_TraceHull( startPos, endPos, -Vector(4,4,4), Vector(4,4,4), MASK_SHOT|CONTENTS_GRATE, pOwner, COLLISION_GROUP_NONE, &tr );

    if ( ( tr.fraction != 1.0f ) && ( tr.m_pEnt != NULL ) )
    {
        // FIXME: Try just having the elements always open when pointed at a physics object
        if ( CanPickupObject( tr.m_pEnt ) || Pickup_ForcePhysGunOpen( tr.m_pEnt, pOwner ) )
        // if ( ( tr.m_pEnt->VPhysicsGetObject() != NULL ) && ( tr.m_pEnt->GetMoveType() == MOVETYPE_VPHYSICS ) )
        {
            m_nChangeState = ELEMENT_STATE_NONE;
            return;
        }
    }

    // Close the elements after a delay to prevent overact state switching
    if ( ( m_flElementDebounce < gpGlobals->curtime ) && ( m_nChangeState == ELEMENT_STATE_NONE ) )
    {
        m_nChangeState = ELEMENT_STATE_CLOSED;
        m_flElementDebounce = gpGlobals->curtime + 0.5f;
    }
#endif
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CZMWeaponCarry::ItemPostFrame()
{
    CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
    if ( pOwner == NULL )
    {
        // We found an object. Debounce the button
        m_nAttack2Debounce = 0;
        return;
    }

    //Check for object in pickup range
    if ( m_bActive == false )
    {
        CheckForTarget();

        if ( ( m_flElementDebounce < gpGlobals->curtime ) && ( m_nChangeState != ELEMENT_STATE_NONE ) )
        {
            m_nChangeState = ELEMENT_STATE_NONE;
        }
    }

    // NOTE: Attack2 will be considered to be pressed until the first item is picked up.
    int nAttack2Mask = pOwner->m_nButtons & (~m_nAttack2Debounce);
    if ( nAttack2Mask & IN_ATTACK2 )
    {
        SecondaryAttack();
    }
    else
    {
        // Reset our debouncer
        m_flLastDenySoundPlayed = false;

    }
    
    if (( pOwner->m_nButtons & IN_ATTACK2 ) == 0 )
    {
        m_nAttack2Debounce = 0;
    }

    if ( pOwner->m_nButtons & IN_ATTACK )
    {
        PrimaryAttack();
    }
    else 
    {
        WeaponIdle();
    }
}

void CZMWeaponCarry::LaunchObject( const Vector &vecDir, float flIAmUseless )
{
    CBaseEntity *pObject = m_grabController.GetAttached();

    //if ( !(m_hLastPuntedObject == pObject && gpGlobals->curtime < m_flRepuntObjectTime) )
    {
        // FIRE!!!
        if( pObject != NULL )
        {
            DetachObject( false, true );

            //m_hLastPuntedObject = pObject;
            //m_flRepuntObjectTime = gpGlobals->curtime + 0.5f;

            // Launch
            ApplyVelocityBasedForce( pObject, vecDir );

            // Don't allow the gun to regrab a thrown object!!
            m_flNextSecondaryAttack = m_flNextPrimaryAttack = gpGlobals->curtime + 0.5;
            
            Vector	center = pObject->WorldSpaceCenter();


            m_hAttachedObject = NULL;
            m_bActive = false;
        }
    }



    //Close the elements and suppress checking for a bit
    m_nChangeState = ELEMENT_STATE_CLOSED;
    m_flElementDebounce = gpGlobals->curtime + 0.1f;
    m_flCheckSuppressTime = gpGlobals->curtime + 0.25f;
}

//bool UTIL_IsCombineBall( CBaseEntity *pEntity );

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTarget - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CZMWeaponCarry::CanPickupObject( CBaseEntity *pTarget )
{
#ifndef CLIENT_DLL
    if ( pTarget == NULL )
        return false;

    if ( pTarget->GetBaseAnimating() && pTarget->GetBaseAnimating()->IsDissolving() )
        return false;

    if ( pTarget->IsEFlagSet( EFL_NO_PHYSCANNON_INTERACTION ) )
        return false;


    CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
    
    if ( !pOwner )
        return false;

    if ( pOwner->GetGroundEntity() == pTarget )
        return false;

    if ( pTarget->VPhysicsIsFlesh( ) )
        return false;

    IPhysicsObject *pObj = pTarget->VPhysicsGetObject();	

    if ( pObj && pObj->GetGameFlags() & FVPHYSICS_PLAYER_HELD )
        return false;

    // ZMRCHANGE
    // Can't be picked up if there are npcs/players on it.
    // However, do allow if our targetname is something.
    // This should stop players from standing on top of objective items.
    if ( STRING( pTarget->GetEntityName() )[0] == NULL )
    {
        groundlink_t *link;
        groundlink_t *root = (groundlink_t*)GetDataObject( GROUNDLINK );
        if ( root )
        {
            for ( link = root->nextLink; link != root; link = link->nextLink )
            {
                if ( link->entity && (link->entity->IsNPC() || link->entity->IsPlayer()) )
                {
                    return false;
                }
            }
        }
    }
    

    return CBasePlayer::CanPickupObject( pTarget, physcannon_maxmass.GetFloat(), 0 );
#else
    return false;
#endif
    
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CZMWeaponCarry::GetLoadPercentage( void )
{
    float loadWeight = m_grabController.GetLoadWeight();
    loadWeight /= physcannon_maxmass.GetFloat();	
    loadWeight = clamp( loadWeight, 0.0f, 1.0f );
    return loadWeight;
}

//-----------------------------------------------------------------------------
// EXTERNAL API
//-----------------------------------------------------------------------------
void PhysCannonForceDrop( CBaseCombatWeapon *pActiveWeapon, CBaseEntity *pOnlyIfHoldingThis )
{
    CZMWeaponCarry *pCannon = dynamic_cast<CZMWeaponCarry *>(pActiveWeapon);
    if ( pCannon )
    {
        if ( pOnlyIfHoldingThis )
        {
            pCannon->DropIfEntityHeld( pOnlyIfHoldingThis );
        }
        else
        {
            pCannon->ForceDrop();
        }
    }
}

bool PlayerPickupControllerIsHoldingEntity( CBaseEntity *pPickupControllerEntity, CBaseEntity *pHeldEntity )
{
    CPlayerPickupController *pController = dynamic_cast<CPlayerPickupController *>(pPickupControllerEntity);

    return pController ? pController->IsHoldingEntity( pHeldEntity ) : false;
}


float PhysCannonGetHeldObjectMass( CBaseCombatWeapon *pActiveWeapon, IPhysicsObject *pHeldObject )
{
    float mass = 0.0f;
    CZMWeaponCarry *pCannon = dynamic_cast<CZMWeaponCarry *>(pActiveWeapon);
    if ( pCannon )
    {
        CGrabController &grab = pCannon->GetGrabController();
        mass = grab.GetSavedMass( pHeldObject );
    }

    return mass;
}

CBaseEntity *PhysCannonGetHeldEntity( CBaseCombatWeapon *pActiveWeapon )
{
    CZMWeaponCarry *pCannon = dynamic_cast<CZMWeaponCarry *>(pActiveWeapon);
    if ( pCannon )
    {
        CGrabController &grab = pCannon->GetGrabController();
        return grab.GetAttached();
    }

    return NULL;
}

float PlayerPickupGetHeldObjectMass( CBaseEntity *pPickupControllerEntity, IPhysicsObject *pHeldObject )
{
    float mass = 0.0f;
    CPlayerPickupController *pController = dynamic_cast<CPlayerPickupController *>(pPickupControllerEntity);
    if ( pController )
    {
        CGrabController &grab = pController->GetGrabController();
        mass = grab.GetSavedMass( pHeldObject );
    }
    return mass;
}

#ifdef CLIENT_DLL

//extern void FX_GaussExplosion( const Vector &pos, const Vector &dir, int type );
/*
void CallbackPhyscannonImpact( const CEffectData &data )
{

}

DECLARE_CLIENT_EFFECT( "PhyscannonImpact", CallbackPhyscannonImpact );
*/
#endif

void PlayerAttemptPickup( CBasePlayer* pPlayer, CBaseEntity* pEntity )
{
    CZMWeaponCarry* pWeapon = static_cast<CZMWeaponCarry*>( pPlayer->Weapon_OwnsThisType( "weapon_zm_carry" ) );
    if ( !pWeapon ) return;


    // Don't even switch to the weapon if we can't pick it up.
    if ( !pWeapon->CanPickupObject( pEntity ) )
        return;

    if ( pPlayer->GetActiveWeapon() != pWeapon )
    {
        if ( !pPlayer->Weapon_Switch( pWeapon ) )
            return;
    }

    pWeapon->SecondaryAttack();
}
