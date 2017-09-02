#include "cbase.h"
#include "ammodef.h"

/*
    NOTE: Remove GetAmmoDef() from hl2mp/hl2mp_gamerules.cpp
*/

#define BULLET_MASS_GRAINS_TO_LB(grains)	(0.002285*(grains)/16.0f)
#define BULLET_MASS_GRAINS_TO_KG(grains)	lbs2kg(BULLET_MASS_GRAINS_TO_LB(grains))

// exaggerate all of the forces, but use real numbers to keep them consistent
#define BULLET_IMPULSE_EXAGGERATION			3.5
// convert a velocity in ft/sec and a mass in grains to an impulse in kg in/s
#define BULLET_IMPULSE(grains, ftpersec)	((ftpersec)*12*BULLET_MASS_GRAINS_TO_KG(grains)*BULLET_IMPULSE_EXAGGERATION)


#define ADD_ZM_AMMOTYPE(def,ammotype,dmg,tracer,maxcarry,force) def.AddAmmoType(ammotype,dmg,tracer,0,0,maxcarry,force,0);

CAmmoDef* GetAmmoDef()
{
    static CAmmoDef def;
    static bool bInitted = false;
    
    if ( !bInitted )
    {
        bInitted = true;


        // Stuff we use.
        ADD_ZM_AMMOTYPE( def, "Pistol", DMG_BULLET, TRACER_LINE_AND_WHIZ, 80, BULLET_IMPULSE( 200, 1225 ) );
        ADD_ZM_AMMOTYPE( def, "357", DMG_BULLET, TRACER_LINE_AND_WHIZ, 21, BULLET_IMPULSE( 800, 5000 ) ); // Rifle
        ADD_ZM_AMMOTYPE( def, "Buckshot", DMG_BULLET | DMG_BUCKSHOT, TRACER_LINE_AND_WHIZ, 24, BULLET_IMPULSE( 400, 1200 ) ); // Shotgun
        ADD_ZM_AMMOTYPE( def, "SMG1", DMG_BULLET, TRACER_LINE_AND_WHIZ, 60, BULLET_IMPULSE( 200, 1225 ) ); // Mac-10
        
        // ZM Custom
        ADD_ZM_AMMOTYPE( def, "Molotov", DMG_BURN, TRACER_NONE, 1, 0 );
        ADD_ZM_AMMOTYPE( def, "Revolver", DMG_BULLET, TRACER_LINE_AND_WHIZ, 36, BULLET_IMPULSE( 800, 2000 ) );



        // ZMRTODO: Remove these when removing the HL2DM stuff.
        /*
        ADD_ZM_AMMOTYPE( def, "AR2", DMG_BULLET, TRACER_LINE_AND_WHIZ, 60, BULLET_IMPULSE( 200, 1225 ) );
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
