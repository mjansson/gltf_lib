/* mesh.c  -  glTF library  -  Public Domain  -  2018 Mattias Jansson
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
#include "mesh.h"
#include "hashstrings.h"

#include <foundation/memory.h>
#include <foundation/json.h>
#include <foundation/array.h>
#include <foundation/bucketarray.h>
#include <foundation/virtualarray.h>
#include <foundation/array.h>
#include <foundation/log.h>
#include <foundation/hashstrings.h>

#include <mesh/mesh.h>
#include <vector/vector.h>

static void
gltf_primitive_finalize(gltf_primitive_t* primitive) {
	if (primitive->attributes_custom) {
		array_deallocate(primitive->attributes_custom);
	}
}

static void
gltf_mesh_finalize(gltf_mesh_t* mesh) {
	if (mesh->primitives) {
		for (uint iprim = 0, primitives_count = array_count(mesh->primitives); iprim < primitives_count; ++iprim)
			gltf_primitive_finalize(mesh->primitives + iprim);
		array_deallocate(mesh->primitives);
	}
}

void
gltf_meshes_finalize(gltf_t* gltf) {
	if (gltf->meshes) {
		for (uint imesh = 0, meshes_count = array_count(gltf->meshes); imesh < meshes_count; ++imesh)
			gltf_mesh_finalize(gltf->meshes + imesh);
		array_deallocate(gltf->meshes);
	}
}

static bool
gltf_primitive_parse_attributes(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken,
                                gltf_primitive_t* primitive) {
	if (tokens[itoken].type != JSON_OBJECT) {
		log_error(HASH_GLTF, ERROR_INVALID_VALUE, STRING_CONST("Primitive attributes has invalid type"));
		return false;
	}

	uint custom_count = 0;
	size_t iparent = itoken;
	itoken = tokens[iparent].child;
	while (itoken) {
		string_const_t identifier = json_token_identifier(gltf->buffer, tokens + itoken);
		hash_t identifier_hash = string_hash(STRING_ARGS(identifier));
		if ((identifier_hash == HASH_POSITION) &&
		    !gltf_token_to_integer(gltf, buffer, tokens, itoken, &primitive->attributes[GLTF_POSITION]))
			return false;
		else if ((identifier_hash == HASH_NORMAL) &&
		         !gltf_token_to_integer(gltf, buffer, tokens, itoken, &primitive->attributes[GLTF_NORMAL]))
			return false;
		else if ((identifier_hash == HASH_TANGENT) &&
		         !gltf_token_to_integer(gltf, buffer, tokens, itoken, &primitive->attributes[GLTF_TANGENT]))
			return false;
		else if ((identifier_hash == HASH_TEXCOORD_0) &&
		         !gltf_token_to_integer(gltf, buffer, tokens, itoken, &primitive->attributes[GLTF_TEXCOORD_0]))
			return false;
		else if ((identifier_hash == HASH_TEXCOORD_1) &&
		         !gltf_token_to_integer(gltf, buffer, tokens, itoken, &primitive->attributes[GLTF_TEXCOORD_1]))
			return false;
		else if ((identifier_hash == HASH_COLOR_0) &&
		         !gltf_token_to_integer(gltf, buffer, tokens, itoken, &primitive->attributes[GLTF_COLOR_0]))
			return false;
		else if ((identifier_hash == HASH_JOINTS_0) &&
		         !gltf_token_to_integer(gltf, buffer, tokens, itoken, &primitive->attributes[GLTF_JOINTS_0]))
			return false;
		else if ((identifier_hash == HASH_WEIGHTS_0) &&
		         !gltf_token_to_integer(gltf, buffer, tokens, itoken, &primitive->attributes[GLTF_WEIGHTS_0]))
			return false;
		else
			++custom_count;

		itoken = tokens[itoken].sibling;
	}

	primitive->attributes_custom = 0;
	if (custom_count) {
		array_resize(primitive->attributes_custom, custom_count);
		for (uint iattrib = 0; iattrib < custom_count; ++iattrib)
			primitive->attributes_custom[iattrib].accessor = GLTF_INVALID_INDEX;
	}

	itoken = tokens[iparent].child;
	while (itoken && custom_count) {
		string_const_t identifier = json_token_identifier(gltf->buffer, tokens + itoken);
		hash_t identifier_hash = string_hash(STRING_ARGS(identifier));
		if ((identifier_hash == HASH_POSITION) || (identifier_hash == HASH_NORMAL) ||
		    (identifier_hash == HASH_TANGENT) || (identifier_hash == HASH_TEXCOORD_0) ||
		    (identifier_hash == HASH_TEXCOORD_1) || (identifier_hash == HASH_COLOR_0) ||
		    (identifier_hash == HASH_JOINTS_0) || (identifier_hash == HASH_WEIGHTS_0)) {
			itoken = tokens[itoken].sibling;
			continue;
		}

		gltf_attribute_t* attrib = primitive->attributes_custom + custom_count;
		attrib->semantic = identifier;
		if (!gltf_token_to_integer(gltf, buffer, tokens, itoken, &attrib->accessor))
			return false;
		--custom_count;

		itoken = tokens[itoken].sibling;
	}

	return true;
}

static int
gltf_mesh_parse_primitive(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken,
                          gltf_primitive_t* primitive) {
	if (tokens[itoken].type != JSON_OBJECT) {
		log_error(HASH_GLTF, ERROR_INVALID_VALUE, STRING_CONST("Mesh attribute has invalid type"));
		return false;
	}

	primitive->mode = GLTF_TRIANGLES;

	for (int iattrib = 0; iattrib < GLTF_ATTRIBUTE_COUNT; ++iattrib)
		primitive->attributes[iattrib] = GLTF_INVALID_INDEX;

	itoken = tokens[itoken].child;
	while (itoken) {
		string_const_t identifier = json_token_identifier(gltf->buffer, tokens + itoken);
		hash_t identifier_hash = string_hash(STRING_ARGS(identifier));
		if ((identifier_hash == HASH_ATTRIBUTES) &&
		    !gltf_primitive_parse_attributes(gltf, buffer, tokens, itoken, primitive))
			return false;
		else if ((identifier_hash == HASH_INDICES) &&
		         !gltf_token_to_integer(gltf, buffer, tokens, itoken, &primitive->indices))
			return false;
		else if ((identifier_hash == HASH_MATERIAL) &&
		         !gltf_token_to_integer(gltf, buffer, tokens, itoken, &primitive->material))
			return false;
		else if ((identifier_hash == HASH_MODE) &&
		         !gltf_token_to_integer(gltf, buffer, tokens, itoken, (uint*)&primitive->mode))
			return false;
		else if ((identifier_hash == HASH_EXTENSIONS) && (tokens[itoken].type == JSON_STRING))
			primitive->extensions = json_token_value(buffer, tokens + itoken);
		else if ((identifier_hash == HASH_EXTRAS) && (tokens[itoken].type == JSON_STRING))
			primitive->extras = json_token_value(buffer, tokens + itoken);

		itoken = tokens[itoken].sibling;
	}

	return true;
}

static bool
gltf_mesh_parse_primitives(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken, gltf_mesh_t* mesh) {
	if (tokens[itoken].type != JSON_ARRAY) {
		log_error(HASH_GLTF, ERROR_INVALID_VALUE, STRING_CONST("Mesh primitives attribute has invalid type"));
		return false;
	}

	size_t primitives_count = tokens[itoken].value_length;
	if (primitives_count > GLTF_MAX_INDEX)
		return false;

	array_resize(mesh->primitives, primitives_count);

	if (!primitives_count)
		return true;

	uint iprim = 0;
	itoken = tokens[itoken].child;
	while (itoken) {
		if (!gltf_mesh_parse_primitive(gltf, buffer, tokens, itoken, mesh->primitives + iprim))
			return false;
		itoken = tokens[itoken].sibling;
		++iprim;
	}

	return true;
}

static bool
gltf_meshes_parse_mesh(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken, gltf_mesh_t* mesh) {
	if (tokens[itoken].type != JSON_OBJECT) {
		log_error(HASH_GLTF, ERROR_INVALID_VALUE, STRING_CONST("Mesh attribute has invalid type"));
		return false;
	}

	itoken = tokens[itoken].child;
	while (itoken) {
		string_const_t identifier = json_token_identifier(gltf->buffer, tokens + itoken);
		hash_t identifier_hash = string_hash(STRING_ARGS(identifier));
		if ((identifier_hash == HASH_PRIMITIVES) && !gltf_mesh_parse_primitives(gltf, buffer, tokens, itoken, mesh))
			return false;
		else if (identifier_hash == HASH_NAME)
			mesh->name = json_token_value(buffer, tokens + itoken);
		else if ((identifier_hash == HASH_EXTENSIONS) && (tokens[itoken].type == JSON_STRING))
			mesh->extensions = json_token_value(buffer, tokens + itoken);
		else if ((identifier_hash == HASH_EXTRAS) && (tokens[itoken].type == JSON_STRING))
			mesh->extras = json_token_value(buffer, tokens + itoken);

		itoken = tokens[itoken].sibling;
	}

	return true;
}

bool
gltf_meshes_parse(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken) {
	if (tokens[itoken].type != JSON_ARRAY) {
		log_error(HASH_GLTF, ERROR_INVALID_VALUE, STRING_CONST("Main meshes attribute has invalid type"));
		return false;
	}

	size_t meshes_count = tokens[itoken].value_length;
	if (meshes_count > GLTF_MAX_INDEX)
		return false;

	array_resize(gltf->meshes, meshes_count);

	if (!meshes_count)
		return true;

	uint icounter = 0;
	size_t imesh = tokens[itoken].child;
	while (imesh) {
		if (!gltf_meshes_parse_mesh(gltf, buffer, tokens, imesh, gltf->meshes + icounter))
			return false;
		imesh = tokens[imesh].sibling;
		++icounter;
	}

	return true;
}

uint
gltf_mesh_add_mesh(gltf_t* gltf, const mesh_t* mesh) {
	if (!mesh || !mesh->triangle.count || !mesh->vertex.count)
		return GLTF_INVALID_INDEX;

	gltf_mesh_t gltf_mesh = {0};

	string_t mesh_name = string_clone(STRING_ARGS(mesh->name));
	array_push(gltf->string_array, mesh_name);

	gltf_mesh.name = string_const(STRING_ARGS(mesh_name));

	// First setup the vertex attribute accessors
	// Make sure we have an output buffer ready
	if (!gltf->output_buffer)
		gltf->output_buffer = virtualarray_allocate(1, 1024 * 1024 * 1024);
	uint current_offset = (uint)gltf->output_buffer->count;
	virtualarray_resize(gltf->output_buffer, current_offset + (sizeof(float) * mesh->vertex.count * 3));

	// Coordinates
	uint coordinate_accessor = GLTF_INVALID_INDEX;
	{
		gltf_accessor_t accessor = {0};
		accessor.type = GLTF_DATA_VEC3;
		accessor.component_type = GLTF_COMPONENT_FLOAT;
		accessor.count = (uint)mesh->vertex.count;
		accessor.byte_offset = 0;
		accessor.buffer_view = array_count(gltf->buffer_views);

		gltf_buffer_view_t buffer_view = {0};
		buffer_view.buffer = 0;
		buffer_view.byte_offset = current_offset;
		buffer_view.byte_length = sizeof(float) * accessor.count * 3;

		array_push(gltf->buffer_views, buffer_view);

		vector_t vmin = vector_uniform(REAL_MAX);
		vector_t vmax = vector_uniform(-REAL_MAX);
		float* vertex_component = pointer_offset(gltf->output_buffer->storage, buffer_view.byte_offset);
		for (uint ivert = 0; ivert < mesh->vertex.count; ++ivert) {
			const mesh_vertex_t* mesh_vertex = bucketarray_get_const(&mesh->vertex, ivert);
			const mesh_coordinate_t* mesh_coordinate =
			    bucketarray_get_const(&mesh->coordinate, mesh_vertex->coordinate);
			*vertex_component++ = vector_x(*mesh_coordinate);
			*vertex_component++ = vector_y(*mesh_coordinate);
			*vertex_component++ = vector_z(*mesh_coordinate);
			vmin = vector_min(vmin, *mesh_coordinate);
			vmax = vector_max(vmax, *mesh_coordinate);
		}

		current_offset += buffer_view.byte_length;

		accessor.min[0] = vector_x(vmin);
		accessor.min[1] = vector_y(vmin);
		accessor.min[2] = vector_z(vmin);
		accessor.min[3] = 1;
		accessor.max[0] = vector_x(vmax);
		accessor.max[1] = vector_y(vmax);
		accessor.max[2] = vector_z(vmax);
		accessor.max[3] = 1;

		coordinate_accessor = array_count(gltf->accessors);
		array_push(gltf->accessors, accessor);
	}

	// Normals
	uint normal_accessor = GLTF_INVALID_INDEX;
	if (mesh->normal.count) {
		gltf_accessor_t accessor = {0};
		accessor.type = GLTF_DATA_VEC3;
		accessor.component_type = GLTF_COMPONENT_FLOAT;
		accessor.count = (uint)mesh->vertex.count;
		accessor.byte_offset = 0;
		accessor.buffer_view = array_count(gltf->buffer_views);

		gltf_buffer_view_t buffer_view = {0};
		buffer_view.buffer = 0;
		buffer_view.byte_offset = current_offset;
		buffer_view.byte_length = sizeof(float) * accessor.count * 3;

		array_push(gltf->buffer_views, buffer_view);

		vector_t vmin = vector_uniform(REAL_MAX);
		vector_t vmax = vector_uniform(-REAL_MAX);
		float* normal_component = pointer_offset(gltf->output_buffer->storage, buffer_view.byte_offset);
		for (uint ivert = 0; ivert < mesh->vertex.count; ++ivert) {
			const mesh_vertex_t* mesh_vertex = bucketarray_get_const(&mesh->vertex, ivert);
			const mesh_normal_t* mesh_normal = bucketarray_get_const(&mesh->normal, mesh_vertex->normal);
			*normal_component++ = vector_x(*mesh_normal);
			*normal_component++ = vector_y(*mesh_normal);
			*normal_component++ = vector_z(*mesh_normal);
			vmin = vector_min(vmin, *mesh_normal);
			vmax = vector_max(vmax, *mesh_normal);
		}

		current_offset += buffer_view.byte_length;

		accessor.min[0] = vector_x(vmin);
		accessor.min[1] = vector_y(vmin);
		accessor.min[2] = vector_z(vmin);
		accessor.min[3] = 1;
		accessor.max[0] = vector_x(vmax);
		accessor.max[1] = vector_y(vmax);
		accessor.max[2] = vector_z(vmax);
		accessor.max[3] = 1;

		normal_accessor = array_count(gltf->accessors);
		array_push(gltf->accessors, accessor);
	}

	// Now create primitives and index accessors
	// Make sure we have an output buffer ready
	virtualarray_resize(gltf->output_buffer, current_offset + (sizeof(uint) * mesh->triangle.count * 3));

	uint triangle_restart = 0;
	while (triangle_restart != GLTF_INVALID_INDEX) {
		// One triangle index buffer per primitive
		uint* index = pointer_offset(gltf->output_buffer->storage, current_offset);

		// One primitive per material
		gltf_primitive_t primitive = {0};
		for (int iattrib = 0; iattrib < GLTF_ATTRIBUTE_COUNT; ++iattrib)
			primitive.attributes[iattrib] = GLTF_INVALID_INDEX;

		// Start at the first encountered remaining triangle
		uint triangle_start = triangle_restart;
		triangle_restart = GLTF_INVALID_INDEX;

		const mesh_triangle_t* triangle = bucketarray_get_const(&mesh->triangle, triangle_start);
		uint current_material = triangle->material;
		uint triangle_count = 0;

		for (uint itri = triangle_start; itri < mesh->triangle.count; ++itri) {
			triangle = bucketarray_get_const(&mesh->triangle, itri);
			if (triangle->material > current_material) {
				// Defer to a new primitive, store start triangle index if not set
				if (triangle_restart == GLTF_INVALID_INDEX)
					triangle_restart = itri;
			} else if (triangle->material < current_material) {
				// Already processed
			} else {
				*index++ = triangle->vertex[0];
				*index++ = triangle->vertex[1];
				*index++ = triangle->vertex[2];
				++triangle_count;
			}
		}

		// All triangles for this primitive collected, setup primitive and create index accessor
		// All primitives share the vertex attribute accessors
		primitive.material = current_material;
		primitive.mode = GLTF_TRIANGLES;
		primitive.attributes[GLTF_POSITION] = coordinate_accessor;
		primitive.attributes[GLTF_NORMAL] = normal_accessor;
		primitive.indices = array_count(gltf->accessors);

		array_push(gltf_mesh.primitives, primitive);

		gltf_accessor_t accessor = {0};
		accessor.type = GLTF_DATA_SCALAR;
		accessor.component_type = GLTF_COMPONENT_UNSIGNED_INT;
		accessor.count = triangle_count * 3;
		accessor.byte_offset = 0;
		accessor.buffer_view = array_count(gltf->buffer_views);

		array_push(gltf->accessors, accessor);

		gltf_buffer_view_t buffer_view = {0};
		buffer_view.buffer = 0;
		buffer_view.byte_offset = current_offset;
		buffer_view.byte_length = sizeof(uint) * accessor.count;

		array_push(gltf->buffer_views, buffer_view);

		current_offset += buffer_view.byte_length;
	}
	FOUNDATION_ASSERT(current_offset == (uint)gltf->output_buffer->count);

	array_push(gltf->meshes, gltf_mesh);

	return (uint)(array_count(gltf->meshes) - 1);
}
