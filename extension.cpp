#include "extension.h"
#include "CDetour/detours.h"
#include <vstdlib/random.h>
#include <vstdlib/IKeyValuesSystem.h>
#include "natives.h"
#include "../imatchext/IMatchExtInterface.h"

CDirectorAvatarExt g_DirectorAvatarExt;

IMatchExtInterface *imatchext = nullptr;
CBaseEntityList *g_pEntityList = nullptr;
IServerGameClients *serverGameClients = nullptr;
ICvar *g_pCVar = nullptr;

SMEXT_LINK(&g_DirectorAvatarExt);

SH_DECL_HOOK2_void(IServerGameClients, ClientCommandKeyValues, SH_NOATTRIB, 0, edict_t *, KeyValues *);

static bool HasPlayerControlledZombies()
{
	static ConVarRef mp_gamemode("mp_gamemode");

	const char *pszName = mp_gamemode.GetString();

#if SOURCE_ENGINE == SE_LEFT4DEAD2
	KeyValues *pModeInfo = imatchext->GetIMatchExtL4D()->GetGameModeInfo(pszName);

	if (pModeInfo)
	{
		return pModeInfo->GetInt("playercontrolledzombies") > 0;
	}

	return false;
#else
	return !V_stricmp(pszName, "versus") || !V_stricmp(pszName, "teamversus");
#endif
}

static bool IsTeamOnTeamMode()
{
	static ConVarRef mp_gamemode("mp_gamemode");

	const char *pszName = mp_gamemode.GetString();

	// teamversus/teamscavenge
	return !V_strncmp(pszName, "team", 4);
}

static SurvivorCharacterType GetCharacterFromName(const char *pszName)
{
	if (!V_stricmp(pszName, "Gambler") || !V_stricmp(pszName, "Nick"))
	{
		return SurvivorCharacter_Gambler;
	}

	if (!V_stricmp(pszName, "Producer") || !V_stricmp(pszName, "Rochelle"))
	{
		return SurvivorCharacter_Producer;
	}

	if (!V_stricmp(pszName, "Coach"))
	{
		return SurvivorCharacter_Coach;
	}

	if (!V_stricmp(pszName, "Mechanic") || !V_stricmp(pszName, "Ellis"))
	{
		return SurvivorCharacter_Mechanic;
	}

	if (!V_stricmp(pszName, "NamVet") || !V_stricmp(pszName, "Bill"))
	{
		return SurvivorCharacter_NamVet;
	}

	if (!V_stricmp(pszName, "TeenGirl") || !V_stricmp(pszName, "TeenAngst") || !V_stricmp(pszName, "Zoey"))
	{
		return SurvivorCharacter_TeenGirl;
	}

	if (!V_stricmp(pszName, "Biker") || !V_stricmp(pszName, "Francis"))
	{
		return SurvivorCharacter_Biker;
	}

	if (!V_stricmp(pszName, "Manager") || !V_stricmp(pszName, "Louis"))
	{
		return SurvivorCharacter_Manager;
	}

	return SurvivorCharacter_Unknown;
}

const char *GetNameFromCharacter(SurvivorCharacterType eSurvivorCharacter)
{
	if (eSurvivorCharacter <= SurvivorCharacter_Manager)
	{
		static const char *pszSurvivorCharacterName[] =
		{
			"Gambler",
			"Producer",
			"Coach",
			"Mechanic",

			"NamVet",
			"TeenGirl",
			"Biker",
			"Manager",
		};

		return pszSurvivorCharacterName[eSurvivorCharacter];
	}

	return "Unknown";
}

static void TryToApplyAvatarInfoChanges(edict_t *pEdict, KeyValues *pAvatarInfo)
{
	const char *pszTeamName = pAvatarInfo->GetString("team");

	// Spectators should remain spectators and not join any team
	if (!V_stricmp(pszTeamName, "Spectator"))
	{
		engine->ClientCommandKeyValues(pEdict, pAvatarInfo->MakeCopy());
	}

	// Team modes don't allow team changes
	else if (!IsTeamOnTeamMode())
	{
		engine->ClientCommandKeyValues(pEdict, pAvatarInfo->MakeCopy());
	}
}

DETOUR_DECL_MEMBER2(DetourFunc_CDirector_GetPlayerCount, int, int, iTeamNum, CDirector::PlayerCountType, ePlayerCountType)
{
	int nPlayerCount = DETOUR_MEMBER_CALL(DetourFunc_CDirector_GetPlayerCount)(iTeamNum, ePlayerCountType);

	if (ePlayerCountType == CDirector::BOTS)
	{
		return nPlayerCount;
	}

	if (HasPlayerControlledZombies())
	{
		nPlayerCount += g_DirectorAvatarExt.CountTransitioningPlayers(iTeamNum);
	}

	return nPlayerCount;
}

