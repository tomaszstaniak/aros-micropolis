/*
 * Safe replacement of a save file with a freshly written temp file.
 *
 * Shared POSIX file operations on AROS (posixc) and host. Host tests force
 * rename/promotion/rollback errors; they do not emulate power loss or
 * prove filesystem crash durability. One writer per city path is required.
 * Existing recovery files are retained; numbered suffixes avoid overwriting
 * them. Naming is not a lock against other processes writing concurrently.
 *
 * Invariants this must keep (the two data-loss bugs found in review):
 * - never operate on aliased names: a path that would make tmp/bak
 *   truncate to the target's name is refused, not worked on;
 * - never delete an existing .bak before the new city is secured - after
 *   an earlier interrupted run, .bak may be the only surviving copy.
 */

#ifndef MICROPOLIS_SAVE_REPLACE_H
#define MICROPOLIS_SAVE_REPLACE_H

#include <cstddef>

/* Select unused .tmp[.N] and .bak[.N] names. Returns false without writes
 * if lookup fails, names are exhausted (1000 candidates), or buffers truncate.
 * The caller may write tmp only after success; existing recovery files stay. */
bool makeAuxNames(const char *path, char *tmp, size_t tmpSize,
                  char *bak, size_t bakSize);

/* Replace `path` with the complete new city in `tmp`.
 * feedback gets a human-readable outcome naming the ACTUAL location of
 * any preserved copy. Returns true when the new city replaced the old. */
bool replaceWithNewFile(const char *path, const char *tmp,
                        char *feedback, size_t feedbackSize);

#endif /* MICROPOLIS_SAVE_REPLACE_H */