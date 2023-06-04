#pragma once

#include "smsdk_ext.h"
#include "wrappers.h"
#include "IDirectorAvatarFix.h"
#include <ISDKTools.h>
#include <iplayerinfo.h>
#include <shareddefs.h>

class CDetour;

class CDirectorAvatarExt : public SDKExtension, public IDirectorAvatarFix, public IClientListener
{
public:
	/**
	 * @brief This is called after the initial loading sequence has been processed.
	 *
	 * @param error		Error message buffer.
	 * @param maxlen	Size of error message buffer.
	 * @param late		Whether or not the module was loaded after map load.
	 * @return			True to succeed loading, false to fail.
	 */
	virtual bool SDK_OnLoad(char *error, size_t maxlen, bool late) override;

	/**
	 * @brief This is called right before the extension is unloaded.
	 */
	virtual void SDK_OnUnload() override;

	/**
	 * @brief This is called once all known extensions have been loaded.
	 * Note: It is is a good idea to add natives here, if any are provided.
	 */
	virtual void SDK_OnAllLoaded() override;

#if defined SMEXT_CONF_METAMOD
	/**
	 * @brief Called when Metamod is attached, before the extension version is called.
	 *
	 * @param error			Error buffer.
	 * @param maxlen		Maximum size of error buffer.
	 * @param late			Whether or not Metamod considers this a late load.
	 * @return				True to succeed, false to fail.
	 */
	virtual bool SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlen, bool late) override;

	/**
	 * @brief Called when Metamod is detaching, after the extension version is called.
	 * NOTE: By default this is blocked unless sent from SourceMod.
	 *
	 * @param error			Error buffer.
	 * @param maxlen		Maximum size of error buffer.
	 * @return				True to succeed, false to fail.
	 */
	virtual bool SDK_OnMetamodUnload(char *error, size_t maxlength) override;
#endif

	/**
	 * @brief Return false to tell Core that your extension should be considered unusable.
	 *
	 * @param error				Error buffer.
	 * @param maxlength			Size of error buffer.
	 * @return					True on success, false otherwise.
	 */
	virtual bool QueryRunning(char *error, size_t maxlen) override;

	/**
	 * @brief Notifies the extension that an external interface it uses is being removed.
	 *
	 * @param pInterface		Pointer to interface being dropped.  This
	 * 							pointer may be opaque, and it should not
	 *							be queried using SMInterface functions unless
	 *							it can be verified to match an existing
	 */
	virtual void NotifyInterfaceDrop(SMInterface *pInterface) override;

	/**
	 * @brief Called when the server is activated.
	 */
	virtual void OnServerActivated(int max_clients) override;

	// The client has submitted a keyvalues command
	void ClientCommandKeyValues(edict_t *pEntity, KeyValues *pKeyValues);

	FORCEINLINE bool AreTeamsFlipped() const
	{ 
		return *(reinterpret_cast<char *>(m_pSDKTools->GetGameRules()) + m_nOffset_CTerrorGameRulesProxy_m_bAreTeamsFlipped); 
	};

	bool HasAvailableUnusedSurvivorAvatar(KeyValues *pAvatarInfo) const;

	int CountHumanPlayersInGame(int iTeamNum) const;
	int CountTransitioningPlayers(int iTeamNum) const;
	int CollectAvatars(CUtlVector<SurvivorCharacterType> *pAvatarVector, KeyValues *pSkipAvatarInfo = nullptr) const;
	virtual int CollectAvailableAvatars(CUtlVector<SurvivorCharacterType> *pAvatarVector, KeyValues *pSkipAvatarInfo = nullptr) const override;
	
	FORCEINLINE int GetMaxPlayers() const 
	{
		return m_iMaxPlayers;
	}

	FORCEINLINE SurvivorCharacterType GetSurvivorCharacter(CBaseEntity *pPlayer) const
	{ 
		return *reinterpret_cast<SurvivorCharacterType *>(reinterpret_cast<char *>(pPlayer) + m_nOffset_CTerrorPlayer_m_survivorCharacter); 
	};

	void SetRandomUnusedSurvivorAvatar(KeyValues *pAvatarInfo) const;
	void SetAvatarBasedOnCurrentTeam(CBaseEntity *pPlayer, KeyValues *pAvatarInfo) const;
	virtual void PlayerAvatarUnpack(KeyValues *pAvatarInfo, int& iTeamNum, SurvivorCharacterType& eSurvivorCharacter) const override;

	virtual KeyValues *PlayerAvatarGet(edict_t *pEdict) const override;

	CBaseEntity *FindPlayerBySurvivorCharacter(SurvivorCharacterType eSurvivorCharacter) const;

private:
	ISDKTools *m_pSDKTools = nullptr;

	int m_iMaxPlayers = 0;

	CDetour *m_pDetour_CDirector_JoinNewPlayer = nullptr;
	CDetour *m_pDetour_CDirector_NewPlayerFindAndPossessBot = nullptr;
	CDetour *m_pDetour_CTerrorPlayer_UpdateTeamDesired = nullptr;

	CDirector *m_pTheDirector = nullptr;

	int m_nOffset_CTerrorPlayer_m_survivorCharacter = -1;
	int m_nOffset_CTerrorGameRulesProxy_m_bAreTeamsFlipped = -1;

public:
	CDetour *m_pDetour_CDirector_GetPlayerCount = nullptr;
	CDetour *m_pDetour_CDirector_IsHumanSpectatorValid = nullptr;
};