DETOUR_DECL_MEMBER1(DetourFunc_CDirector_JoinNewPlayer, void, DirectorNewPlayerType_t &, eNewPlayerType)
{
	CBaseEntity *pNewPlayer = eNewPlayerType.m_hNewPlayer;

	int iClient = gamehelpers->EntityToBCompatRef(pNewPlayer);

	edict_t *pEdict = gamehelpers->EdictOfIndex(iClient);

	KeyValues *pAvatarInfo = g_DirectorAvatarExt.PlayerAvatarGet(pEdict);

	int iAvatarTeam = TEAM_UNASSIGNED;
	SurvivorCharacterType eAvatarSurvivorCharacter = SurvivorCharacter_Unknown;

	g_DirectorAvatarExt.PlayerAvatarUnpack(pAvatarInfo, iAvatarTeam, eAvatarSurvivorCharacter);

// #define TEST_FAKE_CLIENT_COMMAND_KEY_VALUES
#if defined TEST_FAKE_CLIENT_COMMAND_KEY_VALUES
	// Sometimes client doesn't submit key-values command for avatar information (I don't know the conditions yet)
	if (!pAvatarInfo)
	{
		KeyValues *pKeyValues = new KeyValues("avatarinfo");
		KeyValues::AutoDelete autodelete(pKeyValues);

		engine->ClientCommandKeyValues(pEdict, pKeyValues->MakeCopy());

		pAvatarInfo = g_DirectorAvatarExt.PlayerAvatarGet(pEdict);
	}
#endif

	bool bIsAvatarTeamUnassigned = (iAvatarTeam == TEAM_UNASSIGNED);

	// bug#1: Players will be sent to random teams without respecting other transitioning players which can lead to losing spots
	// This function counts players in both teams to determine where the new players should join
	if (bIsAvatarTeamUnassigned)
	{
		g_DirectorAvatarExt.m_pDetour_CDirector_GetPlayerCount->EnableDetour();
	}

	DETOUR_MEMBER_CALL(DetourFunc_CDirector_JoinNewPlayer)(eNewPlayerType);

	g_DirectorAvatarExt.m_pDetour_CDirector_GetPlayerCount->DisableDetour();

	if (pAvatarInfo)
	{
		int iTeamNum = playerhelpers->GetGamePlayer(iClient)->GetPlayerInfo()->GetTeamIndex();

		// bug#2: Clients don't send avatar information to the server if it's unreserved when connecting directly to the server
		if (bIsAvatarTeamUnassigned)
		{
			g_DirectorAvatarExt.SetAvatarBasedOnCurrentTeam(pNewPlayer, pAvatarInfo);

			TryToApplyAvatarInfoChanges(pEdict, pAvatarInfo);
		}

		// bug#3: Players that weren't able to find a suitable team for themselves won't stay in spectators
		else if (iTeamNum == TEAM_SPECTATOR)
		{
			pAvatarInfo->SetString("team", "Spectator");

			TryToApplyAvatarInfoChanges(pEdict, pAvatarInfo);
		}

		// bug#4: Players won't be sent to the right teams if they join the game while teams have been flipped if their destination team is full
		else if (g_DirectorAvatarExt.AreTeamsFlipped())
		{
			// Not supposed to be on the avatar team if teams are flipped
			if (iTeamNum == iAvatarTeam)
			{
				switch (iTeamNum)
				{
					case TEAM_SURVIVOR:
					{
						pAvatarInfo->SetString("team", "Infected");
						pAvatarInfo->SetString("avatar", "infected");
						break;
					}

					case TEAM_ZOMBIE:
					{
						g_DirectorAvatarExt.SetRandomUnusedSurvivorAvatar(pAvatarInfo);
						break;
					}
				}

				TryToApplyAvatarInfoChanges(pEdict, pAvatarInfo);
			}
		}

		// bug#5: Players won't be sent to the right teams after level transitions if they join the game while their spots have been occupied by other players

		// Supposed to be on the avatar team if teams aren't flipped
		else if (iTeamNum != iAvatarTeam)
		{
			switch (iTeamNum)
			{
				case TEAM_SURVIVOR:
				{
					g_DirectorAvatarExt.SetRandomUnusedSurvivorAvatar(pAvatarInfo);
					break;
				}

				case TEAM_ZOMBIE:
				{
					pAvatarInfo->SetString("team", "Infected");
					pAvatarInfo->SetString("avatar", "infected");
					break;
				}
			}

			TryToApplyAvatarInfoChanges(pEdict, pAvatarInfo);
		}
	}
}

