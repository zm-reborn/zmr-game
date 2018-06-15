#pragma once


#ifdef CLIENT_DLL
#include "zmr/npcs/c_zmr_zombiebase.h"
#else
#include "zmr/npcs/zmr_zombiebase.h"
#endif


#ifdef CLIENT_DLL
#define CZMBaseZombie C_ZMBaseZombie
#endif


class CZMZombieManager
{
public:
    int GetNumZombies() const { return m_vZombies.Count(); }
    CZMBaseZombie* GetZombieByIndex( int index ) const { return m_vZombies[index]; }


    template<typename EachFunc>
    inline void ForEachZombieRet( EachFunc func )
    {
        int len = m_vZombies.Count();
        for ( int i = 0; i < len; ++i )
        {
            if ( func( i, m_vZombies[i] ) )
                return;
        }
    }

    template<typename EachFunc>
    inline void ForEachZombie( EachFunc func )
    {
        int len = m_vZombies.Count();
        for ( int i = 0; i < len; ++i )
        {
            func( m_vZombies[i] );
        }
    }

    template<typename EachFunc>
    inline void ForEachAliveZombie( EachFunc func )
    {
        int len = m_vZombies.Count();
        for ( int i = 0; i < len; ++i )
        {
            CZMBaseZombie* pZombie = m_vZombies[i];
            if ( !pZombie->IsAlive() ) continue;

            func( pZombie );
        }
    }

    template<typename EachFunc>
    inline void ForEachSelectedZombie( int iSelectorIndex, EachFunc func )
    {
        int len = m_vZombies.Count();
        for ( int i = 0; i < len; ++i )
        {
            CZMBaseZombie* pZombie = m_vZombies[i];
            if ( !pZombie->IsAlive() ) continue;
            if ( pZombie->GetSelectorIndex() != iSelectorIndex ) continue;

            func( pZombie );
        }
    }

    template<typename EachFunc>
    inline void ForEachSelectedZombie( CZMPlayer* pPlayer, EachFunc func )
    {
        int iSelectorIndex = pPlayer->entindex();
        int len = m_vZombies.Count();
        for ( int i = 0; i < len; ++i )
        {
            CZMBaseZombie* pZombie = m_vZombies[i];
            if ( !pZombie->IsAlive() ) continue;
            if ( pZombie->GetSelectorIndex() != iSelectorIndex ) continue;

            func( pZombie );
        }
    }

private:
    void AddZombie( CZMBaseZombie* pZombie ) { m_vZombies.AddToTail( pZombie ); }
    bool RemoveZombie( CZMBaseZombie* pZombie ) { return m_vZombies.FindAndRemove( pZombie ); }


    CUtlVector<CZMBaseZombie*> m_vZombies;

    friend class CZMBaseZombie;
};

extern CZMZombieManager g_ZombieManager;
