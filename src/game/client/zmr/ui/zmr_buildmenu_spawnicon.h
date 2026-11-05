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


    virtual void ApplySettings( KeyValues* kv ) override;
    virtual void Paint() override;
    virtual void PaintBackground() override;


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
