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

#include <ctime>
#include <sstream>
#include <chrono>
#include <iomanip>

#include "odamex.h"

#include "m_wdlstats.h"


#include "c_dispatch.h"
#include "g_levelstate.h"
#include "p_local.h"

#define WDLSTATS_VERSION 6

extern Players players;

EXTERN_CVAR(sv_gametype)
EXTERN_CVAR(sv_teamspawns)
EXTERN_CVAR(sv_playerbeacons)

// Strings for WDL events
static const char* wdlevstrings[] = {
	"DAMAGE",
	"CARRIERDAMAGE",
	"KILL",
	"CARRIERKILL",
	"ENVIRODAMAGE",
	"ENVIROCARRIERDAMAGE",
	"ENVIROKILL",
	"ENVIROCARRIERKILL",
	"TOUCH",
	"PICKUPTOUCH",
	"CAPTURE",
	"PICKUPCAPTURE",
	"ASSIST",
	"RETURNFLAG",
	"PICKUPITEM",
	"SPREADACCURACY",
    "SSACCURACY",
	"TRACERACCURACY",
	"PROJACCURACY",
	"SPAWNPLAYER",
    "SPAWNITEM",
	"JOINGAME",
	"DISCONNECT",
	"PLAYERBEACON",
    "CARRIERBEACON",
    "PROJFIRE",
	//"RJUMPGO",
    //"RJUMPLAND",
	//"RJUMPAPEX",
    //"MOBBEACON",
    //"SPAWNMOB",
};

static struct WDLState {
	// Directory to log stats to.
	std::string logdir;

	// True if we're recording stats for this map or restart, otherwise false.
	bool recording;

	// The starting gametic of the most recent log.
	int begintic;

	// [Blair] Toggle for whether that recording has playerbeacons enabled.
	bool enablebeacons;
} wdlstate;

// A single tracked player
struct WDLPlayer
{
	int id;
	std::string netname;
	team_t team;
};

// WDL Players that we're keeping track of.
typedef std::vector<WDLPlayer> WDLPlayers;
static WDLPlayers wdlplayers;

// A single tracked spawn
struct WDLPlayerSpawn
{
	int id;
	int x;
	int y;
	int z;
	team_t team;
};

// WDL spawns that we're keeping track of.
typedef std::vector<WDLPlayerSpawn> WDLPlayerSpawns;
static WDLPlayerSpawns wdlplayerspawns;

// A single tracked item spawn
struct WDLItemSpawn
{
	int id;
	int x;
	int y;
	int z;
	WDLPowerups item;
};

// WDL spawns that we're keeping track of.
typedef std::vector<WDLItemSpawn> WDLItemSpawns;
static WDLItemSpawns wdlitemspawns;

// A single tracked flag pedistool
struct WDLFlagLocation
{
	team_t team;
	int x;
	int y;
	int z;
};

// WDL spawns that we're keeping track of.
typedef std::vector<WDLFlagLocation> WDLFlagLocations;
static WDLFlagLocations wdlflaglocations;

// A single event.
struct WDLEvent
{
	WDLEvents ev;
	int activator;
	int target;
	int gametic;
	fixed_t apos[3];
	fixed_t tpos[3];
	int arg0;
	int arg1;
	int arg2;
	int arg3;
};

// Events that we're keeping track of.
typedef std::vector<WDLEvent> WDLEventLog;
static WDLEventLog wdlevents;

// Turn an event enum into a string.
static const char* WDLEventString(WDLEvents i)
{
	if (i >= ARRAY_LENGTH(::wdlevstrings) || i < 0)
		return "UNKNOWN";
	return ::wdlevstrings[i];
}

static void AddWDLPlayer(const player_t* player)
{
	// Don't add player if their name is already in the vector.
	// [BC] Check the player's team too as version six tracks all connects/disconnects/team switches
	WDLPlayers::const_iterator it = ::wdlplayers.begin();
	for (; it != ::wdlplayers.end(); ++it)
	{
		if ((*it).netname == player->userinfo.netname &&
		    (*it).team == player->userinfo.team)
			return;
	}

	WDLPlayer wdlplayer = {
	    ::wdlplayers.size() + 1,
		player->userinfo.netname,
		player->userinfo.team,
	};
	::wdlplayers.push_back(wdlplayer);
}

