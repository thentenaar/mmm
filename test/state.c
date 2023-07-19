/**
 * Minimal Migration Manager - State Management Tests
 * Copyright (C) 2015 Tim Hentenaar.
 *
 * This code is licenced under the Simplified BSD License.
 * See the LICENSE file for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <check.h>
#include "tests.h"

/* from test_runner.c */
extern char errbuf[];

/* {{{ DB stubs */
typedef int (*db_row_callback_t)(void *userdata, int n_cols,
                                 char **fields, char **column_names);

static int db_query(const char *query, db_row_callback_t cb,
                    void *userdata);

#define DB_H
#include "../src/state.h"
#include "../src/state.c"

static const char *expected_query = NULL;

/**
 * Values to test state fetching.
 */
static const time_t tstamp = 1434730500;

static char xrow_0[] = "1434730500";
static char xrow_1[] = "1";
static char xrow_2[] = "cur_rev";
static char xrow_3[] = "prev_rev";

static char *xrow[] = {
	xrow_0, xrow_1, xrow_2, xrow_3
};

static char xcolnames_0[] = "tstamp";
static char xcolnames_1[] = "version";
static char xcolnames_2[] = "revision";
static char xcolnames_3[] = "previous";

static char *xcolnames[] = {
	xcolnames_0, xcolnames_1, xcolnames_2, xcolnames_3
};

static int xcols = 4;
static size_t set_states_loaded = 0;

/**
 * Database query stub
 */
static int db_query(const char *query, db_row_callback_t cb,
                    void *userdata)
{
	int retval = 1;

	if (!query) goto ret;

	/* Check the query */
	if (expected_query)
		ck_assert_str_eq(query, expected_query);

	if (cb) {
		ck_assert(cb == get_state_cb);
		if (set_states_loaded)
			states_loaded = set_states_loaded;
		retval = cb(userdata, xcols, xrow, xcolnames);
	} else retval = 0;

ret:
	return retval;
}
/* }}} */

/**
 * Test that state_init() and state_uninit() work.
 */
START_TEST(test_state_init_uninit)
{
	ck_assert_int_eq(state_init(0), 0);
	ck_assert_uint_eq(states_allocated, 1);
	ck_assert_uint_eq(states_loaded, 0);

	state_uninit();
	ck_assert_uint_eq(states_allocated, 0);
	ck_assert_uint_eq(states_loaded, 0);

	ck_assert_int_eq(state_init(1), 0);
	ck_assert_uint_eq(states_allocated, 1);
	ck_assert_uint_eq(states_loaded, 0);
	state_uninit();

	ck_assert_int_eq(state_init(N_STATES << 1), 0);
	ck_assert_uint_eq(states_allocated, N_STATES);
	ck_assert_uint_eq(states_loaded, 0);
	state_uninit();
}
END_TEST

/**
 * Test that state_create() runs the correct query.
 */
START_TEST(test_state_create)
{
	expected_query = create_state_table;
	state_create();
}
END_TEST

/**
 * Test that state_get_current() returns NULL if no
 * space for states was allocated.
 */
START_TEST(state_get_current_null_states)
{
	state_uninit();
	ck_assert_uint_eq(states_allocated, 0);
	ck_assert_uint_eq(states_loaded, 0);
	ck_assert_ptr_null(state_get_current());
}
END_TEST

/**
 * Test that state_get_current() returns the currently
 * allocated state if one exists.
 */
START_TEST(state_get_current_returns_loaded_state)
{
	state_init(1);
	states_loaded = 1;
	memcpy(states[0].revision, "XXX", 4);
	ck_assert_ptr_eq(state_get_current(), states[0].revision);
	states_loaded = 0;
	state_uninit();
}
END_TEST

/**
 * Test that state_get_current() returns NULL if there's no room
 * left in the array for a new state.
 */
START_TEST(state_get_current_no_room_for_new_state)
{
	state_init(1);
	set_states_loaded = states_allocated;
	expected_query = get_current_state;
	ck_assert_ptr_null(state_get_current());
	state_uninit();

	state_init(1);
	set_states_loaded = states_allocated + 1;
	expected_query = get_current_state;
	ck_assert_ptr_null(state_get_current());
	state_uninit();
}
END_TEST

/**
 * Test that state_get_current() fetches the current state if it
 * hasn't already been loaded.
 */
