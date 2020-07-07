#pragma once

#include "c_baseplayer.h"
#include "beamdraw.h"

#include "c_zmr_charcircle.h"

#include "zmr_playeranimstate.h"
#include "zmr_shareddefs.h"

#include "weapons/zmr_base.h"
#include "zmr_playerlocaldata.h"


struct ZMFireBulletsInfo_t;
class CZMPlayerAttackTraceFilter;

class C_ZMRagdoll;
class C_ZMBaseWeapon;
class CZMFlashlightEffect;


class C_ZMPlayer : public C_BasePlayer
{
public:
    DECLARE_CLASS( C_ZMPlayer, C_BasePlayer );
    DECLARE_CLIENTCLASS();
    DECLARE_PREDICTABLE(); 
    DECLARE_INTERPOLATION();


    C_ZMPlayer();
    ~C_ZMPlayer();

	void				Zoom( float fov, float flTime );
	float				GetZoom();

    virtual void Spawn() OVERRIDE;
    virtual void ClientThink() OVERRIDE;
    virtual void PreThink() OVERRIDE;
    virtual bool CreateMove( float delta, CUserCmd* cmd ) OVERRIDE;

    void            OnSpawn();
    virtual void    TeamChange( int iNewTeam ) OVERRIDE;
    static void     TeamChangeStatic( int iNewTeam );


    virtual void            Simulate() OVERRIDE;
    virtual void            CreateLightEffects() OVERRIDE {};
    virtual ShadowType_t    ShadowCastType() OVERRIDE;
    virtual bool            ShouldReceiveProjectedTextures( int flags ) OVERRIDE;
    virtual bool            ShouldDraw() OVERRIDE;
    virtual int             DrawModel( int flags ) OVERRIDE;
    int                     DrawModelAndEffects( int flags );
    virtual const QAngle&   GetRenderAngles() OVERRIDE;
    virtual const QAngle&   EyeAngles() OVERRIDE;
    void                    SetLocalAngles( const QAngle& angles );
    virtual float           GetFOV() OVERRIDE;
    virtual float           GetMinFOV() const OVERRIDE { return 5.0f; };
    static float            GetLocalDefaultFOV();
    virtual void            ProcessMuzzleFlashEvent() OVERRIDE;

    int GetIDTarget() const;

    virtual void UpdateClientSideAnimation() OVERRIDE;
    virtual void CalculateIKLocks( float currentTime ) OVERRIDE;

    virtual C_BaseAnimating*    BecomeRagdollOnClient() OVERRIDE;
    IRagdoll*                   GetRepresentativeRagdoll() const OVERRIDE;
    C_ZMRagdoll*                GetRagdoll() const;
    int                         GetEyeAttachment() const;
    virtual void                CalcView( Vector& eyeOrigin, QAngle& eyeAngles, float& zNear, float& zFar, float& fov ) OVERRIDE;

    void                        DeathCam_Firstperson( C_ZMPlayer* pPlayer, Vector& eyeOrigin, QAngle& eyeAngles, float& zNear, float& zFar, float& fov );
    void                        DeathCam_Thirdperson( C_ZMPlayer* pPlayer, Vector& eyeOrigin, QAngle& eyeAngles, float& zNear, float& zFar, float& fov );

    virtual CStudioHdr*         OnNewModel() OVERRIDE;
    void                        Initialize();
    inline void                 SetLookat( const Vector& pos ) { m_viewtarget = pos; };
    inline void                 BlinkEyes() { m_blinktoggle = !m_blinktoggle; };

    virtual void                TraceAttack( const CTakeDamageInfo& info, const Vector& vecDir, trace_t* ptr, CDmgAccumulator* pAccumulator ) OVERRIDE;
    virtual void                DoImpactEffect( trace_t& tr, int nDamageType ) OVERRIDE;

    virtual void                NotifyShouldTransmit( ShouldTransmitState_t state ) OVERRIDE;
    virtual void                OnDataChanged( DataUpdateType_t type ) OVERRIDE;
    virtual void                PostDataUpdate( DataUpdateType_t updateType ) OVERRIDE;
    virtual bool                ShouldInterpolate() OVERRIDE;

