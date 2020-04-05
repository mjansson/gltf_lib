/* extension.c  -  glTF library  -  Public Domain  -  2019 Mattias Jansson
 *
 * This library provides a cross-platform glTF I/O library in C11 providing
 * glTF ascii/binary reading and writing functionality.
 *
 * The latest source code maintained by Mattias Jansson is always available at
 *
 * https://github.com/mjansson/gltf_lib
 *
 * This library is put in the public domain; you can redistribute it and/or modify it without any
 * restrictions.
 *
 */

#include "gltf.h"
#include "accessor.h"
#include "hashstrings.h"

#include <foundation/memory.h>
#include <foundation/json.h>
#include <foundation/log.h>
#include <foundation/hashstrings.h>

static bool
gltf_extensions_array_parse(const char* data, json_token_t* tokens, size_t itoken, string_const_t** array,
                            unsigned int* size) {
	if (tokens[itoken].type != JSON_ARRAY) {
		log_error(HASH_GLTF, ERROR_INVALID_VALUE, STRING_CONST("Extensions used/required attribute has invalid type"));
		return false;
	}

	size_t num_extensions = tokens[itoken].value_length;
	if (num_extensions > GLTF_MAX_INDEX)
		return false;
	if (!num_extensions)
		return true;

	size_t storage_size = sizeof(string_const_t) * num_extensions;
	memory_deallocate(*array);
	*size = (unsigned int)num_extensions;
	*array = memory_allocate(HASH_GLTF, storage_size, 0, MEMORY_PERSISTENT);

	unsigned int icounter = 0;
	size_t iext = tokens[itoken].child;
	while (iext) {
		(*array)[icounter] = json_token_value(data, tokens + iext);
		iext = tokens[iext].sibling;
		++icounter;
	}

	return true;
}

bool
gltf_extensions_used_parse(gltf_t* gltf, const char* data, json_token_t* tokens, size_t itoken) {
	return gltf_extensions_array_parse(data, tokens, itoken, &gltf->extensions_used, &gltf->extensions_used_count);
}

bool
gltf_extensions_required_parse(gltf_t* gltf, const char* data, json_token_t* tokens, size_t itoken) {
	return gltf_extensions_array_parse(data, tokens, itoken, &gltf->extensions_required, &gltf->extensions_required_count);
}
