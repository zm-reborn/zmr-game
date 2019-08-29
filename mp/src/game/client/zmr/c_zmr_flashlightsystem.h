#pragma once



struct ZMPotentialPlayer_t
{
    C_ZMPlayer* GetPlayer() const;

    int entindex;
    float dotToLocalView;
};

class CZMFlashlightSystem : public CAutoGameSystemPerFrame
{
public:
    CZMFlashlightSystem();
    ~CZMFlashlightSystem();


    virtual void LevelInitPostEntity() OVERRIDE;
    virtual void LevelShutdownPostEntity() OVERRIDE;

    virtual void Update( float delta ) OVERRIDE;


    bool AddPlayerToExpensiveList( C_ZMPlayer* pPlayer );
    int GetNumExpensiveFlashlights() const;

    static int GetExpensiveFlashlightLimit();

protected:
    void Disable();
    int FindPlayer( int entindex ) const;


    float m_flNextUpdate;

    CUtlVector<ZMPotentialPlayer_t> m_vCurActiveExpensive;
};

extern CZMFlashlightSystem* GetZMFlashlightSystem();
