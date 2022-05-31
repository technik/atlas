// Dear ImGui: standalone example application for DirectX 12
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

// Important: to compile on 32-bit systems, the DirectX12 backend requires code to be compiled with '#define ImTextureID ImU64'.
// This is because we need ImTextureID to carry a 64-bit value and by default ImTextureID is defined as void*.
// This define is set in the example .vcxproj file and need to be replicated in your app or by adding it to your imconfig.h file.

#include "imgui.h"
#include "implot.h"
#include <cmath>
#include "app.h"

#include "orbits.h"
#include "planetaryMission.h"

class OrbitViewer : public App
{
public:
    OrbitViewer()
        : m_sun(SolarRadius, SolarMass)
    {
    }

    void update() override
    {
        float x_data[kNumOrbitSegments +1];
        float y_data[kNumOrbitSegments +1];

        if (ImGui::Begin("Orbit control"))
        {
            ImGui::Checkbox("Show Earth", &m_showEarth);
            ImGui::Checkbox("Show Mars", &m_showMars);
            //ImGui::SliderFloat("Transfer eccentricity", &m_transferEccentricity, 0, 1);
            ImGui::SliderFloat("Transfer argument", &m_transferStartArgument, 0, TwoPi);
            ImGui::Text("Eccentricity: %f", m_transferEccentricity);
        }
        ImGui::End();

        if (ImGui::Begin("Orbit viewer"))
        {
            // Plot visualization time
            ImGui::SliderFloat("Time", &m_currentTime, 20.f, 30.f);
            TimePoint vizT = J2000 + duration_cast<seconds>((2030y - 2000y) * ((m_currentTime)/30));

            if (ImPlot::BeginPlot("Orbits", ImVec2(-1, -1), ImPlotFlags_Equal))
            {
                // Set up rigid axes
                ImPlot::SetupAxis(ImAxis_X1, NULL, ImPlotAxisFlags_AuxDefault);
                ImPlot::SetupAxis(ImAxis_Y1, NULL, ImPlotAxisFlags_AuxDefault);

                // Plot base solar system for context
                plotBasePlanets(vizT);

                m_sun.plot(x_data, y_data, kNumObjectSegments);
                ImPlot::SetNextLineStyle(SunColor);
                ImPlot::PlotLine("Sun", x_data, y_data, kNumObjectSegments+1);

                // Plot extra orbits
                MarsOrbiter orbiterMission(vizT);

                orbiterMission.plot(x_data, y_data, kNumOrbitSegments);
                ImPlot::PlotLine("Transfer orbit", x_data, y_data, kNumOrbitSegments + 1);

                ImPlot::EndPlot();
            }
        }
        ImGui::End();

        //ImPlot::ShowDemoWindow();
    }

    void plotBasePlanets(TimePoint vizT)
    {
        float x_data[kNumOrbitSegments + 1];
        float y_data[kNumOrbitSegments + 1];

        if (m_showEarth)
        {
            ImPlot::SetNextLineStyle(EarthColor);
            EarthOrbit.plot(x_data, y_data, kNumOrbitSegments);
            ImPlot::PlotLine("Earth orbit", x_data, y_data, kNumOrbitSegments + 1);

            // Plot Earth's sphere of influence
            auto earthTrueAnomaly = EarthOrbit.TrueAnomaly(vizT);
            auto influenceRadius = sphereOfInfluenceRadius(EarthMass, EarthPerihelion, EarthAphelion);
            auto xPos = EarthOrbit.position(earthTrueAnomaly);

            ImPlot::SetNextLineStyle(EarthColor);
            plotCircle("Earth influence", xPos.x(), xPos.y(), influenceRadius);
        }
        if (m_showMars)
        {
            ImPlot::SetNextLineStyle(MarsColor);
            MarsOrbit.plot(x_data, y_data, kNumOrbitSegments);
            ImPlot::PlotLine("Mars Orbit", x_data, y_data, kNumOrbitSegments + 1);

            // Plot the Mars's sphere of influence
            auto marsTrueAnomaly = MarsOrbit.TrueAnomaly(vizT);
            auto influenceRadius = sphereOfInfluenceRadius(MarsMass, MarsPerihelion, MarsAphelion);
            auto xPos = MarsOrbit.position(marsTrueAnomaly);

            ImPlot::SetNextLineStyle(MarsColor);
            plotCircle("Mars influence", xPos.x(), xPos.y(), influenceRadius);
        }
    }

    static void plotCircle(const char* name, float x0, float y0, float radius)
    {
        float x[kNumObjectSegments + 1];
        float y[kNumObjectSegments + 1];
        for (int i = 0; i < kNumObjectSegments + 1; ++i)
        {
            auto theta = i * TwoPi / kNumObjectSegments;
            x[i] = radius * cos(theta) + x0;
            y[i] = radius * sin(theta) + y0;
        }
        ImPlot::PlotLine(name, x, y, kNumObjectSegments + 1);
    }
private:
    static constexpr auto EarthColor = ImVec4(0.02624122189f, 0.6866853124353135f, 1.f, 1.f);
    static constexpr auto MarsColor = ImVec4(1.f, 86/255.f, 25/255.f, 1.f);
    static constexpr auto SunColor = ImVec4(1.0f, 0.84687323f, 0.0f, 1.f);
    static constexpr int kNumOrbitSegments = 1000;
    static constexpr int kNumObjectSegments = 1000;

    bool m_showMars;
    bool m_showEarth;
    CircularOrbit m_sun; // Use a circular orbit to plot the sun as a circle

    float m_transferStartArgument = 0;
    float m_transferEccentricity = 0;
    float m_currentTime = 22;
};

// Main code
int main(int, char**)
{
    OrbitViewer app;
    if (!app.init())
        return -1;

    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        app.beginFrame();
        app.update();
        app.render();
    }

    app.end();

    return 0;
}
