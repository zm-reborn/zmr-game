#pragma once


#include "nav_mesh.h"

class CZMRNavMesh : public CNavMesh
{
public:
    typedef CNavMesh BaseClass;
    typedef CZMRNavMesh ThisClass;
    //DECLARE_CLASS( CZMRNavMesh, CNavMesh );

    CZMRNavMesh();
    ~CZMRNavMesh();

    virtual CNavArea* CreateArea() const OVERRIDE;


    virtual void OnServerActivate() OVERRIDE;


    virtual void FireGameEvent( IGameEvent* pEvent ) OVERRIDE;
};