static void AddWDLPlayerSpawn(const mapthing2_t *mthing)
{

	team_t team = TEAM_NONE;

	if (sv_teamspawns != 0)
	{
		if (mthing->type == 5080)
			team = TEAM_BLUE;
		else if (mthing->type == 5081)
			team = TEAM_RED;
		else if (mthing->type == 5083)
			team = TEAM_GREEN;
	}

	// [BC] Add player spawns to the table with team info.
	WDLPlayerSpawns::const_iterator it = ::wdlplayerspawns.begin();
	for (; it != ::wdlplayerspawns.end(); ++it)
	{
		if ((*it).x == mthing->x && (*it).y == mthing->y && (*it).z == mthing->z && (*it).team == team)
			return;
	}

	WDLPlayerSpawn wdlplayerspawn = {::wdlplayerspawns.size() + 1, mthing->x, mthing->y,
	                                 mthing->z, team};
	::wdlplayerspawns.push_back(wdlplayerspawn);
}

static void AddWDLFlagLocation(const mapthing2_t* mthing, team_t team)
{
	// [BC] Add flag pedestals to the table.
	WDLFlagLocations::const_iterator it = ::wdlflaglocations.begin();
	for (; it != ::wdlflaglocations.end(); ++it)
	{
		if ((*it).x == mthing->x && (*it).y == mthing->y && (*it).z == mthing->z && (*it).team == team)
			return;
	}

	WDLFlagLocation wdlflaglocation = {team, mthing->x,
	                                   mthing->y,
	                                 mthing->z};
	::wdlflaglocations.push_back(wdlflaglocation);
}

static fixed_t AddWDLItemSpawn(const AActor* actor, int item)
{
	// [BC] Add item spawn to the table.
	fixed_t itemid; // ID of the spawn to return.

	WDLItemSpawns::const_iterator it = ::wdlitemspawns.begin();
	for (; it != ::wdlitemspawns.end(); ++it)
	{
		if ((*it).x == actor->x && (*it).y == actor->y && (*it).z == actor->z && (*it).item == item)
			return (*it).id;
	}

	itemid = ::wdlitemspawns.size() + 1;

	WDLItemSpawn wdlitemspawn = {itemid, actor->x, actor->y,
	                                 actor->z, static_cast<WDLPowerups>(item)};
	::wdlitemspawns.push_back(wdlitemspawn);

	return itemid;
}

static WDLPowerups GetWDLItemByActor(const AActor* actor)
{
	// [BC] Return a WDL item based on the actor that was spawned.
	// Helps with pickup table lookups.

	WDLPowerups itemid;

	switch (actor->type)
	{
		case MT_MISC0:
		itemid = WDL_PICKUP_GREENARMOR;
		break;
	    case MT_MISC1:
		    itemid = WDL_PICKUP_BLUEARMOR;
		break;
	    case MT_MISC2:
		    itemid = WDL_PICKUP_HEALTHBONUS;
		break;
	    case MT_MISC3:
		    itemid = WDL_PICKUP_ARMORBONUS;
		break;
	    case MT_MISC10:
		    itemid = WDL_PICKUP_STIMPACK;
		break;
	    case MT_MISC11:
		    itemid = WDL_PICKUP_MEDKIT;
		break;
	    case MT_MISC12:
		    itemid = WDL_PICKUP_SOULSPHERE;
		break;
	    case MT_INV:
		    itemid = WDL_PICKUP_INVULNSPHERE;
		break;
	    case MT_MISC13:
		    itemid = WDL_PICKUP_BERSERK;
		break;
	    case MT_INS:
		    itemid = WDL_PICKUP_INVISSPHERE;
		break;
	    case MT_MISC14:
		    itemid = WDL_PICKUP_RADSUIT;
		break;
	    case MT_MISC15:
		    itemid = WDL_PICKUP_COMPUTERMAP;
		break;
	    case MT_MISC16:
		    itemid = WDL_PICKUP_GOGGLES;
		break;
	    case MT_MEGA:
		    itemid = WDL_PICKUP_MEGASPHERE;
		break;
	    case MT_CLIP:
		    itemid = WDL_PICKUP_CLIP;
		break;
	    case MT_MISC17:
		    itemid = WDL_PICKUP_AMMOBOX;
		break;
	    case MT_MISC18:
		    itemid = WDL_PICKUP_ROCKET;
		break;
	    case MT_MISC19:
		    itemid = WDL_PICKUP_ROCKETBOX;
		break;
	    case MT_MISC20:
		    itemid = WDL_PICKUP_CELL;
		break;
	    case MT_MISC21:
		    itemid = WDL_PICKUP_CELLPACK;
		break;
	    case MT_MISC22:
		    itemid = WDL_PICKUP_SHELLS;
		break;
	    case MT_MISC23:
		    itemid = WDL_PICKUP_SHELLBOX;
		break;
	    case MT_MISC24:
		    itemid = WDL_PICKUP_BACKPACK;
		break;
	    case MT_MISC25:
		    itemid = WDL_PICKUP_BFG;
		break;
	    case MT_CHAINGUN:
		    itemid = WDL_PICKUP_CHAINGUN;
		break;
	    case MT_MISC26:
		    itemid = WDL_PICKUP_CHAINSAW;
		break;
	    case MT_MISC27:
		    itemid = WDL_PICKUP_ROCKETLAUNCHER;
		break;
	    case MT_MISC28:
		    itemid = WDL_PICKUP_PLASMAGUN;
		break;
	    case MT_SHOTGUN:
		    itemid = WDL_PICKUP_SHOTGUN;
		break;
	    case MT_SUPERSHOTGUN:
		    itemid = WDL_PICKUP_SUPERSHOTGUN;
		break;
	    default:
		    itemid = WDL_PICKUP_UNKNOWN;
		break;
	}

	return itemid;
}

