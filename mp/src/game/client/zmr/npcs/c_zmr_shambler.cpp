#include "cbase.h"
#include "gamestringpool.h"


#include "c_zmr_shambler.h"


#define ZOMBIE_SANTAHAT     "models/props/misc/zombie_santahat01.mdl"


extern ConVar zm_sv_happyzombies;
ConVar zm_cl_happyzombies_disable( "zm_cl_happyzombies_disable", "0", 0, "No fun :(" );
ConVar zm_cl_happyzombies_chance( "zm_cl_happyzombies_chance", "0.2", FCVAR_ARCHIVE );



IMPLEMENT_CLIENTCLASS_DT( C_ZMShambler, DT_ZM_Shambler, CZMShambler )
END_RECV_TABLE()

// ZMRTODO: Put this somewhere nice.
static int UTIL_CreateClientModel( const char* pszModel )
{
    int index;
    index = modelinfo->GetModelIndex( pszModel );

    if ( index == -1 )
    {
        // No model found, register our own.
        index = modelinfo->RegisterDynamicModel( pszModel, true );
    }
    
    return index;
}

// ZMRTODO: If we ever care enough, we need to re-do this completely.
// This is not a good way of doing this.
bool C_ZMHolidayHat::Initialize( C_BaseEntity* pOwner, const char* pszModel )
{
    int modelIndex = UTIL_CreateClientModel( pszModel );
    if ( modelIndex == -1 )
        return false;

    if ( !InitializeAsClientEntityByIndex( modelIndex, RENDER_GROUP_OPAQUE_ENTITY ) )
    {
        return false;
    }


    SetOwnerEntity( pOwner );

	SetNextClientThink( CLIENT_THINK_NEVER );

    return true;
}

bool C_ZMHolidayHat::Parent( const char* pszAttachment )
{
    C_BaseEntity* pOwner = GetOwnerEntity();


    int iAttachment = pOwner->LookupAttachment( pszAttachment );

    if ( iAttachment <= 0 )
        return false;


    // Disables annoying asserts.
    bool lastvalid = C_BaseEntity::IsAbsQueriesValid();
    C_BaseEntity::SetAbsQueriesValid( true );

    SetParent( pOwner, iAttachment );
    SetAbsOrigin( pOwner->GetAbsOrigin() );
    SetAbsAngles( vec3_angle );

    C_BaseEntity::SetAbsQueriesValid( lastvalid );

    SetLocalOrigin( vec3_origin );
    SetLocalAngles( vec3_angle );

    UpdateVisibility();

    return true;
}

C_ZMShambler::C_ZMShambler()
{
    SetZombieClass( ZMCLASS_SHAMBLER );
    m_pHat = nullptr;
}

C_ZMShambler::~C_ZMShambler()
{
    ReleaseHat();
}

CStudioHdr* C_ZMShambler::OnNewModel()
{
    CStudioHdr* hdr = BaseClass::OnNewModel();
    

    if (zm_sv_happyzombies.GetInt() > 0
    &&  !zm_cl_happyzombies_disable.GetBool()
    &&  random->RandomFloat( 0.0f, 1.0f ) <= zm_cl_happyzombies_chance.GetFloat())
    {
        CreateHat();

        MakeHappy();
    }

    return hdr;
}

C_BaseAnimating* C_ZMShambler::BecomeRagdollOnClient()
{
    C_BaseAnimating* pRagdoll = BaseClass::BecomeRagdollOnClient();

    ReleaseHat();

    return pRagdoll;
}

void C_ZMShambler::UpdateVisibility()
{
    BaseClass::UpdateVisibility();

    // Stay parented, silly.
    if ( m_pHat )
    {
        m_pHat->UpdateVisibility();

        if ( !IsDormant() )
            m_pHat->Parent( "eyes" );
    }
}

void C_ZMShambler::CreateHat()
{
    ReleaseHat();


    m_pHat = new C_ZMHolidayHat();

    // Add more hats here.
    const char* model = nullptr;

    switch ( zm_sv_happyzombies.GetInt() )
    {
    case 1 :
    default : model = ZOMBIE_SANTAHAT; break;
    }


    if ( !m_pHat || !m_pHat->Initialize( this, model ) || !m_pHat->Parent( "eyes" ) )
    {
        ReleaseHat();
    }
}

void C_ZMShambler::MakeHappy()
{
    SetFlexWeightSafe( "smile", 1.0f );
    SetFlexWeightSafe( "jaw_clench", 1.0f );
    SetFlexWeightSafe( "right_inner_raiser", 1.0f );
    SetFlexWeightSafe( "right_outer_raiser", 1.0f );
    SetFlexWeightSafe( "left_lowerer", 1.0f );
}

LocalFlexController_t C_ZMShambler::GetFlexControllerNumByName( const char* pszName )
{
    for ( LocalFlexController_t i = (LocalFlexController_t)1; i < GetNumFlexControllers(); i++ )
    {
        if ( Q_strcmp( GetFlexControllerName( i ), pszName ) == 0 )
        {
            return i;
        }
    }

    return (LocalFlexController_t)0;
}

void C_ZMShambler::SetFlexWeightSafe( const char* pszName, float value )
{
    LocalFlexController_t cntrl = GetFlexControllerNumByName( pszName );

    if ( cntrl == (LocalFlexController_t)0 )
        return;


    SetFlexWeight( cntrl, value );
}

void C_ZMShambler::ReleaseHat()
{
    if ( m_pHat )
    {
        m_pHat->Release();
        m_pHat = nullptr;
    }
}