// The Director will search for occupiable bots even when the survivor team is full
DETOUR_DECL_MEMBER1(DetourFunc_CDirector_NewPlayerFindAndPossessBot, bool, DirectorNewPlayerType_t &, eNewPlayerType)
{
	static ConVarRef survivor_limit("survivor_limit");

	CBaseEntity *pNewPlayer = eNewPlayerType.m_hNewPlayer;

	int iClient = gamehelpers->EntityToBCompatRef(pNewPlayer);

	edict_t *pEdict = gamehelpers->EdictOfIndex(iClient);

	KeyValues *pAvatarInfo = g_DirectorAvatarExt.PlayerAvatarGet(pEdict);

	int iAvatarTeam = TEAM_UNASSIGNED;
	SurvivorCharacterType eAvatarSurvivorCharacter = SurvivorCharacter_Unknown;

	g_DirectorAvatarExt.PlayerAvatarUnpack(pAvatarInfo, iAvatarTeam, eAvatarSurvivorCharacter);

	if (g_DirectorAvatarExt.AreTeamsFlipped())
	{
		switch (iAvatarTeam)
		{
			case TEAM_SURVIVOR:
			{
				iAvatarTeam = TEAM_ZOMBIE;
				break;
			}

			case TEAM_ZOMBIE:
			{
				iAvatarTeam = TEAM_SURVIVOR;
				break;
			}
		}
	}

	if (iAvatarTeam != TEAM_SURVIVOR && g_DirectorAvatarExt.CountTransitioningPlayers(TEAM_SURVIVOR) >= survivor_limit.GetInt())
	{
		return false;
	}

	// This is down here so that in case players who couldn't take over their character for some reason won't be taking anybody else's character
	g_DirectorAvatarExt.m_pDetour_CDirector_IsHumanSpectatorValid->EnableDetour();

	bool bInvocationResult = DETOUR_MEMBER_CALL(DetourFunc_CDirector_NewPlayerFindAndPossessBot)(eNewPlayerType);

	g_DirectorAvatarExt.m_pDetour_CDirector_IsHumanSpectatorValid->DisableDetour();

	return bInvocationResult;
}

// bug#6: This function doesn't properly update avatars on team changes which will cause survivors to lose the desired character after level transitions when teams aren't flipped
// bug#7: Having L4D1 avatars stored in avatar information prevents transitioning survivors to play as their desired character if all survivor bots have already joined on L4D2 maps
DETOUR_DECL_MEMBER0(DetourFunc_CTerrorPlayer_UpdateTeamDesired, void)
{
	CBaseEntity *pPlayer = reinterpret_cast<CBaseEntity *>(this);

	edict_t *pEdict = gamehelpers->EdictOfIndex(gamehelpers->EntityToBCompatRef(pPlayer));

	KeyValues *pAvatarInfo = g_DirectorAvatarExt.PlayerAvatarGet(pEdict);

	if (pAvatarInfo)
	{
		int iAvatarTeam = TEAM_UNASSIGNED;
		SurvivorCharacterType eAvatarSurvivorCharacter = SurvivorCharacter_Unknown;

		g_DirectorAvatarExt.PlayerAvatarUnpack(pAvatarInfo, iAvatarTeam, eAvatarSurvivorCharacter);
		g_DirectorAvatarExt.SetAvatarBasedOnCurrentTeam(pPlayer, pAvatarInfo);

		TryToApplyAvatarInfoChanges(pEdict, pAvatarInfo);
	}
}

// bug#9: The Director doesn't respect survivor bots that are reserved for other transitioning players during CDirector::NewPlayerFindAndPossessBot call
DETOUR_DECL_MEMBER1(DetourFunc_CDirector_IsHumanSpectatorValid, bool, CBaseEntity *, pSurvivorBot)
{
	// Infected players don't have survivor character avatars
	if (g_DirectorAvatarExt.AreTeamsFlipped())
	{
		return DETOUR_MEMBER_CALL(DetourFunc_CDirector_IsHumanSpectatorValid)(pSurvivorBot);
	}

	for (int iClient = 1; iClient <= g_DirectorAvatarExt.GetMaxPlayers(); iClient++)
	{
		IGamePlayer *pGamePlayer = playerhelpers->GetGamePlayer(iClient);

		if (!pGamePlayer)
		{
			continue;
		}

		if (!pGamePlayer->IsConnected())
		{
			continue;
		}

		edict_t *pEdict = gamehelpers->EdictOfIndex(iClient);

		KeyValues *pAvatarInfo = g_DirectorAvatarExt.PlayerAvatarGet(pEdict);

		int iAvatarTeam = TEAM_UNASSIGNED;
		SurvivorCharacterType eAvatarSurvivorCharacter = SurvivorCharacter_Unknown;

		g_DirectorAvatarExt.PlayerAvatarUnpack(pAvatarInfo, iAvatarTeam, eAvatarSurvivorCharacter);

		if (iAvatarTeam != TEAM_SURVIVOR)
		{
			continue;
		}

		if (g_DirectorAvatarExt.GetSurvivorCharacter(pSurvivorBot) != eAvatarSurvivorCharacter)
		{
			continue;
		}

		// Lie to the Director so they're not going to give this bot to the new player
		return true;
	}

	return DETOUR_MEMBER_CALL(DetourFunc_CDirector_IsHumanSpectatorValid)(pSurvivorBot);
}

