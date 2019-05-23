/* stream.c  -  glTF library  -  Public Domain  -  2018 Mattias Jansson / Rampant Pixels
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

#include <gltf/stream.h>
#include <gltf/hashstrings.h>

#include <foundation/string.h>
#include <foundation/stream.h>
#include <foundation/bufferstream.h>
#include <foundation/path.h>
#include <foundation/log.h>
#include <foundation/base64.h>
#include <foundation/memory.h>
#include <foundation/system.h>
#include <foundation/time.h>

static stream_vtable_t gltf_stream_base64_vtable;
static stream_vtable_t gltf_stream_substream_vtable;

extern int
gltf_module_stream_initialize(void);

extern void
gltf_module_stream_finalize(void);

typedef struct gltf_stream_base64_t gltf_stream_base64_t;
typedef struct gltf_stream_substream_t gltf_stream_substream_t;

struct gltf_stream_base64_t {
	FOUNDATION_DECLARE_STREAM;
	/*! Current read offset */
	size_t current;
	/*! Current offset within buffer */
	size_t offset;
	/*! Current size of buffer (always less or equal than capacity) */
	size_t buffer_size;
	/*! Current allocated capacity of buffer */
	size_t buffer_capacity;
	/*! Memory buffer */
	void* buffer;
	/*! Source data (base64 encoded) */
	const char* source;
	/*! Source data length */
	size_t source_length;
	/*! Size of unpacked source data */
	size_t total_size;
	/*! Timestamp of last modification */
	tick_t lastmod;
};

struct gltf_stream_substream_t {
	FOUNDATION_DECLARE_STREAM;
	/*! Actual stream */
	stream_t* stream;
	/*! Offset within actual stream */
	size_t offset;
	/*! Size of substream */
	size_t size;
	/*! Current position within substream window */
	size_t current;
};

static void
gltf_stream_base64_finalize(stream_t* stream) {
	gltf_stream_base64_t* gltf_stream = (gltf_stream_base64_t*)stream;
	if (!gltf_stream || (stream->type != STREAMTYPE_MEMORY))
		return;

	memory_deallocate(gltf_stream->buffer);
	gltf_stream->buffer = nullptr;
}

static size_t
gltf_stream_base64_read(stream_t* stream, void* dest, size_t num) {
	size_t available, num_read;
	gltf_stream_base64_t* gltf_stream = (gltf_stream_base64_t*)stream;

	available = gltf_stream->total_size - gltf_stream->current;
	num_read = (num < available) ? num : available;

	size_t was_read = 0;
	if (num_read > 0) {
		size_t to_read = num_read;
		FOUNDATION_ASSERT(gltf_stream->buffer_size >= gltf_stream->offset);
		available = gltf_stream->buffer_size - gltf_stream->offset;
		while (to_read > available) {
			if (available) {
				if (available > to_read)
					available = to_read;

				memcpy(pointer_offset(dest, was_read),
				       pointer_offset(gltf_stream->buffer, gltf_stream->offset), available);
				gltf_stream->offset += available;
				gltf_stream->current += available;

				was_read += available;
				to_read -= available;
				available = 0;
			} else {
				gltf_stream->offset = 0;
				if (!gltf_stream->buffer) {
					gltf_stream->buffer_capacity = 3 * 10 * 1024;
					gltf_stream->buffer = memory_allocate(HASH_GLTF, gltf_stream->buffer_capacity,
					                                      32, MEMORY_PERSISTENT);
				}

				// Locate the correct byte triplet
				size_t byte_triplet = (gltf_stream->current / 3);
				size_t source_offset = byte_triplet * 4;
				size_t new_current = byte_triplet * 3;

				FOUNDATION_ASSERT(gltf_stream->current >= new_current);
				gltf_stream->offset =
				    (gltf_stream->current > new_current) ? (gltf_stream->current - new_current) : 0;
				gltf_stream->current = new_current;
				FOUNDATION_ASSERT(gltf_stream->offset <= 2);

				gltf_stream->buffer_size = base64_decode(
				    gltf_stream->source + source_offset, gltf_stream->source_length - source_offset,
				    gltf_stream->buffer, gltf_stream->buffer_capacity);

				available = gltf_stream->buffer_size - gltf_stream->offset;
			}
		}

		memcpy(pointer_offset(dest, was_read),
		       pointer_offset(gltf_stream->buffer, gltf_stream->offset), to_read);
		gltf_stream->offset += to_read;
		gltf_stream->current += to_read;

		was_read += to_read;
	}

	FOUNDATION_ASSERT(was_read == num_read);
	return was_read;
}

