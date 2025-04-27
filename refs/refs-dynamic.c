#include "git-compat-util.h"
#include "refs.h"
#include "refs/refs-internal.h"
#include "exec-cmd.h"
#include "dlfcn.h"
#include "trace2.h"

/*
 * Dynamically load the Rust backend library.
 * Returns a handle to the loaded library, or NULL on failure.
 */
static void *load_rust_library(void)
{
	static void *rust_handle = NULL;
	/* Return cached handle if already loaded */
	if (rust_handle)
		return rust_handle;

	/* Determine path to the Rust library */
	const char *path = getenv("GIT_RUST_REFS_LIB");
	if (!path) {
		const char *exec_path = git_exec_path();
		static char lib_path[PATH_MAX];
		if (exec_path) {
			snprintf(lib_path, sizeof(lib_path), "%s/librefs_rs.so", exec_path);
			path = lib_path;
		} else {
			path = "librefs_rs.so";  /* Fallback */
		}
	}

	/* Load the library */
	rust_handle = dlopen(path, RTLD_NOW);
	if (!rust_handle) {
		trace2_data_string("refs", NULL, "rust-load-error", dlerror());
		return NULL;
	}

	return rust_handle;
}

/*
 * Load Rust backends and register them with Git.
 * Returns 0 on success, -1 on failure.
 */
int load_rust_backends(void)
{
	void *handle = load_rust_library();
	if (!handle)
		return -1;

	/* Get functions to discover backends */
	int (*count_fn)(void) = dlsym(handle, "rust_ref_backends_count");
	void *(*backend_fn)(int) = dlsym(handle, "rust_ref_backend_by_index");
	const char *(*name_fn)(int) = dlsym(handle, "rust_ref_backend_name_by_index");

	if (!count_fn || !backend_fn || !name_fn) {
		warning("Required Rust backend functions not found");
		return -1;
	}

	/* Register each backend */
	int count = count_fn();
	for (int i = 0; i < count; i++) {
		void *backend = backend_fn(i);
		const char *name = name_fn(i);

		if (backend && name) {
			enum ref_storage_format format = refs_register_backend(name, backend);
			if (format == REF_STORAGE_FORMAT_UNKNOWN) {
				warning("Failed to register Rust backend '%s'", name);
			}
		}
	}

	return 0;
}

