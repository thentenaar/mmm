/**
 * Minimal Migration Manager - File Source Tests
 * Copyright (C) 2015 Tim Hentenaar.
 *
 * This code is licenced under the Simplified BSD License.
 * See the LICENSE file for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <CUnit/CUnit.h>
#include "tests.h"

/* from test_runner.c */
extern char errbuf[];

#include "libgit2_stubs.h"
#include "../src/source/git.c"

/* {{{ Diff test inputs */

static git_diff diff_0_deltas = {
	0,
	{
		{ 0, {{ 0 }}, {{ 0 }} },
		{ 0, {{ 0 }}, {{ 0 }} },
		{ 0, {{ 0 }}, {{ 0 }} }
	}
};

static git_diff diff_1_rename = {
	1,
	{
		{ GIT_DELTA_RENAMED, {{ "1.sql" }}, {{ "2.sql" }} },
		{ 0, {{ 0 }}, {{ 0 }} },
		{ 0, {{ 0 }}, {{ 0 }} }
	}
};

static git_diff diff_1_delete = {
	1,
	{
		{ GIT_DELTA_DELETED, {{ "1.sql" }}, {{ 0 }} },
		{ 0, {{ 0 }}, {{ 0 }} },
		{ 0, {{ 0 }}, {{ 0 }} }
	}
};

static git_diff diff_1_add = {
	1,
	{
		{ GIT_DELTA_ADDED, {{ 0 }}, {{ "1.sql" }} },
		{ 0, {{ 0 }}, {{ 0 }} },
		{ 0, {{ 0 }}, {{ 0 }} }
	}
};

static git_diff diff_add_rename = {
	2,
	{
		{ GIT_DELTA_ADDED, {{ 0 }}, {{ "1.sql" }} },
		{ GIT_DELTA_RENAMED, {{ "1.sql" }}, {{ "2.sql" }} },
		{ 0, {{ 0 }}, {{ 0 }} }
	}
};

static git_diff diff_add_and_delete = {
	3,
	{
		{ GIT_DELTA_ADDED, {{ 0 }}, {{ "1.sql" }} },
		{ GIT_DELTA_DELETED, {{ "1.sql" }}, {{ 0 }} },
		{ 0, {{ 0 }}, {{ 0 }} }
	}
};

static git_diff diff_add_dup = {
	3,
	{
		{ GIT_DELTA_ADDED, {{ 0 }}, {{ "1.sql" }} },
		{ GIT_DELTA_ADDED, {{ 0 }}, {{ "xxx.sql" }} },
		{ GIT_DELTA_ADDED, {{ 0 }}, {{ "1.sql" }} },
	}
};

static git_diff diff_add_del_rn = {
	3,
	{
		{ GIT_DELTA_ADDED, {{ 0 }}, {{ "1.sql" }} },
		{ GIT_DELTA_DELETED, {{ "1.sql" }}, {{ 0 }} },
		{ GIT_DELTA_RENAMED, {{ "1.sql" }}, {{ "22.sql" }} }
	}
};

static git_diff diff_add_xrn = {
	2,
	{
		{ GIT_DELTA_ADDED, {{ 0 }}, {{ "1.sql" }} },
		{ GIT_DELTA_RENAMED, {{ "11.sql" }}, {{ "22.sql" }} }
	}
};

static git_diff diff_add_del_empty = {
	2,
	{
		{ GIT_DELTA_ADDED, {{ 0 }}, {{ "1.sql" }} },
		{ GIT_DELTA_DELETED, {{ 0 }}, {{ "22.sql" }} }
	}
};

/* }}} */

/**
 * Test that git_init() and git_uninit() work.
 */
static void test_git_init_uninit(void)
{
	git_configure();
	CU_ASSERT_EQUAL(0, git_init());
	CU_ASSERT_EQUAL(0, git_uninit());
}

/**
 * Test that git_get_head() works.
 */
