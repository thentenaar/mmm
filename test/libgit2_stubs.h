/**
 * Minimal Migration Manager - libgit2 stubs for tests
 * Copyright (C) 2015 Tim Hentenaar.
 *
 * This code is licenced under the Simplified BSD License.
 * See the LICENSE file for details.
 */
#ifndef TEST_LIBGIT2_STUBS_H
#define TEST_LIBGIT2_STUBS_H

#include <stdio.h>

/* {{{ GCC: Disable warnings
 *
 * The parameters in the functions below are intentionally unused,
 * and not all functions may be used in the translation unit including
 * this file. Thus, we want GCC to see this as a system header and
 * not complain about unused functions and the like.
 */
#if defined(__GNUC__) && __GNUC__ >= 3
#pragma GCC system_header
#endif /* GCC >= 3 }}} */

/* {{{ libgit2 structs and macros */
#define LIBGIT2_VER_MAJOR 0
#define LIBGIT2_VER_MINOR 9999

#define GIT_OK 0
#define GIT_SORT_REVERSE 1
#define GIT_SORT_TOPOLOGICAL 2
#define GIT_DIFF_OPTIONS_VERSION 1
#define GIT_DIFF_FIND_OPTIONS_VERSION 1

typedef struct git_sa {
	char **strings;
	size_t count;
} git_strarray;

typedef enum {
	GIT_DIFF_IGNORE_FILEMODE = (1u << 8),
	GIT_DIFF_SKIP_BINARY_CHECK = (1u << 13),
	GIT_DIFF_IGNORE_WHITESPACE = (1u << 22),
	GIT_DIFF_IGNORE_WHITESPACE_CHANGE = (1u << 23),
	GIT_DIFF_IGNORE_WHITESPACE_EOL = (1u << 24)
} git_diff_option_t;

typedef struct git_diff_opts {
	int x;
	unsigned long flags;
	git_strarray pathspec;
} git_diff_options;

typedef enum {
	GIT_DIFF_FIND_IGNORE_WHITESPACE = (1u << 12),
	GIT_DIFF_FIND_EXACT_MATCH_ONLY = (1u << 14)
} git_diff_find_t;

typedef struct git_find_opts {
	int x;
	unsigned long flags;
} git_diff_find_options;

typedef enum {
	GIT_DELTA_NONE = 0,
	GIT_DELTA_ADDED = 1,
	GIT_DELTA_DELETED = 2,
	GIT_DELTA_RENAMED = 4
} git_delta_t;

typedef struct git_diff_f {
	char path[10];
} git_diff_file;

typedef struct git_del {
	git_delta_t status;
	git_diff_file old_file;
	git_diff_file new_file;
} git_diff_delta;

typedef struct git_diff_rec {
	size_t num_deltas;
	git_diff_delta deltas[3];
} git_diff;

typedef int git_oid;
typedef int git_repository;
typedef int git_object;
typedef int git_revwalk;
typedef int git_commit;
typedef int git_tree;

static struct git_err {
	const char *message;
};
/* }}} */

/* {{{ libgit2 stub return values */
static struct git_err last_giterr = { NULL };

static struct git_err *giterr_last_returns = &last_giterr;
static git_commit *git_commit_lookup_commit = NULL;
static git_commit *git_commit_parent_commit = NULL;
static git_tree *git_commit_tree_tree = NULL;
static git_tree *git_commit_tree_parent_tree = NULL;
static git_diff *git_diff_tree_to_tree_diff = NULL;
static git_repository *git_repository_open_repo = NULL;
static git_object *git_revparse_single_out = NULL;
static git_object *git_revparse_single_out_head = NULL;
static int git_commit_lookup_returns = GIT_OK;
static int git_commit_parentcount_returns = 1;
static int git_commit_parent_returns = GIT_OK;
static int git_commit_tree_returns = GIT_OK;
static int git_commit_tree_parent_returns = GIT_OK;
static int git_diff_tree_to_tree_returns = GIT_OK;
static int git_diff_find_similar_returns = GIT_OK;
static int git_repository_open_returns = GIT_OK;
static int git_revparse_single_returns = GIT_OK;
static int git_revparse_single_returns_head = GIT_OK;
static int git_revwalk_new_returns = GIT_OK;
static int git_revwalk_push_returns = GIT_OK;
static int git_revwalk_hide_returns = GIT_OK;
static int git_revwalk_next_returns = 1;

