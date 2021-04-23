#pragma once

#include <functional>

class CEntityMapData;

//
// Item action system performs custom actions on map entities (items, ie. weapons and ammo)
//
namespace ZMItemAction
{
    // Every item belongs to one or more classes.
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
        CLASS_MELEE_BIG         = ( 1 << 17 ), // Sledge / fireaxe
        CLASS_MELEE_MEDIUM      = ( 1 << 18 ), // Crowbar

        CLASS_DONTUSE           = ( 1 << 19 ), // Class that shouldn't be used. Ie. this item shouldn't be spawned.
    };

    enum ItemSpawnTime_t
    {
        TIME_WORLDRESET = 0, // Mapper placed items on round start
        TIME_ENTSPAWN // Entity spawns (info_loadout)
    };


    //
    // Holds the data for a specific item. An item is tied to an entity classname.
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


    //
    // Holds the data for a specific class. A class is tied to its class name.
    //
    struct ClassData_t
    {
        ClassData_t( const char* name, unsigned int flag )
        {
            m_pszName = name;
            m_fClassFlag = flag;
        }


        const char* m_pszName;
        unsigned int m_fClassFlag;
    };


    //
    // A wrapper for item data.
    // The base data is stored in a keyvalue string. This will cache certain values for easy access.
    //
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


    typedef CUtlVector<ItemEntData_t*> ItemList_t;

    //
    // The data actions will use.
    //
    struct ActionData_t
    {
        bool bIsOldMap;
        ItemList_t items;
    };
    //


    //
    // The base action class.
    // An action will affect items in some way.
    //
    abstract_class CZMMapItemAction
    {
    public:
        virtual int     PerformAction( ActionData_t& actiondata, ItemSpawnTime_t status ) { return false; }
        virtual bool    AffectsItem( ItemEntData_t& itemEntData, ItemSpawnTime_t status ) const { return false; }
    };
    //


    //
    // Add action. Add items to map.
    //
    class CZMMapItemActionAdd : public CZMMapItemAction
    {
    private:
        CZMMapItemActionAdd();
    public:
        ~CZMMapItemActionAdd();


        static void                 LoadActions( KeyValues* kv, CUtlVector<CZMMapItemAction*>& actions );
        static CZMMapItemActionAdd* Create( KeyValues* kv );

        virtual int PerformAction( ActionData_t& actiondata, ItemSpawnTime_t status ) { return false; }
    };
    //


    //
    // Replace action. Replace existing items.
    //
    class CZMMapItemActionReplace : public CZMMapItemAction
    {
    private:
        CZMMapItemActionReplace();
    public:
        ~CZMMapItemActionReplace();


        static void                     LoadActions( KeyValues* kv, CUtlVector<CZMMapItemAction*>& actions );
        static CZMMapItemActionReplace* Create( KeyValues* kv );


        virtual int     PerformAction( ActionData_t& actiondata, ItemSpawnTime_t status ) OVERRIDE;
        virtual bool    AffectsItem( ItemEntData_t& itemEntData, ItemSpawnTime_t status ) const OVERRIDE;

    private:
        bool CalcChance();
        void ReplacePerc( ItemList_t& items );


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
        int m_iFilterItemIndex;

        bool m_bMapItemsOnly;

        bool m_bOnlyOldMaps;


        friend class CZMMapItemActionReplace;
    };
    //


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
        static unsigned int GetClassFlagByName( const char* itemclass );
        static const ItemBaseData_t* GetItemData( int index );
        static const ItemBaseData_t* GetItemData( const char* itemclass );
        static bool         GetMapItemsByClass( unsigned int classflags, CUtlVector<const ItemBaseData_t*>& items );
        static bool         GetMapItems( std::function<bool(const ItemBaseData_t&)> func, CUtlVector<const ItemBaseData_t*>& items );
 



        static const ItemBaseData_t m_ItemData[];
        static const ClassData_t m_Classes[];

        static int FindItemClassByName( const char* itemclass );
        static int FindItemByClassname( const char* classname );

    private:
        bool LoadActionsFromFile( const char* filename );
        bool LoadActionsFromMapFile();

        void GetCopiedData( ItemList_t& items );
        void LoadItemsFromMap();
        void ComputeMapVersion();



        ItemList_t m_vEntData;
        CUtlVector<CZMMapItemAction*> m_vActions;


        bool m_bIsOldMap;
    };
    //

    extern CZMMapItemSystem g_ZMMapItemSystem;
}