static void test_git_get_head(void)
{
	local_head[0] = '\0';
	CU_ASSERT_PTR_NULL(git_get_head());
	memcpy(local_head, "1", 2);
	CU_ASSERT_PTR_NOT_NULL(git_get_head());
}

/**
 * Test that git_get_migration_path() works.
 */
static void test_git_get_migration_path(void)
{
	CU_ASSERT_EQUAL(git_get_migration_path(), config.repo_path);
}

/**
 * Test that git_find_migrations() returns NULL if
 * size is NULL.
 */
static void git_find_migrations_null_size(void)
{
	CU_ASSERT_PTR_NULL(git_find_migrations("1", "2", NULL));
}

/**
 * Test that git_find_migrations() returns NULL if no
 * migration_path was set.
 */
static void git_find_migrations_no_migration_path(void)
{
	size_t size = 9999;

	config.migration_path[0] = '\0';
	CU_ASSERT_PTR_NULL(git_find_migrations("1", "2", &size));
	CU_ASSERT_EQUAL(size, 0);
}

/**
 * Test that git_find_migrations() return NULL if
 * git_repository_open() fails.
 */
static void git_find_migrations_open_repo_fails(void)
{
	size_t size;

	reset_libgit2_stubs();
	git_repository_open_returns = ~GIT_OK;
	memcpy(config.migration_path, "/tmp", 5);
	CU_ASSERT_PTR_NULL(git_find_migrations("1", "2", &size));
	CU_ASSERT_EQUAL(size, 0);
}

/**
 * Test that git_find_migrations() return NULL if
 * git_revparse_single() fails for HEAD.
 */
static void git_find_migrations_parse_head_fails(void)
{
	size_t size;

	reset_libgit2_stubs();
	git_revparse_single_returns_head = ~GIT_OK;
	memcpy(config.migration_path, "/tmp", 5);
	CU_ASSERT_PTR_NULL(git_find_migrations("1", NULL, &size));
	CU_ASSERT_EQUAL(size, 0);
}

/**
 * Test that git_find_migrations() return NULL if
 * git_revparse_single() fails for current_head.
 */
static void git_find_migrations_parse_current_head_fails(void)
{
	size_t size;

	reset_libgit2_stubs();
	git_revparse_single_returns = ~GIT_OK;
	memcpy(config.migration_path, "/tmp", 5);
	CU_ASSERT_PTR_NULL(git_find_migrations("1", NULL, &size));
	CU_ASSERT_EQUAL(size, 0);
}

/**
 * Test that git_find_migrations() return NULL if
 * git_revwalk_new() fails, and that it sets the local_head.
 */
static void git_find_migrations_revwalk_new_fails(void)
{
	size_t size;
	int head = 1234;

	reset_libgit2_stubs();
	git_revparse_single_out_head = &head;
	git_revwalk_new_returns = ~GIT_OK;
	memcpy(config.migration_path, "/tmp", 5);
	local_head[0] = '\0';

	CU_ASSERT_PTR_NULL(git_find_migrations("1", NULL, &size));
	CU_ASSERT_EQUAL(size, 0);
	CU_ASSERT_STRING_EQUAL(local_head, "1234");
}

/**
 * Test that git_find_migrations() return NULL if
 * git_revwalk_push_head() fails.
 */
static void git_find_migrations_revwalk_push_head_fails(void)
{
	size_t size;
	int head = 1234;

	reset_libgit2_stubs();
	git_revparse_single_out_head = &head;
	git_revwalk_push_returns = ~GIT_OK;
	memcpy(config.migration_path, "/tmp", 5);
	local_head[0] = '\0';

	CU_ASSERT_PTR_NULL(git_find_migrations("1", NULL, &size));
	CU_ASSERT_EQUAL(size, 0);
}

/**
 * Test that git_find_migrations() return NULL if
 * git_revwalk_hide() fails.
 */
