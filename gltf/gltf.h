/* gltf.h  -  glTF library  -  Public Domain  -  2018 Mattias Jansson / Rampant Pixels
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

/*! \file gltf.h
    glTF library entry points */

#include <gltf/types.h>

/*! Initialize glTF library
    \return 0 if success, <0 if error */
GLTF_API int
gltf_module_initialize(gltf_config_t config);

/*! Finalize glTF library */
GLTF_API void
gltf_module_finalize(void);

/*! Query if glTF library is initialized
    \return true if library is initialized, false if not */
GLTF_API bool
gltf_module_is_initialized(void);

/*! Query version of glTF library
    \return Library version */
GLTF_API version_t
gltf_module_version(void);

/*! Parse config declarations from JSON buffer
\param buffer Data buffer
\param size Size of data buffer
\param tokens JSON tokens
\param num_tokens Number of JSON tokens */
GLTF_API void
gltf_module_parse_config(const char* path, size_t path_size,
                         const char* buffer, size_t size,
                         const struct json_token_t* tokens, size_t num_tokens);
