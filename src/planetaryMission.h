#pragma once
#include "orbits.h"

// Mision parameters:
// - Starts on low earth orbit
// - Assumes all orbits are on the ecliptic plane
// - Assumes mars will be at the intersection point when the orbiter arrives
// - Only the gravitational effect of the strongest influence body has any effect (Patched conic approximation)
// Performs the following maneouvers:
// - Earth escape: Leo to earth escape velocity into an escape (parabolic or hyperbolic) trajectory.
// - Transfer injection: Injection into an elliptical transfer orbit with periapsis on the earth-sun influence sphere
// - Mars capture: transfer to low mars orbit
// - Mars orbit circularization
class MarsOrbiter
{
public:
    MarsOrbiter(TimePoint departureTime);
    double totalDeltaV() const;
    double transitTime() const;

    void plot() const;

private:
    ConicOrbit m_earthEscapeTrajectory;
    EllipticalOrbit m_transferOrbit;
    ConicOrbit m_marsCaptureTrajectory;
};