// Generate a log filename based on the current time.
static std::string GenerateTimestamp()
{
	time_t ti = time(NULL);
	struct tm *lt = localtime(&ti);

	char buf[128];
	if (!strftime(&buf[0], ARRAY_LENGTH(buf), "%Y.%m.%d.%H.%M.%S", lt))
		return "";

	return std::string(buf, strlen(&buf[0]));
}

static void WDLStatsHelp()
{
	Printf(PRINT_HIGH,
		"wdlstats - Starts logging WDL statistics to the given directory.  Unless "
		"you are running a WDL server, you probably are not interested in this.\n\n"
		"Usage:\n"
		"  ] wdlstats <DIRNAME>\n"
		"  Starts logging WDL statistics in the directory DIRNAME.\n");
}

BEGIN_COMMAND(wdlstats)
{
	if (argc < 2)
	{
		WDLStatsHelp();
		return;
	}

	// Setting the stats dir tells us that we intend to log.
	::wdlstate.logdir = argv[1];

	// Ensure our path ends with a slash.
	if (*(::wdlstate.logdir.end() - 1) != PATHSEPCHAR)
		::wdlstate.logdir += PATHSEPCHAR;

	Printf(
		PRINT_HIGH, "wdlstats: Enabled, will log to directory \"%s\" on next map change.\n",
		wdlstate.logdir.c_str()
	);
}
END_COMMAND(wdlstats)

void M_StartWDLLog()
{
	if (::wdlstate.logdir.empty())
	{
		::wdlstate.recording = false;
		return;
	}

	// This used to only support CTF
	// but now, this supports all game modes.
	/*
	if (sv_gametype != 3)
	{
		::wdlstate.recording = false;
		Printf(
			PRINT_HIGH,
			"wdlstats: Not logging, incorrect gametype.\n"
		);
		return;
	}
	*/

	// Ensure that we're not in an invalid warmup state.
	if (::levelstate.getState() != LevelState::INGAME)
	{
		// [AM] This is a little too much inside baseball to print about.
		::wdlstate.recording = false;
		return;
	}

	// Clear our ingame players.
	::wdlplayers.clear();

	// Start with a fresh slate of events.
	::wdlevents.clear();

	// Ditto for flag locations, item spawns and player spawns.
	//::wdlflaglocations.clear();
	::wdlitemspawns.clear();
	//::wdlplayerspawns.clear();

	// set playerbeacons 
	if (sv_playerbeacons)
		::wdlstate.enablebeacons = true;
	else
		::wdlstate.enablebeacons = false;
	// Turn on recording.
	::wdlstate.recording = true;

	// Set our starting tic.
	::wdlstate.begintic = ::gametic;

	Printf(
		PRINT_HIGH, "wdlstats: Started, will log to directory \"%s\".\n",
		wdlstate.logdir.c_str()
	);
}

