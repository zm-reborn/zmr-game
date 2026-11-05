#pragma once


#include "zmr_grabcontroller.h"
#include "zmr_basemelee.h"


/*
    NOTE:
    
    Remove hl2mp/hl2mp_physcannon.cpp/.h from projects.
*/


#ifdef CLIENT_DLL
#define CZMWeaponHands C_ZMWeaponHands
#endif

class CZMWeaponHands : public CZMBaseMeleeWeapon
{
public:
    DECLARE_CLASS( CZMWeaponHands, CZMBaseMeleeWeapon );
    DECLARE_NETWORKCLASS();
    DECLARE_PREDICTABLE(); // Requires semicolon, thanks Valve.

    CZMWeaponHands();


    virtual void	ItemPreFrame() override;
    virtual void	ItemPostFrame() override;

    virtual void	PrimaryAttack() override;
    virtual void	SecondaryAttack() override;
    void			TertiaryAttack();

    virtual void	Drop( const Vector& vecVelocity ) override;
    void			ForceDrop( CBaseEntity* pEnt );
    virtual bool	CanHolster() const override;
    virtual bool	Holster( CBaseCombatWeapon* pSwitchingTo = nullptr ) override;
    virtual bool	Deploy() override;

    virtual Activity GetIdleActivity() const override;


#ifdef CLIENT_DLL
    virtual bool	ShouldDrawPickup() override { return false; }
    virtual bool    IsOverridingViewmodel() override;
#endif
    virtual bool	HasAnyAmmo() override { return true; }
    virtual bool	CanBeDropped() const override { return false; }


    bool TryPickupObject( CBaseEntity* pEnt );
    bool PullObject( CBaseEntity* pEnt );
    bool PushObject( CBaseEntity* pEnt, const Vector& vecHitPos );

    float               PickupDistance() const;
    float				GetHeldObjectMass() const;
    CGrabController&	GetGrabController() { return m_grabController; }
    CBaseEntity*		GetHeldObject() const { return m_hAttachedObject.Get(); }

    static bool IsValidTargetObject( CBaseEntity* pEnt );

    bool CanPickupObject( CBaseEntity* pEnt ) const;

    bool IsCarryingObject( CBaseEntity* pEnt ) const;
    bool IsCarryingObject() const;
    bool IsAbleToPickupObjects() const;


    virtual float	GetRange() const override { return 45.0f; }

    virtual Activity	GetPrimaryAttackActivity() override { return ACT_VM_HITCENTER; }
    virtual float		GetDamageForActivity( Activity act ) const override { return 5.0f; }
    virtual void		Hit( trace_t& traceHit, Activity iHitActivity ) override;


protected:
    // Pickup and throw objects.
    bool CheckForTarget();
    
    CBaseEntity*	FindObject( Vector* pvecHitPos = nullptr ) const;
    bool			AttachObject( CBaseEntity* pObject );

    void			UpdateObject();
    void			DetachObject( bool bLaunch = false );
    void			LaunchObject();

    // Velocity-based throw common to punt and launch code.
    void			ApplyVelocityBasedForce( CBaseEntity* pEntity, const Vector &forward );

#ifdef GAME_DLL
    // What happens when the physgun picks up something 
    void	Physgun_OnPhysGunPickup( CBaseEntity *pEntity, CBasePlayer *pOwner, PhysGunPickup_t reason );
#endif	// GAME_DLL

#ifdef CLIENT_DLL
    virtual void	OnDataChanged( DataUpdateType_t type ) override;
    
    void			ManagePredictedObject();
#endif	// CLIENT_DLL


private:
    bool	m_bResetOwnerEntity;
#ifdef CLIENT_DLL
    bool	m_bResetPhysicsObject;
#endif

    CGrabController m_grabController;


#ifdef GAME_DLL
    CNetworkQAngle( m_attachedAnglesPlayerSpace );
#else
    QAngle m_attachedAnglesPlayerSpace;
#endif
    CNetworkVector( m_attachedPositionObjectSpace );
    CNetworkHandle( CBaseEntity, m_hAttachedObject );
    EHANDLE m_hOldAttachedObject;
};


bool PlayerHasMegaPhysCannon();

// force the physcannon to drop an object (if carried)
void PhysCannonForceDrop( CBaseCombatWeapon *pActiveWeapon, CBaseEntity *pOnlyIfHoldingThis );
void PhysCannonBeginUpgrade( CBaseAnimating *pAnim );

bool PlayerPickupControllerIsHoldingEntity( CBaseEntity *pPickupController, CBaseEntity *pHeldEntity );
float PlayerPickupGetHeldObjectMass( CBaseEntity *pPickupControllerEntity, IPhysicsObject *pHeldObject );
float PhysCannonGetHeldObjectMass( CBaseCombatWeapon *pActiveWeapon, IPhysicsObject *pHeldObject );

CBaseEntity *PhysCannonGetHeldEntity( CBaseCombatWeapon *pActiveWeapon );

void PlayerAttemptPickup( CBasePlayer* pPlayer, CBaseEntity* pObject );
