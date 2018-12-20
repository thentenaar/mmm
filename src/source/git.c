/**
 * Minimal Migration Manager - Git Migration Source
 * Copyright (C) 2015 Tim Hentenaar.
 *
 * This code is licenced under the Simplified BSD License.
 * See the LICENSE file for details.
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifndef IN_TESTS
#include <git2.h>
#endif

#include "backend.h"
#include "../config.h"
#include "../utils.h"

/* {{{ libgit2 < 0.22.0 compatibility macros */
/**
 * git_threads_init and git_threads_shutdown were renamed
 * in libgit2 0.22.0.
 */
#ifndef IN_TESTS
#include <git2/version.h>

#if LIBGIT2_VER_MAJOR == 0 && LIBGIT2_VER_MINOR < 22
#define git_libgit2_init git_threads_init
#define git_libgit2_shutdown git_threads_shutdown
#endif
#endif /* IN_TESTS }}} */

/**
 * Configuration values:
 *
 * migration_path - Path to migrations, relative to the repo.
 * repo_path      - Path to the git repository (default: ".")
 */
static struct config {
	char migration_path[256];
	char repo_path[256];
} config;

/* Representation of the path to libgit2's diff routines. */
static char *migration_paths[2] = {
	config.migration_path,
	NULL
};

static const git_strarray migration_strs = {
	migration_paths,
	1
};

/* The local HEAD revision ID */
static char local_head[50];

/**
 * A list of migrations corresponding to
 * one revision.
 */
struct mlist {
	size_t size;
	char **migrations;
	struct mlist *next;
};

static struct mlist *mlist_head = NULL;
static struct mlist *mlist_tail = NULL;
static git_diff_options opts;
static git_diff_find_options findopts;

/* {{{ static void initialize_diff_options(void) */
static void initialize_diff_options(void)
{
#if LIBGIT2_VER_MAJOR > 0 ||\
    (LIBGIT2_VER_MAJOR == 0 && LIBGIT2_VER_MINOR > 20)
	git_diff_init_options(&opts, GIT_DIFF_OPTIONS_VERSION);
#else
	memset(&opts, 0, sizeof(opts));
	opts.version = GIT_DIFF_OPTIONS_VERSION;
	opts.ignore_submodules = GIT_SUBMODULE_IGNORE_DEFAULT;
	opts.context_lines = 3;
#endif
	opts.flags |= GIT_DIFF_IGNORE_WHITESPACE_EOL |
	    GIT_DIFF_IGNORE_WHITESPACE_CHANGE |
	    GIT_DIFF_IGNORE_WHITESPACE |
#ifdef GIT_DIFF_IGNORE_FILE_MODE
	    GIT_DIFF_IGNORE_FILE_MODE |
#endif
	    GIT_DIFF_SKIP_BINARY_CHECK;

	/* Migration pathspec (from config) */
	opts.pathspec = migration_strs;

	/* Set the diff find options */
#if LIBGIT2_VER_MAJOR > 0 ||\
    (LIBGIT2_VER_MAJOR == 0 && LIBGIT2_VER_MINOR > 20)
	git_diff_find_init_options(&findopts,
	                           GIT_DIFF_FIND_OPTIONS_VERSION);
#else
	memset(&findopts, 0, sizeof(findopts));
	findopts.version = GIT_DIFF_FIND_OPTIONS_VERSION;
#endif
	findopts.flags |= GIT_DIFF_FIND_IGNORE_WHITESPACE |
	    GIT_DIFF_FIND_EXACT_MATCH_ONLY;
} /* }}} */

/* {{{ static int is_path_sql(const char *path) */
/**
 * Is this path a ".sql" file?
 *
 * \param[in] path Path to check
 * \return 1 if the path is an SQL file, 0 otherwise.
 */
static int is_path_sql(const char *path)
{
	size_t i;
	int retval = 0;

	if (!path) goto ret;

	i = strlen(path);
	if (i < 5 || memcmp(&path[i - 4], ".sql", 5))
		goto ret;
	++retval;

ret:
	return retval;
} /* }}} */

