#include "cbase.h"

#include "filesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//
// Increment me to tell clients to remove their pre-existing sound cache.
//
#define SNDCACHE_CURVERSION         1

#define SNDCACHE_FILE               "sound/sound.cache"

#define SNDCACHE_VERSIONFILE        "sound/soundcacheversion.txt"


class CZMClearSoundCache : public CAutoGameSystem
{
public:
    CZMClearSoundCache();
    ~CZMClearSoundCache();

    virtual void PostInit() OVERRIDE;

    static void CheckCache();
    static void RemoveCacheFile();
};


//
//
//
CZMClearSoundCache::CZMClearSoundCache() : CAutoGameSystem( "ZMClearSoundCache" )
{
}

CZMClearSoundCache::~CZMClearSoundCache()
{
}

void CZMClearSoundCache::PostInit()
{
    CheckCache();
}

void CZMClearSoundCache::CheckCache()
{
    bool clear = true;

    CUtlBuffer buf( 0, 0, CUtlBuffer::TEXT_BUFFER );

    if ( filesystem->ReadFile( SNDCACHE_VERSIONFILE, "MOD", buf ) )
    {
        int ver = buf.GetInt();

        if ( ver == SNDCACHE_CURVERSION )
            clear = false;
    }

    if ( clear )
    {
        RemoveCacheFile();


        buf.Clear();

        char buffer[32];
        Q_snprintf( buffer, sizeof( buffer ), "%i", SNDCACHE_CURVERSION );

        buf.PutString( buffer );

        filesystem->WriteFile( SNDCACHE_VERSIONFILE, "MOD", buf );


        Msg( "Saved sound cache version %i! (%s)\n", SNDCACHE_CURVERSION, SNDCACHE_VERSIONFILE );


        buf.Clear();
    }
}

void CZMClearSoundCache::RemoveCacheFile()
{
    if ( filesystem->FileExists( SNDCACHE_FILE, "MOD" ) )
    {
        Msg( "Removing sound cache file (%s) because sounds were changed!\n", SNDCACHE_FILE );

        filesystem->RemoveFile( SNDCACHE_FILE, "MOD" );
    }
}


static CZMClearSoundCache* g_pZMClearSoundCache = new CZMClearSoundCache();
