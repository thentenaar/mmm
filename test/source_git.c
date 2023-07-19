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

#include <check.h>
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
START_TEST(test_git_init_uninit)
{
	git_configure();
	ck_assert_int_eq(git_init(), 0);
	ck_assert_int_eq(git_uninit(), 0);
}
END_TEST

/**
 * Test that git_get_head() works.
 */
START_TEST(test_git_get_head)
{
	*local_head = '\0';
	ck_assert_ptr_null(git_get_head());
	memcpy(local_head, "1", 2);
	ck_assert_ptr_nonnull(git_get_head());
}
END_TEST

/**
 * Test that git_get_migration_path() works.
 */
START_TEST(test_git_get_migration_path)
{
	ck_assert_ptr_eq(git_get_migration_path(), config.repo_path);
}
END_TEST

/**
 * Test that git_find_migrations() returns NULL if
 * size is NULL.
 */
START_TEST(git_find_migrations_null_size)
{
	ck_assert_ptr_null(git_find_migrations("1", "2", NULL));
}
END_TEST

/**
 * Test that git_find_migrations() returns NULL if no
 * migration_path was set.
 */
START_TEST(git_find_migrations_no_migration_path)
{
	size_t size;

	*config.migration_path = '\0';
	ck_assert_ptr_null(git_find_migrations("1", "2", &size));
	ck_assert_uint_eq(size, 0);
}
END_TEST

/**
 * Test that git_find_migrations() return NULL if
 * git_repository_open() fails.
 */
START_TEST(git_find_migrations_open_repo_fails)
{
	size_t size;

	git_repository_open_returns = ~GIT_OK;
	memcpy(config.migration_path, "/tmp", 5);
	ck_assert_ptr_null(git_find_migrations("1", "2", &size));
	ck_assert_uint_eq(size, 0);
}
END_TEST

/**
 * Test that git_find_migrations() return NULL if
 * git_revparse_single() fails for HEAD.
 */
START_TEST(git_find_migrations_parse_head_fails)
{
	size_t size;

	git_revparse_single_returns_head = ~GIT_OK;
	memcpy(config.migration_path, "/tmp", 5);
	ck_assert_ptr_null(git_find_migrations("1", NULL, &size));
	ck_assert_uint_eq(size, 0);
}
END_TEST

/**
 * Test that git_find_migrations() return NULL if
 * git_revparse_single() fails for current_head.
 */
START_TEST(git_find_migrations_parse_current_head_fails)
{
	size_t size;

	git_revparse_single_returns = ~GIT_OK;
	memcpy(config.migration_path, "/tmp", 5);
	ck_assert_ptr_null(git_find_migrations("1", NULL, &size));
	ck_assert_uint_eq(size, 0);
}
END_TEST

/**
 * Test that git_find_migrations() return NULL if
 * git_revwalk_new() fails, and that it sets the local_head.
 */
START_TEST(git_find_migrations_revwalk_new_fails)
{
	size_t size;
	int head = 1234;

	git_revparse_single_out_head = &head;
	git_revwalk_new_returns      = ~GIT_OK;
	memcpy(config.migration_path, "/tmp", 5);
	*local_head = '\0';

	ck_assert_ptr_null(git_find_migrations("1", NULL, &size));
	ck_assert_uint_eq(size, 0);
	ck_assert_str_eq(local_head, "1234");
}
END_TEST

/**
 * Test that git_find_migrations() return NULL if
 * git_revwalk_push_head() fails.
 */
START_TEST(git_find_migrations_revwalk_push_head_fails)
{
	size_t size;
	int head = 1234;

	git_revparse_single_out_head = &head;
	git_revwalk_push_returns     = ~GIT_OK;
	memcpy(config.migration_path, "/tmp", 5);
	*local_head = '\0';

	ck_assert_ptr_null(git_find_migrations("1", NULL, &size));
	ck_assert_uint_eq(size, 0);
}
END_TEST

/**
 * Test that git_find_migrations() return NULL if
 * git_revwalk_hide() fails.
 */
START_TEST(git_find_migrations_revwalk_hide_fails)
{
	size_t size;
	int head = 1234;

	git_revparse_single_out_head = &head;
	git_revparse_single_out      = &head;
	git_revwalk_hide_returns     = ~GIT_OK;
	memcpy(config.migration_path, "/tmp", 5);
	*local_head = '\0';

	ck_assert_ptr_null(git_find_migrations("1", "2", &size));
	ck_assert_uint_eq(size, 0);
}
END_TEST