    // Custom...
    inline bool IsZM() const { return GetTeamNumber() == ZMTEAM_ZM; }
    inline bool IsHuman() const { return GetTeamNumber() == ZMTEAM_HUMAN; }

    static C_ZMPlayer* GetLocalPlayer();

    // Implemented in zm_player_shared
    bool                    HasEnoughResToSpawn( ZombieClass_t zclass ) const;
    bool                    HasEnoughRes( int cost ) const;
    int                     GetWeaponSlotFlags() const;
    int                     GetResources() const;
    void                    IncResources( int res, bool bLimit = false );
    void                    SetResources( int res );
    float                   GetFlashlightBattery() const;
    void                    SetFlashlightBattery( float battery );
    bool                    Weapon_CanSwitchTo( C_BaseCombatWeapon* pWeapon ) OVERRIDE;
    Participation_t         GetParticipation() const;
    static Participation_t  GetLocalParticipation();
    static void             SetLocalParticipation( Participation_t part );
    virtual void            PlayStepSound( Vector& vecOrigin, surfacedata_t* psurface, float fvol, bool force ) OVERRIDE;
    C_BaseCombatWeapon*     GetWeaponForAmmo( int iAmmoType ) const;
    Vector                  GetAttackSpread( C_BaseCombatWeapon* pWeapon, C_BaseEntity* pTarget = nullptr );
    Vector                  GetAutoaimVector( float flScale ) OVERRIDE;
    void                    DoAnimationEvent( PlayerAnimEvent_t playerAnim, int nData = 0 );
    float                   GetAccuracyRatio() const;
    void                    UpdateAccuracyRatio();
    void                    GetZMMovementVars( float& maxspd, float& accel, float& decel ) const;
    virtual void            FireBullets( const FireBulletsInfo_t& info ) OVERRIDE;
    void                    SimulateBullet( ZMFireBulletsInfo_t& bulletinfo );
    bool                    HandleBulletPenetration( trace_t& tr, const ZMFireBulletsInfo_t& bulletinfo, Vector& vecNextSrc, float& flDistance );
    bool                    HandleShotImpactingWater( const FireBulletsInfo_t& info, const Vector& vecEnd, CTraceFilter* pFilter );
    virtual void            DoMuzzleFlash() OVERRIDE;
    int                     GetTotalAmmoAmount( int iValidAmmoIndex ) const;
    int                     GetAmmoRoom( int iValidAmmoIndex ) const;
    bool                    IsFlashlightOn() const;

    void SetMouseWheelMove( float dir );

    bool HasExpensiveFlashlightOn() const;
    void PreferExpensiveFlashlight( bool state );

protected:
    void UpdateFlashlight();


    CZMCharCircle* m_fxInner;
    CZMCharCircle* m_fxHealth;

private:
    CNetworkVarEmbedded( CZMPlayerLocalData, m_ZMLocal );

    QAngle                      m_angEyeAngles;
    CInterpolatedVar<QAngle>    m_iv_angEyeAngles;
    int                         m_iSpawnInterpCounter;
    int                         m_iSpawnInterpCounterCache;
    CHandle<C_ZMRagdoll>        m_hRagdoll;

    CZMPlayerAnimState* m_pPlayerAnimState;
    int m_iAttachmentEyes;
    int m_iAttachmentRH;

    void UpdateIDTarget();

    int m_iIDEntIndex;

    void ReleaseFlashlight();
    CZMFlashlightEffect* m_pFlashlight;


    // Only used locally.
    float m_flUpMove;
    float m_flNextUpMove;

	float				m_flZoomStart;
	float				m_flZoomEnd;
	float				m_flZoomRate;
	float				m_flZoomStartTime;

    friend class CZMGameMovement;
};

inline C_ZMPlayer* ToZMPlayer( C_BaseEntity* pEntity )
{
	if ( !pEntity || !pEntity->IsPlayer() )
		return nullptr;

	return static_cast<C_ZMPlayer*>( pEntity );
}

inline C_ZMPlayer* ToZMPlayer( C_BasePlayer* pPlayer )
{
	return static_cast<C_ZMPlayer*>( pPlayer );
}
