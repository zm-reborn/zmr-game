#include "cbase.h"


#include "zmr_ammodef.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define BULLET_MASS_GRAINS_TO_LB(grains)	(0.002285*(grains)/16.0f)
#define BULLET_MASS_GRAINS_TO_KG(grains)	lbs2kg(BULLET_MASS_GRAINS_TO_LB(grains))

// exaggerate all of the forces, but use real numbers to keep them consistent
#define BULLET_IMPULSE_EXAGGERATION			3.5
// convert a velocity in ft/sec and a mass in grains to an impulse in kg in/s
#define BULLET_IMPULSE(grains, ftpersec)	((ftpersec)*12*BULLET_MASS_GRAINS_TO_KG(grains)*BULLET_IMPULSE_EXAGGERATION)


#define ADD_ZM_AMMOTYPE(def,ammotype,itemname,dropamount,dmg,tracer,maxcarry,force) \
    i = def.m_nAmmoIndex; \
    def.AddAmmoType(ammotype,dmg,tracer,0,0,maxcarry,force,0); \
    def.SetAdditional(i,itemname,dropamount);

void CZMAmmoDef::SetAdditional( int ammoindex, const char* itemname, int dropamount )
{
    m_Additional[ammoindex].pszItemName = itemname;
    m_Additional[ammoindex].nDropAmount = dropamount;
}

CZMAmmoDef* ZMAmmoDef()
{
    return static_cast<CZMAmmoDef*>( GetAmmoDef() );
}



CAmmoDef* GetAmmoDef()
{
    static CZMAmmoDef def;
    static bool bInitted = false;
    
    if ( !bInitted )
    {
        bInitted = true;


        int i;

        // Stuff we use.
        ADD_ZM_AMMOTYPE( def, "Pistol", "item_ammo_pistol", 20, DMG_BULLET, TRACER_LINE_AND_WHIZ, 80, 2400 );
        ADD_ZM_AMMOTYPE( def, "357", "item_ammo_357", 11, DMG_BULLET, TRACER_LINE_AND_WHIZ, 22, 3800 ); // Rifle
        ADD_ZM_AMMOTYPE( def, "Buckshot", "item_box_buckshot", 8, DMG_BULLET | DMG_BUCKSHOT, TRACER_LINE_AND_WHIZ, 24, 1300 ); // Shotgun
        ADD_ZM_AMMOTYPE( def, "SMG1", "item_ammo_smg1", 30, DMG_BULLET, TRACER_LINE_AND_WHIZ, 60, 2200 ); // Mac-10
        
        // ZM Custom
        ADD_ZM_AMMOTYPE( def, "Molotov", "", 0, DMG_BURN, TRACER_NONE, 1, 0 );
        ADD_ZM_AMMOTYPE( def, "Revolver", "item_ammo_revolver", 6, DMG_BULLET, TRACER_LINE_AND_WHIZ, 36, 3200 );


        // This is used by func_tank. (eg. zm_desert_laboratory)
        ADD_ZM_AMMOTYPE( def, "AR2", "", 0, DMG_BULLET, TRACER_LINE_AND_WHIZ, 60, BULLET_IMPULSE( 200, 1225 ) );

        // ZMRTODO: Remove these when removing the HL2DM stuff.
        /*
        ADD_ZM_AMMOTYPE( def, "AR2AltFire", DMG_DISSOLVE, TRACER_NONE, 3, 0 );
        ADD_ZM_AMMOTYPE( def, "XBowBolt", DMG_BULLET, TRACER_LINE, 10, BULLET_IMPULSE( 800, 8000 ) );
        ADD_ZM_AMMOTYPE( def, "RPG_Round", DMG_BURN, TRACER_NONE, 3, 0 );
        ADD_ZM_AMMOTYPE( def, "SMG1_Grenade", DMG_BURN, TRACER_NONE, 3, 0 );
        ADD_ZM_AMMOTYPE( def, "Grenade", DMG_BURN, TRACER_NONE, 5, 0 );
        ADD_ZM_AMMOTYPE( def, "slam", DMG_BURN, TRACER_NONE, 5, 0 );
        */
    }

    return &def;
}
