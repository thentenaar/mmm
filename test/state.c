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
#include <CUnit/CUnit.h>
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

static char *xrow[] = {
	"1434730500",
	"1",
	"cur_rev",
	"prev_rev"
};

static char *xcolnames[] = {
	"tstamp",
	"version",
	"revision",
	"previous"
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
		CU_ASSERT_STRING_EQUAL_FATAL(query, expected_query);

	if (cb) {
		CU_ASSERT_EQUAL(cb, get_state_cb);
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
static void test_state_init_uninit(void)
{
	CU_ASSERT_EQUAL(0, state_init(0));
	CU_ASSERT_EQUAL(states_allocated, 1);
	CU_ASSERT_EQUAL(states_loaded, 0);

	state_uninit();
	CU_ASSERT_EQUAL(states_allocated, 0);
	CU_ASSERT_EQUAL(states_loaded, 0);

	CU_ASSERT_EQUAL(0, state_init(1));
	CU_ASSERT_EQUAL(states_allocated, 1);
	CU_ASSERT_EQUAL(states_loaded, 0);
	state_uninit();

	CU_ASSERT_EQUAL(0, state_init(N_STATES << 1));
	CU_ASSERT_EQUAL(states_allocated, N_STATES);
	CU_ASSERT_EQUAL(states_loaded, 0);
	state_uninit();
}

/**
 * Test that state_create() runs the correct query.
 */
static void test_state_create(void)
{
	expected_query = create_state_table;
	state_create();
}

/**
 * Test that state_get_current() returns NULL if no
 * space for states was allocated.
 */
static void state_get_current_null_states(void)
{
	state_uninit();
	CU_ASSERT_EQUAL(states_allocated, 0);
	CU_ASSERT_EQUAL(states_loaded, 0);
	CU_ASSERT_PTR_NULL(state_get_current());
}

/**
 * Test that state_get_current() returns the currently
 * allocated state if one exists.
 */
static void state_get_current_returns_loaded_state(void)
{
	state_init(1);
	states_loaded = 1;
	memcpy(states[0].revision, "XXX", 4);
	CU_ASSERT_PTR_EQUAL(state_get_current(), states[0].revision);
	states_loaded = 0;
	state_uninit();
}

/**
 * Test that state_get_current() returns NULL if there's no room
 * left in the array for a new state.
 */
static void state_get_current_no_room_for_new_state(void)
{
	state_init(1);
	set_states_loaded = states_allocated;
	expected_query = get_current_state;
	CU_ASSERT_PTR_NULL(state_get_current());
	state_uninit();

	state_init(1);
	set_states_loaded = states_allocated + 1;
	expected_query = get_current_state;
	CU_ASSERT_PTR_NULL(state_get_current());
	state_uninit();
}

/**
 * Test that state_get_current() fetches the current state if it
 * hasn't already been loaded.
 */
static void state_get_current_fetches_current_state(void)
{
	state_init(1);
	states_loaded = set_states_loaded = 0;
	expected_query = get_current_state;
	CU_ASSERT_PTR_EQUAL(state_get_current(), states[0].revision);
	CU_ASSERT_EQUAL(states[0].timestamp, tstamp);
	CU_ASSERT_EQUAL(states[0].version, STATE_VERSION);
	CU_ASSERT_STRING_EQUAL(states[0].revision, xrow[2]);
	CU_ASSERT_STRING_EQUAL(states[0].previous, xrow[3]);
	CU_ASSERT_EQUAL(states_loaded, 1);
	CU_ASSERT_EQUAL(states_allocated, 1);
	state_uninit();
}

/**
 * Test that state_get_previous() returns NULL if no
 * space for states was allocated.
 */
static void state_get_previous_null_states(void)
{
	state_uninit();
	CU_ASSERT_EQUAL(states_allocated, 0);
	CU_ASSERT_EQUAL(states_loaded, 0);
	CU_ASSERT_PTR_NULL(state_get_previous());
}

/**
 * Test that state_get_previous() returns the currently
 * allocated state's previous revision, if one exists.
 */
static void state_get_previous_returns_loaded_state(void)
{
	state_init(1);
	states_loaded = 1;
	memcpy(states[0].previous, "XXX", 4);
	CU_ASSERT_PTR_EQUAL(state_get_previous(), states[0].previous);
	states_loaded = 0;
	state_uninit();
}

/**
 * Test that state_cleanup_table() returns 0 if no states
 * have been allocated.
 */
static void state_cleanup_table_null_states(void)
{
	state_uninit();
	CU_ASSERT_EQUAL(0, state_cleanup_table());
}

/**
 * Test that state_cleanup_table() returns 0 if no states
 * have been loaded.
 */
static void state_cleanup_table_no_states_loaded(void)
{
	state_init(1);
	states_loaded = 0;
	CU_ASSERT_EQUAL(0, state_cleanup_table());
	state_uninit();
}

/**
 * Test that state_cleanup_table() returns 0 if too few
 * states have been loaded to justify a cleanup.
 */
static void state_cleanup_table_too_few_states_loaded(void)
{
	state_init(3);
	states_loaded = 2;
	CU_ASSERT_EQUAL(0, state_cleanup_table());
	state_uninit();
}

/**
 * Test that state_cleanup_table() uses the proper query to
 * remove old states.
 */
static void test_state_cleanup_table(void)
{
	char buf[100];

	state_init(1);
	states_loaded = 1;
	states[0].timestamp = tstamp;
	snprintf(buf, 50, "%s %ld;", delete_state, tstamp);
	expected_query = buf;
	CU_ASSERT_EQUAL(0, state_cleanup_table());
	state_uninit();
}

/**
 * Test that state_add_revision() handles invalid parameters.
 */
static void state_add_revision_invalid_params(void)
{
	CU_ASSERT_EQUAL(1, state_add_revision(NULL));
}

/**
 * Test that state_add_revision() handles an empty revision string.
 */
static void state_add_revision_empty_rev(void)
{
	const char *err = "state_add_revision: revision string empty\n";

	errbuf[0] = '\0';
	CU_ASSERT_EQUAL(1, state_add_revision(""));
	CU_ASSERT_NOT_EQUAL_FATAL(errbuf[0], '\0');
	CU_ASSERT_STRING_EQUAL(errbuf, err);
}

/**
 * Test that state_add_revision() handles a revision string that is
 * too long.
 */
static void state_add_revision_rev_too_long(void)
{
	const char *err = "state_add_revision: revision string too long\n";
	const char *rev = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
	                  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
	                  "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";

	CU_ASSERT_EQUAL(1, state_add_revision(rev));
	CU_ASSERT_NOT_EQUAL_FATAL(errbuf[0], '\0');
	CU_ASSERT_STRING_EQUAL(errbuf, err);
}

/**
 * Test that state_add_revision() correctly handles zero current
 * revisions.
 */
static void state_add_revision_zero_states(void)
{
	time_t timestamp;
	char buf[200];

	state_init(1);
	states_loaded = 0;
	timestamp = time(NULL);
	sprintf(buf, "%s (%ld,%ld,'%s','%s');", insert_state,
	        timestamp, STATE_VERSION, "xxx", states[0].revision);
	expected_query = buf;

	CU_ASSERT_EQUAL(0, state_add_revision("xxx"));
	CU_ASSERT_EQUAL(states_allocated, 1);
	CU_ASSERT_EQUAL(states[0].timestamp, timestamp);
	CU_ASSERT_EQUAL(states[0].version, STATE_VERSION);
	CU_ASSERT_NOT_EQUAL(states[0].timestamp, 0);
	CU_ASSERT_NOT_EQUAL_FATAL(states[0].revision[0], '\0');
	CU_ASSERT_EQUAL_FATAL(states[0].previous[0], '\0');
	CU_ASSERT_EQUAL(strlen(states[0].revision), 3);
	CU_ASSERT_STRING_EQUAL(states[0].revision, "xxx");
}

/**
 * Test that state_add_revision() correctly handles one current
 * revision.
 */
static void state_add_revision_one_state(void)
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

	CU_ASSERT_EQUAL(0, state_add_revision("xxx"));
	CU_ASSERT_EQUAL(states_allocated, 2);
	CU_ASSERT_EQUAL(states_loaded, 2);
	CU_ASSERT_EQUAL(states[0].timestamp, timestamp);
	CU_ASSERT_EQUAL(states[0].version, STATE_VERSION);
	CU_ASSERT_NOT_EQUAL(states[0].timestamp, 0);
	CU_ASSERT_NOT_EQUAL_FATAL(states[0].revision[0], '\0');
	CU_ASSERT_NOT_EQUAL_FATAL(states[0].previous[0], '\0');
	CU_ASSERT_EQUAL(strlen(states[0].revision), 3);
	CU_ASSERT_EQUAL(strlen(states[0].previous), 4);
	CU_ASSERT_STRING_EQUAL(states[0].revision, "xxx");
	CU_ASSERT_STRING_EQUAL(states[0].previous, "test");
	CU_ASSERT_NOT_EQUAL_FATAL(states[1].revision, '\0');
	CU_ASSERT_EQUAL(strlen(states[1].revision), 4);
	CU_ASSERT_STRING_EQUAL(states[1].revision, "test");
}

/**
 * Test that state_add_revision() correctly handles two current
 * revisions.
 */
static void state_add_revision_two_states(void)
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

	CU_ASSERT_EQUAL(0, state_add_revision("xxx"));
	CU_ASSERT_EQUAL(states_loaded, 3);
	CU_ASSERT_EQUAL(states[0].timestamp, timestamp);
	CU_ASSERT_EQUAL(states[0].version, STATE_VERSION);
	CU_ASSERT_NOT_EQUAL_FATAL(states[0].revision[0], '\0');
	CU_ASSERT_NOT_EQUAL_FATAL(states[0].previous[0], '\0');
	CU_ASSERT_EQUAL(strlen(states[0].revision), 3);
	CU_ASSERT_EQUAL(strlen(states[0].previous), 4);
	CU_ASSERT_STRING_EQUAL(states[0].revision, "xxx");
	CU_ASSERT_STRING_EQUAL(states[0].previous, "test");

	CU_ASSERT_NOT_EQUAL_FATAL(states[1].revision[0], '\0');
	CU_ASSERT_NOT_EQUAL_FATAL(states[1].previous[0], '\0');
	CU_ASSERT_EQUAL(strlen(states[1].revision), 4);
	CU_ASSERT_EQUAL(strlen(states[1].previous), 4);
	CU_ASSERT_STRING_EQUAL(states[1].revision, "test");
	CU_ASSERT_STRING_EQUAL(states[1].previous, "xxxx");

	CU_ASSERT_NOT_EQUAL_FATAL(states[2].revision[0], '\0');
	CU_ASSERT_EQUAL_FATAL(states[2].previous[0], '\0');
	CU_ASSERT_EQUAL(strlen(states[2].revision), 4);
	CU_ASSERT_EQUAL(strlen(states[2].previous), 0);
	CU_ASSERT_STRING_EQUAL(states[2].revision, "xxxx");
}

