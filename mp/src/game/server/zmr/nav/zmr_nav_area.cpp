#include "cbase.h"

#include "zmr_nav_area.h"


CZMRNavArea::CZMRNavArea()
{
    m_bHasNoFloor = false;
}

CZMRNavArea::~CZMRNavArea()
{
}

void CZMRNavArea::ComputeHidingSpots()
{
}

void CZMRNavArea::ComputeSniperSpots()
{
}

void CZMRNavArea::ComputeSpotEncounters()
{
}

bool CZMRNavArea::IsBlocked( int teamID, bool ignoreNavBlockers ) const
{
    return BaseClass::IsBlocked( teamID, ignoreNavBlockers ) || m_bHasNoFloor;
}