bool CDirectorAvatarExt::SDK_OnLoad(char *error, size_t maxlen, bool late)
{
	sm_sendprop_info_t info;
	if (gamehelpers->FindSendPropInfo("CTerrorPlayer", "m_survivorCharacter", &info))
	{
		m_nOffset_CTerrorPlayer_m_survivorCharacter = info.actual_offset;
	}
	else
	{
		ke::SafeStrcpy(error, maxlen, "Unable to find SendProp \"CTerrorPlayer::m_survivorCharacter\"");

		return false;
	}

	if (gamehelpers->FindSendPropInfo("CTerrorGameRulesProxy", "m_bAreTeamsFlipped", &info))
	{
		m_nOffset_CTerrorGameRulesProxy_m_bAreTeamsFlipped = info.actual_offset;
	}
	else
	{
		ke::SafeStrcpy(error, maxlen, "Unable to find SendProp \"CTerrorGameRulesProxy::m_bAreTeamsFlipped\"");

		return false;
	}

	IGameConfig *pGameConfig;
	if (!gameconfs->LoadGameConfigFile(SMEXT_CONF_GAMEDATA_FILE, &pGameConfig, error, sizeof(error)))
	{
		ke::SafeStrcpy(error, maxlen, "Unable to load gamedata file \"" SMEXT_CONF_GAMEDATA_FILE ".txt\"");

		return false;
	}

	if (!pGameConfig->GetAddress("CDirector", reinterpret_cast<void **>(&m_pTheDirector)))
	{
		ke::SafeStrcpy(error, maxlen, "Unable to find gamedata address entry for \"CDirector\"");

		gameconfs->CloseGameConfigFile(pGameConfig);

		return false;
	}

	if (m_pTheDirector == nullptr)
	{
		ke::SafeStrcpy(error, maxlen, "Unable to find signature in binary for gamedata entry \"CDirector\"");

		gameconfs->CloseGameConfigFile(pGameConfig);

		return false;
	}

	CDetourManager::Init(smutils->GetScriptingEngine(), pGameConfig);

	// VS: Enable detour dynamically, so CDirector::GetPlayerCount will include transitioning players only during the CDirector::JoinNewPlayer call
	m_pDetour_CDirector_GetPlayerCount = DETOUR_CREATE_MEMBER(DetourFunc_CDirector_GetPlayerCount, "CDirector::GetPlayerCount");

	if (!m_pDetour_CDirector_GetPlayerCount)
	{
		ke::SafeStrcpy(error, maxlen, "Unable to find gamedata signature entry or signature in binary for \"CDirector::GetPlayerCount\"");

		gameconfs->CloseGameConfigFile(pGameConfig);

		return false;
	}

	m_pDetour_CDirector_JoinNewPlayer = DETOUR_CREATE_MEMBER(DetourFunc_CDirector_JoinNewPlayer, "CDirector::JoinNewPlayer");

	if (m_pDetour_CDirector_JoinNewPlayer)
	{
		m_pDetour_CDirector_JoinNewPlayer->EnableDetour();
	}
	else
	{
		ke::SafeStrcpy(error, maxlen, "Unable to find gamedata signature entry or signature in binary for CDirector::JoinNewPlayer");

		gameconfs->CloseGameConfigFile(pGameConfig);

		SDK_OnUnload();

		return false;
	}

	m_pDetour_CDirector_NewPlayerFindAndPossessBot = DETOUR_CREATE_MEMBER(DetourFunc_CDirector_NewPlayerFindAndPossessBot, "CDirector::NewPlayerFindAndPossessBot");

	if (m_pDetour_CDirector_NewPlayerFindAndPossessBot)
	{
		m_pDetour_CDirector_NewPlayerFindAndPossessBot->EnableDetour();
	}
	else
	{
		ke::SafeStrcpy(error, maxlen, "Unable to find gamedata signature entry or signature in binary for CDirector::NewPlayerFindAndPossessBot");

		gameconfs->CloseGameConfigFile(pGameConfig);

		SDK_OnUnload();

		return false;
	}

	m_pDetour_CTerrorPlayer_UpdateTeamDesired = DETOUR_CREATE_MEMBER(DetourFunc_CTerrorPlayer_UpdateTeamDesired, "CTerrorPlayer::UpdateTeamDesired");

	if (m_pDetour_CTerrorPlayer_UpdateTeamDesired)
	{
		m_pDetour_CTerrorPlayer_UpdateTeamDesired->EnableDetour();
	}
	else
	{
		ke::SafeStrcpy(error, maxlen, "Unable to find gamedata signature entry or signature in binary for CTerrorPlayer::UpdateTeamDesired");

		gameconfs->CloseGameConfigFile(pGameConfig);

		SDK_OnUnload();

		return false;
	}

	m_pDetour_CDirector_IsHumanSpectatorValid = DETOUR_CREATE_MEMBER(DetourFunc_CDirector_IsHumanSpectatorValid, "CDirector::IsHumanSpectatorValid");

	if (!m_pDetour_CDirector_IsHumanSpectatorValid)
	{
		ke::SafeStrcpy(error, maxlen, "Unable to find gamedata signature entry or signature in binary for CDirector::IsHumanSpectatorValid");

		gameconfs->CloseGameConfigFile(pGameConfig);

		SDK_OnUnload();

		return false;
	}

	gameconfs->CloseGameConfigFile(pGameConfig);

	playerhelpers->AddClientListener(this);

	if (late && playerhelpers->IsServerActivated())
	{
		OnServerActivated(playerhelpers->GetMaxClients());
	}

	g_pEntityList = reinterpret_cast<CBaseEntityList *>(gamehelpers->GetGlobalEntityList());

	sharesys->AddDependency(myself, "imatchext.ext", true, true);
	sharesys->AddDependency(myself, "sdktools.ext", true, true);

	sharesys->AddNatives(myself, g_Natives);
	sharesys->AddInterface(myself, this);
	sharesys->RegisterLibrary(myself, "director_avatar_fix");

	return true;
}

