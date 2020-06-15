#include "cbase.h"

#ifdef CLIENT_DLL
#include "vcollide_parse.h"
#include "iviewrender_beams.h"
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

#include "in_buttons.h"
#include "debugoverlay_shared.h"


#include "zmr_grabcontroller.h"
#include "zmr_basemelee.h"
#include "zmr_fistscarry.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#define OBJECT_MOVE_DELTA           0.05f
#define OBJECT_PUSH_MAXSPD          100.0f
#define OBJECT_PULL_MAXSPD          50.0f


#define HOLD_ACTIVITY               ACT_VM_RELOAD


ConVar	zm_sv_debug_carry( "zm_sv_debug_carry", "0", FCVAR_REPLICATED );

ConVar physcannon_minforce( "physcannon_minforce", "50", FCVAR_REPLICATED );
ConVar physcannon_maxforce( "physcannon_maxforce", "300", FCVAR_REPLICATED );
ConVar physcannon_maxmass( "physcannon_maxmass", "40", FCVAR_REPLICATED );
ConVar physcannon_tracelength( "physcannon_tracelength", "75", FCVAR_REPLICATED );
ConVar physcannon_pullforce( "physcannon_pullforce", "3500", FCVAR_REPLICATED );
ConVar physcannon_pushforce( "physcannon_pushforce", "3000", FCVAR_REPLICATED );


//#define PHYSCANNON_BEAM_SPRITE "sprites/orangelight1.vmt"
//#define PHYSCANNON_BEAM_SPRITE_NOZ "sprites/orangelight1_noz.vmt"
//#define PHYSCANNON_GLOW_SPRITE "sprites/glow04_noz"
//#define PHYSCANNON_ENDCAP_SPRITE "sprites/orangeflare1"
//#define PHYSCANNON_CENTER_GLOW "sprites/orangecore1"
//#define PHYSCANNON_BLAST_SPRITE "sprites/orangecore2"
//
//#ifdef CLIENT_DLL
////Precahce the effects
//CLIENTEFFECT_REGISTER_BEGIN( PrecacheEffectPhysCannon )
//	CLIENTEFFECT_MATERIAL( "sprites/orangelight1" )
//	CLIENTEFFECT_MATERIAL( "sprites/orangelight1_noz" )
//	CLIENTEFFECT_MATERIAL( PHYSCANNON_GLOW_SPRITE )
//	CLIENTEFFECT_MATERIAL( PHYSCANNON_ENDCAP_SPRITE )
//	CLIENTEFFECT_MATERIAL( PHYSCANNON_CENTER_GLOW )
//	CLIENTEFFECT_MATERIAL( PHYSCANNON_BLAST_SPRITE )
//CLIENTEFFECT_REGISTER_END()
//#endif	// CLIENT_DLL



class CTraceFilterNoOwnerTest : public CTraceFilterSimple
{
public:
    DECLARE_CLASS( CTraceFilterNoOwnerTest, CTraceFilterSimple );
    
    CTraceFilterNoOwnerTest( const IHandleEntity *passentity, int collisionGroup )
        : CTraceFilterSimple( nullptr, collisionGroup ), m_pPassNotOwner(passentity)
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


BEGIN_NETWORK_TABLE( CZMWeaponHands, DT_ZM_WeaponHands )
#ifdef CLIENT_DLL
    RecvPropEHandle( RECVINFO( m_hAttachedObject ) ),
    RecvPropVector( RECVINFO( m_attachedPositionObjectSpace ) ),
    RecvPropFloat( RECVINFO( m_attachedAnglesPlayerSpace[0] ) ),
    RecvPropFloat( RECVINFO( m_attachedAnglesPlayerSpace[1] ) ),
    RecvPropFloat( RECVINFO( m_attachedAnglesPlayerSpace[2] ) ),
#else
    SendPropEHandle( SENDINFO( m_hAttachedObject ) ),
    SendPropVector(SENDINFO( m_attachedPositionObjectSpace ), -1, SPROP_COORD),
    SendPropAngle( SENDINFO_VECTORELEM( m_attachedAnglesPlayerSpace, 0 ), 11 ),
    SendPropAngle( SENDINFO_VECTORELEM( m_attachedAnglesPlayerSpace, 1 ), 11 ),
    SendPropAngle( SENDINFO_VECTORELEM( m_attachedAnglesPlayerSpace, 2 ), 11 ),
#endif
END_NETWORK_TABLE()

IMPLEMENT_NETWORKCLASS_ALIASED( ZMWeaponHands, DT_ZM_WeaponHands )

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CZMWeaponHands )
END_PREDICTION_DATA()
#endif



