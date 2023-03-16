#include <gtest/gtest.h>

import double3;

import forcefield;
import component;
import system;
import simulationbox;
import energy_factor;
import units;
TEST(Ewald, Test_2_CO2_in_Box_10_10_10)
{
  ForceField forceField = ForceField(
    { PseudoAtom("Si",    28.0855,   2.05,  14, false),
      PseudoAtom("O",     15.999,   -1.025,  8, false),
      PseudoAtom("CH4",   16.04246,  0.0,    6, false),
      PseudoAtom("C_co2", 12.0,      0.6512, 6, false),
      PseudoAtom("O_co2", 15.9994,  -0.3256, 8, false),
    },
    { VDWParameters(22.0 / 1.2027242847, 2.30),
      VDWParameters(53.0 / 1.2027242847, 3.3),
      VDWParameters(158.5 / 1.2027242847, 3.72),
      VDWParameters(29.933 / 1.2027242847, 2.745),
      VDWParameters(85.671 / 1.2027242847, 3.017)
    },
    ForceField::MixingRule::Lorentz_Berthelot,
    12.0,
    true,
    false);
  Component c = Component(0,
    "CO2",
    43.9988,
    SimulationBox(10.0, 10.0, 10.0),
    304.1282, 7377300.0, 0.22394,
    {
       Atom(double3(0.0, 0.0,  1.149), -0.3256, 1.0, 4, 0, 0),
       Atom(double3(0.0, 0.0,  0.0),    0.6512, 1.0, 3, 0, 0),
       Atom(double3(0.0, 0.0, -1.149), -0.3256, 1.0, 4, 0, 0)
    }, 5);

  System system = System(0, 300.0, 1e4, forceField, { c }, { 2 }, 5);

  std::span<Atom> atomPositions = system.spanOfMoleculeAtoms();
  atomPositions[0].position = double3(-1.0, 0.0,  1.149);
  atomPositions[1].position = double3(-1.0, 0.0,  0.0);
  atomPositions[2].position = double3(-1.0, 0.0, -1.149);
  atomPositions[3].position = double3( 1.0, 0.0,  1.149);
  atomPositions[4].position = double3( 1.0, 0.0,  0.0);
  atomPositions[5].position = double3( 1.0, 0.0, -1.149);

  RunningEnergy energy, rigidenergy;
  //double3 perpendicularWidths = system.simulationBox.perpendicularWidths();
  //system.forceField.initializeEwaldParameters(perpendicularWidths);
  system.forceField.EwaldAlpha = 0.25;
  system.forceField.numberOfWaveVectors = int3(8, 8, 8);
  system.registerEwaldFourierEnergySingleIon(double3(0.0, 0.0, 0.0), 1.0);
  system.computeEwaldFourierRigidEnergy(system.simulationBox, rigidenergy);
  system.computeEwaldFourierEnergy(system.simulationBox, system.atomPositions, energy);

  EXPECT_NEAR((energy.ewald - rigidenergy.ewald) * 1.2027242847, 90.54613836, 1e-6);
}

/*
TEST(Ewald, Test_1_Na_1_Cl_in_Box_10_10_10_Gradient)
{
  ForceField forceField = ForceField(
    { PseudoAtom("Si",    28.0855,   2.05,  14, false),
      PseudoAtom("O",     15.999,   -1.025,  8, false),
      PseudoAtom("CH4",   16.04246,  0.0,    6, false),
      PseudoAtom("Na+", 12.0,      1.0, 6, false),
      PseudoAtom("Cl-", 15.9994,  -1.0, 8, false),
    },
    { VDWParameters(22.0 / 1.2027242847, 2.30),
      VDWParameters(53.0 / 1.2027242847, 3.3),
      VDWParameters(158.5 / 1.2027242847, 3.72),
      VDWParameters(75.0 / 1.2027242847, 1.0),
      VDWParameters(75.0 / 1.2027242847, 1.0)
    },
    ForceField::MixingRule::Lorentz_Berthelot,
    12.0,
    true,
    false);
  Component na = Component(0,
    "Na",
    43.9988,
    SimulationBox(10.0, 10.0, 10.0),
    304.1282, 7377300.0, 0.22394,
    {
       Atom(double3(0.0, 0.0, 0.0), 1.0, 1.0, 3, 0, 0),
    }, 5);
  Component cl = Component(1,
    "Cl",
    43.9988,
    SimulationBox(10.0, 10.0, 10.0),
    304.1282, 7377300.0, 0.22394,
    {
       Atom(double3(0.0, 0.0, 0.0), -1.0, 1.0, 4, 1, 0),
    }, 5);

  System system = System(0, 300.0, 1e4, forceField, { na, cl }, { 1, 1 }, 5);

  std::span<Atom> spanOfMoleculeAtoms = system.spanOfMoleculeAtoms();
  std::vector<Atom> atomPositions = std::vector<Atom>(spanOfMoleculeAtoms.begin(), spanOfMoleculeAtoms.end());

  for (Atom& atom : atomPositions)
  {
    atom.gradient = double3(0.0, 0.0, 0.0);
  }

  

  RunningEnergy energy, rigidenergy;
  //double3 perpendicularWidths = system.simulationBox.perpendicularWidths();
  //system.forceField.initializeEwaldParameters(perpendicularWidths);
  system.forceField.EwaldAlpha = 0.25;
  system.forceField.numberOfWaveVectors = int3(8, 8, 8);
  system.registerEwaldFourierEnergySingleIon(double3(0.0, 0.0, 0.0), 1.0);
  system.computeEwaldFourierRigidEnergy(system.simulationBox, rigidenergy);

  EnergyFactor factor = system.computeEwaldFourierGradient();

  double delta = 1e-5;
  double tolerance = 1e-5;
  double3 gradient;
  for (size_t i = 0; i < atomPositions.size(); ++i)
  {
    RunningEnergy x1, x2, y1, y2, z1, z2;

    // finite difference x
    atomPositions[i].position.x = spanOfMoleculeAtoms[i].position.x + 0.5 * delta;
    system.computeEwaldFourierEnergy(system.simulationBox, std::span<const Atom>(atomPositions), x2);

    atomPositions[i].position.x = spanOfMoleculeAtoms[i].position.x - 0.5 * delta;
    system.computeEwaldFourierEnergy(system.simulationBox, std::span<const Atom>(atomPositions), x1);
    atomPositions[i].position.x = spanOfMoleculeAtoms[i].position.x;

    // finite difference y
    atomPositions[i].position.y = spanOfMoleculeAtoms[i].position.y + 0.5 * delta;
    system.computeEwaldFourierEnergy(system.simulationBox, std::span<const Atom>(atomPositions), y2);

    atomPositions[i].position.y = spanOfMoleculeAtoms[i].position.y - 0.5 * delta;
    system.computeEwaldFourierEnergy(system.simulationBox, std::span<const Atom>(atomPositions), y1);
    atomPositions[i].position.y = spanOfMoleculeAtoms[i].position.y;

    // finite difference z
    atomPositions[i].position.z = spanOfMoleculeAtoms[i].position.z + 0.5 * delta;
    system.computeEwaldFourierEnergy(system.simulationBox, std::span<const Atom>(atomPositions), z2);

    atomPositions[i].position.z = spanOfMoleculeAtoms[i].position.z - 0.5 * delta;
    system.computeEwaldFourierEnergy(system.simulationBox, std::span<const Atom>(atomPositions), z1);
    atomPositions[i].position.z = spanOfMoleculeAtoms[i].position.z;

    gradient.x = (x2.ewald - x1.ewald) / delta;
    gradient.y = (y2.ewald - y1.ewald) / delta;
    gradient.z = (z2.ewald - z1.ewald) / delta;

    EXPECT_NEAR(spanOfMoleculeAtoms[i].gradient.x, gradient.x, tolerance) << "Wrong x-gradient";
    EXPECT_NEAR(spanOfMoleculeAtoms[i].gradient.y, gradient.y, tolerance) << "Wrong y-gradient";
    EXPECT_NEAR(spanOfMoleculeAtoms[i].gradient.z, gradient.z, tolerance) << "Wrong z-gradient";
  }

}*/

