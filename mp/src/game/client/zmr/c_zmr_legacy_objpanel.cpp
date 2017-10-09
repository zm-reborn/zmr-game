#include "cbase.h"
#include "iclientmode.h"
#include "filesystem.h"
#include "KeyValues.h"
#include "cdll_client_int.h"
#include "networkstringtabledefs.h"
#include "baseviewport.h"

#include "zmr/c_zmr_legacy_objpanel.h"
#include "zmr/c_zmr_player.h"


// NOTE: We have to use string tables because of how the info panel parses the file.
// If you don't use this method, it will assume it's just a file and not parse it as HTML if necessary.

#define OBJ_MAX_FILE_SIZE           ( 4 * 1024 )

#define OBJ_STRINGTABLE_ENTRY       "mapinfo"


extern INetworkStringTable* g_pStringTableInfoPanel;

bool ZMLegacyObjPanel::ObjectivesExist()
{
    if ( !g_pStringTableInfoPanel ) return false;


    int index = g_pStringTableInfoPanel->FindStringIndex( OBJ_STRINGTABLE_ENTRY );

    if ( index == INVALID_STRING_INDEX ) return false;

    const char* sz = g_pStringTableInfoPanel->GetString( index );
    if ( !sz || !*sz ) return false;


    return Q_strlen( sz ) > 0;
}

void ZMLegacyObjPanel::ResetObjectives()
{
    if ( !g_pStringTableInfoPanel ) return;


    int index = g_pStringTableInfoPanel->FindStringIndex( OBJ_STRINGTABLE_ENTRY );

    if ( index == INVALID_STRING_INDEX ) return;

    
    g_pStringTableInfoPanel->SetStringUserData( index, 0, nullptr );
}

bool ZMLegacyObjPanel::LoadObjectivesFromFile()
{
    if ( !g_pStringTableInfoPanel ) return false;


    // Will return path to map.
    char szPath[256];
    Q_strncpy( szPath, engine->GetLevelName(), sizeof( szPath ) );

    // Find the last dot & remove the .bsp
    char* c = nullptr;
    char* k = szPath;
    while ( (k = strchr( k, '.' )) != nullptr ) { c = k; ++k; }

    if ( c ) *c = 0;


    Q_snprintf( szPath, sizeof( szPath ), "%s.txt", szPath );

    int len = filesystem->Size( szPath, "GAME" );

    if ( len <= 0 )
    {
        DevMsg( "No map info in '%s'!\n", szPath );
        return false;
    }

    if ( len >= OBJ_MAX_FILE_SIZE )
    {
        Warning( "Map info '%s' exceeds maximum size (%i)!\n", szPath, OBJ_MAX_FILE_SIZE );
        return false;
    }


    FileHandle_t file = filesystem->Open( szPath, "rb", "GAME" );

    if ( file == FILESYSTEM_INVALID_HANDLE )
    {
        DevMsg( "Couldn't open map info '%s' for read!\n", szPath );
        return false;
    }


    char* szBuffer = new char[len + 1];

    filesystem->Read( szBuffer, len, file );
    filesystem->Close( file );

    szBuffer[len] = 0;

    g_pStringTableInfoPanel->AddString( false, OBJ_STRINGTABLE_ENTRY, len + 1, szBuffer );

    delete[] szBuffer;

    return true;
}

void ZMLegacyObjPanel::ShowPanel()
{
    if ( !ObjectivesExist() )
    {
        if ( !LoadObjectivesFromFile() )
            return;
    }


    IViewPortPanel* viewport = gViewPortInterface->FindPanelByName( PANEL_INFO );

    if ( !viewport )
    {
        DevMsg( "Couldn't find viewport with name '%s'!\n", PANEL_INFO );
        return;
    }


    KeyValues* kv = new KeyValues( "I hope nobody will read this... MEMES ARE FUNNY" );

    kv->SetString( "title", "Objectives" );
    kv->SetString( "msg", OBJ_STRINGTABLE_ENTRY );
    kv->SetInt( "type", 1 ); // We're an index to a string table.


    viewport->SetData( kv );

    gViewPortInterface->ShowPanel( viewport, true );

    kv->deleteThis();
}

CON_COMMAND( zm_showmaptext, "Displays map info (usually objectives)" )
{
    ZMLegacyObjPanel::ShowPanel();
}