/**
 * Log a damage event.
 * 
 * Because damage can come in multiple pieces, this checks for an existing
 * event this tic and adds to it if it finds one.
 * 
 * Returns true if the function successfully appended to an existing event,
 * otherwise false if we need to generate a new event.
 */
static bool LogDamageEvent(
	WDLEvents event, player_t* activator, player_t* target,
	int arg0, int arg1, int arg2
)
{
	WDLEventLog::reverse_iterator it = ::wdlevents.rbegin();
	for (;it != ::wdlevents.rend(); ++it)
	{
		if ((*it).gametic != ::gametic)
		{
			// We're too late for events from last tic, so we must have a
			// new event.
			return false;
		}

		// Event type is the same?
		if ((*it).ev != event)
			continue;

		// Activator is the same?
		if ((*it).activator != activator->id)
			continue;

		// Target is the same?
		if ((*it).target != target->id)
			continue;

		// Update our existing event.
		(*it).arg0 += arg0;
		(*it).arg1 += arg1;
		return true;
	}

	// We ran through all our events, must be a new event.
	return false;
}

/**
 * Log accuracy event.
 * 
 * Returns true if the function successfully appended to an existing event,
 * otherwise false if we need to generate a new event.
 */
static bool LogAccuracyEvent(
	WDLEvents event, player_t* activator, player_t* target,
	int arg0, int arg1, int arg2
)
{
	WDLEventLog::reverse_iterator it = ::wdlevents.rbegin();
	for (;it != ::wdlevents.rend(); ++it)
	{
		if ((*it).gametic != ::gametic)
		{
			// We're too late for events from last tic, so we must have a
			// new event.
			return false;
		}

		// Event type is the same?
		if ((*it).ev != event)
			continue;

		// Activator is the same?
		if ((*it).activator != activator->id)
			continue;

		// Target is the same?
		if ((*it).target != target->id)
			continue;

		// Update our existing event - by doing nothing.
		return true;
	}

	// We ran through all our events, must be a new event.
	return false;
}

/**
 * Log a WDL flag location.
 *
 *
 * Logs the initial flag location on spawn and puts it in the flag locations table.
 */
void M_LogWDLFlagLocation(mapthing2_t* activator, team_t team)
{
	AddWDLFlagLocation(activator, team);
}

/**
 * Log a WDL item respawn event.
 *
 *
 * Logs each time an item respawned during a WDL log recording.
 */
void M_LogWDLItemRespawnEvent(AActor* activator)
{
	if (!::wdlstate.recording)
		return;

	// Activator
	fixed_t itemspawnid = 0;
	fixed_t itemtype = 0;
	int ax = 0;
	int ay = 0;
	int az = 0;
	if (activator != NULL)
	{
		itemtype = GetWDLItemByActor(activator);
		// Add the id from the pickups table.
		itemspawnid = AddWDLItemSpawn(activator, itemtype);

		// Add the activator's body information.
		ax = activator->x;
		ay = activator->y;
		az = activator->z;
	}

	// Add the event to the log.
	WDLEvent evt = {WDL_EVENT_SPAWNITEM, NULL,  NULL,         ::gametic, {ax, ay, az},
	                {0, 0, 0},           itemtype, itemspawnid, 0};
	::wdlevents.push_back(evt);
}

/**
 * Log a WDL pickup event.
 *
 * 
 * This will log a player item or weapon pickup, and check it against the current pickup spawn table to determine if it needs to be added.
 * This does have a chance to record a ton of moving pickups on a conveyer belt or something, but whatever consumes the data can ignore
 * item pickups that only get picked up at the same location once if item respawn is on.
 */