/**
 * Test that git_find_migrations() return NULL if
 * git_revwalk_next() produces no revs.
 */
START_TEST(git_find_migrations_revwalk_next_no_revs)
{
	size_t size;
	int head = 1234;

	giterr_last_returns          = NULL;
	git_revparse_single_out_head = &head;
	git_revparse_single_out      = &head;
	git_revwalk_next_returns     = 1;
	memcpy(config.migration_path, "/tmp", 5);
	*local_head = '\0';

	ck_assert_ptr_null(git_find_migrations("1", NULL, &size));
	ck_assert_uint_eq(size, 0);
}
END_TEST

/**
 * Test that git_find_migrations() handles error
 * messages from libgit2.
 */
START_TEST(git_find_migrations_handles_libgit2_error_messages)
{
	size_t size;
	int head = 1234;

	last_giterr.message = "All your base are belong to us";
	git_revparse_single_out_head = &head;
	git_revparse_single_out      = &head;
	git_revwalk_next_returns     = 1;
	memcpy(config.migration_path, "/tmp", 5);
	*errbuf = *local_head = '\0';

	ck_assert_ptr_null(git_find_migrations("1", NULL, &size));
	ck_assert_uint_eq(size, 0);
	ck_assert_str_eq(errbuf, "All your base are belong to us\n");
}
END_TEST

/**
 * Test that git_find_migrations() return NULL if
 * git_commit_lookup() fails.
 */
START_TEST(git_find_migrations_commit_lookup_fails)
{
	size_t size;
	int head = 1234;

	giterr_last_returns          = NULL;
	git_revparse_single_out_head = &head;
	git_revparse_single_out      = &head;
	git_revwalk_next_returns     = 2;
	git_commit_lookup_returns    = ~GIT_OK;
	memcpy(config.migration_path, "/tmp", 5);
	*local_head = '\0';

	ck_assert_ptr_null(git_find_migrations("1", NULL, &size));
	ck_assert_uint_eq(size, 0);
}
END_TEST

/**
 * Test that git_find_migrations() returns NULL if the commit
 * has 0 parents.
 */
START_TEST(git_find_migrations_zero_parents)
{
	size_t size;
	int head = 1234;

	giterr_last_returns            = NULL;
	git_revparse_single_out_head   = &head;
	git_revparse_single_out        = &head;
	git_revwalk_next_returns       = 2;
	git_commit_lookup_commit       = (git_commit *)1234;
	git_commit_parentcount_returns = 0;
	git_commit_parent_returns      = ~GIT_OK;
	memcpy(config.migration_path, "/tmp", 5);
	*local_head = '\0';

	ck_assert_ptr_null(git_find_migrations("1", NULL, &size));
	ck_assert_uint_eq(size, 0);
}
END_TEST

/**
 * Test that git_find_migrations() returns NULL if the commit
 * has 2 parents.
 */
START_TEST(git_find_migrations_two_parents)
{
	size_t size;
	int head = 1234;

	giterr_last_returns            = NULL;
	git_revparse_single_out_head   = &head;
	git_revparse_single_out        = &head;
	git_revwalk_next_returns       = 2;
	git_commit_lookup_commit       = (git_commit *)1234;
	git_commit_parentcount_returns = 2;
	git_commit_parent_returns      = ~GIT_OK;
	memcpy(config.migration_path, "/tmp", 5);
	*local_head = '\0';

	ck_assert_ptr_null(git_find_migrations("1", NULL, &size));
	ck_assert_uint_eq(size, 0);
}
END_TEST

/**
 * Test that git_find_migrations() returns NULL if we can't
 * resolve the parent commit.
 */
START_TEST(git_find_migrations_cant_resolve_parent)
{
	size_t size;
	int head = 1234;

	giterr_last_returns            = NULL;
	git_revparse_single_out_head   = &head;
	git_revparse_single_out        = &head;
	git_revwalk_next_returns       = 2;
	git_commit_lookup_commit       = (git_commit *)1234;
	git_commit_parentcount_returns = 1;
	git_commit_parent_returns      = ~GIT_OK;
	memcpy(config.migration_path, "/tmp", 5);
	*local_head = '\0';

	ck_assert_ptr_null(git_find_migrations("1", NULL, &size));
	ck_assert_uint_eq(size, 0);
}
END_TEST

