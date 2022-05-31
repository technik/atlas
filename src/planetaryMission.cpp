#include "planetaryMission.h"

MarsOrbiter::MarsOrbiter(TimePoint departureTime)
{
    // --- Compute transfer trajectory first ---
    // As an approximation, this transfer will ignore the planets and their spheres of influence.
    // It also assumes the travel will be a full half orbit, so Earth departure will be at the transfer's
    // perihelion and Mars injection will happen at the transfer's Aphelion
    //
    // Start by computing the earth's argument at departure time
    const auto departureTrueAnomaly = EarthOrbit.TrueAnomaly(departureTime);

    auto startRadius = EarthOrbit.radius(departureTrueAnomaly);
    auto endRadius = MarsOrbit.radius(departureTrueAnomaly + Pi);
    auto argumentOfPeriapsis = EarthOrbit.MeanAnomaly(departureTime) + EarthPeriHelArg + EarthLongitudeOfAscendingNode;

    m_transferOrbit = EllipticalOrbit(G * SolarMass, startRadius, endRadius, 0, argumentOfPeriapsis, 0, EarthMeanLongitude);
}