static void git_find_migrations_revwalk_hide_fails(void)
{
	size_t size;
	int head = 1234;

	reset_libgit2_stubs();
	git_revparse_single_out_head = &head;
	git_revparse_single_out = &head;
	git_revwalk_hide_returns = ~GIT_OK;
	memcpy(config.migration_path, "/tmp", 5);
	local_head[0] = '\0';

	CU_ASSERT_PTR_NULL(git_find_migrations("1", "2", &size));
	CU_ASSERT_EQUAL(size, 0);
}

/**
 * Test that git_find_migrations() return NULL if
 * git_revwalk_next() produces no revs.
 */
static void git_find_migrations_revwalk_next_no_revs(void)
{
	size_t size;
	int head = 1234;

	reset_libgit2_stubs();
	giterr_last_returns = NULL;
	git_revparse_single_out_head = &head;
	git_revparse_single_out = &head;
	git_revwalk_next_returns = 1;
	memcpy(config.migration_path, "/tmp", 5);
	local_head[0] = '\0';

	CU_ASSERT_PTR_NULL(git_find_migrations("1", NULL, &size));
	CU_ASSERT_EQUAL(size, 0);
}

/**
 * Test that git_find_migrations() handles error
 * messages from libgit2.
 */
static void git_find_migrations_handles_libgit2_error_messages(void)
{
	size_t size;
	int head = 1234;

	reset_libgit2_stubs();
	last_giterr.message = "All your base are belong to us";
	git_revparse_single_out_head = &head;
	git_revparse_single_out = &head;
	git_revwalk_next_returns = 1;
	memcpy(config.migration_path, "/tmp", 5);
	local_head[0] = '\0';
	errbuf[0] = '\0';

	CU_ASSERT_PTR_NULL(git_find_migrations("1", NULL, &size));
	CU_ASSERT_EQUAL(size, 0);
	CU_ASSERT_STRING_EQUAL(errbuf, "git_find_migrations: "
	                       "All your base are belong to us\n");
}

/**
 * Test that git_find_migrations() return NULL if
 * git_commit_lookup() fails.
 */
static void generate_diff_commit_lookup_fails(void)
{
	size_t size;
	int head = 1234;

	reset_libgit2_stubs();
	giterr_last_returns = NULL;
	git_revparse_single_out_head = &head;
	git_revparse_single_out = &head;
	git_revwalk_next_returns = 2;
	git_commit_lookup_returns = ~GIT_OK;
	memcpy(config.migration_path, "/tmp", 5);
	local_head[0] = '\0';

	CU_ASSERT_PTR_NULL(git_find_migrations("1", NULL, &size));
	CU_ASSERT_EQUAL(size, 0);
}

/**
 * Test that git_find_migrations() returns NULL if the commit
 * has 0 parents.
 */
static void generate_diff_parent_count_0(void)
{
	size_t size;
	int head = 1234;

	reset_libgit2_stubs();
	giterr_last_returns = NULL;
	git_revparse_single_out_head = &head;
	git_revparse_single_out = &head;
	git_revwalk_next_returns = 2;
	git_commit_lookup_commit = (git_commit *)1234;
	git_commit_parentcount_returns = 0;
	git_commit_parent_returns = ~GIT_OK;
	memcpy(config.migration_path, "/tmp", 5);
	local_head[0] = '\0';

	CU_ASSERT_PTR_NULL(git_find_migrations("1", NULL, &size));
	CU_ASSERT_EQUAL(size, 0);
}

/**
 * Test that git_find_migrations() returns NULL if the commit
 * has 2 parents.
 */
static void generate_diff_parent_count_2(void)
{
	size_t size;
	int head = 1234;

	reset_libgit2_stubs();
	giterr_last_returns = NULL;
	git_revparse_single_out_head = &head;
	git_revparse_single_out = &head;
	git_revwalk_next_returns = 2;
	git_commit_lookup_commit = (git_commit *)1234;
	git_commit_parentcount_returns = 2;
	git_commit_parent_returns = ~GIT_OK;
	memcpy(config.migration_path, "/tmp", 5);
	local_head[0] = '\0';

	CU_ASSERT_PTR_NULL(git_find_migrations("1", NULL, &size));
	CU_ASSERT_EQUAL(size, 0);
}

