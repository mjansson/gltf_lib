/* buffer.c  -  glTF library  -  Public Domain  -  2018 Mattias Jansson
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
#include "buffer.h"
#include "hashstrings.h"

#include <foundation/memory.h>
#include <foundation/json.h>
#include <foundation/array.h>
#include <foundation/log.h>
#include <foundation/hashstrings.h>

void
gltf_buffers_finalize(gltf_t* gltf) {
	if (gltf->buffers)
		array_deallocate(gltf->buffers);
}

static void
gltf_buffer_initialize(gltf_buffer_t* buffer) {
	FOUNDATION_UNUSED(buffer);
}

static bool
gltf_buffers_parse_buffer(gltf_t* gltf, const char* data, json_token_t* tokens, size_t itoken, gltf_buffer_t* buffer) {
	if (tokens[itoken].type != JSON_OBJECT)
		return false;

	gltf_buffer_initialize(buffer);

	itoken = tokens[itoken].child;
	while (itoken) {
		string_const_t identifier = json_token_identifier(gltf->buffer, tokens + itoken);
		hash_t identifier_hash = string_hash(STRING_ARGS(identifier));
		if ((identifier_hash == HASH_NAME) && (tokens[itoken].type == JSON_STRING))
			buffer->name = json_token_value(data, tokens + itoken);
		else if ((identifier_hash == HASH_URI) && (tokens[itoken].type == JSON_STRING))
			buffer->uri = json_token_value(data, tokens + itoken);
		else if ((identifier_hash == HASH_BYTELENGTH) &&
		         !gltf_token_to_integer(gltf, data, tokens, itoken, &buffer->byte_length))
			return false;
		else if ((identifier_hash == HASH_EXTENSIONS) && (tokens[itoken].type == JSON_STRING))
			buffer->extensions = json_token_value(data, tokens + itoken);
		else if ((identifier_hash == HASH_EXTRAS) && (tokens[itoken].type == JSON_STRING))
			buffer->extras = json_token_value(data, tokens + itoken);

		itoken = tokens[itoken].sibling;
	}

	return true;
}

bool
gltf_buffers_parse(gltf_t* gltf, const char* data, json_token_t* tokens, size_t itoken) {
	if (tokens[itoken].type != JSON_ARRAY) {
		log_error(HASH_GLTF, ERROR_INVALID_VALUE, STRING_CONST("Main buffers attribute has invalid type"));
		return false;
	}

	size_t buffers_count = tokens[itoken].value_length;
	if (buffers_count > GLTF_MAX_INDEX)
		return false;

	array_resize(gltf->buffers, buffers_count);

	if (!buffers_count)
		return true;

	uint icounter = 0;
	size_t ibuffer = tokens[itoken].child;
	while (ibuffer) {
		if (!gltf_buffers_parse_buffer(gltf, data, tokens, ibuffer, gltf->buffers + icounter))
			return false;
		ibuffer = tokens[ibuffer].sibling;
		++icounter;
	}

	return true;
}

void
gltf_buffer_views_finalize(gltf_t* gltf) {
	if (gltf->buffer_views)
		array_deallocate(gltf->buffer_views);
}

static void
gltf_buffer_view_initialize(gltf_buffer_view_t* buffer_view) {
	buffer_view->buffer = GLTF_INVALID_INDEX;
}

static bool
gltf_buffer_view_parse_view(gltf_t* gltf, const char* data, json_token_t* tokens, size_t itoken,
                            gltf_buffer_view_t* buffer_view) {
	if (tokens[itoken].type != JSON_OBJECT)
		return false;

	gltf_buffer_view_initialize(buffer_view);

	itoken = tokens[itoken].child;
	while (itoken) {
		string_const_t identifier = json_token_identifier(gltf->buffer, tokens + itoken);
		hash_t identifier_hash = string_hash(STRING_ARGS(identifier));
		if ((identifier_hash == HASH_NAME) && (tokens[itoken].type == JSON_STRING))
			buffer_view->name = json_token_value(data, tokens + itoken);
		else if ((identifier_hash == HASH_BUFFER) &&
		         !gltf_token_to_integer(gltf, data, tokens, itoken, &buffer_view->buffer))
			return false;
		else if ((identifier_hash == HASH_BYTEOFFSET) &&
		         !gltf_token_to_integer(gltf, data, tokens, itoken, &buffer_view->byte_offset))
			return false;
		else if ((identifier_hash == HASH_BYTELENGTH) &&
		         !gltf_token_to_integer(gltf, data, tokens, itoken, &buffer_view->byte_length))
			return false;
		else if ((identifier_hash == HASH_BYTESTRIDE) &&
		         !gltf_token_to_integer(gltf, data, tokens, itoken, &buffer_view->byte_stride))
			return false;
		else if ((identifier_hash == HASH_TARGET) &&
		         !gltf_token_to_integer(gltf, data, tokens, itoken, &buffer_view->target))
			return false;
		else if ((identifier_hash == HASH_EXTENSIONS) && (tokens[itoken].type == JSON_STRING))
			buffer_view->extensions = json_token_value(data, tokens + itoken);
		else if ((identifier_hash == HASH_EXTRAS) && (tokens[itoken].type == JSON_STRING))
			buffer_view->extras = json_token_value(data, tokens + itoken);

		itoken = tokens[itoken].sibling;
	}

	return true;
}

bool
gltf_buffer_views_parse(gltf_t* gltf, const char* data, json_token_t* tokens, size_t itoken) {
	if (tokens[itoken].type != JSON_ARRAY) {
		log_error(HASH_GLTF, ERROR_INVALID_VALUE, STRING_CONST("Main buffer views attribute has invalid type"));
		return false;
	}

	size_t views_count = tokens[itoken].value_length;
	if (views_count > GLTF_MAX_INDEX)
		return false;

	array_resize(gltf->buffer_views, views_count);

	if (!views_count)
		return true;

	uint icounter = 0;
	size_t iview = tokens[itoken].child;
	while (iview) {
		if (!gltf_buffer_view_parse_view(gltf, data, tokens, iview, gltf->buffer_views + icounter))
			return false;
		iview = tokens[iview].sibling;
		++icounter;
	}

	return true;
}
