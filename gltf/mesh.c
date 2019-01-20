/* mesh.c  -  glTF library  -  Public Domain  -  2018 Mattias Jansson / Rampant Pixels
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
#include "mesh.h"
#include "hashstrings.h"

#include <foundation/memory.h>
#include <foundation/json.h>
#include <foundation/log.h>
#include <foundation/hashstrings.h>

void
gltf_meshes_finalize(gltf_t* gltf) {
	if (gltf->meshes) {
		memory_deallocate(gltf->meshes);
	}
}

int
gltf_meshes_parse(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken) {
	if (tokens[itoken].type != JSON_ARRAY) {
		log_error(HASH_GLTF, ERROR_INVALID_VALUE,
		          STRING_CONST("Main meshes attribute has invalid type"));
		return -1;
	}

	size_t num_meshes = tokens[itoken].value_length;
	if (num_meshes > GLTF_MAX_INDEX)
		return -1;
	if (!num_meshes)
		return 0;

	size_t storage_size = sizeof(gltf_mesh_t) * num_meshes;
	gltf_meshes_finalize(gltf);
	gltf->num_meshes = (unsigned int)num_meshes;
	gltf->meshes = memory_allocate(HASH_GLTF, storage_size, 0,
	                                  MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);

	int result = 0;
	unsigned int icounter = 0;
	size_t iscene = tokens[itoken].child;
	while (iscene) {
		result = gltf_meshes_parse_mesh(gltf, buffer, tokens, iscene, gltf->meshes + icounter);
		if (result)
			break;
		iscene = tokens[iscene].sibling;
		++icounter;
	}

	return result;
}
