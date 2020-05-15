#include "cbase.h"


#include "zmr/zmr_player_shared.h"
#include "zmr_viewmodel.h"

#ifdef CLIENT_DLL
#include <materialsystem/imaterialvar.h>
#include "proxyentity.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


LINK_ENTITY_TO_CLASS( zm_viewmodel, CZMViewModel );

IMPLEMENT_NETWORKCLASS_ALIASED( ZMViewModel, DT_ZM_ViewModel )

BEGIN_NETWORK_TABLE( CZMViewModel, DT_ZM_ViewModel )
#ifdef CLIENT_DLL
    RecvPropFloat( RECVINFO( m_flClr[0] ) ),
    RecvPropFloat( RECVINFO( m_flClr[1] ) ),
    RecvPropFloat( RECVINFO( m_flClr[2] ) ),
#else
    SendPropFloat( SENDINFO_ARRAYELEM( m_flClr, 0 ), -1, 0, 0.0f, 1.0f ),
    SendPropFloat( SENDINFO_ARRAYELEM( m_flClr, 1 ), -1, 0, 0.0f, 1.0f ),
    SendPropFloat( SENDINFO_ARRAYELEM( m_flClr, 2 ), -1, 0, 0.0f, 1.0f ),


    SendPropExclude( "DT_BaseAnimating", "m_flPoseParameter" ),
    SendPropExclude( "DT_BaseAnimating", "m_flEncodedController" ),
    SendPropExclude( "DT_BaseViewModel", "m_flPoseParameter" ),
    //SendPropExclude( "DT_BaseAnimating", "m_flPlaybackRate" ),
    //SendPropExclude( "DT_BaseAnimating", "m_nSequence" ),
    //SendPropExclude( "DT_BaseAnimatingOverlay", "overlay_vars" ),

    //SendPropExclude( "DT_ServerAnimationData" , "m_flCycle" ),	
    //SendPropExclude( "DT_AnimTimeMustBeFirst" , "m_flAnimTime" ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( C_ZMViewModel )

    //DEFINE_PRED_ARRAY( m_flPoseParameter, FIELD_FLOAT, MAXSTUDIOPOSEPARAM, FTYPEDESC_OVERRIDE | FTYPEDESC_NOERRORCHECK ),
    //DEFINE_PRED_ARRAY_TOL( m_flEncodedController, FIELD_FLOAT, MAXSTUDIOBONECTRLS, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE, 0.02f ),

END_PREDICTION_DATA()
#endif


CZMViewModel::CZMViewModel()
{
#ifdef CLIENT_DLL
    m_bDrawVM = true;
    m_iOverrideModelIndex = -1;
    m_pOverrideModelWeapon = nullptr;
    m_pLastWeapon = nullptr;
#else
    SetModelColor2( 1.0f, 1.0f, 1.0f );
#endif

    //UseClientSideAnimation();

    //m_flPlaybackRate = 1.0f;
}

CZMViewModel::~CZMViewModel()
{
}

CBaseCombatWeapon* CZMViewModel::GetOwningWeapon()
{
    auto* pOwner = BaseClass::GetOwningWeapon();
    if ( pOwner )
        return pOwner;


    if ( ViewModelIndex() == VMINDEX_HANDS )
    {
        auto* pPlayer = static_cast<C_ZMPlayer*>( GetOwner() );

        if ( pPlayer )
        {
            CBaseViewModel* vm = pPlayer->GetViewModel( VMINDEX_WEP, false );

            // Apparently this is possible...
            // ???
            if ( vm && vm->ViewModelIndex() == VMINDEX_WEP )
            {
                return vm->GetOwningWeapon();
            }
        }
    }

    return nullptr;
}

void CZMViewModel::SetWeaponModelEx( const char* pszModel, CBaseCombatWeapon* pWep, bool bOverriden )
{
#ifdef CLIENT_DLL
    // Set override model
    auto* pCurWeapon = GetWeapon();

    int newIndex = modelinfo->GetModelIndex( pszModel );

    if ( pWep != pCurWeapon )
    {
        m_iOverrideModelIndex = bOverriden ? newIndex : -1;
        m_pOverrideModelWeapon = pWep;

        m_pLastWeapon = pCurWeapon;
    }

#endif

    SetWeaponModel( pszModel, pWep );
}

