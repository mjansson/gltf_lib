/* types.h  -  glTF library  -  Public Domain  -  2018 Mattias Jansson
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

#pragma once

/*! \file types.h
    glTF data types */

#include <foundation/platform.h>
#include <foundation/types.h>

#include <vector/types.h>

#include <gltf/build.h>

#if defined(GLTF_COMPILE) && GLTF_COMPILE
#ifdef __cplusplus
#define GLTF_EXTERN extern "C"
#define GLTF_API extern "C"
#else
#define GLTF_EXTERN extern
#define GLTF_API extern
#endif
#else
#ifdef __cplusplus
#define GLTF_EXTERN extern "C"
#define GLTF_API extern "C"
#else
#define GLTF_EXTERN extern
#define GLTF_API extern
#endif
#endif

#define GLTF_MAX_INDEX 0x7FFFFFFF
#define GLTF_INVALID_INDEX 0xFFFFFFFF

enum gltf_file_type {
	GLTF_FILE_GLTF = 0,
	GLTF_FILE_GLTF_EMBED,
	GLTF_FILE_GLB,
	GLTF_FILE_GLB_EMBED,
};

enum gltf_data_type {
	GLTF_DATA_SCALAR = 0,
	GLTF_DATA_VEC2,
	GLTF_DATA_VEC3,
	GLTF_DATA_VEC4,
	GLTF_DATA_MAT2,
	GLTF_DATA_MAT3,
	GLTF_DATA_MAT4
};

enum gltf_component_type {
	GLTF_COMPONENT_BYTE = 5120,
	GLTF_COMPONENT_UNSIGNED_BYTE = 5121,
	GLTF_COMPONENT_SHORT = 5122,
	GLTF_COMPONENT_UNSIGNED_SHORT = 5123,
	GLTF_COMPONENT_UNSIGNED_INT = 5125,
	GLTF_COMPONENT_FLOAT = 5126
};

enum gltf_alpha_mode { GLTF_ALPHA_MODE_OPAQUE = 0, GLTF_ALPHA_MODE_MASK, GLTF_ALPHA_MODE_BLEND };

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

enum gltf_primitive_mode {
	GLTF_POINTS = 0,
	GLTF_LINES,
	GLTF_LINE_LOOP,
	GLTF_LINE_STRIP,
	GLTF_TRIANGLES,
	GLTF_TRIANGLE_STRIP,
	GLTF_TRIANGLE_FAN
};

typedef struct gltf_t gltf_t;
typedef struct gltf_accessor_t gltf_accessor_t;
typedef struct gltf_accessor_sparse_t gltf_accessor_sparse_t;
typedef struct gltf_asset_t gltf_asset_t;
typedef struct gltf_attribute_t gltf_attribute_t;
typedef struct gltf_buffer_t gltf_buffer_t;
typedef struct gltf_buffer_view_t gltf_buffer_view_t;
typedef struct gltf_config_t gltf_config_t;
typedef struct gltf_glb_header_t gltf_glb_header_t;
typedef struct gltf_image_t gltf_image_t;
typedef struct gltf_material_t gltf_material_t;
typedef struct gltf_mesh_t gltf_mesh_t;
typedef struct gltf_node_t gltf_node_t;
typedef struct gltf_pbr_metallic_roughness_t gltf_pbr_metallic_roughness_t;
typedef struct gltf_primitive_t gltf_primitive_t;
typedef struct gltf_scene_t gltf_scene_t;
typedef struct gltf_sparse_indices_t gltf_sparse_indices_t;
typedef struct gltf_sparse_values_t gltf_sparse_values_t;
typedef struct gltf_texture_info_t gltf_texture_info_t;
typedef struct gltf_texture_t gltf_texture_t;
typedef struct gltf_transform_t gltf_transform_t;
typedef struct gltf_binary_chunk_t gltf_binary_chunk_t;

typedef enum gltf_component_type gltf_component_type;
typedef enum gltf_file_type gltf_file_type;
typedef enum gltf_data_type gltf_data_type;
typedef enum gltf_alpha_mode gltf_alpha_mode;
typedef enum gltf_attribute gltf_attribute;
typedef enum gltf_primitive_mode gltf_primitive_mode;

struct gltf_config_t {
	size_t unused;
};

struct gltf_sparse_indices_t {
	uint buffer_view;
	uint byte_offset;
	gltf_component_type component_type;
	string_const_t extensions;
	string_const_t extras;
};

struct gltf_sparse_values_t {
	uint buffer_view;
	uint byte_offset;
	string_const_t extensions;
	string_const_t extras;
};

struct gltf_accessor_sparse_t {
	uint count;
	gltf_sparse_indices_t indices;
	gltf_sparse_values_t values;
	string_const_t extensions;
	string_const_t extras;
};

