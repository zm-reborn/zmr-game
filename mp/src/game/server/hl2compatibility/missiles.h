#pragma once


#include "Sprite.h"
#include "beam_shared.h"

class CLaserDot;
class RocketTrail;

//###########################################################################
//	>> CMissile		(missile launcher class is below this one!)
//###########################################################################
class CMissile : public CBaseCombatCharacter
{
	DECLARE_CLASS( CMissile, CBaseCombatCharacter );

public:
	static const int EXPLOSION_RADIUS = 200;

	CMissile();
	~CMissile();

#ifdef HL1_DLL
	Class_T Classify( void ) { return CLASS_NONE; }
#else
	Class_T Classify( void ) { return CLASS_MISSILE; }
#endif
	
	void	Spawn( void );
	void	Precache( void );
	void	MissileTouch( CBaseEntity *pOther );
	void	Explode( void );
	void	ShotDown( void );
	void	AccelerateThink( void );
	void	AugerThink( void );
	void	IgniteThink( void );
	void	SeekThink( void );
	void	DumbFire( void );
	void	SetGracePeriod( float flGracePeriod );

	int		OnTakeDamage_Alive( const CTakeDamageInfo &info );
	void	Event_Killed( const CTakeDamageInfo &info );
	
	virtual float	GetDamage() { return m_flDamage; }
	virtual void	SetDamage(float flDamage) { m_flDamage = flDamage; }

	unsigned int PhysicsSolidMaskForEntity( void ) const;

	static CMissile *Create( const Vector &vecOrigin, const QAngle &vecAngles, edict_t *pentOwner );

	void CreateDangerSounds( bool bState ){ m_bCreateDangerSounds = bState; }

	static void AddCustomDetonator( CBaseEntity *pEntity, float radius, float height = -1 );
	static void RemoveCustomDetonator( CBaseEntity *pEntity );

protected:
	virtual void DoExplosion();	
	virtual void ComputeActualDotPosition( CLaserDot *pLaserDot, Vector *pActualDotPosition, float *pHomingSpeed );
	virtual int AugerHealth() { return m_iMaxHealth - 20; }

	// Creates the smoke trail
	void CreateSmokeTrail( void );

	// Gets the shooting position 
	void GetShootPosition( CLaserDot *pLaserDot, Vector *pShootPosition );

	CHandle<RocketTrail>	m_hRocketTrail;
	float					m_flAugerTime;		// Amount of time to auger before blowing up anyway
	float					m_flMarkDeadTime;
	float					m_flDamage;

	struct CustomDetonator_t
	{
		EHANDLE hEntity;
		float radiusSq;
		float halfHeight;
	};

	static CUtlVector<CustomDetonator_t> gm_CustomDetonators;

private:
	float					m_flGracePeriodEndsAt;
	bool					m_bCreateDangerSounds;

	DECLARE_DATADESC();
};


//-----------------------------------------------------------------------------
// Laser dot control
//-----------------------------------------------------------------------------
CBaseEntity *CreateLaserDot( const Vector &origin, CBaseEntity *pOwner, bool bVisibleDot );
void SetLaserDotTarget( CBaseEntity *pLaserDot, CBaseEntity *pTarget );
void EnableLaserDot( CBaseEntity *pLaserDot, bool bEnable );


//-----------------------------------------------------------------------------
// Specialized mizzizzile
//-----------------------------------------------------------------------------
class CAPCMissile : public CMissile
{
	DECLARE_CLASS( CMissile, CMissile );
	DECLARE_DATADESC();

public:
	static CAPCMissile *Create( const Vector &vecOrigin, const QAngle &vecAngles, const Vector &vecVelocity, CBaseEntity *pOwner );

	CAPCMissile();
	~CAPCMissile();
	void	IgniteDelay( void );
	void	AugerDelay( float flDelayTime );
	void	ExplodeDelay( float flDelayTime );
	void	DisableGuiding();
#if defined( HL2_DLL )
	virtual Class_T Classify ( void ) { return CLASS_COMBINE; }
#endif

	void	AimAtSpecificTarget( CBaseEntity *pTarget );
	void	SetGuidanceHint( const char *pHintName );

	void	APCSeekThink( void );

	CAPCMissile			*m_pNext;

protected:
	virtual void DoExplosion();	
	virtual void ComputeActualDotPosition( CLaserDot *pLaserDot, Vector *pActualDotPosition, float *pHomingSpeed );
	virtual int AugerHealth();

private:
	void Init();
	void ComputeLeadingPosition( const Vector &vecShootPosition, CBaseEntity *pTarget, Vector *pLeadPosition );
	void BeginSeekThink();
	void AugerStartThink();
	void ExplodeThink();
	void APCMissileTouch( CBaseEntity *pOther );

	float	m_flReachedTargetTime;
	float	m_flIgnitionTime;
	bool	m_bGuidingDisabled;
	float   m_flLastHomingSpeed;
	EHANDLE m_hSpecificTarget;
	string_t m_strHint;
};


//-----------------------------------------------------------------------------
// Finds apc missiles in cone
//-----------------------------------------------------------------------------
CAPCMissile *FindAPCMissileInCone( const Vector &vecOrigin, const Vector &vecDirection, float flAngle );

