#pragma once


#include "player_resource.h"


class CZMPlayerResource : public CPlayerResource
{
public:
    DECLARE_CLASS( CZMPlayerResource, CPlayerResource );
    DECLARE_SERVERCLASS();
    DECLARE_DATADESC();
};