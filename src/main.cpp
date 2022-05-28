// MIT License
// 
// Copyright(c) 2020 Carmelo J. Fernández-Agüera
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this softwareand associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// 
// The above copyright noticeand this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <cmath>
#include <iostream>
#include <cassert>
#include <numbers>

// Math constants
static constexpr auto Pi = std::numbers::pi_v<double>;
static constexpr auto TwoPi = 2 * std::numbers::pi_v<double>;

constexpr double RadFromDeg(double deg)
{
	return deg * (Pi / 180);
}

// Physical constants
double G = 6.67259e-11;

// Solar system constants
// Celestial body masses
double SolarMass = 1.9884e30;
double EarthMass = 5.9722e24;
double MarsMass = 6.4171e23;
double MoonMass = 7.34767309e22;
double SolarGravitationalConstant = 1.32712440018e20;

// Mean distances from the sun

// Other orbital parameters
double EarthAphelion = 1.521e11;
double EarthPerihelion = 1.47095e11;
double EarthSemimajorAxis = 1.49598023e11;
double EarthEccentricity = 0.0167086;
double EarthMeanAnomaly = RadFromDeg(358.617);
double EarthPeriHelArg = RadFromDeg(114.207);

double MarsAphelion = 2.492e11;
double MarsPerihelion = 2.067e11;
double MarsSemimajorAxis = 2.279392e11;
double MarsEccentricity = 0.0934;
double MarsLongAscNode = RadFromDeg(49.558);
double MarsPeriHelArg = RadFromDeg(286.502);

double daysFromSeconds(double seconds)
{
	return seconds / (24 * 3600);
}

class CircularOrbit
{
public:
	CircularOrbit(double radius, double mainBodyMass, double orbiterMass = 0)
		: m_radius(radius)
	{
		m_mu = G * (mainBodyMass + orbiterMass);
	}

	double radius() const
	{
		return m_radius;
	}

	// Linear speed of the orbiting body
	double velocity() const
	{
		return sqrt(m_mu / m_radius);
	}

	// Time to complete a full orbit
	double period() const
	{
		return TwoPi * m_radius* sqrt(m_radius / m_mu);
	}

	double gravitationalConstant() const { return m_mu; }

private:
	double m_mu; // Gravitational constant
	double m_radius;
};

class EllipticalOrbit
{
public:
	EllipticalOrbit() = default;
	EllipticalOrbit(double focalBodyGravitationalParam, double periapsis, double apoapsis)
		: m_periapsis(periapsis)
		, m_apoapsis(apoapsis)
		, m_mu(focalBodyGravitationalParam)
	{
		m_e = (m_apoapsis - m_periapsis) / (m_apoapsis + m_periapsis);
		m_p = m_periapsis * (1 + m_e);
	}

	double radius(double theta) const
	{
		return m_p / (1 + m_e * cos(theta));
	}

	double velocity(double theta) const
	{
		const auto a = semiMajorAxis();
		const auto r = radius(theta);
		return sqrt(2 * m_mu / r - m_mu / a);
	}

	double perihelionVelocity() const
	{
		return sqrt(2 * m_mu * (1 / m_periapsis - 1 / (m_periapsis + m_apoapsis)));
	}

	double aphelionVelocity() const
	{
		return sqrt(2 * m_mu * (1 / m_apoapsis - 1 / (m_periapsis + m_apoapsis)));
	}

	double deltaV(double v0, double v1) const
	{
		const auto deltaVStart = abs(v0 - perihelionVelocity());
		const auto deltaVEnd = abs(v1 - aphelionVelocity());
		return deltaVStart + deltaVEnd;
	}

	double semiMajorAxis() const
	{
		return 0.5 * (m_apoapsis + m_periapsis);
	}

	double period() const
	{
		const auto a = semiMajorAxis();
		return TwoPi * a * sqrt(a / m_mu);
	}

	double transferTime() const
	{
		const auto a = semiMajorAxis();
		return Pi * a * sqrt(a / m_mu);
	}

	double eccentricity() const
	{
		return m_e;
	}

