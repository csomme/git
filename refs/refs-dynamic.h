//
// Created by Chris Somme on 4/25/25.
//

#ifndef GIT_REFS_DYNAMIC_H
#define GIT_REFS_DYNAMIC_H

struct ref_storage_be;

/**
 * Load the Rust backend library.
 * Returns 0 on success, -1 on failure.
 */
int load_rust_backend(void);

/**
 * Get the Rust backend reference.
 * Returns NULL if the backend couldn't be loaded.
 */
const struct ref_storage_be *get_rust_backend(void);

/**
 * Unload the Rust backend.
 */
void unload_rust_backend(void);

#endif // GIT_REFS_DYNAMIC_H
