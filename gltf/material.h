/* material.h  -  glTF library  -  Public Domain  -  2018 Mattias Jansson / Rampant Pixels
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

#include "gltf.h"

GLTF_API void
gltf_materials_finalize(gltf_t* gltf);

GLTF_API int
gltf_materials_parse(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken);
