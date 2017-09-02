#include "team.h"


class CZMTeam : public CTeam
{
public:
    DECLARE_CLASS( CZMTeam, CTeam );
    //DECLARE_SERVERCLASS();

    virtual bool ShouldTransmitToPlayer( CBasePlayer* pRecipient, CBaseEntity* pEntity ) OVERRIDE;
};
