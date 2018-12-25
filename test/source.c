/**
 * Minimal Migration Manager - Migration Source Layer Tests
 * Copyright (C) 2015 Tim Hentenaar.
 *
 * This code is licenced under the Simplified BSD License.
 * See the LICENSE file for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <CUnit/CUnit.h>
#include "tests.h"

/* from test_runner.c */
extern char errbuf[];

#include "../src/source.h"
#include "../src/source.c"

/* {{{ Source backend stubs */
/**
 * This is a test-controllable instance of a source
 * backend, which allows us to track whether certain
 * functions were called, that they were passed the
 * proper params, etc.
 */
static int backend_init_called               = 0;
static int backend_uninit_called             = 0;
static int backend_find_migrations_called    = 0;
static int backend_get_head_called           = 0;
static int backend_get_migration_path_called = 0;

static int backend_init(void)
{
	backend_init_called++;

	/* Fail on the 2nd call */
	if (backend_init_called == 2)
		return 1;
	return 0;
}

static int backend_uninit(void)
{
	backend_uninit_called++;

	/* Fail on the 2nd call */
	if (backend_uninit_called == 2)
		return 1;
	return 0;
}

static void backend_config(void)
{
	return;
}

static char **backend_find_migrations(const char *cur_rev,
                                      const char *UNUSED(prev_rev),
                                      size_t *size)
{
	backend_find_migrations_called++;
	CU_ASSERT_TRUE(!!cur_rev);
	CU_ASSERT_TRUE(!!size);
	*size = 12345;
	return (char **)1234;
}

static const char *backend_get_head(void)
{
	backend_get_head_called++;
	return "head";
}

static const char *backend_get_migration_path(void)
{
	backend_get_migration_path_called++;
	return ".";
}

const struct source_backend_vtable backend_without_init = {
	"no-init",
	NULL, /* backend_config */
	NULL, /* backend_init */
	NULL, /* backend_find_migrations */
	NULL, /* backend_get_head  */
	NULL, /* backend_get_file_revision */
	NULL, /* backend_get_migration_path */
	NULL  /* backend_uninit, */
};

const struct source_backend_vtable backend_with_init = {
	"init",
	backend_config,
	backend_init,
	backend_find_migrations,
	backend_get_head,
	NULL,
	backend_get_migration_path,
	backend_uninit
};
/* }}} */

/**
 * Test that source_init() skips a backend without an
 * init callback.
 */
static void source_init_skips_backends_without_init(void)
{
	errbuf[0] = '\0';
	backend_init_called = 0;
	sources[0] = &backend_without_init;
	source_init();
	CU_ASSERT_TRUE(!!sources[0]);
	CU_ASSERT_EQUAL(errbuf[0], '\0');
	CU_ASSERT_FALSE(backend_init_called);
}

/**
 * That that we get the correct error message if source_init()
 * fails to initialize a backend.
 */
static void source_init_fails_to_init_backend(void)
{
	const char *err = "failed to initialize 'init'\n";
	errbuf[0] = '\0';
	sources[0] = &backend_with_init;
	backend_init_called = 1;
	source_init();
	CU_ASSERT_EQUAL(backend_init_called, 2);
	CU_ASSERT_FALSE(sources[0]);
	CU_ASSERT_NOT_EQUAL_FATAL(errbuf[0], '\0');
	CU_ASSERT_EQUAL_FATAL(strlen(errbuf), strlen(err));
	CU_ASSERT_STRING_EQUAL(errbuf, err);
}

/**
 * Test that source_get_config_cb() returns NULL if
 * passed invalid parameters.
 */
static void source_get_config_cb_invalid_params(void)
{
	memset(sources, 0, sizeof(sources));
	sources[0] = &backend_with_init;
	CU_ASSERT_TRUE(!source_get_config_cb(NULL, 1));
	CU_ASSERT_TRUE(!source_get_config_cb("init", 0));
}

/**
 * Test that source_get_config_cb() fails if no backends
 * are usable.
 */
static void source_get_config_cb_no_usable_backends(void)
{
	memset(sources, 0, sizeof(sources));
	CU_ASSERT_TRUE(!source_get_config_cb("init", 4));
}

/**
 * Test that source_get_config_cb() returns the specified
 * backend's config callback.
 */