LINK_ENTITY_TO_CLASS( weapon_zm_fistscarry, CZMWeaponHands );
#ifdef GAME_DLL
LINK_ENTITY_TO_CLASS( weapon_zm_carry, CZMWeaponHands );
#endif
PRECACHE_WEAPON_REGISTER( weapon_zm_fistscarry );

acttable_t CZMWeaponHands::m_acttable[] = 
{
    { ACT_MP_STAND_IDLE,				ACT_HL2MP_IDLE_MELEE,                   false },
    { ACT_MP_CROUCH_IDLE,				ACT_HL2MP_IDLE_CROUCH_MELEE,			false },
    { ACT_MP_RUN,					    ACT_HL2MP_RUN_MELEE,					false },
    { ACT_MP_CROUCHWALK,			    ACT_HL2MP_WALK_CROUCH_MELEE,			false },
    { ACT_MP_ATTACK_STAND_PRIMARYFIRE,  ACT_HL2MP_GESTURE_RANGE_ATTACK_MELEE,   false },
    { ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	ACT_HL2MP_GESTURE_RANGE_ATTACK_MELEE,	false },
    { ACT_MP_RELOAD_STAND,			    ACT_HL2MP_GESTURE_RELOAD_PHYSGUN,			false },
    { ACT_MP_RELOAD_CROUCH,				ACT_HL2MP_GESTURE_RELOAD_PHYSGUN,			false },
    { ACT_MP_JUMP,					    ACT_HL2MP_JUMP_MELEE,					false },
};
IMPLEMENT_ACTTABLE( CZMWeaponHands );


CZMWeaponHands::CZMWeaponHands()
{
#ifdef CLIENT_DLL
    m_bResetPhysicsObject = false;
#endif

    SetSlotFlag( ZMWEAPONSLOT_NONE );
    SetConfigSlot( ZMWeaponConfig::ZMCONFIGSLOT_FISTSCARRY );
}

void CZMWeaponHands::ItemPreFrame()
{
    BaseClass::ItemPreFrame();

#ifdef CLIENT_DLL
    C_BasePlayer* pLocal = C_BasePlayer::GetLocalPlayer();

    if ( pLocal && pLocal->IsAlive() && pLocal == GetPlayerOwner() )
        ManagePredictedObject();
#endif

    // Update the object if the weapon is switched on.
    if( IsCarryingObject() )
    {
        UpdateObject();
    }
}

void CZMWeaponHands::ItemPostFrame()
{
    auto* pOwner = GetPlayerOwner();
    if ( !pOwner )
        return;


    if ( pOwner->m_nButtons & IN_ATTACK )
    {
        PrimaryAttack();
    }
    if ( pOwner->m_nButtons & IN_ATTACK2 )
    {
        SecondaryAttack();
    }
    if ( pOwner->m_nButtons & IN_ATTACK3 )
    {
        TertiaryAttack();
    }


    CheckForTarget();
    WeaponIdle();

    // Pass to base to call anim event attacks.
    BaseClass::ItemPostFrame();
}

//
// We want to punch / throw object
//
void CZMWeaponHands::PrimaryAttack()
{
    if ( IsCarryingObject() )
    {
        // Throw instead.
        TertiaryAttack();
        return;
    }


    if ( m_flNextPrimaryAttack > gpGlobals->curtime )
        return;


    Swing( false );


    float delay = GetPrimaryFireRate();

    m_flNextPrimaryAttack = gpGlobals->curtime + delay;
    m_flNextSecondaryAttack = gpGlobals->curtime + delay;
}

