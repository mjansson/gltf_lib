#!/usr/bin/env python

"""Ninja build configurator for glTF library"""

import sys
import os

sys.path.insert(0, os.path.join('build', 'ninja'))

import generator

dependlibs = ['gltf', 'mesh', 'vector', 'foundation']

generator = generator.Generator(project = 'gltf', dependlibs = dependlibs, variables = [('bundleidentifier', 'com.maniccoder.gltf.$(binname)')])
target = generator.target
writer = generator.writer
toolchain = generator.toolchain
extrasources = []
includepaths = []

gltf_sources = [
  'accessor.c', 'buffer.c', 'extension.c', 'gltf.c', 'image.c', 'material.c', 'mesh.c', 'node.c', 'scene.c', 'stream.c', 'texture.c', 'version.c' ]

gltf_lib = generator.lib(module = 'gltf', sources = gltf_sources + extrasources)
#gltf_so = generator.sharedlib(module = 'gltf', sources = gltf_sources + extrasources)

if not target.is_ios() and not target.is_android() and not target.is_tizen():
  configs = [config for config in toolchain.configs if config not in ['profile', 'deploy']]

if generator.skip_tests():
  sys.exit()

includepaths = generator.test_includepaths()

test_cases = []
if toolchain.is_monolithic() or target.is_ios() or target.is_android() or target.is_tizen():
  #Build one fat binary with all test cases
  test_resources = []
  test_extrasources = []
  test_cases += ['all']
  if target.is_ios():
    test_resources = [os.path.join('all', 'ios', item) for item in ['test-all.plist', 'Images.xcassets', 'test-all.xib']]
    test_extrasources = [os.path.join('all', 'ios', 'viewcontroller.m')]
  elif target.is_android():
    test_resources = [os.path.join('all', 'android', item) for item in [
      'AndroidManifest.xml', os.path.join('layout', 'main.xml'), os.path.join('values', 'strings.xml'),
      os.path.join('drawable-ldpi', 'icon.png'), os.path.join('drawable-mdpi', 'icon.png'), os.path.join('drawable-hdpi', 'icon.png'),
      os.path.join('drawable-xhdpi', 'icon.png'), os.path.join('drawable-xxhdpi', 'icon.png'), os.path.join('drawable-xxxhdpi', 'icon.png')
    ]]
    test_extrasources = [os.path.join('all', 'android', 'java', 'com', 'maniccoder', 'gltf', 'test', item) for item in [
      'TestActivity.java'
    ]]
  elif target.is_tizen():
    test_resources = [os.path.join('all', 'tizen', item) for item in [
      'tizen-manifest.xml', os.path.join('res', 'tizenapp.png')
    ]]
  dependlibs = ['test'] + dependlibs
  if target.is_macos() or target.is_ios() or target.is_android() or target.is_tizen():
    generator.app(module = '', sources = [os.path.join(module, 'main.c') for module in test_cases] + test_extrasources, binname = 'test-gltf', basepath = 'test', implicit_deps = [gltf_lib], libs = dependlibs, dependlibs = dependlibs, resources = test_resources, includepaths = includepaths)
  else:
    generator.bin(module = '', sources = [os.path.join(module, 'main.c') for module in test_cases] + test_extrasources, binname = 'test-gltf', basepath = 'test', implicit_deps = [gltf_lib], libs = dependlibs, dependlibs = dependlibs, resources = test_resources, includepaths = includepaths)
else:
  #Build one binary per test case
  if not generator.is_subninja:
    generator.bin(module = 'all', sources = ['main.c'], binname = 'test-all', basepath = 'test', implicit_deps = [gltf_lib], libs = dependlibs, dependlibs = dependlibs, includepaths = includepaths)
  dependlibs = ['test'] + dependlibs
  for test in test_cases:
    if target.is_macos():
      test_resources = [os.path.join('osx', item) for item in ['test-' + test + '.plist', 'Images.xcassets', 'test-' + test + '.xib']]
      generator.app(module = test, sources = ['main.c' ], binname = 'test-' + test, basepath = 'test', implicit_deps = [gltf_lib], libs = dependlibs, dependlibs = dependlibs, resources = test_resources, includepaths = includepaths)
    else:
      generator.bin(module = test, sources = ['main.c' ], binname = 'test-' + test, basepath = 'test', implicit_deps = [gltf_lib], libs = dependlibs, dependlibs = dependlibs, includepaths = includepaths)
