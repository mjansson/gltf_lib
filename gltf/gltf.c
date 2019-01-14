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
#include "hashstrings.h"

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

static int
glb_read(gltf_t* gltf, stream_t* stream) {
	FOUNDATION_UNUSED(gltf);
	FOUNDATION_UNUSED(stream);
	return -1;
}

int
gltf_read(gltf_t* gltf, stream_t* stream) {
	stream_set_byteorder(stream, BYTEORDER_LITTLEENDIAN);

	ssize_t stream_offset = stream_tell(stream);

	gltf_glb_header_t glb_header;
	if (stream_read(&glb_header, sizeof(glb_header)) != sizeof(glb_header))
		return -1;

	if (glb_header.magic == 0x46546C67)
		return glb_read(gltf, stream);

	stream_seek(stream, 0, STREAM_SEEK_END);
	size_t capacity = stream_tell(stream) - stream_offset;
	stream_seek(stream, stream_offset, STREAM_SEEK_BEGIN);

	char* buffer = memory_allocate(HASH_GLTF, capacity, 0, MEMORY_TEMPORARY);
	int result = -1;

	size_t token = 0;
	size_t num_tokens = 0;
	size_t token_capacity = capacity / 16;
	json_token_t* tokens = memory_allocate(HASH_GLTF, sizeof(json_token_t) * token_capacity, 0,
	                                       MEMORY_TEMPORARY);

	if (stream_read(stream, buffer, capacity) != capacity)
		goto exit;

	num_tokens = json_parse(buffer, capacity, tokens, token_capacity);
	if (num_tokens > token_capacity) {
		tokens = memory_reallocate(tokens, sizeof(json_token_t) * num_tokens, 0,
		                           sizeof(json_token_t) * token_capacity, MEMORY_TEMPORARY);
		token_capacity = num_tokens;
		num_tokens = json_parse(buffer, capacity, tokens, token_capacity);
		if (num_tokens > token_capacity)
			goto exit;
	}

	if (tokens[0].type != JSON_OBJECT)
		goto exit;

	token = tokens[0].child;
	while (token) {
		string_const_t identifier = json_token_identifier(buffer, tokens + token);
		hash_t identifier_hash = string_hash(STRING_ARGS(identifier));
		if (identifier_hash == HASH_ASSET) {
			
			result = 0;
		}

		token = tokens[token].sibling;
	}

exit:
	memory_free(tokens);
	memory_free(buffer);
	return result;
}

int
gltf_write(const gltf_t* gltf, stream_t* stream, gltf_write_mode write_mode) {
	stream_set_byteorder(stream, BYTEORDER_LITTLEENDIAN);

}