static void test_source_get_config_cb(void)
{
	memset(sources, 0, sizeof(sources));
	sources[0] = &backend_with_init;
	sources[1] = &backend_without_init;
	CU_ASSERT_EQUAL(source_get_config_cb("init", 4), backend_config);
	CU_ASSERT_EQUAL(source_get_config_cb("no-init", 7), NULL);
}

/**
 * Test that source_find_migrations() returns NULL if
 * passed invalid parameters.
 */
static void source_find_migrations_invalid_params(void)
{
	size_t size = SIZE_MAX >> 1;
	const char *head = "head";
	const char *prev = "prev";

	backend_find_migrations_called = 0;
	memset(sources, 0, sizeof(sources));
	sources[0] = &backend_with_init;

	/* !source */
	CU_ASSERT_PTR_NULL(source_find_migrations(NULL, head, prev, &size));
	CU_ASSERT_EQUAL(size, 0);
	CU_ASSERT_FALSE(backend_find_migrations_called);

	/* !size */
	CU_ASSERT_PTR_NULL(source_find_migrations("init", head, prev, NULL));
	CU_ASSERT_EQUAL(size, 0);
	CU_ASSERT_FALSE(backend_find_migrations_called);
}

/**
 * Test that source_find_migrations() fails if no backends
 * are usable.
 */
static void source_find_migrations_no_usable_backends(void)
{
	size_t size = SIZE_MAX >> 1;
	const char *head = "head";

	backend_find_migrations_called = 0;
	memset(sources, 0, sizeof(sources));

	CU_ASSERT_PTR_NULL(source_find_migrations("init", head, NULL, &size));
	CU_ASSERT_EQUAL(size, 0);
	CU_ASSERT_FALSE(backend_find_migrations_called);
}

/**
 * Test that source_find_migrations() calls the specified
 * backend's callback, and returns the result.
 */
static void test_source_find_migrations(void)
{
	char **m;
	size_t size = SIZE_MAX >> 1;
	const char *head = "head";

	backend_find_migrations_called = 0;
	memset(sources, 0, sizeof(sources));
	sources[0] = &backend_without_init;
	sources[1] = &backend_with_init;

	m = source_find_migrations("init", head, NULL, &size);
	CU_ASSERT_EQUAL(m, (char **)1234);
	CU_ASSERT_EQUAL(size, 12345);
	CU_ASSERT_TRUE(backend_find_migrations_called);
}

/**
 * Test that source_get_local_head() returns NULL if
 * passed invalid parameters.
 */
static void source_get_local_head_invalid_params(void)
{
	backend_get_head_called = 0;
	memset(sources, 0, sizeof(sources));
	sources[0] = &backend_with_init;

	/* !source */
	CU_ASSERT_FALSE(source_get_local_head(NULL));
	CU_ASSERT_FALSE(source_get_local_head("\0"));
	CU_ASSERT_FALSE(backend_get_head_called);
}

/**
 * Test that source_get_local_head() fails if no backends
 * are usable.
 */
static void source_get_local_head_no_usable_backends(void)
{
	backend_get_head_called = 0;
	memset(sources, 0, sizeof(sources));
	CU_ASSERT_FALSE(source_get_local_head("init"));
	CU_ASSERT_FALSE(backend_get_head_called);
}

/**
 * Test that source_get_local_head() calls the specified
 * backend's callback, and returns the result.
 */
static void test_source_get_local_head(void)
{
	const char *head;
	backend_get_head_called = 0;
	memset(sources, 0, sizeof(sources));
	sources[0] = &backend_without_init;
	sources[1] = &backend_with_init;

	head = source_get_local_head("init");
	CU_ASSERT_TRUE_FATAL(!!head);
	CU_ASSERT_STRING_EQUAL(head, "head");
	CU_ASSERT_TRUE(backend_get_head_called);
}

/**
 * Test that source_get_migration_path() returns NULL if
 * passed invalid parameters.
 */
static void source_get_migration_path_invalid_params(void)
{
	backend_get_migration_path_called = 0;
	memset(sources, 0, sizeof(sources));
	sources[0] = &backend_with_init;

	/* !source */
	CU_ASSERT_FALSE(source_get_migration_path(NULL));
	CU_ASSERT_FALSE(source_get_migration_path("\0"));
	CU_ASSERT_FALSE(backend_get_migration_path_called);
}