void M_LogWDLPickupEvent(player_t* activator, AActor* target, WDLPowerups pickuptype)
{
	if (!::wdlstate.recording)
		return;

	// Activator
	fixed_t aid = 0;
	int ax = 0;
	int ay = 0;
	int az = 0;
	if (activator != NULL)
	{
		// Add the activator.
		AddWDLPlayer(activator);
		aid = activator->id;

		// Add the activator's body information.
		if (activator->mo)
		{
			ax = activator->mo->x >> FRACBITS;
			ay = activator->mo->y >> FRACBITS;
			az = activator->mo->z >> FRACBITS;
		}
	}

	// Target
	fixed_t tid = 0;
	fixed_t itemspawnid = 0;
	int tx = 0;
	int ty = 0;
	int tz = 0;
	if (target != NULL)
	{
		tx = target->x;
		ty = target->y;
		tz = target->z;

		// Add the target.
		itemspawnid = AddWDLItemSpawn(target, pickuptype);
	}

	// Add the event to the log.
	WDLEvent evt = {WDL_EVENT_PICKUPITEM, aid,  tid,  ::gametic, {ax, ay, az},
	    {tx, ty, tz},         pickuptype, itemspawnid, 0};
	::wdlevents.push_back(evt);
}

/**
 * Log a WDL event.
 * 
 * The particulars of what you pass to this needs to be checked against the document.
 */
void M_LogWDLEvent(
	WDLEvents event, player_t* activator, player_t* target,
	int arg0, int arg1, int arg2, int arg3
)
{
	if (!::wdlstate.recording)
		return;

	if (event == WDL_EVENT_PLAYERBEACON && !::wdlstate.enablebeacons)
		return;

	// Activator
	fixed_t aid = 0;
	int ax = 0;
	int ay = 0;
	int az = 0;
	if (activator != NULL)
	{
		// Add the activator.
		AddWDLPlayer(activator);
		aid = activator->id;

		// Add the activator's body information.
		if (activator->mo)
		{
			ax = activator->mo->x >> FRACBITS;
			ay = activator->mo->y >> FRACBITS;
			az = activator->mo->z >> FRACBITS;
		}
	}

	// Target
	fixed_t tid = 0;
	int tx = 0;
	int ty = 0;
	int tz = 0;
	if (target != NULL)
	{
		// Add the target.
		AddWDLPlayer(target);
		tid = target->id;

		// Add the target's body information.
		if (target->mo)
		{
			tx = target->mo->x >> FRACBITS;
			ty = target->mo->y >> FRACBITS;
			tz = target->mo->z >> FRACBITS;
		}
	}

	// Damage events are handled specially.
	if (
		activator && target &&
		(event == WDL_EVENT_DAMAGE || event == WDL_EVENT_CARRIERDAMAGE)
	) {
		if (LogDamageEvent(event, activator, target, arg0, arg1, arg2))
			return;
	}

	// Add the event to the log.
	WDLEvent evt = {
		event, aid, tid, ::gametic,
		{ ax, ay, az }, { tx, ty, tz },
		arg0, arg1, arg2
	};
	::wdlevents.push_back(evt);
}

/**
 * Log a WDL event when you have actor pointers.
 */
void M_LogActorWDLEvent(
	WDLEvents event, AActor* activator, AActor* target,
	int arg0, int arg1, int arg2
)
{
	if (!::wdlstate.recording)
		return;

	player_t* ap = NULL;
	if (activator != NULL && activator->type == MT_PLAYER)
		ap = activator->player;

	player_t* tp = NULL;
	if (target != NULL && target->type == MT_PLAYER)
		tp = target->player;

	M_LogWDLEvent(event, ap, tp, arg0, arg1, arg2, 0);
}

/**
 * Log a shot attempt made by a player.
 * 
 * If there's already an accuracy record for this gametic with a populated actor
 * then create a new one because the shot hit more than 1 player.
 */
