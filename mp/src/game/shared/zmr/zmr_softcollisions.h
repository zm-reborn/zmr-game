#pragma once



struct ZMSoftCol_t
{
    CBaseEntity* pOrigin;
    CBaseEntity* pOther;
};

class CZMSoftCollisions : 
#ifdef GAME_DLL
    public CAutoGameSystemPerFrame
#else
    // IMPORTANT: We need to call update manually through local player, in prediction, to get results.
    public CAutoGameSystem
#endif
{
public:
#ifdef GAME_DLL
    virtual void FrameUpdatePostEntityThink() OVERRIDE;
#else
    virtual void Update( float frametime ) OVERRIDE;
#endif

    void OnPlayerCollide( CBaseEntity* pOrigin, CBaseEntity* pOther );
#ifdef GAME_DLL
    void OnZombieCollide( CBaseEntity* pOrigin, CBaseEntity* pOther );
#endif

protected:
    void PerformPlayerSoftCollisions();
    void ClearPlayerCollisions();
    
#ifdef GAME_DLL
    void PerformZombieSoftCollisions();
    void ClearZombieCollisions();
#endif

    CUtlVector<ZMSoftCol_t> m_vPlayerCollisions;
#ifdef GAME_DLL
    CUtlVector<ZMSoftCol_t> m_vZombieCollisions;
#endif
};

extern CZMSoftCollisions* GetZMSoftCollisions();
