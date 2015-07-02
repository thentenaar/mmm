/**
 * \file file.h
 *
 * Minimal Migration Manager - File-mapping Functions
 * Copyright (C) 2015 Tim Hentenaar.
 *
 * This code is licenced under the Simplified BSD License.
 * See the LICENSE file for details.
 */
#ifndef FILE_H
#define FILE_H

/**
 * Read a file into our buffer.
 *
 * This will fail for any file which has a size too large
 * to hold in a off_t. The files this program uses shouldn't
 * be that large anyhow.
 *
 * \param[in]  path Path to the file to be mapped.
 * \param[out] size Size of the file (in bytes.)
 * \return A pointer to the file buffer. On error, NULL will
 *         be returned, and size will be set to 0.
 */
char *map_file(const char *path, size_t *size);

/**
 * Unmap the previously mapped file.
 */
void unmap_file(void);

#endif /* FILE_H */