//
// Pickup / pull
//
void CZMWeaponHands::SecondaryAttack()
{
    if ( !IsAbleToPickupObjects() )
        return;


    auto* pOwner = GetPlayerOwner();
    
    if ( !pOwner )
        return;


    if ( IsCarryingObject() )
    {
        //
        // Drop the held object
        //
        m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->curtime + 0.5f;

        DetachObject();
    }
    else
    {
        //
        // Pull/pickup object
        //
        auto* pEnt = FindObject();

        if ( pEnt )
        {
            bool bDone = false;

            // If we hit something, pick it up or pull it
            if ( CanPickupObject( pEnt ) )
            {
#ifdef GAME_DLL
                // This will return false if the prop is constrainted and being detached.
                if ( !Pickup_OnAttemptPhysGunPickup( pEnt, pOwner, PICKED_UP_BY_CANNON ) )
                    bDone = true;
#endif

                // Try attaching
                if ( !bDone && AttachObject( pEnt ) )
                {
                    m_flNextSecondaryAttack = gpGlobals->curtime + 0.5f;
                    bDone = true;
                }
            }

            // If all else fails, pull
            // Puller must be on ground.
            if ( !bDone && pOwner->GetFlags() & FL_ONGROUND )
            {
               PullObject( pEnt );


               if ( GetActivity() != HOLD_ACTIVITY )
                   SendWeaponAnim( HOLD_ACTIVITY );

               m_flTimeWeaponIdle = gpGlobals->curtime + 0.2f;


               m_flNextSecondaryAttack = gpGlobals->curtime + OBJECT_MOVE_DELTA;
            }
        }
    }
}

//
// Push / throw
//
void CZMWeaponHands::TertiaryAttack()
{
    if ( !IsAbleToPickupObjects() )
        return;

    auto* pOwner = GetPlayerOwner();
    
    if ( !pOwner )
        return;


    if( IsCarryingObject() )
    {
        //
        // Punch the object being held
        //
        LaunchObject();

        return;
    }

    
    //
    // Push object in front of us
    //
    Vector hitPos;
    CBaseEntity* pEntity = FindObject( &hitPos );

    if ( !pEntity )
        return;

    // Allow pushing in the air in case they get stuck or something.
    //if ( pOwner->GetFlags() & FL_ONGROUND )
    {
        PushObject( pEntity, hitPos );
    }


    if ( GetActivity() != HOLD_ACTIVITY )
        SendWeaponAnim( HOLD_ACTIVITY );

    m_flTimeWeaponIdle = gpGlobals->curtime + 0.2f;


    m_flNextPrimaryAttack = gpGlobals->curtime + 0.2f;
    m_flNextSecondaryAttack = gpGlobals->curtime + OBJECT_MOVE_DELTA;
}

#ifdef CLIENT_DLL
ConVar zm_cl_drawhands( "zm_cl_drawhands", "1", FCVAR_ARCHIVE );

bool CZMWeaponHands::IsOverridingViewmodel()
{
    return !zm_cl_drawhands.GetBool();
}

void CZMWeaponHands::OnDataChanged( DataUpdateType_t type )
{
    BaseClass::OnDataChanged( type );

    if ( type == DATA_UPDATE_CREATED )
    {
        SetNextClientThink( CLIENT_THINK_ALWAYS );
    }

    if ( !GetOwner() )
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
}
#endif

bool CZMWeaponHands::Deploy()
{
    bool bReturn = BaseClass::Deploy();


    // Reset attack times to allow instant pickup.
    m_flNextSecondaryAttack = gpGlobals->curtime;


    auto* pOwner = GetPlayerOwner();

    if ( pOwner )
    {
        pOwner->SetNextAttack( gpGlobals->curtime );
    }

    return bReturn;
}

void CZMWeaponHands::ForceDrop( CBaseEntity* pEnt )
{
    if ( pEnt && GetHeldObject() == pEnt )
        return;

    DetachObject( false );
}

void CZMWeaponHands::Drop( const Vector& vecVelocity )
{
    DetachObject( false );

    BaseClass::Drop( vecVelocity );
}

bool CZMWeaponHands::CanHolster() const
{ 
    // Don't holster this weapon if we're holding onto something
    if ( IsCarryingObject() )
        return false;

    return BaseClass::CanHolster();
}