void M_LogWDLAccuracyShot(WDLEvents event, player_t* activator, int mod, angle_t angle)
{
	if (!::wdlstate.recording)
		return;

	fixed_t aid = 0;
	int ax = 0;
	int ay = 0;
	int az = 0;
	if (activator != NULL)
	{
		// Add the activator.
		AddWDLPlayer(activator);
		aid = activator->id;

		// Add the activator's body information.
		if (activator->mo)
		{
			ax = activator->mo->x >> FRACBITS;
			ay = activator->mo->y >> FRACBITS;
			az = activator->mo->z >> FRACBITS;
		}
	}

	// See if we have an existing accuracy event for this tic.
	// If not, we need to create a new one
	// If there is an existing accuracy event for this tic and it has a target,
	// then there were more than 1 hits, create a new event.
	WDLEventLog::reverse_iterator it = ::wdlevents.rbegin();
	for (; it != ::wdlevents.rend(); ++it)
	{
		if ((*it).gametic != ::gametic)
		{
			// Whoops, we went a whole gametic without seeing an accuracy
			// to our name.
			break;
		}

		// Event type is the same?
		if ((*it).ev != event)
			continue;

		// Activator is the same?
		if ((*it).activator != activator->id)
			continue;

		// We found an existing accuracy event for this tic.
		// Do nothing.
		return;
	}

	// Add the event to the log.
	WDLEvent evt = {
		event, aid, 0, ::gametic,
		{ ax, ay, az }, { 0, 0, 0 }, angle, mod, 0, GetMaxShotsForMod(mod)
	};
	::wdlevents.push_back(evt);
}

/**
 * Log a hit shot by a player
 *
 * Looks for an accuracy log somewhere in the backlog, if there is none, it
 * logs a message but continues.
 * 
 * 
 */
void M_LogWDLAccuracyHit(WDLEvents event, player_t* activator, player_t* target, int mod,
                         int hits)
{
	if (!::wdlstate.recording)
		return;

	fixed_t aid = 0;
	int ax = 0;
	int ay = 0;
	int az = 0;
	if (activator != NULL)
	{
		// Add the activator.
		AddWDLPlayer(activator);
		aid = activator->id;

		// Add the activator's body information.
		if (activator->mo)
		{
			ax = activator->mo->x >> FRACBITS;
			ay = activator->mo->y >> FRACBITS;
			az = activator->mo->z >> FRACBITS;
		}
	}

	// Target
	fixed_t tid = 0;
	int tx = 0;
	int ty = 0;
	int tz = 0;
	if (target != NULL)
	{
		// Add the target.
		AddWDLPlayer(target);
		tid = target->id;

		// Add the target's body information.
		if (target->mo)
		{
			tx = target->mo->x >> FRACBITS;
			ty = target->mo->y >> FRACBITS;
			tz = target->mo->z >> FRACBITS;
		}
	}

	// See if we have an existing accuracy event for this tic.
	WDLEventLog::reverse_iterator it = ::wdlevents.rbegin();
	for (; it != ::wdlevents.rend(); ++it)
	{
		if ((*it).gametic != ::gametic)
		{
			// Whoops, we went a whole gametic without seeing an accuracy
			// to our name.
			break;
		}

		// Event type is the same?
		if ((*it).ev != event)
			continue;

		// Activator is the same?
		if ((*it).activator != activator->id)
			continue;

		// Target is the same?
		if ((*it).target != target->id && (*it).target != 0)
			continue;

		// We found an existing accuracy event for this tic - increment the number of
		// shots hit if its a spread type
		(*it).target = target->id;
		(*it).arg2 += hits;
		(*it).tpos[0] = tx;
		(*it).tpos[1] = ty;
		(*it).tpos[2] = tz;
		return;
	}

	// Add the event to the log if for some reason it does not exist.
	WDLEvent evt = {event, aid, tid, ::gametic, {ax, ay, az}, {tx, ty, tz}, 0, mod, hits, GetMaxShotsForMod(mod)};
	::wdlevents.push_back(evt);
}

void M_LogWDLPlayerSpawn(mapthing2_t *mthing)
{
	AddWDLPlayerSpawn(mthing);
}

void M_HandleWDLNameChange(team_t team, std::string oldname, std::string newname)
{
	// [BC] Add player spawns to the table with team info.
	WDLPlayers::iterator it = ::wdlplayers.begin();
	for (; it != ::wdlplayers.end(); ++it)
	{
		// If there are multiple spawns in the same exact area, it will only count as one
		// spawn.
		if ((*it).netname == oldname && (*it).team == team)
		{
			(*it).netname = newname;
			return;
		}
	}
}

