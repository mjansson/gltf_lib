/* image.c  -  glTF library  -  Public Domain  -  2018 Mattias Jansson / Rampant Pixels
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

static int
gltf_images_parse_image(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken,
                        gltf_image_t* image) {
	if (tokens[itoken].type != JSON_OBJECT)
		return -1;

	gltf_image_initialize(image);

	int result = 0;
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
		else if (identifier_hash == HASH_BUFFERVIEW)
			result = gltf_token_to_integer(gltf, buffer, tokens, itoken, &image->buffer_view);
		else if ((identifier_hash == HASH_MIMETYPE) && (tokens[itoken].type == JSON_STRING))
			image->mime_type = json_token_value(buffer, tokens + itoken);
		else if ((identifier_hash == HASH_URI) && (tokens[itoken].type == JSON_STRING))
			image->uri = json_token_value(buffer, tokens + itoken);

		if (result)
			break;
		itoken = tokens[itoken].sibling;
	}

	return result;
}

int
gltf_images_parse(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken) {
	if (tokens[itoken].type != JSON_ARRAY) {
		log_error(HASH_GLTF, ERROR_INVALID_VALUE,
		          STRING_CONST("Main images attribute has invalid type"));
		return -1;
	}

	size_t num_images = tokens[itoken].value_length;
	if (num_images > GLTF_MAX_INDEX)
		return -1;
	if (!num_images)
		return 0;

	size_t storage_size = sizeof(gltf_image_t) * num_images;
	gltf_images_finalize(gltf);
	gltf->num_images = (unsigned int)num_images;
	gltf->images =
	    memory_allocate(HASH_GLTF, storage_size, 0, MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);

	int result = 0;
	unsigned int icounter = 0;
	size_t iimg = tokens[itoken].child;
	while (iimg) {
		result = gltf_images_parse_image(gltf, buffer, tokens, iimg, gltf->images + icounter);
		if (result)
			break;
		iimg = tokens[iimg].sibling;
		++icounter;
	}

	return result;
}