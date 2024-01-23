module;

module mc_moves_reinsertion;

import component;
import atom;
import double3;
import double3x3;
import simd_quatd;
import simulationbox;
import cbmc;
import cbmc_chain_data;
import cbmc_interactions;
import randomnumbers;
import system;
import energy_factor;
import energy_status;
import energy_status_inter;
import running_energy;
import property_lambda_probability_histogram;
import property_widom;
import averages;
import forcefield;
import move_statistics;
import mc_moves_probabilities_particles;
import interactions_framework_molecule;
import interactions_intermolecular;

import <complex>;
import <vector>;
import <array>;
import <tuple>;
import <optional>;
import <span>;
import <optional>;
import <tuple>;
import <algorithm>;
import <chrono>;
import <cmath>;
import <iostream>;
import <iomanip>;


std::optional<RunningEnergy> 
MC_Moves::reinsertionMove(RandomNumber &random, System& system, size_t selectedComponent, size_t selectedMolecule, 
                          std::span<Atom> molecule)
{
  system.components[selectedComponent].mc_moves_statistics.reinsertionMove_CBMC.counts += 1;
  system.components[selectedComponent].mc_moves_statistics.reinsertionMove_CBMC.totalCounts += 1;

  if (system.numberOfMoleculesPerComponent[selectedComponent] > 0)
  {
    double cutOffVDW = system.forceField.useDualCutOff ? system.forceField.dualCutOff : system.forceField.cutOffVDW;
    double cutOffCoulomb = system.forceField.useDualCutOff ? system.forceField.dualCutOff : system.forceField.cutOffCoulomb;

    std::chrono::system_clock::time_point t1 = std::chrono::system_clock::now();
    std::optional<ChainData> growData = 
      CBMC::growMoleculeReinsertion(random, system.components, system.forceField, system.simulationBox, 
                                    system.spanOfFrameworkAtoms(), system.spanOfMoleculeAtoms(), system.beta,
                                    cutOffVDW, cutOffCoulomb, selectedComponent, selectedMolecule, molecule, 
                                    system.numberOfTrialDirections);
    std::chrono::system_clock::time_point t2 = std::chrono::system_clock::now();
    system.components[selectedComponent].mc_moves_cputime.reinsertionMoveCBMCNonEwald += (t2 - t1);
    system.mc_moves_cputime.reinsertionMoveCBMCNonEwald += (t2 - t1);

    if (!growData) return std::nullopt;

    std::span<const Atom> newMolecule = std::span(growData->atom.begin(), growData->atom.end());

    system.components[selectedComponent].mc_moves_statistics.reinsertionMove_CBMC.constructed += 1;
    system.components[selectedComponent].mc_moves_statistics.reinsertionMove_CBMC.totalConstructed += 1;

    std::chrono::system_clock::time_point u1 = std::chrono::system_clock::now();
    ChainData retraceData = 
      CBMC::retraceMoleculeReinsertion(random, system.components, system.forceField, system.simulationBox, 
                                       system.spanOfFrameworkAtoms(), system.spanOfMoleculeAtoms(), system.beta,
                                       cutOffVDW, cutOffCoulomb, selectedComponent, selectedMolecule, molecule, 
                                       growData->storedR, system.numberOfTrialDirections);
    std::chrono::system_clock::time_point u2 = std::chrono::system_clock::now();
    system.components[selectedComponent].mc_moves_cputime.reinsertionMoveCBMCNonEwald += (u2 - u1);
    system.mc_moves_cputime.reinsertionMoveCBMCNonEwald += (u2 - u1);

    std::chrono::system_clock::time_point v1 = std::chrono::system_clock::now();
    RunningEnergy energyFourierDifference = 
      system.energyDifferenceEwaldFourier(system.storedEik, newMolecule, molecule);
    std::chrono::system_clock::time_point v2 = std::chrono::system_clock::now();
    system.components[selectedComponent].mc_moves_cputime.reinsertionMoveCBMCEwald += (v2 - v1);
    system.mc_moves_cputime.reinsertionMoveCBMCEwald += (v2 - v1);

    double correctionFactorDualCutOff = 1.0;
    std::optional<RunningEnergy> energyNew;
    std::optional<RunningEnergy> energyOld;
    if(system.forceField.useDualCutOff)
    {
      energyNew = CBMC::computeExternalNonOverlappingEnergyDualCutOff(system.forceField, system.simulationBox, 
                                 system.spanOfFrameworkAtoms(), system.spanOfMoleculeAtoms(), 
                                 system.forceField.cutOffVDW, system.forceField.cutOffCoulomb, growData->atom);
      energyOld = CBMC::computeExternalNonOverlappingEnergyDualCutOff(system.forceField, system.simulationBox, 
                                 system.spanOfFrameworkAtoms(), system.spanOfMoleculeAtoms(),
                                 system.forceField.cutOffVDW, system.forceField.cutOffCoulomb, retraceData.atom);
      correctionFactorDualCutOff = std::exp(-system.beta * (energyNew->total() - growData->energies.total() 
                                                         - (energyOld->total() - retraceData.energies.total())) );
    }

    double correctionFactorFourier = std::exp(-system.beta * energyFourierDifference.total());

    if (random.uniform() < correctionFactorDualCutOff * correctionFactorFourier * 
                           growData->RosenbluthWeight / retraceData.RosenbluthWeight)
    {
      system.components[selectedComponent].mc_moves_statistics.reinsertionMove_CBMC.accepted += 1;
      system.components[selectedComponent].mc_moves_statistics.reinsertionMove_CBMC.totalAccepted += 1;

      system.acceptEwaldMove();
      std::copy(newMolecule.begin(), newMolecule.end(), molecule.begin());

      if(system.forceField.useDualCutOff)
      {
        return (energyNew.value() - energyOld.value()) + energyFourierDifference;
      }

      return (growData->energies - retraceData.energies) + energyFourierDifference;
    };
  }

  return std::nullopt;
}
