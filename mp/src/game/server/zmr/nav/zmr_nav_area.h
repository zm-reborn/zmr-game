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
    virtual void UpdateBlocked( bool force = false, int teamID = TEAM_ANY ) OVERRIDE;

    virtual bool IsBlocked( int teamID, bool ignoreNavBlockers = false ) const OVERRIDE;
    void SetNoFloor( bool state ) { m_bHasNoFloor = state; }

private:
    bool m_bHasNoFloor;
};