/* {{{ static int is_duplicate(const char *path) */
/**
 * Simple O(n) test for duplicates.
 *
 * \param[in] path Path to check
 * \return 1 if the path is a duplicate, 0 otherwise.
 */
static int is_duplicate(const char *path)
{
	size_t i, plen;
	struct mlist *ml;
	int retval = 0;

	plen = strlen(path);
	for (ml = mlist_head; ml; ml = ml->next) {
		for (i = 0; i < ml->size; i++) {
			if (plen != strlen(ml->migrations[i]))
				continue;

			if (!memcmp(path, ml->migrations[i], plen)) {
				++retval;
				goto ret;
			}
		}
	}

ret:
	return retval;
} /* }}} */

/**
 * Apply a rename or a delete to the migration list.
 *
 * \param[in] old_path Old path
 * \param[in] new_path New path
 * \return 0 if the modification was successful, non-zero otherwise.
 */
static int modify_mlist(const char *old_path, const char *new_path)
{
	size_t i, old_len, new_len;
	struct mlist *ml;
	int retval = 0;

	if (!old_path) goto err;

	old_len = strlen(old_path);
	if (!old_len) goto ret;

	for (ml = mlist_head; ml; ml = ml->next) {
		for (i = 0; i < ml->size; i++) {
			if (!ml->migrations[i])
				continue;
			if (strlen(ml->migrations[i]) != old_len)
				continue;
			if (!memcmp(ml->migrations[i], old_path, old_len))
				break;
		}

		if (i == ml->size)
			continue;

		/* Apply a delete */
		free(ml->migrations[i]);
		ml->migrations[i] = NULL;
		if (!new_path) continue;

		/* Apply a rename */
		new_len = strlen(new_path);
		errno = 0;
		ml->migrations[i] = malloc(new_len + 1);
		if (!ml->migrations[i]) goto err;
		memcpy(ml->migrations[i], new_path, new_len + 1);
	}

ret:
	return retval;

err:
	++retval;
	goto ret;
}

/**
 * Generate a diff between a commit and its parent.
 *
 * \param[in] repo Git repository
 * \param[in] oid  Commit object ID
 * \return A pointer to a git_diff object, or NULL on failure.
 */
static git_diff *generate_diff(git_repository *repo, git_oid oid)
{
	git_diff *diff = NULL;
	git_commit *commit = NULL, *parent = NULL;
	git_tree *tree = NULL, *parent_tree = NULL;

	/**
	 * Skip any revision which doesn't resolve to a commit
	 */
	if (git_commit_lookup(&commit, repo, &oid) != GIT_OK)
		goto ret;

	/* Skip commits that don't have 1 parent */
	if (!commit || git_commit_parentcount(commit) != 1)
		goto ret;

	if (git_commit_parent(&parent, commit, 0) != GIT_OK)
		goto ret;

	/* Diff this commit and its parent */
	if (!parent || git_commit_tree(&tree, commit) != GIT_OK
	    || git_commit_tree(&parent_tree, parent) != GIT_OK)
		goto ret;

	if (git_diff_tree_to_tree(&diff, repo, parent_tree, tree,
	                          &opts) != GIT_OK)
		goto err;

	if (git_diff_find_similar(diff, &findopts) != GIT_OK)
		goto err;

	/* Ensure we have some deltas in the diff */
	if (git_diff_num_deltas(diff) < 1)
		goto err;

ret:
	git_tree_free(parent_tree);
	git_tree_free(tree);
	git_commit_free(parent);
	git_commit_free(commit);
	return diff;

err:
	git_diff_free(diff);
	diff = NULL;
	goto ret;
}

/**
 * Process the deltas in a diff.
 *
 * \param[in] diff Diff to process
 * \return The numeber of migrations added, or SIZE_MAX on fatal error
 */
