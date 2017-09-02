#pragma once

struct LayerRecordNPC
{
    int m_sequence;
    float m_cycle;
    float m_weight;
    int m_order;
 
    LayerRecordNPC()
    {
        m_sequence = 0;
        m_cycle = 0;
        m_weight = 0;
        m_order = 0;
    }
 
    LayerRecordNPC( const LayerRecordNPC& src )
    {
        m_sequence = src.m_sequence;
        m_cycle = src.m_cycle;
        m_weight = src.m_weight;
        m_order = src.m_order;
    }
};
 
struct LagRecordNPC
{
public:
    LagRecordNPC()
    {
        m_fFlags = 0;
        m_vecOrigin.Init();
        m_vecAngles.Init();
        m_vecMins.Init();
        m_vecMaxs.Init();
        m_flSimulationTime = -1;
        m_masterSequence = 0;
        m_masterCycle = 0;
    }
 
    LagRecordNPC( const LagRecordNPC& src )
    {
        m_fFlags = src.m_fFlags;
        m_vecOrigin = src.m_vecOrigin;
        m_vecAngles = src.m_vecAngles;
        m_vecMins = src.m_vecMins;
        m_vecMaxs = src.m_vecMaxs;
        m_flSimulationTime = src.m_flSimulationTime;
        for( int layerIndex = 0; layerIndex < CBaseAnimatingOverlay::MAX_OVERLAYS; ++layerIndex )
        {
            m_layerRecords[layerIndex] = src.m_layerRecords[layerIndex];
        }
        m_masterSequence = src.m_masterSequence;
        m_masterCycle = src.m_masterCycle;
    }
 
    // Did NPC die this frame
    int						m_fFlags;
 
    // NPC position, orientation and bbox
    Vector					m_vecOrigin;
    QAngle					m_vecAngles;
    Vector					m_vecMins;
    Vector					m_vecMaxs;
 
    float					m_flSimulationTime;	
 
    // NPC animation details, so we can get the legs in the right spot.
    LayerRecordNPC			m_layerRecords[CBaseAnimatingOverlay::MAX_OVERLAYS];
    int						m_masterSequence;
    float					m_masterCycle;
};

abstract_class CZMNPCLagCompensation
{
public:
    DECLARE_CLASS_NOBASE( CZMNPCLagCompensation )

    CZMNPCLagCompensation()
    {
        m_LagTrack = new CUtlFixedLinkedList<LagRecordNPC>();
    }
    ~CZMNPCLagCompensation()
    {
        m_LagTrack->Purge();
        delete m_LagTrack;
    }


    CUtlFixedLinkedList<LagRecordNPC>* GetLagTrack() { return m_LagTrack; }
    LagRecordNPC*   GetLagRestoreData() { if ( m_RestoreData != NULL ) return m_RestoreData; else return new LagRecordNPC(); }
    LagRecordNPC*   GetLagChangeData() { if ( m_ChangeData != NULL ) return m_ChangeData; else return new LagRecordNPC(); }
    void            SetLagRestoreData(LagRecordNPC* l) { if ( m_RestoreData != NULL ) delete m_RestoreData; m_RestoreData = l; }
    void            SetLagChangeData(LagRecordNPC* l) { if ( m_ChangeData != NULL ) delete m_ChangeData; m_ChangeData = l; }
    void            FlagForLagCompensation( bool tempValue ) { m_bFlaggedForLagCompensation = tempValue; }
    bool            IsLagFlagged() { return m_bFlaggedForLagCompensation; }
    
private:
    CUtlFixedLinkedList<LagRecordNPC>* m_LagTrack;
    LagRecordNPC*   m_RestoreData;
    LagRecordNPC*   m_ChangeData;
    bool            m_bFlaggedForLagCompensation;
};