/**
 * Test that git_find_migrations() returns NULL if the parent
 * commit is NULL.
 */
START_TEST(git_find_migrations_null_parent)
{
	size_t size;
	int head = 1234;

	giterr_last_returns            = NULL;
	git_revparse_single_out_head   = &head;
	git_revparse_single_out        = &head;
	git_revwalk_next_returns       = 2;
	git_commit_lookup_commit       = (git_commit *)1234;
	git_commit_parentcount_returns = 1;
	memcpy(config.migration_path, "/tmp", 5);
	*local_head = '\0';

	ck_assert_ptr_null(git_find_migrations("1", NULL, &size));
	ck_assert_uint_eq(size, 0);
}
END_TEST

/**
 * Test that git_find_migrations() returns NULL if we can't
 * resolve the commit's tree.
 */
START_TEST(git_find_migrations_cant_resolve_tree)
{
	size_t size;
	int head = 1234;

	giterr_last_returns            = NULL;
	git_revparse_single_out_head   = &head;
	git_revparse_single_out        = &head;
	git_revwalk_next_returns       = 2;
	git_commit_lookup_commit       = (git_commit *)1234;
	git_commit_parent_commit       = (git_commit *)5678;
	git_commit_parentcount_returns = 1;
	git_commit_tree_returns        = ~GIT_OK;
	memcpy(config.migration_path, "/tmp", 5);
	*local_head = '\0';

	ck_assert_ptr_null(git_find_migrations("1", NULL, &size));
	ck_assert_uint_eq(size, 0);
}
END_TEST

/**
 * Test that git_find_migrations() returns NULL if we can't
 * resolve the parent commit's tree.
 */
START_TEST(git_find_migrations_cant_resolve_parent_tree)
{
	size_t size;
	int head = 1234;

	giterr_last_returns            = NULL;
	git_revparse_single_out_head   = &head;
	git_revparse_single_out        = &head;
	git_revwalk_next_returns       = 2;
	git_commit_lookup_commit       = (git_commit *)1234;
	git_commit_parent_commit       = (git_commit *)5678;
	git_commit_parentcount_returns = 1;
	git_commit_tree_parent_returns = ~GIT_OK;
	memcpy(config.migration_path, "/tmp", 5);
	*local_head = '\0';

	ck_assert_ptr_null(git_find_migrations("1", NULL, &size));
	ck_assert_uint_eq(size, 0);
}
END_TEST

/**
 * Test that git_find_migrations() returns NULL if we can't
 * diff the two trees.
 */
START_TEST(git_find_migrations_tree_to_tree_diff_fails)
{
	size_t size;
	int head = 1234;

	giterr_last_returns            = NULL;
	git_revparse_single_out_head   = &head;
	git_revparse_single_out        = &head;
	git_revwalk_next_returns       = 2;
	git_commit_lookup_commit       = (git_commit *)1234;
	git_commit_parent_commit       = (git_commit *)5678;
	git_commit_parentcount_returns = 1;
	git_diff_tree_to_tree_returns  = ~GIT_OK;
	memcpy(config.migration_path, "/tmp", 5);
	*local_head = '\0';

	ck_assert_ptr_null(git_find_migrations("1", NULL, &size));
	ck_assert_uint_eq(size, 0);
}
END_TEST

/**
 * Test that git_find_migrations() returns NULL if we can't
 * remove duplicates from the diff.
 */
START_TEST(git_find_migrations_find_similar_fails)
{
	size_t size;
	int head = 1234;

	giterr_last_returns            = NULL;
	git_revparse_single_out_head   = &head;
	git_revparse_single_out        = &head;
	git_revwalk_next_returns       = 2;
	git_commit_lookup_commit       = (git_commit *)1234;
	git_commit_parent_commit       = (git_commit *)5678;
	git_commit_parentcount_returns = 1;
	git_diff_find_similar_returns  = ~GIT_OK;
	memcpy(config.migration_path, "/tmp", 5);
	*local_head = '\0';

	ck_assert_ptr_null(git_find_migrations("1", NULL, &size));
	ck_assert_uint_eq(size, 0);
}
END_TEST

