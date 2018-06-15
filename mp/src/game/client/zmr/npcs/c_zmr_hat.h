#pragma once

#include "c_baseanimating.h"


class C_ZMHolidayHat : public C_BaseAnimating
{
public:
    DECLARE_CLASS( C_ZMHolidayHat, C_BaseAnimating )

    
    virtual bool Initialize( C_BaseEntity* pOwner, const char* pszModel );
    virtual void AttachToEntity( C_BaseEntity* pOwner );

    //virtual C_BaseAnimating* FindFollowedEntity() OVERRIDE;
};
