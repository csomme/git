#include "git-compat-util.h"
#include "refs.h"
#include "refs/refs-internal.h"
#include "exec-cmd.h"
#include "dlfcn.h"

/* Handle for the dynamically loaded Rust backend */
static void *rust_backend_handle = NULL;

/* Storage for the loaded Rust backend */
static struct ref_storage_be rust_refs_be = { .name = "rust" };

/* 
 * Load the Rust backend library dynamically.
 * Returns 0 on success, -1 on failure.
 */
int load_rust_backend(void)
{
	static int loaded = 0;
	if (loaded)
		return 0;

	const char *path = getenv("GIT_RUST_REFS_BACKEND");
	if (!path) {
		const char *exec_path = git_exec_path();
		static char backend_path[PATH_MAX];
		if (exec_path) {
			snprintf(backend_path, sizeof(backend_path), "%s/librefs_rs.so", exec_path);
			path = backend_path;
		} else {
			path = "librefs_rs.so";  // Fallback
		}
	}

	rust_backend_handle = dlopen(path, RTLD_NOW);
	if (!rust_backend_handle) {
		warning("Failed to load Rust refs backend from %s: %s", path, dlerror());
		return -1;
	}

	const struct ref_storage_be *(*init_fn)(void) = dlsym(rust_backend_handle, "refs_backend_init");
	if (!init_fn) {
		warning("Rust refs backend at %s does not export refs_backend_init: %s",
			path, dlerror());
		dlclose(rust_backend_handle);
		rust_backend_handle = NULL;
		return -1;
	}

	const struct ref_storage_be *be = init_fn();
	if (!be) {
		warning("Rust refs backend initialization failed");
		dlclose(rust_backend_handle);
		rust_backend_handle = NULL;
		return -1;
	}

	/* Copy the backend's structure, preserving the name */
	const char *name = rust_refs_be.name;
	memcpy(&rust_refs_be, be, sizeof(rust_refs_be));
	rust_refs_be.name = name;

	loaded = 1;
	return 0;
}

/* 
 * Function to unload the Rust backend when Git exits.
 * Should be called during cleanup.
 */
void unload_rust_backend(void)
{
	if (rust_backend_handle) {
		dlclose(rust_backend_handle);
		rust_backend_handle = NULL;
	}
}

/* 
 * Get the Rust backend reference.
 * Returns NULL if the backend couldn't be loaded.
 */
const struct ref_storage_be *get_rust_backend(void)
{
	if (load_rust_backend() < 0)
		return NULL;
	return &rust_refs_be;
}
