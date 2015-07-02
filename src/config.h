/**
 * \file config.h
 *
 * Minimal Migration Manager - Configuration Parser
 * Copyright (C) 2015 Tim Hentenaar.
 *
 * This code is licenced under the Simplified BSD License.
 * See the LICENSE file for details.
 */
#ifndef CONFIG_H
#define CONFIG_H

/**
 * Type values for config_set_value().
 */
#define CONFIG_STRING 0
#define CONFIG_NUMBER 1

/**
 * \def CONFIG_SET_STRING
 *
 * Convenience wrapper around config_set_value() for
 * string values.
 */
#define CONFIG_SET_STRING(k, klen, dest) do {\
	if (!config_set_value(CONFIG_STRING, (dest), sizeof((dest)), (k),\
	                      (klen)))\
		return;\
} while(0);

/**
 * \def CONFIG_SET_NUMBER
 *
 * Convenience wrapper around config_set_value() for
 * unsigned integer values.
 */
#define CONFIG_SET_NUMBER(k, klen, dest) do {\
	if (!config_set_value(CONFIG_NUMBER, &(dest), sizeof((dest)),\
	                      (k), (klen)))\
		return;\
} while(0);

/**
 * Callback for processing configuration values.
 *
 * These should be handled with the CONFIG_SET_* macros.
 */
typedef void (*config_callback_t)(void);

/**
 * Parse the config, line-by-line.
 *
 * This approach doesn't modify config or duplicate
 * any part of it on the heap. The idea is to scan
 * the config file once, and then let the callback
 * decide what to do with it.
 *
 * \param[in] config Configuration file contents
 * \param[in] len    Length of \a config.
 * \return 0 on success, non-zero on error.
 */
int parse_config(const char *config, size_t len);

/**
 * Set the main configuration callback.
 *
 * \param[in] main_config_cb Main config. callback
 */
void config_init(config_callback_t main_config_cb);

/**
 * Convenience function for setting a value from the config.
 *
 * If key matches expected_key, strings are copied into place,
 * and numbers are interpreted as unsigned.
 *
 * \param[in] type         Expected type of the value.
 * \param[in] dest         Destination memory area.
 * \param[in] dest_size    Size of the destination memory area.
 * \param[in] expected_key If key matches this, the value is set.
 * \param[in] key_len      Length of \a expected_key.
 * \return 0 on success, non-zero on error.
 */
int config_set_value(int type, void *dest, size_t dest_size,
                     const char *expected_key, size_t key_len);

#endif /* CONFIG_H */
