#pragma once

#include "zmr_zombiebase.h"
#include "zmr/zmr_shareddefs.h"


class CZMZombieModelGroup;
class CZMZombieDataPerType;
class CZMZombieModelGroupSystem;


//
class CZMZombieModelData
{
public:
    CZMZombieModelData( KeyValues* kv );
    ~CZMZombieModelData();


    const char* GetModelDataName() const { return STRING( m_sName ); }
    const char* GetModelPath() const { return STRING( m_sModelPath ); }


    unsigned int GetSkinBitFlag() const { return m_fSkins; }
    unsigned int GetBodygroupBitFlag() const { return m_fBodygroups; }

    int GetPrecacheIndex() const { return m_iPrecacheIndex; }


    bool PrecacheMe();


    static bool IsValidModel( const char* mdlpath );

private:
    void CopyData( KeyValues* kv );


    string_t m_sName;
    string_t m_sModelPath;
    unsigned int m_fSkins;
    unsigned int m_fBodygroups;

    int m_iPrecacheIndex;


    friend class CZMZombieDataPerType;
};
//


//
class CZMZombieDataPerType
{
public:
    CZMZombieDataPerType();
    ~CZMZombieDataPerType();

    void AddModelData( KeyValues* kv );

    void LoadSection( KeyValues* kv );

    int FindModelByName( const char* name ) const;

    bool PrecacheMe();

private:
    void CopyData( KeyValues* kv );

    CUtlVector<CZMZombieModelData*> m_vModels;

    friend class CZMZombieModelGroup;
    friend class CZMZombieModelGroupSystem;
};
//


//
class CZMZombieModelGroup
{
public:
    CZMZombieModelGroup( KeyValues* kv );
    ~CZMZombieModelGroup();


    const char* GetModelGroupName() const { return STRING( m_sModelGroup ); }


    const CZMZombieModelData* RandomModel( ZombieClass_t zclass ) const;

private:
    void CopyData( KeyValues* kv );


    string_t m_sModelGroup;

    CZMZombieDataPerType m_Data[ZMCLASS_MAX];

    friend class CZMZombieModelGroupSystem;
};
//


//
class CZMZombieModelGroupSystem : private CAutoGameSystem
{
public:
    CZMZombieModelGroupSystem();
    ~CZMZombieModelGroupSystem();

    //
    virtual void LevelInitPreEntity() OVERRIDE;
    virtual bool Init() OVERRIDE;
    //


    void Reload();


    void OnZombieSpawn( CZMBaseZombie* pZombie );


    const char* GetDefaultModelGroup() const { return STRING( m_sDefaultModelGroup ); }
    void SetDefaultModelGroup( const char* group );

    void PrecacheModelGroup( const char* group );
    bool PrecacheModelGroup( ZombieClass_t zclass, const char* group );

    const CZMZombieModelData* PickRandomModelFromGroup( ZombieClass_t zclass, const char* group ) const;

private:
    void AddModelGroup( KeyValues* kv );
    int FindModelGroupByName( const char* name ) const;


    void LoadFiles();
    void LoadFromFile( KeyValues* kv );


    CUtlVector<CZMZombieModelGroup*> m_vModelGroups;

    string_t m_sDefaultModelGroup;
};
//


extern CZMZombieModelGroupSystem g_ZombieModelGroups;
