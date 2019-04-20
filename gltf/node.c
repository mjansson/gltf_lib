/* node.c  -  glTF library  -  Public Domain  -  2018 Mattias Jansson / Rampant Pixels
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

#include "node.h"
#include "hashstrings.h"

#include <foundation/memory.h>
#include <foundation/json.h>
#include <foundation/log.h>
#include <foundation/hashstrings.h>

void
gltf_nodes_finalize(gltf_t* gltf) {
	if (gltf->nodes) {
		for (unsigned int inode = 0; inode < gltf->num_nodes; ++inode) {
			gltf_node_t* node = gltf->nodes + inode;
			if (node->children != node->base_children)
				memory_deallocate(node->children);
		}
		memory_deallocate(gltf->nodes);
	}
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
	transform->has_matrix = false;
}

static void
gltf_node_initialize(gltf_node_t* node) {
	node->mesh = GLTF_INVALID_INDEX;
	node->num_children = 0;
	node->children = node->base_children;

	gltf_transform_initialize(&node->transform);
}

static int
gltf_nodes_parse_node(gltf_t* gltf, const char* data, json_token_t* tokens, size_t itoken,
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
			node->name = json_token_value(data, tokens + itoken);
		else if (identifier_hash == HASH_CHILDREN) {
			node->num_children = tokens[itoken].value_length;
			if (node->num_children > GLTF_NODE_BASE_CHILDREN)
				node->children = memory_allocate(HASH_GLTF, sizeof(unsigned int) * node->num_children, 0,
				                                 MEMORY_PERSISTENT);
			result =
			    gltf_token_to_integer_array(gltf, data, tokens, itoken, node->children, node->num_children);
		} else if (identifier_hash == HASH_MESH)
			result = gltf_token_to_integer(gltf, data, tokens, itoken, &node->mesh);
		else if (identifier_hash == HASH_SCALE)
			result =
			    gltf_token_to_double_array(gltf, data, tokens, itoken, (double*)node->transform.scale, 3);
		else if (identifier_hash == HASH_ROTATION)
			result =
			    gltf_token_to_double_array(gltf, data, tokens, itoken, (double*)node->transform.rotation, 4);
		else if (identifier_hash == HASH_TRANSLATION)
			result = gltf_token_to_double_array(gltf, data, tokens, itoken,
			                                    (double*)node->transform.translation, 3);
		else if (identifier_hash == HASH_MATRIX) {
			node->transform.has_matrix = true;
			result =
			    gltf_token_to_double_array(gltf, data, tokens, itoken, (double*)node->transform.matrix, 16);
		} else if ((identifier_hash == HASH_EXTENSIONS) && (tokens[itoken].type == JSON_STRING))
			node->extensions = json_token_value(data, tokens + itoken);
		else if ((identifier_hash == HASH_EXTRAS) && (tokens[itoken].type == JSON_STRING))
			node->extras = json_token_value(data, tokens + itoken);

		if (result)
			break;
		itoken = tokens[itoken].sibling;
	}

	return result;
}

int
gltf_nodes_parse(gltf_t* gltf, const char* data, json_token_t* tokens, size_t itoken) {
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
	gltf->nodes = memory_allocate(HASH_GLTF, storage_size, 0, MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);

	int result = 0;
	unsigned int icounter = 0;
	size_t iscene = tokens[itoken].child;
	while (iscene) {
		result = gltf_nodes_parse_node(gltf, data, tokens, iscene, gltf->nodes + icounter);
		if (result)
			break;
		iscene = tokens[iscene].sibling;
		++icounter;
	}

	return result;
}