static bool
gltf_stream_base64_eos(stream_t* stream) {
	gltf_stream_base64_t* gltf_stream = (gltf_stream_base64_t*)stream;
	return gltf_stream->current >= gltf_stream->total_size;
}

/*lint -e{818} Function prototype must match stream interface */
static size_t
gltf_stream_base64_size(stream_t* stream) {
	gltf_stream_base64_t* gltf_stream = (gltf_stream_base64_t*)stream;
	return gltf_stream->total_size;
}

static void
gltf_stream_base64_seek(stream_t* stream, ssize_t offset, stream_seek_mode_t direction) {
	gltf_stream_base64_t* gltf_stream = (gltf_stream_base64_t*)stream;
	size_t new_current = 0;
	/*lint --e{571} Used when offset < 0*/
	size_t abs_offset = (size_t)((offset < 0) ? -offset : offset);
	if (direction == STREAM_SEEK_CURRENT) {
		if (offset < 0)
			new_current =
			    (abs_offset > gltf_stream->current) ? 0 : (gltf_stream->current - abs_offset);
		else
			new_current = gltf_stream->current + abs_offset;
	} else if (direction == STREAM_SEEK_BEGIN)
		new_current = (offset > 0) ? abs_offset : 0;
	else if (direction == STREAM_SEEK_END)
		new_current = (offset < 0) ? gltf_stream->total_size - abs_offset : gltf_stream->total_size;

	size_t buffer_start = gltf_stream->current - gltf_stream->offset;
	if ((new_current < buffer_start) ||
	    (new_current >= (buffer_start + gltf_stream->buffer_size))) {
		gltf_stream->offset = 0;
		gltf_stream->buffer_size = 0;
	} else {
		gltf_stream->offset = new_current - buffer_start;
	}

	if (new_current >= gltf_stream->total_size)
		gltf_stream->current = gltf_stream->total_size;
	else
		gltf_stream->current = new_current;
}

static size_t
gltf_stream_base64_tell(stream_t* stream) {
	gltf_stream_base64_t* gltf_stream = (gltf_stream_base64_t*)stream;
	return gltf_stream->current;
}

static tick_t
gltf_stream_base64_lastmod(const stream_t* stream) {
	const gltf_stream_base64_t* gltf_stream = (const gltf_stream_base64_t*)stream;
	return gltf_stream->lastmod;
}

static size_t
gltf_stream_base64_available_read(stream_t* stream) {
	const gltf_stream_base64_t* gltf_stream = (gltf_stream_base64_t*)stream;
	return gltf_stream->total_size - gltf_stream->current;
}

static stream_t*
gltf_allocate_stream_base64(const char* data, size_t length, size_t unpacked_length) {
	gltf_stream_base64_t* stream = memory_allocate(HASH_GLTF, sizeof(gltf_stream_base64_t), 8,
	                                               MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);
	stream_initialize((stream_t*)stream, system_byteorder());

	stream->type = STREAMTYPE_MEMORY;
	stream->path =
	    string_allocate_format(STRING_CONST("gltf-base64://0x%" PRIfixPTR), (uintptr_t)stream);
	stream->mode = STREAM_IN | STREAM_BINARY;
	stream->lastmod = time_current();
	stream->source = data;
	stream->source_length = length;
	stream->total_size = unpacked_length;

	stream->vtable = &gltf_stream_base64_vtable;

	return (stream_t*)stream;
}

