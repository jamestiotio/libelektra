/**
 * @file
 *
 * @brief Defines for record.
 *
 * @copyright BSD License (see doc/COPYING or https://www.libelektra.org)
 */
#ifndef ELEKTRA_KDBRECORD_H
#define ELEKTRA_KDBRECORD_H

#include <kdb.h>
#include <kdbplugin.h>
#include <kdbtypes.h>
#include <kdbdiff.h>

#ifdef __cplusplus
namespace ckdb
{
extern "C" {
#endif

bool elektraRecordEnableRecording (KDB * handle, const Key * parentKey, Key * errorKey);
bool elektraRecordDisableRecording (KDB * handle, Key * errorKey);
bool elektraRecordClearSession (KDB * handle, Key * errorKey);
bool elektraRecordExportSession (KDB * handle, Plugin * plugin, Key * parentKey, Key * errorKey);

bool elektraRecordRecord (KDB * handle, KDB * sessionStorageHandle, KeySet * newKeys, Key * parentKey, Key * errorKey);
bool elektraRecordUndo (KDB * handle, KDB * sessionStorageHandle, Key * parentKey, Key * errorKey);
bool elektraRecordRemoveKey (KDB * handle, Key * toRemove, Key * errorKey);
bool elektraRecordGetDiff (KDB * handle, ElektraDiff ** diff, Key * errorKey);
bool elektraRecordIsActive (KDB * handle);

#ifdef __cplusplus
}
}
#endif


#endif // ELEKTRA_KDBRECORD_H