START_TEST(state_get_current_fetches_current_state)
{
	state_init(1);
	states_loaded = set_states_loaded = 0;
	expected_query = get_current_state;

	ck_assert_ptr_eq(state_get_current(), states[0].revision);
	ck_assert_int_eq(states[0].timestamp, tstamp);
	ck_assert_int_eq(states[0].version, STATE_VERSION);
	ck_assert_str_eq(states[0].revision, xrow[2]);
	ck_assert_str_eq(states[0].previous, xrow[3]);
	ck_assert_uint_eq(states_loaded, 1);
	ck_assert_uint_eq(states_allocated, 1);
	state_uninit();
}
END_TEST

/**
 * Test that state_get_previous() returns NULL if no
 * space for states was allocated.
 */
START_TEST(state_get_previous_null_states)
{
	state_uninit();
	ck_assert_uint_eq(states_allocated, 0);
	ck_assert_uint_eq(states_loaded, 0);
	ck_assert_ptr_null(state_get_previous());
}
END_TEST

/**
 * Test that state_get_previous() returns the currently
 * allocated state's previous revision, if one exists.
 */
START_TEST(state_get_previous_returns_loaded_state)
{
	state_init(1);
	states_loaded = 1;
	memcpy(states[0].previous, "XXX", 4);
	ck_assert_ptr_eq(state_get_previous(), states[0].previous);
	states_loaded = 0;
	state_uninit();
}
END_TEST

/**
 * Test that state_cleanup_table() returns 0 if no states
 * have been allocated.
 */
START_TEST(state_cleanup_table_null_states)
{
	state_uninit();
	ck_assert_int_eq(state_cleanup_table(), 0);
}
END_TEST

/**
 * Test that state_cleanup_table() returns 0 if no states
 * have been loaded.
 */
START_TEST(state_cleanup_table_no_states_loaded)
{
	state_init(1);
	states_loaded = 0;
	ck_assert_int_eq(state_cleanup_table(), 0);
	state_uninit();
}
END_TEST

/**
 * Test that state_cleanup_table() returns 0 if too few
 * states have been loaded to justify a cleanup.
 */
START_TEST(state_cleanup_table_too_few_states_loaded)
{
	state_init(3);
	states_loaded = 2;
	ck_assert_int_eq(state_cleanup_table(), 0);
	state_uninit();
}
END_TEST

/**
 * Test that state_cleanup_table() uses the proper query to
 * remove old states.
 */
START_TEST(test_state_cleanup_table)
{
	char buf[100];

	state_init(1);
	states_loaded = 1;
	states[0].timestamp = tstamp;
	sprintf(buf, "%s %ld;", delete_state, tstamp);
	expected_query = buf;
	ck_assert_int_eq(state_cleanup_table(), 0);
	state_uninit();
}
END_TEST

/**
 * Test that state_add_revision() handles invalid parameters.
 */
START_TEST(state_add_revision_invalid_params)
{
	ck_assert_int_ne(state_add_revision(NULL), 0);
}
END_TEST

/**
 * Test that state_add_revision() handles an empty revision string.
 */
START_TEST(state_add_revision_empty_rev)
{
	*errbuf = '\0';
	ck_assert_int_ne(state_add_revision(""), 0);
	ck_assert_str_eq(errbuf, "revision string empty\n");
}
END_TEST

/**
 * Test that state_add_revision() handles a revision string that is
 * too long.
 */
START_TEST(state_add_revision_rev_too_long)
{
	const char *rev = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
	                  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
	                  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";

	ck_assert_int_ne(state_add_revision(rev), 0);
	ck_assert_str_eq(errbuf, "revision string too long\n");
}
END_TEST

/**
 * Test that state_add_revision() correctly handles zero current
 * revisions.
 */
START_TEST(state_add_revision_zero_states)
{
	time_t timestamp;
	char buf[200];

	state_init(1);
	states_loaded = 0;
	timestamp = time(NULL);
	sprintf(buf, "%s (%ld,%ld,'%s','%s');", insert_state,
	        timestamp, STATE_VERSION, "xxx", states[0].revision);
	expected_query = buf;

	ck_assert_int_eq(state_add_revision("xxx"), 0);
	ck_assert_uint_eq(states_allocated, 1);
	ck_assert_int_eq(states[0].timestamp, timestamp);
	ck_assert_int_eq(states[0].version, STATE_VERSION);
	ck_assert_str_eq(states[0].revision, "xxx");
	ck_assert(!*states[0].previous);
}
END_TEST

