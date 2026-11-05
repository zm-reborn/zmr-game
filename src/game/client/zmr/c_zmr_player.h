#pragma once

#include "c_baseplayer.h"
#include "beamdraw.h"

#include "c_zmr_charcircle.h"

#include "zmr_playeranimstate.h"
#include "zmr_shareddefs.h"

#include "zmr_viewmodel.h"
#include "weapons/zmr_base.h"
#include "zmr_playerlocaldata.h"


struct ZMFireBulletsInfo_t;
class CZMPlayerAttackTraceFilter;

class C_ZMRagdoll;
class C_ZMBaseWeapon;
class CZMFlashlightEffect;
class C_ZMViewModel;
class C_ZMBaseWeapon;


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

    virtual void Spawn() override;
    virtual void ClientThink() override;
    virtual void PreThink() override;
    virtual bool CreateMove( float delta, CUserCmd* cmd ) override;

    void            OnSpawn();
    virtual void    TeamChange( int iNewTeam ) override;
    static void     TeamChangeStatic( int iNewTeam );


    virtual void            Simulate() override;
    virtual void            CreateLightEffects() override {};
    virtual ShadowType_t    ShadowCastType() override;
    virtual bool            ShouldReceiveProjectedTextures( int flags ) override;
    virtual bool            ShouldDraw() override;
    virtual int             DrawModel( int flags ) override;
    int                     DrawModelAndEffects( int flags );
    virtual const QAngle&   GetRenderAngles() override;
    virtual const QAngle&   EyeAngles() override;
    void                    SetLocalAngles( const QAngle& angles );
    virtual float           GetFOV() override;
    virtual float           GetMinFOV() const override { return 5.0f; };
    static float            GetLocalDefaultFOV();
    virtual void            ProcessMuzzleFlashEvent() override;

    int GetIDTarget() const;

    virtual void UpdateClientSideAnimation() override;
    virtual void CalculateIKLocks( float currentTime ) override;

    virtual C_BaseAnimating*    BecomeRagdollOnClient() override;
    IRagdoll*                   GetRepresentativeRagdoll() const override;
    C_ZMRagdoll*                GetRagdoll() const;
    int                         GetEyeAttachment() const;
    virtual void                CalcView( Vector& eyeOrigin, QAngle& eyeAngles, float& zNear, float& zFar, float& fov ) override;

    void                        DeathCam_Firstperson( C_ZMPlayer* pPlayer, Vector& eyeOrigin, QAngle& eyeAngles, float& zNear, float& zFar, float& fov );
    void                        DeathCam_Thirdperson( C_ZMPlayer* pPlayer, Vector& eyeOrigin, QAngle& eyeAngles, float& zNear, float& zFar, float& fov );

    virtual CStudioHdr*         OnNewModel() override;
    void                        Initialize();
    inline void                 SetLookat( const Vector& pos ) { m_viewtarget = pos; };
    inline void                 BlinkEyes() { m_blinktoggle = !m_blinktoggle; };

    virtual void                TraceAttack( const CTakeDamageInfo& info, const Vector& vecDir, trace_t* ptr, CDmgAccumulator* pAccumulator ) override;
    virtual void                DoImpactEffect( trace_t& tr, int nDamageType ) override;

    virtual void                NotifyShouldTransmit( ShouldTransmitState_t state ) override;
    virtual void                OnDataChanged( DataUpdateType_t type ) override;
    virtual void                PostDataUpdate( DataUpdateType_t updateType ) override;
    virtual bool                ShouldInterpolate() override;

    // Custom...
    inline bool IsZM() const { return GetTeamNumber() == ZMTEAM_ZM; }
    inline bool IsHuman() const { return GetTeamNumber() == ZMTEAM_HUMAN; }
    inline bool IsRat() const { return GetTeamNumber() == ZMTEAM_SPECTATOR && !IsObserver(); }

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
    bool                    Weapon_CanSwitchTo( C_BaseCombatWeapon* pWeapon ) override;
    Participation_t         GetParticipation() const;
    static Participation_t  GetLocalParticipation();
    static void             SetLocalParticipation( Participation_t part );
    virtual void            PlayStepSound( Vector& vecOrigin, surfacedata_t* psurface, float fvol, bool force ) override;
    C_BaseCombatWeapon*     GetWeaponForAmmo( int iAmmoType ) const;
    Vector                  GetAttackSpread( C_BaseCombatWeapon* pWeapon, C_BaseEntity* pTarget = nullptr );
    Vector                  GetAutoaimVector( float flScale ) override;
    void                    DoAnimationEvent( PlayerAnimEvent_t playerAnim, int nData = 0 );
    float                   GetAccuracyRatio() const;
    void                    UpdateAccuracyRatio();
    void                    GetZMMovementVars( float& maxspd, float& accel, float& decel ) const;
    virtual void            FireBullets( const FireBulletsInfo_t& info ) override;
    void                    SimulateBullet( ZMFireBulletsInfo_t& bulletinfo );
    bool                    HandleBulletPenetration( trace_t& tr, const ZMFireBulletsInfo_t& bulletinfo, Vector& vecNextSrc, float& flDistance );
    bool                    HandleShotImpactingWater( const FireBulletsInfo_t& info, const Vector& vecEnd, CTraceFilter* pFilter );
    virtual void            DoMuzzleFlash() override;
    int                     GetTotalAmmoAmount( int iValidAmmoIndex ) const;
    int                     GetAmmoRoom( int iValidAmmoIndex ) const;
    bool                    IsFlashlightOn() const;
    C_ZMViewModel*          GetViewModel( int vmIndex = VMINDEX_WEP, bool bObserverOk = true ) const;
    C_ZMBaseWeapon*         GetActiveZMWeapon() const;

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
