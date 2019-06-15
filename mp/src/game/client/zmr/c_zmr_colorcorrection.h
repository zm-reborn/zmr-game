#pragma once


#include "cbase.h"
#include "colorcorrectionmgr.h"


//
class CZMBaseCCEffect
{
public:
    CZMBaseCCEffect( ClientCCHandle_t hndl = INVALID_CLIENT_CCHANDLE );
    virtual ~CZMBaseCCEffect();

    ClientCCHandle_t    GetHandle() const;

    virtual float       GetWeight() = 0;
    virtual bool        IsDone() = 0;

    //virtual bool        OnChangeTeam();
    //virtual bool        OnDeath();


protected:
    ClientCCHandle_t m_Hndl;
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

private:
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


    //void AddEffect( CZMBaseCCEffect* pEffect );
    void RemoveEffect( CZMBaseCCEffect* pEffect );



    void OnDeath();
    void OnTeamChange( int iTeam );



    void ReleaseCCEnt();

    bool IsReady() const;

private:
    bool CheckCC();
    void InitCC();
    void InitEnt();

    

    CUtlVector<CZMBaseCCEffect*> m_vEffects;


    C_ZMEntColorCorrection* m_pCCEnt;


    friend class C_ZMEntColorCorrection;
};
//


extern CZMColorCorrectionSystem g_ZMColorCorrection;
