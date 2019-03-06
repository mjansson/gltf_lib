/* accessor.c  -  glTF library  -  Public Domain  -  2019 Mattias Jansson / Rampant Pixels
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
#include "accessor.h"
#include "hashstrings.h"

#include <foundation/memory.h>
#include <foundation/json.h>
#include <foundation/log.h>
#include <foundation/hashstrings.h>

void
gltf_accessors_finalize(gltf_t* gltf) {
	if (gltf->accessors)
		memory_deallocate(gltf->accessors);
}

static void
gltf_accessor_initialize(gltf_accessor_t* accessor) {
	memset(accessor, 0, sizeof(gltf_accessor_t));
}

static int
gltf_accessor_parse_sparse(gltf_t* gltf, const char* data, json_token_t* tokens, size_t itoken,
                           gltf_accessor_t* accessor) {
	FOUNDATION_UNUSED(gltf);
	FOUNDATION_UNUSED(data);
	FOUNDATION_UNUSED(tokens);
	FOUNDATION_UNUSED(itoken);
	FOUNDATION_UNUSED(accessor);
	return 0;
}

static int
gltf_accessors_parse_accessor(gltf_t* gltf, const char* data, json_token_t* tokens, size_t itoken,
                              gltf_accessor_t* accessor) {
	if (tokens[itoken].type != JSON_OBJECT)
		return -1;

	gltf_accessor_initialize(accessor);

	int result = 0;
	itoken = tokens[itoken].child;
	while (itoken && !result) {
		string_const_t identifier = json_token_identifier(gltf->buffer, tokens + itoken);
		hash_t identifier_hash = string_hash(STRING_ARGS(identifier));
		if ((identifier_hash == HASH_NAME) && (tokens[itoken].type == JSON_STRING))
			accessor->name = json_token_value(data, tokens + itoken);
		else if (identifier_hash == HASH_BUFFERVIEW)
			result = gltf_token_to_integer(gltf, data, tokens, itoken, &accessor->buffer_view);
		else if (identifier_hash == HASH_BYTEOFFSET)
			result = gltf_token_to_integer(gltf, data, tokens, itoken, &accessor->byte_offset);
		else if (identifier_hash == HASH_COMPONENTTYPE)
			result = gltf_token_to_component_type(gltf, data, tokens, itoken, &accessor->component_type);
		else if (identifier_hash == HASH_NORMALIZED)
			result = gltf_token_to_boolean(gltf, data, tokens, itoken, &accessor->normalized);
		else if (identifier_hash == HASH_COUNT)
			result = gltf_token_to_integer(gltf, data, tokens, itoken, &accessor->count);
		else if (identifier_hash == HASH_TYPE)
			result = gltf_token_to_data_type(gltf, data, tokens, itoken, &accessor->type);
		else if (identifier_hash == HASH_MIN)
			result = gltf_token_to_double_array(gltf, data, tokens, itoken, accessor->max, 3);
		else if (identifier_hash == HASH_MAX)
			result = gltf_token_to_double_array(gltf, data, tokens, itoken, accessor->max, 3);
		else if (identifier_hash == HASH_SPARSE)
			result = gltf_accessor_parse_sparse(gltf, data, tokens, itoken, accessor);
		else if ((identifier_hash == HASH_EXTENSIONS) && (tokens[itoken].type == JSON_STRING))
			accessor->extensions = json_token_value(data, tokens + itoken);
		else if ((identifier_hash == HASH_EXTRAS) && (tokens[itoken].type == JSON_STRING))
			accessor->extras = json_token_value(data, tokens + itoken);

		if (result)
			break;
		itoken = tokens[itoken].sibling;
	}

	return result;
}

int
gltf_accessors_parse(gltf_t* gltf, const char* data, json_token_t* tokens, size_t itoken) {
	if (tokens[itoken].type != JSON_ARRAY) {
		log_error(HASH_GLTF, ERROR_INVALID_VALUE,
		          STRING_CONST("Main accessors attribute has invalid type"));
		return -1;
	}

	size_t num_accessors = tokens[itoken].value_length;
	if (num_accessors > GLTF_MAX_INDEX)
		return -1;
	if (!num_accessors)
		return 0;

	size_t storage_size = sizeof(gltf_accessor_t) * num_accessors;
	gltf_buffers_finalize(gltf);
	gltf->num_accessors = (unsigned int)num_accessors;
	gltf->accessors = memory_allocate(HASH_GLTF, storage_size, 0, MEMORY_PERSISTENT);

	int result = 0;
	unsigned int icounter = 0;
	size_t iscene = tokens[itoken].child;
	while (iscene) {
		result = gltf_accessors_parse_accessor(gltf, data, tokens, iscene, gltf->accessors + icounter);
		if (result)
			break;
		iscene = tokens[iscene].sibling;
		++icounter;
	}

	return result;
}