TEST(Ewald, Test_2_CO2_in_ITQ_29_1x1x1)
{
  ForceField forceField = ForceField(
    { PseudoAtom("Si",    28.0855,   2.05,  14, false),
      PseudoAtom("O",     15.999,   -1.025,  8, false),
      PseudoAtom("CH4",   16.04246,  0.0,    6, false),
      PseudoAtom("C_co2", 12.0,      0.6512, 6, false),
      PseudoAtom("O_co2", 15.9994,  -0.3256, 8, false),
    },
    { VDWParameters(22.0 / 1.2027242847, 2.30),
      VDWParameters(53.0 / 1.2027242847, 3.3),
      VDWParameters(158.5 / 1.2027242847, 3.72),
      VDWParameters(29.933 / 1.2027242847, 2.745),
      VDWParameters(85.671 / 1.2027242847, 3.017)
    },
    ForceField::MixingRule::Lorentz_Berthelot,
    12.0,
    true,
    false);
  Component f = Component(0, "ITQ-29", 46144.748974602669, SimulationBox(11.8671, 11.8671, 11.8671),
    517,
    {
      Atom(double3(0.3683, 0.1847, 0),       2.05, 1.0, 0, 0, 0),
      Atom(double3(0.5,    0.2179, 0),      -1.025, 1.0, 1, 0, 0),
      Atom(double3(0.2939, 0.2939, 0),      -1.025, 1.0, 1, 0, 0),
      Atom(double3(0.3429, 0.1098, 0.1098), -1.025, 1.0, 1, 0, 0)
    },
    int3(1, 1, 1), 5);
  Component c = Component(1,
    "CO2",
    43.9988,
    SimulationBox(0.0, 0.0, 0.0),
    304.1282, 7377300.0, 0.22394,
    {
       Atom(double3(0.0, 0.0,  1.149), -0.3256, 1.0, 4, 1, 0),
       Atom(double3(0.0, 0.0,  0.0),  0.6512, 1.0, 3, 1, 0),
       Atom(double3(0.0, 0.0, -1.149), -0.3256, 1.0, 4, 1, 0)
    }, 5);

  System system = System(0, 300.0, 1e4, forceField, { f, c }, { 0, 2 }, 5);

  std::span<Atom> atomPositions = system.spanOfMoleculeAtoms();
  std::span<const Atom> frameworkAtoms = system.spanOfFrameworkAtoms();
  atomPositions[0].position = double3(5.93355, 7.93355, 5.93355 + 1.149);
  atomPositions[1].position = double3(5.93355, 7.93355, 5.93355 + 0.0);
  atomPositions[2].position = double3(5.93355, 7.93355, 5.93355 - 1.149);
  atomPositions[3].position = double3(5.93355, 3.93355, 5.93355 + 1.149);
  atomPositions[4].position = double3(5.93355, 3.93355, 5.93355 + 0.0);
  atomPositions[5].position = double3(5.93355, 3.93355, 5.93355 - 1.149);

  RunningEnergy energy, rigidenergy;
  system.forceField.EwaldAlpha = 0.25;
  system.forceField.numberOfWaveVectors = int3(8, 8, 8);
  system.registerEwaldFourierEnergySingleIon(double3(0.0, 0.0, 0.0), 1.0);
  system.computeEwaldFourierRigidEnergy(system.simulationBox, rigidenergy);
  system.computeEwaldFourierEnergy(system.simulationBox, system.atomPositions, energy);

  EXPECT_NEAR(system.CoulombicFourierEnergySingleIon * 1.2027242847, 17464.2371790130, 1e-6);
  EXPECT_NEAR((energy.ewald - rigidenergy.ewald) * 1.2027242847, -702.65863478, 1e-6);
}

