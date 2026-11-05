#pragma once

#include "Sprite.h"

#include "c_cliententitysystem.h"


class C_ClientSprite : public C_Sprite, public C_BaseClientEnt
{
public:
    DECLARE_CLASS( C_ClientSprite, C_Sprite );


    C_ClientSprite();
    ~C_ClientSprite();

    virtual bool            IsDormant() { return false; };

    virtual bool            KeyValue( const char* szKeyName, const char* szValue ) override;

    virtual bool            Initialize() override;
    virtual void            ParseMapData( CEntityMapData* mapData ) override { C_Sprite::ParseMapData( mapData ); };
    virtual void            Release() override { C_Sprite::Release(); };
    

    static C_BaseClientEnt* CreateNew();
};
