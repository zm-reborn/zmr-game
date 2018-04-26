#pragma once


namespace NPCR
{
    class CBaseNPC;

    class NPCManager
    {
    public:
        NPCManager();
        ~NPCManager();

        float GetLastUpdateTime() { return m_flLastUpdateTime; };
        
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

    private:
        float m_flUpdateStartTime;
        float m_flLastUpdateTime;

        float m_flUpdateSum;
        int m_iLastFrame;

        CUtlVector<CBaseNPC*> m_vNPCs;
    };

    extern NPCManager g_NPCManager;
}
