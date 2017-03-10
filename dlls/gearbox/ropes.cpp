/***
*
*	Copyright (c) 2001, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "squadmonster.h"
#include "player.h"
#include "weapons.h"
#include "decals.h"
#include "gamerules.h"
#include "effects.h"
#include "saverestore.h"

#define clamp( val, min, max ) ( ((val) > (max)) ? (max) : ( ((val) < (min)) ? (min) : (val) ) )

class CRope : public CBaseToggle
{
public:
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );
	void Precache ( void );
	void Initialize ( void );

	int m_iSegments;
	BOOL m_fDisabled;
	Vector vecAngles;

	string_t m_EndingModel;
	string_t m_BodyModel;
};
//=======================
// Opposing Force Rope Segment
//=======================
class CRopeSample : public CBaseToggle
{
public:
	void Spawn ( void );
	void Precache ( void );
	void GetAttachment ( int iAttachment, Vector &origin, Vector &angles );
	void CreateSample ( void );
	void Initialize ( void );
	void EXPORT RopeTouch ( CBaseEntity *pOther );
	void EXPORT RopeThink ( void );

	int m_iSegments;
	BOOL m_fFirstSegment;
	BOOL m_fDisabled;
	BOOL m_fElectrified;
	string_t m_Model;
	string_t m_EndingModel;

public:
	static CRopeSample *CreateSample( Vector vecOrigin, Vector vecAngles, string_t m_BodyModel, string_t m_EndingModel, BOOL m_fDisabled, int m_iSegments, BOOL m_fElectrified, CBaseEntity *pOwner );
};

LINK_ENTITY_TO_CLASS( env_rope, CRope );
void CRope :: Spawn ( void )
{
	pev->effects |= EF_NODRAW;
	Precache();
	Initialize();
}
void CRope :: Precache ( void )
{
	//PRECACHE_MODEL( (char *)STRING(m_BodyModel) );
	//PRECACHE_MODEL( (char *)STRING(m_EndingModel) );
	PRECACHE_MODEL( "models/wire_blue8.mdl" );
	PRECACHE_MODEL( "models/wire_copper16.mdl" );
}
void CRope :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "segments"))
	{
		m_iSegments = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	if (FStrEq(pkvd->szKeyName, "bodymodel"))
	{
		m_BodyModel = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "endingmodel"))
	{
		m_EndingModel = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "disable"))
	{
		m_fDisabled = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}
void CRope :: Initialize ( void )
{
	vecAngles.z = 270;
	BOOL m_fElectrified = FALSE;
	CRopeSample *pSegment = CRopeSample::CreateSample( pev->origin, vecAngles, m_BodyModel,  m_EndingModel, m_fDisabled, m_iSegments, m_fElectrified, this );
}
//=======================
// Opposing Force Rope Sample
//=======================
CRopeSample *CRopeSample::CreateSample( Vector vecOrigin, Vector vecAngles, string_t m_BodyModel, string_t m_EndingModel, BOOL m_fDisabled, int m_iSegments, BOOL m_fElectrified, CBaseEntity *pOwner )
{
	CRopeSample *pSegment = GetClassPtr( (CRopeSample *)NULL );

	UTIL_SetOrigin( pSegment->pev, vecOrigin );
	pSegment->pev->angles = vecAngles;
	pSegment->pev->owner = pOwner->edict();
	pSegment->m_Model = m_BodyModel;
	pSegment->m_EndingModel = m_EndingModel;
	pSegment->m_iSegments = m_iSegments;
	pSegment->m_fDisabled = m_fDisabled;
	pSegment->m_fElectrified = m_fElectrified;
	pSegment->m_iSegments = m_iSegments;
	pSegment->Spawn();

	return pSegment;
}
void CRopeSample :: Spawn ( void )
{
	Precache();
	Initialize();
	pev->movetype = MOVETYPE_PUSH;
	pev->solid = SOLID_BBOX;
	pev->gravity = 1;

	//if ( m_iSegments == 0 ) 
	//	SET_MODEL(ENT(pev), STRING( m_EndingModel ));
	//else
	//	SET_MODEL(ENT(pev), STRING( m_Model ));

	if ( m_iSegments == 0 ) 
		SET_MODEL(ENT(pev), "models/wire_copper16.mdl");
	else
		SET_MODEL(ENT(pev), "models/wire_blue8.mdl");

	SetTouch ( &CRopeSample::RopeTouch );

	UTIL_SetSize(pev, Vector( 0, 0, 0), Vector(0, 0, 0));
	if (m_iSegments != 0)
	{
		CreateSample();
	}
	else
	{
		SetThink ( &CRopeSample::RopeThink );
		pev->nextthink = gpGlobals->time + 0.1;
	}
}
void CRopeSample :: Initialize ( void )
{
}
void CRopeSample :: RopeTouch ( CBaseEntity *pOther )
{
	Vector vecEnd, vecSrc, vecAngles;
	TraceResult		tr1;

	if ( pOther->IsPlayer() )
	{
		GET_ATTACHMENT( ENT(pev), 0, vecEnd, NULL );
		UTIL_TraceLine( vecSrc, vecEnd, ignore_monsters, ENT(pev), &tr1 );

		pOther->pev->flags = PFLAG_LATCHING;
	}
	else
	{
		GET_ATTACHMENT( ENT(pev), 0, vecEnd, NULL );
		UTIL_TraceLine( vecSrc, vecSrc + vecEnd, ignore_monsters, ENT(pev), &tr1 );
	}
}
void CRopeSample :: RopeThink ( void )
{
	Vector vecSrc, vecAngles;
	entvars_t *pevOwner = VARS( pev->owner );
	if ( pevOwner )
	{
		if ( FClassnameIs( pevOwner, "env_rope" ) )
		{
			pev->origin = pevOwner->origin;
		}
		else
		{
			GET_ATTACHMENT( ENT(pevOwner), 0, vecSrc, NULL );
			pev->origin = vecSrc;

			pev->nextthink = gpGlobals->time + 0.1;
		}
	}
}
void CRopeSample :: Precache ( void )
{
	PRECACHE_SOUND( "items/rope1.wav" );
	PRECACHE_SOUND( "items/rope2.wav" );
	PRECACHE_SOUND( "items/rope3.wav" );

	PRECACHE_MODEL("sprites/xflare1.spr");

	PRECACHE_SOUND( "items/grab_rope.wav" );

	//PRECACHE_MODEL( (char *)STRING(m_Model) );
	//PRECACHE_MODEL( (char *)STRING(m_EndingModel) );

	PRECACHE_MODEL( "models/wire_copper16.mdl" );
	PRECACHE_MODEL( "models/wire_blue8.mdl" );
}
void CRopeSample :: GetAttachment ( int iAttachment, Vector &origin, Vector &angles )
{
	GET_ATTACHMENT( ENT(pev), iAttachment, origin, angles );
}
void CRopeSample :: CreateSample( void )
{
	Vector vecSrc, vecAngles;
	m_iSegments --;

	GetAttachment ( 0, vecSrc, vecAngles );
	vecAngles.z = 270;
	CRopeSample *pSegment = CRopeSample::CreateSample( vecSrc, vecAngles, m_Model, m_EndingModel, m_fDisabled, m_iSegments, m_fElectrified, this );

	SetThink ( &CRopeSample::RopeThink );
	pev->nextthink = gpGlobals->time + 0.1;
}
LINK_ENTITY_TO_CLASS( rope_segment, CRopeSample );
