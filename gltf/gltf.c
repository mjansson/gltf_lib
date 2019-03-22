/* gltf.c  -  glTF library  -  Public Domain  -  2018 Mattias Jansson / Rampant Pixels
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

#include <foundation/stream.h>
#include <foundation/memory.h>
#include <foundation/json.h>
#include <foundation/path.h>
#include <foundation/log.h>
#include <foundation/hashstrings.h>

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

void
gltf_initialize(gltf_t* gltf) {
	memset(gltf, 0, sizeof(gltf_t));
}

void
gltf_finalize(gltf_t* gltf) {
	if (gltf) {
		gltf_meshes_finalize(gltf);
		gltf_materials_finalize(gltf);
		gltf_nodes_finalize(gltf);
		gltf_scenes_finalize(gltf);
		gltf_buffer_views_finalize(gltf);
		gltf_buffers_finalize(gltf);
		gltf_accessors_finalize(gltf);
		memory_deallocate(gltf->extensions_used);
		memory_deallocate(gltf->extensions_required);
		memory_deallocate(gltf->buffer);
		string_deallocate(gltf->base_path.str);
	}
}

int
gltf_token_to_integer(const gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken,
                      unsigned int* value) {
	FOUNDATION_UNUSED(gltf);
	if (!itoken || ((tokens[itoken].type != JSON_PRIMITIVE) && (tokens[itoken].type != JSON_STRING))) {
		log_error(HASH_GLTF, ERROR_INVALID_VALUE, STRING_CONST("Integer attribute has invalid type"));
		return -1;
	}

	string_const_t strval = json_token_value(buffer, tokens + itoken);
	*value = string_to_uint(STRING_ARGS(strval), false);
	return 0;
}

int
gltf_token_to_boolean(const gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken,
                      bool* value) {
	FOUNDATION_UNUSED(gltf);
	if (!itoken || ((tokens[itoken].type != JSON_PRIMITIVE) && (tokens[itoken].type != JSON_STRING))) {
		log_error(HASH_GLTF, ERROR_INVALID_VALUE, STRING_CONST("Boolean attribute has invalid type"));
		return -1;
	}

	string_const_t strval = json_token_value(buffer, tokens + itoken);
	*value = !string_equal(STRING_ARGS(strval), STRING_CONST("false"));
	return 0;
}

int
gltf_token_to_double(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken,
                     double* value) {
	FOUNDATION_UNUSED(gltf);
	if (!itoken || ((tokens[itoken].type != JSON_STRING) && (tokens[itoken].type != JSON_PRIMITIVE))) {
		log_error(HASH_GLTF, ERROR_INVALID_VALUE, STRING_CONST("Integer attribute has invalid type"));
		return -1;
	}

	string_const_t strval = json_token_value(buffer, tokens + itoken);
	*value = string_to_float64(STRING_ARGS(strval));
	return 0;
}

int
gltf_token_to_double_array(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken,
                           double* values, unsigned int dim) {
	if (!itoken || (tokens[itoken].type != JSON_ARRAY) || (tokens[itoken].value_length != dim)) {
		log_error(HASH_GLTF, ERROR_INVALID_VALUE, STRING_CONST("Double array attribute has invalid type"));
		return -1;
	}

	itoken = tokens[itoken].child;
	for (unsigned int ielem = 0; ielem < dim; ++ielem) {
		if (gltf_token_to_double(gltf, buffer, tokens, itoken, values + ielem))
			return -1;
		itoken = tokens[itoken].sibling;
	}
	return 0;
}

int
gltf_token_to_component_type(const gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken,
                             gltf_component_type* value) {
	unsigned int intval = 0;
	int result = gltf_token_to_integer(gltf, buffer, tokens, itoken, &intval);
	if (!result)
		*value = (gltf_component_type)intval;
	return result;
}

int
gltf_token_to_data_type(const gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken,
                        gltf_data_type* value) {
	FOUNDATION_UNUSED(gltf);
	if (!itoken || (tokens[itoken].type != JSON_STRING)) {
		log_error(HASH_GLTF, ERROR_INVALID_VALUE, STRING_CONST("Data type attribute has invalid type"));
		return -1;
	}

	string_const_t strval = json_token_value(buffer, tokens + itoken);
	hash_t strhash = hash(strval.str, strval.length);
	if (strhash == HASH_SCALAR)
		*value = GLTF_DATA_SCALAR;
	else if (strhash == HASH_VEC2)
		*value = GLTF_DATA_VEC2;
	else if (strhash == HASH_VEC3)
		*value = GLTF_DATA_VEC3;
	else if (strhash == HASH_VEC4)
		*value = GLTF_DATA_VEC4;
	else if (strhash == HASH_MAT2)
		*value = GLTF_DATA_MAT2;
	else if (strhash == HASH_MAT3)
		*value = GLTF_DATA_MAT3;
	else if (strhash == HASH_MAT4)
		*value = GLTF_DATA_MAT4;
	else {
		log_error(HASH_GLTF, ERROR_INVALID_VALUE, STRING_CONST("Data type attribute has invalid value"));
		return -1;
	}
	return 0;
}

static int
gltf_parse_asset(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken) {
	if (tokens[itoken].type != JSON_OBJECT) {
		log_error(HASH_GLTF, ERROR_INVALID_VALUE, STRING_CONST("Main asset attribute has invalid type"));
		return -1;
	}

	itoken = tokens[itoken].child;
	while (itoken) {
		string_const_t identifier = json_token_identifier(buffer, tokens + itoken);
		hash_t identifier_hash = string_hash(STRING_ARGS(identifier));
		if ((identifier_hash == HASH_GENERATOR) && (tokens[itoken].type == JSON_STRING))
			gltf->asset.generator = json_token_value(buffer, tokens + itoken);
		else if ((identifier_hash == HASH_VERSION) && ((tokens[itoken].type == JSON_STRING) ||
		                                               (tokens[itoken].type == JSON_PRIMITIVE)))
			gltf->asset.version = json_token_value(buffer, tokens + itoken);

		itoken = tokens[itoken].sibling;
	}

	return 0;
}

static int
gltf_parse_scene_nodes(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken,
                       gltf_scene_t* scene) {
	FOUNDATION_UNUSED(gltf);
	if (tokens[itoken].type != JSON_ARRAY) {
		log_error(HASH_GLTF, ERROR_INVALID_VALUE, STRING_CONST("Scene nodes attribute has invalid type"));
		return -1;
	}

	size_t num_nodes = tokens[itoken].value_length;
	if (num_nodes > GLTF_MAX_INDEX)
		return -1;
	if (!num_nodes)
		return 0;

	scene->num_nodes = (unsigned int)num_nodes;
	scene->nodes = memory_allocate(HASH_GLTF, sizeof(unsigned int) * num_nodes, 0, MEMORY_PERSISTENT);

	unsigned int icounter = 0;
	size_t inode = tokens[itoken].child;
	while (inode) {
		if ((tokens[inode].type != JSON_STRING) && (tokens[inode].type != JSON_PRIMITIVE))
			return -1;

		string_const_t value = json_token_value(buffer, tokens + inode);
		unsigned int node = string_to_uint(STRING_ARGS(value), false);
		if (node > GLTF_MAX_INDEX)
			return -1;
		scene->nodes[icounter] = node;

		inode = tokens[inode].sibling;
		++icounter;
	}

	return 0;
}

static int
gltf_parse_scenes_scene(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken,
                        gltf_scene_t* scene) {
	if (tokens[itoken].type != JSON_OBJECT)
		return -1;

	int result = 0;
	itoken = tokens[itoken].child;
	while (itoken) {
		string_const_t identifier = json_token_identifier(gltf->buffer, tokens + itoken);
		hash_t identifier_hash = string_hash(STRING_ARGS(identifier));
		if (identifier_hash == HASH_NODES)
			result = gltf_parse_scene_nodes(gltf, buffer, tokens, itoken, scene);
		else if ((identifier_hash == HASH_NAME) && (tokens[itoken].type == JSON_STRING))
			scene->name = json_token_value(buffer, tokens + itoken);
		else if ((identifier_hash == HASH_EXTENSIONS) && (tokens[itoken].type == JSON_STRING))
			scene->extensions = json_token_value(buffer, tokens + itoken);
		else if ((identifier_hash == HASH_EXTRAS) && (tokens[itoken].type == JSON_STRING))
			scene->extras = json_token_value(buffer, tokens + itoken);

		if (result)
			break;
		itoken = tokens[itoken].sibling;
	}

	return result;
}

static int
gltf_parse_scenes(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken) {
	if (tokens[itoken].type != JSON_ARRAY) {
		log_error(HASH_GLTF, ERROR_INVALID_VALUE, STRING_CONST("Main scenes attribute has invalid type"));
		return -1;
	}

	size_t num_scenes = tokens[itoken].value_length;
	if (num_scenes > GLTF_MAX_INDEX)
		return -1;
	if (!num_scenes)
		return 0;

	size_t storage_size = sizeof(gltf_scene_t) * num_scenes;
	gltf_scenes_finalize(gltf);
	gltf->num_scenes = (unsigned int)num_scenes;
	gltf->scenes = memory_allocate(HASH_GLTF, storage_size, 0,
	                               MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);

	int result = 0;
	unsigned int icounter = 0;
	size_t iscene = tokens[itoken].child;
	while (iscene) {
		result = gltf_parse_scenes_scene(gltf, buffer, tokens, iscene, gltf->scenes + icounter);
		if (result)
			break;
		iscene = tokens[iscene].sibling;
		++icounter;
	}

	return result;
}

static int
gltf_parse_scene(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken) {
	if ((tokens[itoken].type != JSON_STRING) && (tokens[itoken].type != JSON_PRIMITIVE)) {
		log_error(HASH_GLTF, ERROR_INVALID_VALUE, STRING_CONST("Main scene attribute has invalid type"));
		return -1;
	}

	string_const_t value = json_token_value(buffer, tokens + itoken);
	gltf->scene = string_to_uint(STRING_ARGS(value), false);
	return 0;
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

	string_deallocate(gltf->base_path.str);
	string_const_t path = stream_path(stream);
	path = path_directory_name(STRING_ARGS(path));
	gltf->base_path = string_clone(STRING_ARGS(path));

	gltf_glb_header_t glb_header;
	if (stream_read(stream, &glb_header, sizeof(glb_header)) != sizeof(glb_header))
		return -1;

	if (glb_header.magic == 0x46546C67)
		return glb_read(gltf, stream);

	stream_seek(stream, 0, STREAM_SEEK_END);
	size_t capacity = stream_tell(stream) - stream_offset;
	stream_seek(stream, stream_offset, STREAM_SEEK_BEGIN);

	memory_deallocate(gltf->buffer);
	gltf->buffer = memory_allocate(HASH_GLTF, capacity, 0, MEMORY_PERSISTENT);
	int result = -1;

	size_t itoken = 0;
	size_t num_tokens = 0;
	size_t token_capacity = capacity / 16;
	json_token_t* tokens = memory_allocate(HASH_GLTF, sizeof(json_token_t) * token_capacity, 0,
	                                       MEMORY_TEMPORARY);

	if (stream_read(stream, gltf->buffer, capacity) != capacity)
		goto exit;

	num_tokens = json_parse(gltf->buffer, capacity, tokens, token_capacity);
	if (num_tokens > token_capacity) {
		tokens = memory_reallocate(tokens, sizeof(json_token_t) * num_tokens, 0,
		                           sizeof(json_token_t) * token_capacity, MEMORY_TEMPORARY);
		token_capacity = num_tokens;
		num_tokens = json_parse(gltf->buffer, capacity, tokens, token_capacity);
		if (num_tokens > token_capacity)
			goto exit;
	}

	if (tokens[0].type != JSON_OBJECT)
		goto exit;

	result = 0;
	itoken = tokens[0].child;
	while (itoken) {
		string_const_t identifier = json_token_identifier(gltf->buffer, tokens + itoken);
		hash_t identifier_hash = string_hash(STRING_ARGS(identifier));
		if (identifier_hash == HASH_ASSET)
			result = gltf_parse_asset(gltf, gltf->buffer, tokens, itoken);
		else if (identifier_hash == HASH_SCENE)
			result = gltf_scene_parse(gltf, gltf->buffer, tokens, itoken);
		else if (identifier_hash == HASH_SCENES)
			result = gltf_scenes_parse(gltf, gltf->buffer, tokens, itoken);
		else if (identifier_hash == HASH_NODES)
			result = gltf_nodes_parse(gltf, gltf->buffer, tokens, itoken);
		else if (identifier_hash == HASH_MATERIALS)
			result = gltf_materials_parse(gltf, gltf->buffer, tokens, itoken);
		else if (identifier_hash == HASH_MESHES)
			result = gltf_meshes_parse(gltf, gltf->buffer, tokens, itoken);
		else if (identifier_hash == HASH_BUFFERS)
			result = gltf_buffers_parse(gltf, gltf->buffer, tokens, itoken);
		else if (identifier_hash == HASH_BUFFERVIEWS)
			result = gltf_buffer_views_parse(gltf, gltf->buffer, tokens, itoken);
		else if (identifier_hash == HASH_ACCESSORS)
			result = gltf_accessors_parse(gltf, gltf->buffer, tokens, itoken);
		else if (identifier_hash == HASH_EXTENSIONSUSED)
			result = gltf_extensions_used_parse(gltf, gltf->buffer, tokens, itoken);
		else if (identifier_hash == HASH_EXTENSIONSREQUIRED)
			result = gltf_extensions_required_parse(gltf, gltf->buffer, tokens, itoken);

		if (result)
			break;
		itoken = tokens[itoken].sibling;
	}

	if (result == 0) {
		log_infof(HASH_GLTF, STRING_CONST("Read glTF file version %.*s - %.*s"),
		          STRING_FORMAT(gltf->asset.version), STRING_FORMAT(gltf->asset.generator));
		log_infof(HASH_GLTF, STRING_CONST("  %u scenes"), gltf->num_scenes);
		for (unsigned int iscene = 0; iscene < gltf->num_scenes; ++iscene) {
			gltf_scene_t* scene = gltf->scenes + iscene;
			log_infof(HASH_GLTF, STRING_CONST("    %u: \"%.*s\" %u nodes"), iscene,
			          STRING_FORMAT(scene->name), scene->num_nodes);
		}
		log_infof(HASH_GLTF, STRING_CONST("  %u nodes"), gltf->num_nodes);
		for (unsigned int inode = 0; inode < gltf->num_nodes; ++inode) {
			gltf_node_t* node = gltf->nodes + inode;
			log_infof(HASH_GLTF, STRING_CONST("    %u: \"%.*s\" mesh %d"), inode,
			          STRING_FORMAT(node->name), (int)node->mesh);
		}
		log_infof(HASH_GLTF, STRING_CONST("  %u meshes"), gltf->num_meshes);
		for (unsigned int imesh = 0; imesh < gltf->num_meshes; ++imesh) {
			gltf_mesh_t* mesh = gltf->meshes + imesh;
			log_infof(HASH_GLTF, STRING_CONST("    %u: \"%.*s\" %d primitives"), imesh,
			          STRING_FORMAT(mesh->name), (int)mesh->num_primitives);
			for (unsigned int iprim = 0; iprim < mesh->num_primitives; ++iprim) {
				gltf_primitive_t* prim = mesh->primitives + iprim;
				log_infof(HASH_GLTF, STRING_CONST("      %u: type %d material %d"), iprim,
				          prim->mode, prim->material);
			}
		}
	}
	else {
		log_infof(HASH_GLTF, STRING_CONST("Failed reading glTF file: %d"), result);
	}

exit:
	memory_deallocate(tokens);
	return result;
}

int
gltf_write(const gltf_t* gltf, stream_t* stream, gltf_write_mode write_mode) {
	FOUNDATION_UNUSED(gltf);
	FOUNDATION_UNUSED(write_mode);

	stream_set_byteorder(stream, BYTEORDER_LITTLEENDIAN);

	return -1;
}