static size_t process_diff(git_diff *diff)
{
	const git_diff_delta *delta;
	struct mlist *ml;
	char **tmp;
	size_t size = 0, i, x;

	/* Allocate a new mlist for this batch */
	errno = 0;
	ml = calloc(1, sizeof(struct mlist));
	if (!ml) goto err;

	/* Add this batch to the list */
	if (!mlist_head) {
		mlist_head = mlist_tail = ml;
	} else {
		mlist_tail->next = ml;
		mlist_tail = ml;
	}

	for (i = 0; i < git_diff_num_deltas(diff); i++) {
		delta = git_diff_get_delta(diff, i);
		if (!delta) continue;

		switch (delta->status) {
		case GIT_DELTA_ADDED:
			if (is_duplicate(delta->new_file.path))
				break;

			ml->size++;
			errno = 0;
			tmp = realloc(ml->migrations,
			              ml->size * sizeof(char *));
			if (!tmp) goto err;
			ml->migrations = tmp;

			x = strlen(delta->new_file.path);
			errno = 0;
			tmp[ml->size - 1] = malloc(x + 1);
			if (!tmp[ml->size - 1])
				goto err;
			memcpy(tmp[ml->size - 1], delta->new_file.path,
			       x + 1);
			break;
		case GIT_DELTA_RENAMED:
			if (modify_mlist(delta->old_file.path,
			                 delta->new_file.path))
				goto err;
			break;
		case GIT_DELTA_DELETED:
			if (modify_mlist(delta->old_file.path, NULL))
				goto err;
			break;
		default:
			break;
		}
	}

	size = ml->size;
ret:
	return size;

err:
	if (errno == ENOMEM) {
		error("failed to allocate memory: %s",
		      strerror(ENOMEM));
	}

	size = SIZE_MAX;
	goto ret;
}

/**
 * Flatten the migration list into an array
 *
 * \param[in]  maxlen Maximum length of the array
 * \param[out] size   Actial size of the array
 * \return array of strings
 */
static char **flatten_mlist(size_t maxlen, size_t *size)
{
	size_t i, j = 0, k = 0;
	struct mlist *ml;
	char **migrations = NULL, **tmp;

	if (!maxlen) goto ret;

	++maxlen;
	errno = 0;
	migrations = malloc(maxlen * sizeof(char *));
	if (!migrations) goto malloc_err;

	/* Copy the migrations, sorting each batch. */
	for (ml = mlist_head; ml; ml = ml->next) {
		k = j;
		for (i = 0; i < ml->size; i++) {
			if (!is_path_sql(ml->migrations[i]))
				continue;
			migrations[j++] = ml->migrations[i];
		}

		if (j > k) bubblesort(&migrations[k], j - k);
	}

	/* Shrink the array if necessary */
	if (j) {
		if (maxlen > j) {
			errno = 0;
			tmp = realloc(migrations, j * sizeof(char *));
			if (!tmp) goto malloc_err;
			migrations = tmp;
		}

		*size = j;
	} else {
		free(migrations);
		migrations = NULL;
		*size = 0;
	}

ret:
	if (mlist_head) {
		for (ml = mlist_head; ml; ml = mlist_tail) {
			mlist_tail = ml->next;
			free(ml->migrations);
			free(ml);
		}

		mlist_head = mlist_tail = NULL;
	}

	return migrations;

malloc_err:
	if (errno == ENOMEM)
		error("failed to allocate memory: %s", strerror(errno));

	if (migrations) {
		while (*size) free(migrations[--*size]);
		free(migrations);
		migrations = NULL;
	} else *size = 0;

	goto ret;
}

/**
 * Provide an ordered list of migration files depending on
 * whether or not they should be applied.
 *
 * NOTE: The returned list of migrations must be freed.
 *
 * \param[in]  cur_rev  Last applied revision
 * \param[in]  prev_rev Prevous revision (for rollbacks.)
 * \param[out] size     Number of migrations in the list
 * \returns array of pointers to strings representing an ordered
 *          list of migration filenames.
 */
