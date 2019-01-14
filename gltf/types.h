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

enum gltf_write_mode {
	GLTF_WRITE_GLTF,
	GLTF_WRITE_GLTF_EMBED,
	GLTF_WRITE_GLB,
	GLTF_WRITE_GLB_EMBED,
};

enum gltf_data_type {
	GLTF_DATA_VEC3
};

enum gltf_component_type {
	GLTF_COMPONENT_SOMETHING = 5100
};

typedef struct gltf_t                    gltf_t;
typedef struct gltf_accessor_t           gltf_accessor_t;
typedef struct gltf_asset_t              gltf_asset_t;
typedef struct gltf_buffer_t             gltf_buffer_t;
typedef struct gltf_buffer_view_t        gltf_buffer_view_t;
typedef struct gltf_config_t             gltf_config_t;
typedef struct gltf_glb_header_t         gltf_glb_header_t;

typedef enum gltf_component_type         gltf_component_type;
typedef enum gltf_data_type              gltf_data_type;
typedef enum gltf_write_mode             gltf_write_mode;

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
	unsigned int            buffer;
	unsigned int            byte_offset;
	unsigned int            byte_length;
};

struct gltf_buffer_t {
	string_const_t          uri;
	unsigned int            byte_length;
};

struct gltf_glb_header_t {
	uint32_t magic;
	uint32_t version;
	uint32_t length;
};

struct gltf_t {
	gltf_asset_t*           asset;
	gltf_accessor_t*        accessors;
	gltf_buffer_view_t*     buffer_views;
	gltf_buffer_t*          buffers;
};