void CDirectorAvatarExt::SDK_OnUnload()
{
	if (m_pDetour_CDirector_GetPlayerCount)
	{
		m_pDetour_CDirector_GetPlayerCount->Destroy();
		m_pDetour_CDirector_GetPlayerCount = nullptr;
	}

	if (m_pDetour_CDirector_JoinNewPlayer)
	{
		m_pDetour_CDirector_JoinNewPlayer->Destroy();
		m_pDetour_CDirector_JoinNewPlayer = nullptr;
	}

	if (m_pDetour_CDirector_NewPlayerFindAndPossessBot)
	{
		m_pDetour_CDirector_NewPlayerFindAndPossessBot->Destroy();
		m_pDetour_CDirector_NewPlayerFindAndPossessBot = nullptr;
	}

	if (m_pDetour_CTerrorPlayer_UpdateTeamDesired)
	{
		m_pDetour_CTerrorPlayer_UpdateTeamDesired->Destroy();
		m_pDetour_CTerrorPlayer_UpdateTeamDesired = nullptr;
	}

	if (m_pDetour_CDirector_IsHumanSpectatorValid)
	{
		m_pDetour_CDirector_IsHumanSpectatorValid->Destroy();
		m_pDetour_CDirector_IsHumanSpectatorValid = nullptr;
	}

	playerhelpers->RemoveClientListener(this);
}

void CDirectorAvatarExt::SDK_OnAllLoaded()
{
	SM_GET_LATE_IFACE(SDKTOOLS, m_pSDKTools);
	SM_GET_LATE_IFACE(IMATCHEXT, imatchext);
}

bool CDirectorAvatarExt::SDK_OnMetamodLoad(ISmmAPI * ismm, char * error, size_t maxlen, bool late)
{
	GET_V_IFACE_CURRENT(GetServerFactory, serverGameClients, IServerGameClients, INTERFACEVERSION_SERVERGAMECLIENTS);
	GET_V_IFACE_CURRENT(GetEngineFactory, g_pCVar, ICvar, CVAR_INTERFACE_VERSION);

	SH_ADD_HOOK(IServerGameClients, ClientCommandKeyValues, serverGameClients, SH_MEMBER(this, &CDirectorAvatarExt::ClientCommandKeyValues), false);

	return true;
}

bool CDirectorAvatarExt::SDK_OnMetamodUnload(char *error, size_t maxlength)
{
	SH_REMOVE_HOOK(IServerGameClients, ClientCommandKeyValues, serverGameClients, SH_MEMBER(this, &CDirectorAvatarExt::ClientCommandKeyValues), false);

	return true;
}

bool CDirectorAvatarExt::QueryRunning(char *error, size_t maxlength)
{
	SM_CHECK_IFACE(SDKTOOLS, m_pSDKTools);
	SM_CHECK_IFACE(IMATCHEXT, imatchext);

	return true;
}