/**
 * Test that git_find_migrations() returns NULL if we can't
 * resolve the parent commit.
 */
static void generate_diff_cant_resolve_parent(void)
{
	size_t size;
	int head = 1234;

	reset_libgit2_stubs();
	giterr_last_returns = NULL;
	git_revparse_single_out_head = &head;
	git_revparse_single_out = &head;
	git_revwalk_next_returns = 2;
	git_commit_lookup_commit = (git_commit *)1234;
	git_commit_parentcount_returns = 1;
	git_commit_parent_returns = ~GIT_OK;
	memcpy(config.migration_path, "/tmp", 5);
	local_head[0] = '\0';

	CU_ASSERT_PTR_NULL(git_find_migrations("1", NULL, &size));
	CU_ASSERT_EQUAL(size, 0);
}

/**
 * Test that git_find_migrations() returns NULL if the parent
 * commit is NULL.
 */
static void generate_diff_null_parent(void)
{
	size_t size;
	int head = 1234;

	reset_libgit2_stubs();
	giterr_last_returns = NULL;
	git_revparse_single_out_head = &head;
	git_revparse_single_out = &head;
	git_revwalk_next_returns = 2;
	git_commit_lookup_commit = (git_commit *)1234;
	git_commit_parentcount_returns = 1;
	memcpy(config.migration_path, "/tmp", 5);
	local_head[0] = '\0';

	CU_ASSERT_PTR_NULL(git_find_migrations("1", NULL, &size));
	CU_ASSERT_EQUAL(size, 0);
}

/**
 * Test that git_find_migrations() returns NULL if we can't
 * resolve the commit's tree.
 */
static void generate_diff_cant_resolve_tree(void)
{
	size_t size;
	int head = 1234;

	reset_libgit2_stubs();
	giterr_last_returns = NULL;
	git_revparse_single_out_head = &head;
	git_revparse_single_out = &head;
	git_revwalk_next_returns = 2;
	git_commit_lookup_commit = (git_commit *)1234;
	git_commit_parent_commit = (git_commit *)5678;
	git_commit_parentcount_returns = 1;
	git_commit_tree_returns = ~GIT_OK;
	memcpy(config.migration_path, "/tmp", 5);
	local_head[0] = '\0';

	CU_ASSERT_PTR_NULL(git_find_migrations("1", NULL, &size));
	CU_ASSERT_EQUAL(size, 0);
}

/**
 * Test that git_find_migrations() returns NULL if we can't
 * resolve the parent commit's tree.
 */
static void generate_diff_cant_resolve_parent_tree(void)
{
	size_t size;
	int head = 1234;

	reset_libgit2_stubs();
	giterr_last_returns = NULL;
	git_revparse_single_out_head = &head;
	git_revparse_single_out = &head;
	git_revwalk_next_returns = 2;
	git_commit_lookup_commit = (git_commit *)1234;
	git_commit_parent_commit = (git_commit *)5678;
	git_commit_parentcount_returns = 1;
	git_commit_tree_parent_returns = ~GIT_OK;
	memcpy(config.migration_path, "/tmp", 5);
	local_head[0] = '\0';

	CU_ASSERT_PTR_NULL(git_find_migrations("1", NULL, &size));
	CU_ASSERT_EQUAL(size, 0);
}

/**
 * Test that git_find_migrations() returns NULL if we can't
 * diff the two trees.
 */
