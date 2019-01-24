#pragma once


class CEntityMapData;

namespace ZMItemAction
{
    enum ItemClass_t
    {
        CLASS_WEAPON            = ( 1 << 0 ),
        CLASS_PRIMARY           = ( 1 << 1 ),
        CLASS_SECONDARY         = ( 1 << 2 ),
        CLASS_SHOTGUN           = ( 1 << 3 ),
        CLASS_SMG               = ( 1 << 4 ),
        CLASS_RIFLE             = ( 1 << 5 ),
        CLASS_PISTOL            = ( 1 << 6 ),
        CLASS_BIGPISTOL         = ( 1 << 7 ),

        CLASS_AMMO              = ( 1 << 8 ),
        CLASS_AMMO_SHOTGUN      = ( 1 << 9 ),
        CLASS_AMMO_RIFLE        = ( 1 << 10 ),
        CLASS_AMMO_SMG          = ( 1 << 11 ),
        CLASS_AMMO_PISTOL       = ( 1 << 12 ),
        CLASS_AMMO_BIGPISTOL    = ( 1 << 13 ),

        CLASS_MOLOTOV           = ( 1 << 14 ),

        CLASS_ITEM_CRATE        = ( 1 << 15 ),

        CLASS_MELEE             = ( 1 << 16 ),
    };

    enum ItemSpawnTime_t
    {
        TIME_WORLDRESET = 0, // Mapper placed items on round start
        TIME_ENTSPAWN // Entity spawns (info_loadout)
    };

    struct ItemBaseData_t;
    struct ItemClassData_t;
    struct ItemEntData_t;
    class CZMMapItemAction;


    //
    class CZMMapItemSystem : public CAutoGameSystem
    {
    public:
        CZMMapItemSystem();
        ~CZMMapItemSystem();


        // Entry points
        virtual void LevelInitPreEntity() OVERRIDE;

        void        SpawnItems();
        const char* OnCreateItem( const char* classname );



        static bool         ShouldAffectEntity( CEntityMapData& entData, char* buffer );
        static bool         AffectsItem( const char* classname );
        static unsigned int GetItemFlags( const char* classname );
        static unsigned int GetClassFlag( const char* classname );
        static bool         GetMapItemsByClass( unsigned int flags, CUtlVector<const ItemBaseData_t*>& items );



        static const ItemBaseData_t m_vItemData[];
        static const ItemClassData_t m_vItemClasses[];

        static int FindClassByName( const char* classname );
        static int FindItemByClassname( const char* classname );

    private:
        bool LoadActionsFromFile( const char* filename );
        bool LoadActionsFromMapFile();

        void GetCopiedData( CUtlVector<ItemEntData_t*>& items );
        void LoadItemsFromMap();



        CUtlVector<ItemEntData_t*> m_vEntData;
        CUtlVector<CZMMapItemAction*> m_vActions;
    };
    //


    //
    class CZMMapItemAction
    {
    public:
        virtual int     PerformAction( CUtlVector<ItemEntData_t*>& items, ItemSpawnTime_t status ) { return false; }
        virtual bool    AffectsItem( ItemEntData_t& itemEntData, ItemSpawnTime_t status ) const { return false; }
    };
    //


    //
    class CZMMapItemActionAdd : public CZMMapItemAction
    {
    private:
        CZMMapItemActionAdd();
    public:
        ~CZMMapItemActionAdd();


        static void                 LoadActions( KeyValues* kv, CUtlVector<CZMMapItemAction*>& actions );
        static CZMMapItemActionAdd* Create( KeyValues* kv );

        virtual int PerformAction( CUtlVector<ItemEntData_t*>& items, ItemSpawnTime_t status ) { return false; }
    };
    //


    //
    class CZMMapItemActionReplace : public CZMMapItemAction
    {
    private:
        CZMMapItemActionReplace();
    public:
        ~CZMMapItemActionReplace();


        static void                     LoadActions( KeyValues* kv, CUtlVector<CZMMapItemAction*>& actions );
        static CZMMapItemActionReplace* Create( KeyValues* kv );


        virtual int     PerformAction( CUtlVector<ItemEntData_t*>& items, ItemSpawnTime_t status ) OVERRIDE;
        virtual bool    AffectsItem( ItemEntData_t& itemEntData, ItemSpawnTime_t status ) const OVERRIDE;

    private:
        bool CalcChance();
        void ReplacePerc( CUtlVector<ItemEntData_t*>& items );


        bool ParseReplaceTarget( const char* target );
        bool ParseReplaceFilter( KeyValues* kv );


        bool Replace( ItemEntData_t& itemEntData );


        float m_flChanceFrac;
        float m_flReplaceFrac;

        CUtlVector<CZMMapItemActionReplace*> m_vSubReplaces;
    


        unsigned int m_fFlag;
        char m_szClassname[64];

        float m_flRangeCheck;
        Vector m_vecRangeCheckPos;
        unsigned int m_fFilterFlags;

        bool m_bMapItemsOnly;


        friend class CZMMapItemActionReplace;
    };
    //


    struct ItemBaseData_t
    {
        ItemBaseData_t( const char* classname, unsigned int flags )
        {
            m_pszClassname = classname;
            m_fClassFlags = flags;
        }

        bool IsWeapon() const { return (m_fClassFlags & CLASS_WEAPON) != 0; }
        bool IsAmmo() const { return (m_fClassFlags & CLASS_AMMO) != 0; }
        bool IsItemCrate() const { return (m_fClassFlags & CLASS_ITEM_CRATE) != 0; }


        const char* m_pszClassname;
        unsigned int m_fClassFlags;
    };

    struct ItemClassData_t
    {
        ItemClassData_t( const char* classname, unsigned int flag )
        {
            m_pszClassname = classname;
            m_fClassFlag = flag;
        }


        const char* m_pszClassname;
        unsigned int m_fClassFlag;
    };


    struct ItemEntData_t
    {
        ItemEntData_t( const char* pszClassname, unsigned int flags )
        {
            fClassFlags = flags;
            pszItemClassname = pszClassname;
            vecPos = vec3_origin;
            pszEntData = nullptr;
            iEntDataSize = -1;
        }

        ItemEntData_t( const char* pszData, int iDataSize, unsigned int flags, Vector pos, const char* pszClassname )
        {
            pszEntData = pszData;
            iEntDataSize = iDataSize;
            fClassFlags = flags;
            vecPos = pos;
            pszItemClassname = pszClassname;
        }

        ~ItemEntData_t()
        {
            delete[] pszEntData;
        }


        static ItemEntData_t* Create( const char* pszEntData );



        const char* pszEntData;
        int iEntDataSize;

        unsigned int fClassFlags;
        Vector vecPos;
        const char* pszItemClassname;
    };



    extern CZMMapItemSystem g_ZMMapItemSystem;
}
