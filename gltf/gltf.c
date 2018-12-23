/* gltf.h  -  glTF library  -  Public Domain  -  2018 Mattias Jansson / Rampant Pixels
 *
 * This library provides a cross-platform glTF I/O library in C11 providing
 * glTF ascii/binary reading and writing functionality.
 *
 * The latest source code maintained by Rampant Pixels is always available at
 *
 * https://github.com/rampantpixels/gltf_lib
 *
 * This library is put in the public domain; you can redistribute it and/or modify it without any restrictions.
 *
 */

#include "gltf.h"

int
gltf_module_initialize(gltf_config_t config) {
	FOUNDATION_UNUSED(config);
	return 0;
}

void
gltf_module_finalize(void) {
}

bool
gltf_module_is_initialized(void) {
	return true;
}

void
gltf_module_parse_config(const char* path, size_t path_size,
                         const char* buffer, size_t size,
                         const struct json_token_t* tokens, size_t num_tokens) {
	FOUNDATION_UNUSED(path);
	FOUNDATION_UNUSED(path_size);
	FOUNDATION_UNUSED(buffer);
	FOUNDATION_UNUSED(size);
	FOUNDATION_UNUSED(tokens);
	FOUNDATION_UNUSED(num_tokens);
}
