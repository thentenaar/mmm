/**
 * Minimal Migration Manager - Main Routines
 * Copyright (C) 2015 Tim Hentenaar.
 *
 * This code is licenced under the Simplified BSD License.
 * See the LICENSE file for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "file.h"
#include "config.h"
#include "db.h"
#include "source.h"
#include "commands.h"
#include "state.h"
#include "config_gen.h"
#include "stringbuf.h"
#include "utils.h"

#ifdef USE_OPENSSL
#include <openssl/ssl.h>
#include <openssl/err.h>
static void uninit_openssl(void);
#endif

/* Default config file path */
static const char *default_config = "mmm.conf";

/**
 * Help Text
 */
static const char *usage_1 =
    "Minimal Migration Manager (mmm) 1.0\n"
    "Usage: %s [-h] [-f config_file] command [command options...]\n\n"
    "  Options:\n"
    "    -h              Show this message.\n"
    "    -f config_file  Configuration file to use.\n"
    "                    (defaults to ./%s)\n\n"
    "  Commands:\n"
    "     init [config]       Create a new migration directory and config\n"
    "                         file unless 'config' is specified, which\n"
    "                         creates a config file only.\n";

static const char *usage_2 =
    "     seed <seed file>    Seed the database with a .sql file.\n"
    "     pending             List all migrations yet unapplied.\n"
    "     migrate             Apply all pending migrations.\n"
    "     rollback [revision] Unapply all migrations since <revision>\n"
    "                         which defaults to the current previous\n"
    "                         revision.\n"
    "     assimilate          Track an existing database, assuming\n"
    "                         that all migrations have been applied.\n";

/**
 * Configurable parameters.
 */
static struct config {
	char file[256];        /**< Config file path */
	char source[10];       /**< Migration source */
	char driver[10];       /**< Database driver */
	char host[256];        /**< Database host */
	unsigned short port;   /**< Database port */
	char username[50];     /**< Database username */
	char password[50];     /**< Database password */
	char db[256];          /**< Database name */
	size_t history;        /**< Number of states to keep */
} config;

/**
 * Handle configuration options from the [main] section.
 */
static void main_config(void)
{
	CONFIG_SET_STRING("source", 6, config.source);
	CONFIG_SET_STRING("driver", 6, config.driver);
	CONFIG_SET_STRING("host", 4, config.host);
	CONFIG_SET_NUMBER("port", 4, config.port);
	CONFIG_SET_STRING("username", 8, config.username);
	CONFIG_SET_STRING("password", 8, config.password);
	CONFIG_SET_STRING("db", 2, config.db);
	CONFIG_SET_NUMBER("history", 7, config.history);
}

/**
 * Load the config, and check for required parameters.
 */
static int load_config(void)
{
	int retval = 0;
	size_t size;
	const char *conf;

	/* Load the config file */
	conf = map_file(config.file, &size);
	if (!conf) goto err;

	if (parse_config(conf, size)) {
		ERROR("failed to parse config");
		exit(EXIT_FAILURE);
	}

	/* Check for source and driver */
	if (!*config.source) {
		ERROR("no source specified in config");
		goto err;
	}

	if (!*config.driver) {
		ERROR("no driver specified in config");
		goto err;
	}

	/* If history wasn't specified, default to 3. */
	if (config.history == SIZE_MAX)
		config.history = 3;

ret:
	unmap_file();
	return retval;
err:
	++retval;
	goto ret;
}

/**
 * Handle command-line switches.
 *
 * \return -1 on error, otherwise the position within \a argv where
 *        the actual command starts.
 */
static int parse_args(int argc, char *argv[])
{
	int n_args = 1;
	size_t len;

	do {
		/* End of switches */
		if (argv[n_args][0] != '-')
			break;

		/* Show the help text (-h) */
		if (argv[n_args][1] == 'h')
			goto err;

		/* ... or --help */
		if (argv[n_args][1] == '-' &&
		    argv[n_args][2] == 'h')
		    goto err;

		/* Set the config file path */
		if (argv[n_args][1] == 'f') {
			len = strlen(argv[++n_args]) + 1;
			if (len <= sizeof(config.file))
				memcpy(config.file, argv[n_args], len);
		}
	} while (++n_args < argc);
	return n_args;

err:
	return -1;
}

