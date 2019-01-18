#pragma once

#define ZM_OBJ_MAX_SIZE                 ( 4 * 1024 )
#define ZM_OBJ_STRINGTABLE_ENTRY        "mapinfo"

namespace ZMLegacyObjPanel
{
    const char* GetString();
    bool ObjectivesExist();
    bool LoadObjectivesFromFile( const char* mapname, char* buffer, int len );
    bool SaveToStringTable( const char* mapname );
    void ResetObjectives();

    void ShowPanel();
};
