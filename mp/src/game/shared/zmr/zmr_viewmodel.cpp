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
#endif
END_NETWORK_TABLE()


CZMViewModel::CZMViewModel()
{
#ifdef CLIENT_DLL
    m_bDrawVM = true;
#else
    SetModelColor2( 1.0f, 1.0f, 1.0f );
#endif
}

CZMViewModel::~CZMViewModel()
{
}

#ifdef CLIENT_DLL
int C_ZMViewModel::DrawModel( int flags )
{
    if ( m_bDrawVM )
    {
        return BaseClass::DrawModel( flags );
    }

    return 0;
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

bool CViewModelColorMaterialProxy::Init( IMaterial* pMaterial, KeyValues* pKeyValues )
{
    bool foundVar;
    m_defaultVar = pMaterial->FindVar( pKeyValues->GetString( "resultVar", "" ), &foundVar, false );


    CSplitString split( pKeyValues->GetString( "default" ), " " );

    if ( split.Count() > 0 ) m_defR = atof( split[0] );
    if ( split.Count() > 1 ) m_defG = atof( split[1] );
    if ( split.Count() > 2 ) m_defB = atof( split[2] );

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
