/**
 * Minimal Migration Manager - State Management
 * Copyright (C) 2015 Tim Hentenaar.
 *
 * This code is licenced under the Simplified BSD License.
 * See the LICENSE file for details.
 */

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "db.h"
#include "state.h"
#include "stringbuf.h"
#include "utils.h"

/* Current version of the table schema */
#define STATE_VERSION 0x00000001L

/**
 * The statement which will create our tracking table.
 *
 * Note: INTEGER is expected to be big enough to
 * hold a time_t. This likely won't be an issue
 * till 2038.
 *
 * This statement should work on any RDBMS
 * conforming to SQL-92 (Transitional) or better.
 *
 */
static const char *create_state_table =
    "CREATE TABLE mmm_state(\n"
    "  tstamp    INTEGER      NOT NULL PRIMARY KEY,\n"
    "  version   INTEGER      NOT NULL,\n"
    "  revision  VARCHAR(50)  NOT NULL,\n"
    "  previous  VARCHAR(50)  NOT NULL\n" ");";

/**
 * SQL-92 compliant queries to manage the current state.
 */
static const char *get_current_state =
    "SELECT * FROM mmm_state ORDER BY tstamp DESC;";

static const char *insert_state =
    "INSERT INTO mmm_state(tstamp, version, revision, previous) "
    "VALUES";

static const char *delete_state = "DELETE FROM mmm_state WHERE tstamp <";

static const char *drop_state = "DROP TABLE mmm_state;";

/**
 * Structure representing a state record.
 */
struct state {
	time_t timestamp;
	long version;
	char revision[50];
	char previous[50];
};

/**
 * State records.
 */
#define N_STATES 10
static struct state states[N_STATES];
static size_t states_allocated = 0;
static size_t states_loaded = 0;

/**
 * \param[in] n_states Number of states to keep.
 * \return 0 on success, 1 on error.
 */
int state_init(size_t n_states)
{
	if (!n_states) n_states++;
	if (n_states > N_STATES) n_states = N_STATES;

	memset(&states, 0, sizeof(states));
	states_allocated = n_states;
	states_loaded = 0;
	return 0;
}

/**
 * Free previously-allocated states.
 */
void state_uninit(void)
{
	states_allocated = 0;
	states_loaded = 0;
	memset(&states, 0, sizeof(states));
}

/**
 * This callback expects one row containing the
 * current state.
 */
static int get_state_cb(void *UNUSED(userdata), int n_cols,
                        char **fields, char **column_names)
{
	int i, retval = 0;
	size_t len;
	struct state *state;

	/* Fill-in the next state */
	if (states_loaded >= states_allocated) {
		++retval;
		goto ret;
	} else state = &states[states_loaded++];

	for (i = 0; i < n_cols; i++) {
		if (!fields[i]) continue;

		if (!strcmp(column_names[i], "tstamp"))
			state->timestamp = strtol(fields[i], NULL, 10);

		if (!strcmp(column_names[i], "version"))
			state->version = strtol(fields[i], NULL, 10);

		if (!strcmp(column_names[i], "revision")) {
			len = strlen(fields[i]) + 1;
			if (len < sizeof(state->revision))
				memcpy(state->revision, fields[i], len);
		}

		if (!strcmp(column_names[i], "previous")) {
			len = strlen(fields[i]) + 1;
			if (len < sizeof(state->previous))
				memcpy(state->previous, fields[i], len);
		}
	}

ret:
	return retval;
}

/**
 * Create the state tracking table, assuming it
 * doesn't exist.
 *
 * \return 0 on success, non-zero on error.
 */
int state_create(void)
{
	return db_query(create_state_table, NULL, NULL);
}

/**
 * Fetch the current state from the database, and
 * return the current revision.
 *
 * \return A pointer to the current revision ID, or NULL
 *         if it could not be obtained.
 */
const char *state_get_current(void)
{
	const char *retval = NULL;

	if (states_loaded)
		goto ret;

	if (!states_allocated)
		goto err;

	if (db_query(get_current_state, get_state_cb, NULL))
		goto err;

ret:
	retval = states[0].revision;
err:
	return retval;
}

/**
 * If we've read state data from the database, return
 * the previous revision.
 *
 * \return A pointer to the previous revision ID, or NULL
 *         if it could not be obtained.
 */
const char *state_get_previous(void)
{
	const char *retval = NULL;

	if (state_get_current())
		retval = states[0].previous;
	return retval;
}

/**
 * Delete any states older than the oldest revision we've
 * fetched.
 *
 * \return 0 on success, non-zero on error.
 */
int state_cleanup_table(void)
{
	int retval = 0;

	if (!states_loaded)
		goto ret;

	if (states_loaded < states_allocated)
		goto ret;

	sbuf_reset(0);
	if (sbuf_add_str(delete_state, SBUF_TSPACE, 0) ||
	    sbuf_add_snum(states[states_allocated - 1].timestamp,
	                  SBUF_SCOLON)) {
		ERROR("Unable to build DELETE query");
		++retval;
	} else retval = db_query(sbuf_get_buffer(), NULL, NULL);

ret:
	return retval;
}

/**
 * Add a new revision to the state table.
 *
 * \param[in] rev  Current local revision
 * \return 0 on success, non-zero on error.
 */
int state_add_revision(const char *rev)
{
	size_t i;
	int retval = 0;

	if (!rev) goto err;
	i = strlen(rev);

	if (!i) {
		ERROR("revision string empty");
		goto err;
	}

	if (i >= sizeof(states[0].revision)) {
		ERROR("revision string too long");
		goto err;
	}

	/* Move any existing states down */
	if (++states_loaded > states_allocated)
		states_loaded = states_allocated;
	memmove(&states[1], &states[0],
	        states_loaded * sizeof(struct state));

	/* Setup the new state */
	memmove(states[0].revision, rev, i + 1);
	memcpy(states[0].previous, &states[1].revision,
	       sizeof(states[0].revision));
	states[0].timestamp = time(NULL);
	states[0].version = STATE_VERSION;

	/* ... build our insert query ... */
	sbuf_reset(0);
	if (sbuf_add_str(insert_state, SBUF_TSPACE, 0)
	    || sbuf_add_snum(states[0].timestamp,
	                     SBUF_LPAREN | SBUF_COMMA)
	    || sbuf_add_snum(states[0].version, SBUF_COMMA)
	    || sbuf_add_str(states[0].revision,
	                    SBUF_QUOTE | SBUF_COMMA, 0)
	    || sbuf_add_str(states[0].previous,
	                    SBUF_QUOTE | SBUF_RPAREN | SBUF_SCOLON, 0)) {
		ERROR("Unable to build insert query");
		++retval;
	} else {
		/* and send it to the database. */
		retval = db_query(sbuf_get_buffer(), NULL, NULL);
	}

ret:
	return retval;

err:
	++retval;
	goto ret;
}

/**
 * Drop the state table.
 *
 * \return 0 on success, non-zero on error.
 */
int state_destroy(void)
{
	return db_query(drop_state, NULL, NULL);
}
