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

    CZMViewModel();
    ~CZMViewModel();

#ifdef CLIENT_DLL
    virtual int                 DrawModel( int flags ) OVERRIDE;
    virtual C_BaseAnimating*    FindFollowedEntity() OVERRIDE;
#endif

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
