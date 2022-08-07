#include "cbase.h"
#include "c_zmr_player_ragdoll.h"


#include "c_zmr_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


IMPLEMENT_CLIENTCLASS_DT_NOBASE( C_ZMRagdoll, DT_ZM_Ragdoll, CZMRagdoll )
    RecvPropVector( RECVINFO( m_vecRagdollOrigin ) ),
    RecvPropEHandle( RECVINFO( m_hPlayer ) ),
    RecvPropInt( RECVINFO( m_nModelIndex ) ),
    RecvPropInt( RECVINFO( m_nForceBone ) ),
    RecvPropVector( RECVINFO( m_vecForce ) ),
    RecvPropVector( RECVINFO( m_vecRagdollVelocity ) )
END_RECV_TABLE()


C_ZMRagdoll::C_ZMRagdoll()
{

}

C_ZMRagdoll::~C_ZMRagdoll()
{
    PhysCleanupFrictionSounds( this );

    if ( m_hPlayer )
    {
        m_hPlayer->CreateModelInstance();
    }
}

void C_ZMRagdoll::Interp_Copy( C_BaseAnimatingOverlay *pSourceEntity )
{
    if ( !pSourceEntity )
        return;
    
    VarMapping_t *pSrc = pSourceEntity->GetVarMapping();
    VarMapping_t *pDest = GetVarMapping();
        
    // Find all the VarMapEntry_t's that represent the same variable.
    for ( int i = 0; i < pDest->m_Entries.Count(); i++ )
    {
        VarMapEntry_t *pDestEntry = &pDest->m_Entries[i];
        const char *pszName = pDestEntry->watcher->GetDebugName();
        for ( int j=0; j < pSrc->m_Entries.Count(); j++ )
        {
            VarMapEntry_t *pSrcEntry = &pSrc->m_Entries[j];
            if ( !Q_strcmp( pSrcEntry->watcher->GetDebugName(), pszName ) )
            {
                pDestEntry->watcher->Copy( pSrcEntry->watcher );
                break;
            }
        }
    }
}

void C_ZMRagdoll::ImpactTrace( trace_t* pTrace, int iDamageType, const char* pCustomImpactName )
{
    IPhysicsObject *pPhysicsObject = VPhysicsGetObject();

    if( !pPhysicsObject )
        return;

    Vector dir = pTrace->endpos - pTrace->startpos;

    if ( iDamageType == DMG_BLAST )
    {
        dir *= 4000;  // adjust impact strenght
                
        // apply force at object mass center
        pPhysicsObject->ApplyForceCenter( dir );
    }
    else
    {
        Vector hitpos;  
    
        VectorMA( pTrace->startpos, pTrace->fraction, dir, hitpos );
        VectorNormalize( dir );

        dir *= 4000;  // adjust impact strenght

        // apply force where we hit it
        pPhysicsObject->ApplyForceOffset( dir, hitpos );	

        // Blood spray!
//		FX_CS_BloodSpray( hitpos, dir, 10 );
    }

    m_pRagdoll->ResetRagdollSleepAfterTime();
}