int M_GetPlayerSpawn(int x, int y, int z, team_t team)
{
	WDLPlayerSpawns::const_iterator it = ::wdlplayerspawns.begin();
	for (; it != ::wdlplayerspawns.end(); ++it)
	{
		if ((*it).x == x && (*it).y == y && (*it).z == z && (*it).team == team)
			return (*it).id;
	}
}

weapontype_t M_MODToWeapon(int mod)
{
	switch (mod)
	{
	case MOD_FIST:
		return wp_fist;
	case MOD_PISTOL:
		return wp_pistol;
	case MOD_SHOTGUN:
		return wp_shotgun;
	case MOD_CHAINGUN:
		return wp_chaingun;
	case MOD_ROCKET:
	case MOD_R_SPLASH:
		return wp_missile;
	case MOD_PLASMARIFLE:
		return wp_plasma;
	case MOD_BFG_BOOM:
	case MOD_BFG_SPLASH:
		return wp_bfg;
	case MOD_CHAINSAW:
		return wp_chainsaw;
	case MOD_SSHOTGUN:
		return wp_supershotgun;
	}

	return NUMWEAPONS;
}

// [BC] Helper function to determine max amount of shots that a mod shoots at a time.
int GetMaxShotsForMod(int mod)
{
	switch (mod)
	{
	case MOD_FIST:
	case MOD_PISTOL:
	case MOD_CHAINGUN:
	case MOD_ROCKET:
	case MOD_R_SPLASH:
	case MOD_CHAINSAW:
	case MOD_PLASMARIFLE:
	case MOD_BFG_BOOM:
		return 1;
	case MOD_SHOTGUN:
		return 7;
	case MOD_BFG_SPLASH:
		return 40;
	case MOD_SSHOTGUN:
		return 20;
	}

	return 1;
}

void M_CommitWDLLog()
{
	if (!::wdlstate.recording || wdlevents.empty())
		return;

	// See if we can write a file.
	std::string timestamp = GenerateTimestamp();
	std::string filename = ::wdlstate.logdir + "wdl_" + timestamp + ".log";

	// [BC] Make the in-file timestamp ISO 8601 instead of a homegrown one.
	// However, keeping the homegrown one for filename as ISO 8601 characters aren't supported in Windows filenames.
	std::stringstream iso8601timestamp;
	std::time_t t =
	    std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	iso8601timestamp << std::put_time(std::localtime(&t), "%FT%T%z");

	FILE* fh = fopen(filename.c_str(), "w+");
	if (fh == NULL)
	{
		::wdlstate.recording = false;
		Printf(PRINT_HIGH, "wdlstats: Could not save\"%s\" for writing.\n", filename.c_str());
		return;
	}

	// Header
	fprintf(fh, "version=%d\n", WDLSTATS_VERSION);
	fprintf(fh, "time=%s\n", iso8601timestamp.str().c_str());
	fprintf(fh, "levelnum=%d\n", ::level.levelnum);
	fprintf(fh, "levelname=%s\n", ::level.level_name);
	fprintf(fh, "levelhash=%s\n", ::level.level_hash.c_str());
	fprintf(fh, "gametype=%s\n", ::sv_gametype.cstring());
	fprintf(fh, "duration=%d\n", ::gametic - ::wdlstate.begintic);
	fprintf(fh, "endgametic=%d\n", ::gametic);

	// Players
	fprintf(fh, "players\n");
	WDLPlayers::const_iterator pit = ::wdlplayers.begin();
	for (; pit != ::wdlplayers.end(); ++pit)
		fprintf(fh, "%d,%d,%s\n", pit->id,pit->team, pit->netname.c_str());

	// ItemSpawns
	fprintf(fh, "itemspawns\n");
	WDLItemSpawns::const_iterator isit = ::wdlitemspawns.begin();
	for (; isit != ::wdlitemspawns.end(); ++isit)
		fprintf(fh, "%d,%d,%d,%d,%d\n", isit->id, isit->x, isit->y, isit->z, isit->item);

	// PlayerSpawns
	fprintf(fh, "playerspawns\n");
	WDLPlayerSpawns::const_iterator psit = ::wdlplayerspawns.begin();
	for (; psit != ::wdlplayerspawns.end(); ++psit)
		fprintf(fh, "%d,%d,%d,%d,%d\n", psit->id, psit->team, psit->x, psit->y, psit->z);

	if (sv_gametype == GM_CTF)
	{
		// FlagLocation
		fprintf(fh, "flaglocations\n");
		WDLFlagLocations::const_iterator flit = ::wdlflaglocations.begin();
		for (; flit != ::wdlflaglocations.end(); ++flit)
			fprintf(fh, "%d,%d,%d,%d\n", flit->team, flit->x, flit->y, flit->z);
	}

	// Events
	fprintf(fh, "events\n");
	WDLEventLog::const_iterator eit = ::wdlevents.begin();
	for (; eit != ::wdlevents.end(); ++eit)
	{
		//          "ev,ac,tg,gt,ax,ay,az,tx,ty,tz,a0,a1,a2,a3"
		fprintf(fh, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
			eit->ev, eit->activator, eit->target, eit->gametic,
			eit->apos[0], eit->apos[1], eit->apos[2],
			eit->tpos[0], eit->tpos[1], eit->tpos[2],
			eit->arg0, eit->arg1, eit->arg2, eit->arg3);
	}

	fclose(fh);

	// Turn off stat recording global - it must be turned on again by the
	// log starter next go-around.
	::wdlstate.recording = false;

	Printf(PRINT_HIGH, "wdlstats: Log saved as \"%s\".\n", filename.c_str());
}

