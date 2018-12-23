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

typedef struct gltf_config_t             gltf_config_t;

struct gltf_config_t {
	size_t _unused;
};
