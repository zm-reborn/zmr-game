#pragma once

//#include "c_baseplayer.h"
#include "hl2/c_basehlplayer.h"
#include "beamdraw.h"

#include "c_zmr_charcircle.h"

#include "zmr/zmr_playeranimstate.h"
#include "zmr/zmr_shareddefs.h"

#include "zmr/zmr_playerlocaldata.h"


class C_ZMRagdoll;


class C_ZMPlayer : public C_BaseHLPlayer
{
public:
    DECLARE_CLASS( C_ZMPlayer, C_BaseHLPlayer );
    DECLARE_CLIENTCLASS();
    DECLARE_PREDICTABLE(); 
    DECLARE_INTERPOLATION();


    C_ZMPlayer();
    ~C_ZMPlayer();

    virtual void ClientThink() OVERRIDE;
    virtual void PreThink() OVERRIDE;
    virtual void TeamChange( int ) OVERRIDE;
    virtual bool CreateMove( float delta, CUserCmd* cmd ) OVERRIDE;

    virtual void            AddEntity() OVERRIDE;
    virtual ShadowType_t    ShadowCastType() OVERRIDE;
    virtual bool            ShouldReceiveProjectedTextures( int flags ) OVERRIDE;
    virtual bool            ShouldDraw() OVERRIDE;
    virtual int             DrawModel( int ) OVERRIDE;
    virtual const QAngle&   GetRenderAngles() OVERRIDE;
    virtual const QAngle&   EyeAngles() OVERRIDE;
    virtual float           GetFOV() OVERRIDE;
    virtual float           GetMinFOV() const OVERRIDE { return 5.0f; };

    int GetIDTarget() const;

    virtual void UpdateClientSideAnimation() OVERRIDE;
    virtual void CalculateIKLocks( float currentTime ) OVERRIDE;

    virtual C_BaseAnimating*    BecomeRagdollOnClient() OVERRIDE;
    IRagdoll*                   GetRepresentativeRagdoll() const OVERRIDE;
    virtual void                CalcView( Vector& eyeOrigin, QAngle& eyeAngles, float& zNear, float& zFar, float& fov ) OVERRIDE;

    void                        DeathCam_Firstperson( Vector& eyeOrigin, QAngle& eyeAngles, float& zNear, float& zFar, float& fov );
    void                        DeathCam_Thirdperson( Vector& eyeOrigin, QAngle& eyeAngles, float& zNear, float& zFar, float& fov );

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
    inline bool IsZM() { return GetTeamNumber() == ZMTEAM_ZM; };
    inline bool IsHuman() { return GetTeamNumber() == ZMTEAM_HUMAN; };

    static C_ZMPlayer* GetLocalPlayer();

    // Implemented in zm_player_shared
    bool                    HasEnoughResToSpawn( ZombieClass_t );
    bool                    HasEnoughRes( int );
    int                     GetWeaponSlotFlags();
    int                     GetResources();
    void                    IncResources( int, bool bLimit = false );
    void                    SetResources( int );
    float                   GetFlashlightBattery();
    void                    SetFlashlightBattery( float );
    bool                    Weapon_CanSwitchTo( C_BaseCombatWeapon* ) OVERRIDE;
    Participation_t         GetParticipation();
    static Participation_t  GetLocalParticipation();
    static void             SetLocalParticipation( Participation_t );
    virtual void            PlayStepSound( Vector& vecOrigin, surfacedata_t* psurface, float fvol, bool force ) OVERRIDE;
    C_BaseCombatWeapon*     GetWeaponForAmmo( int iAmmoType );
    Vector                  GetAttackSpread( C_BaseCombatWeapon* pWeapon, C_BaseEntity* pTarget = nullptr );
    Vector                  GetAutoaimVector( float flScale ) OVERRIDE;
    void                    DoAnimationEvent( PlayerAnimEvent_t playerAnim, int nData = 0 );

    void SetMouseWheelMove( float dir );

protected:
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

    void UpdateIDTarget();

    int m_iIDEntIndex;

    void ReleaseFlashlight();
    Beam_t* m_pFlashlightBeam;


    // Only used locally.
    float m_flUpMove;
    float m_flNextUpMove;
};

inline C_ZMPlayer* ToZMPlayer( C_BaseEntity* pEntity )
{
	if ( !pEntity || !pEntity->IsPlayer() )
		return nullptr;

	return dynamic_cast<C_ZMPlayer*>( pEntity );
}

inline C_ZMPlayer* ToZMPlayer( C_BasePlayer* pPlayer )
{
	return static_cast<C_ZMPlayer*>( pPlayer );
}