static void
gltf_stream_substream_finalize(stream_t* stream) {
	gltf_stream_substream_t* gltf_stream = (gltf_stream_substream_t*)stream;
	if (!gltf_stream)
		return;

	stream_deallocate(gltf_stream->stream);
	gltf_stream->stream = nullptr;
}

static size_t
gltf_stream_substream_read(stream_t* stream, void* dest, size_t num) {
	gltf_stream_substream_t* gltf_stream = (gltf_stream_substream_t*)stream;
	return stream_read(gltf_stream->stream, dest, num);
}

static bool
gltf_stream_substream_eos(stream_t* stream) {
	gltf_stream_substream_t* gltf_stream = (gltf_stream_substream_t*)stream;
	return gltf_stream->current >= gltf_stream->size;
}

/*lint -e{818} Function prototype must match stream interface */
static size_t
gltf_stream_substream_size(stream_t* stream) {
	gltf_stream_substream_t* gltf_stream = (gltf_stream_substream_t*)stream;
	return gltf_stream->size;
}

static void
gltf_stream_substream_seek(stream_t* stream, ssize_t offset, stream_seek_mode_t direction) {
	gltf_stream_substream_t* gltf_stream = (gltf_stream_substream_t*)stream;
	size_t new_current = 0;
	/*lint --e{571} Used when offset < 0*/
	size_t abs_offset = (size_t)((offset < 0) ? -offset : offset);
	if (direction == STREAM_SEEK_CURRENT) {
		if (offset < 0)
			new_current =
			    (abs_offset > gltf_stream->current) ? 0 : (gltf_stream->current - abs_offset);
		else
			new_current = gltf_stream->current + abs_offset;
	} else if (direction == STREAM_SEEK_BEGIN)
		new_current = (offset > 0) ? abs_offset : 0;
	else if (direction == STREAM_SEEK_END)
		new_current = (offset < 0) ? gltf_stream->size - abs_offset : gltf_stream->size;

	gltf_stream->current = (new_current < gltf_stream->size) ? new_current : gltf_stream->size;
	stream_seek(gltf_stream->stream, gltf_stream->offset + gltf_stream->current, STREAM_SEEK_BEGIN);
}

static size_t
gltf_stream_substream_tell(stream_t* stream) {
	gltf_stream_substream_t* gltf_stream = (gltf_stream_substream_t*)stream;
	return gltf_stream->current;
}

static tick_t
gltf_stream_substream_lastmod(const stream_t* stream) {
	const gltf_stream_substream_t* gltf_stream = (const gltf_stream_substream_t*)stream;
	return stream_last_modified(gltf_stream->stream);
}

static size_t
gltf_stream_substream_available_read(stream_t* stream) {
	const gltf_stream_substream_t* gltf_stream = (gltf_stream_substream_t*)stream;
	size_t subavail = stream_available_read(gltf_stream->stream);
	size_t maxavail = gltf_stream->size - gltf_stream->current;
	return (subavail < maxavail) ? subavail : maxavail;
}

static stream_t*
gltf_allocate_glb_substream(const char* path, size_t path_length, size_t offset, size_t size) {
	stream_t* substream = stream_open(path, path_length, STREAM_IN | STREAM_BINARY);
	if (!substream)
		return nullptr;
	stream_seek(substream, offset, STREAM_SEEK_BEGIN);

	gltf_stream_substream_t* stream = memory_allocate(HASH_GLTF, sizeof(gltf_stream_substream_t), 8,
	                                                  MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);
	stream_initialize((stream_t*)stream, system_byteorder());

	string_const_t subpath = stream_path(substream);
	stream->type = substream->type;
	stream->persistent = 0;
	stream->reliable = substream->reliable;
	stream->sequential = substream->sequential;
	stream->inorder = substream->inorder;
	stream->path =
	    string_allocate_format(STRING_CONST("gltf-substream://%" PRIsize ":%" PRIsize "@%.*s"),
	                           offset, size, STRING_FORMAT(subpath));
	stream->mode = STREAM_IN | STREAM_BINARY;
	stream->offset = offset;
	stream->size = size;
	stream->current = 0;
	stream->stream = substream;
	stream->vtable = &gltf_stream_substream_vtable;

	return (stream_t*)stream;
}