TEST(Ewald, Test_2_CO2_in_ITQ_29_2x2x2)
{
  ForceField forceField = ForceField(
    { PseudoAtom("Si",    28.0855,   2.05,  14, false),
      PseudoAtom("O",     15.999,   -1.025,  8, false),
      PseudoAtom("CH4",   16.04246,  0.0,    6, false),
      PseudoAtom("C_co2", 12.0,      0.6512, 6, false),
      PseudoAtom("O_co2", 15.9994,  -0.3256, 8, false),
    },
    { VDWParameters(22.0 / 1.2027242847, 2.30),
      VDWParameters(53.0 / 1.2027242847, 3.3),
      VDWParameters(158.5 / 1.2027242847, 3.72),
      VDWParameters(29.933 / 1.2027242847, 2.745),
      VDWParameters(85.671 / 1.2027242847, 3.017)
    },
    ForceField::MixingRule::Lorentz_Berthelot,
    12.0,
    true,
    false);
  Component f = Component(0, "ITQ-29", 46144.748974602669, SimulationBox(11.8671, 11.8671, 11.8671),
    517,
    {
      Atom(double3(0.3683, 0.1847, 0),       2.05, 1.0, 0, 0, 0),
      Atom(double3(0.5,    0.2179, 0),      -1.025, 1.0, 1, 0, 0),
      Atom(double3(0.2939, 0.2939, 0),      -1.025, 1.0, 1, 0, 0),
      Atom(double3(0.3429, 0.1098, 0.1098), -1.025, 1.0, 1, 0, 0)
    },
    int3(2, 2, 2), 5);
  Component c = Component(1,
    "CO2",
    43.9988,
    SimulationBox(0.0, 0.0, 0.0),
    304.1282, 7377300.0, 0.22394,
    {
       Atom(double3(0.0, 0.0,  1.149), -0.3256, 1.0, 4, 1, 0),
       Atom(double3(0.0, 0.0,  0.0),  0.6512, 1.0, 3, 1, 0),
       Atom(double3(0.0, 0.0, -1.149), -0.3256, 1.0, 4, 1, 0)
    }, 5);

  System system = System(0, 300.0, 1e4, forceField, { f, c }, { 0, 2 }, 5);

  std::span<Atom> atomPositions = system.spanOfMoleculeAtoms();
  std::span<const Atom> frameworkAtoms = system.spanOfFrameworkAtoms();
  atomPositions[0].position = double3(5.93355, 7.93355, 5.93355 + 1.149);
  atomPositions[1].position = double3(5.93355, 7.93355, 5.93355 + 0.0);
  atomPositions[2].position = double3(5.93355, 7.93355, 5.93355 - 1.149);
  atomPositions[3].position = double3(5.93355, 3.93355, 5.93355 + 1.149);
  atomPositions[4].position = double3(5.93355, 3.93355, 5.93355 + 0.0);
  atomPositions[5].position = double3(5.93355, 3.93355, 5.93355 - 1.149);

  RunningEnergy energy, rigidenergy;
  system.forceField.EwaldAlpha = 0.25;
  system.forceField.numberOfWaveVectors = int3(8, 8, 8);
  system.registerEwaldFourierEnergySingleIon(double3(0.0, 0.0, 0.0), 1.0);
  system.computeEwaldFourierRigidEnergy(system.simulationBox, rigidenergy);
  system.computeEwaldFourierEnergy(system.simulationBox, system.atomPositions, energy);

  EXPECT_NEAR(system.CoulombicFourierEnergySingleIon * 1.2027242847, 9673.9032373025, 1e-6);
  EXPECT_NEAR((energy.ewald - rigidenergy.ewald) * 1.2027242847, -721.64644486, 1e-6);
}

