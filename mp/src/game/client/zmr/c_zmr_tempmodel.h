#pragma once

#include "c_baseanimating.h"


class C_ZMTempModel : public C_BaseAnimating
{
public:
    DECLARE_CLASS( C_ZMTempModel, C_BaseAnimating )


    static C_ZMTempModel* Create( const char* pszModel );
    

    virtual bool Initialize( const char* pszModel );
};