static char **git_find_migrations(const char *cur_rev,
                                  const char *prev_rev, size_t *size)
{
	git_oid oid;
	git_repository *repo = NULL;
	git_object *head = NULL, *prev = NULL;
	git_revwalk *walk = NULL;
	git_diff *diff = NULL;
	char **migrations = NULL;
	size_t i, j;

	if (!size) goto ret;

	*size = 0;
	if (!*config.migration_path) {
		error("no migration_path specified");
		goto ret;
	}

	/* Ensure the migration path has a trailing / */
	i = strlen(config.migration_path);
	if (config.migration_path[i - 1] != '/') {
		config.migration_path[i++] = '/';
		config.migration_path[i] = '\0';
	}

	if (git_repository_open(&repo, config.repo_path) != GIT_OK)
		goto ret;

	if (prev_rev) {
		if (git_revparse_single(&head, repo, cur_rev) != GIT_OK)
			goto ret;

		if (git_revparse_single(&prev, repo, prev_rev) != GIT_OK)
			goto ret;
	} else {
		prev_rev = "HEAD^{commit}";
		if (git_revparse_single(&head, repo, prev_rev) != GIT_OK)
			goto ret;

		if (cur_rev &&
		    git_revparse_single(&prev, repo, cur_rev) != GIT_OK)
			goto ret;
	}

	/* Save the local HEAD revision */
	if (head && git_object_id(head)) {
		git_oid_tostr(local_head, sizeof(local_head),
		              git_object_id(head));
	}

	/* Initialize options for git_diff_* */
	initialize_diff_options();

	/* Walk all revisions between the previous and current HEAD */
	if (git_revwalk_new(&walk, repo) != GIT_OK ||
	    git_revwalk_push(walk, git_object_id(head)) != GIT_OK ||
	    (prev &&
	     git_revwalk_hide(walk, git_object_id(prev)) != GIT_OK))
		goto ret;

	git_revwalk_sorting(walk, GIT_SORT_TOPOLOGICAL |
	                    GIT_SORT_REVERSE);
	i = 0;

	while (!git_revwalk_next(&oid, walk)) {
		diff = generate_diff(repo, oid);
		if (!diff) continue;

		j = process_diff(diff);
		if (j == SIZE_MAX) {
			i = 0;
			git_diff_free(diff);
			break;
		}

		i += j;
		git_diff_free(diff);
	}
	migrations = flatten_mlist(i, size);

ret:
	if (giterr_last())
		error("%s", giterr_last()->message);
	git_revwalk_free(walk);
	git_object_free(prev);
	git_object_free(head);
	git_repository_free(repo);
	return migrations;
}

/**
 * Callback for receiving configuration key/value pairs.
 *
 * Valid values for this module are:
 *
 * migration_path - Path to the directory in which migrations are stored,
 *                  relative to the root of the git repository.
 *   Vaiid values are a string less than 1024 bytes.
 *
 * repo_path - Path to the git repository.
 *   Vaiid values are a string less than 1024 bytes.
 */
static void git_configure(void)
{
	CONFIG_SET_STRING("repo_path", 9, config.repo_path);
	CONFIG_SET_STRING("migration_path", 14, config.migration_path);
}

/**
 * Get the latest local revision.
 *
 * \return The latest local revision as a string, or NULL if
 *         the local revision couldn't be determined.
 */
static const char *git_get_head(void)
{
	return (local_head[0] ? local_head : NULL);
}

/**
 * Get the base path for migrations.
 *
 * \return The base path for migrations.
 */
static const char *git_get_migration_path(void)
{
	return config.repo_path;
}

/**
 * Initialize the git source backend.
 *
 * The repo_path defaults to "." if unspecified.
 *
 * \return 0 on success, non-zero on failure.
 */
static int git_init(void)
{
	memset(&config, 0, sizeof(config));
	config.repo_path[0] = '.';
	memset(&local_head, 0, sizeof(local_head));
	return git_libgit2_init() < 0;
}

/**
 * Uninitialize the git source backend.
 *
 * \return 0 on success, non-zero on failure.
 */
static int git_uninit(void)
{
	git_libgit2_shutdown();
	return 0;
}

struct source_backend_vtable git_vtable = {
	"git",
	git_configure,
	git_init,
	git_find_migrations,
	git_get_head,
	git_get_migration_path,
	git_uninit
};