TEST(Ewald, Test_2_CO2_in_MFI_1x1x1)
{
  ForceField forceField = ForceField(
    { PseudoAtom("Si",    28.0855,   2.05,  14, false),
      PseudoAtom("O",     15.999,   -1.025,  8, false),
      PseudoAtom("CH4",   16.04246,  0.0,    6, false),
      PseudoAtom("C_co2", 12.0,      0.6512, 6, false),
      PseudoAtom("O_co2", 15.9994,  -0.3256, 8, false),
    },
    { VDWParameters(22.0 / 1.2027242847, 2.30),
      VDWParameters(53.0 / 1.2027242847, 3.3),
      VDWParameters(158.5 / 1.2027242847, 3.72),
      VDWParameters(29.933 / 1.2027242847, 2.745),
      VDWParameters(85.671 / 1.2027242847, 3.017)
    },
    ForceField::MixingRule::Lorentz_Berthelot,
    12.0,
    true,
    false);
  Component f = Component(0, "MFI_SI", 46144.748974602669, SimulationBox(20.022, 19.899, 13.383),
    292,
    {
      Atom(double3(0.42238,  0.0565,  -0.33598), 2.05,  1.0, 0, 0, 0),
      Atom(double3(0.30716,  0.02772, -0.1893),  2.05,  1.0, 0, 0, 0),
      Atom(double3(0.27911,  0.06127,  0.0312),  2.05,  1.0, 0, 0, 0),
      Atom(double3(0.12215,  0.06298,  0.0267),  2.05,  1.0, 0, 0, 0),
      Atom(double3(0.07128,  0.02722, -0.18551), 2.05,  1.0, 0, 0, 0),
      Atom(double3(0.18641,  0.05896, -0.32818), 2.05,  1.0, 0, 0, 0),
      Atom(double3(0.42265, -0.1725,  -0.32718), 2.05,  1.0, 0, 0, 0),
      Atom(double3(0.30778, -0.13016, -0.18548), 2.05,  1.0, 0, 0, 0),
      Atom(double3(0.27554, -0.17279,  0.03109), 2.05,  1.0, 0, 0, 0),
      Atom(double3(0.12058, -0.1731,   0.02979), 2.05,  1.0, 0, 0, 0),
      Atom(double3(0.07044, -0.13037, -0.182),   2.05,  1.0, 0, 0, 0),
      Atom(double3(0.18706, -0.17327, -0.31933), 2.05,  1.0, 0, 0, 0),
      Atom(double3(0.3726,   0.0534,  -0.2442), -1.025, 1.0, 1, 0, 0),
      Atom(double3(0.3084,   0.0587,  -0.0789), -1.025, 1.0, 1, 0, 0),
      Atom(double3(0.2007,   0.0592,   0.0289), -1.025, 1.0, 1, 0, 0),
      Atom(double3(0.0969,   0.0611,  -0.0856), -1.025, 1.0, 1, 0, 0),
      Atom(double3(0.1149,   0.0541,  -0.2763), -1.025, 1.0, 1, 0, 0),
      Atom(double3(0.2435,   0.0553,  -0.246),  -1.025, 1.0, 1, 0, 0),
      Atom(double3(0.3742,  -0.1561,  -0.2372), -1.025, 1.0, 1, 0, 0),
      Atom(double3(0.3085,  -0.1552,  -0.0728), -1.025, 1.0, 1, 0, 0),
      Atom(double3(0.198,   -0.1554,   0.0288), -1.025, 1.0, 1, 0, 0),
      Atom(double3(0.091,   -0.1614,  -0.0777), -1.025, 1.0, 1, 0, 0),
      Atom(double3(0.1169,  -0.1578,  -0.2694), -1.025, 1.0, 1, 0, 0),
      Atom(double3(0.2448,  -0.1594,  -0.2422), -1.025, 1.0, 1, 0, 0),
      Atom(double3(0.3047,  -0.051,   -0.1866), -1.025, 1.0, 1, 0, 0),
      Atom(double3(0.0768,  -0.0519,  -0.1769), -1.025, 1.0, 1, 0, 0),
      Atom(double3(0.4161,   0.1276,  -0.3896), -1.025, 1.0, 1, 0, 0),
      Atom(double3(0.4086,  -0.0017,  -0.4136), -1.025, 1.0, 1, 0, 0),
      Atom(double3(0.402,   -0.1314,  -0.4239), -1.025, 1.0, 1, 0, 0),
      Atom(double3(0.1886,   0.1298,  -0.3836), -1.025, 1.0, 1, 0, 0),
      Atom(double3(0.194,    0.0007,  -0.4082), -1.025, 1.0, 1, 0, 0),
      Atom(double3(0.1951,  -0.1291,  -0.419),  -1.025, 1.0, 1, 0, 0),
      Atom(double3(-0.0037,  0.0502,  -0.208),  -1.025, 1.0, 1, 0, 0),
      Atom(double3(-0.004,  -0.1528,  -0.2078), -1.025, 1.0, 1, 0, 0),
      Atom(double3(0.4192,  -0.25,    -0.354),  -1.025, 1.0, 1, 0, 0),
      Atom(double3(0.1884,  -0.25,    -0.3538), -1.025, 1.0, 1, 0, 0),
      Atom(double3(0.2883,  -0.25,     0.0579), -1.025, 1.0, 1, 0, 0),
      Atom(double3(0.1085,  -0.25,     0.0611), -1.025, 1.0, 1, 0, 0)
    },
    int3(1, 1, 1), 5);
  Component c = Component(1,
    "CO2",
    43.9988,
    SimulationBox(0.0, 0.0, 0.0),
    304.1282, 7377300.0, 0.22394,
    {
       Atom(double3(0.0, 0.0,  1.149), -0.3256, 1.0, 4, 1, 0),
       Atom(double3(0.0, 0.0,  0.0),  0.6512, 1.0, 3, 1, 0),
       Atom(double3(0.0, 0.0, -1.149), -0.3256, 1.0, 4, 1, 0)
    }, 5);

  System system = System(0, 300.0, 1e4, forceField, { f, c }, { 0, 2 }, 5);

  std::span<Atom> atomPositions = system.spanOfMoleculeAtoms();
  std::span<const Atom> frameworkAtoms = system.spanOfFrameworkAtoms();
  atomPositions[0].position = double3(10.011, 4.97475 + 2.0, 1.149);
  atomPositions[1].position = double3(10.011, 4.97475 + 2.0, 0.0);
  atomPositions[2].position = double3(10.011, 4.97475 + 2.0, -1.149);
  atomPositions[3].position = double3(10.011, 4.97475 - 2.0, 1.149);
  atomPositions[4].position = double3(10.011, 4.97475 - 2.0, 0.0);
  atomPositions[5].position = double3(10.011, 4.97475 - 2.0, -1.149);

  RunningEnergy energy, rigidenergy;
  system.forceField.EwaldAlpha = 0.25;
  system.forceField.numberOfWaveVectors = int3(8, 8, 8);
  system.registerEwaldFourierEnergySingleIon(double3(0.0, 0.0, 0.0), 1.0);
  system.computeEwaldFourierRigidEnergy(system.simulationBox, rigidenergy);
  system.computeEwaldFourierEnergy(system.simulationBox, system.atomPositions, energy);

  EXPECT_NEAR(system.CoulombicFourierEnergySingleIon * 1.2027242847, 12028.1731827280, 1e-6);
  EXPECT_NEAR((energy.ewald - rigidenergy.ewald) * 1.2027242847, -1191.77790165, 1e-6);
}

