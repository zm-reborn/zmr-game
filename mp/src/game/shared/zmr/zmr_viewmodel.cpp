#include "cbase.h"


#include "zmr/zmr_player_shared.h"
#include "zmr_viewmodel.h"

#ifdef CLIENT_DLL
#include <materialsystem/imaterialvar.h>
#include "proxyentity.h"
#include "ivieweffects.h"
#include "prediction.h"
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
    SendPropExclude( "DT_BaseViewModel", "m_flPoseParameter" ),

    SendPropExclude( "DT_BaseAnimating", "m_flEncodedController" ),

    SendPropExclude( "DT_BaseAnimating", "m_flPlaybackRate" ),
    SendPropExclude( "DT_BaseViewModel", "m_flPlaybackRate" ),

    //SendPropExclude( "DT_BaseAnimating", "m_nSequence" ),
    SendPropExclude( "DT_BaseAnimatingOverlay", "overlay_vars" ),

    SendPropExclude( "DT_ServerAnimationData" , "m_flCycle" ),	
    SendPropExclude( "DT_AnimTimeMustBeFirst" , "m_flAnimTime" ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( C_ZMViewModel )

    DEFINE_PRED_FIELD( m_flCycle, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
    DEFINE_PRED_FIELD( m_flAnimTime, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),

    DEFINE_PRED_FIELD( m_flPlaybackRate, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
    //DEFINE_PRED_ARRAY( m_flPoseParameter, FIELD_FLOAT, MAXSTUDIOPOSEPARAM, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
    DEFINE_PRED_ARRAY_TOL( m_flPoseParameter, FIELD_FLOAT, MAXSTUDIOPOSEPARAM, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK, 0.0f ),

    DEFINE_PRED_ARRAY_TOL( m_flEncodedController, FIELD_FLOAT, MAXSTUDIOBONECTRLS, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK, 0.02f ),

END_PREDICTION_DATA()
#endif


CZMViewModel::CZMViewModel()
{
#ifdef CLIENT_DLL
    m_bDrawVM = true;
    m_iOverrideModelIndex = -1;
    m_pOverrideModelWeapon = nullptr;
    m_pLastWeapon = nullptr;

    m_bInIronSight = false;
    m_flIronSightFrac = 0.0f;
#else
    SetModelColor2( 1.0f, 1.0f, 1.0f );
#endif

    UseClientSideAnimation();

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

void CZMViewModel::CalcViewModelView( CBasePlayer* pOwner, const Vector& eyePosition, const QAngle& eyeAngles )
{
    Vector originalPos = eyePosition;
    QAngle originalAng = eyeAngles;

    Vector newPos = originalPos;
    QAngle newAng = originalAng;

#ifdef CLIENT_DLL
    // Let the viewmodel shake at about 10% of the amplitude of the player's view
    vieweffects->ApplyShake( newPos, newAng, 0.1f );

    PerformIronSight( newPos, newAng );

    PerformLag( newPos, newAng, originalAng );
#endif

    SetLocalOrigin( newPos );
    SetLocalAngles( newAng );
}

#ifdef CLIENT_DLL
static ConVar testbob("testbob", "1");

void C_ZMViewModel::UpdateClientSideAnimation()
{
    PerformAnimBobbing();

    BaseClass::UpdateClientSideAnimation();
}

bool C_ZMViewModel::Interpolate( float currentTime )
{
    // We need to skip the C_BaseViewModel interpolation as it fucks up our client-side cycle.
    return C_BaseAnimating::Interpolate( currentTime );
}

bool C_ZMViewModel::PerformIronSight( Vector& vecOut, QAngle& angOut )
{
    if ( ViewModelIndex() != VMINDEX_WEP ) return false;


    auto iAttachment = LookupAttachment( "ironsight" );
    if ( iAttachment <= 0 ) return false;


    auto* pWeapon = C_ZMViewModel::GetWeapon();

    bool bPrevState = m_bInIronSight;
    bool bChanged = false;
    if ( prediction->IsFirstTimePredicted() )
    {
        bChanged = pWeapon->IsZoomed() != bPrevState;
    }

    if ( bChanged )
    {
        m_bInIronSight = !m_bInIronSight;
    }
    
    if ( m_bInIronSight )
    {
        if ( m_flIronSightFrac != 1.0f )
        {
            m_flIronSightFrac += gpGlobals->frametime * 1.2f;
            m_flIronSightFrac = MIN( m_flIronSightFrac, 1.0f );
        }
    }
    else
    {
        if ( m_flIronSightFrac != 0.0f )
        {
            m_flIronSightFrac -= gpGlobals->frametime * 1.2f;
            m_flIronSightFrac = MAX( m_flIronSightFrac, 0.0f );
        }
    }


    Vector vecLocal;
    Vector vecIronsightPos;
    Vector vecEyePos = vec3_origin;


    // Get the iron sight position relative to the camera position.
    auto* pHdr = GetModelPtr();
    auto& attachment = pHdr->pAttachment( iAttachment-1 );

    // The attachment position is local to the bone.
    // Here we are assuming that the bone is at origin.
    MatrixPosition( attachment.local, vecLocal );

    
    VectorRotate( vecLocal, angOut, vecIronsightPos );


    auto smootherstep = []( float x ) { return x * x * x * (x * (x * 6 - 15) + 10); };

    Vector vecCur = Lerp( smootherstep( m_flIronSightFrac ), vecEyePos, vecIronsightPos );
    

    vecOut -= vecCur;

    return true;
}

bool C_ZMViewModel::PerformLag( Vector& vecPos, QAngle& ang, const QAngle& origAng )
{
    QAngle origAngles = origAng;
    CalcViewModelLag( vecPos, ang, origAngles );

    return true;
}

void C_ZMViewModel::PerformAnimBobbing()
{
    // ZMRTODO: Put this somewhere else.
    auto* pOwner = ToZMPlayer( GetOwner() );
    if ( !pOwner ) return;

    auto* pVM = this;//pOwner->GetViewModel( m_nViewModelIndex );

    int iPoseParamIndex = pVM->LookupPoseParameter( "move_x" );
    int iVerticalPoseParamIndex = pVM->LookupPoseParameter( "ver_aims" );


    //pVM->SetPlaybackRate( 0.2f );

    if ( iVerticalPoseParamIndex != -1 )
    {
        float vert = pOwner->EyeAngles().x;
        vert = clamp( vert, -90.0f, 90.0f );
        vert /= 90.0f;

        pVM->SetPoseParameter( iVerticalPoseParamIndex, vert );
    }

    if ( iPoseParamIndex != -1 && testbob.GetBool() )
    {
        float spd = pOwner->GetLocalVelocity().Length2D();
        float target = spd > 0.1f ? spd / 190.0f : 0.0f;
        target = clamp( target, 0.0f, 1.0f );

        //
        // The pose parameter goes from:
        // Backwards, no movement, forwards
        // In numbers:
        // -1.0 .. 0.0 .. 1
        //
        // But when we retrieve the actual pose parameter it will be:
        //  0.0 .. 0.5 .. 1
        //
        // When setting the pose parameter, we need to use former scale.
        // 

        float cur = pVM->GetPoseParameter( iPoseParamIndex );
        cur = clamp( cur, 0.5f, 1.0f );

        // Translate from
        // 0.5 .. 1 to 0 .. 1
        cur /= 0.5f;
        cur -= 1.0f;

        float add = gpGlobals->frametime * 1.0f;
        if ( target < cur )
        {
            add *= -1.0f * 1.2f;
        }
        //else if ( cur == target )
        //    return;

        float newratio = cur + add;

        pVM->SetPoseParameter( iPoseParamIndex, clamp( newratio, 0.001f, 1.0f ) );
        //pVM->SetPlaybackRate( clamp( newratio, 0.001f, 1.0f ) );
    }
    else
    {
        //pVM->SetPlaybackRate( 1.0f );
    }
}

#ifdef CLIENT_DLL
CON_COMMAND(testbobbingvalue, "")
{
#ifdef CLIENT_DLL
    auto* pOwner = C_ZMPlayer::GetLocalPlayer();
#else
    auto* pOwner = ToZMPlayer( UTIL_GetCommandClient() );
#endif

    if ( !pOwner ) return;


    float value = atof( args.Arg( 1 ) );
    // ZMRTODO: Put this somewhere else.
    auto* pVM = pOwner->GetViewModel( VMINDEX_WEP );

    int iPoseParamIndex = pVM->LookupPoseParameter( "move_x" );
    if ( iPoseParamIndex != -1 )
    {
        pVM->SetPoseParameter( iPoseParamIndex, value );
    }
}
#endif // CLIENT_DLL

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