stream_t*
gltf_stream_open(gltf_t* gltf, const char* uri, size_t length, unsigned int mode) {
	if (!length) {
		if (gltf->file_type != GLTF_FILE_GLB_EMBED)
			return nullptr;

		if (gltf->binary_chunk.data)
			return buffer_stream_allocate(gltf->binary_chunk.data, STREAM_IN | STREAM_BINARY,
			                              gltf->binary_chunk.length, gltf->binary_chunk.length,
			                              false, false);
		else
			return gltf_allocate_glb_substream(STRING_ARGS(gltf->binary_chunk.uri),
			                                   gltf->binary_chunk.offset,
			                                   gltf->binary_chunk.length);
	}

	if ((length > 5) && string_equal(uri, 5, STRING_CONST("data:"))) {
		uri += 5;
		length -= 5;

		string_const_t mime_type, encoding, data;
		string_split(uri, length, STRING_CONST(";"), &mime_type, &encoding, false);
		string_split(STRING_ARGS(encoding), STRING_CONST(","), &encoding, &data, false);

		if (string_equal(STRING_ARGS(encoding), STRING_CONST("base64"))) {
			size_t unpacked_length = (data.length / 4) * 3;
			stream_t* stream = gltf_allocate_stream_base64(data.str, data.length, unpacked_length);
			if (stream)
				stream->mime_type = mime_type;
			return stream;
		}

		log_warn(HASH_GLTF, WARNING_UNSUPPORTED, STRING_CONST("Unsupported data uri type"));
		return nullptr;
	}

	stream_t* stream = stream_open(uri, length, mode);
	if (stream)
		return stream;

	string_t full_path = path_allocate_concat(STRING_ARGS(gltf->base_path), uri, length);
	stream = stream_open(STRING_ARGS(full_path), mode);
	string_deallocate(full_path.str);

	return stream;
}

int
gltf_module_stream_initialize(void) {
	memset(&gltf_stream_base64_vtable, 0, sizeof(gltf_stream_base64_vtable));
	gltf_stream_base64_vtable.read = gltf_stream_base64_read;
	gltf_stream_base64_vtable.eos = gltf_stream_base64_eos;
	gltf_stream_base64_vtable.size = gltf_stream_base64_size;
	gltf_stream_base64_vtable.seek = gltf_stream_base64_seek;
	gltf_stream_base64_vtable.tell = gltf_stream_base64_tell;
	gltf_stream_base64_vtable.lastmod = gltf_stream_base64_lastmod;
	gltf_stream_base64_vtable.available_read = gltf_stream_base64_available_read;
	gltf_stream_base64_vtable.finalize = gltf_stream_base64_finalize;

	memset(&gltf_stream_substream_vtable, 0, sizeof(gltf_stream_substream_vtable));
	gltf_stream_substream_vtable.read = gltf_stream_substream_read;
	gltf_stream_substream_vtable.eos = gltf_stream_substream_eos;
	gltf_stream_substream_vtable.size = gltf_stream_substream_size;
	gltf_stream_substream_vtable.seek = gltf_stream_substream_seek;
	gltf_stream_substream_vtable.tell = gltf_stream_substream_tell;
	gltf_stream_substream_vtable.lastmod = gltf_stream_substream_lastmod;
	gltf_stream_substream_vtable.available_read = gltf_stream_substream_available_read;
	gltf_stream_substream_vtable.finalize = gltf_stream_substream_finalize;
	return 0;
}

void
gltf_module_stream_finalize(void) {
}
