module;

#ifdef USE_LEGACY_HEADERS
#include <format>
#include <tuple>
#include <vector>
#include <string>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <ostream>
#include <vector>
#include <array>
#include <map>
#include <optional>
#include <span>
#if defined(__has_include) && __has_include(<print>)
  #include <print>
#endif
#endif

export module component;

#ifndef USE_LEGACY_HEADERS
import <format>;
import <tuple>;
import <vector>;
import <string>;
import <chrono>;
import <cstdint>;
import <fstream>;
import <sstream>;
import <ostream>;
import <vector>;
import <array>;
import <map>;
import <optional>;
import <span>;
#if defined(__has_include) && __has_include(<print>)
  import <print>;
#endif
#endif

#if !(defined(__has_include) && __has_include(<print>))
  import print;
#endif

import stringutils;
import archive;
import randomnumbers;
import int3;
import double3;
import averages;
import atom;
import forcefield;
import property_lambda_probability_histogram;
import simulationbox;
import property_widom;
import isotherm;
import multi_site_isotherm;
import bond_potential;
import move_statistics;
import mc_moves_probabilities_particles;
import mc_moves_statistics_particles;
import mc_moves_cputime;
import mc_moves_count;


export struct Component
{
  enum class Type : size_t
  {
    Framework = 0,
    Adsorbate = 1,
    Cation = 2
  };

  enum class GrowType: size_t
  {
    Rigid =  0,
    Flexible = 1,
  };

  Component();

  // Construct from file
  Component(Component::Type type, size_t currentComponent, const ForceField &forceField, const std::string &componentName, 
            std::optional<const std::string> fileName, size_t numberOfBlocks, size_t numberOfLambdaBins,
            const MCMoveProbabilitiesParticles &systemProbabilities = MCMoveProbabilitiesParticles()) 
            noexcept(false);

  // Construct programmatically
  Component(size_t componentId, const ForceField &forceField, std::string componentName,
            double T_c, double P_c, double w, std::vector<Atom> definedAtoms, 
            size_t numberOfBlocks, size_t numberOfLambdaBins, 
            const MCMoveProbabilitiesParticles &systemProbabilities = MCMoveProbabilitiesParticles()) noexcept(false);

  uint64_t versionNumber{ 1 };

  Type type { 0 };
  GrowType growType{ 0  };

  size_t componentId{ 0 };
  std::string name{};
  std::optional<std::string> filenameData{};
  std::string filename{};

  bool rigid { true };

  double criticalTemperature{ 0.0 };
  double criticalPressure{ 0.0 };
  double acentricFactor{ 0.0 };
  double molFraction{ 1.0 };
  bool swapable{ false };
  double partialPressure{ 0.0 };

  double mass{ 0.0 };
  std::optional<double> fugacityCoefficient{};
  double amountOfExcessMolecules { 0.0 };
  double bulkFluidDensity{ 0.0 };
  double compressibility{ 0.0 };

  std::optional<double> idealGasRosenbluthWeight{};
  std::optional<double> idealGasEnergy{};

  double netCharge{ 0.0 };
  size_t startingBead{ 0 };
  std::vector<Atom> definedAtoms{};
  std::vector<Atom> atoms{};

  size_t initialNumberOfMolecules{ 0 };

  PropertyLambdaProbabilityHistogram lambdaGC;
  PropertyLambdaProbabilityHistogram lambdaGibbs;
  bool hasFractionalMolecule{ false };

  std::vector<size_t> chiralCenters{};
  std::vector<BondPotential> bonds{};
  std::vector<std::pair<size_t, size_t>> bondDipoles{};
  std::vector<std::tuple<size_t, size_t, size_t>> bends{};
  std::vector<std::pair<size_t, size_t>>  UreyBradley{};
  std::vector<std::tuple<size_t, size_t, size_t, size_t>> inversionBends{};
  std::vector<std::tuple<size_t, size_t, size_t, size_t>> Torsion{};
  std::vector<std::tuple<size_t, size_t, size_t, size_t>> ImproperTorsions{};
  std::vector<std::tuple<size_t, size_t, size_t>> bondBonds{};
  std::vector<std::tuple<size_t, size_t, size_t>> stretchBends{};
  std::vector<std::tuple<size_t, size_t, size_t, size_t>> bendBends{};
  std::vector<std::tuple<size_t, size_t, size_t, size_t>> stretchTorsions{};
  std::vector<std::tuple<size_t, size_t, size_t, size_t>> bendTorsions{};
  std::vector<std::pair<size_t, size_t>> intraVDW{};
  std::vector<std::pair<size_t, size_t>> intraCoulomb{};
  std::vector<std::pair<size_t, size_t>> excludedIntraCoulomb{};
  std::vector<std::pair<size_t, std::vector<size_t>>> configMoves{};

  std::vector<bool> connectivityTable{};
  
  MCMoveProbabilitiesParticles mc_moves_probabilities;
  MCMoveStatisticsParticles mc_moves_statistics;
  MCMoveCpuTime mc_moves_cputime;
  MCMoveCount mc_moves_count;

  PropertyWidom averageRosenbluthWeights;

  MultiSiteIsotherm isotherm{};      // isotherm information
  double massTransferCoefficient{ 0.0 };    // breakthrough masstransfer coefficient [1/s]
  double axialDispersionCoefficient{ 0.0 }; // breakthrough axial disperion coefficient [m^2/s]
  bool isCarrierGas{ false };        // whether or not this is the carrier-gas

  size_t columnPressure{ 0 };
  size_t columnLoading{ 1 };
  size_t columnError{ 2 };

  double lnPartitionFunction { 0 };

  enum class PressureScale
  {
    Log = 0,
    Normal = 1
  };

  PressureScale pressureScale{ PressureScale::Log };

  void readComponent(const ForceField& forceField, const std::string& fileName);

  std::string printStatus(const ForceField& forceField) const;
  std::string printBreakthroughStatus() const;

  std::vector<double3> randomlyRotatedPositionsAroundStartingBead(RandomNumber &random) const;
  std::vector<Atom> recenteredCopy(double scaling, size_t moleculeId) const;
  std::vector<Atom> copyAtoms(std::span<Atom> molecule, double scaling, size_t moleculeId) const;
  std::vector<Atom> copyAtomsRandomlyRotatedAt(RandomNumber &random, double3 position, std::span<Atom> molecule, 
                                               double scaling, size_t moleculeId) const;
  std::vector<Atom> copiedAtoms(std::span<Atom> molecule) const;

  friend Archive<std::ofstream> &operator<<(Archive<std::ofstream> &archive, const Component &c);
  friend Archive<std::ifstream> &operator>>(Archive<std::ifstream> &archive, Component &c);

  std::string repr() const;
};


template<typename T>
std::vector<T> parseListOfParameters(const std::string& arguments, size_t lineNumber)
{
  std::vector<T> list{};

  std::string str;
  std::istringstream ss(arguments);

  while (ss >> str)
  {
    if (trim(str).rfind("//", 0) == 0)
    {
      if (list.empty())
      {
        throw std::runtime_error(std::format("No values could be read at line: {}\n", lineNumber));
      }
      return list;
    }
    T value;
    std::istringstream s(str);
    if (s >> value)
    {
      list.push_back(value);
    }
    else
    {
      if (list.empty())
      {
        throw std::runtime_error(std::format("No values could be read at line: {}\n", lineNumber));
      }
      return list;
    }
  };

  return list;
}

