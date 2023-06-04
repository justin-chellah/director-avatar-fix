#if defined _director_avatar_fix_included
 #endinput
#endif
#define _director_avatar_fix_included

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

/**
 * Retrieves avatar information of the player.
 *
 * @param iClient		Client index.
 * @param hAvatarInfo	KeyValues handle to store avatar information.
 *
 * @return				True on success, false otherwise.
 */
native bool PlayerAvatarGet(int iClient, KeyValues hAvatarInfo);

/**
 * Retrieves team and character type of the player from avatar KeyValues handle.
 *
 * @param hAvatarInfo				KeyValues handle with avatar information.
 * @param iAvatarTeam				Team index.
 * @param eAvatarSurvivorCharacter	Survivor character type.
 *
 * @noreturn
 */
native void PlayerAvatarUnpack(KeyValues hAvatarInfo, int& iAvatarTeam, SurvivorCharacterType& eAvatarSurvivorCharacter);

/**
 * Wrapper which sets avatar and team accordingly for the player, respecting whether teams were flipped.
 *
 * @param iClient					Client index.
 * @param hAvatarInfo				KeyValues handle with avatar information.
 *
 * @noreturn
 */
native void SetAvatarBasedOnCurrentTeam(int iClient, KeyValues hAvatarInfo);

public Extension __ext_director_avatar_fix =
{
	name = "[L4D2] Director Avatar Fix",
	file = "director_avatar_fix.ext",
#if defined AUTOLOAD_EXTENSIONS
	autoload = 1,
#else
	autoload = 0,
#endif
#if defined REQUIRE_EXTENSIONS
	required = 1,
#else
	required = 0,
#endif
};

#if !defined REQUIRE_EXTENSIONS
public void __ext_director_avatar_fix_SetNTVOptional()
{
	MarkNativeAsOptional("PlayerAvatarGet");
	MarkNativeAsOptional("PlayerAvatarUnpack");
	MarkNativeAsOptional("SetAvatarBasedOnCurrentTeam");
}
#endif