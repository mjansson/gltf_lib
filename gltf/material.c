/* material.c  -  glTF library  -  Public Domain  -  2018 Mattias Jansson / Rampant Pixels
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
		material->metallic_roughness.base_color_factor[ielem] = 1.0;
	material->metallic_roughness.metallic_factor = 1.0;
	material->metallic_roughness.roughness_factor = 1.0;
	material->normal_scale = 1.0;
	material->occlusion_strength = 1.0;
	material->alpha_mode = GLTF_ALPHA_MODE_OPAQUE;
	material->alpha_cutoff = 0.5;
}

static int
gltf_material_parse_textureinfo(gltf_t* gltf, const char* buffer, json_token_t* tokens,
                                size_t itoken, gltf_texture_info_t* texture) {
	if (tokens[itoken].type != JSON_OBJECT) {
		log_error(HASH_GLTF, ERROR_INVALID_VALUE,
		          STRING_CONST("Texture info attribute has invalid type"));
		return -1;
	}

	int result = 0;
	itoken = tokens[itoken].child;
	while (itoken) {
		string_const_t identifier = json_token_identifier(gltf->buffer, tokens + itoken);
		hash_t identifier_hash = string_hash(STRING_ARGS(identifier));
		if (identifier_hash == HASH_INDEX)
			result = gltf_token_to_integer(gltf, buffer, tokens, itoken, &texture->index);
		else if (identifier_hash == HASH_TEXCOORD)
			result = gltf_token_to_integer(gltf, buffer, tokens, itoken, &texture->texcoord);
		else if ((identifier_hash == HASH_EXTENSIONS) && (tokens[itoken].type == JSON_STRING))
			texture->extensions = json_token_value(buffer, tokens + itoken);
		else if ((identifier_hash == HASH_EXTRAS) && (tokens[itoken].type == JSON_STRING))
			texture->extras = json_token_value(buffer, tokens + itoken);

		if (result)
			break;
		itoken = tokens[itoken].sibling;
	}

	return result;
}

static int
gltf_material_parse_occlusiontexture(gltf_t* gltf, const char* buffer, json_token_t* tokens,
                                     size_t itoken, gltf_material_t* material) {
	if (gltf_material_parse_textureinfo(gltf, buffer, tokens, itoken, &material->occlusion_texture))
		return -1;

	int result = 0;
	itoken = tokens[itoken].child;
	while (itoken) {
		string_const_t identifier = json_token_identifier(gltf->buffer, tokens + itoken);
		hash_t identifier_hash = string_hash(STRING_ARGS(identifier));
		if (identifier_hash == HASH_STRENGTH)
			result =
			    gltf_token_to_double(gltf, buffer, tokens, itoken, &material->occlusion_strength);

		if (result)
			break;
		itoken = tokens[itoken].sibling;
	}

	return result;
}

static int
gltf_material_parse_normaltexture(gltf_t* gltf, const char* buffer, json_token_t* tokens,
                                  size_t itoken, gltf_material_t* material) {
	if (gltf_material_parse_textureinfo(gltf, buffer, tokens, itoken, &material->occlusion_texture))
		return -1;

	int result = 0;
	itoken = tokens[itoken].child;
	while (itoken) {
		string_const_t identifier = json_token_identifier(gltf->buffer, tokens + itoken);
		hash_t identifier_hash = string_hash(STRING_ARGS(identifier));
		if (identifier_hash == HASH_SCALE)
			result = gltf_token_to_double(gltf, buffer, tokens, itoken, &material->normal_scale);

		if (result)
			break;
		itoken = tokens[itoken].sibling;
	}

	return result;
}

static int
gltf_material_parse_pbrmetallicroughness(gltf_t* gltf, const char* buffer, json_token_t* tokens,
                                         size_t itoken,
                                         gltf_pbr_metallic_roughness_t* metallic_roughness) {
	if (tokens[itoken].type != JSON_OBJECT)
		return -1;

	int result = 0;
	itoken = tokens[itoken].child;
	while (itoken) {
		string_const_t identifier = json_token_identifier(gltf->buffer, tokens + itoken);
		hash_t identifier_hash = string_hash(STRING_ARGS(identifier));
		if ((identifier_hash == HASH_EXTENSIONS) && (tokens[itoken].type == JSON_STRING))
			metallic_roughness->extensions = json_token_value(buffer, tokens + itoken);
		else if ((identifier_hash == HASH_EXTRAS) && (tokens[itoken].type == JSON_STRING))
			metallic_roughness->extras = json_token_value(buffer, tokens + itoken);
		else if (identifier_hash == HASH_BASECOLORTEXTURE)
			result = gltf_material_parse_textureinfo(gltf, buffer, tokens, itoken,
			                                         &metallic_roughness->base_color_texture);
		else if (identifier_hash == HASH_METALLICROUGHNESSTEXTURE)
			result = gltf_material_parse_textureinfo(
			    gltf, buffer, tokens, itoken, &metallic_roughness->metallic_roughness_texture);
		else if (identifier_hash == HASH_BASECOLORFACTOR)
			result = gltf_token_to_double_array(gltf, buffer, tokens, itoken,
			                                    (double*)metallic_roughness->base_color_factor, 4);
		else if (identifier_hash == HASH_METALLICFACTOR)
			result = gltf_token_to_double(gltf, buffer, tokens, itoken,
			                              (double*)&metallic_roughness->metallic_factor);
		else if (identifier_hash == HASH_ROUGHNESSFACTOR)
			result = gltf_token_to_double(gltf, buffer, tokens, itoken,
			                              (double*)&metallic_roughness->roughness_factor);

		if (result)
			break;
		itoken = tokens[itoken].sibling;
	}

	return result;
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
		else if (identifier_hash == HASH_EMISSIVETEXTURE)
			result = gltf_material_parse_textureinfo(gltf, buffer, tokens, itoken,
			                                         &material->emissive_texture);
		else if (identifier_hash == HASH_EMISSIVEFACTOR)
			result = gltf_token_to_double_array(gltf, buffer, tokens, itoken,
			                                    (double*)material->emissive_factor, 3);
		else if (identifier_hash == HASH_NORMALTEXTURE)
			result = gltf_material_parse_normaltexture(gltf, buffer, tokens, itoken, material);
		else if (identifier_hash == HASH_OCCLUSIONTEXTURE)
			result = gltf_material_parse_occlusiontexture(gltf, buffer, tokens, itoken, material);
		else if (identifier_hash == HASH_PBRMETALLICROUGHNESS)
			result = gltf_material_parse_pbrmetallicroughness(gltf, buffer, tokens, itoken,
			                                                  &material->metallic_roughness);

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
	gltf->materials =
	    memory_allocate(HASH_GLTF, storage_size, 0, MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);

	int result = 0;
	unsigned int icounter = 0;
	size_t imat = tokens[itoken].child;
	while (imat) {
		result =
		    gltf_materials_parse_material(gltf, buffer, tokens, imat, gltf->materials + icounter);
		if (result)
			break;
		imat = tokens[imat].sibling;
		++icounter;
	}

	return result;
}