static void generate_diff_tree_to_tree_fails(void)
{
	size_t size;
	int head = 1234;

	reset_libgit2_stubs();
	giterr_last_returns = NULL;
	git_revparse_single_out_head = &head;
	git_revparse_single_out = &head;
	git_revwalk_next_returns = 2;
	git_commit_lookup_commit = (git_commit *)1234;
	git_commit_parent_commit = (git_commit *)5678;
	git_commit_parentcount_returns = 1;
	git_diff_tree_to_tree_returns = ~GIT_OK;
	memcpy(config.migration_path, "/tmp", 5);
	local_head[0] = '\0';

	CU_ASSERT_PTR_NULL(git_find_migrations("1", NULL, &size));
	CU_ASSERT_EQUAL(size, 0);
}

/**
 * Test that git_find_migrations() returns NULL if we can't
 * remove duplicates from the diff.
 */
static void generate_diff_find_similar_fails(void)
{
	size_t size;
	int head = 1234;

	reset_libgit2_stubs();
	giterr_last_returns = NULL;
	git_revparse_single_out_head = &head;
	git_revparse_single_out = &head;
	git_revwalk_next_returns = 2;
	git_commit_lookup_commit = (git_commit *)1234;
	git_commit_parent_commit = (git_commit *)5678;
	git_commit_parentcount_returns = 1;
	git_diff_find_similar_returns = ~GIT_OK;
	memcpy(config.migration_path, "/tmp", 5);
	local_head[0] = '\0';

	CU_ASSERT_PTR_NULL(git_find_migrations("1", NULL, &size));
	CU_ASSERT_EQUAL(size, 0);
}

/**
 * Test that git_find_migrations() returns NULL if one diff
 * is generated with 0 deltas.
 */
static void generate_diff_0_deltas(void)
{
	size_t size;
	int head = 1234;

	reset_libgit2_stubs();
	giterr_last_returns = NULL;
	git_revparse_single_out_head = &head;
	git_revparse_single_out = &head;
	git_revwalk_next_returns = 2;
	git_commit_lookup_commit = (git_commit *)1234;
	git_commit_parent_commit = (git_commit *)5678;
	git_commit_parentcount_returns = 1;
	git_diff_tree_to_tree_diff = &diff_0_deltas;
	memcpy(config.migration_path, "/tmp", 5);
	local_head[0] = '\0';

	CU_ASSERT_PTR_NULL(git_find_migrations("1", NULL, &size));
	CU_ASSERT_EQUAL(size, 0);
}

/**
 * Test that git_find_migrations() returns NULL if one diff
 * is generated with 1 RENAMED delta only.
 */
static void process_diff_1_rename(void)
{
	size_t size;
	int head = 1234;

	reset_libgit2_stubs();
	giterr_last_returns = NULL;
	git_revparse_single_out_head = &head;
	git_revparse_single_out = &head;
	git_revwalk_next_returns = 2;
	git_commit_lookup_commit = (git_commit *)1234;
	git_commit_parent_commit = (git_commit *)5678;
	git_commit_parentcount_returns = 1;
	git_diff_tree_to_tree_diff = &diff_1_rename;
	memcpy(config.migration_path, "/tmp", 5);
	local_head[0] = '\0';

	CU_ASSERT_PTR_NULL(git_find_migrations("1", NULL, &size));
	CU_ASSERT_EQUAL(size, 0);
}

/**
 * Test that git_find_migrations() returns NULL if one diff
 * is generated with 1 DELETED delta only.
 */
static void process_diff_1_delete(void)
{
	size_t size;
	int head = 1234;

	reset_libgit2_stubs();
	giterr_last_returns = NULL;
	git_revparse_single_out_head = &head;
	git_revparse_single_out = &head;
	git_revwalk_next_returns = 2;
	git_commit_lookup_commit = (git_commit *)1234;
	git_commit_parent_commit = (git_commit *)5678;
	git_commit_parentcount_returns = 1;
	git_diff_tree_to_tree_diff = &diff_1_delete;
	memcpy(config.migration_path, "/tmp", 5);
	local_head[0] = '\0';

	CU_ASSERT_PTR_NULL(git_find_migrations("1", NULL, &size));
	CU_ASSERT_EQUAL(size, 0);
}