/**
 * Test that state_add_revision() correctly handles one current
 * revision.
 */
START_TEST(state_add_revision_one_state)
{
	time_t timestamp;
	char buf[200];

	state_init(2);
	states_loaded = 1;
	memcpy(states[0].revision, "test", 5);
	timestamp = time(NULL);
	sprintf(buf, "%s (%ld,%ld,'%s','%s');", insert_state,
	        timestamp, STATE_VERSION, "xxx", states[0].revision);
	expected_query = buf;

	ck_assert_int_eq(state_add_revision("xxx"), 0);
	ck_assert_uint_eq(states_allocated, 2);
	ck_assert_uint_eq(states_loaded, 2);
	ck_assert_int_eq(states[0].timestamp, timestamp);
	ck_assert_int_eq(states[0].version, STATE_VERSION);
	ck_assert_str_eq(states[0].revision, "xxx");
	ck_assert_str_eq(states[0].previous, "test");
	ck_assert_str_eq(states[1].revision, "test");
}
END_TEST

/**
 * Test that state_add_revision() correctly handles two current
 * revisions.
 */
START_TEST(state_add_revision_two_states)
{
	time_t timestamp;
	char buf[200];

	state_init(3);
	states_loaded = 3;
	timestamp = time(NULL);
	states[0].timestamp = timestamp;
	states[0].version   = STATE_VERSION;
	states[1].timestamp = timestamp;
	states[1].version   = STATE_VERSION;
	memcpy(states[0].revision, "test", 5);
	memcpy(states[0].previous, "xxxx", 5);
	memcpy(states[1].revision, "xxxx", 5);
	sprintf(buf, "%s (%ld,%ld,'%s','%s');", insert_state,
	        timestamp, STATE_VERSION, "xxx", states[0].revision);
	expected_query = buf;

	ck_assert_int_eq(state_add_revision("xxx"), 0);
	ck_assert_uint_eq(states_loaded, 3);
	ck_assert_int_eq(states[0].timestamp, timestamp);
	ck_assert_int_eq(states[0].version, STATE_VERSION);
	ck_assert_str_eq(states[0].revision, "xxx");
	ck_assert_str_eq(states[0].previous, "test");
	ck_assert_str_eq(states[1].revision, "test");
	ck_assert_str_eq(states[1].previous, "xxxx");
	ck_assert_str_eq(states[2].revision, "xxxx");
	ck_assert(!*states[2].previous);
}
END_TEST

/**
 * Test that state_destroy() works.
 */
START_TEST(test_state_destroy)
{
	expected_query = drop_state;
	ck_assert_int_eq(state_destroy(), 0);
}
END_TEST

Suite *state_suite(void)
{
	Suite *s;
	TCase *t;

	s = suite_create("State Management");
	t = tcase_create("state_init/uninit");
	tcase_add_test(t, test_state_init_uninit);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	t = tcase_create("state_create");
	tcase_add_test(t, test_state_create);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	t = tcase_create("state_get_current");
	tcase_add_test(t, state_get_current_null_states);
	tcase_add_test(t, state_get_current_returns_loaded_state);
	tcase_add_test(t, state_get_current_no_room_for_new_state);
	tcase_add_test(t, state_get_current_fetches_current_state);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	t = tcase_create("state_get_previous");
	tcase_add_test(t, state_get_previous_null_states);
	tcase_add_test(t, state_get_previous_returns_loaded_state);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	t = tcase_create("state_cleanup_table");
	tcase_add_test(t, state_cleanup_table_null_states);
	tcase_add_test(t, state_cleanup_table_no_states_loaded);
	tcase_add_test(t, state_cleanup_table_too_few_states_loaded);
	tcase_add_test(t, test_state_cleanup_table);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	t = tcase_create("state_add_revision");
	tcase_add_test(t, state_add_revision_invalid_params);
	tcase_add_test(t, state_add_revision_empty_rev);
	tcase_add_test(t, state_add_revision_rev_too_long);
	tcase_add_test(t, state_add_revision_zero_states);
	tcase_add_test(t, state_add_revision_one_state);
	tcase_add_test(t, state_add_revision_two_states);
	tcase_set_timeout(t, 3);
	suite_add_tcase(s, t);

	t = tcase_create("state_destroy");
	tcase_add_test(t, test_state_destroy);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	return s;
}