/**
 * Test that git_find_migrations() returns NULL if one diff
 * is generated with 0 deltas.
 */
START_TEST(git_find_migrations_zero_deltas)
{
	size_t size;
	int head = 1234;

	giterr_last_returns            = NULL;
	git_revparse_single_out_head   = &head;
	git_revparse_single_out        = &head;
	git_revwalk_next_returns       = 2;
	git_commit_lookup_commit       = (git_commit *)1234;
	git_commit_parent_commit       = (git_commit *)5678;
	git_commit_parentcount_returns = 1;
	git_diff_tree_to_tree_diff     = &diff_0_deltas;
	memcpy(config.migration_path, "/tmp", 5);
	*local_head = '\0';

	ck_assert_ptr_null(git_find_migrations("1", NULL, &size));
	ck_assert_uint_eq(size, 0);
}
END_TEST

/**
 * Test that git_find_migrations() returns NULL if one diff
 * is generated with 1 RENAMED delta only.
 */
START_TEST(git_find_migrations_one_renamed)
{
	size_t size;
	int head = 1234;

	giterr_last_returns            = NULL;
	git_revparse_single_out_head   = &head;
	git_revparse_single_out        = &head;
	git_revwalk_next_returns       = 2;
	git_commit_lookup_commit       = (git_commit *)1234;
	git_commit_parent_commit       = (git_commit *)5678;
	git_commit_parentcount_returns = 1;
	git_diff_tree_to_tree_diff     = &diff_1_rename;
	memcpy(config.migration_path, "/tmp", 5);
	*local_head = '\0';

	ck_assert_ptr_null(git_find_migrations("1", NULL, &size));
	ck_assert_uint_eq(size, 0);
}
END_TEST

/**
 * Test that git_find_migrations() returns NULL if one diff
 * is generated with 1 DELETED delta only.
 */
START_TEST(git_find_migrations_one_deleted)
{
	size_t size;
	int head = 1234;

	giterr_last_returns            = NULL;
	git_revparse_single_out_head   = &head;
	git_revparse_single_out        = &head;
	git_revwalk_next_returns       = 2;
	git_commit_lookup_commit       = (git_commit *)1234;
	git_commit_parent_commit       = (git_commit *)5678;
	git_commit_parentcount_returns = 1;
	git_diff_tree_to_tree_diff     = &diff_1_delete;
	memcpy(config.migration_path, "/tmp", 5);
	*local_head = '\0';

	ck_assert_ptr_null(git_find_migrations("1", NULL, &size));
	ck_assert_uint_eq(size, 0);
}
END_TEST

/**
 * Test that git_find_migrations() yields the correct result
 * with 1 ADDED delta only.
 */
START_TEST(git_find_migrations_one_added)
{
	size_t size;
	int head = 1234;
	char **migrations;

	giterr_last_returns            = NULL;
	git_revparse_single_out_head   = &head;
	git_revparse_single_out        = &head;
	git_revwalk_next_returns       = 2;
	git_commit_lookup_commit       = (git_commit *)1234;
	git_commit_parent_commit       = (git_commit *)5678;
	git_commit_parentcount_returns = 1;
	git_diff_tree_to_tree_diff     = &diff_1_add;
	memcpy(config.migration_path, "/tmp", 5);
	*local_head = '\0';

	ck_assert_ptr_nonnull(migrations = git_find_migrations("1", NULL, &size));
	ck_assert_uint_eq(size, 1);
	ck_assert_str_eq(*migrations, diff_1_add.deltas[0].new_file.path);
	free(*migrations);
	free(migrations);
}
END_TEST

/**
 * Test that git_find_migrations() returns NULL if one diff
 * is generated with 1 ADDED and 1 DELETED delta for
 * the same file.
 */
START_TEST(git_find_migrations_one_added_and_deleted)
{
	size_t size;
	int head = 1234;

	giterr_last_returns            = NULL;
	git_revparse_single_out_head   = &head;
	git_revparse_single_out        = &head;
	git_revwalk_next_returns       = 2;
	git_commit_lookup_commit       = (git_commit *)1234;
	git_commit_parent_commit       = (git_commit *)5678;
	git_commit_parentcount_returns = 1;
	git_diff_tree_to_tree_diff     = &diff_add_and_delete;
	memcpy(config.migration_path, "/tmp", 5);
	*local_head = '\0';

	ck_assert_ptr_null(git_find_migrations("1", NULL, &size));
	ck_assert_uint_eq(size, 0);
}
END_TEST

