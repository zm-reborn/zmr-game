#pragma once

#include "c_playerresource.h"


class C_ZMPlayerResource : public C_PlayerResource
{
public:
    DECLARE_CLASS( C_ZMPlayerResource, C_PlayerResource );
    DECLARE_CLIENTCLASS();


    C_ZMPlayerResource();
};