Activity CZMWeaponHands::GetIdleActivity() const
{
    return IsCarryingObject() ? HOLD_ACTIVITY : ACT_VM_IDLE;
}

bool CZMWeaponHands::Holster( CBaseCombatWeapon *pSwitchingTo )
{
    // Don't holster this weapon if we're holding onto something
    if ( IsCarryingObject() )
        return false;


    return BaseClass::Holster( pSwitchingTo );
}

#ifndef CLIENT_DLL
void CZMWeaponHands::Physgun_OnPhysGunPickup( CBaseEntity *pEntity, CBasePlayer *pOwner, PhysGunPickup_t reason )
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

bool CZMWeaponHands::PushObject( CBaseEntity* pEnt, const Vector& vecHitPos )
{
#ifndef CLIENT_DLL
    CBasePlayer* pOwner = GetPlayerOwner();
    if ( !pOwner )
        return false;


    //CTakeDamageInfo info;
    //info.SetAttacker( pOwner );
    //info.SetInflictor( this );
    //info.SetDamage( 0.0f );
    //info.SetDamageType( DMG_PHYSGUN );
    //pEntity->DispatchTraceAttack( info, forward, &tr );
    //ApplyMultiDamage();


    Vector forward;
    pOwner->EyeVectors( &forward );


    if ( !Pickup_OnAttemptPhysGunPickup( pEnt, pOwner, PUNTED_BY_CANNON ) )
        return false;

    IPhysicsObject* pObj = pEnt->VPhysicsGetObject();
    if ( !pObj )
        return false;

    if ( (pObj->GetGameFlags() & FVPHYSICS_CONSTRAINT_STATIC) )
        return false;


    if( forward.z < 0 )
    {
        //reflect, but flatten the trajectory out a bit so it's easier to hit standing targets
        forward.z *= -0.65f;
        forward.NormalizeInPlace();
    }
                
    // NOTE: Do this first to enable motion (if disabled) - so forces will work
    // Tell the object it's been punted
    Physgun_OnPhysGunPickup( pEnt, pOwner, PUNTED_BY_CANNON );



    Vector vel, veldir;
    AngularImpulse temp;
    pObj->GetVelocity( &vel, &temp );
    veldir = vel.Normalized();


    Vector objdir = pEnt->WorldSpaceCenter() - pOwner->EyePosition();
    objdir.NormalizeInPlace();


    const float flMaxPushSpd = OBJECT_PUSH_MAXSPD;

    float spd = vel.Length() * objdir.Dot( veldir );
    spd = clamp( spd, 0.0f, flMaxPushSpd );
    
    float ratio = (flMaxPushSpd - spd) / flMaxPushSpd;

    pObj->ApplyForceCenter( forward * physcannon_pushforce.GetFloat() * ratio );
    pObj->ApplyForceOffset( forward * 30.0f, vecHitPos );
#endif

    return true;
}

//
// Applies throw velocity
//
void CZMWeaponHands::ApplyVelocityBasedForce( CBaseEntity *pEntity, const Vector &forward )
{
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
    AngularImpulse aVel = RandomAngularImpulse( -600, 600 );
        
    pPhysicsObject->AddVelocity( &vVel, &aVel );
}

float CZMWeaponHands::PickupDistance() const
{
    return physcannon_tracelength.GetFloat();
}