	static double meanRadius(double perihelion, double eccentricity)
	{
		return perihelion * (1 + eccentricity);
	}

private:
	const double m_mu = 1;
	const double m_periapsis = 1;
	const double m_apoapsis = 1;
	double m_e = 1; // Orbital eccentricity
	double m_p = 1; // Orbital parameter
};

EllipticalOrbit HohmannTransfer(const CircularOrbit& startOrbit, const CircularOrbit& destOrbit)
{
	// Make sure the two orbits are compatible
	assert(startOrbit.gravitationalConstant() == destOrbit.gravitationalConstant());
	auto mu = startOrbit.gravitationalConstant();
	return EllipticalOrbit(mu, startOrbit.radius(), destOrbit.radius());
}

int main(int, const char**)
{
	double meanMarsRad = EllipticalOrbit::meanRadius(MarsPerihelion, MarsEccentricity);
	CircularOrbit marsCircularOrbit(meanMarsRad, SolarMass, MarsMass);
	std::cout << "Mars´s mean distance from the sun:" << meanMarsRad << "m\n";
	std::cout << "Mars´s average speed: " << marsCircularOrbit.velocity() << "m/s\n";

	double meanEarthRad = EllipticalOrbit::meanRadius(EarthPerihelion, EarthEccentricity);
	CircularOrbit earthCircularOrbit(meanEarthRad, SolarMass, EarthMass);
	std::cout << "Mars´s mean distance from the sun:" << meanEarthRad << "m\n";
	std::cout << "Earth´s average speed: " << earthCircularOrbit.velocity() << "m/s\n";

	// Compute first approx Earth-Mars Hohmann transfer
	EllipticalOrbit earthMarsTransfer(SolarGravitationalConstant, meanEarthRad, meanMarsRad);
	std::cout << "Earth-mass Hohmann transfer eccentricity: " << earthMarsTransfer.eccentricity() << "\n";
	std::cout << "Hohmann transfer time Earth to Mars: " << daysFromSeconds(earthMarsTransfer.transferTime()) << " days\n";
	auto deltaV = earthMarsTransfer.deltaV(earthCircularOrbit.velocity(), marsCircularOrbit.velocity());
	std::cout << "Required deltaV: " << deltaV << " m/s\n";

	// Suboptimal interception transfer analysis
	std::cout << "Computing Mars to Earth outbound tangent interception ellipses\n";
	const auto marsMeanRadius = EllipticalOrbit::meanRadius(MarsPerihelion, MarsEccentricity);
	const auto earthMeanRadius = EllipticalOrbit::meanRadius(EarthPerihelion, EarthEccentricity);
	const auto radiiRatio = marsMeanRadius / earthMeanRadius;
	std::cout << "Mars/Earth radius ratio = " << radiiRatio << "\n";

	// Compute the delta-v requisites of orbits tangential to earth's orbits, that intersect mars's orbit.
	// This only takes into account the earth orbit to transfer insertion delta-V, but not the transfer to mars orbit.
	// Phi is the angle between earth on departure, and mars on arrival.
	// Assuming circular orbits.
	// The maximun travel phase is Pi, corresponding to a Hohmann transfer.
	// The minimun travel phase would be acos(marsMeanOrbitRadius/earthMeanOrbitRadius), but that would require infinite
	// energy and a straight line trajectory.
	// Instead, we can set a limit to injection DV, travel time or phase.
	const int numSamples = 20;
	const auto earthV = earthCircularOrbit.velocity();
	const auto marsV = marsCircularOrbit.velocity();
	for (int i = 0; i < numSamples; ++i)
	{
		const auto phi = (0.5f*Pi * i) / (numSamples-1) + Pi/2;
		// Interception point
		const auto yi = radiiRatio * sin(phi);
		const auto xi = radiiRatio * cos(phi);

		// Compute eccentricity of interception orbit
		const auto e = (radiiRatio - 1) / (1 + xi);
		// Orbital parameter
		const auto p = (1 + e) * earthMeanRadius;
		const auto h = sqrt(SolarGravitationalConstant * p);
		const auto v0 = h / earthMeanRadius;

		// Compute flight time using Lambert's theorem
		std::cout << "Phi: " << phi << ", deltaV: " << v0 - earthV << "\n";
	}

	return 0;
}