void C_ZMRagdoll::CreateRagdoll()
{
    // First, initialize all our data. If we have the player's entity on our client,
    // then we can make ourselves start out exactly where the player is.
    C_ZMPlayer* pPlayer = dynamic_cast<C_ZMPlayer*>( m_hPlayer.Get() );
    
    if ( pPlayer && !pPlayer->IsDormant() )
    {
        // move my current model instance to the ragdoll's so decals are preserved.
        pPlayer->SnatchModelInstance( this );

        VarMapping_t *varMap = GetVarMapping();

        // Copy all the interpolated vars from the player entity.
        // The entity uses the interpolated history to get bone velocity.
        bool bRemotePlayer = (pPlayer != C_BasePlayer::GetLocalPlayer());			
        if ( bRemotePlayer )
        {
            Interp_Copy( pPlayer );

            SetAbsAngles( pPlayer->GetRenderAngles() );
            GetRotationInterpolator().Reset();

            m_flAnimTime = pPlayer->m_flAnimTime;
            SetSequence( pPlayer->GetSequence() );
            m_flPlaybackRate = pPlayer->GetPlaybackRate();
        }
        else
        {
            // This is the local player, so set them in a default
            // pose and slam their velocity, angles and origin
            SetAbsOrigin( m_vecRagdollOrigin );
            
            SetAbsAngles( pPlayer->GetRenderAngles() );

            SetAbsVelocity( m_vecRagdollVelocity );

            int iSeq = pPlayer->GetSequence();
            if ( iSeq == -1 )
            {
                Assert( false );	// missing walk_lower?
                iSeq = 0;
            }
            
            SetSequence( iSeq );	// walk_lower, basic pose
            SetCycle( 0.0 );

            Interp_Reset( varMap );
        }		
    }
    else
    {
        // overwrite network origin so later interpolation will
        // use this position
        SetNetworkOrigin( m_vecRagdollOrigin );

        SetAbsOrigin( m_vecRagdollOrigin );
        SetAbsVelocity( m_vecRagdollVelocity );

        Interp_Reset( GetVarMapping() );
        
    }

    SetModelIndex( m_nModelIndex );

    // Make us a ragdoll..
    m_nRenderFX = kRenderFxRagdoll;

    auto boneDelta0 = new matrix3x4_t[MAXSTUDIOBONES];
    auto boneDelta1 = new matrix3x4_t[MAXSTUDIOBONES];
    auto currentBones = new matrix3x4_t[MAXSTUDIOBONES];
    const float boneDt = 0.05f;

    if ( pPlayer && !pPlayer->IsDormant() )
    {
        pPlayer->GetRagdollInitBoneArrays( boneDelta0, boneDelta1, currentBones, boneDt );
    }
    else
    {
        GetRagdollInitBoneArrays( boneDelta0, boneDelta1, currentBones, boneDt );
    }

    InitAsClientRagdoll( boneDelta0, boneDelta1, currentBones, boneDt );

    delete[] boneDelta0;
    delete[] boneDelta1;
    delete[] currentBones;
}


void C_ZMRagdoll::OnDataChanged( DataUpdateType_t type )
{
    BaseClass::OnDataChanged( type );

    if ( type == DATA_UPDATE_CREATED )
    {
        CreateRagdoll();
    }
}

IRagdoll* C_ZMRagdoll::GetIRagdoll() const
{
    return m_pRagdoll;
}

void C_ZMRagdoll::UpdateOnRemove( void )
{
    VPhysicsSetObject( nullptr );

    BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: clear out any face/eye values stored in the material system
//-----------------------------------------------------------------------------
void C_ZMRagdoll::SetupWeights( const matrix3x4_t* pBoneToWorld, int nFlexWeightCount, float* pFlexWeights, float* pFlexDelayedWeights )
{
    BaseClass::SetupWeights( pBoneToWorld, nFlexWeightCount, pFlexWeights, pFlexDelayedWeights );

    static float destweight[128];
    static bool bIsInited = false;

    CStudioHdr *hdr = GetModelPtr();
    if ( !hdr )
        return;

    int nFlexDescCount = hdr->numflexdesc();
    if ( nFlexDescCount )
    {
        Assert( !pFlexDelayedWeights );
        memset( pFlexWeights, 0, nFlexWeightCount * sizeof(float) );
    }

    if ( m_iEyeAttachment > 0 )
    {
        matrix3x4_t attToWorld;
        if (GetAttachment( m_iEyeAttachment, attToWorld ))
        {
            Vector local, tmp;
            local.Init( 1000.0f, 0.0f, 0.0f );
            VectorTransform( local, attToWorld, tmp );
            modelrender->SetViewTarget( GetModelPtr(), GetBody(), tmp );
        }
    }
}
