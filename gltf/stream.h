/* stream.h  -  glTF library  -  Public Domain  -  2018 Mattias Jansson
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

/*! \file stream.h
    glTF streams */

#include <gltf/types.h>

/*! Open a stream for a data URI
    \return Stream, null pointer if failed */
GLTF_API stream_t*
gltf_stream_open(gltf_t* gltf, const char* uri, size_t length, unsigned int mode);
