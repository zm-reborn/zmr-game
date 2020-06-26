#pragma once


//
class CZMPlayerModelData
{
public:
    CZMPlayerModelData( const KeyValues* kv, bool bIsCustom = false );
    ~CZMPlayerModelData();

    static KeyValues* CreateEmptyModelData( const char* model, const char* name = "" );

    const char* GetModelName() const { return m_szModelName; }
    KeyValues*  GetModelData() const { return m_pKvData; }
    int         GetGender() const { return GetModelData()->GetInt( "gender" ); }
    const char* GetArmModel() const { return GetModelData()->GetString( "armsmodel" ); }
    int         GetArmSkin() const { return GetModelData()->GetInt( "armsskin" ); }
    Color       GetArmColor() const { return GetModelData()->GetColor( "armscolor" ); }
    bool        IsCustom() const { return m_bIsCustom; }

private:
    const char* m_szModelName;
    KeyValues* m_pKvData;
    bool m_bIsCustom;
};
//


//
typedef CUtlVector<CZMPlayerModelData*> ZMPlayerModelList_t;

class CZMPlayerModelSystem : CAutoGameSystem
{
public:
    CZMPlayerModelSystem();
    ~CZMPlayerModelSystem();

#ifdef GAME_DLL
    virtual void LevelInitPreEntity() OVERRIDE;
#endif

    static bool IsDebugging();

    ZMPlayerModelList_t* GetPlayerModels() { return &m_vPlayerModels; }
    static const char* GetDefaultPlayerModel();


    int LoadModelsFromFile();
    int PrecachePlayerModels();

    void ParseCustomModels( ZMPlayerModelList_t& list );
    void SaveCustomModelsToStringTable();


    CZMPlayerModelData*     GetRandomPlayerModel() const;
    CZMPlayerModelData*     GetPlayerModelData( const char* model ) const;

private:
    int FindPlayerModel( const char* model ) const;


    int LoadStockModels();
    int LoadCustomModels();
    int LoadModelData( KeyValues* kv, ZMPlayerModelList_t& list, bool bIsCustom = false );
    void AddFallbackModel();


#ifdef GAME_DLL
    bool m_bLoadedFromFile;
#endif


    ZMPlayerModelList_t m_vPlayerModels;
};
//


extern CZMPlayerModelSystem* ZMGetPlayerModels();
