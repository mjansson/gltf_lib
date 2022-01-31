/* accessor.c  -  glTF library  -  Public Domain  -  2019 Mattias Jansson
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

void
gltf_accessors_finalize(gltf_t* gltf) {
	if (gltf->accessors)
		memory_deallocate(gltf->accessors);
}

static void
gltf_accessor_initialize(gltf_accessor_t* accessor) {
	memset(accessor, 0, sizeof(gltf_accessor_t));
}

static bool
gltf_accessor_parse_sparse_indices(gltf_t* gltf, const char* data, json_token_t* tokens, size_t itoken,
                                   gltf_sparse_indices_t* indices) {
	if (tokens[itoken].type != JSON_OBJECT) {
		log_error(HASH_GLTF, ERROR_INVALID_VALUE, STRING_CONST("Accessor sparse indices attribute has invalid type"));
		return false;
	}

	itoken = tokens[itoken].child;
	while (itoken) {
		string_const_t identifier = json_token_identifier(gltf->buffer, tokens + itoken);
		hash_t identifier_hash = string_hash(STRING_ARGS(identifier));
		if ((identifier_hash == HASH_BUFFERVIEW) &&
		    !gltf_token_to_integer(gltf, data, tokens, itoken, &indices->buffer_view))
			return false;
		else if ((identifier_hash == HASH_BYTEOFFSET) &&
		         !gltf_token_to_integer(gltf, data, tokens, itoken, &indices->byte_offset))
			return false;
		else if ((identifier_hash == HASH_COMPONENTTYPE) &&
		         !gltf_token_to_component_type(gltf, data, tokens, itoken, &indices->component_type))
			return false;
		else if ((identifier_hash == HASH_EXTENSIONS) && (tokens[itoken].type == JSON_STRING))
			indices->extensions = json_token_value(data, tokens + itoken);
		else if ((identifier_hash == HASH_EXTRAS) && (tokens[itoken].type == JSON_STRING))
			indices->extras = json_token_value(data, tokens + itoken);

		itoken = tokens[itoken].sibling;
	}

	return true;
}

static bool
gltf_accessor_parse_sparse_values(gltf_t* gltf, const char* data, json_token_t* tokens, size_t itoken,
                                  gltf_sparse_values_t* values) {
	if (tokens[itoken].type != JSON_OBJECT) {
		log_error(HASH_GLTF, ERROR_INVALID_VALUE, STRING_CONST("Accessor sparse vaöies attribute has invalid type"));
		return false;
	}

	itoken = tokens[itoken].child;
	while (itoken) {
		string_const_t identifier = json_token_identifier(gltf->buffer, tokens + itoken);
		hash_t identifier_hash = string_hash(STRING_ARGS(identifier));
		if ((identifier_hash == HASH_BUFFERVIEW) &&
		    !gltf_token_to_integer(gltf, data, tokens, itoken, &values->buffer_view))
			return false;
		else if ((identifier_hash == HASH_BYTEOFFSET) &&
		         !gltf_token_to_integer(gltf, data, tokens, itoken, &values->byte_offset))
			return false;
		else if ((identifier_hash == HASH_EXTENSIONS) && (tokens[itoken].type == JSON_STRING))
			values->extensions = json_token_value(data, tokens + itoken);
		else if ((identifier_hash == HASH_EXTRAS) && (tokens[itoken].type == JSON_STRING))
			values->extras = json_token_value(data, tokens + itoken);

		itoken = tokens[itoken].sibling;
	}

	return true;
}

static bool
gltf_accessor_parse_sparse(gltf_t* gltf, const char* data, json_token_t* tokens, size_t itoken,
                           gltf_accessor_sparse_t* sparse) {
	if (tokens[itoken].type != JSON_OBJECT) {
		log_error(HASH_GLTF, ERROR_INVALID_VALUE, STRING_CONST("Accessor sparse attribute has invalid type"));
		return false;
	}

	itoken = tokens[itoken].child;
	while (itoken) {
		string_const_t identifier = json_token_identifier(gltf->buffer, tokens + itoken);
		hash_t identifier_hash = string_hash(STRING_ARGS(identifier));
		if ((identifier_hash == HASH_COUNT) && !gltf_token_to_integer(gltf, data, tokens, itoken, &sparse->count))
			return false;
		else if ((identifier_hash == HASH_INDICES) &&
		         !gltf_accessor_parse_sparse_indices(gltf, data, tokens, itoken, &sparse->indices))
			return false;
		else if ((identifier_hash == HASH_VALUES) &&
		         !gltf_accessor_parse_sparse_values(gltf, data, tokens, itoken, &sparse->values))
			return false;
		else if ((identifier_hash == HASH_EXTENSIONS) && (tokens[itoken].type == JSON_STRING))
			sparse->extensions = json_token_value(data, tokens + itoken);
		else if ((identifier_hash == HASH_EXTRAS) && (tokens[itoken].type == JSON_STRING))
			sparse->extras = json_token_value(data, tokens + itoken);

		itoken = tokens[itoken].sibling;
	}

	return true;
}

static bool
gltf_accessors_parse_accessor(gltf_t* gltf, const char* data, json_token_t* tokens, size_t itoken,
                              gltf_accessor_t* accessor) {
	if (tokens[itoken].type != JSON_OBJECT) {
		log_error(HASH_GLTF, ERROR_INVALID_VALUE, STRING_CONST("Accessor attribute has invalid type"));
		return false;
	}

	gltf_accessor_initialize(accessor);

	itoken = tokens[itoken].child;
	while (itoken) {
		string_const_t identifier = json_token_identifier(gltf->buffer, tokens + itoken);
		hash_t identifier_hash = string_hash(STRING_ARGS(identifier));
		if ((identifier_hash == HASH_NAME) && (tokens[itoken].type == JSON_STRING))
			accessor->name = json_token_value(data, tokens + itoken);
		else if ((identifier_hash == HASH_BUFFERVIEW) &&
		         !gltf_token_to_integer(gltf, data, tokens, itoken, &accessor->buffer_view))
			return false;
		else if ((identifier_hash == HASH_BYTEOFFSET) &&
		         !gltf_token_to_integer(gltf, data, tokens, itoken, &accessor->byte_offset))
			return false;
		else if ((identifier_hash == HASH_COMPONENTTYPE) &&
		         !gltf_token_to_component_type(gltf, data, tokens, itoken, &accessor->component_type))
			return false;
		else if ((identifier_hash == HASH_NORMALIZED) &&
		         !gltf_token_to_boolean(gltf, data, tokens, itoken, &accessor->normalized))
			return false;
		else if ((identifier_hash == HASH_COUNT) &&
		         !gltf_token_to_integer(gltf, data, tokens, itoken, &accessor->count))
			return false;
		else if ((identifier_hash == HASH_TYPE) &&
		         !gltf_token_to_data_type(gltf, data, tokens, itoken, &accessor->type))
			return false;
		else if ((identifier_hash == HASH_MIN) &&
		         !gltf_token_to_double_array(gltf, data, tokens, itoken, accessor->min, 4))
			return false;
		else if ((identifier_hash == HASH_MAX) &&
		         !gltf_token_to_double_array(gltf, data, tokens, itoken, accessor->max, 4))
			return false;
		else if ((identifier_hash == HASH_SPARSE) &&
		         !gltf_accessor_parse_sparse(gltf, data, tokens, itoken, &accessor->sparse))
			return false;
		else if ((identifier_hash == HASH_EXTENSIONS) && (tokens[itoken].type == JSON_STRING))
			accessor->extensions = json_token_value(data, tokens + itoken);
		else if ((identifier_hash == HASH_EXTRAS) && (tokens[itoken].type == JSON_STRING))
			accessor->extras = json_token_value(data, tokens + itoken);

		itoken = tokens[itoken].sibling;
	}

	return true;
}

bool
gltf_accessors_parse(gltf_t* gltf, const char* data, json_token_t* tokens, size_t itoken) {
	if (tokens[itoken].type != JSON_ARRAY) {
		log_error(HASH_GLTF, ERROR_INVALID_VALUE, STRING_CONST("Main accessors attribute has invalid type"));
		return false;
	}

	size_t accessors_count = tokens[itoken].value_length;
	if (accessors_count > GLTF_MAX_INDEX)
		return false;
	if (!accessors_count)
		return true;

	size_t storage_size = sizeof(gltf_accessor_t) * accessors_count;
	gltf_buffers_finalize(gltf);
	gltf->accessors_count = (uint)accessors_count;
	gltf->accessors = memory_allocate(HASH_GLTF, storage_size, 0, MEMORY_PERSISTENT);

	uint icounter = 0;
	size_t iscene = tokens[itoken].child;
	while (iscene) {
		if (!gltf_accessors_parse_accessor(gltf, data, tokens, iscene, gltf->accessors + icounter))
			return false;
		iscene = tokens[iscene].sibling;
		++icounter;
	}

	return true;
}
