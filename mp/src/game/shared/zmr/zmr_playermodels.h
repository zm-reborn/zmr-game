#pragma once


//
class CZMPlayerModelData
{
public:
    CZMPlayerModelData( KeyValues* kv, bool bIsCustom = false );
    ~CZMPlayerModelData();

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

class CZMPlayerModelSystem
{
public:
    CZMPlayerModelSystem();
    ~CZMPlayerModelSystem();


    ZMPlayerModelList_t* GetPlayerModels() { return &m_vPlayerModels; }
    static const char* GetDefaultPlayerModel();


    int LoadModelsFromFile();
    int PrecachePlayerModels();




    CZMPlayerModelData*     GetRandomPlayerModel() const;
    CZMPlayerModelData*     GetPlayerModelData( const char* model ) const;

private:
    int FindPlayerModel( const char* model ) const;

    int LoadModelData( KeyValues* kv );
    void AddFallbackModel();


    ZMPlayerModelList_t m_vPlayerModels;
};
//


extern CZMPlayerModelSystem* ZMGetPlayerModels();
