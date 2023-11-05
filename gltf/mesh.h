/* mesh.h  -  glTF library  -  Public Domain  -  2018 Mattias Jansson
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

#include "gltf.h"

GLTF_API void
gltf_meshes_finalize(gltf_t* gltf);

GLTF_API bool
gltf_meshes_parse(gltf_t* gltf, const char* buffer, json_token_t* tokens, size_t itoken);

//! External data structure
struct mesh_t;

GLTF_API uint
gltf_mesh_add_mesh(gltf_t* gltf, const struct mesh_t* mesh, uint* mesh_material_map);
