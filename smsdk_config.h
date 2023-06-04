#pragma once

#define SMEXT_CONF_NAME				"[L4D2] Director Avatar Fix"
#define SMEXT_CONF_DESCRIPTION		"Fixes various issues related to avatars such as those which cause players to occasionally end up on the opposite team after map changes"
#define SMEXT_CONF_VERSION			"1.11.0"
#define SMEXT_CONF_AUTHOR			"Justin \"Sir Jay\" Chellah"
#define SMEXT_CONF_URL				"https://justin-chellah.com"
#define SMEXT_CONF_LOGTAG			"L4D2-DAF"
#define SMEXT_CONF_LICENSE			"MIT"
#define SMEXT_CONF_DATESTRING		__DATE__
#define SMEXT_CONF_GAMEDATA_FILE	"director_avatar_fix.l4d2"

#define SMEXT_LINK(name) 			SDKExtension *g_pExtensionIface = name;

#define SMEXT_CONF_METAMOD

#define SMEXT_ENABLE_GAMECONF
#define SMEXT_ENABLE_PLAYERHELPERS
#define SMEXT_ENABLE_GAMEHELPERS