/**
 * Test that git_find_migrations() yields the correct result
 * with 1 ADDED delta only.
 */
static void process_diff_1_add(void)
{
	size_t size;
	int head = 1234;
	char **migrations;

	reset_libgit2_stubs();
	giterr_last_returns = NULL;
	git_revparse_single_out_head = &head;
	git_revparse_single_out = &head;
	git_revwalk_next_returns = 2;
	git_commit_lookup_commit = (git_commit *)1234;
	git_commit_parent_commit = (git_commit *)5678;
	git_commit_parentcount_returns = 1;
	git_diff_tree_to_tree_diff = &diff_1_add;
	memcpy(config.migration_path, "/tmp", 5);
	local_head[0] = '\0';

	migrations = git_find_migrations("1", NULL, &size);
	CU_ASSERT_PTR_NOT_NULL_FATAL(migrations);
	CU_ASSERT_EQUAL(size, 1);
	CU_ASSERT_PTR_NOT_NULL(migrations[0]);
	CU_ASSERT_STRING_EQUAL(migrations[0],
	                       diff_1_add.deltas[0].new_file.path);
	free(migrations[0]);
	free(migrations);
}

/**
 * Test that git_find_migrations() returns NULL if one diff
 * is generated with 1 ADDED and 1 DELETED delta for
 * the same file.
 */
static void process_diff_1_add_and_delete(void)
{
	size_t size;
	int head = 1234;

	reset_libgit2_stubs();
	giterr_last_returns = NULL;
	git_revparse_single_out_head = &head;
	git_revparse_single_out = &head;
	git_revwalk_next_returns = 2;
	git_commit_lookup_commit = (git_commit *)1234;
	git_commit_parent_commit = (git_commit *)5678;
	git_commit_parentcount_returns = 1;
	git_diff_tree_to_tree_diff = &diff_add_and_delete;
	memcpy(config.migration_path, "/tmp", 5);
	local_head[0] = '\0';

	CU_ASSERT_PTR_NULL(git_find_migrations("1", NULL, &size));
	CU_ASSERT_EQUAL(size, 0);
}

/**
 * Test that git_find_migrations() yields the correct result
 * with 1 ADDED and 1 RENAMED delta for the same file.
 */
static void process_diff_1_add_and_rename(void)
{
	size_t size;
	int head = 1234;
	char **migrations;

	reset_libgit2_stubs();
	giterr_last_returns = NULL;
	git_revparse_single_out_head = &head;
	git_revparse_single_out = &head;
	git_revwalk_next_returns = 2;
	git_commit_lookup_commit = (git_commit *)1234;
	git_commit_parent_commit = (git_commit *)5678;
	git_commit_parentcount_returns = 1;
	git_diff_tree_to_tree_diff = &diff_add_rename;
	memcpy(config.migration_path, "/tmp", 5);
	local_head[0] = '\0';

	migrations = git_find_migrations("1", NULL, &size);
	CU_ASSERT_PTR_NOT_NULL_FATAL(migrations);
	CU_ASSERT_EQUAL(size, 1);
	CU_ASSERT_PTR_NOT_NULL(migrations[0]);
	CU_ASSERT_STRING_EQUAL(migrations[0],
	                       diff_add_rename.deltas[1].new_file.path);
	free(migrations[0]);
	free(migrations);
}

/**
 * Test that git_find_migrations() yields the correct result
 * with 2 ADDED deltas for the same file.
 */
