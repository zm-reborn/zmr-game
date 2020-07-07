#pragma once


#include "zmr_shareddefs.h"
#include "zmr_imagerow.h"

//
class CZMImageRowItemSpawn : public CZMImageRowItem
{
public:
    DECLARE_CLASS_SIMPLE( CZMImageRowItemSpawn, CZMImageRowItem );

    CZMImageRowItemSpawn( vgui::Panel* pParent, const char* name );
    ~CZMImageRowItemSpawn();


    virtual void ApplySettings( KeyValues* kv ) OVERRIDE;
    virtual void Paint() OVERRIDE;
    virtual void PaintBackground() OVERRIDE;


    // HACK:
    bool IsOldMenu() const { return m_FillColor[3] != 0; }


    void UpdateData( int count, ZombieClass_t type );
    void UpdateSpawnTime();
    bool CanSpawn() const;
    void SetPrimary() { m_bIsPrimary = true; }

    void SetCost( int pCost ) { m_iCost = pCost; }

    void ResetMe();

private:
    float m_flStartTime;
    int m_nCount;
    int m_iCost;
    ZombieClass_t m_iZombieClass;
    bool m_bIsPrimary;
    float m_flDelay;


    // Old stuff
    Color m_FillColor;
};
//