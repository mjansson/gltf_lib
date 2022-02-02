/* scene.c  -  glTF library  -  Public Domain  -  2018 Mattias Jansson
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
#include "scene.h"
#include "hashstrings.h"

#include <foundation/memory.h>
#include <foundation/json.h>
#include <foundation/log.h>
#include <foundation/hashstrings.h>

void
gltf_scenes_finalize(gltf_t* gltf) {
	if (gltf->scenes) {
		for (uint iscene = 0; iscene < gltf->scenes_count; ++iscene)
			memory_deallocate(gltf->scenes[iscene].nodes);
		memory_deallocate(gltf->scenes);
	}
}

static bool
gltf_scene_parse_nodes(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken, gltf_scene_t* scene) {
	FOUNDATION_UNUSED(gltf);
	if (tokens[itoken].type != JSON_ARRAY) {
		log_error(HASH_GLTF, ERROR_INVALID_VALUE, STRING_CONST("Scene nodes attribute has invalid type"));
		return false;
	}

	size_t nodes_count = tokens[itoken].value_length;
	if (nodes_count > GLTF_MAX_INDEX)
		return false;
	if (!nodes_count)
		return true;

	scene->nodes_count = (uint)nodes_count;
	scene->nodes = memory_allocate(HASH_GLTF, sizeof(uint) * nodes_count, 0, MEMORY_PERSISTENT);

	uint icounter = 0;
	size_t inode = tokens[itoken].child;
	while (inode) {
		if ((tokens[inode].type != JSON_STRING) && (tokens[inode].type != JSON_PRIMITIVE))
			return false;

		string_const_t value = json_token_value(buffer, tokens + inode);
		uint node = string_to_uint(STRING_ARGS(value), false);
		if (node > GLTF_MAX_INDEX)
			return false;
		scene->nodes[icounter] = node;

		inode = tokens[inode].sibling;
		++icounter;
	}

	return true;
}

static int
gltf_scenes_parse_scene(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken, gltf_scene_t* scene) {
	if (tokens[itoken].type != JSON_OBJECT)
		return false;

	itoken = tokens[itoken].child;
	while (itoken) {
		string_const_t identifier = json_token_identifier(gltf->buffer, tokens + itoken);
		hash_t identifier_hash = string_hash(STRING_ARGS(identifier));
		if ((identifier_hash == HASH_NODES) && !gltf_scene_parse_nodes(gltf, buffer, tokens, itoken, scene))
			return false;
		else if ((identifier_hash == HASH_NAME) && (tokens[itoken].type == JSON_STRING))
			scene->name = json_token_value(buffer, tokens + itoken);
		else if ((identifier_hash == HASH_EXTENSIONS) && (tokens[itoken].type == JSON_STRING))
			scene->extensions = json_token_value(buffer, tokens + itoken);
		else if ((identifier_hash == HASH_EXTRAS) && (tokens[itoken].type == JSON_STRING))
			scene->extras = json_token_value(buffer, tokens + itoken);

		itoken = tokens[itoken].sibling;
	}

	return true;
}

bool
gltf_scenes_parse(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken) {
	if (tokens[itoken].type != JSON_ARRAY) {
		log_error(HASH_GLTF, ERROR_INVALID_VALUE, STRING_CONST("Main scenes attribute has invalid type"));
		return false;
	}

	size_t scenes_count = tokens[itoken].value_length;
	if (scenes_count > GLTF_MAX_INDEX)
		return false;
	if (!scenes_count)
		return true;

	size_t storage_size = sizeof(gltf_scene_t) * scenes_count;
	gltf_scenes_finalize(gltf);
	gltf->scenes_count = (uint)scenes_count;
	gltf->scenes = memory_allocate(HASH_GLTF, storage_size, 0, MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);

	uint icounter = 0;
	size_t iscene = tokens[itoken].child;
	while (iscene) {
		if (!gltf_scenes_parse_scene(gltf, buffer, tokens, iscene, gltf->scenes + icounter))
			return false;
		iscene = tokens[iscene].sibling;
		++icounter;
	}

	return true;
}

bool
gltf_scene_parse(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken) {
	if ((tokens[itoken].type != JSON_STRING) && (tokens[itoken].type != JSON_PRIMITIVE)) {
		log_error(HASH_GLTF, ERROR_INVALID_VALUE, STRING_CONST("Main scene attribute has invalid type"));
		return false;
	}

	string_const_t value = json_token_value(buffer, tokens + itoken);
	gltf->scene = string_to_uint(STRING_ARGS(value), false);
	return true;
}

gltf_scene_t*
gltf_scene_add(gltf_t* gltf) {
	uint scenes_count = gltf->scenes_count;
	size_t old_storage_size = sizeof(gltf_scene_t) * scenes_count;
	size_t storage_size = sizeof(gltf_scene_t) * (++scenes_count);
	gltf->scenes_count = (uint)scenes_count;
	gltf->scenes = memory_reallocate(gltf->scenes, storage_size, 0, old_storage_size, MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);
	if ((gltf->scene == GLTF_INVALID_INDEX) && (scenes_count == 1))
		gltf->scene = 0;
	return pointer_offset(gltf->scenes, old_storage_size);
}

void
gltf_scene_add_node(gltf_t* gltf, gltf_scene_t* scene, uint node) {
	FOUNDATION_UNUSED(gltf);
	uint nodes_count = scene->nodes_count;
	size_t old_storage_size = sizeof(uint) * nodes_count;
	size_t storage_size = sizeof(uint) * (++nodes_count);
	scene->nodes_count = (uint)nodes_count;
	scene->nodes = memory_reallocate(scene->nodes, storage_size, 0, old_storage_size, MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);
	scene->nodes[nodes_count - 1] = node;
}
