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
 * Map a file into memory
 *
 * \param[in]  path   Path to the file to be mapped.
 * \param[out] size   Size of the file (in bytes.)
 * \return A pointer to the file buffer. On error, NULL will
 *         be returned, and size will be set to 0.
 */
char *map_file(const char *path, size_t *size);

/**
 * Unmap a previously mapped file
 *
 * \param[in] mem Base address of the file mapping
 * \param[in] len Length of the file
 */
void unmap_file(char *mem, size_t len);

#endif /* FILE_H */
