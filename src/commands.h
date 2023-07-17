/**
 * \file commands.h
 *
 * Minimal Migration Manager - Command Processing
 * Copyright (C) 2015 Tim Hentenaar.
 *
 * This code is licenced under the Simplified BSD License.
 * See the LICENSE file for details.
 */
#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdlib.h>

/**
 * \def COMMAND_INVALID_ARGS
 *
 * A constant for the purpose of differentiating "invalid args"
 * from success and failure for run_command().
 *
 * The values of EXIT_SUCCESS and EXIT_FAILURE may be 0 and 1, or they
 * may not be. Thus, we try to find a constant that doesn't conflict
 * with either.
 */
#define COMMAND_INVALID_ARGS (((EXIT_SUCCESS + EXIT_FAILURE) << 2) + 2)

/**
 * \def COMMAND_NOT_FOUND
 *
 * A constant for the purpose of differentiating "command not found"
 * from success and failure for run_command().
 */
#define COMMAND_NOT_FOUND (((EXIT_SUCCESS + EXIT_FAILURE) << 2) + 3)

/**
 * Run a command.
 *
 * \param[in] source Migration source
 * \param[in] argc   Number of arguments
 * \param[in] argv   Arguments (argv[0] = command to run)
 * \return EXIT_SUCCESS on success, COMMAND_INVALID_ARGS if the args
 *         aren't valid for the command, and EXIT_FAILURE if the
 *         command fails.
 */
int run_command(const char *source, int argc, char *argv[]);

#endif /* COMMANDS_H */
