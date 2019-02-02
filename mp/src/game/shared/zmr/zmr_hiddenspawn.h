#pragma once


enum HiddenSpawnError_t
{
    HSERROR_OK = 0,

    // Shared
    HSERROR_TOOCLOSE,
    HSERROR_CANSEE,
    HSERROR_NOTENOUGHRES,
    HSERROR_NOTENOUGHPOP,
    HSERROR_BADCLASS,

    // Server
    HSERROR_BLOCKEDSPOT,
    HSERROR_INVALIDSPOT,

    HSERROR_UNKNOWN,
};

class CZMHiddenSpawnSystem
{
public:
    CZMHiddenSpawnSystem();
    ~CZMHiddenSpawnSystem();


    HiddenSpawnError_t Spawn( ZombieClass_t zclass, CZMPlayer* pZM, const Vector& pos, int* pResourceCost = nullptr );


    float   GetMinimumDistance() const;
    int     GetResourcesMax( ZombieClass_t zclass ) const;

    bool CanSee( CZMPlayer* pSurvivor, const Vector& zombiepos ) const;
    bool CanSpawnClass( ZombieClass_t zclass ) const;


    int ComputeResourceCost( ZombieClass_t zclass, float closestZombieDist ) const;

private:
    void Trace( CZMPlayer* pSurvivor, const Vector& endpos, trace_t& tr ) const;
};


extern CZMHiddenSpawnSystem g_ZMHiddenSpawn;
