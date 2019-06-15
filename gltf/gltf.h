/* gltf.h  -  glTF library  -  Public Domain  -  2018 Mattias Jansson / Rampant Pixels
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

#pragma once

/*! \file gltf.h
    glTF library entry points */

#include <gltf/types.h>
#include <gltf/hashstrings.h>
#include <gltf/accessor.h>
#include <gltf/buffer.h>
#include <gltf/extension.h>
#include <gltf/node.h>
#include <gltf/scene.h>
#include <gltf/stream.h>
#include <gltf/material.h>
#include <gltf/mesh.h>
#include <gltf/image.h>
#include <gltf/texture.h>

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
gltf_module_parse_config(const char* path, size_t path_size, const char* buffer, size_t size,
                         const struct json_token_t* tokens, size_t num_tokens);

/*! Initialize glTF data structure
\param gltf Target glTF data structure */
GLTF_API void
gltf_initialize(gltf_t* gltf);

/*! Finalize glTF data structure
\param gltf glTF data structure */
GLTF_API void
gltf_finalize(gltf_t* gltf);

/*! Read glTF or glb data
\param gltf Target glTF data structure
\param stream Source stream
\return true if success, false if error */
GLTF_API bool
gltf_read(gltf_t* gltf, stream_t* stream);

/*! Write glTF or glb data
\param gltf Source glTF data structure
\param stream Target stream
\return true if success, false if error */
GLTF_API bool
gltf_write(const gltf_t* gltf, stream_t* stream);

bool
gltf_token_to_integer(const gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken,
                      unsigned int* value);

bool
gltf_token_to_double(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken,
                     double* value);

bool
gltf_token_to_integer_array(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken,
                            unsigned int* values, unsigned int dim);

bool
gltf_token_to_double_array(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken,
                           double* values, unsigned int dim);

bool
gltf_token_to_string_array(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken,
                           string_const_t* values, unsigned int dim);

bool
gltf_token_to_data_type(const gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken,
                        gltf_data_type* value);

bool
gltf_token_to_component_type(const gltf_t* gltf, const char* buffer, json_token_t* tokens,
                             size_t itoken, gltf_component_type* value);

bool
gltf_token_to_boolean(const gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken,
                      bool* value);
