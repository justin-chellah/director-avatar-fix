#pragma once
 
#include <IShareSys.h>
 
#define SMINTERFACE_DIRECTORAVATARFIX_NAME			"IDirectorAvatarFix"
#define SMINTERFACE_DIRECTORAVATARFIX_VERSION		11
 
namespace SourceMod
{
	class IDirectorAvatarFix : public SMInterface
	{
	public:
		virtual const char *GetInterfaceName()
		{
			return SMINTERFACE_DIRECTORAVATARFIX_NAME;
		}
		virtual unsigned int GetInterfaceVersion()
		{
			return SMINTERFACE_DIRECTORAVATARFIX_VERSION;
		}
	public:
		virtual int CollectAvailableAvatars(CUtlVector<SurvivorCharacterType> *pAvatarVector, KeyValues *pSkipAvatarInfo = NULL) const = 0;

		virtual void PlayerAvatarUnpack(KeyValues *pAvatarInfo, int& iTeamNum, SurvivorCharacterType& eSurvivorCharacter) const = 0;

		virtual KeyValues *PlayerAvatarGet(edict_t *pEdict) const = 0;
	};
}