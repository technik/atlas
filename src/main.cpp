// MIT License
// 
// Copyright(c) 2020-2022 Carmelo J. Fernández-Agüera
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
#include <Windows.h>

// Windows Runtime Library. Needed for Microsoft::WRL::ComPtr<> template class.
#include <wrl.h>
namespace WRL = Microsoft::WRL;

// DirectX 12 specific headers.
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

// D3D12 extension library.
#include <d3dx12.h>

// Viewer projects
#include "BaseApp.h"
#include "cmdLineParser.h"
#include "gfx/dxHelpers.h"
#include "gfx/renderer.h"
#include "gfx/GPUAllocator.h"

#include <cmath>
#include <iostream>
#include <cassert>
#include <numbers>

#include <imgui.h>
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx12.h"

static ID3D12DescriptorHeap* g_pd3dSrvDescHeap = NULL;

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

int plotMission(int, const char**)
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

BaseApp* gApp = nullptr;

// Window callback function.
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

class ModelViewer : public BaseApp
{
public:
    template<class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

    void parseCommandLineArguments(int argc, const char** argv)
    {
        CmdLineParser argParser;
        argParser.addOption("w", &m_ClientWidth);
        argParser.addOption("h", &m_ClientHeight);
        argParser.addFlag("fullScreen", m_fullScreen);
        argParser.parse(argc, argv);
    }

    bool init(int argc, const char** argv)
    {
        parseCommandLineArguments(argc, argv);

        if (!__super::init())
            return false;

        m_renderer = std::make_unique<gfx::Renderer>(m_ClientWidth, m_ClientHeight);

		// Init Imgui
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		m_imguiIO = &ImGui::GetIO();

		// Setup Platform/Renderer backends
		ImGui_ImplWin32_Init(window()->m_hWnd);
		auto ctxt = gfx::RenderContext();

		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		desc.NumDescriptors = 1;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		if (ctxt->m_dx12Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3dSrvDescHeap)) != S_OK)
			return false;

		constexpr int NUM_FRAMES_IN_FLIGHT = 2;
		ImGui_ImplDX12_Init(ctxt->m_dx12Device.Get(), NUM_FRAMES_IN_FLIGHT,
			DXGI_FORMAT_R8G8B8A8_UNORM,
			g_pd3dSrvDescHeap,
			g_pd3dSrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
			g_pd3dSrvDescHeap->GetGPUDescriptorHandleForHeapStart());

        return true;
    }

    void resize(uint32_t width, uint32_t height)
    {
        BaseApp::resize(width, height);
        m_renderer->OnWindowResize(width, height);
    }

    bool update() override
    {
        if (!pollWindowMessages())
            return false;

        static uint64_t frameCount = 0;
        static double totalTime = 0.0;

        constexpr float dt = 1.f / 60;
        totalTime += dt;
        frameCount++;

        if (totalTime > 1.0)
        {
            double fps = frameCount / totalTime;

            char buffer[512];
            sprintf_s(buffer, "FPS: %f\n", fps);
            OutputDebugStringA(buffer);

            frameCount = 0;
            totalTime = 0.0;
        }

        return true;
    }

    void render()
    {
        auto backBuffer = swapChain()->backBuffer().Get();
        auto colorRTV = swapChain()->backBufferView();

        // Update the MVP matrix
        m_renderer->BeginRender(backBuffer,
            colorRTV,
            swapChain()->backBufferFence());


		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		// Present
		bool show = true;
		ImGui::ShowDemoWindow(&show);
		// Create a window called "My First Tool", with a menu bar.
		ImGui::Begin("My First Tool");
		ImGui::End();


		ImGui::Render();

		// Actual Imgui render
		auto& gfxQueue = gfx::RenderContext()->graphicsQueue();
		auto cmd = gfxQueue.getNewCommandList();
		auto dx12Cmd = cmd->m_dx12CommandList;
		cmd->m_dx12CommandList->SetDescriptorHeaps(1, &g_pd3dSrvDescHeap);
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), dx12Cmd.Get());
		dx12Cmd->Close();
		gfxQueue.submitCommandList(cmd);

		m_renderer->EndRender(backBuffer);

        // Present
        // The swap chain has internal access to the graphics queue, and will signal the queue right after presenting.
        swapChain()->Present();
    }

private:
    // Scene data
	std::unique_ptr<gfx::Renderer> m_renderer;

	ImGuiIO* m_imguiIO = {};
};

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (gApp)
    {
        if (!gApp->processWindowMessage(hwnd, message, wParam, lParam))
        {
            return ::DefWindowProc(hwnd, message, wParam, lParam);
        }
    }
    else
    {
        return ::DefWindowProc(hwnd, message, wParam, lParam);
    }

    return 0;
}

int main(int argc, const char** argv)
{
    ModelViewer app;

    if (!app.init(argc, argv))
        return -1;

    gApp = &app;

    for (;;)
    {
        if (!app.update())
        {
            break;
        }

        app.render();
    }

    app.end();

    return 0;
}