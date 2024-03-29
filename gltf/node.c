/* node.c  -  glTF library  -  Public Domain  -  2018 Mattias Jansson
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

#include "node.h"
#include "hashstrings.h"

#include <foundation/memory.h>
#include <foundation/json.h>
#include <foundation/array.h>
#include <foundation/log.h>
#include <foundation/hashstrings.h>

void
gltf_nodes_finalize(gltf_t* gltf) {
	if (gltf->nodes) {
		for (uint inode = 0, nodes_count = array_count(gltf->nodes); inode < nodes_count; ++inode) {
			gltf_node_t* node = gltf->nodes + inode;
			if (node->children_ext)
				memory_deallocate(node->children_ext);
		}
		array_deallocate(gltf->nodes);
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
	node->children_count = 0;
	node->children_ext = nullptr;

	gltf_transform_initialize(&node->transform);
}

static bool
gltf_nodes_parse_node(gltf_t* gltf, const char* data, json_token_t* tokens, size_t itoken, gltf_node_t* node) {
	if (tokens[itoken].type != JSON_OBJECT)
		return false;

	gltf_node_initialize(node);

	itoken = tokens[itoken].child;
	while (itoken) {
		string_const_t identifier = json_token_identifier(gltf->buffer, tokens + itoken);
		hash_t identifier_hash = string_hash(STRING_ARGS(identifier));
		if ((identifier_hash == HASH_NAME) && (tokens[itoken].type == JSON_STRING))
			node->name = json_token_value(data, tokens + itoken);
		else if (identifier_hash == HASH_CHILDREN) {
			uint* children = node->children_base;
			node->children_count = tokens[itoken].value_length;
			if (node->children_count > GLTF_NODE_BASE_CHILDREN) {
				node->children_ext =
				    memory_allocate(HASH_GLTF, sizeof(uint) * node->children_count, 0, MEMORY_PERSISTENT);
				children = node->children_ext;
			}
			if (!gltf_token_to_integer_array(gltf, data, tokens, itoken, children, node->children_count))
				return false;
		} else if ((identifier_hash == HASH_MESH) && !gltf_token_to_integer(gltf, data, tokens, itoken, &node->mesh))
			return false;

		else if ((identifier_hash == HASH_SCALE) &&
		         !gltf_token_to_real_array(gltf, data, tokens, itoken, (real*)node->transform.scale, 3))
			return false;
		else if ((identifier_hash == HASH_ROTATION) &&
		         !gltf_token_to_real_array(gltf, data, tokens, itoken, (real*)node->transform.rotation, 4))
			return false;
		else if ((identifier_hash == HASH_TRANSLATION) &&
		         !gltf_token_to_real_array(gltf, data, tokens, itoken, (real*)node->transform.translation, 3))
			return false;
		else if (identifier_hash == HASH_MATRIX) {
			node->transform.has_matrix = true;
			if (!gltf_token_to_real_array(gltf, data, tokens, itoken, (real*)node->transform.matrix, 16))
				return false;
		} else if ((identifier_hash == HASH_EXTENSIONS) && (tokens[itoken].type == JSON_STRING))
			node->extensions = json_token_value(data, tokens + itoken);
		else if ((identifier_hash == HASH_EXTRAS) && (tokens[itoken].type == JSON_STRING))
			node->extras = json_token_value(data, tokens + itoken);

		itoken = tokens[itoken].sibling;
	}

	return true;
}

bool
gltf_nodes_parse(gltf_t* gltf, const char* data, json_token_t* tokens, size_t itoken) {
	if (tokens[itoken].type != JSON_ARRAY) {
		log_error(HASH_GLTF, ERROR_INVALID_VALUE, STRING_CONST("Main nodes attribute has invalid type"));
		return false;
	}

	size_t nodes_count = tokens[itoken].value_length;
	if (nodes_count > GLTF_MAX_INDEX)
		return false;

	array_resize(gltf->nodes, nodes_count);

	if (!nodes_count)
		return true;

	uint icounter = 0;
	size_t inode = tokens[itoken].child;
	while (inode) {
		if (!gltf_nodes_parse_node(gltf, data, tokens, inode, gltf->nodes + icounter))
			return false;
		inode = tokens[inode].sibling;
		++icounter;
	}

	return true;
}

uint
gltf_node_add(gltf_t* gltf, const char* name, size_t name_length, uint mesh_index, const matrix_t* transform) {
	gltf_node_t gltf_node = {0};

	string_t node_name = string_clone(name, name_length);
	array_push(gltf->string_array, node_name);

	gltf_node.name = string_const(STRING_ARGS(node_name));
	gltf_node.mesh = mesh_index;
	if (transform) {
		gltf_node.transform.has_matrix = 1;
		for (uint irow = 0; irow < 4; ++irow) {
			for (uint icol = 0; icol < 4; ++icol) {
				gltf_node.transform.matrix[irow][icol] = transform->frow[irow][icol];
			}
		}
	}

	array_push_memcpy(gltf->nodes, &gltf_node);

	return (array_count(gltf->nodes) - 1);
}