static void usage(const char *progname);

int main(int argc, char *argv[])
{
	int n_args;
	int retval = EXIT_SUCCESS;

	/* We must have at least one arg. */
	if (argc == 1) goto show_usage;

	/* Initialize the config area */
	memset(&config, 0, sizeof(config));
	config.history = SIZE_MAX;
	memcpy(config.file, default_config, strlen(default_config) + 1);

	/* Parse command-line options */
	if ((n_args = parse_args(argc, argv)) < 0)
		goto show_usage;
	if (n_args) argc -= n_args;

	/**
	 * Check for the 'init' command so we can execute it
	 * BEFORE loading the config, etc.
	 */
	if (argc >= 1 && !strcmp(argv[n_args], "init")) {
		if (argc > 1 && !strcmp(argv[n_args + 1], "config"))
			retval = generate_config(config.file, 1);
		else retval = generate_config(config.file, 0);

		if (retval) goto err;
		goto ret;
	}

	/* Initialize subsystems */
	sbuf_reset(1);
	db_init();
	source_init();
	config_init(main_config);
	if (load_config())
		goto err;
	if (state_init(config.history))
		goto err;

	/* Ensure source is valid */
	if (!source_get_config_cb(config.source, strlen(config.source))) {
		ERROR_1("unknown source: %s", config.source);
		goto err;
	}

	/* Connect to the database */
	if (db_connect(config.driver, config.host, config.port,
	               config.username, config.password, config.db)) {
		ERROR("failed to connect to the database");
		goto err;
	}

	/* Run the specified command */
	retval = run_command(config.source, argc, &argv[n_args]);
	if (retval == COMMAND_INVALID_ARGS) {
		if (argv[n_args]) {
			ERROR_1("%s: invalid command", argv[n_args]);
		} else {
			ERROR("invalid command");
		}
	}

ret:
	/* Clean up */
	db_disconnect();
	state_uninit();
	source_uninit();
	db_uninit();

#ifdef USE_OPENSSL
	uninit_openssl();
#endif
	return retval;

show_usage:
	usage(argv[0]);

err:
	exit(EXIT_FAILURE);
}

/* {{{ GCC >= 3.x: ignore -Wformat-security here
 * We know that the contents of usage are okay, even if they
 * aren't passed as string literals.
 */
#ifdef __GNUC__
#if (__GNUC__ >= 3) && (((__GNUC__ * 100) + __GNUC_MINOR__) < 402)
#pragma GCC system_header
#endif /* GCC >= 3 && GCC <= 4.2 */
#if ((__GNUC__ * 100) + __GNUC_MINOR__) >= 406
#pragma GCC diagnostic push
#endif /* GCC >= 4.6 */
#if ((__GNUC__ * 100) + __GNUC_MINOR__) >= 402
#pragma GCC diagnostic ignored "-Wformat-security"
#endif /* GCC >= 4.2 */
#endif /* }}} */

/**
 * Show usage info.
 */
static void usage(const char *progname)
{
	printf(usage_1, progname, default_config);
	puts(usage_2);
}

/* {{{ GCC >= 4.6: restore -Wformat-security */
#ifdef __GNUC__
#if ((__GNUC__ * 100) + __GNUC_MINOR__) >= 406
#pragma GCC diagnostic pop
#endif /* GCC >= 4.6 */
#endif /* }}} */

#ifdef USE_OPENSSL
/**
 * If any libraries we're using are linking against
 * OpenSSL, we need to do some cleanup to avoid
 * potentially leaking memory. However, if the libraries
 * we're using don't cleanup the SSL contexts they allocate
 * we may still leak a few related blocks here and there.
 *
 * Damn OpenSSL and its lack of a coherent initialization /
 * uninitialization API to hell and back again...
 *
 * See:
 * https://wiki.openssl.org/index.php/Library_Initialization
 */
static void uninit_openssl(void)
{
	EVP_cleanup();
	CRYPTO_cleanup_all_ex_data();
	ERR_remove_state(0);
	ERR_free_strings();
}
#endif
