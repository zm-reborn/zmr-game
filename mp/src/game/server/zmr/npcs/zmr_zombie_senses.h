#pragma once

#include "npcr/npcr_senses.h"


class CZMZombieSenses : public NPCR::CBaseSenses
{
public:
    typedef NPCR::CBaseSenses BaseClass;
    typedef CZMZombieSenses ThisClass;

    CZMZombieSenses( NPCR::CBaseNPC* pNPC );
    ~CZMZombieSenses();


    virtual void OnDamaged( const CTakeDamageInfo& info ) OVERRIDE;
};
