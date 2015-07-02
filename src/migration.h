/**
 * \file migration.h
 *
 * Minimal Migration Manager - Migration Handling
 * Copyright (C) 2015 Tim Hentenaar.
 *
 * This code is licenced under the Simplified BSD License.
 * See the LICENSE file for details.
 */
#ifndef MIGRATION_H
#define MIGRATION_H

/**
 * Run the "up" portion of a migration.
 *
 * \param[in] path Migration to run
 * \return 0 on success, non-zero on failure.
 */
int migration_upgrade(const char *path);

/**
 * Run the "down" portion of a migration.
 *
 * \param[in] path Migration to run
 * \return 0 on success, non-zero on failure.
 */
int migration_downgrade(const char *path);

#endif /* MIGRATION_H */
