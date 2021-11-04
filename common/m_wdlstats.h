// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2020 by The Odamex Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//     Defines needed by the WDL stats logger.
//
//-----------------------------------------------------------------------------

#ifndef __WDLSTATS_H__
#define __WDLSTATS_H__

#include "d_player.h"

enum WDLEvents {
	WDL_EVENT_DAMAGE,
	WDL_EVENT_CARRIERDAMAGE,
	WDL_EVENT_KILL,
	WDL_EVENT_CARRIERKILL,
	WDL_EVENT_ENVIRODAMAGE,
	WDL_EVENT_ENVIROCARRIERDAMAGE,
	WDL_EVENT_ENVIROKILL,
	WDL_EVENT_ENVIROCARRIERKILL,
	WDL_EVENT_TOUCH,
	WDL_EVENT_PICKUPTOUCH,
	WDL_EVENT_CAPTURE,
	WDL_EVENT_PICKUPCAPTURE,
	WDL_EVENT_ASSIST,
	WDL_EVENT_RETURNFLAG,
	WDL_EVENT_PICKUPITEM,
	WDL_EVENT_SPREADACCURACY,
	WDL_EVENT_SSACCURACY,
	WDL_EVENT_TRACERACCURACY,
	WDL_EVENT_PROJACCURACY,
	WDL_EVENT_SPAWNPLAYER,
	WDL_EVENT_SPAWNITEM,
	WDL_EVENT_JOINGAME,
	WDL_EVENT_DISCONNECT,
	WDL_EVENT_PLAYERBEACON,
	WDL_EVENT_PROJFIRE,
	WDL_EVENT_TELEPORTPLAYER,
	//WDL_EVENT_RJUMPGO,
	//WDL_EVENT_RJUMPLAND,
	//WDL_EVENT_RJUMPAPEX,
	//WDL_EVENT_SPAWNMOB,
	//WDL_EVENT_MOBBEACON,
	//WDL_EVENT_TRACERBEACON,
	//WDL_EVENT_TELEPORTMOB,
	//WDL_EVENT_MOBSHOOT,
	//WDL_EVENT_ARCHFIRE,
	//WDL_EVENT_MOBPROJ,
	//WDL_EVENT_TELEPORTMOB,
	//WDL_EVENT_EXITLEVEL,
};

enum WDLPowerups {
	WDL_PICKUP_SOULSPHERE,
	WDL_PICKUP_MEGASPHERE,
	WDL_PICKUP_BLUEARMOR,
	WDL_PICKUP_GREENARMOR,
	WDL_PICKUP_BERSERK,
	WDL_PICKUP_STIMPACK,
	WDL_PICKUP_MEDKIT,
	WDL_PICKUP_HEALTHBONUS,
	WDL_PICKUP_ARMORBONUS,
	WDL_PICKUP_YELLOWKEY,
	WDL_PICKUP_REDKEY,
	WDL_PICKUP_BLUEKEY,
	WDL_PICKUP_YELLOWSKULL,
	WDL_PICKUP_REDSKULL,
	WDL_PICKUP_BLUESKULL,
	WDL_PICKUP_INVULNSPHERE,
	WDL_PICKUP_INVISSPHERE,
	WDL_PICKUP_RADSUIT,
	WDL_PICKUP_COMPUTERMAP,
	WDL_PICKUP_GOGGLES,
	WDL_PICKUP_CLIP,
	WDL_PICKUP_AMMOBOX,
	WDL_PICKUP_ROCKET,
	WDL_PICKUP_ROCKETBOX,
	WDL_PICKUP_CELL,
	WDL_PICKUP_CELLPACK,
	WDL_PICKUP_SHELLS,
	WDL_PICKUP_SHELLBOX,
	WDL_PICKUP_BACKPACK,
	WDL_PICKUP_BFG,
	WDL_PICKUP_CHAINGUN,
	WDL_PICKUP_CHAINSAW,
	WDL_PICKUP_ROCKETLAUNCHER,
	WDL_PICKUP_PLASMAGUN,
	WDL_PICKUP_SHOTGUN,
	WDL_PICKUP_SUPERSHOTGUN,
	WDL_PICKUP_UNKNOWN,
};

/**
 * Weapons used by the Accuracy event.
 * 
 * There's probably an equivalent enum for these elsewhere.
 */
enum WDLWeapons {
	WDL_WEAPON_FIST,
	WDL_WEAPON_PISTOL,
	WDL_WEAPON_SHOTGUN,
	WDL_WEAPON_CHAINGUN,
	WDL_WEAPON_MISSLE,
	WDL_WEAPON_PLASMA,
	WDL_WEAPON_BFG,
	WDL_WEAPON_CHAINSAW,
	WDL_WEAPON_SSG,
};

void M_StartWDLLog();
void M_LogWDLEvent(
	WDLEvents event, player_t* activator, player_t* target,
	int arg0, int arg1, int arg2, int arg3
);
void M_LogActorWDLEvent(
	WDLEvents event, AActor* activator, AActor* target,
	int arg0, int arg1, int arg2
);
void M_LogWDLPlayerSpawn(mapthing2_t *mthing);
void M_LogWDLItemRespawnEvent(AActor* activator);
void M_LogWDLFlagLocation(mapthing2_t* activator, team_t team);
void M_LogWDLPickupEvent(player_t* activator, AActor* target, WDLPowerups pickuptype);
int M_GetPlayerSpawn(int x, int y, int z, team_t team);
void M_LogWDLAccuracyShot(
	WDLEvents event, player_t* activator, int mod, 
	angle_t angle
);
void M_LogWDLAccuracyHit(
	WDLEvents event, player_t* activator, player_t* target, 
	int mod, int hits
);
void M_HandleWDLNameChange(team_t team, std::string oldname, std::string newname);
weapontype_t M_MODToWeapon(int mod);
int GetMaxShotsForMod(int mod);
void M_CommitWDLLog();

#endif
