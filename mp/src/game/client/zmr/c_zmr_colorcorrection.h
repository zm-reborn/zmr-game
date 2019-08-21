#pragma once


#include "cbase.h"
#include "colorcorrectionmgr.h"


//
class CZMBaseCCEffect
{
public:
    CZMBaseCCEffect( const char* name, const char* filename );
    virtual ~CZMBaseCCEffect();


    const char*         GetName() const;
    const char*         GetFilename() const;
    ClientCCHandle_t    GetHandle() const;

    void                SetHandle( ClientCCHandle_t hndl );

    bool                HasHandle() const;


    virtual float       GetWeight() const = 0;
    virtual bool        IsDone() const = 0;

    virtual bool        OnTeamChange( int iTeam ) { return false; }
    virtual bool        OnDeath() { return false; }


private:
    ClientCCHandle_t m_Hndl;

    const char* m_pszName;
    const char* m_pszFilename;
};
//


// Unfortunately, we have to create a client entity to control 
class C_ZMEntColorCorrection : public C_BaseEntity
{
public:
    DECLARE_CLASS( C_ZMEntColorCorrection, C_BaseEntity );

    C_ZMEntColorCorrection();
    ~C_ZMEntColorCorrection();


    virtual void ClientThink() OVERRIDE;


    void AddCC( CZMBaseCCEffect* cc );
    void ClearEffects();


    CUtlVector<CZMBaseCCEffect*> m_vCCs;
};
//



//
class CZMColorCorrectionSystem : public CAutoGameSystem, public CGameEventListener
{
public:
    //DECLARE_CLASS( CZMColorCorrectionSystem, CAutoGameSystemPerFrame );

    CZMColorCorrectionSystem();
    virtual ~CZMColorCorrectionSystem();


    virtual bool Init() OVERRIDE;
    virtual void FireGameEvent( IGameEvent* event ) OVERRIDE;


    void AddEffect( CZMBaseCCEffect* pEffect );
    bool RemoveEffect( CZMBaseCCEffect* pEffect );

    void OnDeath();
    void OnTeamChange( int iTeam );



    void ReleaseCCEnt();

private:
    bool CheckCC();
    void InitEnt();
    void InitEffects();

    

    CUtlVector<CZMBaseCCEffect*> m_vEffects;


    C_ZMEntColorCorrection* m_pCCEnt;


    friend class C_ZMEntColorCorrection;
};
//


extern CZMColorCorrectionSystem* ZMGetCCSystem();