/**
 * Test that git_find_migrations() yields the correct result
 * with 1 ADDED and 1 RENAMED delta for the same file.
 */
START_TEST(git_find_migrations_one_added_and_renamed)
{
	size_t size;
	int head = 1234;
	char **migrations;

	giterr_last_returns            = NULL;
	git_revparse_single_out_head   = &head;
	git_revparse_single_out        = &head;
	git_revwalk_next_returns       = 2;
	git_commit_lookup_commit       = (git_commit *)1234;
	git_commit_parent_commit       = (git_commit *)5678;
	git_commit_parentcount_returns = 1;
	git_diff_tree_to_tree_diff     = &diff_add_rename;
	memcpy(config.migration_path, "/tmp", 5);
	*local_head = '\0';

	ck_assert_ptr_nonnull(migrations = git_find_migrations("1", NULL, &size));
	ck_assert_uint_eq(size, 1);
	ck_assert_str_eq(*migrations, diff_add_rename.deltas[1].new_file.path);
	free(*migrations);
	free(migrations);
}
END_TEST

/**
 * Test that git_find_migrations() yields the correct result
 * with 2 ADDED deltas for the same file.
 */
START_TEST(git_find_migrations_two_added)
{
	size_t size;
	int head = 1234;
	char **migrations;

	giterr_last_returns            = NULL;
	git_revparse_single_out_head   = &head;
	git_revparse_single_out        = &head;
	git_revwalk_next_returns       = 2;
	git_commit_lookup_commit       = (git_commit *)1234;
	git_commit_parent_commit       = (git_commit *)5678;
	git_commit_parentcount_returns = 1;
	git_diff_tree_to_tree_diff     = &diff_add_dup;
	memcpy(config.migration_path, "/tmp", 5);
	*local_head = '\0';

	ck_assert_ptr_nonnull(migrations = git_find_migrations("1", NULL, &size));
	ck_assert_uint_eq(size, 2);
	ck_assert_str_eq(migrations[0], diff_add_dup.deltas[0].new_file.path);
	ck_assert_str_eq(migrations[1], diff_add_dup.deltas[1].new_file.path);
	free(migrations[1]);
	free(migrations[0]);
	free(migrations);
}
END_TEST

/**
 * Test that git_find_migrations() yields the correct result
 * with 1 ADDED + 1 DELETED for the same file, and a RENAMED
 * delta for the same file.
 */
START_TEST(git_find_migrations_add_del_rn)
{
	size_t size;
	int head = 1234;

	giterr_last_returns            = NULL;
	git_revparse_single_out_head   = &head;
	git_revparse_single_out        = &head;
	git_revwalk_next_returns       = 2;
	git_commit_lookup_commit       = (git_commit *)1234;
	git_commit_parent_commit       = (git_commit *)5678;
	git_commit_parentcount_returns = 1;
	git_diff_tree_to_tree_diff     = &diff_add_del_rn;
	memcpy(config.migration_path, "/tmp", 5);
	*local_head = '\0';

	ck_assert_ptr_null(git_find_migrations("1", NULL, &size));
	ck_assert_uint_eq(size, 0);
}
END_TEST

/**
 * Test that git_find_migrations() yields the correct result
 * with 1 ADDED delta and 1 RENAMED delta for a different
 * file.
 */
START_TEST(git_find_migrations_add_xrn)
{
	size_t size;
	int head = 1234;
	char **migrations;

	giterr_last_returns            = NULL;
	git_revparse_single_out_head   = &head;
	git_revparse_single_out        = &head;
	git_revwalk_next_returns       = 2;
	git_commit_lookup_commit       = (git_commit *)1234;
	git_commit_parent_commit       = (git_commit *)5678;
	git_commit_parentcount_returns = 1;
	git_diff_tree_to_tree_diff     = &diff_add_xrn;
	memcpy(config.migration_path, "/tmp", 5);
	*local_head = '\0';

	ck_assert_ptr_nonnull(migrations = git_find_migrations("1", NULL, &size));
	ck_assert_uint_eq(size, 1);
	ck_assert_str_eq(*migrations, diff_add_xrn.deltas[0].new_file.path);
	free(*migrations);
	free(migrations);
}
END_TEST