TEST(Ewald, Test_2_CO2_in_MFI_2x2x2)
{
  ForceField forceField = ForceField(
    { PseudoAtom("Si",    28.0855,   2.05,  14, false),

      PseudoAtom("O",     15.999,   -1.025,  8, false),
      PseudoAtom("CH4",   16.04246,  0.0,    6, false),
      PseudoAtom("C_co2", 12.0,      0.6512, 6, false),
      PseudoAtom("O_co2", 15.9994,  -0.3256, 8, false),
    },
    { VDWParameters(22.0 / 1.2027242847, 2.30),
      VDWParameters(53.0 / 1.2027242847, 3.3),
      VDWParameters(158.5 / 1.2027242847, 3.72),
      VDWParameters(29.933 / 1.2027242847, 2.745),
      VDWParameters(85.671 / 1.2027242847, 3.017)
    },
    ForceField::MixingRule::Lorentz_Berthelot,
    12.0,
    true,
    false);
  Component f = Component(0, "MFI_SI", 46144.748974602669, SimulationBox(20.022, 19.899, 13.383),
    292,
    {
      Atom(double3(0.42238,  0.0565,  -0.33598), 2.05,  1.0, 0, 0, 0),
      Atom(double3(0.30716,  0.02772, -0.1893),  2.05,  1.0, 0, 0, 0),
      Atom(double3(0.27911,  0.06127,  0.0312),  2.05,  1.0, 0, 0, 0),
      Atom(double3(0.12215,  0.06298,  0.0267),  2.05,  1.0, 0, 0, 0),
      Atom(double3(0.07128,  0.02722, -0.18551), 2.05,  1.0, 0, 0, 0),
      Atom(double3(0.18641,  0.05896, -0.32818), 2.05,  1.0, 0, 0, 0),
      Atom(double3(0.42265, -0.1725,  -0.32718), 2.05,  1.0, 0, 0, 0),
      Atom(double3(0.30778, -0.13016, -0.18548), 2.05,  1.0, 0, 0, 0),
      Atom(double3(0.27554, -0.17279,  0.03109), 2.05,  1.0, 0, 0, 0),
      Atom(double3(0.12058, -0.1731,   0.02979), 2.05,  1.0, 0, 0, 0),
      Atom(double3(0.07044, -0.13037, -0.182),   2.05,  1.0, 0, 0, 0),
      Atom(double3(0.18706, -0.17327, -0.31933), 2.05,  1.0, 0, 0, 0),
      Atom(double3(0.3726,   0.0534,  -0.2442), -1.025, 1.0, 1, 0, 0),
      Atom(double3(0.3084,   0.0587,  -0.0789), -1.025, 1.0, 1, 0, 0),
      Atom(double3(0.2007,   0.0592,   0.0289), -1.025, 1.0, 1, 0, 0),
      Atom(double3(0.0969,   0.0611,  -0.0856), -1.025, 1.0, 1, 0, 0),
      Atom(double3(0.1149,   0.0541,  -0.2763), -1.025, 1.0, 1, 0, 0),
      Atom(double3(0.2435,   0.0553,  -0.246),  -1.025, 1.0, 1, 0, 0),
      Atom(double3(0.3742,  -0.1561,  -0.2372), -1.025, 1.0, 1, 0, 0),
      Atom(double3(0.3085,  -0.1552,  -0.0728), -1.025, 1.0, 1, 0, 0),
      Atom(double3(0.198,   -0.1554,   0.0288), -1.025, 1.0, 1, 0, 0),
      Atom(double3(0.091,   -0.1614,  -0.0777), -1.025, 1.0, 1, 0, 0),
      Atom(double3(0.1169,  -0.1578,  -0.2694), -1.025, 1.0, 1, 0, 0),
      Atom(double3(0.2448,  -0.1594,  -0.2422), -1.025, 1.0, 1, 0, 0),
      Atom(double3(0.3047,  -0.051,   -0.1866), -1.025, 1.0, 1, 0, 0),
      Atom(double3(0.0768,  -0.0519,  -0.1769), -1.025, 1.0, 1, 0, 0),
      Atom(double3(0.4161,   0.1276,  -0.3896), -1.025, 1.0, 1, 0, 0),
      Atom(double3(0.4086,  -0.0017,  -0.4136), -1.025, 1.0, 1, 0, 0),
      Atom(double3(0.402,   -0.1314,  -0.4239), -1.025, 1.0, 1, 0, 0),
      Atom(double3(0.1886,   0.1298,  -0.3836), -1.025, 1.0, 1, 0, 0),
      Atom(double3(0.194,    0.0007,  -0.4082), -1.025, 1.0, 1, 0, 0),
      Atom(double3(0.1951,  -0.1291,  -0.419),  -1.025, 1.0, 1, 0, 0),
      Atom(double3(-0.0037,  0.0502,  -0.208),  -1.025, 1.0, 1, 0, 0),
      Atom(double3(-0.004,  -0.1528,  -0.2078), -1.025, 1.0, 1, 0, 0),
      Atom(double3(0.4192,  -0.25,    -0.354),  -1.025, 1.0, 1, 0, 0),
      Atom(double3(0.1884,  -0.25,    -0.3538), -1.025, 1.0, 1, 0, 0),
      Atom(double3(0.2883,  -0.25,     0.0579), -1.025, 1.0, 1, 0, 0),
      Atom(double3(0.1085,  -0.25,     0.0611), -1.025, 1.0, 1, 0, 0)
    },
    int3(2, 2, 2), 5);
  Component c = Component(1,
    "CO2",
    43.9988,
    SimulationBox(0.0, 0.0, 0.0),
    304.1282, 7377300.0, 0.22394,
    {
       Atom(double3(0.0, 0.0,  1.149), -0.3256, 1.0, 4, 1, 0),
       Atom(double3(0.0, 0.0,  0.0),  0.6512, 1.0, 3, 1, 0),
       Atom(double3(0.0, 0.0, -1.149), -0.3256, 1.0, 4, 1, 0)
    }, 5);

  System system = System(0, 300.0, 1e4, forceField, { f, c }, { 0, 2 }, 5);

  std::span<Atom> atomPositions = system.spanOfMoleculeAtoms();
  std::span<const Atom> frameworkAtoms = system.spanOfFrameworkAtoms();
  atomPositions[0].position = double3(10.011, 4.97475 + 2.0, 1.149);
  atomPositions[1].position = double3(10.011, 4.97475 + 2.0, 0.0);
  atomPositions[2].position = double3(10.011, 4.97475 + 2.0, -1.149);
  atomPositions[3].position = double3(10.011, 4.97475 - 2.0, 1.149);
  atomPositions[4].position = double3(10.011, 4.97475 - 2.0, 0.0);
  atomPositions[5].position = double3(10.011, 4.97475 - 2.0, -1.149);

  RunningEnergy energy, rigidenergy;
  system.forceField.EwaldAlpha = 0.25;
  system.forceField.numberOfWaveVectors = int3(8, 8, 8);
  system.registerEwaldFourierEnergySingleIon(double3(0.0, 0.0, 0.0), 1.0);
  system.computeEwaldFourierRigidEnergy(system.simulationBox, rigidenergy);
  system.computeEwaldFourierEnergy(system.simulationBox, system.atomPositions, energy);

  EXPECT_NEAR(system.CoulombicFourierEnergySingleIon * 1.2027242847, 6309.7866899037, 1e-6);
  EXPECT_NEAR((energy.ewald - rigidenergy.ewald) * 1.2027242847, -1197.23909965, 1e-6);
}


