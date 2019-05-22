/**
 * @file
 *
 * @brief Source for macaddr plugin
 *
 * @copyright BSD License (see LICENSE.md or https://www.libelektra.org)
 *
 */

#include "macaddr.h"

#include <assert.h>
#include <ctype.h>
#include <kdberrors.h>
#include <kdbhelper.h>
#include <kdbprivate.h>
#include <regex.h>
#include <stdio.h>

#define VALIDATION_SUCCESS 0
#define VALIDATION_ERROR 1
#define VALIDATION_ISINT 2

#define MAXMACINT 281474976710655

void transformMac (Key * key)
{
	const char * macKey = keyString (key);


	char * macWithoutSeparators = elektraMalloc (12);

	size_t len = strlen (macKey);
	size_t j = 0;
	for (size_t i = 0; i < len; ++i)
	{
		if (macKey[i] == ':' || macKey[i] == '-') continue;

		macWithoutSeparators[j++] = macKey[i];
	}

	keySetString (key, macWithoutSeparators);
}

int checkRegex (const char * mac, const char * regexString)
{
	regex_t regex;

	int reg = regcomp (&regex, regexString, REG_NOSUB | REG_EXTENDED | REG_NEWLINE);
	if (reg) return -1;

	reg = regexec (&regex, mac, 0, NULL, 0);
	regfree (&regex);

	return reg == REG_NOMATCH ? VALIDATION_ERROR : VALIDATION_SUCCESS;
}

int checkIntMac (const char * mac)
{
	if (strlen (mac) < 2) return 1;
	char * endptr;

	unsigned long long ret = 0;
	ret = strtoull (mac, &endptr, 10);

	if (errno == EINVAL || errno == ERANGE || *endptr != '\0') return VALIDATION_ERROR;
	if (ret > MAXMACINT || ret < 0) return VALIDATION_ERROR;

	return VALIDATION_ISINT;
}

int validateMac (Key * key)
{
	const Key * metaKey = keyGetMeta (key, "check/macaddr");
	if (!metaKey) return 1;

	const char * mac = keyString (key);

	const char * regexColon = "^([0-9A-Fa-f]{2}:){5}([0-9A-Fa-f]{2})$";
	const char * regexHyphen = "^([0-9A-Fa-f]{2}-){5}([0-9A-Fa-f]{2})$";
	const char * regexHyphenSingle = "^([0-9A-Fa-f]{6}-)([0-9A-Fa-f]{6})$";

	const char * regexStrings[] = { regexColon, regexHyphen, regexHyphenSingle };

	int ret;
	int i = 0;

	ret = checkIntMac (mac);

	while (ret == VALIDATION_ERROR && i < 3)
	{
		ret = checkRegex (mac, regexStrings[i]);
		++i;
	}

	return ret;
}

void transformToInt (Key * key)
{
	const char * mac = keyString (key);

	long intValue = strtol (mac, NULL, 16);

	const int n = snprintf (NULL, 0, "%lu", intValue);
	char * buffer = elektraMalloc (n + 1);
	snprintf (buffer, n + 1, "%lu", intValue);

	keySetString (key, buffer);
	elektraFree (buffer);
}

int elektraMacaddrGet (Plugin * handle ELEKTRA_UNUSED, KeySet * returned, Key * parentKey)
{
	if (!elektraStrCmp (keyName (parentKey), "system/elektra/modules/macaddr"))
	{
		KeySet * contract =
			ksNew (30, keyNew ("system/elektra/modules/macaddr", KEY_VALUE, "macaddr plugin waits for your orders", KEY_END),
			       keyNew ("system/elektra/modules/macaddr/exports", KEY_END),
			       keyNew ("system/elektra/modules/macaddr/exports/get", KEY_FUNC, elektraMacaddrGet, KEY_END),
			       keyNew ("system/elektra/modules/macaddr/exports/set", KEY_FUNC, elektraMacaddrSet, KEY_END),

#include ELEKTRA_README

			       keyNew ("system/elektra/modules/macaddr/infos/version", KEY_VALUE, PLUGINVERSION, KEY_END), KS_END);
		ksAppend (returned, contract);
		ksDel (contract);

		return ELEKTRA_PLUGIN_STATUS_SUCCESS;
	}
	ksRewind (returned);
	Key * cur;
	while ((cur = ksNext (returned)) != NULL)
	{
		const Key * meta = keyGetMeta (cur, "check/macaddr");
		if (!meta) continue;
		/*const Key * origValue = keyGetMeta (cur, "origvalue");
		if (origValue)
		{
			ELEKTRA_SET_ERRORF (
				ELEKTRA_ERROR_STATE, parentKey,
				"Meta key 'origvalue' for key %s not expected to be set, another plugin has already set this meta key!",
				keyString (cur));
			return ELEKTRA_PLUGIN_STATUS_ERROR;
		}*/
		int rc = validateMac (cur);
		if (rc == VALIDATION_ERROR)
		{
			ELEKTRA_SET_ERRORF (ELEKTRA_ERROR_INVALID_FORMAT, parentKey, "%s is not in a supported format.", keyString (cur));
			return ELEKTRA_PLUGIN_STATUS_ERROR;
		}

		if (rc != VALIDATION_ISINT)
		{
			transformMac (cur);
			transformToInt (cur);
		}
		return ELEKTRA_PLUGIN_STATUS_SUCCESS;
	}

	return ELEKTRA_PLUGIN_STATUS_NO_UPDATE;
}

int elektraMacaddrSet (Plugin * handle ELEKTRA_UNUSED, KeySet * returned, Key * parentKey)
{
	ksRewind (returned);
	Key * cur;
	while ((cur = ksNext (returned)) != NULL)
	{
		const Key * meta = keyGetMeta (cur, "check/macaddr");
		if (!meta) continue;

		const Key * origValue = keyGetMeta (cur, "origvalue");
		if (origValue)
		{
			keySetString (cur, keyString (origValue));
			return ELEKTRA_PLUGIN_STATUS_SUCCESS;
		}

		int rc = validateMac (cur);
		if (rc == VALIDATION_ERROR)
		{
			ELEKTRA_SET_ERRORF (ELEKTRA_ERROR_INVALID_FORMAT, parentKey,
					    "%s is not in a supported format. Supported formats are:\nXX:XX:XX:XX:XX:XX\n"
					    "XX-XX-XX-XX-XX-XX\nXXXXXX-XXXXXX",
					    keyString (cur));
			return ELEKTRA_PLUGIN_STATUS_ERROR;
		}

		keySetMeta (cur, "origvalue", keyString (cur));
		return ELEKTRA_PLUGIN_STATUS_SUCCESS;
	}

	return ELEKTRA_PLUGIN_STATUS_NO_UPDATE;
}

Plugin * ELEKTRA_PLUGIN_EXPORT
{
	// clang-format off
	return elektraPluginExport ("macaddr",
				    ELEKTRA_PLUGIN_GET,	&elektraMacaddrGet,
				    ELEKTRA_PLUGIN_SET,	&elektraMacaddrSet);
	// clang-format on
}
