module;

module mc_moves_swap_cfcmc_cbmc;

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
import <type_traits>;

import component;
import atom;
import double3;
import double3x3;
import simd_quatd;
import simulationbox;
import cbmc;
import cbmc_chain_data;
import randomnumbers;
import system;
import energy_factor;
import energy_status;
import energy_status_inter;
import property_lambda_probability_histogram;
import property_widom;
import averages;
import running_energy;
import forcefield;
import move_statistics;
import mc_moves_probabilities_particles;
import interactions_framework_molecule;
import interactions_intermolecular;
import interactions_ewald;


std::pair<std::optional<RunningEnergy>, double3> 
MC_Moves::swapMove_CFCMC_CBMC(RandomNumber &random, System& system, size_t selectedComponent, size_t selectedMolecule,
                              bool insertionDisabled, bool deletionDisabled)
{
  PropertyLambdaProbabilityHistogram& lambda = system.components[selectedComponent].lambdaGC;
  size_t oldBin = lambda.currentBin;
  double deltaLambda = lambda.delta;
  double oldLambda = system.components[selectedComponent].lambdaGC.lambdaValue();
  double maxChange = 
    system.components[selectedComponent].mc_moves_statistics.swapMove_CFCMC_CBMC.maxChange[2];
  std::make_signed_t<std::size_t> selectedNewBin = lambda.selectNewBin(random, maxChange);
  size_t oldN = system.numberOfIntegerMoleculesPerComponent[selectedComponent];

  size_t indexFractionalMolecule = system.indexOfGCFractionalMoleculesPerComponent_CFCMC(selectedComponent);

  if (selectedNewBin >= std::make_signed_t<std::size_t>(lambda.numberOfBins)) // Insertion move
  {
    if(insertionDisabled)
    {
      return {std::nullopt, double3(0.0, 0.0, 0.0)};
    }

    // Steps for insertion Lambda_new = 1 + epsilon
    // ===================================================================
    // (1) Unbiased: the fractional molecule with lambda=lambda_old is made integer (lambda=1) and deltaU is computed
    // (2) Biased: a new fractional molecule is grown with lambda_new = epsilon

    double cutOffVDW = system.forceField.cutOffVDW;
    double cutOffCoulomb = system.forceField.cutOffCoulomb;
    Component::GrowType growType = system.components[selectedComponent].growType;

    size_t newBin = static_cast<size_t>(selectedNewBin - std::make_signed_t<std::size_t>(lambda.numberOfBins));
    double newLambda = deltaLambda * static_cast<double>(newBin);

    system.components[selectedComponent].mc_moves_statistics.swapMove_CFCMC_CBMC.counts[0] += 1;
    system.components[selectedComponent].mc_moves_statistics.swapMove_CFCMC_CBMC.totalCounts[0] += 1;

    std::span<Atom> fractionalMolecule = system.spanOfMolecule(selectedComponent, indexFractionalMolecule);

    // make copy of old fractional molecule for reference and restoring
    std::vector<Atom> oldFractionalMolecule(fractionalMolecule.begin(), fractionalMolecule.end());

    // fractional particle becomes integer (lambda=1.0)
    for (Atom& atom : fractionalMolecule) 
    { 
      atom.setScalingToInteger();
    }        

    std::chrono::system_clock::time_point t1 = std::chrono::system_clock::now();
    std::optional<RunningEnergy> frameworkDifference = 
      Interactions::computeFrameworkMoleculeEnergyDifference(system.forceField, system.simulationBox,
                                   system.spanOfFrameworkAtoms(), fractionalMolecule, oldFractionalMolecule);
    std::chrono::system_clock::time_point t2 = std::chrono::system_clock::now();
    system.components[selectedComponent].mc_moves_cputime.swapLambdaInsertionMoveCBCFCMCNonEwald += (t2 - t1);
    system.mc_moves_cputime.swapLambdaInsertionMoveCBCFCMCNonEwald += (t2 - t1);

    if(!frameworkDifference.has_value())
    {
      // reject, set fractional molecule back to old state
      std::copy(oldFractionalMolecule.begin(), oldFractionalMolecule.end(), fractionalMolecule.begin());
      return {std::nullopt, double3(0.0, 1.0, 0.0)};
    }

    std::chrono::system_clock::time_point u1 = std::chrono::system_clock::now();
    std::optional<RunningEnergy> moleculeDifference = 
      Interactions::computeInterMolecularEnergyDifference(system.forceField, system.simulationBox,
                                         system.spanOfMoleculeAtoms(), fractionalMolecule, oldFractionalMolecule);
    std::chrono::system_clock::time_point u2 = std::chrono::system_clock::now();
    system.components[selectedComponent].mc_moves_cputime.swapLambdaInsertionMoveCBCFCMCNonEwald += (u2 - u1);
    system.mc_moves_cputime.swapLambdaInsertionMoveCBCFCMCNonEwald += (u2 - u1);

    if(!moleculeDifference.has_value())
    {
      // reject, set fractional molecule back to old state
      std::copy(oldFractionalMolecule.begin(), oldFractionalMolecule.end(), fractionalMolecule.begin());
      
      return {std::nullopt, double3(0.0, 1.0, 0.0)};
    }


    std::chrono::system_clock::time_point v1 = std::chrono::system_clock::now();
    RunningEnergy EwaldEnergyDifference = 
      //system.energyDifferenceEwaldFourier(system.storedEik, fractionalMolecule, oldFractionalMolecule);
      Interactions::energyDifferenceEwaldFourier(system.eik_x, system.eik_y, system.eik_z, system.eik_xy,
                                               system.storedEik, system.totalEik,
                                               system.forceField, system.simulationBox,
                                               fractionalMolecule, oldFractionalMolecule);
    std::chrono::system_clock::time_point v2 = std::chrono::system_clock::now();
    system.components[selectedComponent].mc_moves_cputime.swapLambdaInsertionMoveCBCFCMCEwald += (v2 - v1);
    system.mc_moves_cputime.swapLambdaInsertionMoveCBCFCMCEwald += (v2 - v1);

    RunningEnergy tailEnergyDifference = 
      Interactions::computeInterMolecularTailEnergyDifference(system.forceField, system.simulationBox,                      
                                         system.spanOfMoleculeAtoms(), fractionalMolecule, oldFractionalMolecule) +
      Interactions::computeFrameworkMoleculeTailEnergyDifference(system.forceField, system.simulationBox,
                                         system.spanOfFrameworkAtoms(), fractionalMolecule, oldFractionalMolecule);

    RunningEnergy energyDifference = frameworkDifference.value() + moleculeDifference.value() + EwaldEnergyDifference + 
                                     tailEnergyDifference;

    // grow molecule with newLambda
    size_t newMolecule = system.numberOfMoleculesPerComponent[selectedComponent];

    // copy atoms from the old-fractional molecule, including the groupdIds
    std::vector<Atom> newatoms = 
      system.components[selectedComponent].copyAtoms(oldFractionalMolecule, newLambda, 
                                                     system.numberOfMoleculesPerComponent[selectedComponent]);

    std::chrono::system_clock::time_point w1 = std::chrono::system_clock::now();
    std::optional<ChainData> growData = 
      CBMC::growMoleculeSwapInsertion(random, system.components, system.forceField, system.simulationBox, 
                                      system.spanOfFrameworkAtoms(), system.spanOfMoleculeAtoms(), system.beta,
                                      growType, cutOffVDW, cutOffCoulomb, selectedComponent, newMolecule, newLambda, 
                                      newatoms, system.numberOfTrialDirections);
    std::chrono::system_clock::time_point w2 = std::chrono::system_clock::now();
    system.components[selectedComponent].mc_moves_cputime.swapLambdaInsertionMoveCBCFCMCNonEwald += (w2 - w1);
    system.mc_moves_cputime.swapLambdaInsertionMoveCBCFCMCNonEwald += (w2 - w1);

    if (!growData)
    {
      // reject, set fractional molecule back to old state
      std::copy(oldFractionalMolecule.begin(), oldFractionalMolecule.end(), fractionalMolecule.begin());
      
      return {std::nullopt, double3(0.0, 1.0, 0.0)};
    }

    system.components[selectedComponent].mc_moves_statistics.swapMove_CFCMC_CBMC.constructed[0] += 1;
    system.components[selectedComponent].mc_moves_statistics.swapMove_CFCMC_CBMC.totalConstructed[0] += 1;

    std::chrono::system_clock::time_point y1 = std::chrono::system_clock::now();
    RunningEnergy energyFourierDifference = 
      //system.energyDifferenceEwaldFourier(system.totalEik, std::span(growData->atom.begin(), growData->atom.end()), {});
      Interactions::energyDifferenceEwaldFourier(system.eik_x, system.eik_y, system.eik_z, system.eik_xy,
                                               system.storedEik, system.totalEik,
                                               system.forceField, system.simulationBox,
                                               std::span(growData->atom.begin(), growData->atom.end()), {});
    std::chrono::system_clock::time_point y2 = std::chrono::system_clock::now();
    system.components[selectedComponent].mc_moves_cputime.swapLambdaInsertionMoveCBCFCMCEwald += (y2 - y1);
    system.mc_moves_cputime.swapLambdaInsertionMoveCBCFCMCEwald += (y2 - y1);

    RunningEnergy tailEnergyDifferenceGrow = 
      Interactions::computeInterMolecularTailEnergyDifference(system.forceField, system.simulationBox,
                          system.spanOfMoleculeAtoms(), std::span(growData->atom.begin(), growData->atom.end()), {}) +
      Interactions::computeFrameworkMoleculeTailEnergyDifference(system.forceField, system.simulationBox,
                         system.spanOfFrameworkAtoms(), std::span(growData->atom.begin(), growData->atom.end()), {});

    double correctionFactorEwald = 
      std::exp(-system.beta * (energyFourierDifference.total() + tailEnergyDifferenceGrow.total()));

    double idealGasRosenbluthWeight = system.components[selectedComponent].idealGasRosenbluthWeight.value_or(1.0);
    double preFactor = correctionFactorEwald * system.beta * system.components[selectedComponent].molFraction * 
                       system.pressure * system.simulationBox.volume /
                       static_cast<double>(1 + system.numberOfIntegerMoleculesPerComponent[selectedComponent]);
    double biasTerm = lambda.biasFactor[newBin] - lambda.biasFactor[oldBin];
    double Pacc = preFactor * (growData->RosenbluthWeight / idealGasRosenbluthWeight) * 
                               exp(-system.beta * energyDifference.total() + biasTerm);

    double biasTransitionMatrix = system.tmmc.biasFactor(oldN + 1, oldN);
    
    if(system.tmmc.doTMMC)
    {
      size_t newN = oldN + 1;
      if(newN > system.tmmc.maxMacrostate)
      {
        return {std::nullopt, double3(0.0, 1.0 - Pacc, Pacc)};
      }
    }


    if (random.uniform() < biasTransitionMatrix * Pacc)
    {
      //system.acceptEwaldMove();
      Interactions::acceptEwaldMove(system.forceField, system.storedEik, system.totalEik);

      system.components[selectedComponent].lambdaGC.setCurrentBin(newBin);

      // Note: inserting invalidates iterators and spans (the vector could reallocate memory)
      system.insertMolecule(selectedComponent, growData->atom);

      // swap first and last molecule (selectedMolecule) so that molecule 'indexFractionalMolecule' 
      // is always the fractional molecule 
      size_t lastMoleculeId = system.numberOfMoleculesPerComponent[selectedComponent] - 1;
      std::span<Atom> lastMolecule = system.spanOfMolecule(selectedComponent, lastMoleculeId);
      fractionalMolecule = system.spanOfMolecule(selectedComponent, indexFractionalMolecule);
      std::swap_ranges(fractionalMolecule.begin(), fractionalMolecule.end(), lastMolecule.begin());
      
      system.components[selectedComponent].mc_moves_statistics.swapMove_CFCMC_CBMC.accepted[0] += 1;
      system.components[selectedComponent].mc_moves_statistics.swapMove_CFCMC_CBMC.totalAccepted[0] += 1;

      return {energyDifference + growData->energies + energyFourierDifference + tailEnergyDifferenceGrow, 
              double3(0.0, 1.0 - Pacc, Pacc)};
    };

    // Restore old lamba
    std::copy(oldFractionalMolecule.begin(), oldFractionalMolecule.end(), fractionalMolecule.begin());
    
    return {std::nullopt, double3(0.0, 1.0, 0.0)};
  }
  else if (selectedNewBin < 0) // Deletion move
  {
    if(deletionDisabled)
    {
      return {std::nullopt, double3(0.0, 0.0, 0.0)};
    }
    // Steps for deletion Lambda_new = -epsilon
    // ===================================================================
    // (1) Biased: the existing fractional molecule is retraced using CBMC with lambda=lambda_o, 
    //             fractional molecule is removed.
    // (2) Unbiased: A new fractional molecule is chosen with lambda_new = 1 - epsilon, deltaU is computed.

    if (system.numberOfIntegerMoleculesPerComponent[selectedComponent] > 0)
    {
      double cutOffVDW = system.forceField.cutOffVDW;
      double cutOffCoulomb = system.forceField.cutOffCoulomb;

      selectedMolecule = system.randomIntegerMoleculeOfComponent(random, selectedComponent);
      
      system.components[selectedComponent].mc_moves_statistics.swapMove_CFCMC_CBMC.counts[1] += 1;
      system.components[selectedComponent].mc_moves_statistics.swapMove_CFCMC_CBMC.totalCounts[1] += 1;
      
      std::span<Atom> fractionalMolecule = system.spanOfMolecule(selectedComponent, indexFractionalMolecule);
      std::span<Atom> newFractionalMolecule = system.spanOfMolecule(selectedComponent, selectedMolecule);
      
      // make copy of old fractional molecule for reference and restoring
      std::vector<Atom> oldFractionalMolecule(fractionalMolecule.begin(), fractionalMolecule.end());
      std::vector<Atom> oldNewFractionalMolecule(newFractionalMolecule.begin(), newFractionalMolecule.end());
      
      // (1) Biased: the existing fractional molecule is retraced using CBMC with lambda=lambda_o, fractional 
      //             molecule is removed.
      std::chrono::system_clock::time_point t1 = std::chrono::system_clock::now();
      ChainData retraceData = 
        CBMC::retraceMoleculeSwapDeletion(random, system.components, system.forceField, system.simulationBox, 
                                          system.spanOfFrameworkAtoms(), system.spanOfMoleculeAtoms(), system.beta,
                                          cutOffVDW, cutOffCoulomb, selectedComponent, indexFractionalMolecule, 
                                          fractionalMolecule, oldLambda, 0.0, system.numberOfTrialDirections);
      std::chrono::system_clock::time_point t2 = std::chrono::system_clock::now();
      system.components[selectedComponent].mc_moves_cputime.swapLambdaDeletionMoveCBCFCMCNonEwald += (t2 - t1);
      system.mc_moves_cputime.swapLambdaDeletionMoveCBCFCMCNonEwald += (t2 - t1);
      
      std::chrono::system_clock::time_point u1 = std::chrono::system_clock::now();
      RunningEnergy energyFourierDifference = 
        //system.energyDifferenceEwaldFourier(system.storedEik, {}, fractionalMolecule);
        Interactions::energyDifferenceEwaldFourier(system.eik_x, system.eik_y, system.eik_z, system.eik_xy,
                                               system.storedEik, system.totalEik,
                                               system.forceField, system.simulationBox,
                                               {}, fractionalMolecule);
      std::chrono::system_clock::time_point u2 = std::chrono::system_clock::now();
      system.components[selectedComponent].mc_moves_cputime.swapLambdaDeletionMoveCBCFCMCEwald += (u2 - u1);
      system.mc_moves_cputime.swapLambdaDeletionMoveCBCFCMCEwald += (u2 - u1);

      RunningEnergy tailEnergyDifferenceRetrace = 
        Interactions::computeInterMolecularTailEnergyDifference(system.forceField, system.simulationBox,
                                         system.spanOfMoleculeAtoms(), {}, fractionalMolecule) +
        Interactions::computeFrameworkMoleculeTailEnergyDifference(system.forceField, system.simulationBox,
                                         system.spanOfFrameworkAtoms(), {}, fractionalMolecule);
      
      double correctionFactorEwald = 
        std::exp(-system.beta * (energyFourierDifference.total() + tailEnergyDifferenceRetrace.total()));
      
      for (Atom &atom : fractionalMolecule) 
      { 
        atom.setScalingFullyOff();
        atom.groupId = uint8_t{ 0 };
      }
      
      std::vector<Atom> savedFractionalMolecule(newFractionalMolecule.begin(), newFractionalMolecule.end());
      
      // (2) Unbiased: A new fractional molecule is chosen with lambda_new = 1 - epsilon, deltaU is computed.
      size_t newBin = static_cast<size_t>(selectedNewBin + std::make_signed_t<std::size_t>(lambda.numberOfBins));
      double newLambda = deltaLambda * static_cast<double>(newBin);
     
      // get the groupIds from the fractional molecule, set new Lambda
      std::transform(newFractionalMolecule.begin(), newFractionalMolecule.end(), 
                     oldFractionalMolecule.begin(), newFractionalMolecule.begin(), 
                     [newLambda](const Atom& a, const Atom& b) { return Atom(a.position, a.charge, newLambda, 
                                                                 a.moleculeId, a.type, a.componentId, b.groupId); });
     
      std::chrono::system_clock::time_point v1 = std::chrono::system_clock::now();
      std::optional<RunningEnergy> frameworkDifference = 
        Interactions::computeFrameworkMoleculeEnergyDifference(system.forceField, system.simulationBox,
                                   system.spanOfFrameworkAtoms(), newFractionalMolecule, savedFractionalMolecule);
      std::chrono::system_clock::time_point v2 = std::chrono::system_clock::now();
      system.components[selectedComponent].mc_moves_cputime.swapLambdaDeletionMoveCBCFCMCNonEwald += (v2 - v1);
      system.mc_moves_cputime.swapLambdaDeletionMoveCBCFCMCNonEwald += (v2 - v1);
      if(!frameworkDifference.has_value())
      {
        std::copy(oldFractionalMolecule.begin(), oldFractionalMolecule.end(), fractionalMolecule.begin());
        std::copy(oldNewFractionalMolecule.begin(), oldNewFractionalMolecule.end(), newFractionalMolecule.begin());
       
        return {std::nullopt, double3(0.0, 1.0, 0.0)};
      }
      
      std::chrono::system_clock::time_point w1 = std::chrono::system_clock::now();
      std::optional<RunningEnergy> moleculeDifference = 
        Interactions::computeInterMolecularEnergyDifference(system.forceField, system.simulationBox,
                                       system.spanOfMoleculeAtoms(), newFractionalMolecule, savedFractionalMolecule);
      std::chrono::system_clock::time_point w2 = std::chrono::system_clock::now();
      system.components[selectedComponent].mc_moves_cputime.swapLambdaDeletionMoveCBCFCMCNonEwald += (w2 - w1);
      system.mc_moves_cputime.swapLambdaDeletionMoveCBCFCMCNonEwald += (w2 - w1);
      if(!moleculeDifference.has_value())
      {
        std::copy(oldFractionalMolecule.begin(), oldFractionalMolecule.end(), fractionalMolecule.begin());
        std::copy(oldNewFractionalMolecule.begin(), oldNewFractionalMolecule.end(), newFractionalMolecule.begin());
        
        return {std::nullopt, double3(0.0, 1.0, 0.0)};
      }
      
      std::chrono::system_clock::time_point y1 = std::chrono::system_clock::now();
      RunningEnergy EwaldEnergyDifference = 
        //system.energyDifferenceEwaldFourier(system.totalEik, newFractionalMolecule, savedFractionalMolecule);
        Interactions::energyDifferenceEwaldFourier(system.eik_x, system.eik_y, system.eik_z, system.eik_xy,
                                               system.storedEik, system.totalEik,
                                               system.forceField, system.simulationBox,
                                               newFractionalMolecule, savedFractionalMolecule);
      std::chrono::system_clock::time_point y2 = std::chrono::system_clock::now();
      system.components[selectedComponent].mc_moves_cputime.swapLambdaDeletionMoveCBCFCMCEwald += (y2 - y1);
      system.mc_moves_cputime.swapLambdaDeletionMoveCBCFCMCEwald += (y2 - y1);
      
      RunningEnergy tailEnergyDifference = 
        Interactions::computeInterMolecularTailEnergyDifference(system.forceField, system.simulationBox,
                                     system.spanOfMoleculeAtoms(), newFractionalMolecule, savedFractionalMolecule) +
        Interactions::computeFrameworkMoleculeTailEnergyDifference(system.forceField, system.simulationBox,
                                     system.spanOfFrameworkAtoms(), newFractionalMolecule, savedFractionalMolecule);
      RunningEnergy energyDifference = frameworkDifference.value() + moleculeDifference.value() + 
                                       EwaldEnergyDifference + tailEnergyDifference;
      
      system.components[selectedComponent].mc_moves_statistics.swapMove_CFCMC_CBMC.constructed[1] += 1;
      system.components[selectedComponent].mc_moves_statistics.swapMove_CFCMC_CBMC.totalConstructed[1] += 1;
      
      double idealGasRosenbluthWeight = system.components[selectedComponent].idealGasRosenbluthWeight.value_or(1.0);
      double preFactor = 
               correctionFactorEwald * double(system.numberOfIntegerMoleculesPerComponent[selectedComponent]) /
               (system.beta * system.components[selectedComponent].molFraction * 
                system.pressure * system.simulationBox.volume);
      double biasTerm = lambda.biasFactor[newBin] - lambda.biasFactor[oldBin];
      double Pacc = preFactor * (idealGasRosenbluthWeight / 
                            retraceData.RosenbluthWeight) * exp(-system.beta * energyDifference.total() + biasTerm);

      double biasTransitionMatrix = system.tmmc.biasFactor(oldN - 1, oldN);

      if(system.tmmc.doTMMC)
      {
        size_t newN = oldN - 1;
        if(newN < system.tmmc.minMacrostate)
        {
          return {std::nullopt, double3(Pacc, 1.0 - Pacc, 0.0)};
        }
      }

      if (random.uniform() < biasTransitionMatrix * Pacc)
      {
        //system.acceptEwaldMove();
        Interactions::acceptEwaldMove(system.forceField, system.storedEik, system.totalEik);
        system.components[selectedComponent].lambdaGC.setCurrentBin(newBin);
      
        // Swap first and last molecule (selectedMolecule) so that molecule 'indexFractionalMolecule' 
        // is always the fractional molecule 
        std::swap_ranges(newFractionalMolecule.begin(), newFractionalMolecule.end(), fractionalMolecule.begin());
      
        system.deleteMolecule(selectedComponent, selectedMolecule, newFractionalMolecule);
      
        system.components[selectedComponent].mc_moves_statistics.swapMove_CFCMC_CBMC.accepted[1] += 1;
        system.components[selectedComponent].mc_moves_statistics.swapMove_CFCMC_CBMC.totalAccepted[1] += 1;
      
        return {energyDifference + energyFourierDifference + tailEnergyDifferenceRetrace - retraceData.energies, 
                double3(Pacc, 1.0 - Pacc, 0.0)};
      };

      // Restore the old- and the newly chosen fractional molecule
      std::copy(oldFractionalMolecule.begin(), oldFractionalMolecule.end(), fractionalMolecule.begin());
      std::copy(oldNewFractionalMolecule.begin(), oldNewFractionalMolecule.end(), newFractionalMolecule.begin());
      
      return {std::nullopt, double3(Pacc, 1.0 - Pacc, 0.0)};
    }
    return {std::nullopt, double3(0.0, 1.0, 0.0)};
  }
  else // Lambda-move
  {
    size_t newBin = static_cast<size_t>(selectedNewBin);
    double newLambda = deltaLambda * static_cast<double>(newBin);

    system.components[selectedComponent].mc_moves_statistics.swapMove_CFCMC_CBMC.counts[2] += 1;
    system.components[selectedComponent].mc_moves_statistics.swapMove_CFCMC_CBMC.totalCounts[2] += 1;

    std::span<Atom> molecule = system.spanOfMolecule(selectedComponent, indexFractionalMolecule);

    std::vector<Atom> trialPositions(molecule.begin(), molecule.end());
    std::transform(molecule.begin(), molecule.end(), trialPositions.begin(),
        [&](Atom a) { a.setScaling(newLambda); return a; });

    std::chrono::system_clock::time_point t1 = std::chrono::system_clock::now();
    std::optional<RunningEnergy> frameworkEnergyDifference = 
      Interactions::computeFrameworkMoleculeEnergyDifference(system.forceField, system.simulationBox,
                                                  system.spanOfFrameworkAtoms(), trialPositions, molecule);
    std::chrono::system_clock::time_point t2 = std::chrono::system_clock::now();
    if (insertionDisabled || deletionDisabled)
    {
      system.components[selectedComponent].mc_moves_cputime.WidomMoveCBCFCMCNonEwald += (t2 - t1);
      system.mc_moves_cputime.WidomMoveCBCFCMCNonEwald += (t2 - t1);
    }
    else
    {
      system.components[selectedComponent].mc_moves_cputime.swapLambdaChangeMoveCBCFCMCNonEwald += (t2 - t1);
      system.mc_moves_cputime.swapLambdaChangeMoveCBCFCMCNonEwald += (t2 - t1);
    }

    if(!frameworkEnergyDifference.has_value()) return {std::nullopt, double3(0.0, 1.0, 0.0)};

    std::chrono::system_clock::time_point u1 = std::chrono::system_clock::now();
    std::optional<RunningEnergy> interEnergyDifference = 
      Interactions::computeInterMolecularEnergyDifference(system.forceField, system.simulationBox,
                                         system.spanOfMoleculeAtoms(), trialPositions, molecule);
    std::chrono::system_clock::time_point u2 = std::chrono::system_clock::now();
    if (insertionDisabled || deletionDisabled)
    {
      system.components[selectedComponent].mc_moves_cputime.WidomMoveCBCFCMCNonEwald += (t2 - t1);
      system.mc_moves_cputime.WidomMoveCBCFCMCNonEwald += (t2 - t1);
    }
    else
    {
      system.components[selectedComponent].mc_moves_cputime.swapLambdaChangeMoveCBCFCMCNonEwald += (u2 - u1);
      system.mc_moves_cputime.swapLambdaChangeMoveCBCFCMCNonEwald += (u2 - u1);
    }

    if(!interEnergyDifference.has_value()) return {std::nullopt, double3(0.0, 1.0, 0.0)};

    std::chrono::system_clock::time_point v1 = std::chrono::system_clock::now();
    RunningEnergy EwaldFourierDifference = 
      //system.energyDifferenceEwaldFourier(system.storedEik, trialPositions, molecule);
      Interactions::energyDifferenceEwaldFourier(system.eik_x, system.eik_y, system.eik_z, system.eik_xy,
                                               system.storedEik, system.totalEik,
                                               system.forceField, system.simulationBox,
                                               trialPositions, molecule);
    std::chrono::system_clock::time_point v2 = std::chrono::system_clock::now();
    if (insertionDisabled || deletionDisabled)
    {
      system.components[selectedComponent].mc_moves_cputime.WidomMoveCBCFCMCEwald += (t2 - t1);
      system.mc_moves_cputime.WidomMoveCBCFCMCEwald += (t2 - t1);
    }
    else
    {
      system.components[selectedComponent].mc_moves_cputime.swapLambdaChangeMoveCBCFCMCEwald += (v2 - v1);
      system.mc_moves_cputime.swapLambdaChangeMoveCBCFCMCEwald += (v2 - v1);
    }

    std::chrono::system_clock::time_point w1 = std::chrono::system_clock::now();
    [[maybe_unused]] RunningEnergy tailEnergyDifference = 
      Interactions::computeInterMolecularTailEnergyDifference(system.forceField, system.simulationBox,
                                         system.spanOfMoleculeAtoms(), trialPositions, molecule) +
      Interactions::computeFrameworkMoleculeTailEnergyDifference(system.forceField, system.simulationBox,
                                         system.spanOfFrameworkAtoms(), trialPositions, molecule);
    std::chrono::system_clock::time_point w2 = std::chrono::system_clock::now();
    system.components[selectedComponent].mc_moves_cputime.swapLambdaChangeMoveCBCFCMCTail += (w2 - w1);
    system.mc_moves_cputime.swapLambdaChangeMoveCBCFCMCTail += (w2 - w1);

    RunningEnergy energyDifference = frameworkEnergyDifference.value() + interEnergyDifference.value() + 
                                     EwaldFourierDifference + tailEnergyDifference;

    system.components[selectedComponent].mc_moves_statistics.swapMove_CFCMC_CBMC.constructed[2] += 1;
    system.components[selectedComponent].mc_moves_statistics.swapMove_CFCMC_CBMC.totalConstructed[2] += 1;


    double biasTerm = lambda.biasFactor[newBin] - lambda.biasFactor[oldBin];
    if (random.uniform() < std::exp(-system.beta * energyDifference.total() + biasTerm))
    {
      //system.acceptEwaldMove();
      Interactions::acceptEwaldMove(system.forceField, system.storedEik, system.totalEik);
      system.components[selectedComponent].mc_moves_statistics.swapMove_CFCMC_CBMC.accepted[2] += 1;
      system.components[selectedComponent].mc_moves_statistics.swapMove_CFCMC_CBMC.totalAccepted[2] += 1;
      
      std::copy(trialPositions.begin(), trialPositions.end(), molecule.begin());

      system.components[selectedComponent].lambdaGC.setCurrentBin(newBin);

      return {energyDifference, double3(0.0, 1.0, 0.0)};
    };
    return {std::nullopt, double3(0.0, 1.0, 0.0)};
  }
}

