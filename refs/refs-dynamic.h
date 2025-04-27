//
// Created by Chris Somme on 4/25/25.
//

#ifndef GIT_REFS_DYNAMIC_H
#define GIT_REFS_DYNAMIC_H

struct ref_storage_be;

/*
 * Load Rust backends and register them with Git.
 * Returns 0 on success, -1 on failure.
 */
int load_rust_backends(void);

#endif // GIT_REFS_DYNAMIC_H
