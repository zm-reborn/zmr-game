#pragma once

#include "zmr/zmr_gamerules.h"
#include "zmr/zmr_objectives.h"


struct ObjInputData_t
{
public:
    ObjInputData_t() {};

    ObjInputData_t( const char* pszEntName, const char* pszOutput )
    {
        Q_strncpy( m_szEntName, pszEntName, sizeof( m_szEntName ) );
        Q_strncpy( m_szOutput, pszOutput, sizeof( m_szOutput ) );
    }

    char m_szEntName[64];
    char m_szOutput[256];
};

struct ObjStartInputData_t
{
public:
    ObjStartInputData_t() {};

    ObjStartInputData_t( const char* pszInput, const char* pszValue )
    {
        Q_strncpy( m_szInput, pszInput, sizeof( m_szInput ) );
        Q_strncpy( m_szValue, pszValue, sizeof( m_szValue ) );
    }

    char m_szInput[64];
    char m_szValue[128];
};

class CZMEntObjectivesManager : public CServerOnlyPointEntity
{
public:
    DECLARE_CLASS( CZMEntObjectivesManager, CServerOnlyPointEntity )
    DECLARE_DATADESC()


    CZMEntObjectivesManager();
    ~CZMEntObjectivesManager();


    void Spawn( void ) OVERRIDE;
    

    static void Display( CZMEntObjectives* pObj, CBaseEntity* pActivator = nullptr, ObjRecipient_t rec = OBJRECIPIENT_INVALID );
    static void Update( CZMEntObjectives* pObj, CBaseEntity* pActivator = nullptr, ObjRecipient_t rec = OBJRECIPIENT_INVALID );
    static void UpdateLine( CZMEntObjectives* pObj, int line, CBaseEntity* pActivator = nullptr, ObjRecipient_t rec = OBJRECIPIENT_INVALID );
    void ChangeDisplay( CZMPlayer* );


    void InitObjectiveEntities();
    void RoundStart();

    static void ResetObjectives();


    inline bool IsLoaded() { return m_bLoaded; };

private:
    bool LoadFromFile();
    void ReadInput( bool start, KeyValues* pValue, const char* format, const char* entname );

    void CreateInputs();

    bool m_bLoaded;
    bool m_bDeleteGameText;

    CUtlVector<ObjInputData_t*> m_vInputs;
    CUtlVector<ObjStartInputData_t*> m_vStartInputs;

    CHandle<CZMEntObjectives> m_pDefObjZM;
    CHandle<CZMEntObjectives> m_pDefObjHuman;
};
