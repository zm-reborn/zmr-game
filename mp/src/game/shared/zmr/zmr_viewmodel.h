#pragma once


#ifdef CLIENT_DLL
#define CZMViewModel C_ZMViewModel
#endif

#include "baseviewmodel_shared.h"

#include "weapons/zmr_base.h"


class CZMPlayer;
class CZMBaseWeapon;


class CZMViewModel : public CBaseViewModel
{
public:
    DECLARE_CLASS( CZMViewModel, CBaseViewModel );
    DECLARE_NETWORKCLASS();
#ifdef CLIENT_DLL
    DECLARE_PREDICTABLE();
#endif

    CZMViewModel();
    ~CZMViewModel();

#ifdef CLIENT_DLL
    virtual int                 CalcOverrideModelIndex() OVERRIDE;
    virtual CStudioHdr*         OnNewModel() OVERRIDE;
    virtual int                 DrawModel( int flags ) OVERRIDE;
    virtual bool                ShouldReceiveProjectedTextures( int flags ) OVERRIDE;
    virtual C_BaseAnimating*    FindFollowedEntity() OVERRIDE;

    virtual void                UpdateClientSideAnimation() OVERRIDE;

    virtual bool                ShouldPredict() OVERRIDE;
    virtual bool                Interpolate( float currentTime ) OVERRIDE;

    CZMBaseWeapon* GetWeapon() const;


    bool CanAnimBob() const;
    bool PerformOldBobbing( Vector& vecPos, QAngle& ang );
    void PerformAnimBobbing();

    bool IsInIronsights() const;
    bool PerformIronSight( Vector& vecPos, QAngle& ang, const QAngle& origAng );

    bool PerformLag( Vector& vecPos, QAngle& ang, const Vector& origPos, const QAngle& origAng );
    bool PerformAngleLag( Vector& vecPos, QAngle& ang, const QAngle& origAng, const Vector& right, const Vector& up );
    bool PerformMovementLag( Vector& vecPos, QAngle& ang, const Vector& fwd, const Vector& right );
    bool PerformImpactLag( Vector& vecPos, QAngle& ang, const Vector& origPos );
#endif

    CZMPlayer* GetOwner() const;

    virtual void CalcViewModelView( CBasePlayer* pOwner, const Vector& eyePosition, const QAngle& eyeAngles ) OVERRIDE;


    void SetWeaponModelEx( const char* pszModel, CBaseCombatWeapon* pWep, bool bOverriden );

    virtual CBaseCombatWeapon* GetOwningWeapon() OVERRIDE;

#ifdef GAME_DLL
    void SetModelColor2( float r, float g, float b ) { m_flClr.Set( 0, r ); m_flClr.Set( 1, g ); m_flClr.Set( 2, b ); };
#else
    void GetModelColor2( float& r, float& g, float& b ) { r = m_flClr[0]; g = m_flClr[1]; b = m_flClr[2]; };

    void SetDrawVM( bool state ) { m_bDrawVM = state; };
#endif

private:
    CNetworkArray( float, m_flClr, 3 );

#ifdef CLIENT_DLL
    // This is used to lag the viewmodel.
    CInterpolatedVar<QAngle> m_LagAnglesHistory;
    QAngle m_vLagAngles;

    CInterpolatedVar<float> m_flLagEyePosZHistory;
    float m_flLagEyePosZ;

    float m_flLastEyePosZ;


    bool m_bDrawVM; // We have to override this so the client can decide whether to draw it.

    int m_iOverrideModelIndex;
    CBaseCombatWeapon* m_pOverrideModelWeapon;
    CBaseCombatWeapon* m_pLastWeapon;

    bool m_bInIronSight;
    float m_flIronSightFrac;

    int m_iPoseParamMoveX;
    int m_iPoseParamVertAim;
    int m_iAttachmentIronsight;

    bool m_bOnGround;
    float m_flGroundTime; // When we left or arrived.

    float m_flImpactTargetDelta;
    float m_flLastImpactDelta;
    float m_flImpactVel;
    float m_flImpactVelOrig;

    Vector m_vecLastVel;
#endif
};

#ifdef CLIENT_DLL
/*
class C_ZMViewModelClient : public C_PredictedViewModel
{
public:
    DECLARE_CLASS( C_ZMViewModelClient, C_PredictedViewModel );

    ~C_ZMViewModelClient();

    virtual C_BaseAnimating* FindFollowedEntity() OVERRIDE;
    virtual bool ShouldInterpolate() OVERRIDE;
    virtual void UpdateVisibility() OVERRIDE;
    virtual void SetWeaponModel( const char* pszModelname, CBaseCombatWeapon* weapon );

};
*/
#endif
