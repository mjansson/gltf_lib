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
#include <foundation/array.h>
#include <foundation/virtualarray.h>
#include <foundation/log.h>
#include <foundation/hashstrings.h>

#if FOUNDATION_COMPILER_CLANG
#if __has_warning("-Wfloat-equal")
#pragma clang diagnostic ignored "-Wfloat-equal"
#endif
#endif

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
                         const struct json_token_t* tokens, size_t token_count) {
	FOUNDATION_UNUSED(path);
	FOUNDATION_UNUSED(path_size);
	FOUNDATION_UNUSED(buffer);
	FOUNDATION_UNUSED(size);
	FOUNDATION_UNUSED(tokens);
	FOUNDATION_UNUSED(token_count);
}

void
gltf_initialize(gltf_t* gltf) {
	memset(gltf, 0, sizeof(gltf_t));
	gltf->scene = GLTF_INVALID_INDEX;
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
		string_array_deallocate(gltf->string_array);
		virtualarray_deallocate(gltf->output_buffer);
	}
}

bool
gltf_token_to_integer(const gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken, uint* value) {
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
gltf_token_to_real(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken, real* value) {
	FOUNDATION_UNUSED(gltf);
	if (!itoken || ((tokens[itoken].type != JSON_STRING) && (tokens[itoken].type != JSON_PRIMITIVE))) {
		log_error(HASH_GLTF, ERROR_INVALID_VALUE, STRING_CONST("Integer attribute has invalid type"));
		return false;
	}

	string_const_t strval = json_token_value(buffer, tokens + itoken);
	*value = string_to_float32(STRING_ARGS(strval));
	return true;
}

bool
gltf_token_to_integer_array(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken, uint* values,
                            uint dim) {
	uint array_dim = tokens[itoken].value_length;
	if (!itoken || (tokens[itoken].type != JSON_ARRAY) || (array_dim > dim)) {
		log_error(HASH_GLTF, ERROR_INVALID_VALUE, STRING_CONST("Integer array attribute has invalid type"));
		return -false;
	}

	itoken = tokens[itoken].child;
	for (uint ielem = 0; ielem < array_dim; ++ielem) {
		if (!gltf_token_to_integer(gltf, buffer, tokens, itoken, values + ielem))
			return false;
		itoken = tokens[itoken].sibling;
	}
	return true;
}

bool
gltf_token_to_real_array(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken, real* values,
                         uint dim) {
	uint array_dim = tokens[itoken].value_length;
	if (!itoken || (tokens[itoken].type != JSON_ARRAY) || (array_dim > dim)) {
		log_error(HASH_GLTF, ERROR_INVALID_VALUE, STRING_CONST("Double array attribute has invalid type"));
		return false;
	}

	itoken = tokens[itoken].child;
	for (uint ielem = 0; ielem < array_dim; ++ielem) {
		if (!gltf_token_to_real(gltf, buffer, tokens, itoken, values + ielem))
			return false;
		itoken = tokens[itoken].sibling;
	}
	return true;
}

bool
gltf_token_to_component_type(const gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken,
                             gltf_component_type* value) {
	uint intval = 0;
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
	size_t token_count = 0;
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

	token_count = json_parse(gltf->buffer, json_size, tokens, token_capacity);
	if (token_count > token_capacity) {
		tokens = memory_reallocate(tokens, sizeof(json_token_t) * token_count, 0, sizeof(json_token_t) * token_capacity,
		                           MEMORY_TEMPORARY);
		token_capacity = token_count;
		token_count = json_parse(gltf->buffer, json_size, tokens, token_capacity);
		if (token_count > token_capacity)
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
		log_infof(HASH_GLTF, STRING_CONST("  %u scenes"), array_count(gltf->scenes));
		for (uint iscene = 0, scenes_count = array_count(gltf->scenes); iscene < scenes_count; ++iscene) {
			gltf_scene_t* scene = gltf->scenes + iscene;
			log_infof(HASH_GLTF, STRING_CONST("    %u: \"%.*s\" %u nodes"), iscene, STRING_FORMAT(scene->name),
			          array_count(scene->nodes));
		}
		log_infof(HASH_GLTF, STRING_CONST("  %u nodes"), array_count(gltf->nodes));
		for (uint inode = 0, nodes_count = array_count(gltf->nodes); inode < nodes_count; ++inode) {
			gltf_node_t* node = gltf->nodes + inode;
			log_infof(HASH_GLTF, STRING_CONST("    %u: \"%.*s\" mesh %d"), inode, STRING_FORMAT(node->name),
			          (int)node->mesh);
		}
		log_infof(HASH_GLTF, STRING_CONST("  %u meshes"), array_count(gltf->meshes));
		for (uint imesh = 0, meshes_count = array_count(gltf->meshes); imesh < meshes_count; ++imesh) {
			gltf_mesh_t* mesh = gltf->meshes + imesh;
			log_infof(HASH_GLTF, STRING_CONST("    %u: \"%.*s\" %u primitives"), imesh, STRING_FORMAT(mesh->name),
			          array_count(mesh->primitives));
			for (uint iprim = 0, primitives_count = array_count(mesh->primitives); iprim < primitives_count; ++iprim) {
				gltf_primitive_t* prim = mesh->primitives + iprim;
				log_infof(HASH_GLTF, STRING_CONST("      %u: type %d material %d"), iprim, prim->mode, prim->material);
			}
		}
		log_infof(HASH_GLTF, STRING_CONST("  %u textures"), gltf->textures_count);
		log_infof(HASH_GLTF, STRING_CONST("  %u images"), gltf->images_count);
	} else {
		log_info(HASH_GLTF, STRING_CONST("Failed reading glTF file"));
	}

exit:
	memory_deallocate(tokens);
	return success;
}

bool
gltf_write(const gltf_t* gltf, stream_t* stream) {
	stream_set_byteorder(stream, BYTEORDER_LITTLEENDIAN);

	size_t file_size_offset = 8;
	size_t json_size_offset = 12;
	size_t json_start_offset = 20;
	if ((gltf->file_type == GLTF_FILE_GLB) || (gltf->file_type == GLTF_FILE_GLB_EMBED)) {
		// First write GLB header
		gltf_glb_header_t glb_header;
		glb_header.magic = 0x46546C67;
		glb_header.version = 2;
		glb_header.length = 0;

		stream_set_binary(stream, true);
		stream_write(stream, &glb_header, sizeof(glb_header));

		// Now JSON chunk header (size, type)
		stream_write_uint32(stream, 0);
		stream_write_uint32(stream, 0x4E4F534A);

		json_start_offset = stream_tell(stream);
	}

	stream_set_binary(stream, false);

	stream_write(stream, STRING_CONST("{\n"));
	stream_write(stream, STRING_CONST("\t\"asset\": {\n"));
	stream_write(stream, STRING_CONST("\t\t\"generator\": \"gltf_lib\",\n"));
	stream_write(stream, STRING_CONST("\t\t\"version\": \"2.0\"\n"));
	stream_write(stream, STRING_CONST("\t}"));

	if (gltf->output_buffer && gltf->output_buffer->count) {
		char path_buffer[BUILD_MAX_PATHLEN];
		string_t buffer_uri = string(0, 0);
		string_const_t buffer_relative_uri;

		stream_write(stream, STRING_CONST(",\n\t\"buffers\": [\n"));
		stream_write(stream, STRING_CONST("\t\t{\n"));
		if (gltf->file_type == GLTF_FILE_GLB_EMBED) {
			// Leave the buffer URI undefined as per GLB spec
		} else if (gltf->file_type == GLTF_FILE_GLTF_EMBED) {
			// TODO: Implement
		} else {
			string_const_t base_uri = stream_path(stream);
			base_uri = path_base_file_name_with_directory(STRING_ARGS(base_uri));
			buffer_uri = string_concat(path_buffer, sizeof(path_buffer), STRING_ARGS(base_uri), STRING_CONST(".bin"));
			buffer_relative_uri = path_file_name(STRING_ARGS(buffer_uri));
			stream_write_format(stream, STRING_CONST("\t\t\t\"uri\": \"%.*s\",\n"), STRING_FORMAT(buffer_relative_uri));
		}
		stream_write_format(stream, STRING_CONST("\t\t\t\"byteLength\": %" PRIsize "\n"), gltf->output_buffer->count);
		stream_write(stream, STRING_CONST("\t\t}\n"));
		stream_write(stream, STRING_CONST("\t]"));

		if ((gltf->file_type == GLTF_FILE_GLB) || (gltf->file_type == GLTF_FILE_GLTF)) {
			stream_t* buffer_stream =
			    stream_open(STRING_ARGS(buffer_uri), STREAM_OUT | STREAM_BINARY | STREAM_CREATE | STREAM_TRUNCATE);
			if (stream) {
				stream_write(buffer_stream, gltf->output_buffer->storage, gltf->output_buffer->count);
				stream_deallocate(buffer_stream);
			} else {
				log_errorf(HASH_GLTF, ERROR_SYSTEM_CALL_FAIL, STRING_CONST("Failed to open binary buffer stream: %.*s"),
				           STRING_FORMAT(buffer_uri));
				return false;
			}
		}
	}

	if (array_count(gltf->buffer_views)) {
		stream_write(stream, STRING_CONST(",\n\t\"bufferViews\": [\n"));
		for (uint iview = 0, view_count = array_count(gltf->buffer_views); iview < view_count; ++iview) {
			stream_write(stream, STRING_CONST("\t\t{\n"));
			stream_write(stream, STRING_CONST("\t\t\t\"buffer\": 0,\n"));
			stream_write_format(stream, STRING_CONST("\t\t\t\"byteOffset\": %u,\n"),
			                    gltf->buffer_views[iview].byte_offset);
			stream_write_format(stream, STRING_CONST("\t\t\t\"byteLength\": %u\n"),
			                    gltf->buffer_views[iview].byte_length);
			stream_write(stream, STRING_CONST("\t\t}"));
			if (iview < (view_count - 1))
				stream_write(stream, STRING_CONST(","));
			stream_write(stream, STRING_CONST("\n"));
		}
		stream_write(stream, STRING_CONST("\t]"));
	}

	if (array_count(gltf->accessors)) {
		stream_write(stream, STRING_CONST(",\n\t\"accessors\": [\n"));
		for (uint iacc = 0, accessor_count = array_count(gltf->accessors); iacc < accessor_count; ++iacc) {
			stream_write(stream, STRING_CONST("\t\t{\n"));
			stream_write_format(stream, STRING_CONST("\t\t\t\"bufferView\": %u,\n"), gltf->accessors[iacc].buffer_view);
			stream_write_format(stream, STRING_CONST("\t\t\t\"componentType\": %u,\n"),
			                    gltf->accessors[iacc].component_type);
			stream_write_format(stream, STRING_CONST("\t\t\t\"count\": %u,\n"), gltf->accessors[iacc].count);

			uint component_count = 0;
			const char* typestr = "SCALAR";
			switch (gltf->accessors[iacc].type) {
				case GLTF_DATA_VEC2:
					typestr = "VEC2";
					component_count = 2;
					break;
				case GLTF_DATA_VEC3:
					typestr = "VEC3";
					component_count = 3;
					break;
				case GLTF_DATA_VEC4:
					typestr = "VEC4";
					component_count = 4;
					break;
				case GLTF_DATA_MAT2:
					typestr = "MAT2";
					break;
				case GLTF_DATA_MAT3:
					typestr = "MAT3";
					break;
				case GLTF_DATA_MAT4:
					typestr = "MAT4";
					break;
				case GLTF_DATA_SCALAR:
				default:
					break;
			}
			stream_write_format(stream, STRING_CONST("\t\t\t\"type\": \"%s\""), typestr);
			if (component_count) {
				stream_write(stream, STRING_CONST(",\n\t\t\t\"min\": [\n"));
				for (uint icomp = 0; icomp < component_count; ++icomp) {
					stream_write(stream, STRING_CONST("\t\t\t\t"));
					if (gltf->accessors[iacc].component_type == GLTF_COMPONENT_FLOAT)
						stream_write_float32(stream, gltf->accessors[iacc].min[icomp]);
					else
						stream_write_uint32(stream, (uint)gltf->accessors[iacc].min[icomp]);
					if (icomp < (component_count - 1))
						stream_write(stream, STRING_CONST(","));
					stream_write(stream, STRING_CONST("\n"));
				}
				stream_write(stream, STRING_CONST("\t\t\t],\n\t\t\t\"max\": [\n"));
				for (uint icomp = 0; icomp < component_count; ++icomp) {
					stream_write(stream, STRING_CONST("\t\t\t\t"));
					if (gltf->accessors[iacc].component_type == GLTF_COMPONENT_FLOAT)
						stream_write_float32(stream, gltf->accessors[iacc].max[icomp]);
					else
						stream_write_uint32(stream, (uint)gltf->accessors[iacc].max[icomp]);
					if (icomp < (component_count - 1))
						stream_write(stream, STRING_CONST(","));
					stream_write(stream, STRING_CONST("\n"));
				}
				stream_write(stream, STRING_CONST("\t\t\t]"));
			}
			stream_write(stream, STRING_CONST("\n\t\t}"));
			if (iacc < (accessor_count - 1))
				stream_write(stream, STRING_CONST(","));
			stream_write(stream, STRING_CONST("\n"));
		}
		stream_write(stream, STRING_CONST("\t]"));
	}

	if (array_count(gltf->materials)) {
		stream_write(stream, STRING_CONST(",\n\t\"materials\": ["));
		for (uint imat = 0, material_count = array_count(gltf->materials); imat < material_count; ++imat) {
			gltf_material_t* material = gltf->materials + imat;
			if (imat > 0)
				stream_write(stream, STRING_CONST(","));
			stream_write(stream, STRING_CONST("\n\t\t{\n"));
			string_const_t material_name = material->name;
			if (!material_name.length)
				material_name = string_const(STRING_CONST("<unnamed>"));
			stream_write_format(stream, STRING_CONST("\t\t\t\"name\": \"%.*s\""), STRING_FORMAT(material_name));
			stream_write(stream, STRING_CONST(",\n\t\t\t\"pbrMetallicRoughness\": {"));
			stream_write_format(stream, STRING_CONST("\n\t\t\t\t\"baseColorFactor\": [%f, %f, %f, %f]"),
			                    (double)material->metallic_roughness.base_color_factor[0],
			                    (double)material->metallic_roughness.base_color_factor[1],
			                    (double)material->metallic_roughness.base_color_factor[2],
			                    (double)material->metallic_roughness.base_color_factor[3]);
			stream_write(stream, STRING_CONST("\n\t\t\t}"));
			stream_write(stream, STRING_CONST("\n\t\t}"));
		}
		stream_write(stream, STRING_CONST("\n\t]"));
	}

	if (array_count(gltf->meshes)) {
		stream_write(stream, STRING_CONST(",\n\t\"meshes\": [\n"));
		for (uint imesh = 0, meshes_count = array_count(gltf->meshes); imesh < meshes_count; ++imesh) {
			gltf_mesh_t* mesh = gltf->meshes + imesh;
			stream_write(stream, STRING_CONST("\t\t{\n"));
			string_const_t mesh_name = mesh->name;
			if (!mesh_name.length)
				mesh_name = string_const(STRING_CONST("<unnamed>"));
			stream_write_format(stream, STRING_CONST("\t\t\t\"name\": \"%.*s\""), STRING_FORMAT(mesh_name));
			uint primitives_count = array_count(mesh->primitives);
			if (primitives_count)
				stream_write(stream, STRING_CONST(",\n\t\t\t\"primitives\": [\n"));
			for (uint iprim = 0; iprim < primitives_count; ++iprim) {
				gltf_primitive_t* primitive = mesh->primitives + iprim;
				stream_write(stream, STRING_CONST("\t\t\t\t{"));
				uint token_count = 0;
				uint attrib_count = 0;
				for (uint iattrib = 0; iattrib < GLTF_ATTRIBUTE_COUNT; ++iattrib) {
					if (primitive->attributes[iattrib] == GLTF_INVALID_INDEX)
						continue;
					if (attrib_count == 0) {
						stream_write(stream, STRING_CONST("\n\t\t\t\t\t\"attributes\": {\n"));
					} else {
						stream_write(stream, STRING_CONST(",\n"));
					}

					const char* attrib_name = "POSITION";
					switch (iattrib) {
						case GLTF_NORMAL:
							attrib_name = "NORMAL";
							break;
						case GLTF_TANGENT:
							attrib_name = "TANGENT";
							break;
						case GLTF_TEXCOORD_0:
							attrib_name = "TEXCOORD_0";
							break;
						case GLTF_TEXCOORD_1:
							attrib_name = "TEXCOORD_1";
							break;
						case GLTF_COLOR_0:
							attrib_name = "COLOR_0";
							break;
						case GLTF_JOINTS_0:
							attrib_name = "JOINTS_0";
							break;
						case GLTF_WEIGHTS_0:
							attrib_name = "WEIGHTS_0";
							break;
						default:
							break;
					}
					stream_write_format(stream, STRING_CONST("\t\t\t\t\t\t\"%s\": %u"), attrib_name,
					                    primitive->attributes[iattrib]);
					++attrib_count;
				}
				if (attrib_count) {
					stream_write(stream, STRING_CONST("\n\t\t\t\t\t}"));
					++token_count;
				}
				if (primitive->indices != GLTF_INVALID_INDEX) {
					if (token_count)
						stream_write(stream, STRING_CONST(","));
					stream_write_format(stream, STRING_CONST("\n\t\t\t\t\t\"indices\": %u"), primitive->indices);
					++token_count;
				}
				if (array_count(gltf->materials)) {
					if (token_count)
						stream_write(stream, STRING_CONST(","));
					stream_write_format(stream, STRING_CONST("\n\t\t\t\t\t\"material\": %u"), primitive->material);
					++token_count;
				}
				stream_write(stream, STRING_CONST("\n\t\t\t\t}"));
				if (iprim < (primitives_count - 1))
					stream_write(stream, STRING_CONST(","));
				stream_write(stream, STRING_CONST("\n"));
			}
			if (primitives_count)
				stream_write(stream, STRING_CONST("\t\t\t]\n"));
			stream_write(stream, STRING_CONST("\n\t\t}"));
			if (imesh < (meshes_count - 1))
				stream_write(stream, STRING_CONST(","));
			stream_write(stream, STRING_CONST("\n"));
		}
		stream_write(stream, STRING_CONST("\t]"));
	}

	if (array_count(gltf->nodes)) {
		stream_write(stream, STRING_CONST(",\n\t\"nodes\": [\n"));
		for (uint inode = 0, nodes_count = array_count(gltf->nodes); inode < nodes_count; ++inode) {
			gltf_node_t* node = gltf->nodes + inode;
			stream_write(stream, STRING_CONST("\t\t{\n"));
			string_const_t node_name = node->name;
			if (!node_name.length)
				node_name = string_const(STRING_CONST("<unnamed>"));
			stream_write_format(stream, STRING_CONST("\t\t\t\"name\": \"%.*s\""), STRING_FORMAT(node_name));
			if (node->mesh != GLTF_INVALID_INDEX)
				stream_write_format(stream, STRING_CONST(",\n\t\t\t\"mesh\": %u"), node->mesh);
			bool has_matrix = node->transform.has_matrix;
			bool identity_matrix = false;
			if (has_matrix) {
				if ((node->transform.matrix[0][0] == 1) && (node->transform.matrix[1][1] == 1) &&
				    (node->transform.matrix[2][2] == 1) && (node->transform.matrix[3][3] == 1)) {
					if ((node->transform.matrix[0][1] == 0) && (node->transform.matrix[0][2] == 0) &&
					    (node->transform.matrix[0][3] == 0) && (node->transform.matrix[1][0] == 0) &&
					    (node->transform.matrix[1][2] == 0) && (node->transform.matrix[1][3] == 0) &&
					    (node->transform.matrix[2][0] == 0) && (node->transform.matrix[2][1] == 0) &&
					    (node->transform.matrix[2][3] == 0) && (node->transform.matrix[3][0] == 0) &&
					    (node->transform.matrix[3][1] == 0) && (node->transform.matrix[3][2] == 0))
						identity_matrix = true;
				}
			}
			if (has_matrix && !identity_matrix) {
				stream_write(stream, STRING_CONST(",\n\t\t\t\"matrix\": [\n"));
				stream_write_format(stream, STRING_CONST("\t\t\t\t%g, %g, %g, %g,\n"),
				                    (double)node->transform.matrix[0][0], (double)node->transform.matrix[0][1],
				                    (double)node->transform.matrix[0][2], (double)node->transform.matrix[0][3]);
				stream_write_format(stream, STRING_CONST("\t\t\t\t%g, %g, %g, %g,\n"),
				                    (double)node->transform.matrix[1][0], (double)node->transform.matrix[1][1],
				                    (double)node->transform.matrix[1][2], (double)node->transform.matrix[1][3]);
				stream_write_format(stream, STRING_CONST("\t\t\t\t%g, %g, %g, %g,\n"),
				                    (double)node->transform.matrix[2][0], (double)node->transform.matrix[2][1],
				                    (double)node->transform.matrix[2][2], (double)node->transform.matrix[2][3]);
				stream_write_format(stream, STRING_CONST("\t\t\t\t%g, %g, %g, %g\n"),
				                    (double)node->transform.matrix[3][0], (double)node->transform.matrix[3][1],
				                    (double)node->transform.matrix[3][2], (double)node->transform.matrix[3][3]);
				stream_write(stream, STRING_CONST("\t\t\t]"));
			}
			stream_write(stream, STRING_CONST("\n\t\t}"));
			if (inode < (nodes_count - 1))
				stream_write(stream, STRING_CONST(","));
			stream_write(stream, STRING_CONST("\n"));
		}
		stream_write(stream, STRING_CONST("\t]"));
	}

	if (array_count(gltf->scenes)) {
		stream_write(stream, STRING_CONST(",\n\t\"scenes\": [\n"));
		for (uint iscene = 0, scenes_count = array_count(gltf->scenes); iscene < scenes_count; ++iscene) {
			gltf_scene_t* scene = gltf->scenes + iscene;
			stream_write(stream, STRING_CONST("\t\t{\n"));
			uint token_count = 0;
			if (scene->name.length) {
				stream_write_format(stream, STRING_CONST("\t\t\t\"name\": \"%.*s\""), STRING_FORMAT(scene->name));
				++token_count;
			}
			if (array_count(scene->nodes)) {
				if (token_count)
					stream_write_format(stream, STRING_CONST(",\n"));
				stream_write(stream, STRING_CONST("\t\t\t\"nodes\": ["));
				for (uint inode = 0, nodes_count = array_count(scene->nodes); inode < nodes_count; ++inode) {
					if (inode)
						stream_write(stream, STRING_CONST(","));
					if (!(inode % 8))
						stream_write(stream, STRING_CONST("\n\t\t\t\t"));
					else
						stream_write(stream, STRING_CONST(" "));
					stream_write_uint32(stream, scene->nodes[inode]);
				}
				stream_write(stream, STRING_CONST("\n\t\t\t]"));
				++token_count;
			}
			if (token_count)
				stream_write(stream, STRING_CONST("\n"));
			stream_write(stream, STRING_CONST("\t\t}"));
			if (iscene < (scenes_count - 1))
				stream_write(stream, STRING_CONST(","));
			stream_write(stream, STRING_CONST("\n"));
		}
		stream_write(stream, STRING_CONST("\t]"));
	}

	if (gltf->scene != GLTF_INVALID_INDEX) {
		stream_write_format(stream, STRING_CONST(",\n\t\"scene\": %u\n"), gltf->scene);
	}

	stream_write(stream, STRING_CONST("\n}\n"));

	if ((gltf->file_type == GLTF_FILE_GLB) || (gltf->file_type == GLTF_FILE_GLB_EMBED)) {
		stream_set_binary(stream, true);

		size_t current_offset = stream_tell(stream);
		size_t json_chunk_length = current_offset - json_start_offset;

		if (json_chunk_length % 4) {
			uint padding = 4 - (json_chunk_length % 4);
			stream_write(stream, "    ", padding);
			json_chunk_length += padding;
		}

		if ((gltf->file_type == GLTF_FILE_GLB_EMBED) && gltf->output_buffer && gltf->output_buffer->count) {
			uint chunk_size = (uint)gltf->output_buffer->count;
			uint padding = 0;
			if (chunk_size % 4)
				padding = 4 - (chunk_size % 4);
			chunk_size += padding;

			// Write binary chunk header (size, type)
			stream_write_uint32(stream, chunk_size);
			stream_write_uint32(stream, 0x004E4942);

			// Write binary chunk payload
			stream_write(stream, gltf->output_buffer->storage, gltf->output_buffer->count);

			if (padding)
				stream_write(stream, "\0\0\0\0", padding);
		}

		size_t file_size = stream_tell(stream);

		// Now fill in sizes
		stream_seek(stream, (ssize_t)json_size_offset, STREAM_SEEK_BEGIN);
		stream_write_uint32(stream, (uint)json_chunk_length);

		stream_seek(stream, (ssize_t)file_size_offset, STREAM_SEEK_BEGIN);
		stream_write_uint32(stream, (uint)file_size);
	}

	return true;
}
