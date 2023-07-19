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

#include <check.h>
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
                                      const char *prev_rev,
                                      size_t *size)
{
	(void)prev_rev;
	backend_find_migrations_called++;
	ck_assert(!!cur_rev);
	ck_assert(!!size);
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
START_TEST(source_init_skips_backends_without_init)
{
	*errbuf = '\0';
	backend_init_called = 0;
	sources[0] = &backend_without_init;
	source_init();

	ck_assert(!*errbuf);
	ck_assert(!backend_init_called);
}
END_TEST

/**
 * That that we get the correct error message if source_init()
 * fails to initialize a backend.
 */
START_TEST(source_init_fails_to_init_backend)
{
	const char *err = "failed to initialize 'init'\n";

	*errbuf = '\0';
	sources[0] = &backend_with_init;
	backend_init_called = 1;
	source_init();

	ck_assert_int_eq(backend_init_called, 2);
	ck_assert_str_eq(errbuf, err);
}
END_TEST

/**
 * Test that source_get_config_cb() returns NULL if
 * passed invalid parameters.
 */
START_TEST(source_get_config_cb_invalid_params)
{
	memset(sources, 0, sizeof sources);
	sources[0] = &backend_with_init;
	ck_assert(!source_get_config_cb(NULL, 1));
	ck_assert(!source_get_config_cb("init", 0));
}
END_TEST

/**
 * Test that source_get_config_cb() fails if no backends
 * are usable.
 */
START_TEST(source_get_config_cb_no_usable_backends)
{
	memset(sources, 0, sizeof sources);
	ck_assert(!source_get_config_cb("init", 4));
}
END_TEST

/**
 * Test that source_get_config_cb() returns the specified
 * backend's config callback.
 */
START_TEST(test_source_get_config_cb)
{
	memset(sources, 0, sizeof sources);
	sources[0] = &backend_with_init;
	sources[1] = &backend_without_init;
	ck_assert(source_get_config_cb("init", 4) == backend_config);
	ck_assert(!source_get_config_cb("no-init", 7));
}
END_TEST

/**
 * Test that source_find_migrations() returns NULL if
 * passed invalid parameters.
 */
START_TEST(source_find_migrations_invalid_params)
{
	size_t size = SIZE_MAX >> 1;
	const char *head = "head";
	const char *prev = "prev";

	backend_find_migrations_called = 0;
	memset(sources, 0, sizeof sources);
	sources[0] = &backend_with_init;

	/* !source */
	ck_assert_ptr_null(source_find_migrations(NULL, head, prev, &size));
	ck_assert_uint_eq(size, 0);
	ck_assert(!backend_find_migrations_called);

	/* !size */
	ck_assert_ptr_null(source_find_migrations("init", head, prev, NULL));
	ck_assert_uint_eq(size, 0);
	ck_assert(!backend_find_migrations_called);
}
END_TEST

/**
 * Test that source_find_migrations() fails if no backends
 * are usable.
 */
START_TEST(source_find_migrations_no_usable_backends)
{
	size_t size = SIZE_MAX >> 1;
	const char *head = "head";

	backend_find_migrations_called = 0;
	memset(sources, 0, sizeof sources);

	ck_assert_ptr_null(source_find_migrations("init", head, NULL, &size));
	ck_assert_uint_eq(size, 0);
	ck_assert(!backend_find_migrations_called);
}
END_TEST

/**
 * Test that source_find_migrations() calls the specified
 * backend's callback, and returns the result.
 */
START_TEST(test_source_find_migrations)
{
	char **m;
	size_t size = SIZE_MAX >> 1;
	const char *head = "head";

	backend_find_migrations_called = 0;
	memset(sources, 0, sizeof sources);
	sources[0] = &backend_without_init;
	sources[1] = &backend_with_init;

	m = source_find_migrations("init", head, NULL, &size);
	ck_assert_ptr_eq(m, (char **)1234);
	ck_assert_uint_eq(size, 12345);
	ck_assert(backend_find_migrations_called);
}
END_TEST

/**
 * Test that source_get_local_head() returns NULL if
 * passed invalid parameters.
 */
START_TEST(source_get_local_head_invalid_params)
{
	backend_get_head_called = 0;
	memset(sources, 0, sizeof sources);
	sources[0] = &backend_with_init;

	/* !source */
	ck_assert_ptr_null(source_get_local_head(NULL));
	ck_assert_ptr_null(source_get_local_head("\0"));
	ck_assert(!backend_get_head_called);
}
END_TEST

/**
 * Test that source_get_local_head() fails if no backends
 * are usable.
 */
START_TEST(source_get_local_head_no_usable_backends)
{
	backend_get_head_called = 0;
	memset(sources, 0, sizeof sources);
	ck_assert_ptr_null(source_get_local_head("init"));
	ck_assert(!backend_get_head_called);
}
END_TEST

/**
 * Test that source_get_local_head() calls the specified
 * backend's callback, and returns the result.
 */
START_TEST(test_source_get_local_head)
{
	const char *head;
	backend_get_head_called = 0;
	memset(sources, 0, sizeof sources);
	sources[0] = &backend_without_init;
	sources[1] = &backend_with_init;

	ck_assert_ptr_nonnull(head = source_get_local_head("init"));
	ck_assert_str_eq(head, "head");
	ck_assert(backend_get_head_called);
}
END_TEST

/**
 * Test that source_get_migration_path() returns NULL if
 * passed invalid parameters.
 */
START_TEST(source_get_migration_path_invalid_params)
{
	backend_get_migration_path_called = 0;
	memset(sources, 0, sizeof sources);
	sources[0] = &backend_with_init;

	/* !source */
	ck_assert_ptr_null(source_get_migration_path(NULL));
	ck_assert_ptr_null(source_get_migration_path("\0"));
	ck_assert(!backend_get_migration_path_called);
}
END_TEST

/**
 * Test that source_get_migration_path() fails if no backends
 * are usable.
 */
START_TEST(source_get_migration_path_no_usable_backends)
{
	backend_get_migration_path_called = 0;
	memset(sources, 0, sizeof sources);
	ck_assert_ptr_null(source_get_migration_path("init"));
	ck_assert(!backend_get_migration_path_called);
}
END_TEST

/**
 * Test that source_get_migration_path() calls the specified
 * backend's callback, and returns the result.
 */
START_TEST(test_source_get_migration_path)
{
	const char *path;
	backend_get_migration_path_called = 0;
	memset(sources, 0, sizeof sources);
	sources[0] = &backend_without_init;
	sources[1] = &backend_with_init;

	ck_assert_ptr_nonnull(path = source_get_migration_path("init"));
	ck_assert_str_eq(path, ".");
	ck_assert(backend_get_migration_path_called);
}
END_TEST

/**
 * Test that source_uninit() skips a backend without an
 * uninit callback.
 */
START_TEST(source_uninit_skips_backends_without_uninit)
{
	*errbuf = '\0';
	backend_uninit_called = 0;
	memset(sources, 0, sizeof sources);
	sources[0] = &backend_without_init;

	source_uninit();
	ck_assert(!*errbuf);
	ck_assert(!backend_uninit_called);
}
END_TEST

/**
 * That that we get the correct error message if source_uninit()
 * fails to uninitialize a backend.
 */
START_TEST(source_uninit_fails_to_uninit_backend)
{
	const char *err = "failed to uninitialize 'init'\n";

	*errbuf = '\0';
	memset(sources, 0, sizeof sources);
	sources[1] = &backend_with_init;
	backend_uninit_called = 1;

	source_uninit();
	ck_assert_int_eq(backend_uninit_called, 2);
	ck_assert_str_eq(errbuf, err);
}
END_TEST

Suite *source_suite(void)
{
	Suite *s;
	TCase *t;

	s = suite_create("Source");
	t = tcase_create("source_init");
	tcase_add_test(t, source_init_skips_backends_without_init);
	tcase_add_test(t, source_init_fails_to_init_backend);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	t = tcase_create("source_get_config_cb");
	tcase_add_test(t, source_get_config_cb_invalid_params);
	tcase_add_test(t, source_get_config_cb_no_usable_backends);
	tcase_add_test(t, test_source_get_config_cb);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	t = tcase_create("source_find_migrations");
	tcase_add_test(t, source_find_migrations_invalid_params);
	tcase_add_test(t, source_find_migrations_no_usable_backends);
	tcase_add_test(t, test_source_find_migrations);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	t = tcase_create("source_get_local_head");
	tcase_add_test(t, source_get_local_head_invalid_params);
	tcase_add_test(t, source_get_local_head_no_usable_backends);
	tcase_add_test(t, test_source_get_local_head);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	t = tcase_create("source_get_migration_path");
	tcase_add_test(t, source_get_migration_path_invalid_params);
	tcase_add_test(t, source_get_migration_path_no_usable_backends);
	tcase_add_test(t, test_source_get_migration_path);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	t = tcase_create("source_uninit");
	tcase_add_test(t, source_uninit_skips_backends_without_uninit);
	tcase_add_test(t, source_uninit_fails_to_uninit_backend);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	return s;
}

