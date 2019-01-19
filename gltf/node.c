/* node.c  -  glTF library  -  Public Domain  -  2018 Mattias Jansson / Rampant Pixels
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

#include "node.h"
#include "hashstrings.h"

#include <foundation/memory.h>
#include <foundation/json.h>
#include <foundation/log.h>
#include <foundation/hashstrings.h>

void
gltf_nodes_finalize(gltf_t* gltf) {
	if (gltf->nodes) {
		memory_deallocate(gltf->nodes);
	}
}

static int
gltf_token_to_double(const char* buffer, json_token_t* tokens, size_t itoken, double* val) {
	if (!itoken || ((tokens[itoken].type != JSON_STRING) && (tokens[itoken].type != JSON_PRIMITIVE)))
		return -1;

	string_const_t strval = json_token_value(buffer, tokens + itoken);
	*val = string_to_float64(STRING_ARGS(strval));

	return 0;
}

static void
gltf_transform_initialize(gltf_transform_t* transform) {
	transform->scale[0] = transform->scale[1] = transform->scale[2] = 1.0;
	transform->rotation[0] = transform->rotation[1] = transform->rotation[2] = 0.0;
	transform->rotation[3] = 1.0;
	transform->translation[0] = transform->translation[0] = transform->translation[0] = 0.0;
	memset(transform->matrix, 0, sizeof(transform->matrix));
	transform->matrix[0][0] = 1.0;
	transform->matrix[1][1] = 1.0;
	transform->matrix[2][2] = 1.0;
	transform->matrix[3][3] = 1.0;
}

static void
gltf_node_initialize(gltf_node_t* node) {
	node->mesh = GLTF_INVALID_INDEX;

	gltf_transform_initialize(&node->transform);
}

static int
gltf_node_parse_mesh(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken,
                     gltf_node_t* node) {
	FOUNDATION_UNUSED(gltf);
	if ((tokens[itoken].type != JSON_STRING) && (tokens[itoken].type != JSON_PRIMITIVE)) {
		log_error(HASH_GLTF, ERROR_INVALID_VALUE, STRING_CONST("Node mesh attribute has invalid type"));
		return -1;
	}

	string_const_t value = json_token_value(buffer, tokens + itoken);
	node->mesh = string_to_uint(STRING_ARGS(value), false);
	return 0;
}

static int
gltf_node_parse_scale(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken,
                      gltf_node_t* node) {
	FOUNDATION_UNUSED(gltf);
	if ((tokens[itoken].type != JSON_ARRAY) || (tokens[itoken].value_length != 3))
		return -1;

	itoken = tokens[itoken].child;
	for (int ielem = 0; ielem < 3; ++ielem) {
		if (gltf_token_to_double(buffer, tokens, itoken, &node->transform.scale[ielem]))
			return -1;
		itoken = tokens[itoken].sibling;
	}

	return 0;
}

static int
gltf_node_parse_rotation(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken,
                         gltf_node_t* node) {
	FOUNDATION_UNUSED(gltf);
	if ((tokens[itoken].type != JSON_ARRAY) || (tokens[itoken].value_length != 4))
		return -1;

	itoken = tokens[itoken].child;
	for (int ielem = 0; ielem < 4; ++ielem) {
		if (gltf_token_to_double(buffer, tokens, itoken, &node->transform.rotation[ielem]))
			return -1;
		itoken = tokens[itoken].sibling;
	}

	return 0;
}

static int
gltf_node_parse_translation(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken,
                            gltf_node_t* node) {
	FOUNDATION_UNUSED(gltf);
	if ((tokens[itoken].type != JSON_ARRAY) || (tokens[itoken].value_length != 3))
		return -1;

	itoken = tokens[itoken].child;
	for (int ielem = 0; ielem < 3; ++ielem) {
		if (gltf_token_to_double(buffer, tokens, itoken, &node->transform.translation[ielem]))
			return -1;
		itoken = tokens[itoken].sibling;
	}

	return 0;
}

static int
gltf_node_parse_matrix(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken,
                       gltf_node_t* node) {
	FOUNDATION_UNUSED(gltf);
	if ((tokens[itoken].type != JSON_ARRAY) || (tokens[itoken].value_length != 16))
		return -1;

	double* matrix = (double*)node->transform.matrix;
	itoken = tokens[itoken].child;
	for (int ielem = 0; ielem < 16; ++ielem) {
		if (gltf_token_to_double(buffer, tokens, itoken, matrix + ielem))
			return -1;
		itoken = tokens[itoken].sibling;
	}

	return 0;
}

static int
gltf_nodes_parse_node(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken,
                      gltf_node_t* node) {
	if (tokens[itoken].type != JSON_OBJECT)
		return -1;

	gltf_node_initialize(node);

	int result = 0;
	itoken = tokens[itoken].child;
	while (itoken) {
		string_const_t identifier = json_token_identifier(gltf->buffer, tokens + itoken);
		hash_t identifier_hash = string_hash(STRING_ARGS(identifier));
		if ((identifier_hash == HASH_NAME) && (tokens[itoken].type == JSON_STRING))
			node->name = json_token_value(buffer, tokens + itoken);
		else if (identifier_hash == HASH_MESH)
			result = gltf_node_parse_mesh(gltf, buffer, tokens, itoken, node);
		else if (identifier_hash == HASH_SCALE)
			result = gltf_node_parse_scale(gltf, buffer, tokens, itoken, node);
		else if (identifier_hash == HASH_ROTATION)
			result = gltf_node_parse_rotation(gltf, buffer, tokens, itoken, node);
		else if (identifier_hash == HASH_TRANSLATION)
			result = gltf_node_parse_translation(gltf, buffer, tokens, itoken, node);
		else if (identifier_hash == HASH_MATRIX)
			result = gltf_node_parse_matrix(gltf, buffer, tokens, itoken, node);
		else if ((identifier_hash == HASH_EXTENSIONS) && (tokens[itoken].type == JSON_STRING))
			node->extensions = json_token_value(buffer, tokens + itoken);
		else if ((identifier_hash == HASH_EXTRAS) && (tokens[itoken].type == JSON_STRING))
			node->extras = json_token_value(buffer, tokens + itoken);

		if (result)
			break;
		itoken = tokens[itoken].sibling;
	}

	return result;
}

int
gltf_nodes_parse(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken) {
	if (tokens[itoken].type != JSON_ARRAY) {
		log_error(HASH_GLTF, ERROR_INVALID_VALUE, STRING_CONST("Main nodes attribute has invalid type"));
		return -1;
	}

	size_t num_nodes = tokens[itoken].value_length;
	if (num_nodes > GLTF_MAX_INDEX)
		return -1;
	if (!num_nodes)
		return 0;

	size_t storage_size = sizeof(gltf_node_t) * num_nodes;
	gltf_nodes_finalize(gltf);
	gltf->num_nodes = (unsigned int)num_nodes;
	gltf->nodes = memory_allocate(HASH_GLTF, storage_size, 0,
	                              MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);

	int result = 0;
	unsigned int icounter = 0;
	size_t iscene = tokens[itoken].child;
	while (iscene) {
		result = gltf_nodes_parse_node(gltf, buffer, tokens, iscene, gltf->nodes + icounter);
		if (result)
			break;
		iscene = tokens[iscene].sibling;
		++icounter;
	}

	return result;
}