static void PrintWDLEvent(const WDLEvent& evt)
{
	// FIXME: Once we have access to StrFormat, dedupe this format string.
	//                 "ev,ac,tg,gt,ax,ay,az,tx,ty,tz,a0,a1,a2,a3"
	Printf(PRINT_HIGH, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
		evt.ev, evt.activator, evt.target, evt.gametic,
		evt.apos[0], evt.apos[1], evt.apos[2],
		evt.tpos[0], evt.tpos[1], evt.tpos[2],
		evt.arg0, evt.arg1, evt.arg2, evt.arg3);
}

static void WDLInfoHelp()
{
	Printf(PRINT_HIGH,
		"wdlinfo - Looks up internal information about logged WDL events\n\n"
		"Usage:\n"
		"  ] wdlinfo event <ID>\n"
		"  Print the event by ID.\n\n"
		"  ] wdlinfo size\n"
		"  Return the size of the internal event array.\n\n"
		"  ] wdlinfo state\n"
		"  Return relevant WDL stats state.\n\n"
		"  ] wdlinfo tail\n"
		"  Print the last 10 events.\n");
}

BEGIN_COMMAND(wdlinfo)
{
	if (argc < 2)
	{
		WDLInfoHelp();
		return;
	}

	if (stricmp(argv[1], "size") == 0)
	{
		// Count total events.
		Printf(PRINT_HIGH, "%" PRIuSIZE " events found\n", ::wdlevents.size());
		return;
	}
	else if (stricmp(argv[1], "state") == 0)
	{
		// Count total events.
		Printf(PRINT_HIGH, "Currently recording?: %s\n", ::wdlstate.recording ? "Yes" : "No");
		Printf(PRINT_HIGH, "Directory to write logs to: \"%s\"\n", ::wdlstate.logdir.c_str());
		Printf(PRINT_HIGH, "Log starting gametic: %d\n", ::wdlstate.begintic);
		return;
	}
	else if (stricmp(argv[1], "tail") == 0)
	{
		// Show last 10 events.
		WDLEventLog::const_iterator it = ::wdlevents.end() - 10;
		if (it < ::wdlevents.begin())
			it = wdlevents.begin();

		Printf(PRINT_HIGH, "Showing last %" PRIdSIZE " events:\n", ::wdlevents.end() - it);
		for (; it != ::wdlevents.end(); ++it)
			PrintWDLEvent(*it);
		return;
	}

	if (argc < 3)
	{
		WDLInfoHelp();
		return;
	}

	if (stricmp(argv[1], "event") == 0)
	{
		int id = atoi(argv[2]);
		if (id >= ::wdlevents.size())
		{
			Printf(PRINT_HIGH, "Event number %d not found\n", id);
			return;
		}
		WDLEvent evt = ::wdlevents.at(id);
		PrintWDLEvent(evt);
		return;
	}

	// Unknown command.
	WDLInfoHelp();
}
END_COMMAND(wdlinfo)
