#pragma once

void TE_ZMFireBullets( 
	int	iPlayerIndex,
	const Vector &vOrigin,
	const Vector &vDir,
	int	iAmmoID,
	int iSeed,
	int iShots,
	float flSpread, 
	bool bDoTracers,
	bool bDoImpacts );