/**
 * Test that state_destroy() works.
 */
static void test_state_destroy(void)
{
	expected_query = drop_state;
	CU_ASSERT_EQUAL(0, state_destroy());
}

static CU_TestInfo state_tests[] = {
	{
		"state_init() / uninit() - works",
		test_state_init_uninit
	},
	{
		"state_create() - works",
		test_state_create
	},
	{
		"state_get_current() - null states",
		state_get_current_null_states
	},
	{
		"state_get_current() - returns loaded state",
		state_get_current_returns_loaded_state
	},
	{
		"state_get_current() - no room for new state",
		state_get_current_no_room_for_new_state
	},
	{
		"state_get_current() - fetches current state",
		state_get_current_fetches_current_state
	},
	{
		"state_get_previous() - null states",
		state_get_previous_null_states
	},
	{
		"state_get_previous() - returns loaded state",
		state_get_previous_returns_loaded_state
	},
	{
		"state_cleanup_table() - null states",
		state_cleanup_table_null_states
	},
	{
		"state_cleanup_table() - no states loaded",
		state_cleanup_table_no_states_loaded
	},
	{
		"state_cleanup_table() - too few states loaded",
		state_cleanup_table_too_few_states_loaded
	},
	{
		"state_cleanup_table() - works",
		test_state_cleanup_table
	},
	{
		"state_add_revision() - invalid params",
		state_add_revision_invalid_params
	},
	{
		"state_add_revision() - empty revision",
		state_add_revision_empty_rev
	},
	{
		"state_add_revision() - revision too long",
		state_add_revision_rev_too_long
	},
	{
		"state_add_revision() - zero states",
		state_add_revision_zero_states
	},
	{
		"state_add_revision() - one state",
		state_add_revision_one_state
	},
	{
		"state_add_revision() - two states",
		state_add_revision_two_states
	},
	{
		"state_destroy() - works",
		test_state_destroy
	},

	CU_TEST_INFO_NULL
};

void state_add_suite(void)
{
	size_t i = 0;
	CU_pSuite suite;

	suite = CU_add_suite("State Management", NULL, NULL);
	while (state_tests[i].pName) {
		CU_add_test(suite, state_tests[i].pName,
		            state_tests[i].pTestFunc);
		i++;
	}
}

