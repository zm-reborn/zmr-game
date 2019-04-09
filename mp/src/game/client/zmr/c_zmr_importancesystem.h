#pragma once


#include <vgui/IImage.h>


enum ZMImportance_t
{
    ZMIMPORTANCE_NONE = -1,

    ZMIMPORTANCE_DEV,
    ZMIMPORTANCE_TRUSTED,
    ZMIMPORTANCE_PLAYTESTER,

    ZMIMPORTANCE_MAX
};


//
//
//
class C_ZMImportanceSystem : public CAutoGameSystem
{
public:
    struct ImportanceData_t
    {
        void Init( int userId );
        bool IsValid( int playerIndex ) const;

        int userId;
        ZMImportance_t importance;
    };


    C_ZMImportanceSystem();
    ~C_ZMImportanceSystem();


    virtual void PostInit() OVERRIDE;
    virtual void LevelInitPostEntity() OVERRIDE;



    void InitImages();
    void Reset();

    vgui::IImage* GetPlayerImportanceImageIndex( int playerIndex );

    bool IsCached( int playerIndex );

protected:
    bool LoadFromFile();
    int FindSteamIdIndex( uint64 steamId );


    bool ComputePlayerImportance( int playerIndex );
    vgui::IImage* ImportanceToImage( ZMImportance_t index );
    ZMImportance_t ImportanceNameToIndex( const char* name );

private:
    vgui::IImage* m_pImageDev;
    vgui::IImage* m_pImageTrusted;
    vgui::IImage* m_pImagePlaytester;


    ImportanceData_t m_Importance[MAX_PLAYERS];


    // Sorted 64bit steam ids.
    CUtlVector<uint64> m_vSteamIdIndices;

    CUtlVector<ZMImportance_t> m_vPlayerData;
};

extern C_ZMImportanceSystem g_ZMImportanceSystem;
