/* material.c  -  glTF library  -  Public Domain  -  2018 Mattias Jansson
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
	for (uint ielem = 0; ielem < 4; ++ielem)
		material->metallic_roughness.base_color_factor[ielem] = 1.0;
	material->metallic_roughness.metallic_factor = 1.0;
	material->metallic_roughness.roughness_factor = 1.0;
	material->normal_scale = 1.0;
	material->occlusion_strength = 1.0;
	material->emissive_factor[0] = material->emissive_factor[1] = material->emissive_factor[2] = 0;
	material->alpha_mode = GLTF_ALPHA_MODE_OPAQUE;
	material->alpha_cutoff = 0.5;
	material->double_sided = false;
	material->extensions = string_empty();
	material->extras = string_empty();

	gltf_texture_info_initialize(&material->metallic_roughness.base_color_texture);
	gltf_texture_info_initialize(&material->metallic_roughness.metallic_roughness_texture);
	gltf_texture_info_initialize(&material->normal_texture);
	gltf_texture_info_initialize(&material->occlusion_texture);
	gltf_texture_info_initialize(&material->emissive_texture);
}

static bool
gltf_material_parse_alphamode(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken,
                              gltf_material_t* material) {
	FOUNDATION_UNUSED(gltf);
	string_const_t mode = json_token_value(buffer, tokens + itoken);
	if (string_equal(STRING_ARGS(mode), STRING_CONST("OPAQUE")))
		material->alpha_mode = GLTF_ALPHA_MODE_OPAQUE;
	else if (string_equal(STRING_ARGS(mode), STRING_CONST("MASK")))
		material->alpha_mode = GLTF_ALPHA_MODE_MASK;
	else if (string_equal(STRING_ARGS(mode), STRING_CONST("BLEND")))
		material->alpha_mode = GLTF_ALPHA_MODE_BLEND;
	return true;
}

static bool
gltf_material_parse_textureinfo(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken,
                                gltf_texture_info_t* texture) {
	if (tokens[itoken].type != JSON_OBJECT) {
		log_error(HASH_GLTF, ERROR_INVALID_VALUE, STRING_CONST("Texture info attribute has invalid type"));
		return false;
	}

	itoken = tokens[itoken].child;
	while (itoken) {
		string_const_t identifier = json_token_identifier(gltf->buffer, tokens + itoken);
		hash_t identifier_hash = string_hash(STRING_ARGS(identifier));
		if ((identifier_hash == HASH_INDEX) && !gltf_token_to_integer(gltf, buffer, tokens, itoken, &texture->index))
			return false;
		else if ((identifier_hash == HASH_TEXCOORD) &&
		         !gltf_token_to_integer(gltf, buffer, tokens, itoken, &texture->texcoord))
			return false;
		else if ((identifier_hash == HASH_EXTENSIONS) && (tokens[itoken].type == JSON_STRING))
			texture->extensions = json_token_value(buffer, tokens + itoken);
		else if ((identifier_hash == HASH_EXTRAS) && (tokens[itoken].type == JSON_STRING))
			texture->extras = json_token_value(buffer, tokens + itoken);

		itoken = tokens[itoken].sibling;
	}

	return true;
}

static bool
gltf_material_parse_occlusiontexture(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken,
                                     gltf_material_t* material) {
	if (!gltf_material_parse_textureinfo(gltf, buffer, tokens, itoken, &material->occlusion_texture))
		return false;

	itoken = tokens[itoken].child;
	while (itoken) {
		string_const_t identifier = json_token_identifier(gltf->buffer, tokens + itoken);
		hash_t identifier_hash = string_hash(STRING_ARGS(identifier));
		if ((identifier_hash == HASH_STRENGTH) &&
		    !gltf_token_to_real(gltf, buffer, tokens, itoken, &material->occlusion_strength))
			return false;

		itoken = tokens[itoken].sibling;
	}

	return true;
}

static bool
gltf_material_parse_normaltexture(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken,
                                  gltf_material_t* material) {
	if (!gltf_material_parse_textureinfo(gltf, buffer, tokens, itoken, &material->normal_texture))
		return false;

	itoken = tokens[itoken].child;
	while (itoken) {
		string_const_t identifier = json_token_identifier(gltf->buffer, tokens + itoken);
		hash_t identifier_hash = string_hash(STRING_ARGS(identifier));
		if ((identifier_hash == HASH_SCALE) &&
		    !gltf_token_to_real(gltf, buffer, tokens, itoken, &material->normal_scale))
			return false;

		itoken = tokens[itoken].sibling;
	}

	return true;
}

static bool
gltf_material_parse_pbrmetallicroughness(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken,
                                         gltf_pbr_metallic_roughness_t* metallic_roughness) {
	if (tokens[itoken].type != JSON_OBJECT)
		return false;

	itoken = tokens[itoken].child;
	while (itoken) {
		string_const_t identifier = json_token_identifier(gltf->buffer, tokens + itoken);
		hash_t identifier_hash = string_hash(STRING_ARGS(identifier));
		if ((identifier_hash == HASH_EXTENSIONS) && (tokens[itoken].type == JSON_STRING))
			metallic_roughness->extensions = json_token_value(buffer, tokens + itoken);
		else if ((identifier_hash == HASH_EXTRAS) && (tokens[itoken].type == JSON_STRING))
			metallic_roughness->extras = json_token_value(buffer, tokens + itoken);
		else if ((identifier_hash == HASH_BASECOLORTEXTURE) &&
		         !gltf_material_parse_textureinfo(gltf, buffer, tokens, itoken,
		                                          &metallic_roughness->base_color_texture))
			return false;
		else if ((identifier_hash == HASH_METALLICROUGHNESSTEXTURE) &&
		         !gltf_material_parse_textureinfo(gltf, buffer, tokens, itoken,
		                                          &metallic_roughness->metallic_roughness_texture))
			return false;
		else if ((identifier_hash == HASH_BASECOLORFACTOR) &&
		         !gltf_token_to_real_array(gltf, buffer, tokens, itoken,
										   metallic_roughness->base_color_factor, 4))
			return false;
		else if ((identifier_hash == HASH_METALLICFACTOR) &&
		         !gltf_token_to_real(gltf, buffer, tokens, itoken, (real*)&metallic_roughness->metallic_factor))
			return false;
		else if ((identifier_hash == HASH_ROUGHNESSFACTOR) &&
		         !gltf_token_to_real(gltf, buffer, tokens, itoken, (real*)&metallic_roughness->roughness_factor))
			return false;

		itoken = tokens[itoken].sibling;
	}

	return true;
}

static bool
gltf_materials_parse_material(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken,
                              gltf_material_t* material) {
	if (tokens[itoken].type != JSON_OBJECT)
		return false;

	gltf_material_initialize(material);

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
		else if ((identifier_hash == HASH_ALPHAMODE) && (tokens[itoken].type == JSON_STRING))
			gltf_material_parse_alphamode(gltf, buffer, tokens, itoken, material);
		else if ((identifier_hash == HASH_ALPHACUTOFF) &&
		         ((tokens[itoken].type == JSON_STRING) || (tokens[itoken].type == JSON_PRIMITIVE)) &&
		         !gltf_token_to_real(gltf, buffer, tokens, itoken, &material->alpha_cutoff))
			return false;
		else if ((identifier_hash == HASH_DOUBLESIDED) &&
		         ((tokens[itoken].type == JSON_STRING) || (tokens[itoken].type == JSON_PRIMITIVE)) &&
		         !gltf_token_to_boolean(gltf, buffer, tokens, itoken, &material->double_sided))
			return false;
		else if ((identifier_hash == HASH_EMISSIVETEXTURE) &&
		         !gltf_material_parse_textureinfo(gltf, buffer, tokens, itoken, &material->emissive_texture))
			return false;
		else if ((identifier_hash == HASH_EMISSIVEFACTOR) &&
		         !gltf_token_to_real_array(gltf, buffer, tokens, itoken, (real*)material->emissive_factor, 3))
			return false;
		else if ((identifier_hash == HASH_NORMALTEXTURE) &&
		         !gltf_material_parse_normaltexture(gltf, buffer, tokens, itoken, material))
			return false;
		else if ((identifier_hash == HASH_OCCLUSIONTEXTURE) &&
		         !gltf_material_parse_occlusiontexture(gltf, buffer, tokens, itoken, material))
			return false;
		else if ((identifier_hash == HASH_PBRMETALLICROUGHNESS) &&
		         !gltf_material_parse_pbrmetallicroughness(gltf, buffer, tokens, itoken, &material->metallic_roughness))
			return false;

		itoken = tokens[itoken].sibling;
	}

	return true;
}

bool
gltf_materials_parse(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken) {
	if (tokens[itoken].type != JSON_ARRAY) {
		log_error(HASH_GLTF, ERROR_INVALID_VALUE, STRING_CONST("Main materials attribute has invalid type"));
		return false;
	}

	size_t materials_count = tokens[itoken].value_length;
	if (materials_count > GLTF_MAX_INDEX)
		return false;
	if (!materials_count)
		return true;

	size_t storage_size = sizeof(gltf_material_t) * materials_count;
	gltf_materials_finalize(gltf);
	gltf->materials_count = (uint)materials_count;
	gltf->materials = memory_allocate(HASH_GLTF, storage_size, 0, MEMORY_PERSISTENT | MEMORY_ZERO_INITIALIZED);

	uint icounter = 0;
	size_t imat = tokens[itoken].child;
	while (imat) {
		if (!gltf_materials_parse_material(gltf, buffer, tokens, imat, gltf->materials + icounter))
			return false;
		imat = tokens[imat].sibling;
		++icounter;
	}

	return true;
}
