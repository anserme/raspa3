module;

#ifdef USE_LEGACY_HEADERS
#include <cstdint>
#include <type_traits>
#endif

export module skdefinitions;

#ifndef USE_LEGACY_HEADERS
import <type_traits>;
import <cstdint>;
#endif

export enum class Symmorphicity : size_t { asymmorphic = 0, symmorphic = 1, hemisymmorphic = 2 };

export enum class Centring : size_t {
  none = 0,
  primitive = 1,
  body = 2,
  a_face = 3,
  b_face = 4,
  c_face = 5,
  face = 6,
  base = 7,
  r = 8,
  h = 9,
  d = 10
};

export enum class Holohedry : size_t {
  none = 0,
  triclinic = 1,
  monoclinic = 2,
  orthorhombic = 3,
  tetragonal = 4,
  trigonal = 5,
  hexagonal = 6,
  cubic = 7
};
