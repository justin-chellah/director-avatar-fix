#include <sourcemod>
#include <sdktools_gamerules>

#define REQUIRE_EXTENSIONS
#include <director_avatar_fix>

#define TEAM_UNASSIGNED		0
#define TEAM_SURVIVOR		2
#define TEAM_INFECTED		3

ConVar survivor_limit = null;
ConVar z_max_player_zombies = null;

SurvivorCharacterType GetCharacterFromName( const char[] szName )
{
	if ( StrEqual( szName, "Gambler", false ) || StrEqual( szName, "Nick", false ) )
	{
		return SurvivorCharacter_Gambler;
	}

	if ( StrEqual( szName, "Producer", false ) || StrEqual( szName, "Rochelle", false ) )
	{
		return SurvivorCharacter_Producer;
	}

	if ( StrEqual( szName, "Coach", false ) )
	{
		return SurvivorCharacter_Coach;
	}

	if ( StrEqual( szName, "Mechanic", false ) || StrEqual( szName, "Ellis", false ) )
	{
		return SurvivorCharacter_Mechanic;
	}

	if ( StrEqual( szName, "NamVet", false ) || StrEqual( szName, "Bill", false ) )
	{
		return SurvivorCharacter_NamVet;
	}

	if ( StrEqual( szName, "TeenGirl", false ) || StrEqual( szName, "TeenAngst", false ) || StrEqual( szName, "Zoey", false ) )
	{
		return SurvivorCharacter_TeenGirl;
	}

	if ( StrEqual( szName, "Biker", false ) || StrEqual( szName, "Francis", false ) )
	{
		return SurvivorCharacter_Biker;
	}

	if ( StrEqual( szName, "Manager", false ) || StrEqual( szName, "Louis", false ) )
	{
		return SurvivorCharacter_Manager;
	}

	return SurvivorCharacter_Unknown;
}

int CountPlayerAvatars( int iTeamNum, SurvivorCharacterType eSurvivorCharacter = SurvivorCharacter_Unknown )
{
	KeyValues hAvatarInfo = null;

	int iAvatarTeam = TEAM_UNASSIGNED;
	SurvivorCharacterType eAvatarSurvivorCharacter = SurvivorCharacter_Unknown;

	int nClients = 0;

	bool bAreTeamsFlipped = view_as< bool >( GameRules_GetProp( "m_bAreTeamsFlipped", 1 ) );

	for (int iClient = 1; iClient <= MaxClients; iClient++ )
	{
		if ( !IsClientConnected( iClient ) )
		{
			continue;
		}

		if ( IsFakeClient( iClient ) )
		{
			continue;
		}

		hAvatarInfo = new KeyValues( NULL_STRING );

		if ( PlayerAvatarGet( iClient, hAvatarInfo ) )
		{
			PlayerAvatarUnpack( hAvatarInfo, iAvatarTeam, eAvatarSurvivorCharacter );

			if ( bAreTeamsFlipped )
			{
				switch ( iAvatarTeam )
				{
					case TEAM_SURVIVOR:
					{
						iAvatarTeam = TEAM_INFECTED;
					}

					case TEAM_INFECTED:
					{
						iAvatarTeam = TEAM_SURVIVOR;
					}
				}
			}

			if ( iAvatarTeam != iTeamNum )
			{
				delete hAvatarInfo;

				continue;
			}

			if ( eSurvivorCharacter < SurvivorCharacter_Unknown )
			{
				if ( eSurvivorCharacter != eAvatarSurvivorCharacter )
				{
					delete hAvatarInfo;

					continue;
				}

				nClients++;
				break;
			}

			nClients++;
		}

		delete hAvatarInfo;
	}

	return nClients;
}

public Action CommandListener_jointeam( int iClient, const char[] szCommand, int nArgs )
{
	char szTeam[32];
	GetCmdArg( 1, szTeam, sizeof( szTeam ) );

	int iTeamNum = StringToInt( szTeam );

	if ( iTeamNum == TEAM_SURVIVOR || StrEqual( szTeam, "Survivor", false ) )
	{
		iTeamNum = TEAM_SURVIVOR;
	}
	else if ( iTeamNum == TEAM_INFECTED || StrEqual( szTeam, "Infected", false ) )
	{
		iTeamNum = TEAM_INFECTED;
	}

	if ( iTeamNum != TEAM_SURVIVOR && iTeamNum != TEAM_INFECTED )
	{
		return Plugin_Continue;
	}

	switch ( iTeamNum )
	{
		case TEAM_SURVIVOR:
		{
			if ( CountPlayerAvatars( TEAM_SURVIVOR ) >= survivor_limit.IntValue )
			{
				PrintToChat( iClient, "%t", "Unable to join a team" );

				return Plugin_Handled;
			}

			char szSurvivorCharacter[32];
			GetCmdArg( 2, szSurvivorCharacter, sizeof( szSurvivorCharacter ) );

			SurvivorCharacterType eSurvivorCharacterType = GetCharacterFromName( szSurvivorCharacter );

			if ( eSurvivorCharacterType < SurvivorCharacter_Unknown && CountPlayerAvatars( TEAM_SURVIVOR, eSurvivorCharacterType ) > 0 )
			{
				PrintToChat( iClient, "%t", "Unable to take character" );

				return Plugin_Handled;
			}
		}

		case TEAM_INFECTED:
		{
			if ( CountPlayerAvatars( TEAM_INFECTED ) >= z_max_player_zombies.IntValue )
			{
				PrintToChat( iClient, "%t", "Unable to join a team" );

				return Plugin_Handled;
			}
		}
	}

	return Plugin_Continue;
}

public void OnPluginStart()
{
	LoadTranslations( "no_team_change_during_transition.phrases" );

	survivor_limit = FindConVar( "survivor_limit" );
	z_max_player_zombies = FindConVar( "z_max_player_zombies" );

	AddCommandListener( CommandListener_jointeam, "jointeam" );
}

public Plugin myinfo =
{
	name = "[L4D2] No Team Change During Transition",
	author = "Justin \"Sir Jay\" Chellah",
	description = "Prevents players from changing teams and taking over other characters while they're loading",
	version = "1.0.0",
	url = "https://justin-chellah.com"
};