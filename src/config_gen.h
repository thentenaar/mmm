/**
 * \file config_gen.h
 *
 * Minimal Migration Manager - Configuration Generator
 * Copyright (C) 2015 Tim Hentenaar.
 *
 * This code is licenced under the Simplified BSD License.
 * See the LICENSE file for details.
 */
#ifndef CONFIG_GEN_H
#define CONFIG_GEN_H

/**
 * Create the intial migrations folders and config.
 *
 * \param[in] config_file Default config file name
 * \param[in] config_only If zero, create the migration dirs too.
 * \return 0 on success, 1 on failure.
 */
int generate_config(const char *config_file, int config_only);

#endif /* CONFIG_GEN_H */
