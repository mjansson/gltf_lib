/* gltf.c  -  glTF library  -  Public Domain  -  2018 Mattias Jansson
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
#include "hashstrings.h"

#include <foundation/stream.h>
#include <foundation/memory.h>
#include <foundation/json.h>
#include <foundation/path.h>
#include <foundation/log.h>
#include <foundation/hashstrings.h>

extern int
gltf_module_stream_initialize(void);

extern void
gltf_module_stream_finalize(void);

int
gltf_module_initialize(gltf_config_t config) {
	FOUNDATION_UNUSED(config);
	if (gltf_module_stream_initialize())
		return -1;
	return 0;
}

void
gltf_module_finalize(void) {
	gltf_module_stream_finalize();
}

bool
gltf_module_is_initialized(void) {
	return true;
}

void
gltf_module_parse_config(const char* path, size_t path_size, const char* buffer, size_t size,
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
		gltf_textures_finalize(gltf);
		gltf_nodes_finalize(gltf);
		gltf_scenes_finalize(gltf);
		gltf_images_finalize(gltf);
		gltf_buffer_views_finalize(gltf);
		gltf_buffers_finalize(gltf);
		gltf_accessors_finalize(gltf);
		memory_deallocate(gltf->extensions_used);
		memory_deallocate(gltf->extensions_required);
		memory_deallocate(gltf->buffer);
		string_deallocate(gltf->base_path.str);
		memory_deallocate(gltf->binary_chunk.data);
		string_deallocate(gltf->binary_chunk.uri.str);
	}
}

bool
gltf_token_to_integer(const gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken,
                      unsigned int* value) {
	FOUNDATION_UNUSED(gltf);
	if (!itoken || ((tokens[itoken].type != JSON_PRIMITIVE) && (tokens[itoken].type != JSON_STRING))) {
		log_error(HASH_GLTF, ERROR_INVALID_VALUE, STRING_CONST("Integer attribute has invalid type"));
		return false;
	}

	string_const_t strval = json_token_value(buffer, tokens + itoken);
	*value = string_to_uint(STRING_ARGS(strval), false);
	return true;
}

bool
gltf_token_to_boolean(const gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken, bool* value) {
	FOUNDATION_UNUSED(gltf);
	if (!itoken || ((tokens[itoken].type != JSON_PRIMITIVE) && (tokens[itoken].type != JSON_STRING))) {
		log_error(HASH_GLTF, ERROR_INVALID_VALUE, STRING_CONST("Boolean attribute has invalid type"));
		return false;
	}

	string_const_t strval = json_token_value(buffer, tokens + itoken);
	*value = !string_equal(STRING_ARGS(strval), STRING_CONST("false"));
	return true;
}

bool
gltf_token_to_double(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken, double* value) {
	FOUNDATION_UNUSED(gltf);
	if (!itoken || ((tokens[itoken].type != JSON_STRING) && (tokens[itoken].type != JSON_PRIMITIVE))) {
		log_error(HASH_GLTF, ERROR_INVALID_VALUE, STRING_CONST("Integer attribute has invalid type"));
		return false;
	}

	string_const_t strval = json_token_value(buffer, tokens + itoken);
	*value = string_to_float64(STRING_ARGS(strval));
	return true;
}

bool
gltf_token_to_integer_array(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken, unsigned int* values,
                            unsigned int dim) {
	unsigned int array_dim = tokens[itoken].value_length;
	if (!itoken || (tokens[itoken].type != JSON_ARRAY) || (array_dim > dim)) {
		log_error(HASH_GLTF, ERROR_INVALID_VALUE, STRING_CONST("Integer array attribute has invalid type"));
		return -false;
	}

	itoken = tokens[itoken].child;
	for (unsigned int ielem = 0; ielem < array_dim; ++ielem) {
		if (!gltf_token_to_integer(gltf, buffer, tokens, itoken, values + ielem))
			return false;
		itoken = tokens[itoken].sibling;
	}
	return true;
}

bool
gltf_token_to_double_array(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken, double* values,
                           unsigned int dim) {
	unsigned int array_dim = tokens[itoken].value_length;
	if (!itoken || (tokens[itoken].type != JSON_ARRAY) || (array_dim > dim)) {
		log_error(HASH_GLTF, ERROR_INVALID_VALUE, STRING_CONST("Double array attribute has invalid type"));
		return false;
	}

	itoken = tokens[itoken].child;
	for (unsigned int ielem = 0; ielem < array_dim; ++ielem) {
		if (!gltf_token_to_double(gltf, buffer, tokens, itoken, values + ielem))
			return false;
		itoken = tokens[itoken].sibling;
	}
	return true;
}

bool
gltf_token_to_component_type(const gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken,
                             gltf_component_type* value) {
	unsigned int intval = 0;
	if (!gltf_token_to_integer(gltf, buffer, tokens, itoken, &intval))
		return false;
	*value = (gltf_component_type)intval;
	return true;
}

bool
gltf_token_to_data_type(const gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken,
                        gltf_data_type* value) {
	FOUNDATION_UNUSED(gltf);
	if (!itoken || (tokens[itoken].type != JSON_STRING)) {
		log_error(HASH_GLTF, ERROR_INVALID_VALUE, STRING_CONST("Data type attribute has invalid type"));
		return false;
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
		return false;
	}
	return true;
}

static bool
gltf_parse_asset(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken) {
	if (tokens[itoken].type != JSON_OBJECT) {
		log_error(HASH_GLTF, ERROR_INVALID_VALUE, STRING_CONST("Main asset attribute has invalid type"));
		return false;
	}

	itoken = tokens[itoken].child;
	while (itoken) {
		string_const_t identifier = json_token_identifier(buffer, tokens + itoken);
		hash_t identifier_hash = string_hash(STRING_ARGS(identifier));
		if ((identifier_hash == HASH_GENERATOR) && (tokens[itoken].type == JSON_STRING))
			gltf->asset.generator = json_token_value(buffer, tokens + itoken);
		else if ((identifier_hash == HASH_VERSION) &&
		         ((tokens[itoken].type == JSON_STRING) || (tokens[itoken].type == JSON_PRIMITIVE)))
			gltf->asset.version = json_token_value(buffer, tokens + itoken);

		itoken = tokens[itoken].sibling;
	}

	return true;
}

bool
gltf_read(gltf_t* gltf, stream_t* stream) {
	stream_set_byteorder(stream, BYTEORDER_LITTLEENDIAN);
	size_t stream_offset = stream_tell(stream);

	string_deallocate(gltf->base_path.str);
	string_const_t path = stream_path(stream);
	path = path_directory_name(STRING_ARGS(path));
	gltf->base_path = string_clone(STRING_ARGS(path));

	gltf_glb_header_t glb_header;
	if (stream_read(stream, &glb_header, sizeof(glb_header)) != sizeof(glb_header))
		return false;

	size_t json_size = 0;
	if (glb_header.magic == 0x46546C67) {
		if (glb_header.version != 2) {
			log_warn(HASH_GLTF, WARNING_UNSUPPORTED, STRING_CONST("Unsupported GLB version"));
			return false;
		}

		uint32_t chunk_length = stream_read_uint32(stream);
		uint32_t chunk_type = stream_read_uint32(stream);
		if (chunk_type != 0x4E4F534A) {
			log_warn(HASH_GLTF, WARNING_INVALID_VALUE, STRING_CONST("Invalid GLB first chunk, expected JSON"));
			return false;
		}
		size_t max_size = stream_size(stream);
		max_size = (stream_offset < max_size) ? (max_size - stream_offset) : 0;
		if (!chunk_length || (chunk_length % 4) || (max_size && (chunk_length >= max_size))) {
			log_warn(HASH_GLTF, WARNING_INVALID_VALUE, STRING_CONST("Invalid GLB JSON chunk length"));
			return false;
		}

		json_size = chunk_length;
		gltf->file_type = GLTF_FILE_GLB;
	} else {
		stream_seek(stream, 0, STREAM_SEEK_END);
		json_size = stream_tell(stream) - stream_offset;
		stream_seek(stream, (ssize_t)stream_offset, STREAM_SEEK_BEGIN);
		gltf->file_type = GLTF_FILE_GLTF;
	}

	bool success = false;
	if (json_size > 0x7FFFFFFF) {
		log_warn(HASH_GLTF, WARNING_INVALID_VALUE, STRING_CONST("Invalid glTF/GLB JSON length"));
		return false;
	}

	memory_deallocate(gltf->buffer);
	gltf->buffer = memory_allocate(HASH_GLTF, json_size, 0, MEMORY_PERSISTENT);

	size_t itoken = 0;
	size_t num_tokens = 0;
	size_t token_capacity = json_size / 10;
	json_token_t* tokens = memory_allocate(HASH_GLTF, sizeof(json_token_t) * token_capacity, 0, MEMORY_TEMPORARY);

	if (stream_read(stream, gltf->buffer, json_size) != json_size)
		goto exit;

	if (gltf->file_type == GLTF_FILE_GLB) {
		// Check if we have an embedded data chunk
		uint32_t chunk_length = stream_read_uint32(stream);
		uint32_t chunk_type = stream_read_uint32(stream);
		if ((chunk_type == 0x004E4942) && chunk_length) {
			gltf->file_type = GLTF_FILE_GLB_EMBED;
			gltf->binary_chunk.offset = stream_tell(stream);
			gltf->binary_chunk.length = chunk_length;
			if (stream->persistent && stream->reliable && stream->inorder && !stream->sequential) {
				gltf->binary_chunk.uri = string_clone_string(stream_path(stream));
				gltf->binary_chunk.data = nullptr;
			} else {
				gltf->binary_chunk.uri = string(0, 0);
				gltf->binary_chunk.data = memory_allocate(HASH_GLTF, chunk_length, 0, MEMORY_PERSISTENT);
				stream_read(stream, gltf->binary_chunk.data, chunk_length);
			}
		}
	}

	num_tokens = json_parse(gltf->buffer, json_size, tokens, token_capacity);
	if (num_tokens > token_capacity) {
		tokens = memory_reallocate(tokens, sizeof(json_token_t) * num_tokens, 0, sizeof(json_token_t) * token_capacity,
		                           MEMORY_TEMPORARY);
		token_capacity = num_tokens;
		num_tokens = json_parse(gltf->buffer, json_size, tokens, token_capacity);
		if (num_tokens > token_capacity)
			goto exit;
	}

	if (tokens[0].type != JSON_OBJECT)
		goto exit;

	success = true;
	itoken = tokens[0].child;
	while (itoken) {
		string_const_t identifier = json_token_identifier(gltf->buffer, tokens + itoken);
		hash_t identifier_hash = string_hash(STRING_ARGS(identifier));
		if (identifier_hash == HASH_ASSET)
			success = gltf_parse_asset(gltf, gltf->buffer, tokens, itoken);
		else if (identifier_hash == HASH_SCENE)
			success = gltf_scene_parse(gltf, gltf->buffer, tokens, itoken);
		else if (identifier_hash == HASH_SCENES)
			success = gltf_scenes_parse(gltf, gltf->buffer, tokens, itoken);
		else if (identifier_hash == HASH_NODES)
			success = gltf_nodes_parse(gltf, gltf->buffer, tokens, itoken);
		else if (identifier_hash == HASH_MATERIALS)
			success = gltf_materials_parse(gltf, gltf->buffer, tokens, itoken);
		else if (identifier_hash == HASH_MESHES)
			success = gltf_meshes_parse(gltf, gltf->buffer, tokens, itoken);
		else if (identifier_hash == HASH_BUFFERS)
			success = gltf_buffers_parse(gltf, gltf->buffer, tokens, itoken);
		else if (identifier_hash == HASH_BUFFERVIEWS)
			success = gltf_buffer_views_parse(gltf, gltf->buffer, tokens, itoken);
		else if (identifier_hash == HASH_ACCESSORS)
			success = gltf_accessors_parse(gltf, gltf->buffer, tokens, itoken);
		else if (identifier_hash == HASH_TEXTURES)
			success = gltf_textures_parse(gltf, gltf->buffer, tokens, itoken);
		else if (identifier_hash == HASH_IMAGES)
			success = gltf_images_parse(gltf, gltf->buffer, tokens, itoken);
		else if (identifier_hash == HASH_EXTENSIONSUSED)
			success = gltf_extensions_used_parse(gltf, gltf->buffer, tokens, itoken);
		else if (identifier_hash == HASH_EXTENSIONSREQUIRED)
			success = gltf_extensions_required_parse(gltf, gltf->buffer, tokens, itoken);

		if (!success)
			break;
		itoken = tokens[itoken].sibling;
	}

	if (success) {
		log_infof(HASH_GLTF, STRING_CONST("Read %s file version %.*s - %.*s"),
		          (gltf->file_type < GLTF_FILE_GLB) ? "glTF" : "GLB", STRING_FORMAT(gltf->asset.version),
		          STRING_FORMAT(gltf->asset.generator));
		log_infof(HASH_GLTF, STRING_CONST("  %u scenes"), gltf->num_scenes);
		for (unsigned int iscene = 0; iscene < gltf->num_scenes; ++iscene) {
			gltf_scene_t* scene = gltf->scenes + iscene;
			log_infof(HASH_GLTF, STRING_CONST("    %u: \"%.*s\" %u nodes"), iscene, STRING_FORMAT(scene->name),
			          scene->num_nodes);
		}
		log_infof(HASH_GLTF, STRING_CONST("  %u nodes"), gltf->num_nodes);
		for (unsigned int inode = 0; inode < gltf->num_nodes; ++inode) {
			gltf_node_t* node = gltf->nodes + inode;
			log_infof(HASH_GLTF, STRING_CONST("    %u: \"%.*s\" mesh %d"), inode, STRING_FORMAT(node->name),
			          (int)node->mesh);
		}
		log_infof(HASH_GLTF, STRING_CONST("  %u meshes"), gltf->num_meshes);
		for (unsigned int imesh = 0; imesh < gltf->num_meshes; ++imesh) {
			gltf_mesh_t* mesh = gltf->meshes + imesh;
			log_infof(HASH_GLTF, STRING_CONST("    %u: \"%.*s\" %d primitives"), imesh, STRING_FORMAT(mesh->name),
			          (int)mesh->num_primitives);
			for (unsigned int iprim = 0; iprim < mesh->num_primitives; ++iprim) {
				gltf_primitive_t* prim = mesh->primitives + iprim;
				log_infof(HASH_GLTF, STRING_CONST("      %u: type %d material %d"), iprim, prim->mode, prim->material);
			}
		}
		log_infof(HASH_GLTF, STRING_CONST("  %u textures"), gltf->num_textures);
		log_infof(HASH_GLTF, STRING_CONST("  %u images"), gltf->num_images);
	} else {
		log_info(HASH_GLTF, STRING_CONST("Failed reading glTF file"));
	}

exit:
	memory_deallocate(tokens);
	return success;
}

bool
gltf_write(const gltf_t* gltf, stream_t* stream) {
	FOUNDATION_UNUSED(gltf);

	stream_set_byteorder(stream, BYTEORDER_LITTLEENDIAN);

	return false;
}