void CDirectorAvatarExt::NotifyInterfaceDrop(SMInterface *pInterface)
{
	SDK_OnUnload();
}

// bug#8: Players that would join the server through matchmaking could cause others to lose their spots if they load faster than them, after level transitions
void CDirectorAvatarExt::ClientCommandKeyValues(edict_t *pEntity, KeyValues *pKeyValues)
{
	if (!pKeyValues)
	{
		RETURN_META(MRES_IGNORED);
	}

	if (!pEntity)
	{
		RETURN_META(MRES_IGNORED);
	}

	IGamePlayer *pGamePlayer = playerhelpers->GetGamePlayer(pEntity);

	// Only intercept upon client connection
	if (pGamePlayer->IsConnected())
	{
		RETURN_META(MRES_IGNORED);
	}

	const char *pszName = pKeyValues->GetName();

	if (!V_stricmp(pszName, "avatarinfo"))
	{
		int iAvatarTeam = TEAM_UNASSIGNED;
		SurvivorCharacterType eAvatarSurvivorCharacter = SurvivorCharacter_Unknown;

		PlayerAvatarUnpack(pKeyValues, iAvatarTeam, eAvatarSurvivorCharacter);

		static ConVarRef survivor_limit("survivor_limit");
		static ConVarRef z_max_player_zombies("z_max_player_zombies");

		int nPlayers = 0;
		int nPlayerLimit = 0;

		bool bApplyChanges = false;

		switch (iAvatarTeam)
		{
			case TEAM_SURVIVOR:
			{
				// We're not connected yet, so leave this as it is
				nPlayers = CountTransitioningPlayers(TEAM_SURVIVOR);

				if (AreTeamsFlipped())
				{
					nPlayers += CountHumanPlayersInGame(TEAM_ZOMBIE);
					nPlayerLimit = z_max_player_zombies.GetInt();
				}
				else
				{
					nPlayers += CountHumanPlayersInGame(TEAM_SURVIVOR);
					nPlayerLimit = survivor_limit.GetInt();
				}

				if (nPlayers >= nPlayerLimit)
				{
					bApplyChanges = true;
				}

				break;
			}

			case TEAM_ZOMBIE:
			{
				// We're not connected yet, so leave this as it is
				nPlayers = CountTransitioningPlayers(TEAM_ZOMBIE);

				if (AreTeamsFlipped())
				{
					nPlayers += CountHumanPlayersInGame(TEAM_SURVIVOR);
					nPlayerLimit = survivor_limit.GetInt();
				}
				else
				{
					nPlayers += CountHumanPlayersInGame(TEAM_ZOMBIE);
					nPlayerLimit = z_max_player_zombies.GetInt();
				}

				if (nPlayers >= nPlayerLimit)
				{
					bApplyChanges = true;
				}

				break;
			}
		}

		if (bApplyChanges)
		{
			// Let the Director decide which team they should join once they've joined the game
			pKeyValues->SetString("team", "");
			pKeyValues->SetString("avatar", "");

			RETURN_META_NEWPARAMS(MRES_HANDLED, &IServerGameClients::ClientCommandKeyValues, (pEntity, pKeyValues));
		}
	}

	RETURN_META(MRES_IGNORED);
}

void CDirectorAvatarExt::OnServerActivated(int max_clients)
{
	m_iMaxPlayers = max_clients;
}

bool CDirectorAvatarExt::HasAvailableUnusedSurvivorAvatar(KeyValues *pAvatarInfo) const
{
	int iAvatarTeam = TEAM_UNASSIGNED;
	SurvivorCharacterType eAvatarSurvivorCharacter = SurvivorCharacter_Unknown;

	PlayerAvatarUnpack(pAvatarInfo, iAvatarTeam, eAvatarSurvivorCharacter);

	if (eAvatarSurvivorCharacter == SurvivorCharacter_Unknown)
	{
		return false;
	}

	CUtlVector<SurvivorCharacterType> vecAvatars;
	CollectAvailableAvatars(&vecAvatars, pAvatarInfo);

	if (vecAvatars.Find(eAvatarSurvivorCharacter) != vecAvatars.InvalidIndex())
	{
		return true;
	}

	return false;
}

int CDirectorAvatarExt::CountHumanPlayersInGame(int iTeamNum) const
{
	int nPlayers = 0;

	for (int iClient = 1; iClient <= m_iMaxPlayers; iClient++)
	{
		IGamePlayer *pGamePlayer = playerhelpers->GetGamePlayer(iClient);

		if (!pGamePlayer
			|| !pGamePlayer->IsConnected()
			|| !pGamePlayer->IsInGame()
			|| pGamePlayer->IsFakeClient())
		{
			continue;
		}

		IPlayerInfo *pPlayerInfo = pGamePlayer->GetPlayerInfo();

		if (!pPlayerInfo)
		{
			continue;
		}

		if (pPlayerInfo->GetTeamIndex() != iTeamNum)
		{
			continue;
		}

		nPlayers++;
	}

	return nPlayers;
}

