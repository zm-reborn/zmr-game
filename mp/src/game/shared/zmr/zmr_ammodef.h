#include "ammodef.h"

struct ZMAmmo_t
{
    const char* pszItemName;
    int nDropAmount;
};

class CZMAmmoDef : public CAmmoDef
{
public:
    void SetAdditional( int ammoindex, const char* itemname, int dropamount );

    ZMAmmo_t m_Additional[MAX_AMMO_TYPES];
};

extern CZMAmmoDef* ZMAmmoDef();
