/* texture.c  -  glTF library  -  Public Domain  -  2018 Mattias Jansson / Rampant Pixels
 *
 * This library provides a cross-platform glTF I/O library in C11 providing
 * glTF ascii/binary reading and writing functionality.
 *
 * The latest source code maintained by Rampant Pixels is always available at
 *
 * https://github.com/rampantpixels/gltf_lib
 *
 * This library is put in the public domain; you can redistribute it and/or modify it without any
 * restrictions.
 *
 */

#include "gltf.h"
#include "texture.h"
#include "hashstrings.h"

#include <foundation/memory.h>
#include <foundation/json.h>
#include <foundation/log.h>
#include <foundation/hashstrings.h>

void
gltf_textures_finalize(gltf_t* gltf) {
	if (gltf->textures) {
		memory_deallocate(gltf->textures);
	}
}

static void
gltf_texture_initialize(gltf_texture_t* texture) {
	texture->sampler = GLTF_INVALID_INDEX;
	texture->source = GLTF_INVALID_INDEX;
}

static int
gltf_textures_parse_texture(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken,
                            gltf_texture_t* texture) {
	if (tokens[itoken].type != JSON_OBJECT)
		return -1;

	gltf_texture_initialize(texture);

	int result = 0;
	itoken = tokens[itoken].child;
	while (itoken) {
		string_const_t identifier = json_token_identifier(gltf->buffer, tokens + itoken);
		hash_t identifier_hash = string_hash(STRING_ARGS(identifier));
		if ((identifier_hash == HASH_NAME) && (tokens[itoken].type == JSON_STRING))
			texture->name = json_token_value(buffer, tokens + itoken);
		else if ((identifier_hash == HASH_EXTENSIONS) && (tokens[itoken].type == JSON_STRING))
			texture->extensions = json_token_value(buffer, tokens + itoken);
		else if ((identifier_hash == HASH_EXTRAS) && (tokens[itoken].type == JSON_STRING))
			texture->extras = json_token_value(buffer, tokens + itoken);
		else if (identifier_hash == HASH_SAMPLER)
			result = gltf_token_to_integer(gltf, buffer, tokens, itoken, &texture->sampler);
		else if (identifier_hash == HASH_SOURCE)
			result = gltf_token_to_integer(gltf, buffer, tokens, itoken, &texture->source);

		if (result)
			break;
		itoken = tokens[itoken].sibling;
	}

	return result;
}

int
gltf_textures_parse(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken) {
	if (tokens[itoken].type != JSON_ARRAY) {
		log_error(HASH_GLTF, ERROR_INVALID_VALUE,
		          STRING_CONST("Main textures attribute has invalid type"));
		return -1;
	}

	size_t num_textures = tokens[itoken].value_length;
	if (num_textures > GLTF_MAX_INDEX)
		return -1;
	if (!num_textures)
		return 0;

	size_t storage_size = sizeof(gltf_texture_t) * num_textures;
	gltf_textures_finalize(gltf);
	gltf->num_textures = (unsigned int)num_textures;
	gltf->textures =
	    memory_allocate(HASH_GLTF, storage_size, 0, MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);

	int result = 0;
	unsigned int icounter = 0;
	size_t itex = tokens[itoken].child;
	while (itex) {
		result = gltf_textures_parse_texture(gltf, buffer, tokens, itex, gltf->textures + icounter);
		if (result)
			break;
		itex = tokens[itex].sibling;
		++icounter;
	}

	return result;
}