static void process_diff_add_duplicate(void)
{
	size_t size;
	int head = 1234;
	char **migrations;

	reset_libgit2_stubs();
	giterr_last_returns = NULL;
	git_revparse_single_out_head = &head;
	git_revparse_single_out = &head;
	git_revwalk_next_returns = 2;
	git_commit_lookup_commit = (git_commit *)1234;
	git_commit_parent_commit = (git_commit *)5678;
	git_commit_parentcount_returns = 1;
	git_diff_tree_to_tree_diff = &diff_add_dup;
	memcpy(config.migration_path, "/tmp", 5);
	local_head[0] = '\0';

	migrations = git_find_migrations("1", NULL, &size);
	CU_ASSERT_PTR_NOT_NULL_FATAL(migrations);
	CU_ASSERT_EQUAL(size, 2);
	CU_ASSERT_PTR_NOT_NULL(migrations[0]);
	CU_ASSERT_PTR_NOT_NULL(migrations[1]);
	CU_ASSERT_STRING_EQUAL(migrations[0],
	                       diff_add_dup.deltas[0].new_file.path);
	CU_ASSERT_STRING_EQUAL(migrations[1],
	                       diff_add_dup.deltas[1].new_file.path);
	free(migrations[1]);
	free(migrations[0]);
	free(migrations);
}

/**
 * Test that git_find_migrations() yields the correct result
 * with 1 ADDED + 1 DELETED for the same file, and a RENAMED
 * delta for the same file.
 */
static void process_diff_add_del_rn(void)
{
	size_t size;
	int head = 1234;
	char **migrations;

	reset_libgit2_stubs();
	giterr_last_returns = NULL;
	git_revparse_single_out_head = &head;
	git_revparse_single_out = &head;
	git_revwalk_next_returns = 2;
	git_commit_lookup_commit = (git_commit *)1234;
	git_commit_parent_commit = (git_commit *)5678;
	git_commit_parentcount_returns = 1;
	git_diff_tree_to_tree_diff = &diff_add_del_rn;
	memcpy(config.migration_path, "/tmp", 5);
	local_head[0] = '\0';

	migrations = git_find_migrations("1", NULL, &size);
	CU_ASSERT_PTR_NULL(migrations);
	CU_ASSERT_EQUAL(size, 0);
}

/**
 * Test that git_find_migrations() yields the correct result
 * with 1 ADDED delta and 1 RENAMED delta for a different
 * file.
 */
static void process_diff_add_xrn(void)
{
	size_t size;
	int head = 1234;
	char **migrations;

	reset_libgit2_stubs();
	giterr_last_returns = NULL;
	git_revparse_single_out_head = &head;
	git_revparse_single_out = &head;
	git_revwalk_next_returns = 2;
	git_commit_lookup_commit = (git_commit *)1234;
	git_commit_parent_commit = (git_commit *)5678;
	git_commit_parentcount_returns = 1;
	git_diff_tree_to_tree_diff = &diff_add_xrn;
	memcpy(config.migration_path, "/tmp", 5);
	local_head[0] = '\0';

	migrations = git_find_migrations("1", NULL, &size);
	CU_ASSERT_PTR_NOT_NULL_FATAL(migrations);
	CU_ASSERT_EQUAL(size, 1);
	CU_ASSERT_PTR_NOT_NULL(migrations[0]);
	CU_ASSERT_STRING_EQUAL(migrations[0],
	                       diff_add_xrn.deltas[0].new_file.path);
	free(migrations[0]);
	free(migrations);
}

/**
 * Test that git_find_migrations() yields the correct result
 * with 1 ADDED delta and 1 DELETED delta with a empty
 * old file name.
 */
static void process_diff_add_del_empty(void)
{
	size_t size;
	int head = 1234;
	char **migrations;

	reset_libgit2_stubs();
	giterr_last_returns = NULL;
	git_revparse_single_out_head = &head;
	git_revparse_single_out = &head;
	git_revwalk_next_returns = 2;
	git_commit_lookup_commit = (git_commit *)1234;
	git_commit_parent_commit = (git_commit *)5678;
	git_commit_parentcount_returns = 1;
	git_diff_tree_to_tree_diff = &diff_add_del_empty;
	memcpy(config.migration_path, "/tmp", 5);
	local_head[0] = '\0';

	migrations = git_find_migrations("1", NULL, &size);
	CU_ASSERT_PTR_NOT_NULL_FATAL(migrations);
	CU_ASSERT_EQUAL(size, 1);
	CU_ASSERT_PTR_NOT_NULL(migrations[0]);
	CU_ASSERT_STRING_EQUAL(migrations[0],
	                       diff_add_xrn.deltas[0].new_file.path);
	free(migrations[0]);
	free(migrations);
}

