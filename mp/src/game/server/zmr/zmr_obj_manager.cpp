#include "cbase.h"
#include "filesystem.h"
#include "eventqueue.h"

#include "zmr_player.h"
#include "zmr_gamerules.h"
#include "zmr_obj_manager.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


LINK_ENTITY_TO_CLASS( zm_objectives_manager, CZMEntObjectivesManager );

BEGIN_DATADESC( CZMEntObjectivesManager )
END_DATADESC()


CZMEntObjectivesManager::CZMEntObjectivesManager()
{
    m_bDeleteGameText = false;
    m_bLoaded = false;

    m_pDefObjHuman = nullptr;
    m_pDefObjZM = nullptr;

    m_vInputs.Purge();
    m_vStartInputs.Purge();


    CZMRules* pRules = ZMRules();
    if ( pRules ) pRules->SetObjManager( this );
}

CZMEntObjectivesManager::~CZMEntObjectivesManager()
{
    m_vInputs.PurgeAndDeleteElements();
    m_vStartInputs.PurgeAndDeleteElements();


    CZMRules* pRules = ZMRules();
    if ( pRules ) pRules->SetObjManager( nullptr );
}

void CZMEntObjectivesManager::Spawn( void )
{
    BaseClass::Spawn();
}

void CZMEntObjectivesManager::InitObjectiveEntities()
{
    bool bFound = false;

    // Find the map's default human/zm objective entities.
    CZMEntObjectives* pObj = nullptr;
    CBaseEntity* pEnt = nullptr;
    while ( (pEnt = gEntList.FindEntityByClassname( pEnt, "info_objectives" )) != nullptr )
    {
        pObj = dynamic_cast<CZMEntObjectives*>( pEnt );

        if ( !pObj ) continue;


        switch ( pObj->GetRecipient() )
        {
        case OBJRECIPIENT_HUMANS : m_pDefObjHuman = pObj; break;
        case OBJRECIPIENT_ZM : m_pDefObjZM = pObj; break;
        default : break;
        }

        bFound = true;
    }



    // Load our objectives from file and create a dummy objectives entity that will accept input for us.
    if ( bFound ) return;


    if ( IsLoaded() || LoadFromFile() )
    {
        pObj = static_cast<CZMEntObjectives*>( CBaseEntity::Create( "info_objectives", vec3_origin, vec3_angle ) );

        if ( pObj )
        {
            m_pDefObjHuman = pObj;
        }
    }
}

void CZMEntObjectivesManager::Display( CZMEntObjectives* pObj, CBaseEntity* pActivator, ObjRecipient_t rec )
{
    CRecipientFilter filter;

    pObj->GetRecipientFilter( pActivator, filter, rec );

    if ( filter.GetRecipientCount() < 1 )
        return;

    filter.MakeReliable();


    UserMessageBegin( filter, "ZMObjDisplay" );
        WRITE_BYTE( pObj->GetRecipient() );

        for ( int i = 0; i < NUM_OBJ_LINES; i++ )
        {
            pObj->WriteDisplayUserMsg( i );
        }
    MessageEnd();
}

void CZMEntObjectivesManager::Update( CZMEntObjectives* pObj, CBaseEntity* pActivator, ObjRecipient_t rec )
{
    CRecipientFilter filter;

    pObj->GetRecipientFilter( pActivator, filter, rec );

    if ( filter.GetRecipientCount() < 1 )
        return;

    filter.MakeReliable();


    UserMessageBegin( filter, "ZMObjUpdate" );
        for ( int i = 0; i < NUM_OBJ_LINES; i++ )
        {
            pObj->WriteUpdateUserMsg( i );
        }
    MessageEnd();
}

void CZMEntObjectivesManager::UpdateLine( CZMEntObjectives* pObj, int line, CBaseEntity* pActivator, ObjRecipient_t rec )
{
    CRecipientFilter filter;

    pObj->GetRecipientFilter( pActivator, filter, rec );

    if ( filter.GetRecipientCount() < 1 )
        return;

    filter.MakeReliable();

    
    UserMessageBegin( filter, "ZMObjUpdateLine" );
        WRITE_BYTE( line );
        WRITE_BYTE( pObj->GetRecipient() );
        pObj->WriteDisplayUserMsg( line );
    MessageEnd();
    
}

void CZMEntObjectivesManager::ChangeDisplay( CZMPlayer* pPlayer )
{
    CZMEntObjectives* pObj = nullptr;

    if ( pPlayer->IsHuman() )
    {
        pObj = m_pDefObjHuman.Get();
    }
    else if ( pPlayer->IsZM() )
    {
        pObj = m_pDefObjZM.Get();
    }


    if ( pObj )
    {
        // Add a bit of delay so the client knows its team.
        g_EventQueue.AddEvent( pObj, "DisplayActivator", 0.1f, pPlayer, this );
    }
}

void CZMEntObjectivesManager::CreateInputs()
{
    CBaseEntity* pObjEnt = m_pDefObjHuman;
    if ( !pObjEnt ) return;


    int i;
    variant_t var;
    CBaseEntity* pEnt;
    ObjInputData_t* data;
    ObjStartInputData_t* startdata;

    // Must go the other way.
    for ( i = m_vInputs.Count() - 1; i >= 0; i-- )
    {
        data = m_vInputs.Element( i );


        pEnt = nullptr;
        while ( (pEnt = gEntList.FindEntityByName( pEnt, data->m_szEntName )) != nullptr )
        {
            castable_string_t sArg( data->m_szOutput );
            var.SetString( sArg );

            pEnt->AcceptInput( "AddOutput", nullptr, nullptr, var, 0 );
        }
    }


    for ( i = 0; i < m_vStartInputs.Count(); i++ )
    {
        startdata = m_vStartInputs.Element( i );

        castable_string_t sArg( startdata->m_szValue );
        var.SetInt( atoi( startdata->m_szValue ) );
        var.SetFloat( atof( startdata->m_szValue ) );
        var.SetString( sArg );

        // Never fire it immediately. Clients don't know their team yet.
        g_EventQueue.AddEvent( pObjEnt, startdata->m_szInput, var, 0.1f, nullptr, nullptr );
    }
}

