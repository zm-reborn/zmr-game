#pragma once

#include "nav_mesh.h"

class CZMRNavArea : public CNavArea
{
public:
    typedef CNavArea BaseClass;
    typedef CZMRNavArea ThisClass;
    //DECLARE_CLASS( CZMRNavArea, CNavArea );

    CZMRNavArea();
    ~CZMRNavArea();

    // Override to make these do nothing.
    virtual void ComputeHidingSpots() OVERRIDE;
    virtual void ComputeSniperSpots() OVERRIDE;
    virtual void ComputeSpotEncounters() OVERRIDE;	
};