static CU_TestInfo source_git_tests[] = {
	{
		"git_init() / git_uninit() work",
		test_git_init_uninit
	},
	{
		"git_get_head() - works",
		test_git_get_head
	},
	{
		"git_get_migration_path() - works",
		test_git_get_migration_path
	},
	{
		"git_find_migrations() - NULL size",
		git_find_migrations_null_size
	},
	{
		"git_find_migrations() - no migration path",
		git_find_migrations_no_migration_path
	},
	{
		"git_find_migrations() - repository open fails",
		git_find_migrations_open_repo_fails
	},
	{
		"git_find_migrations() - parsing HEAD fails",
		git_find_migrations_parse_head_fails
	},
	{
		"git_find_migrations() - parsing current_head fails",
		git_find_migrations_parse_current_head_fails
	},
	{
		"git_find_migrations() - git_revwalk_new() fails",
		git_find_migrations_revwalk_new_fails
	},
	{
		"git_find_migrations() - git_revwalk_push_head() fails",
		git_find_migrations_revwalk_push_head_fails
	},
	{
		"git_find_migrations() - git_revwalk_hide() fails",
		git_find_migrations_revwalk_hide_fails
	},
	{
		"git_find_migrations() - git_revwalk_next() yields no revs",
		git_find_migrations_revwalk_next_no_revs
	},
	{
		"git_find_migrations() - handles libgit2 error messages",
		git_find_migrations_handles_libgit2_error_messages
	},
	{
		"generate_diff() - git_commit_lookup() fails",
		generate_diff_commit_lookup_fails
	},
	{
		"generate_diff() - parent_count 0",
		generate_diff_parent_count_0
	},
	{
		"generate_diff() - parent_count 2",
		generate_diff_parent_count_2
	},
	{
		"generate_diff() - can't resolve parent commit",
		generate_diff_cant_resolve_parent
	},
	{
		"generate_diff() - null parent",
		generate_diff_null_parent
	},
	{
		"generate_diff() - can't resolve tree",
		generate_diff_cant_resolve_tree
	},
	{
		"generate_diff() - can't resolve parent's tree",
		generate_diff_cant_resolve_parent_tree
	},
	{
		"generate_diff() - can't diff trees",
		generate_diff_tree_to_tree_fails
	},
	{
		"generate_diff() - can't remove duplicates",
		generate_diff_find_similar_fails
	},
	{
		"generate_diff() - 1 diff, 0 deltas",
		generate_diff_0_deltas
	},
	{
		"process_diff() - 1 rename",
		process_diff_1_rename
	},
	{
		"process_diff() - 1 delete",
		process_diff_1_delete
	},
	{
		"process_diff() - 1 add",
		process_diff_1_add
	},
	{
		"process_diff() - 1 add + delete",
		process_diff_1_add_and_delete
	},
	{
		"process_diff() - 1 add + rename",
		process_diff_1_add_and_rename
	},
	{
		"process_diff() - 2 adds (duplicate file)",
		process_diff_add_duplicate
	},
	{
		"process_diff() - add + delete + rename",
		process_diff_add_del_rn
	},
	{
		"process_diff() - add + unrelated rename",
		process_diff_add_xrn
	},
	{
		"process_diff() - add + empty modification",
		process_diff_add_del_empty
	},

	CU_TEST_INFO_NULL
};

void source_git_add_suite(void)
{
	size_t i = 0;
	CU_pSuite suite;

	suite = CU_add_suite("Migration Source: Git", NULL, NULL);
	while (source_git_tests[i].pName) {
		CU_add_test(suite, source_git_tests[i].pName,
		            source_git_tests[i].pTestFunc);
		i++;
	}
}

