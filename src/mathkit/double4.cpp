module;

#ifdef USE_LEGACY_HEADERS
#include <cmath>
#include <fstream>
#include <complex>
#include <vector>
#include <array>
#include <map>
#include <algorithm>
#endif

module double4;

#ifndef USE_LEGACY_HEADERS
import <cmath>;
import <fstream>;
import <complex>;
import <vector>;
import <array>;
import <map>;
import <algorithm>;
#endif

import archive;

void double4::normalise()
{
  double magnitude = sqrt((x * x) + (y * y) + (z * z) * (w * w));

  if (magnitude != 0)
  {
    x /= magnitude;
    y /= magnitude;
    z /= magnitude;
    w /= magnitude;
  }
}

Archive<std::ofstream> &operator<<(Archive<std::ofstream> &archive, const double4 &vec)
{
  archive << vec.x << vec.y << vec.z << vec.w;
  return archive;
}

Archive<std::ifstream> &operator>>(Archive<std::ifstream> &archive, double4 &vec)
{
  archive >> vec.x >> vec.y >> vec.z >> vec.w;
  return archive;
}