#ifdef CLIENT_DLL
int C_ZMViewModel::CalcOverrideModelIndex()
{
    if ( m_iOverrideModelIndex != -1 )
    {
        // HACK: Check if we changed weapons.
        auto* pCurWeapon = GetWeapon();
        if ( pCurWeapon != m_pOverrideModelWeapon && pCurWeapon != m_pLastWeapon )
        {
            // Stop overriding.
            m_iOverrideModelIndex = -1;
            m_pOverrideModelWeapon = nullptr;
        }
    }


    return m_iOverrideModelIndex;
}

int C_ZMViewModel::DrawModel( int flags )
{
    if ( m_bDrawVM )
    {
        return BaseClass::DrawModel( flags );
    }

    return 0;
}

bool C_ZMViewModel::ShouldReceiveProjectedTextures( int flags )
{
    //
    // IMPORTANT: There's a common crash is caused by things that return true in ShouldReceiveProjectedTextures
    // I've not been able to debug it, so fuck it.
    //
    return false;
}

C_BaseAnimating* C_ZMViewModel::FindFollowedEntity()
{
    if ( ViewModelIndex() == VMINDEX_HANDS )
    {
        C_ZMPlayer* pPlayer = static_cast<C_ZMPlayer*>( GetOwner() );

        if ( pPlayer )
        {
            C_BaseViewModel* vm = pPlayer->GetViewModel( VMINDEX_WEP );

            if ( vm )
            {
                return vm;
            }
        }
    }

    return C_BaseAnimating::FindFollowedEntity();
}

class CViewModelColorMaterialProxy : public CEntityMaterialProxy
{
public:
    CViewModelColorMaterialProxy();
    ~CViewModelColorMaterialProxy();

    virtual bool Init( IMaterial* pMaterial, KeyValues* pKeyValues ) OVERRIDE;
    virtual IMaterial* GetMaterial() OVERRIDE;

private:
    virtual void OnBind( C_BaseEntity* pEntity ) OVERRIDE;


    IMaterialVar*   m_defaultVar;
    float           m_defR;
    float           m_defG;
    float           m_defB;
};

CViewModelColorMaterialProxy::CViewModelColorMaterialProxy()
{
    m_defaultVar = nullptr;
    m_defR = m_defG = m_defB = 1.0f;
}

CViewModelColorMaterialProxy::~CViewModelColorMaterialProxy()
{
}

void UTIL_ParseFloatColorFromString( const char* str, float clr[], int nColors );

bool CViewModelColorMaterialProxy::Init( IMaterial* pMaterial, KeyValues* pKeyValues )
{
    bool foundVar;
    m_defaultVar = pMaterial->FindVar( pKeyValues->GetString( "resultVar", "" ), &foundVar, false );


    float clr[3];
    UTIL_ParseFloatColorFromString( pKeyValues->GetString( "default", "" ), clr, ARRAYSIZE( clr ) );

    m_defR = clr[0];
    m_defG = clr[1];
    m_defB = clr[2];

    return foundVar;
}

void CViewModelColorMaterialProxy::OnBind( C_BaseEntity* pEnt )
{
    if ( !m_defaultVar )
        return;


    float r, g, b;
    r = m_defR;
    g = m_defG;
    b = m_defB;


    C_ZMViewModel* pVM = static_cast<C_ZMViewModel*>( pEnt );
    if ( pVM )
    {
        pVM->GetModelColor2( r, g, b );
    }


    m_defaultVar->SetVecValue( r, g, b );
}

IMaterial* CViewModelColorMaterialProxy::GetMaterial()
{
    if ( !m_defaultVar )
        return nullptr;

    return m_defaultVar->GetOwningMaterial();
}

EXPOSE_INTERFACE( CViewModelColorMaterialProxy, IMaterialProxy, "ViewModelColor" IMATERIAL_PROXY_INTERFACE_VERSION );
#endif
