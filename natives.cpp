#include "natives.h"

static cell_t PlayerAvatarGet(IPluginContext *pContext, const cell_t *params)
{
	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(params[1]);

	if (!pPlayer)
	{
		pContext->ReportError("Client index %d is invalid", params[1]);

		return 0;
	}

	if (!pPlayer->IsConnected())
	{
		pContext->ReportError("Client %d is not connected", params[1]);

		return 0;
	}

	KeyValues *pDest = smutils->ReadKeyValuesHandle(params[2]);

	if (!pDest)
	{
		pContext->ReportError("Invalid Handle");

		return 0;
	}

	KeyValues *pAvatarInfo = g_DirectorAvatarExt.PlayerAvatarGet(pPlayer->GetEdict());
	if (pAvatarInfo)
	{
		*pDest = *pAvatarInfo;
	}

	return pAvatarInfo != NULL;
}

static cell_t PlayerAvatarUnpack(IPluginContext *pContext, const cell_t *params)
{
	KeyValues *pAvatarInfo = smutils->ReadKeyValuesHandle(params[1]);

	if (!pAvatarInfo)
	{
		pContext->ReportError("Invalid Handle");

		return 0;
	}

	int iAvatarTeam = TEAM_UNASSIGNED;
	SurvivorCharacterType eAvatarSurvivorCharacter = SurvivorCharacter_Unknown;

	g_DirectorAvatarExt.PlayerAvatarUnpack(pAvatarInfo, iAvatarTeam, eAvatarSurvivorCharacter);

	cell_t *pTeam = NULL;
	if (pContext->LocalToPhysAddr(params[2], &pTeam) != SP_ERROR_NONE)
	{
		pTeam = NULL;
	}

	cell_t *pSurvivorCharacter = NULL;
	if (pContext->LocalToPhysAddr(params[3], &pSurvivorCharacter) != SP_ERROR_NONE)
	{
		pSurvivorCharacter = NULL;
	}

	*pTeam = iAvatarTeam;
	*pSurvivorCharacter = eAvatarSurvivorCharacter;

	return 1;
}

static cell_t SetAvatarBasedOnCurrentTeam(IPluginContext *pContext, const cell_t *params)
{
	IGamePlayer *pPlayer = playerhelpers->GetGamePlayer(params[1]);

	if (!pPlayer)
	{
		pContext->ReportError("Client index %d is invalid", params[1]);

		return 0;
	}

	if (!pPlayer->IsConnected())
	{
		pContext->ReportError("Client %d is not connected", params[1]);

		return 0;
	}

	KeyValues *pAvatarInfo = smutils->ReadKeyValuesHandle(params[2]);

	if (!pAvatarInfo)
	{
		pContext->ReportError("Invalid Handle");

		return 0;
	}

	g_DirectorAvatarExt.SetAvatarBasedOnCurrentTeam(gamehelpers->ReferenceToEntity(params[1]), pAvatarInfo);

	return 1;
}

const sp_nativeinfo_t g_Natives[] =
{
	{ "PlayerAvatarGet", PlayerAvatarGet },
	{ "PlayerAvatarUnpack", PlayerAvatarUnpack },
	{ "SetAvatarBasedOnCurrentTeam", SetAvatarBasedOnCurrentTeam },
	{ nullptr, nullptr },
};