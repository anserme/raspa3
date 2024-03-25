export module atom;

import archive;
import double3;
import stringutils;

import scaling;

import <cmath>;
import <cstddef>;
import <istream>;
import <ostream>;
import <fstream>;
import <sstream>;
import <type_traits>;
#if defined(__has_include) && __has_include(<print>)
  import <print>;
#else
  import print;
#endif


#if defined(_WIN32)
  import <cassert>;
#else
  #include <assert.h>
#endif

// Size is 2 times a __256d (double3 is padded and of size __256d)
// C++17 and higher: std::vector<T> is automatically properly aligned based on type T
export struct Atom
{
  double3 position;
  double3 velocity{};    
  double3 gradient{};    
  double charge;
  double scalingVDW{ 1.0 };
  double scalingCoulomb{ 1.0 };
  uint32_t moleculeId{ 0 };
  uint16_t type{ 0 };
  uint8_t componentId{ 0 };
  uint8_t groupId{ 0 };   // defaults to false

  Atom() noexcept = default;
  Atom(const Atom &a) noexcept = default;
  Atom& operator=(const Atom& a) noexcept = default;
  Atom(Atom&& a) noexcept = default;
  Atom& operator=(Atom&& a) noexcept = default;
  ~Atom() noexcept = default;

  bool operator==(Atom const&) const = default;

  //Atom(double3 position, double charge, double lambda, uint16_t type, uint8_t componentId, uint32_t moleculeId) :
  //    position(position), charge(charge), moleculeId(moleculeId), 
  //            type(type), componentId(componentId)
  //{
  //  scalingVDW = Scaling::scalingVDW(lambda);
  //  scalingCoulomb = Scaling::scalingCoulomb(lambda);
  //};

  Atom(double3 position, double charge, double lambda, uint32_t moleculeId, 
       uint16_t type, uint8_t componentId, uint8_t groupId) :
    position(position), charge(charge), moleculeId(moleculeId),
    type(type), componentId(componentId), groupId(groupId)
  {
    scalingVDW = Scaling::scalingVDW(lambda);
    scalingCoulomb = Scaling::scalingCoulomb(lambda);
  };

  // scaling is linear and first switch LJ on in 0-0.5, then the electrostatics from 0.5 to 1.0
  void setScaling(double lambda)
  {
    scalingVDW = Scaling::scalingVDW(lambda);
    scalingCoulomb = Scaling::scalingCoulomb(lambda);
  }

  void setScalingFullyOn()
  {
    scalingVDW = 1.0;
    scalingCoulomb = 1.0;
  }

  void setScalingFullyOff()
  {
    scalingVDW = 0.0;
    scalingCoulomb = 0.0;
  }

  void setScalingToInteger()
  {
    scalingVDW = 1.0;
    scalingCoulomb = 1.0;
    groupId = uint8_t{ 0 };
  }

  friend Archive<std::ofstream> &operator<<(Archive<std::ofstream> &archive, const Atom &atom);
  friend Archive<std::ifstream> &operator>>(Archive<std::ifstream> &archive, Atom &atom);

  inline std::string repr() const
  {
    std::ostringstream stream;
  
    std::print(stream, "({}, {}, {}, [{}, {}, {}, {}])\n", position.x, position.y, position.z,
        moleculeId, type, componentId, groupId);
  
    return stream.str();
  }
};

// should be 4 times double4 = 4x(8x4) = 4x32 = 128 bytes
static_assert(sizeof(Atom) == 128, "struct Atom size is not 128");