bool CZMWeaponHands::AttachObject( CBaseEntity *pObject )
{
    if ( IsCarryingObject() )
        return false;

    if ( !CanPickupObject( pObject ) )
        return false;


    m_grabController.SetIgnorePitch( false );
    m_grabController.SetAngleAlignment( 0 );

    IPhysicsObject *pPhysics = pObject->VPhysicsGetObject();

    // Must be valid
    if ( !pPhysics )
        return false;

    auto* pOwner = GetPlayerOwner();


    if ( pOwner )
    {
#ifdef GAME_DLL
        // NOTE: This can change the mass; so it must be done before max speed setting
        Physgun_OnPhysGunPickup( pObject, pOwner, PICKED_UP_BY_CANNON );
#endif
    }

    // NOTE :This must happen after OnPhysGunPickup because that can change the mass
    m_grabController.AttachEntity( pOwner, pObject, pPhysics, false );
    m_hAttachedObject = pObject;
    m_attachedPositionObjectSpace = m_grabController.m_attachedPositionObjectSpace;
    m_attachedAnglesPlayerSpace = m_grabController.m_attachedAnglesPlayerSpace;

    m_bResetOwnerEntity = false;

    if ( !m_hAttachedObject->GetOwnerEntity() )
    {
        m_hAttachedObject->SetOwnerEntity( pOwner );
        m_bResetOwnerEntity = true;
    }

    // Don't drop again for a slight delay, in case they were pulling objects near them
    m_flNextSecondaryAttack = gpGlobals->curtime + 0.4f;


    SendWeaponAnim( HOLD_ACTIVITY );

    return true;
}

CBaseEntity* CZMWeaponHands::FindObject( Vector* pvecHitPos ) const
{
    const int iMask = MASK_SHOT|CONTENTS_GRATE;
    const Vector vecHull = Vector( 8, 8, 8 );



    auto* pPlayer = GetPlayerOwner();
    if ( !pPlayer )
        return nullptr;
    
    Vector forward;
    pPlayer->EyeVectors( &forward );


    Vector start = pPlayer->Weapon_ShootPosition();
    Vector end = start + forward * PickupDistance();

    // Try to find an object by looking straight ahead
    trace_t tr;
    CTraceFilterNoOwnerTest filter( pPlayer, COLLISION_GROUP_NONE );
    UTIL_TraceLine( start, end, iMask, &filter, &tr );

    if ( !IsValidTargetObject( tr.m_pEnt ) )
    {
        // Try again with a hull trace
        UTIL_TraceHull( start, end, -vecHull, vecHull, iMask, &filter, &tr );
    }

    CBaseEntity* pEntity = tr.m_pEnt ? tr.m_pEnt->GetRootMoveParent() : nullptr;

    if ( !IsValidTargetObject( pEntity ) )
        return nullptr;


    if ( pvecHitPos )
        *pvecHitPos = tr.endpos;

    return pEntity;
}

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
    CalcClosestPointOnLine( end, playerLine+Vector(0,0,playerMins.z), playerLine+Vector(0,0,playerMaxs.z), nearest, nullptr );

    Vector delta = end - nearest;
    float len = VectorNormalize(delta);
    if ( len < radius )
    {
        end = nearest + radius * delta;
    }

    QAngle angles = TransformAnglesFromPlayerSpace( m_attachedAnglesPlayerSpace, pPlayer );

    // Show overlays of radius
    if ( zm_sv_debug_carry.GetBool() )
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


    matrix3x4_t attachedToWorld;
    Vector offset;
    AngleMatrix( angles, attachedToWorld );
    VectorRotate( m_attachedPositionObjectSpace, attachedToWorld, offset );

    SetTargetPosition( end - offset, angles );

    return true;
}

void CZMWeaponHands::UpdateObject()
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
void CZMWeaponHands::DetachObject( bool bLaunch )
{
    if ( !IsCarryingObject() )
        return;


    CBaseEntity* pObject = m_grabController.GetAttached();

    m_grabController.DetachEntity( bLaunch );

#ifdef GAME_DLL
    if ( pObject )
    {
        Pickup_OnPhysGunDrop( pObject, GetPlayerOwner(), bLaunch ? LAUNCHED_BY_CANNON : DROPPED_BY_CANNON );
    }
#endif

    if ( pObject && m_bResetOwnerEntity )
    {
        pObject->SetOwnerEntity( nullptr );
    }


#ifdef GAME_DLL
    auto* pOwner = GetPlayerOwner();
    if ( bLaunch && pOwner )
    {
        Vector fwd;
        pOwner->EyeVectors( &fwd );
        ApplyVelocityBasedForce( pObject, fwd );
    }
#endif

#ifdef CLIENT_DLL
    if ( m_bResetPhysicsObject && m_hAttachedObject.Get() )
    {
        m_hAttachedObject.Get()->VPhysicsDestroyObject();
    }
#endif

    m_hAttachedObject.Set( nullptr );

    m_flTimeWeaponIdle = gpGlobals->curtime + 0.1f;
}