struct gltf_accessor_t {
	string_const_t name;
	uint buffer_view;
	uint byte_offset;
	gltf_data_type type;
	gltf_component_type component_type;
	uint count;
	bool normalized;
	real min[4];
	real max[4];
	gltf_accessor_sparse_t sparse;
	string_const_t extensions;
	string_const_t extras;
};

struct gltf_asset_t {
	string_const_t generator;
	string_const_t version;
};

struct gltf_buffer_view_t {
	string_const_t name;
	uint buffer;
	uint byte_offset;
	uint byte_length;
	uint byte_stride;
	uint target;
	string_const_t extensions;
	string_const_t extras;
};

struct gltf_buffer_t {
	string_const_t name;
	string_const_t uri;
	uint byte_length;
	string_const_t extensions;
	string_const_t extras;
};

struct gltf_texture_info_t {
	uint index;
	uint texcoord;
	string_const_t extensions;
	string_const_t extras;
};

struct gltf_transform_t {
	real scale[3];
	real rotation[4];
	real translation[3];
	real matrix[4][4];
	bool has_matrix;
};

struct gltf_attribute_t {
	string_const_t semantic;
	uint accessor;
};

struct gltf_primitive_t {
	uint material;
	uint indices;
	uint attributes[GLTF_ATTRIBUTE_COUNT];
	//! Array of custom attributes
	gltf_attribute_t* attributes_custom;
	gltf_primitive_mode mode;
	string_const_t extensions;
	string_const_t extras;
};

struct gltf_mesh_t {
	string_const_t name;
	//! Array of primitives
	gltf_primitive_t* primitives;
	string_const_t extensions;
	string_const_t extras;
};

#define GLTF_NODE_BASE_CHILDREN 4
struct gltf_node_t {
	string_const_t name;
	uint mesh;
	gltf_transform_t transform;
	uint children_count;
	uint* children_ext;
	uint children_base[GLTF_NODE_BASE_CHILDREN];
	string_const_t extensions;
	string_const_t extras;
};

struct gltf_pbr_metallic_roughness_t {
	gltf_texture_info_t base_color_texture;
	real base_color_factor[4];
	gltf_texture_info_t metallic_roughness_texture;
	real metallic_factor;
	real roughness_factor;
	string_const_t extensions;
	string_const_t extras;
};

struct gltf_material_t {
	string_const_t name;
	gltf_pbr_metallic_roughness_t metallic_roughness;
	gltf_texture_info_t normal_texture;
	real normal_scale;
	gltf_texture_info_t occlusion_texture;
	real occlusion_strength;
	gltf_texture_info_t emissive_texture;
	real emissive_factor[3];
	gltf_alpha_mode alpha_mode;
	real alpha_cutoff;
	bool double_sided;
	string_const_t extensions;
	string_const_t extras;
};

struct gltf_image_t {
	string_const_t name;
	string_const_t uri;
	string_const_t mime_type;
	uint buffer_view;
	string_const_t extensions;
	string_const_t extras;
};

struct gltf_texture_t {
	string_const_t name;
	uint sampler;
	uint source;
	string_const_t extensions;
	string_const_t extras;
};

struct gltf_scene_t {
	string_const_t name;
	//! Array of root nodes
	uint* nodes;
	string_const_t extensions;
	string_const_t extras;
};

struct gltf_glb_header_t {
	uint32_t magic;
	uint32_t version;
	uint32_t length;
};

struct gltf_binary_chunk_t {
	size_t offset;
	size_t length;
	string_t uri;
	void* data;
};

struct gltf_t {
	string_t base_path;
	gltf_file_type file_type;
	gltf_binary_chunk_t binary_chunk;
	void* buffer;

	gltf_asset_t asset;
	uint extensions_used_count;
	string_const_t* extensions_used;
	uint extensions_required_count;
	string_const_t* extensions_required;
	//! Array of accessors
	gltf_accessor_t* accessors;
	//! Array of buffer views
	gltf_buffer_view_t* buffer_views;
	//! Array of buffers
	gltf_buffer_t* buffers;
	//! Default scene index
	uint scene;
	//! Array of scenes
	gltf_scene_t* scenes;
	//! Array of nodes
	gltf_node_t* nodes;
	//! Array of materials
	gltf_material_t* materials;
	//! Array of meshes
	gltf_mesh_t* meshes;
	gltf_texture_t* textures;
	uint textures_count;
	gltf_image_t* images;
	uint images_count;

	//! String storage during writing
	string_t* string_array;
	//! Output storage for buffers during writing
	virtualarray_t* output_buffer;
};
