#pragma once


#include "zmr/zmr_usercmd.h"


// Leaving empty classes for client.


#ifdef GAME_DLL
class CZMBaseWeapon;
struct ZMUserCmdValidData_t
{
    ZMUserCmdValidData_t( CZMPlayer* pAttacker, CZMBaseWeapon* pWeapon, Vector vecSrc )
    {
        this->pAttacker = pAttacker;
        this->pVictim = nullptr;
        this->pWeapon = pWeapon;
        this->nHits = 0;
        this->nAlreadyHit = 0;
        this->nEntitiesAlreadyHit = 0;
        this->vecSrc = vecSrc;
    }


    CZMPlayer*      pAttacker;
    CBaseEntity*    pVictim;
    CZMBaseWeapon*  pWeapon;
    int     nHits;
    int     nAlreadyHit;
    int     nEntitiesAlreadyHit;
    Vector  vecSrc;
};
#endif

abstract_class CZMUserCmdHitValidator
{
public:
#ifdef GAME_DLL
    CZMUserCmdHitValidator();

    virtual bool IsUserCmdHitsValid( ZMUserCmdValidData_t& data ) { return true; }


    const char* GetUserCmdError() const { return m_pszUserCmdError; }

protected:
    bool OnUserCmdError( const char* error ) { m_pszUserCmdError = error; return false; }

private:
    const char* m_pszUserCmdError;
#endif
};

abstract_class CZMUserCmdHitWepValidator : public CZMUserCmdHitValidator
{
public:
#ifdef GAME_DLL
    virtual bool IsUserCmdHitsValid( ZMUserCmdValidData_t& data ) OVERRIDE;

    virtual float GetMaxDamageDist( ZMUserCmdValidData_t& data ) const { return 1024.0f; }

    virtual int GetMaxUserCmdBullets( ZMUserCmdValidData_t& data ) const { return 1; }
    virtual int GetMaxNumPenetrate( ZMUserCmdValidData_t& data ) const { return 0; }
#endif
};
