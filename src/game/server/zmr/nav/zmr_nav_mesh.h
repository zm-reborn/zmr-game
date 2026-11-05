#pragma once


#include "nav_mesh.h"


#define NAV_MESH_ZMR_NOFLOOR    0x00040000


class CZMNavTransientFilter : public CTraceFilterSimple
{
public:
    CZMNavTransientFilter();

    virtual bool ShouldHitEntity( IHandleEntity* pHandleEntity, int contentsMask ) override;
};

class CZMRNavMesh : public CNavMesh
{
public:
    typedef CNavMesh BaseClass;
    typedef CZMRNavMesh ThisClass;
    //DECLARE_CLASS( CZMRNavMesh, CNavMesh );

    CZMRNavMesh();
    ~CZMRNavMesh();

    virtual void Update() override;

    virtual CNavArea* CreateArea() const override;


    virtual void OnServerActivate() override;


    virtual void FireGameEvent( IGameEvent* pEvent ) override;


    static float GetTransientCheckStartHeight() { return 17.0f; }

    void UpdateTransientAreas();
    void UpdateFloorCheckAreas();

private:
    CountdownTimer m_UpdateTransientTimer;
    CountdownTimer m_UpdateCheckFloorTimer;
};