/**
 * Test that git_find_migrations() yields the correct result
 * with 1 ADDED delta and 1 DELETED delta with a empty
 * old file name.
 */
START_TEST(git_find_migrations_add_del_empty_filename)
{
	size_t size;
	int head = 1234;
	char **migrations;

	giterr_last_returns            = NULL;
	git_revparse_single_out_head   = &head;
	git_revparse_single_out        = &head;
	git_revwalk_next_returns       = 2;
	git_commit_lookup_commit       = (git_commit *)1234;
	git_commit_parent_commit       = (git_commit *)5678;
	git_commit_parentcount_returns = 1;
	git_diff_tree_to_tree_diff     = &diff_add_del_empty;
	memcpy(config.migration_path, "/tmp", 5);
	*local_head = '\0';

	ck_assert_ptr_nonnull(migrations = git_find_migrations("1", NULL, &size));
	ck_assert_uint_eq(size, 1);
	ck_assert_str_eq(*migrations, diff_add_del_empty.deltas[0].new_file.path);
	free(*migrations);
	free(migrations);
}
END_TEST

Suite *source_git_suite(void)
{
	Suite *s;
	TCase *t;

	s = suite_create("Migration Source: Git");
	t = tcase_create("git_init");
	tcase_add_checked_fixture(t, reset_libgit2_stubs, NULL);
	tcase_add_test(t, test_git_init_uninit);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	t = tcase_create("git_get_head");
	tcase_add_checked_fixture(t, reset_libgit2_stubs, NULL);
	tcase_add_test(t, test_git_get_head);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	t = tcase_create("git_get_migration_path");
	tcase_add_checked_fixture(t, reset_libgit2_stubs, NULL);
	tcase_add_test(t, test_git_get_migration_path);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	t = tcase_create("git_find_migrations");
	tcase_add_checked_fixture(t, reset_libgit2_stubs, NULL);
	tcase_add_test(t, git_find_migrations_null_size);
	tcase_add_test(t, git_find_migrations_no_migration_path);
	tcase_add_test(t, git_find_migrations_open_repo_fails);
	tcase_add_test(t, git_find_migrations_parse_head_fails);
	tcase_add_test(t, git_find_migrations_parse_current_head_fails);
	tcase_add_test(t, git_find_migrations_revwalk_new_fails);
	tcase_add_test(t, git_find_migrations_revwalk_push_head_fails);
	tcase_add_test(t, git_find_migrations_revwalk_hide_fails);
	tcase_add_test(t, git_find_migrations_revwalk_next_no_revs);
	tcase_add_test(t, git_find_migrations_handles_libgit2_error_messages);
	tcase_add_test(t, git_find_migrations_commit_lookup_fails);
	tcase_add_test(t, git_find_migrations_zero_parents);
	tcase_add_test(t, git_find_migrations_two_parents);
	tcase_add_test(t, git_find_migrations_cant_resolve_parent);
	tcase_add_test(t, git_find_migrations_null_parent);
	tcase_add_test(t, git_find_migrations_cant_resolve_tree);
	tcase_add_test(t, git_find_migrations_cant_resolve_parent_tree);
	tcase_add_test(t, git_find_migrations_tree_to_tree_diff_fails);
	tcase_add_test(t, git_find_migrations_find_similar_fails);
	tcase_add_test(t, git_find_migrations_zero_deltas);
	tcase_add_test(t, git_find_migrations_one_renamed);
	tcase_add_test(t, git_find_migrations_one_deleted);
	tcase_add_test(t, git_find_migrations_one_added);
	tcase_add_test(t, git_find_migrations_one_added_and_deleted);
	tcase_add_test(t, git_find_migrations_one_added_and_renamed);
	tcase_add_test(t, git_find_migrations_two_added);
	tcase_add_test(t, git_find_migrations_add_del_rn);
	tcase_add_test(t, git_find_migrations_add_xrn);
	tcase_add_test(t, git_find_migrations_add_del_empty_filename);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	return s;
}

