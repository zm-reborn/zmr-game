#include "cbase.h"
#include "iclientmode.h"
#include "filesystem.h"
#include "KeyValues.h"
#include "cdll_client_int.h"
#include "networkstringtabledefs.h"
#include "baseviewport.h"

#include "zmr/c_zmr_legacy_objpanel.h"
#include "zmr/c_zmr_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


// NOTE: We have to use string tables because of how the info panel parses the file.
// If you don't use this method, it will assume it's just a file and not parse it as HTML if necessary.



extern INetworkStringTable* g_pStringTableInfoPanel;

const char* ZMLegacyObjPanel::GetString()
{
    if ( !g_pStringTableInfoPanel )
        return nullptr;


    int index = g_pStringTableInfoPanel->FindStringIndex( ZM_OBJ_STRINGTABLE_ENTRY );

    if ( index == INVALID_STRING_INDEX )
        return nullptr;


    return g_pStringTableInfoPanel->GetString( index );
}

bool ZMLegacyObjPanel::ObjectivesExist()
{
    const char* sz = GetString();
    if ( !sz || !*sz ) return false;


    return Q_strlen( sz ) > 0;
}

void ZMLegacyObjPanel::ResetObjectives()
{
    if ( !g_pStringTableInfoPanel ) return;


    int index = g_pStringTableInfoPanel->FindStringIndex( ZM_OBJ_STRINGTABLE_ENTRY );

    if ( index == INVALID_STRING_INDEX ) return;

    
    g_pStringTableInfoPanel->SetStringUserData( index, 0, nullptr );
}

bool ZMLegacyObjPanel::SaveToStringTable( const char* mapname )
{
    if ( !g_pStringTableInfoPanel ) return false;


    char* buffer = new char[ZM_OBJ_MAX_SIZE];
    if ( !LoadObjectivesFromFile( mapname, buffer, ZM_OBJ_MAX_SIZE ) )
    {
        delete[] buffer;
        return false;
    }



    int size = Q_strlen( buffer ) + 1;

    g_pStringTableInfoPanel->AddString( false, ZM_OBJ_STRINGTABLE_ENTRY, size, buffer );
    delete[] buffer;

    return true;
}

bool ZMLegacyObjPanel::LoadObjectivesFromFile( const char* mapname, char* buffer, int len )
{
    char* c, *c2, *k;
    char fixedname[128];
    char path[256];

    // Find the last forward or back slash and copy starting from there
    c = nullptr;
    k = (char*)mapname;
    while ( (k = strchr( k, '/' )) != nullptr ) { c = k; ++k; }
    c2 = nullptr;
    k = (char*)mapname;
    while ( (k = strchr( k, '\\' )) != nullptr ) { c2 = k; ++k; }

    if ( c ) mapname = c + 1;
    if ( c2 && c2 >= mapname ) mapname = c2 + 1;


    Q_strncpy( fixedname, mapname, sizeof( fixedname ) );

    // Find the last dot & remove the .bsp
    c = nullptr;
    k = fixedname;
    while ( (k = strchr( k, '.' )) != nullptr ) { c = k; ++k; }

    if ( c ) *c = NULL;


    Q_snprintf( path, sizeof( path ), "maps/%s.txt", fixedname );

    int flen = filesystem->Size( path, "GAME" );

    if ( flen <= 0 )
    {
        DevMsg( "No map info in '%s'!\n", path );
        return false;
    }

    if ( flen >= len )
    {
        Warning( "Map info '%s' exceeds maximum size (%i)!\n", path, len );
        return false;
    }


    FileHandle_t file = filesystem->Open( path, "rb", "GAME" );

    if ( file == FILESYSTEM_INVALID_HANDLE )
    {
        DevMsg( "Couldn't open map info '%s' for read!\n", path );
        return false;
    }



    filesystem->Read( buffer, flen, file );
    filesystem->Close( file );

    buffer[flen] = NULL;

    return true;
}

void ZMLegacyObjPanel::ShowPanel()
{
    if ( !ObjectivesExist() )
    {
        if ( !SaveToStringTable( engine->GetLevelName() ) )
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
    kv->SetString( "msg", ZM_OBJ_STRINGTABLE_ENTRY );
    kv->SetInt( "type", 1 ); // We're an index to a string table.


    viewport->SetData( kv );

    gViewPortInterface->ShowPanel( viewport, true );

    kv->deleteThis();
}

CON_COMMAND( zm_showmaptext, "Displays map info (usually objectives)" )
{
    ZMLegacyObjPanel::ShowPanel();
}
