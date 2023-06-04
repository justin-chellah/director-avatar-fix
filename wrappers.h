#pragma once

#include <ehandle.h>
#include <utlmap.h>

#define TEAM_SURVIVOR               2
#define TEAM_ZOMBIE                 3

enum SurvivorCharacterType
{
	SurvivorCharacter_Gambler = 0,
	SurvivorCharacter_Producer,
	SurvivorCharacter_Coach,
	SurvivorCharacter_Mechanic,
	
	SurvivorCharacter_NamVet,
	SurvivorCharacter_TeenGirl,
	SurvivorCharacter_Biker,
	SurvivorCharacter_Manager,

	SurvivorCharacter_Unknown
};

class CDirector
{
private:
	char padding[284];

public:
	CUtlMap< uint64_t, KeyValues * > m_Avatars;

	enum PlayerCountType
	{
		HUMANS = 0,
		BOTS,
		BOTS_AND_HUMANS		// For infected team
	};
};

class DirectorNewPlayerType_t
{
public:
	CHandle< CBaseEntity > m_hNewPlayer;
};

class PlayerCollector
{
private:
	char padding[16];

public:
	CUtlVector< CBaseEntity * > m_vecPlayers;
};