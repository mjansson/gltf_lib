/* image.c  -  glTF library  -  Public Domain  -  2018 Mattias Jansson
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
#include "image.h"
#include "hashstrings.h"

#include <foundation/memory.h>
#include <foundation/json.h>
#include <foundation/log.h>
#include <foundation/hashstrings.h>

void
gltf_images_finalize(gltf_t* gltf) {
	if (gltf->images) {
		memory_deallocate(gltf->images);
	}
}

static void
gltf_image_initialize(gltf_image_t* image) {
	image->buffer_view = GLTF_INVALID_INDEX;
}

static bool
gltf_images_parse_image(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken, gltf_image_t* image) {
	if (tokens[itoken].type != JSON_OBJECT)
		return false;

	gltf_image_initialize(image);

	itoken = tokens[itoken].child;
	while (itoken) {
		string_const_t identifier = json_token_identifier(gltf->buffer, tokens + itoken);
		hash_t identifier_hash = string_hash(STRING_ARGS(identifier));
		if ((identifier_hash == HASH_NAME) && (tokens[itoken].type == JSON_STRING))
			image->name = json_token_value(buffer, tokens + itoken);
		else if ((identifier_hash == HASH_EXTENSIONS) && (tokens[itoken].type == JSON_STRING))
			image->extensions = json_token_value(buffer, tokens + itoken);
		else if ((identifier_hash == HASH_EXTRAS) && (tokens[itoken].type == JSON_STRING))
			image->extras = json_token_value(buffer, tokens + itoken);
		else if ((identifier_hash == HASH_BUFFERVIEW) &&
		         !gltf_token_to_integer(gltf, buffer, tokens, itoken, &image->buffer_view))
			return false;
		else if ((identifier_hash == HASH_MIMETYPE) && (tokens[itoken].type == JSON_STRING))
			image->mime_type = json_token_value(buffer, tokens + itoken);
		else if ((identifier_hash == HASH_URI) && (tokens[itoken].type == JSON_STRING))
			image->uri = json_token_value(buffer, tokens + itoken);

		itoken = tokens[itoken].sibling;
	}

	return true;
}

bool
gltf_images_parse(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken) {
	if (tokens[itoken].type != JSON_ARRAY) {
		log_error(HASH_GLTF, ERROR_INVALID_VALUE, STRING_CONST("Main images attribute has invalid type"));
		return false;
	}

	size_t images_count = tokens[itoken].value_length;
	if (images_count > GLTF_MAX_INDEX)
		return false;
	if (!images_count)
		return true;

	size_t storage_size = sizeof(gltf_image_t) * images_count;
	gltf_images_finalize(gltf);
	gltf->images_count = (unsigned int)images_count;
	gltf->images = memory_allocate(HASH_GLTF, storage_size, 0, MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);

	unsigned int icounter = 0;
	size_t iimg = tokens[itoken].child;
	while (iimg) {
		if (!gltf_images_parse_image(gltf, buffer, tokens, iimg, gltf->images + icounter))
			return false;
		iimg = tokens[iimg].sibling;
		++icounter;
	}

	return true;
}
