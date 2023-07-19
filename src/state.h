/**
 * \file state.h
 *
 * Minimal Migration Manager - State Management
 * Copyright (C) 2015 Tim Hentenaar.
 *
 * This code is licenced under the Simplified BSD License.
 * See the LICENSE file for details.
 */
#ifndef STATE_H
#define STATE_H

#include "db.h"

/**
 * \param[in] n_states Number of states to keep.
 * \return 0 on success, 1 on failure.
 */
int state_init(size_t n_states);

/**
 * Free previously-allocated states.
 */
void state_uninit(void);

/**
 * Create the state tracking table, assuming it
 * doesn't exist.
 *
 * \return 0 on success, non-zero on error.
 */
int state_create(void);

/**
 * Fetch the current state from the database, and
 * return the current revision.
 *
 * \return A pointer to the current revision ID, or NULL
 *         if it could not be obtained.
 */
const char *state_get_current(void);

/**
 * If we've read state data from the database, return
 * the previous revision.
 *
 * \return A pointer to the previous revision ID, or NULL
 *         if it could not be obtained.
 */
const char *state_get_previous(void);

/**
 * Delete any states older than the oldest revision we've
 * fetched.
 *
 * \return 0 on success, non-zero on error.
 */
int state_cleanup_table(void);

/**
 * Add a new revision to the state table.
 *
 * \param[in] rev Current local revision
 * \return 0 on success, non-zero on error.
 */
int state_add_revision(const char *rev);

/**
 * Drop the state table.
 *
 * \return 0 on success, non-zero on error.
 */
int state_destroy(void);

#endif /* STATE_H */