TEST(Ewald, Test_20_Na_Cl_in_Box_25x25x25)
{
  ForceField forceField = ForceField(
    { PseudoAtom("Si",   28.0855,   2.05,  14, false),
      PseudoAtom("O",    15.999,   -1.025,  8, false),
      PseudoAtom("CH4",  16.04246,  0.0,    6, false),
      PseudoAtom("Na+",  12.0,      0.0, 6, false),
      PseudoAtom("Cl-",  15.9994,   0.0, 8, false),
    },
    { VDWParameters(22.0 / 1.2027242847, 2.30),
      VDWParameters(53.0 / 1.2027242847, 3.3),
      VDWParameters(158.5 / 1.2027242847, 3.72),
      VDWParameters(15.0966 / 1.2027242847, 2.65755),
      VDWParameters(142.562 / 1.2027242847, 3.51932)
    },
    ForceField::MixingRule::Lorentz_Berthelot,
    12.0,
    true,
    false);
  Component na = Component(0,
    "Na",
    43.9988,
    SimulationBox(25.0, 25.0, 25.0),
    304.1282, 7377300.0, 0.22394,
    {
       Atom(double3(0.0, 0.0, 0.0), 0.0, 1.0, 3, 0, 0),
    }, 5);
  Component cl = Component(1,
    "Cl",
    43.9988,
    SimulationBox(25.0, 25.0, 25.0),
    304.1282, 7377300.0, 0.22394,
    {
       Atom(double3(0.0, 0.0, 0.0), 0.0, 1.0, 4, 1, 0),
    }, 5);

  System system = System(0, 300.0, 1e4, forceField, { na, cl }, { 20, 20 }, 5);

  //std::fill(system.forceField.data.begin(), system.forceField.data.end(), VDWParameters(0.0, 1.0));

  std::span<Atom> spanOfMoleculeAtoms = system.spanOfMoleculeAtoms();
  std::vector<Atom> atomPositions = std::vector<Atom>(spanOfMoleculeAtoms.begin(), spanOfMoleculeAtoms.end());

  for (Atom& atom : atomPositions)
  {
    atom.gradient = double3(0.0, 0.0, 0.0);
  }

  for (size_t i = 0; i < 20; ++i)
  {
    atomPositions[i].charge = 1.0;
    system.atomPositions[i].charge = 1.0;
  }
  for (size_t i = 0; i < 20; ++i)
  {
    atomPositions[i + 20].charge = -1.0;
    system.atomPositions[i + 20].charge = -1.0;
  }

  for (Atom& atom : atomPositions)
  {
    atom.gradient = double3(0.0, 0.0, 0.0);
  }



  RunningEnergy energy, rigidenergy;
  //double3 perpendicularWidths = system.simulationBox.perpendicularWidths();
  //system.forceField.initializeEwaldParameters(perpendicularWidths);
  system.forceField.EwaldAlpha = 0.25;
  system.forceField.numberOfWaveVectors = int3(8, 8, 8);
  system.registerEwaldFourierEnergySingleIon(double3(0.0, 0.0, 0.0), 1.0);
  system.computeEwaldFourierRigidEnergy(system.simulationBox, rigidenergy);

  EnergyFactor factor = system.computeEwaldFourierGradient();

  double delta = 1e-4;
  double tolerance = 1e-4;
  double3 gradient;
  for (size_t i = 0; i < atomPositions.size(); ++i)
  {
    RunningEnergy forward1_x, forward2_x, backward1_x, backward2_x;
    RunningEnergy forward1_y, forward2_y, backward1_y, backward2_y;
    RunningEnergy forward1_z, forward2_z, backward1_z, backward2_z;

    // finite difference x
    atomPositions[i].position.x = spanOfMoleculeAtoms[i].position.x + 1.0 * delta;
    system.computeEwaldFourierEnergy(system.simulationBox, std::span<const Atom>(atomPositions), forward2_x);
    atomPositions[i].position.x = spanOfMoleculeAtoms[i].position.x + 0.5 * delta;
    system.computeEwaldFourierEnergy(system.simulationBox, std::span<const Atom>(atomPositions), forward1_x);
    atomPositions[i].position.x = spanOfMoleculeAtoms[i].position.x - 0.5 * delta;
    system.computeEwaldFourierEnergy(system.simulationBox, std::span<const Atom>(atomPositions), backward1_x);
    atomPositions[i].position.x = spanOfMoleculeAtoms[i].position.x - 1.0 * delta;
    system.computeEwaldFourierEnergy(system.simulationBox, std::span<const Atom>(atomPositions), backward2_x);
    atomPositions[i].position.x = spanOfMoleculeAtoms[i].position.x;

    // finite difference y
    atomPositions[i].position.y = spanOfMoleculeAtoms[i].position.y + 1.0 * delta;
    system.computeEwaldFourierEnergy(system.simulationBox, std::span<const Atom>(atomPositions), forward2_y);
    atomPositions[i].position.y = spanOfMoleculeAtoms[i].position.y + 0.5 * delta;
    system.computeEwaldFourierEnergy(system.simulationBox, std::span<const Atom>(atomPositions), forward1_y);
    atomPositions[i].position.y = spanOfMoleculeAtoms[i].position.y - 0.5 * delta;
    system.computeEwaldFourierEnergy(system.simulationBox, std::span<const Atom>(atomPositions), backward1_y);
    atomPositions[i].position.y = spanOfMoleculeAtoms[i].position.y - 1.0 * delta;
    system.computeEwaldFourierEnergy(system.simulationBox, std::span<const Atom>(atomPositions), backward2_y);
    atomPositions[i].position.y = spanOfMoleculeAtoms[i].position.y;

    // finite difference z
    atomPositions[i].position.z = spanOfMoleculeAtoms[i].position.z + 1.0 * delta;
    system.computeEwaldFourierEnergy(system.simulationBox, std::span<const Atom>(atomPositions), forward2_z);
    atomPositions[i].position.z = spanOfMoleculeAtoms[i].position.z + 0.5 * delta;
    system.computeEwaldFourierEnergy(system.simulationBox, std::span<const Atom>(atomPositions), forward1_z);
    atomPositions[i].position.z = spanOfMoleculeAtoms[i].position.z - 0.5 * delta;
    system.computeEwaldFourierEnergy(system.simulationBox, std::span<const Atom>(atomPositions), backward1_z);
    atomPositions[i].position.z = spanOfMoleculeAtoms[i].position.z - 1.0 * delta;
    system.computeEwaldFourierEnergy(system.simulationBox, std::span<const Atom>(atomPositions), backward2_z);
    atomPositions[i].position.z = spanOfMoleculeAtoms[i].position.z;

    gradient.x = (-forward2_x.ewald + 8.0 * forward1_x.ewald - 8.0 * backward1_x.ewald + backward2_x.ewald) / (6.0 * delta);
    gradient.y = (-forward2_y.ewald + 8.0 * forward1_y.ewald - 8.0 * backward1_y.ewald + backward2_y.ewald) / (6.0 * delta);
    gradient.z = (-forward2_z.ewald + 8.0 * forward1_z.ewald - 8.0 * backward1_z.ewald + backward2_z.ewald) / (6.0 * delta);

    EXPECT_NEAR(spanOfMoleculeAtoms[i].gradient.x, gradient.x, tolerance) << "Wrong x-gradient";
    EXPECT_NEAR(spanOfMoleculeAtoms[i].gradient.y, gradient.y, tolerance) << "Wrong y-gradient";
    EXPECT_NEAR(spanOfMoleculeAtoms[i].gradient.z, gradient.z, tolerance) << "Wrong z-gradient";
  }
}

