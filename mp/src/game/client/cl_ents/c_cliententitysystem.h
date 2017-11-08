#pragma once


//
enum EntNetworkStatus_t
{
    ENS_CLIENTSIDE = 0,
    ENS_SHARED,
};

class C_BaseClientEnt;
typedef C_BaseClientEnt* (*CLIENTENTCREATENEW)();

struct ClientEntRegister_t
{
    const char* classname;
    CLIENTENTCREATENEW pfn;
};
//



//
abstract_class C_BaseClientEnt
{
public:
    C_BaseClientEnt();
    virtual ~C_BaseClientEnt();


    virtual bool            Initialize() = 0;
    virtual void            ParseMapData( CEntityMapData* mapData ) = 0;
    virtual void            Release() = 0;

    bool                    NeedsServer() const { return m_iNetworkStatus != ENS_CLIENTSIDE; }
    void                    SetNeedsServer() { m_iNetworkStatus = ENS_SHARED; }

private:
    EntNetworkStatus_t      m_iNetworkStatus;
};
//



//
class CClientEntitySystem : CAutoGameSystem
{
public:
	CClientEntitySystem();
	~CClientEntitySystem();


    // Recreate all clientside ents in map
    void            RecreateAll();

    // Clear all clientside created ents
    void            DestroyAll();

    // If creating new client-side only entities, use this.
    void            AddClientSideEntityListener( const char* pszClassname, CLIENTENTCREATENEW fn );


	virtual void    PostInit() OVERRIDE;

	void            ParseAllEntities( const char* pMapData );
	const char*     ParseEntity( const char* pEntData );
private:

    CUtlVector<ClientEntRegister_t> m_vClientEntRegister;
    CUtlVector<C_BaseClientEnt*> m_vClientEnts;


    friend class C_BaseClientEnt;
};

extern CClientEntitySystem g_ClientEntitySystem;
//



//
void RegisterClientEnts();

#define REGISTER_CLIENTENT( classname, fn )    g_ClientEntitySystem.AddClientSideEntityListener( #classname, fn )