int CDirectorAvatarExt::CountTransitioningPlayers(int iTeamNum) const
{
	int nClients = 0;

	for (int iClient = 1; iClient <= m_iMaxPlayers; iClient++)
	{
		IGamePlayer *pGamePlayer = playerhelpers->GetGamePlayer(iClient);

		if (!pGamePlayer)
		{
			continue;
		}

		if (!pGamePlayer->IsConnected())
		{
			continue;
		}

		IPlayerInfo *pPlayerInfo = pGamePlayer->GetPlayerInfo();

		if (pPlayerInfo && pPlayerInfo->GetTeamIndex() != TEAM_UNASSIGNED)
		{
			continue;
		}

		edict_t *pEdict = gamehelpers->EdictOfIndex(iClient);

		KeyValues *pAvatarInfo = PlayerAvatarGet(pEdict);

		int iAvatarTeam = TEAM_UNASSIGNED;
		SurvivorCharacterType eAvatarSurvivorCharacter = SurvivorCharacter_Unknown;

		PlayerAvatarUnpack(pAvatarInfo, iAvatarTeam, eAvatarSurvivorCharacter);

		if (AreTeamsFlipped())
		{
			switch (iAvatarTeam)
			{
				case TEAM_SURVIVOR:
				{
					iAvatarTeam = TEAM_ZOMBIE;
					break;
				}

				case TEAM_ZOMBIE:
				{
					iAvatarTeam = TEAM_SURVIVOR;
					break;
				}
			}
		}

		if (iAvatarTeam != iTeamNum)
		{
			continue;
		}

		nClients++;
	}

	return nClients;
}

int CDirectorAvatarExt::CollectAvatars(CUtlVector<SurvivorCharacterType> *pAvatarVector, KeyValues *pSkipAvatarInfo) const
{
	for (int iClient = 1; iClient <= m_iMaxPlayers; iClient++)
	{
		IGamePlayer *pGamePlayer = playerhelpers->GetGamePlayer(iClient);

		if (!pGamePlayer)
		{
			continue;
		}

		if (!pGamePlayer->IsConnected())
		{
			continue;
		}

		edict_t *pEdict = gamehelpers->EdictOfIndex(iClient);

		KeyValues *pAvatarInfo = PlayerAvatarGet(pEdict);

		if (pAvatarInfo == pSkipAvatarInfo)
		{
			continue;
		}

		int iAvatarTeam = TEAM_UNASSIGNED;
		SurvivorCharacterType eAvatarSurvivorCharacter = SurvivorCharacter_Unknown;

		PlayerAvatarUnpack(pAvatarInfo, iAvatarTeam, eAvatarSurvivorCharacter);

		if (iAvatarTeam != TEAM_SURVIVOR)
		{
			continue;
		}

		pAvatarVector->AddToTail(eAvatarSurvivorCharacter);
	}

	return pAvatarVector->Count();
}

int CDirectorAvatarExt::CollectAvailableAvatars(CUtlVector<SurvivorCharacterType> *pAvatarVector, KeyValues *pSkipAvatarInfo) const
{
	CUtlVector<SurvivorCharacterType> vecAvatars;
	CollectAvatars(&vecAvatars, pSkipAvatarInfo);

	bool bIsAvailable = false;

	// If we couldn't collect any avatars then all of them are available
	if (vecAvatars.Count() == 0)
	{
		bIsAvailable = true;
	}

	int i = static_cast<SurvivorCharacterType>(SurvivorCharacter_NamVet);

	while (i--)
	{
		SurvivorCharacterType eSurvivorCharacter = static_cast<SurvivorCharacterType>(i);

		FOR_EACH_VEC(vecAvatars, iter)
		{
			bIsAvailable = (vecAvatars[iter] != eSurvivorCharacter);

			if (!bIsAvailable)
			{
				break;
			}
		}

		if (bIsAvailable)
		{
			pAvatarVector->AddToTail(eSurvivorCharacter);
		}
	}

	return pAvatarVector->Count();
}