static void reset_libgit2_stubs(void)
{
	last_giterr.message = NULL;
	giterr_last_returns = &last_giterr;
	git_commit_lookup_commit = NULL;
	git_commit_parent_commit = NULL;
	git_commit_tree_tree = NULL;
	git_commit_tree_parent_tree = NULL;
	git_diff_tree_to_tree_diff = NULL;
	git_repository_open_repo = NULL;
	git_revparse_single_out = NULL;
	git_revparse_single_out_head = NULL;
	git_commit_lookup_returns = GIT_OK;
	git_commit_parentcount_returns = 1;
	git_commit_parent_returns = GIT_OK;
	git_commit_tree_returns = GIT_OK;
	git_commit_tree_parent_returns = GIT_OK;
	git_diff_tree_to_tree_returns = GIT_OK;
	git_diff_find_similar_returns = GIT_OK;
	git_repository_open_returns = GIT_OK;
	git_revparse_single_returns = GIT_OK;
	git_revparse_single_returns_head = GIT_OK;
	git_revwalk_new_returns = GIT_OK;
	git_revwalk_push_returns = GIT_OK;
	git_revwalk_hide_returns = GIT_OK;
	git_revwalk_next_returns = -1;
}
/* }}} */

/* {{{ libgit2 lesser stub functions */
static int git_libgit2_init(void)
{
	return 0;
}

static int git_libgit2_shutdown(void)
{
	return 0;
}

static void git_diff_init_options(git_diff_options *opts,
                                  int version)
{
	opts->x = version;
}

static void git_diff_find_init_options(git_diff_find_options *opts,
                                       int version)
{
	opts->x = version;
}

static int git_commit_lookup(git_commit **c, git_repository *repo,
                             git_oid *oid)
{
	*c = git_commit_lookup_commit;
	return git_commit_lookup_returns;
}

static int git_commit_parentcount(git_commit *c)
{
	return git_commit_parentcount_returns;
}

static int git_commit_parent(git_commit **p, git_commit *c, int x)
{
	*p = git_commit_parent_commit;
	return git_commit_parent_returns;
}

static int git_commit_tree(git_tree **tree, git_commit *c)
{
	int retval = git_commit_tree_returns;

	if (c == git_commit_parent_commit) {
		*tree = git_commit_tree_parent_tree;
		retval = git_commit_tree_parent_returns;
	} else *tree = git_commit_tree_tree;
	return retval;
}

static int git_diff_find_similar(git_diff *diff,
                                 git_diff_find_options *opts)
{
	return git_diff_find_similar_returns;
}

static int git_repository_open(git_repository **repo, const char *path)
{
	*repo = git_repository_open_repo;
	return git_repository_open_returns;
}

static git_oid git_object_id(git_object *obj)
{
	return *obj;
}

static void git_oid_tostr(char *buf, size_t bufsize, git_oid oid)
{
	sprintf(buf, "%d", oid);
}

static int git_revwalk_new(git_revwalk **walk, git_repository *repo)
{
	return git_revwalk_new_returns;
}

static int git_revwalk_push(git_revwalk *walk, git_oid oid)
{
	return git_revwalk_push_returns;
}

static int git_revwalk_hide(git_revwalk *walk, git_oid o)
{
	return git_revwalk_hide_returns;
}

static void git_revwalk_sorting(git_revwalk *walk, unsigned long flags)
{
	return;
}

static void git_tree_free(git_tree *tree)
{
	return;
}

static void git_diff_free(git_diff *diff)
{
	return;
}

static void git_commit_free(git_commit *commit)
{
	return;
}

static void git_object_free(git_object *o)
{
	return;
}

static void git_revwalk_free(git_revwalk *walk)
{
	return;
}

static void git_repository_free(git_repository *repo)
{
	return;
}
/* }}} */

/* {{{ libgit2 greater stub functions */
static struct git_err *giterr_last(void)
{
	return giterr_last_returns;
}

static int git_revparse_single(git_object **out, git_repository *repo,
                               const char *rev)
{
	int retval = git_revparse_single_returns_head;

	if (*rev == 'H' && *(rev+1) == 'E') {
		*out = git_revparse_single_out_head;
	} else {
		*out = git_revparse_single_out;
		retval = git_revparse_single_returns;
	}

	return retval;
}

static int git_diff_tree_to_tree(git_diff **diff, git_repository *repo,
                                 git_tree *pt, git_tree *t,
                                 git_diff_options *ppts)
{
	*diff = git_diff_tree_to_tree_diff;
	return git_diff_tree_to_tree_returns;
}

static size_t git_diff_num_deltas(git_diff *diff)
{
	return (diff ? diff->num_deltas : 0);
}

static git_diff_delta *git_diff_get_delta(git_diff *diff, size_t i)
{
	return &diff->deltas[i];
}

static int git_revwalk_next(git_oid *o, git_revwalk *walk)
{
	return (--git_revwalk_next_returns == 0);
}

/* }}} */

#endif /* TEST_LIBGIT2_STUBS_H */
