#pragma once

#include "cbase.h"


class C_ZMRagdoll : public C_BaseAnimatingOverlay
{
public:
    DECLARE_CLASS( C_ZMRagdoll, C_BaseAnimatingOverlay );
    DECLARE_CLIENTCLASS();
    
    C_ZMRagdoll();
    ~C_ZMRagdoll();

    virtual void OnDataChanged( DataUpdateType_t type );

    int GetPlayerEntIndex() const;
    IRagdoll* GetIRagdoll() const;

    void ImpactTrace( trace_t* pTrace, int iDamageType, const char* pCustomImpactName );
    void UpdateOnRemove();
    virtual void SetupWeights( const matrix3x4_t *pBoneToWorld, int nFlexWeightCount, float* pFlexWeights, float* pFlexDelayedWeights );
    
private:
    void Interp_Copy( C_BaseAnimatingOverlay* pDestinationEntity );
    void CreateRagdoll();

private:

    EHANDLE	m_hPlayer;
    CNetworkVector( m_vecRagdollVelocity );
    CNetworkVector( m_vecRagdollOrigin );
};
