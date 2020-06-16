#pragma once


#include "nav_mesh.h"


#define NAV_MESH_ZMR_NOFLOOR    0x00040000


class CZMNavTransientFilter : public CTraceFilterSimple
{
public:
    CZMNavTransientFilter();

    virtual bool ShouldHitEntity( IHandleEntity* pHandleEntity, int contentsMask ) OVERRIDE;
};

class CZMRNavMesh : public CNavMesh
{
public:
    typedef CNavMesh BaseClass;
    typedef CZMRNavMesh ThisClass;
    //DECLARE_CLASS( CZMRNavMesh, CNavMesh );

    CZMRNavMesh();
    ~CZMRNavMesh();

    virtual void Update() OVERRIDE;

    virtual CNavArea* CreateArea() const OVERRIDE;


    virtual void OnServerActivate() OVERRIDE;


    virtual void FireGameEvent( IGameEvent* pEvent ) OVERRIDE;


    static float GetTransientCheckStartHeight() { return 17.0f; }

    void UpdateTransientAreas();
    void UpdateFloorCheckAreas();

    static void GetAreaBounds( const CNavArea* pArea, Vector& mins, Vector& maxs );

private:
    CountdownTimer m_UpdateTransientTimer;
    CountdownTimer m_UpdateCheckFloorTimer;
};