void CDirectorAvatarExt::SetRandomUnusedSurvivorAvatar(KeyValues *pAvatarInfo) const
{
	CUtlVector<SurvivorCharacterType> vecAvatars;
	CollectAvailableAvatars(&vecAvatars);

	pAvatarInfo->SetString("team", "Survivor");

	if (vecAvatars.Count() == 0)
	{
		DevWarning("Couldn't find any available Survivor avatar\n");

		return;
	}

	int iSurvivorCharacter = RandomInt(0, vecAvatars.Count() - 1);

	SurvivorCharacterType eRandSurvivorCharacter = vecAvatars.Element(iSurvivorCharacter);

	const char *pszName = GetNameFromCharacter(eRandSurvivorCharacter);

	pAvatarInfo->SetString("avatar", pszName);
}

void CDirectorAvatarExt::SetAvatarBasedOnCurrentTeam(CBaseEntity *pPlayer, KeyValues *pAvatarInfo) const
{
	int iClient = gamehelpers->EntityToBCompatRef(pPlayer);
	int iTeamNum = playerhelpers->GetGamePlayer(iClient)->GetPlayerInfo()->GetTeamIndex();

	switch (iTeamNum)
	{
		case TEAM_SPECTATOR:
		{
			pAvatarInfo->SetString("team", "Spectator");
			break;
		}

		// Ensure players end up on the right teams after level transitions when teams are flipped
		case TEAM_SURVIVOR:
		{
			if (AreTeamsFlipped())
			{
				pAvatarInfo->SetString("team", "Infected");
				pAvatarInfo->SetString("avatar", "infected");
			}
			else
			{
				SurvivorCharacterType eSurvivorCharacter = GetSurvivorCharacter(pPlayer);

				pAvatarInfo->SetString("team", "Survivor");
				pAvatarInfo->SetString("avatar", GetNameFromCharacter(eSurvivorCharacter));
			}

			break;
		}

		case TEAM_ZOMBIE:
		{
			if (AreTeamsFlipped())
			{
				if (HasAvailableUnusedSurvivorAvatar(pAvatarInfo))
				{
					break;
				}

				SetRandomUnusedSurvivorAvatar(pAvatarInfo);
			}
			else
			{
				pAvatarInfo->SetString("team", "Infected");
				pAvatarInfo->SetString("avatar", "infected");
			}

			break;
		}
	}
}

void CDirectorAvatarExt::PlayerAvatarUnpack(KeyValues *pAvatarInfo, int& iTeamNum, SurvivorCharacterType& eSurvivorCharacter) const
{
	iTeamNum = TEAM_UNASSIGNED;
	eSurvivorCharacter = SurvivorCharacter_Unknown;

	if (pAvatarInfo)
	{
		const char *pszTeamName = pAvatarInfo->GetString("team");

		iTeamNum = strtol(pszTeamName, nullptr, 10);

		if (iTeamNum == TEAM_UNASSIGNED)
		{
			if (!V_stricmp(pszTeamName, "Survivor"))
			{
				iTeamNum = TEAM_SURVIVOR;
			}
			else if (!V_stricmp(pszTeamName, "Infected"))
			{
				iTeamNum = TEAM_ZOMBIE;
			}
			else if (!V_stricmp(pszTeamName, "Spectator"))
			{
				iTeamNum = TEAM_SPECTATOR;
			}

			const char *pszAvatarName = pAvatarInfo->GetString("avatar");

			if (*pszAvatarName)
			{
				eSurvivorCharacter = GetCharacterFromName(pszAvatarName);
			}
		}
	}
}

KeyValues *CDirectorAvatarExt::PlayerAvatarGet(edict_t *pEdict) const
{
	uint64_t ullXuid = engine->GetClientXUID(pEdict);

	int iXuid = m_pTheDirector->m_Avatars.Find(ullXuid);

	if (iXuid != m_pTheDirector->m_Avatars.InvalidIndex())
	{
		return m_pTheDirector->m_Avatars.Element(iXuid);
	}

	return nullptr;
}

CBaseEntity *CDirectorAvatarExt::FindPlayerBySurvivorCharacter(SurvivorCharacterType eSurvivorCharacter) const
{
	for (int iClient = 1; iClient <= m_iMaxPlayers; iClient++)
	{
		IGamePlayer *pGamePlayer = playerhelpers->GetGamePlayer(iClient);

		if (!pGamePlayer
			|| !pGamePlayer->IsConnected()
			|| !pGamePlayer->IsInGame())
		{
			continue;
		}

		IPlayerInfo *pPlayerInfo = pGamePlayer->GetPlayerInfo();

		if (!pPlayerInfo)
		{
			continue;
		}

		if (pPlayerInfo->GetTeamIndex() != TEAM_SURVIVOR)
		{
			continue;
		}

		CBaseEntity *pPlayer = gamehelpers->ReferenceToEntity(iClient);

		if (GetSurvivorCharacter(pPlayer) != eSurvivorCharacter)
		{
			continue;
		}

		return pPlayer;
	}

	return nullptr;
}