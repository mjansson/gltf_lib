/* material.c  -  glTF library  -  Public Domain  -  2018 Mattias Jansson / Rampant Pixels
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
#include "material.h"
#include "hashstrings.h"

#include <foundation/memory.h>
#include <foundation/json.h>
#include <foundation/log.h>
#include <foundation/hashstrings.h>

void
gltf_materials_finalize(gltf_t* gltf) {
	if (gltf->materials) {
		memory_deallocate(gltf->materials);
	}
}

static void
gltf_material_initialize(gltf_material_t* material) {
	for (unsigned int ielem = 0; ielem < 4; ++ielem)
		material->base_color_factor[ielem] = 1.0;
	material->metallic_factor = 1.0;
	material->roughness_factor = 1.0;
	material->normal_scale = 1.0;
	material->occlusion_strength = 1.0;
	material->alpha_mode = GLTF_ALPHA_MODE_OPAQUE;
	material->alpha_cutoff = 0.5;
}

static int
gltf_materials_parse_material(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken,
                              gltf_material_t* material) {
	if (tokens[itoken].type != JSON_OBJECT)
		return -1;

	gltf_material_initialize(material);

	int result = 0;
	itoken = tokens[itoken].child;
	while (itoken) {
		string_const_t identifier = json_token_identifier(gltf->buffer, tokens + itoken);
		hash_t identifier_hash = string_hash(STRING_ARGS(identifier));
		if ((identifier_hash == HASH_NAME) && (tokens[itoken].type == JSON_STRING))
			material->name = json_token_value(buffer, tokens + itoken);
		else if ((identifier_hash == HASH_EXTENSIONS) && (tokens[itoken].type == JSON_STRING))
			material->extensions = json_token_value(buffer, tokens + itoken);
		else if ((identifier_hash == HASH_EXTRAS) && (tokens[itoken].type == JSON_STRING))
			material->extras = json_token_value(buffer, tokens + itoken);

		if (result)
			break;
		itoken = tokens[itoken].sibling;
	}

	return result;
}

int
gltf_materials_parse(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken) {
	if (tokens[itoken].type != JSON_ARRAY) {
		log_error(HASH_GLTF, ERROR_INVALID_VALUE,
		          STRING_CONST("Main materials attribute has invalid type"));
		return -1;
	}

	size_t num_materials = tokens[itoken].value_length;
	if (num_materials > GLTF_MAX_INDEX)
		return -1;
	if (!num_materials)
		return 0;

	size_t storage_size = sizeof(gltf_material_t) * num_materials;
	gltf_materials_finalize(gltf);
	gltf->num_materials = (unsigned int)num_materials;
	gltf->materials = memory_allocate(HASH_GLTF, storage_size, 0,
	                                  MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);

	int result = 0;
	unsigned int icounter = 0;
	size_t iscene = tokens[itoken].child;
	while (iscene) {
		result = gltf_materials_parse_material(gltf, buffer, tokens, iscene, gltf->materials + icounter);
		if (result)
			break;
		iscene = tokens[iscene].sibling;
		++icounter;
	}

	return result;
}
