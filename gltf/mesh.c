/* mesh.c  -  glTF library  -  Public Domain  -  2018 Mattias Jansson / Rampant Pixels
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

#include "gltf.h"
#include "mesh.h"
#include "hashstrings.h"

#include <foundation/memory.h>
#include <foundation/json.h>
#include <foundation/log.h>
#include <foundation/hashstrings.h>

static void
gltf_primitive_finalize(gltf_primitive_t* primitive) {
	if (primitive->attributes_custom) {
		memory_deallocate(primitive->attributes_custom);
	}
}

static void
gltf_mesh_finalize(gltf_mesh_t* mesh) {
	if (mesh->primitives) {
		for (unsigned int iprim = 0; iprim < mesh->num_primitives; ++iprim)
			gltf_primitive_finalize(mesh->primitives + iprim);
		memory_deallocate(mesh->primitives);
	}
}

void
gltf_meshes_finalize(gltf_t* gltf) {
	if (gltf->meshes) {
		for (unsigned int imesh = 0; imesh < gltf->num_meshes; ++imesh)
			gltf_mesh_finalize(gltf->meshes + imesh);
		memory_deallocate(gltf->meshes);
	}
}

static bool
gltf_primitive_parse_attributes(gltf_t* gltf, const char* buffer, json_token_t* tokens,
                                size_t itoken, gltf_primitive_t* primitive) {
	if (tokens[itoken].type != JSON_OBJECT) {
		log_error(HASH_GLTF, ERROR_INVALID_VALUE,
		          STRING_CONST("Primitive attributes has invalid type"));
		return false;
	}

	unsigned int customCount = 0;
	size_t iparent = itoken;
	itoken = tokens[iparent].child;
	while (itoken) {
		string_const_t identifier = json_token_identifier(gltf->buffer, tokens + itoken);
		hash_t identifier_hash = string_hash(STRING_ARGS(identifier));
		if ((identifier_hash == HASH_POSITION) &&
		    !gltf_token_to_integer(gltf, buffer, tokens, itoken,
		                           &primitive->attributes[GLTF_POSITION]))
			return false;
		else if ((identifier_hash == HASH_NORMAL) &&
		         !gltf_token_to_integer(gltf, buffer, tokens, itoken,
		                                &primitive->attributes[GLTF_NORMAL]))
			return false;
		else if ((identifier_hash == HASH_TANGENT) &&
		         !gltf_token_to_integer(gltf, buffer, tokens, itoken,
		                                &primitive->attributes[GLTF_TANGENT]))
			return false;
		else if ((identifier_hash == HASH_TEXCOORD_0) &&
		         !gltf_token_to_integer(gltf, buffer, tokens, itoken,
		                                &primitive->attributes[GLTF_TEXCOORD_0]))
			return false;
		else if ((identifier_hash == HASH_TEXCOORD_1) &&
		         !gltf_token_to_integer(gltf, buffer, tokens, itoken,
		                                &primitive->attributes[GLTF_TEXCOORD_1]))
			return false;
		else if ((identifier_hash == HASH_COLOR_0) &&
		         !gltf_token_to_integer(gltf, buffer, tokens, itoken,
		                                &primitive->attributes[GLTF_COLOR_0]))
			return false;
		else if ((identifier_hash == HASH_JOINTS_0) &&
		         !gltf_token_to_integer(gltf, buffer, tokens, itoken,
		                                &primitive->attributes[GLTF_JOINTS_0]))
			return false;
		else if ((identifier_hash == HASH_WEIGHTS_0) &&
		         !gltf_token_to_integer(gltf, buffer, tokens, itoken,
		                                &primitive->attributes[GLTF_WEIGHTS_0]))
			return false;
		else
			++customCount;

		itoken = tokens[itoken].sibling;
	}

	primitive->num_attributes_custom = customCount;
	primitive->attributes_custom =
	    memory_allocate(HASH_GLTF, sizeof(gltf_attribute_t) * customCount, 0,
	                    MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);

	for (unsigned int iattrib = 0; iattrib < customCount; ++iattrib)
		primitive->attributes_custom[iattrib].accessor = GLTF_INVALID_INDEX;

	customCount = 0;
	itoken = tokens[iparent].child;
	while (itoken && customCount) {
		string_const_t identifier = json_token_identifier(gltf->buffer, tokens + itoken);
		hash_t identifier_hash = string_hash(STRING_ARGS(identifier));
		if ((identifier_hash == HASH_POSITION) || (identifier_hash == HASH_NORMAL) ||
		    (identifier_hash == HASH_TANGENT) || (identifier_hash == HASH_TEXCOORD_0) ||
		    (identifier_hash == HASH_TEXCOORD_1) || (identifier_hash == HASH_COLOR_0) ||
		    (identifier_hash == HASH_JOINTS_0) || (identifier_hash == HASH_WEIGHTS_0))
			continue;

		gltf_attribute_t* attrib = primitive->attributes_custom + customCount;
		attrib->semantic = identifier.str;
		attrib->semantic_length = identifier.length;
		if (!gltf_token_to_integer(gltf, buffer, tokens, itoken, &attrib->accessor))
			return false;
		--customCount;

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
		         !gltf_token_to_integer(gltf, buffer, tokens, itoken,
		                                (unsigned int*)&primitive->mode))
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
gltf_mesh_parse_primitives(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken,
                           gltf_mesh_t* mesh) {
	if (tokens[itoken].type != JSON_ARRAY) {
		log_error(HASH_GLTF, ERROR_INVALID_VALUE,
		          STRING_CONST("Mesh primitives attribute has invalid type"));
		return false;
	}

	size_t num_primitives = tokens[itoken].value_length;
	if (num_primitives > GLTF_MAX_INDEX)
		return false;
	if (!num_primitives)
		return true;

	size_t size = sizeof(gltf_primitive_t) * num_primitives;
	mesh->num_primitives = (unsigned int)num_primitives;
	mesh->primitives =
	    memory_allocate(HASH_GLTF, size, 0, MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);

	unsigned int iprim = 0;
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
gltf_meshes_parse_mesh(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken,
                       gltf_mesh_t* mesh) {
	if (tokens[itoken].type != JSON_OBJECT) {
		log_error(HASH_GLTF, ERROR_INVALID_VALUE, STRING_CONST("Mesh attribute has invalid type"));
		return false;
	}

	itoken = tokens[itoken].child;
	while (itoken) {
		string_const_t identifier = json_token_identifier(gltf->buffer, tokens + itoken);
		hash_t identifier_hash = string_hash(STRING_ARGS(identifier));
		if ((identifier_hash == HASH_PRIMITIVES) &&
		    !gltf_mesh_parse_primitives(gltf, buffer, tokens, itoken, mesh))
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
		log_error(HASH_GLTF, ERROR_INVALID_VALUE,
		          STRING_CONST("Main meshes attribute has invalid type"));
		return false;
	}

	size_t num_meshes = tokens[itoken].value_length;
	if (num_meshes > GLTF_MAX_INDEX)
		return false;
	if (!num_meshes)
		return true;

	size_t storage_size = sizeof(gltf_mesh_t) * num_meshes;
	gltf_meshes_finalize(gltf);
	gltf->num_meshes = (unsigned int)num_meshes;
	gltf->meshes =
	    memory_allocate(HASH_GLTF, storage_size, 0, MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);

	unsigned int icounter = 0;
	size_t imesh = tokens[itoken].child;
	while (imesh) {
		if (!gltf_meshes_parse_mesh(gltf, buffer, tokens, imesh, gltf->meshes + icounter))
			return false;
		imesh = tokens[imesh].sibling;
		++icounter;
	}

	return true;
}