TEST(Ewald, Test_20_Na_Cl_in_Box_25x25x25_strain_derivative)
{
  double delta = 1e-4;
  double tolerance = 1e-3;

  ForceField forceField = ForceField(
    { PseudoAtom("Si",   28.0855,   2.05,  14, false),
      PseudoAtom("O",    15.999,   -1.025,  8, false),
      PseudoAtom("CH4",  16.04246,  0.0,    6, false),
      PseudoAtom("Na+",  12.0,      0.0, 6, false),
      PseudoAtom("Cl-",  15.9994,   0.0, 8, false),
    },
    { VDWParameters(22.0 / 1.2027242847, 2.30),
      VDWParameters(53.0 / 1.2027242847, 3.3),
      VDWParameters(158.5 / 1.2027242847, 3.72),
      VDWParameters(15.0966 / 1.2027242847, 2.65755),
      VDWParameters(142.562 / 1.2027242847, 3.51932)
    },
    ForceField::MixingRule::Lorentz_Berthelot,
    12.0,
    true,
    false);
  Component na = Component(0,
    "Na",
    43.9988,
    SimulationBox(25.0, 25.0, 25.0),
    304.1282, 7377300.0, 0.22394,
    {
       Atom(double3(0.0, 0.0, 0.0), 0.0, 1.0, 3, 0, 0),
    }, 5);
  Component cl = Component(1,
    "Cl",
    43.9988,
    SimulationBox(25.0, 25.0, 25.0),
    304.1282, 7377300.0, 0.22394,
    {
       Atom(double3(0.0, 0.0, 0.0), 0.0, 1.0, 4, 1, 0),
    }, 5);

  System system = System(0, 300.0, 1e4, forceField, { na, cl }, { 20, 20 }, 5);

  //std::fill(system.forceField.data.begin(), system.forceField.data.end(), VDWParameters(0.0, 1.0));

  std::span<Atom> moleculeAtomPositions = system.spanOfMoleculeAtoms();

  for (Atom& atom : moleculeAtomPositions)
  {
    atom.gradient = double3(0.0, 0.0, 0.0);
  }

  for (size_t i = 0; i < 20; ++i)
  {
    moleculeAtomPositions[i].charge = 1.0;
    system.atomPositions[i].charge = 1.0;
  }
  for (size_t i = 0; i < 20; ++i)
  {
    moleculeAtomPositions[i + 20].charge = -1.0;
    system.atomPositions[i + 20].charge = -1.0;
  }

  for (Atom& atom : moleculeAtomPositions)
  {
    atom.gradient = double3(0.0, 0.0, 0.0);
  }

  RunningEnergy energy, rigidenergy;
  //double3 perpendicularWidths = system.simulationBox.perpendicularWidths();
  //system.forceField.initializeEwaldParameters(perpendicularWidths);
  system.forceField.EwaldAlpha = 0.25;
  system.forceField.numberOfWaveVectors = int3(8, 8, 8);
  system.registerEwaldFourierEnergySingleIon(double3(0.0, 0.0, 0.0), 1.0);
  system.computeEwaldFourierRigidEnergy(system.simulationBox, rigidenergy);

  std::pair<EnergyStatus, double3x3> pressureInfo = system.computeEwaldFourierEnergyStrainDerivative();

  std::vector<std::pair<double3x3, double>> strains{
     std::pair{double3x3{double3{delta, 0.0, 0.0}, double3{0.0, 0.0, 0.0}, double3{0.0, 0.0, 0.0} }, pressureInfo.second.ax},
     std::pair{double3x3{double3{0.0, delta, 0.0}, double3{0.0, 0.0, 0.0}, double3{0.0, 0.0, 0.0} }, pressureInfo.second.bx},
     std::pair{double3x3{double3{0.0, 0.0, delta}, double3{0.0, 0.0, 0.0}, double3{0.0, 0.0, 0.0} }, pressureInfo.second.cx},
     std::pair{double3x3{double3{0.0, 0.0, 0.0}, double3{delta, 0.0, 0.0}, double3{0.0, 0.0, 0.0} }, pressureInfo.second.ay},
     std::pair{double3x3{double3{0.0, 0.0, 0.0}, double3{0.0, delta, 0.0}, double3{0.0, 0.0, 0.0} }, pressureInfo.second.by},
     std::pair{double3x3{double3{0.0, 0.0, 0.0}, double3{0.0, 0.0, delta}, double3{0.0, 0.0, 0.0} }, pressureInfo.second.cy},
     std::pair{double3x3{double3{0.0, 0.0, 0.0}, double3{0.0, 0.0, 0.0}, double3{delta, 0.0, 0.0} }, pressureInfo.second.az},
     std::pair{double3x3{double3{0.0, 0.0, 0.0}, double3{0.0, 0.0, 0.0}, double3{0.0, delta, 0.0} }, pressureInfo.second.bz},
     std::pair{double3x3{double3{0.0, 0.0, 0.0}, double3{0.0, 0.0, 0.0}, double3{0.0, 0.0, delta} }, pressureInfo.second.cz}
  };

  double3x3 inv = system.simulationBox.inverseUnitCell;
  double3x3 identity{ double3{1.0, 0.0, 0.0}, double3{0.0, 1.0, 0.0}, double3{0.0, 0.0, 1.0} };

  for (const std::pair<double3x3, double> strain : strains)
  {
    SimulationBox strainBox_forward2 = SimulationBox((identity + strain.first) * system.simulationBox.unitCell, SimulationBox::Type::Triclinic);
    std::vector<Atom> moleculeAtomPositions_forward2{};
    std::transform(moleculeAtomPositions.begin(), moleculeAtomPositions.end(), std::back_inserter(moleculeAtomPositions_forward2),
      [&strainBox_forward2, &inv](const Atom& m) { return Atom(strainBox_forward2.unitCell * (inv * m.position), m.charge, 1.0, m.type, m.componentId, m.moleculeId); });
    RunningEnergy EnergyForward2;
    system.computeEwaldFourierEnergy(strainBox_forward2, moleculeAtomPositions_forward2, EnergyForward2);

    SimulationBox strainBox_forward1 = SimulationBox((identity + 0.5 * strain.first) * system.simulationBox.unitCell, SimulationBox::Type::Triclinic);
    std::vector<Atom> moleculeAtomPositions_forward1{};
    std::transform(moleculeAtomPositions.begin(), moleculeAtomPositions.end(), std::back_inserter(moleculeAtomPositions_forward1),
      [&strainBox_forward1, &inv](const Atom& m) { return Atom(strainBox_forward1.unitCell * (inv * m.position), m.charge, 1.0, m.type, m.componentId, m.moleculeId); });
    RunningEnergy EnergyForward1;
    system.computeEwaldFourierEnergy(strainBox_forward1, moleculeAtomPositions_forward1, EnergyForward1);


    SimulationBox strainBox_backward1 = SimulationBox((identity - 0.5 * strain.first) * system.simulationBox.unitCell, SimulationBox::Type::Triclinic);
    std::vector<Atom> moleculeAtomPositions_backward1{};
    std::transform(moleculeAtomPositions.begin(), moleculeAtomPositions.end(), std::back_inserter(moleculeAtomPositions_backward1),
      [&strainBox_backward1, &inv](const Atom& m) { return Atom(strainBox_backward1.unitCell * (inv * m.position), m.charge, 1.0, m.type, m.componentId, m.moleculeId); });
    RunningEnergy EnergyBackward1;
    system.computeEwaldFourierEnergy(strainBox_backward1, moleculeAtomPositions_backward1, EnergyBackward1);

    SimulationBox strainBox_backward2 = SimulationBox((identity - strain.first) * system.simulationBox.unitCell, SimulationBox::Type::Triclinic);
    std::vector<Atom> moleculeAtomPositions_backward2{};
    std::transform(moleculeAtomPositions.begin(), moleculeAtomPositions.end(), std::back_inserter(moleculeAtomPositions_backward2),
      [&strainBox_backward2, &inv](const Atom& m) { return Atom(strainBox_backward2.unitCell * (inv * m.position), m.charge, 1.0, m.type, m.componentId, m.moleculeId); });
    RunningEnergy EnergyBackward2;
    system.computeEwaldFourierEnergy(strainBox_backward2, moleculeAtomPositions_backward2, EnergyBackward2);

    double strainDerivativeApproximation = (-EnergyForward2.total() + 8.0 * EnergyForward1.total() - 8.0 * EnergyBackward1.total() + EnergyBackward2.total()) / (6.0 * delta);

    EXPECT_NEAR(strainDerivativeApproximation, strain.second, tolerance) << "Wrong strainDerivative";
  }
}