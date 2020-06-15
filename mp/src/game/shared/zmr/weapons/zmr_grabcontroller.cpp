#include "cbase.h"
#include "in_buttons.h"
#include "vphysics/friction.h"

#ifdef GAME_DLL
#include "props.h"
#else
#include "c_physicsprop.h"

#define CPhysicsProp C_PhysicsProp
#endif

#include "zmr_grabcontroller.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



#ifdef GAME_DLL
extern ConVar physcannon_maxmass;
#endif


bool Pickup_GetPreferredCarryAngles( CBaseEntity *pObject, CBasePlayer *pPlayer, matrix3x4_t &localToWorld, QAngle &outputAnglesWorldSpace );


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
    m_attachedEntity = nullptr;
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
        
        if ( pObj )
        {
            pObj->Wake();
        }
        else
        {
            DetachEntity( false );
        }
    }
}

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
            pObj->GetShadowPosition( &pos, nullptr );

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
#ifdef ZMR // ZMRCHANGE: Fixes client and server held object angle inconsistency.
    matrix3x4_t test;
    QAngle angleTest = pPlayer->EyeAngles();
    //angleTest.x = 0;
    AngleMatrix( angleTest, test );
    return TransformAnglesToWorldSpace( anglesIn, test );
#else
    if ( m_bIgnoreRelativePitch )
    {
        matrix3x4_t test;
        QAngle angleTest = pPlayer->EyeAngles();
        angleTest.x = 0;
        AngleMatrix( angleTest, test );
        return TransformAnglesToWorldSpace( anglesIn, test );
    }
    return TransformAnglesToWorldSpace( anglesIn, pPlayer->EntityToWorldTransform() );
#endif
}

bool GetModelPreferredAngles_( CBaseAnimating* pEnt, QAngle& anglesOut )
{
    auto* pModel = pEnt->GetModel();
    if ( !pModel ) return false;

    auto* kv = new KeyValues( "" );
	if ( kv->LoadFromBuffer( modelinfo->GetModelName( pModel ), modelinfo->GetModelKeyValueText( pModel ) ) )
	{
		auto* interactions = kv->FindKey( "physgun_interactions" );
		if ( interactions )
		{
			auto* pszAngles = interactions->GetString( "preferred_carryangles" );
			if ( pszAngles && *pszAngles )
			{
				UTIL_StringToVector( anglesOut.Base(), pszAngles );
				kv->deleteThis();
				return true;
			}
		}
	}

	kv->deleteThis();
	return false;
}

#ifdef GAME_DLL
bool GetPreferredCarryAngles_( CBaseEntity* pObject, CBasePlayer* pPlayer, QAngle& anglesOut )
{
	auto* pPickup = dynamic_cast<IPlayerPickupVPhysics*>( pObject );
	if ( pPickup )
	{
		if ( pPickup->HasPreferredCarryAnglesForPlayer( pPlayer ) )
		{
			anglesOut = pPickup->PreferredCarryAngles();
			return true;
		}
	}

	return false;
}
#endif

void CGrabController::AttachEntity( CBasePlayer *pPlayer, CBaseEntity *pEntity, IPhysicsObject *pPhys, bool bIsMegaPhysCannon )
{
    bool bHasPreferredAngles = false;

    // play the impact sound of the object hitting the player
    // used as feedback to let the player know he picked up the object
#ifndef CLIENT_DLL
    PhysicsImpactSound( pPlayer, pPhys, CHAN_STATIC, pPhys->GetMaterialIndex(), pPlayer->VPhysicsGetObject()->GetMaterialIndex(), 1.0, 64 );
#endif


    Vector position;
    QAngle angles;
    QAngle preferredAngles;
    pPhys->GetPosition( &position, &angles );
    
    
    // If it has a preferred orientation, use that instead.
#ifndef CLIENT_DLL
    // ZMR
    // Apply preferred carry angles fix
    
    // This only applies carry angles if:
    // It has a special prop interaction. Only sawblade and a few other props use it.
    // It's a func_physbox with custom carry angles defined.
    bHasPreferredAngles = GetPreferredCarryAngles_( pEntity, pPlayer, preferredAngles );
#endif


    if ( !bHasPreferredAngles )
    {
        // If it's a prop, see if it has desired carry angles
        // Yes, this is separate from the above one.
        CPhysicsProp* pProp = dynamic_cast<CPhysicsProp*>( pEntity );
        if ( pProp )
        {
            bHasPreferredAngles = GetModelPreferredAngles_( pProp, preferredAngles );
        }
    }

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
        pList[i]->GetDamping( nullptr, &m_savedRotDamping[i] );
        m_flLoadWeight += mass;
        m_savedMass[i] = mass;

        // reduce the mass to prevent the player from adding crazy amounts of energy to the system
        pList[i]->SetMass( REDUCED_CARRY_MASS / flFactor );
        pList[i]->SetDamping( nullptr, &damping );
    }
    
    // Give extra mass to the phys object we're actually picking up
    pPhys->SetMass( REDUCED_CARRY_MASS );
    pPhys->EnableDrag( false );

    m_errorTime = -1.0f; // 1 seconds until error starts accumulating
    m_error = 0;
    m_contactAmount = 0;

    if ( bHasPreferredAngles )
    {
        m_attachedAnglesPlayerSpace = preferredAngles;
    }
    else
    {
        m_attachedAnglesPlayerSpace = TransformAnglesToPlayerSpace( angles, pPlayer );
    }

    if ( m_angleAlignment != 0 )
    {
        m_attachedAnglesPlayerSpace = AlignAngles( m_attachedAnglesPlayerSpace, m_angleAlignment );
    }

    VectorITransform( pEntity->WorldSpaceCenter(), pEntity->EntityToWorldTransform(), m_attachedPositionObjectSpace );
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
            pPhys->SetDamping( nullptr, &m_savedRotDamping[i] );
            PhysClearGameFlags( pPhys, FVPHYSICS_PLAYER_HELD );
            if ( bClearVelocity )
            {
                PhysForceClearVelocity( pPhys );
            }
            else
            {
#ifndef CLIENT_DLL
                const float flMaxWalkSpeed = 200.0f;
                ClampPhysicsVelocity( pPhys, flMaxWalkSpeed * 1.5f, 2.0f * 360.0f );
#endif
            }

        }
    }

    m_attachedEntity = nullptr;
    if ( physenv )
    {
        physenv->DestroyMotionController( m_controller );
    }
    m_controller = nullptr;
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
    hlshadowcontrol_params_t shadowParams = m_shadow;
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
    pObject->SetVelocityInstantaneous( &velocity, nullptr );

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
