#pragma once

#include "predicted_viewmodel.h"

#ifdef CLIENT_DLL
#define CZMViewModel C_ZMViewModel
#endif

class CZMViewModel : public CPredictedViewModel
{
public:
    DECLARE_CLASS( CZMViewModel, CPredictedViewModel );
    DECLARE_NETWORKCLASS();
#ifdef CLIENT_DLL
    DECLARE_PREDICTABLE();
#endif

    CZMViewModel();
    ~CZMViewModel();

#ifdef CLIENT_DLL
    virtual int                 CalcOverrideModelIndex() OVERRIDE;
    virtual int                 DrawModel( int flags ) OVERRIDE;
    virtual bool                ShouldReceiveProjectedTextures( int flags ) OVERRIDE;
    virtual C_BaseAnimating*    FindFollowedEntity() OVERRIDE;

    virtual void                UpdateClientSideAnimation() OVERRIDE;
#endif

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
    bool m_bDrawVM; // We have to override this so the client can decide whether to draw it.

    int m_iOverrideModelIndex;
    CBaseCombatWeapon* m_pOverrideModelWeapon;
    CBaseCombatWeapon* m_pLastWeapon;
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
