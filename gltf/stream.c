/* stream.c  -  glTF library  -  Public Domain  -  2018 Mattias Jansson / Rampant Pixels
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

#include <gltf/stream.h>

#include <foundation/string.h>
#include <foundation/stream.h>
#include <foundation/path.h>

stream_t*
gltf_stream_open(gltf_t* gltf, const char* uri, size_t length, unsigned int mode) {
	stream_t* stream = stream_open(uri, length, mode);
	if (stream)
		return stream;

	string_t full_path = path_allocate_concat(STRING_ARGS(gltf->base_path), uri, length);
	stream = stream_open(STRING_ARGS(full_path), mode);
	string_deallocate(full_path.str);

	return stream;
}
