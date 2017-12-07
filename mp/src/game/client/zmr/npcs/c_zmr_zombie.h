#pragma once

#include "c_zmr_zombiebase.h"


class C_ZMHolidayHat : public C_BaseAnimating
{
public:
    DECLARE_CLASS( C_ZMHolidayHat, C_BaseAnimating )

    
    virtual bool Initialize( C_BaseEntity* pOwner, const char* pszModel );
    virtual bool Parent( const char* pszAttachment );
};

class C_ZMZombie : public C_ZMBaseZombie
{
public:
    DECLARE_CLASS( C_ZMZombie, C_ZMBaseZombie )
	DECLARE_CLIENTCLASS()


    C_ZMZombie();
    ~C_ZMZombie();


    virtual CStudioHdr* OnNewModel() OVERRIDE;
    virtual C_BaseAnimating* BecomeRagdollOnClient() OVERRIDE;
    virtual void UpdateVisibility() OVERRIDE;

    LocalFlexController_t GetFlexControllerNumByName( const char* pszName );
    void SetFlexWeightSafe( const char* pszName, float value );

protected:
    virtual void    CreateHat();
    void            ReleaseHat();
    void            MakeHappy();

    C_ZMHolidayHat* m_pHat;
};
