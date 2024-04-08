module;

#ifdef USE_LEGACY_HEADERS
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cmath>
#include <optional>
#include <array>
#include <format>
#include <exception>
#include <source_location>
#include <complex>
#if defined(__has_include) && __has_include(<print>)
   #include <print>
#endif
#endif

module property_enthalpy;

#ifndef USE_LEGACY_HEADERS
import <string>;
import <iostream>;
import <fstream>;
import <sstream>;
import <vector>;
import <cmath>;
import <optional>;
import <array>;
import <format>;
import <exception>;
import <source_location>;
import <complex>;
#if defined(__has_include) && __has_include(<print>)
  import <print>;
#endif
#endif

#if !(defined(__has_include) && __has_include(<print>))
  import print;
#endif

import archive;
import stringutils;
import component;
import units;
import enthalpy_of_adsorption;
import averages;


std::string PropertyEnthalpy::writeAveragesStatistics(std::vector<size_t> &swapableComponents, 
                                                      std::vector<Component> &components) const
{
  std::ostringstream stream;

  std::print(stream, "Enthalpy of adsorption\n");
  std::print(stream, "===============================================================================\n\n");

  if (swapableComponents.empty())
  {
    std::print(stream, "No fluctuating components present.\n\n");
  }
  else
  {
    std::pair<EnthalpyOfAdsorption, EnthalpyOfAdsorption> enthalpy = averageEnthalpy();
    for (size_t k = 0; k < swapableComponents.size(); k++)
    {
      size_t index = swapableComponents[k];
      double idealGasTerm = components[index].idealGasEnergy.value_or(0.0);
      std::print(stream, "Component {} [{}]\n", components[index].componentId, components[index].name);
      std::print(stream, "-------------------------------------------------------------------------------\n");
      for (size_t i = 0; i < numberOfBlocks; ++i)
      {
        EnthalpyOfAdsorption average = averagedEnthalpy(i);
        std::print(stream, "    Block[ {:2d}] {: .6e}\n", i, Units::EnergyToKelvin * (average.values[k] - idealGasTerm));
      }
      std::print(stream, "    ---------------------------------------------------------------------------\n");
      std::print(stream, "    Enthalpy of adsorption: {: .6e} +/- {: .6e} [K]\n", 
                         Units::EnergyToKelvin * (enthalpy.first.values[k] - idealGasTerm), 
                         Units::EnergyToKelvin * enthalpy.second.values[k]);
      std::print(stream, "                            {: .6e} +/- {: .6e} [kJ/mol]\n", 
                         Units::EnergyToKJPerMol * (enthalpy.first.values[k] - idealGasTerm), 
                         Units::EnergyToKJPerMol * enthalpy.second.values[k]);
      if (!components[index].idealGasEnergy)
      {
        std::print(stream, "    Warning: need to subtract the ideal-gas energy.\n");
      }
      std::print(stream, "\n");
    }
    if (swapableComponents.size() > 1)
    {
      std::print(stream, "Total enthalpy of adsorption\n");
      std::print(stream, "-------------------------------------------------------------------------------\n");

      std::vector < double > totalEnthalpyBlocks{};
      double sum = 0.0;
      double sumSquares = 0.0;
      for (size_t i = 0; i < numberOfBlocks; ++i)
      {
        double totalEnthalpyOfAdsorption = 0.0;
        EnthalpyOfAdsorption average = averagedEnthalpy(i);
        for (size_t k = 0; k < swapableComponents.size(); k++)
        {
          size_t index = swapableComponents[k];
          double idealGasTerm = components[index].idealGasEnergy.value_or(0.0);
          totalEnthalpyOfAdsorption += components[index].molFraction * (average.values[k] - idealGasTerm);
        }
        sum += totalEnthalpyOfAdsorption;
        sumSquares += totalEnthalpyOfAdsorption * totalEnthalpyOfAdsorption;
        totalEnthalpyBlocks.push_back(totalEnthalpyOfAdsorption);
        std::print(stream, "    Block[ {:2d}] {}\n", i, Units::EnergyToKelvin * totalEnthalpyOfAdsorption);
      }
      size_t numberOfSamples = totalEnthalpyBlocks.size();
      double average = sum / static_cast<double>(numberOfSamples);
      size_t degreesOfFreedom = numberOfSamples - 1;
      double sumOfSquares = sumSquares - sum * sum / static_cast<double>(numberOfSamples);
      double standardDeviation = std::sqrt((1.0 / static_cast<double>(degreesOfFreedom)) * sumOfSquares);
      double standardError = (1.0 / std::sqrt(static_cast<double>(numberOfSamples))) * standardDeviation;
      double intermediateStandardNormalDeviate = standardNormalDeviates[degreesOfFreedom][chosenConfidenceLevel];
      double confidenceIntervalError = intermediateStandardNormalDeviate * standardError;
      std::pair<double, double> totalEnthalpy = std::make_pair(average, confidenceIntervalError);
      std::print(stream, "    ---------------------------------------------------------------------------\n");
      std::print(stream, "    Enthalpy of adsorption: {: .6e} +/- {: .6e} [K]\n", 
                         Units::EnergyToKelvin * totalEnthalpy.first, 
                         Units::EnergyToKelvin * totalEnthalpy.second);
      std::print(stream, "                            {: .6e} +/- {: .6e} [kJ/mol]\n", 
                         Units::EnergyToKJPerMol * totalEnthalpy.first, 
                         Units::EnergyToKJPerMol * totalEnthalpy.second);
      for (size_t k = 0; k < swapableComponents.size(); k++)
      {
        size_t index = swapableComponents[k];
        if (!components[index].idealGasEnergy)
        {
          std::print(stream, "    Warning: Recompute value of the total enthalpy of adsorption by hand.\n");
          std::print(stream, "             Need to subtract the ideal-gas energy of component {}.\n", index);
        }
      }
      std::print(stream, "\n");
    }
  }

  std::print(stream, "\n");

  return stream.str();
}

Archive<std::ofstream> &operator<<(Archive<std::ofstream> &archive, const PropertyEnthalpy &p)
{
  archive << p.versionNumber;

  archive << p.numberOfBlocks;
  archive << p.numberOfComponents;
  archive << p.bookKeepingEnthalpyOfAdsorptionTerms;

  return archive;
}

Archive<std::ifstream> &operator>>(Archive<std::ifstream> &archive, PropertyEnthalpy &p)
{
  uint64_t versionNumber;
  archive >> versionNumber;
  if(versionNumber > p.versionNumber)
  {
    const std::source_location& location = std::source_location::current();
    throw std::runtime_error(std::format("Invalid version reading 'PropertyEnthalpy' at line {} in file {}\n",
                                         location.line(), location.file_name()));
  }

  archive >> p.numberOfBlocks;
  archive >> p.numberOfComponents;
  archive >> p.bookKeepingEnthalpyOfAdsorptionTerms;

  return archive;
}