#ifdef CLIENT_DLL
void CZMWeaponHands::ManagePredictedObject()
{
    CBaseEntity *pAttachedObject = m_hAttachedObject.Get();

    if ( m_hAttachedObject )
    {
        // NOTE :This must happen after OnPhysGunPickup because that can change the mass
        if ( pAttachedObject != GetGrabController().GetAttached() )
        {
            IPhysicsObject* pPhysics;
            
            pPhysics = pAttachedObject->VPhysicsGetObject();


            m_bResetPhysicsObject = false;
            if ( !pPhysics )
            {
                solid_t tmpSolid;
                PhysModelParseSolid( tmpSolid, m_hAttachedObject, pAttachedObject->GetModelIndex() );

                pPhysics = pAttachedObject->VPhysicsInitNormal( SOLID_VPHYSICS, 0, false, &tmpSolid );

                m_bResetPhysicsObject = true;
            }

            if ( pPhysics )
            {
                m_grabController.SetIgnorePitch( false );
                m_grabController.SetAngleAlignment( 0 );

                GetGrabController().AttachEntity( GetPlayerOwner(), pAttachedObject, pPhysics, false );
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

            if ( m_bResetPhysicsObject )
                m_hOldAttachedObject->VPhysicsDestroyObject();
        }
    }

    m_hOldAttachedObject = m_hAttachedObject;
}

#endif

bool CZMWeaponHands::CheckForTarget()
{
    auto* pOwner = GetPlayerOwner();

    if ( !pOwner )
        return false;

    if ( IsCarryingObject() )
        return false;


    Activity act = GetActivity();
    if ( act != ACT_VM_IDLE && act != HOLD_ACTIVITY && GetWeaponIdleTime() > gpGlobals->curtime )
        return false;


    Vector fwd;
    pOwner->EyeVectors( &fwd );

    Vector startPos = pOwner->Weapon_ShootPosition();
    Vector endPos = startPos + fwd * PickupDistance();

    trace_t	tr;
    UTIL_TraceHull( startPos, endPos, -Vector(4,4,4), Vector(4,4,4), MASK_SHOT|CONTENTS_GRATE, pOwner, COLLISION_GROUP_NONE, &tr );

    CBaseEntity* pEnt = tr.m_pEnt;
    if ( tr.fraction != 1.0f && pEnt && CanPickupObject( pEnt ) )
    {
        if ( GetActivity() != HOLD_ACTIVITY )
            SendWeaponAnim( HOLD_ACTIVITY );

        m_flTimeWeaponIdle = gpGlobals->curtime + 0.2f;

        return true;
    }

    return false;
}

void CZMWeaponHands::Hit( trace_t& traceHit, Activity iHitActivity )
{
    BaseClass::Hit( traceHit, iHitActivity );


    AddViewKick();

#ifndef CLIENT_DLL
    PlayAISound();
#endif
}

bool CZMWeaponHands::IsCarryingObject( CBaseEntity* pEnt ) const
{
    return m_hAttachedObject.Get() == pEnt;
}

bool CZMWeaponHands::IsCarryingObject() const
{
    return m_hAttachedObject.Get() != nullptr;
}

float CZMWeaponHands::GetHeldObjectMass() const
{
    auto* pEnt = GetHeldObject();
    if ( !pEnt || !pEnt->VPhysicsGetObject() )
        return 0.0f;

    return pEnt->VPhysicsGetObject()->GetMass();
}

// We have the time to do it?
bool CZMWeaponHands::IsAbleToPickupObjects() const
{
    return m_flNextSecondaryAttack <= gpGlobals->curtime;
}

// Does this entity pass basic checks?
bool CZMWeaponHands::IsValidTargetObject( CBaseEntity* pEnt )
{
    if ( !pEnt || pEnt->IsWorld() )
        return false;

    return pEnt->GetMoveType() == MOVETYPE_VPHYSICS && !pEnt->VPhysicsIsFlesh();
}

bool CZMWeaponHands::TryPickupObject( CBaseEntity* pEnt )
{
    auto* pOwner = GetPlayerOwner();
    if ( !pOwner )
        return false;

    if ( !CanPickupObject( pEnt ) )
        return false;

#ifdef GAME_DLL
    if ( !Pickup_OnAttemptPhysGunPickup( pEnt, pOwner, PICKED_UP_BY_CANNON ) )
        return false;
#endif

    return AttachObject( pEnt );
}

bool CZMWeaponHands::PullObject( CBaseEntity* pEnt )
{
    auto* pOwner = GetPlayerOwner();
    Vector start = pOwner->Weapon_ShootPosition();

    IPhysicsObject* pObj = pEnt->VPhysicsGetObject();
    if ( !pObj ) return false;


    Vector vel, veldir;
    AngularImpulse temp;
    pObj->GetVelocity( &vel, &temp );
    veldir = vel.Normalized();


    Vector pull = start - pEnt->WorldSpaceCenter();
    VectorNormalize( pull );

    
    const float flMaxPullSpd = OBJECT_PULL_MAXSPD;


    float spd = vel.Length() * pull.Dot( veldir );
    spd = clamp( spd, 0.0f, flMaxPullSpd );
    
    float ratio = (flMaxPullSpd - spd) / flMaxPullSpd;

    // Nudge it towards us
    pObj->ApplyForceCenter( pull * physcannon_pullforce.GetFloat() * ratio );

    return true;
}

void CZMWeaponHands::LaunchObject()
{
    Assert( GetHeldObject() == m_grabController.GetAttached() );


    DetachObject( true );


    SendWeaponAnim( ACT_VM_SECONDARYATTACK );


    m_flNextPrimaryAttack = gpGlobals->curtime + 0.25f;
    m_flNextSecondaryAttack = gpGlobals->curtime + 0.5f;
}

bool CZMWeaponHands::CanPickupObject( CBaseEntity* pEnt ) const
{
    if ( !IsValidTargetObject( pEnt ) )
        return false;

#ifdef GAME_DLL
    if ( pEnt->GetBaseAnimating() && pEnt->GetBaseAnimating()->IsDissolving() )
        return false;
#endif

    if ( pEnt->IsEFlagSet( EFL_NO_PHYSCANNON_INTERACTION ) )
        return false;


    auto* pOwner = GetPlayerOwner();
    
    if ( !pOwner )
        return false;

    if ( pOwner->GetGroundEntity() == pEnt )
        return false;

    IPhysicsObject* pObj = pEnt->VPhysicsGetObject();	
    if ( !pObj )
        return false;


    if ( pObj->GetGameFlags() & FVPHYSICS_PLAYER_HELD )
        return false;

#ifdef GAME_DLL
    // ZMRCHANGE
    // Can't be picked up if there are npcs/players on it.
    // However, do allow if our targetname is something.
    // This should stop players from standing on top of objective items.
    if ( STRING( pEnt->GetEntityName() )[0] == NULL )
    {
        auto* pMe = const_cast<CZMWeaponHands*>( this );

        groundlink_t* link;
        groundlink_t* root = (groundlink_t*)pMe->GetDataObject( GROUNDLINK );
        if ( root )
        {
            for ( link = root->nextLink; link != root; link = link->nextLink )
            {
                if ( link->entity && (link->entity->IsBaseZombie() || link->entity->IsPlayer()) )
                {
                    return false;
                }
            }
        }
    }
#endif


    const float flMassLimit = physcannon_maxmass.GetFloat();


    // Must be under our threshold weight
    if ( flMassLimit >= 0.0f && pObj->GetMass() > flMassLimit )
        return false;


	if ( pObj->GetGameFlags() & FVPHYSICS_NO_PLAYER_PICKUP )
		return false;
	if ( pObj->IsHinged() )
		return false;

	if ( !pObj->IsMoveable() )
	{
#ifdef GAME_DLL
        // Allow pickup of phys props that are motion enabled on player pickup
        CPhysicsProp* pProp = dynamic_cast<CPhysicsProp*>( pEnt );
        CPhysBox* pBox = dynamic_cast<CPhysBox*>( pEnt );
        if ( !pProp && !pBox )
            return false;

        if ( pProp && !(pProp->HasSpawnFlags( SF_PHYSPROP_ENABLE_ON_PHYSCANNON )) )
            return false;

        if ( pBox && !(pBox->HasSpawnFlags( SF_PHYSBOX_ENABLE_ON_PHYSCANNON )) )
            return false;
#else
        return false;
#endif
	}

    return true;
}



//
// EXTERNAL API
//
void PhysCannonForceDrop( CBaseCombatWeapon *pActiveWeapon, CBaseEntity *pOnlyIfHoldingThis )
{
    CZMWeaponHands *pCannon = dynamic_cast<CZMWeaponHands *>(pActiveWeapon);
    if ( pCannon )
    {
        pCannon->ForceDrop( pOnlyIfHoldingThis );
    }
}

bool PlayerPickupControllerIsHoldingEntity( CBaseEntity *pPickupControllerEntity, CBaseEntity *pHeldEntity )
{
    Assert( 0 );
    //CPlayerPickupController *pController = dynamic_cast<CPlayerPickupController *>(pPickupControllerEntity);

    //return pController ? pController->IsHoldingEntity( pHeldEntity ) : false;
    return false;
}

float PhysCannonGetHeldObjectMass( CBaseCombatWeapon *pActiveWeapon, IPhysicsObject *pHeldObject )
{
    CZMWeaponHands* pCarry = dynamic_cast<CZMWeaponHands *>(pActiveWeapon);

    return pCarry ? pCarry->GetHeldObjectMass() : 0.0f;
}

CBaseEntity *PhysCannonGetHeldEntity( CBaseCombatWeapon *pActiveWeapon )
{
    CZMWeaponHands* pCarry = dynamic_cast<CZMWeaponHands *>(pActiveWeapon);

    return pCarry ? pCarry->GetGrabController().GetAttached() : nullptr;
}

float PlayerPickupGetHeldObjectMass( CBaseEntity *pPickupControllerEntity, IPhysicsObject *pHeldObject )
{
    Assert( 0 );
    //float mass = 0.0f;
    //CPlayerPickupController *pController = dynamic_cast<CPlayerPickupController *>(pPickupControllerEntity);
    //if ( pController )
    //{
    //    CGrabController &grab = pController->GetGrabController();
    //    mass = grab.GetSavedMass( pHeldObject );
    //}
    //return mass;
    return 0.0f;
}

#ifdef CLIENT_DLL
//extern void FX_GaussExplosion( const Vector &pos, const Vector &dir, int type );
//void CallbackPhyscannonImpact( const CEffectData &data )
//{
//}
//DECLARE_CLIENT_EFFECT( "PhyscannonImpact", CallbackPhyscannonImpact );
#endif

void PlayerPickupObject( CBasePlayer *pPlayer, CBaseEntity *pObject )
{
    Assert( 0 );
    PlayerAttemptPickup( pPlayer, pObject );
//#ifndef CLIENT_DLL
//    
//    //Don't pick up if we don't have a phys object.
//    if ( pObject->VPhysicsGetObject() == NULL )
//         return;
//
//    CPlayerPickupController *pController = (CPlayerPickupController *)CBaseEntity::Create( "player_pickup", pObject->GetAbsOrigin(), vec3_angle, pPlayer );
//    
//    if ( !pController )
//        return;
//
//    pController->Init( pPlayer, pObject );
//
//#endif
}

void PlayerAttemptPickup( CBasePlayer* pPlayer, CBaseEntity* pEntity )
{
    CZMWeaponHands* pWeapon = static_cast<CZMWeaponHands*>( pPlayer->Weapon_OwnsThisType( "weapon_zm_fistscarry" ) );
    if ( !pWeapon )
        return;

    // Don't even switch to the weapon if we can't pick it up.
    if ( !pWeapon->IsAbleToPickupObjects() )
        return;

    if ( !pWeapon->CanPickupObject( pEntity ) )
        return;


    if ( pPlayer->GetActiveWeapon() != pWeapon )
    {
        if ( !pPlayer->Weapon_Switch( pWeapon ) )
            return;
    }


    pWeapon->TryPickupObject( pEntity );
}

#ifdef GAME_DLL
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
