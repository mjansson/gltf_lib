/* types.h  -  glTF library  -  Public Domain  -  2018 Mattias Jansson / Rampant Pixels
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

#pragma once

/*! \file types.h
    glTF data types */

#include <foundation/platform.h>
#include <foundation/types.h>

#include <gltf/build.h>

#if defined(GLTF_COMPILE) && GLTF_COMPILE
#  ifdef __cplusplus
#  define GLTF_EXTERN extern "C"
#  define GLTF_API extern "C"
#  else
#  define GLTF_EXTERN extern
#  define GLTF_API extern
#  endif
#else
#  ifdef __cplusplus
#  define GLTF_EXTERN extern "C"
#  define GLTF_API extern "C"
#  else
#  define GLTF_EXTERN extern
#  define GLTF_API extern
#  endif
#endif

#define GLTF_MAX_INDEX 0x7FFFFFFF
#define GLTF_INVALID_INDEX 0xFFFFFFFF

enum gltf_write_mode {
	GLTF_WRITE_GLTF = 0,
	GLTF_WRITE_GLTF_EMBED,
	GLTF_WRITE_GLB,
	GLTF_WRITE_GLB_EMBED,
};

enum gltf_data_type {
	GLTF_DATA_VEC3 = 0
};

enum gltf_component_type {
	GLTF_COMPONENT_SOMETHING = 5100
};

enum gltf_alpha_mode {
	GLTF_ALPHA_MODE_OPAQUE = 0,
	GLTF_ALPHA_MODE_MASK,
	GLTF_ALPHA_MODE_BLEND
};

enum gltf_attribute {
	GLTF_POSITION = 0,
	GLTF_NORMAL,
	GLTF_TANGENT,
	GLTF_TEXCOORD_0,
	GLTF_TEXCOORD_1,
	GLTF_COLOR_0,
	GLTF_JOINTS_0, 
	GLTF_WEIGHTS_0,
	GLTF_ATTRIBUTE_COUNT
};

typedef struct gltf_t                        gltf_t;
typedef struct gltf_accessor_t               gltf_accessor_t;
typedef struct gltf_asset_t                  gltf_asset_t;
typedef struct gltf_attribute_t              gltf_attribute_t;
typedef struct gltf_buffer_t                 gltf_buffer_t;
typedef struct gltf_buffer_view_t            gltf_buffer_view_t;
typedef struct gltf_config_t                 gltf_config_t;
typedef struct gltf_glb_header_t             gltf_glb_header_t;
typedef struct gltf_material_t               gltf_material_t;
typedef struct gltf_mesh_t                   gltf_mesh_t;
typedef struct gltf_node_t                   gltf_node_t;
typedef struct gltf_pbr_metallic_roughness_t gltf_pbr_metallic_roughness_t;
typedef struct gltf_primitive_t              gltf_primitive_t;
typedef struct gltf_scene_t                  gltf_scene_t;
typedef struct gltf_texture_info_t           gltf_texture_info_t;
typedef struct gltf_transform_t              gltf_transform_t;

typedef enum gltf_component_type             gltf_component_type;
typedef enum gltf_data_type                  gltf_data_type;
typedef enum gltf_write_mode                 gltf_write_mode;
typedef enum gltf_alpha_mode                 gltf_alpha_mode;
typedef enum gltf_attribute                  gltf_attribute;

struct gltf_config_t {
	size_t _unused;
};

struct gltf_accessor_t {
	unsigned int            buffer_view;
	gltf_data_type          type;
	gltf_component_type     component_type;
	unsigned int            count;
	float                   min[3];
	float                   max[3];
};

struct gltf_asset_t {
	string_const_t          generator;
	string_const_t          version;
};

struct gltf_buffer_view_t {
	string_const_t          name;
	unsigned int            buffer;
	unsigned int            byte_offset;
	unsigned int            byte_length;
	string_const_t          extensions;
	string_const_t          extras;
};

struct gltf_buffer_t {
	string_const_t          name;
	string_const_t          uri;
	unsigned int            byte_length;
	string_const_t          extensions;
	string_const_t          extras;
};

struct gltf_texture_info_t {
	unsigned int            index;
	unsigned int            texcoord;
	string_const_t          extensions;
	string_const_t          extras;
};

struct gltf_transform_t {
	double                  scale[3];
	double                  rotation[4];
	double                  translation[3];
	double                  matrix[4][4];
};

struct gltf_attribute_t {
	const char*             semantic;
	size_t                  semantic_length;
	unsigned int            accessor;
};

struct gltf_primitive_t {
	unsigned int            material;
	unsigned int            indices;
	unsigned int            attributes[GLTF_ATTRIBUTE_COUNT];
	gltf_attribute_t*       attributes_custom;
	unsigned int            num_attributes_custom;
};

struct gltf_mesh_t {
	string_const_t          name;
	gltf_primitive_t*       primitives;
	unsigned int            num_primitives;
	string_const_t          extensions;
	string_const_t          extras;
};

struct gltf_node_t {
	string_const_t          name;
	unsigned int            mesh;
	gltf_transform_t        transform;
	string_const_t          extensions;
	string_const_t          extras;
};

struct gltf_pbr_metallic_roughness_t {
	gltf_texture_info_t     base_color_texture;
	double                  base_color_factor[4];
	gltf_texture_info_t     metallic_roughness_texture;
	double                  metallic_factor;
	double                  roughness_factor;
	string_const_t          extensions;
	string_const_t          extras;
};

struct gltf_material_t {
	string_const_t          name;
	gltf_pbr_metallic_roughness_t metallic_roughness;
	gltf_texture_info_t     normal_texture;
	double                  normal_scale;
	gltf_texture_info_t     occlusion_texture;
	double                  occlusion_strength;
	gltf_texture_info_t     emissive_texture;
	double                  emissive_factor[3];
	gltf_alpha_mode         alpha_mode;
	double                  alpha_cutoff;
	bool                    double_sided;
	string_const_t          extensions;
	string_const_t          extras;
};

struct gltf_scene_t {
	string_const_t          name;
	unsigned int*           nodes;
	unsigned int            num_nodes;
	string_const_t          extensions;
	string_const_t          extras;
};

struct gltf_glb_header_t {
	uint32_t magic;
	uint32_t version;
	uint32_t length;
};

struct gltf_t {
	void*                   buffer;
	gltf_asset_t            asset;
	gltf_accessor_t*        accessors;
	unsigned int            num_accessors;
	gltf_buffer_view_t*     buffer_views;
	unsigned int            num_buffer_views;
	gltf_buffer_t*          buffers;
	unsigned int            num_buffers;
	unsigned int            scene;
	gltf_scene_t*           scenes;
	unsigned int            num_scenes;
	gltf_node_t*            nodes;
	unsigned int            num_nodes;
	gltf_material_t*        materials;
	unsigned int            num_materials;
	gltf_mesh_t*            meshes;
	unsigned int            num_meshes;
};
