#pragma once



#define ZMTIPMANAGER_TOOLTIPS                   0
#define ZMTIPMANAGER_LOADINGSCREEN              1

enum TipParamType_t
{
    TIPPARAMTYPE_NONE = 0,

    TIPPARAMTYPE_KEY,
    TIPPARAMTYPE_CVAR,
};

struct ZMTipQueue_T
{
    int m_iIndex;
    float m_flDisplay;
};


//
class CZMTip
{
public:
    CZMTip( KeyValues* kv, int index );
    ~CZMTip();
    
    
    struct zmtip_param_t
    {
    public:
        zmtip_param_t()
        {
            pszParam = nullptr;
            iParamType = TIPPARAMTYPE_NONE;
        }

        ~zmtip_param_t()
        {
            delete[] pszParam;
            pszParam = nullptr;
        }

        char* pszParam;
        TipParamType_t iParamType;
    };
    

    void LoadUsed( KeyValues* kv );
    void WriteUsed( KeyValues* kv );


    bool ShouldShow() const;

    void Reset();

    const char* GetName() const;
    void SetName( const char* name );

    void FormatMessage( char* buffer, int len ) const;
    const char* GetMessage() const;
    void SetMessage( const char* msg );
    const char* GetIcon() const;
    void SetIcon( const char* icon );
    
    float GetTime() const { return m_flTime; }
    bool GetPulse() const { return m_bPulse; }
    int GetPriority() const { return m_iPriority; }
    int GetTeam() const { return m_iTeam; }
    bool DoSound() const { return m_bSound; }
    bool CanBeQueued() const { return m_bQueue; }
    bool ShowRandom() const { return m_bRandom; }
    int GetLimit() const { return m_nLimit; }

    int GetIndex() const { return m_iIndex; }


    void OnShownInGame();
    void OnShownInLoadingScreen();


    static TipParamType_t TipNameToType( const char* type );
    
private:
    void AddParam( const char* param );


    char* m_pszName;
    char* m_pszMessage;


    int m_iIndex;
    char* m_pszIcon;

    int m_iTeam;
    float m_flTime;
    int m_iPriority;
    bool m_bQueue;
    bool m_bRandom;
    bool m_bPulse;
    bool m_bSound;
    int m_nLimit;
    int m_nLimitPerGame;

    int m_nShown;
    int m_nShownPerGame;


    CUtlVector<zmtip_param_t*> m_vParams;
};
//


//
class CZMTipManager;

typedef CUtlVector<CZMTip*> ZMTips_t;
typedef CUtlVector<const CZMTip*> ZMConstTips_t;
typedef CUtlVector<CZMTipManager*> ZMTipManagers_t;


class CZMTipManager
{
public:
    CZMTipManager( const char* name );
    ~CZMTipManager();


    const char* GetName() const { return m_pszName; }


    CZMTip* FindTipByName( const char* name ) const;
    CZMTip* FindTipByIndex( int index ) const;


    void GetTips( ZMConstTips_t& vec ) const;


    void AddTip( KeyValues* kv );
    void SaveUsed() const;


private:
    void LoadTips();
    void LoadUsed();


    void FormatFilePath( char* buffer, int len ) const;
    void FormatFilePathUsed( char* buffer, int len ) const;


    ZMTips_t m_vTips;


    const char* m_pszName;
};
//


//
class CZMTipSystem : public CAutoGameSystem
{
public:
    CZMTipSystem();
    ~CZMTipSystem();


    virtual void PostInit() OVERRIDE;
    virtual void LevelShutdownPostEntity() OVERRIDE;


    CZMTipManager* GetManagerByName( const char* name ) const;
    CZMTipManager* GetManagerByIndex( int index ) const;


    void SaveUsed() const;

private:
    ZMTipManagers_t m_vManagers;
};


extern CZMTipSystem g_ZMTipSystem;
//


