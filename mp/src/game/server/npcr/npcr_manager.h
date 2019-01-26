#pragma once


namespace NPCR
{
    class CBaseNPC;

    class NPCManager
    {
    public:
        NPCManager();
        ~NPCManager();


        static double GetProfilingTime();
        
        void RegisterNPC( CBaseNPC* pNPC );
        void UnRegisterNPC( CBaseNPC* pNPC );

        void OnGameFrame();

        void StartUpdate();
        void FinishUpdate();
        bool ShouldUpdate( const CBaseNPC* pNPC ) const;

        template<typename ForEachFunc>
        void ForEachNPC( ForEachFunc func )
        {
            FOR_EACH_VEC( m_vNPCs, i )
            {
                if ( func( m_vNPCs[i] ) )
                    break;
            }
        }


        bool UsesUpdateSlots() const;
        void UpdateSlots();

    private:
        double m_flUpdateStartTime;
        double m_flUpdateSum;

        int m_iLastFrame;

        int m_iCurUpdateSlot;
        int m_nUpdateSlots;

        CUtlVector<CBaseNPC*> m_vNPCs;
    };

    extern NPCManager g_NPCManager;
}