/**
 * Test that source_get_migration_path() fails if no backends
 * are usable.
 */
static void source_get_migration_path_no_usable_backends(void)
{
	backend_get_migration_path_called = 0;
	memset(sources, 0, sizeof(sources));
	CU_ASSERT_FALSE(source_get_migration_path("init"));
	CU_ASSERT_FALSE(backend_get_migration_path_called);
}

/**
 * Test that source_get_migration_path() calls the specified
 * backend's callback, and returns the result.
 */
static void test_source_get_migration_path(void)
{
	const char *path;
	backend_get_migration_path_called = 0;
	memset(sources, 0, sizeof(sources));
	sources[0] = &backend_without_init;
	sources[1] = &backend_with_init;

	path = source_get_migration_path("init");
	CU_ASSERT_TRUE_FATAL(!!path);
	CU_ASSERT_STRING_EQUAL(path, ".");
	CU_ASSERT_TRUE(backend_get_migration_path_called);
}

/**
 * Test that source_uninit() skips a backend without an
 * uninit callback.
 */
static void source_uninit_skips_backends_without_uninit(void)
{
	errbuf[0] = '\0';
	backend_uninit_called = 0;
	memset(sources, 0, sizeof(sources));
	sources[0] = &backend_without_init;

	source_uninit();
	CU_ASSERT_TRUE(!!sources[0]);
	CU_ASSERT_EQUAL(errbuf[0], '\0');
	CU_ASSERT_FALSE(backend_uninit_called);
}

/**
 * That that we get the correct error message if source_uninit()
 * fails to uninitialize a backend.
 */
static void source_uninit_fails_to_uninit_backend(void)
{
	const char *err = "failed to uninitialize 'init'\n";

	errbuf[0] = '\0';
	memset(sources, 0, sizeof(sources));
	sources[1] = &backend_with_init;
	backend_uninit_called = 1;

	source_uninit();
	CU_ASSERT_EQUAL(backend_uninit_called, 2);
	CU_ASSERT_FALSE(sources[0]);
	CU_ASSERT_NOT_EQUAL_FATAL(errbuf[0], '\0');
	CU_ASSERT_EQUAL_FATAL(strlen(errbuf), strlen(err));
	CU_ASSERT_STRING_EQUAL(errbuf, err);
}

static CU_TestInfo source_tests[] = {
	{
		"source_init - skips backends without init()",
		source_init_skips_backends_without_init
	},
	{
		"source_init - fails to init backend",
		source_init_fails_to_init_backend
	},
	{
		"source_get_config_cb - invalid params",
		source_get_config_cb_invalid_params
	},
	{
		"source_get_config_cb - no usable backends",
		source_get_config_cb_no_usable_backends
	},
	{
		"source_get_config_cb works",
		test_source_get_config_cb
	},
	{
		"source_find_migrations - invalid params",
		source_find_migrations_invalid_params
	},
	{
		"source_find_migrations - no usable backends",
		source_find_migrations_no_usable_backends
	},
	{
		"source_find_migrations works",
		test_source_find_migrations
	},
	{
		"source_get_local_head - invalid params",
		source_get_local_head_invalid_params
	},
	{
		"source_get_local_head - no usable backends",
		source_get_local_head_no_usable_backends
	},
	{
		"source_get_local_head works",
		test_source_get_local_head
	},
	{
		"source_get_migration_path - invalid params",
		source_get_migration_path_invalid_params
	},
	{
		"source_get_migration_path - no usable backends",
		source_get_migration_path_no_usable_backends
	},
	{
		"source_get_migration_path works",
		test_source_get_migration_path
	},
	{
		"source_uninit - skips backends without uninit()",
		source_uninit_skips_backends_without_uninit
	},
	{
		"source_uninit - fails to uninit backend",
		source_uninit_fails_to_uninit_backend
	},

	CU_TEST_INFO_NULL
};

void source_add_suite(void)
{
	size_t i = 0;
	CU_pSuite suite;

	suite = CU_add_suite("Source Layer", NULL, NULL);
	while (source_tests[i].pName) {
		CU_add_test(suite, source_tests[i].pName,
		            source_tests[i].pTestFunc);
		i++;
	}
}