void CZMEntObjectivesManager::RoundStart()
{
    InitObjectiveEntities();


    if ( m_pDefObjHuman.Get() )
        CreateInputs();


    if ( m_bDeleteGameText )
    {
        DevMsg( "Deleting game_texts...\n" );

        CBaseEntity* pEnt = nullptr;
        while ( (pEnt = gEntList.FindEntityByClassname( pEnt, "game_text" )) != nullptr )
        {
            UTIL_Remove( pEnt );
        }
    }
}

void CZMEntObjectivesManager::ResetObjectives()
{
    CRecipientFilter filter;

    CZMEntObjectives::RecipientToFilter( nullptr, filter, OBJRECIPIENT_ALL );

    if ( filter.GetRecipientCount() < 1 )
        return;

    filter.MakeReliable();


    UserMessageBegin( filter, "ZMObjDisplay" );
        WRITE_BYTE( OBJRECIPIENT_ALL );

        for ( int i = 0; i < NUM_OBJ_LINES; i++ )
        {
            CObjLine::WriteEmptyDisplayUserMsg();
        }
    MessageEnd();
}

bool CZMEntObjectivesManager::LoadFromFile()
{
    m_bLoaded = true;


    m_vInputs.PurgeAndDeleteElements();
    m_vStartInputs.PurgeAndDeleteElements();

    char format[256];
    const char* pKeyName;
    KeyValues* pKey;
    CUtlVector<char*> vBuf;
    vBuf.Purge();

    KeyValues* kv = new KeyValues( "Objectives" );
    kv->LoadFromFile( filesystem, UTIL_VarArgs( "maps/%s.zmobj", STRING( gpGlobals->mapname ) ), "GAME" );

    pKey = kv->GetFirstSubKey();

    if ( !pKey ) return false;

    bool bFirstKey = true;

    do
    {
        vBuf.PurgeAndDeleteElements();


        pKeyName = pKey->GetName();

        if ( !pKeyName || !*pKeyName ) continue;


        // Why?
        Q_SplitString( pKeyName, ",", vBuf );

        
        // Do we want to do this at the start of the round?
        bool start = false;

        if ( bFirstKey )
        {
            for ( int i = 0; i < vBuf.Count(); i++ )
            {
                if ( Q_stricmp( vBuf[i], "@RoundStart" ) == 0 )
                {
                    start = true;
                }
                else if ( Q_stricmp( vBuf[i], "@DeleteGameText" ) == 0 )
                {
                    m_bDeleteGameText = true;
                }
            }
        }

        bFirstKey = false;


        const char* pEntName = nullptr;
        const char* pOutput = nullptr;
        format[0] = NULL;
       

        if ( !start )
        {
            if ( vBuf.Count() < 2 ) continue;


            int once = 0;
            float delay = 0.0f;

            pEntName = vBuf[0];
            pOutput = vBuf[1];

            if ( !pEntName || strlen( pEntName ) < 1 )
            {
                Warning( "Invalid entity name in objective file!\n" );
                continue;
            }

            if ( !pOutput || strlen( pOutput ) < 1 )
            {
                Warning( "Invalid output in objective file!\n" );
                continue;
            }

            if ( vBuf.Count() >= 3 )
            {
                delay = atof( vBuf[2] );
            }

            if ( vBuf.Count() >= 4 )
            {
                once = atoi( vBuf[3] ) == 0 ? 0 : -1;
            }

            Q_snprintf( format, sizeof( format ), "%s info_objectives:%s:%s:%.2f:%i", pOutput, "%s", "%s", delay, once );
        }


        // Go through the values and create inputs.
        KeyValues* pValue = pKey->GetFirstValue();

        if ( !pValue )
        {
            continue;
        }

        do
        {
            ReadInput( start, pValue, format, pEntName );
        }
        while ( (pValue = pValue->GetNextValue()) != nullptr );
    }
    while ( (pKey = pKey->GetNextKey()) != nullptr );


    vBuf.PurgeAndDeleteElements();
    kv->deleteThis();

    return true;
}

void CZMEntObjectivesManager::ReadInput( bool start, KeyValues* pValue, const char* format, const char* entname )
{
    const char* name = pValue->GetName();
    const char* value = pValue->GetString();
    if ( value )
    {
        if ( start )
        {
            m_vStartInputs.AddToTail( new ObjStartInputData_t( name, value ) );
        }
        else
        {
            char szOutput[256];
            char szVar[162];
            
            Q_strncpy( szVar, value, sizeof( szVar ) );

            // Remove ':' character.
            char* c = szVar;
            while ( (c = strchr( c, ':' )) != nullptr )
            {
                *c = ' ';
                ++c;
            }

            Q_snprintf( szOutput, sizeof( szOutput ), format, name, szVar );

            m_vInputs.AddToTail( new ObjInputData_t( entname, szOutput ) );
        }
    }
}
