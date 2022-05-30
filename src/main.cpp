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

class OrbitViewer : public App
{
public:
    OrbitViewer()
        : m_sun(SolarRadius, SolarMass)
        , m_marsOrbit(SolarGravitationalConstant, MarsPerihelion, MarsAphelion, MarsPeriHelArg)
        , m_earthOrbit(SolarGravitationalConstant, EarthPerihelion, EarthAphelion, EarthPeriHelArg)
    {
    }
    void update() override
    {
        constexpr int kNumSegments = 1000;
        float x_data[kNumSegments+1];
        float y_data[kNumSegments+1];

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
            if (ImPlot::BeginPlot("Orbits", ImVec2(-1, -1), ImPlotFlags_Equal))
            {
                // Set up rigid axes
                ImPlot::SetupAxis(ImAxis_X1, NULL, ImPlotAxisFlags_AuxDefault);
                ImPlot::SetupAxis(ImAxis_Y1, NULL, ImPlotAxisFlags_AuxDefault);

                // Plot base solar system for context
                plotBasePlanets();

                m_sun.plot(x_data, y_data, 20);
                ImPlot::SetNextLineStyle(ImVec4(1, 1, 0, 1));
                ImPlot::PlotLine("Sun", x_data, y_data, 21);

                // Plot extra orbits
                auto startRadius = m_earthOrbit.radius(m_transferStartArgument);
                auto endRadius = m_marsOrbit.radius(m_transferStartArgument + Pi);
                auto transferOrbit = EllipticalOrbit(SolarGravitationalConstant, startRadius, endRadius, m_transferStartArgument);
                m_transferEccentricity = transferOrbit.eccentricity();

                transferOrbit.plot(x_data, y_data, kNumSegments, 0, 1);
                ImPlot::PlotLine("Transfer orbit", x_data, y_data, kNumSegments + 1);

                ImPlot::EndPlot();
            }
        }
        ImGui::End();

        //ImPlot::ShowDemoWindow();
    }

    void plotBasePlanets()
    {
        constexpr int kNumSegments = 1000;
        float x_data[kNumSegments + 1];
        float y_data[kNumSegments + 1];

        if (m_showEarth)
        {
            m_earthOrbit.plot(x_data, y_data, kNumSegments);
            ImPlot::PlotLine("Earth orbit", x_data, y_data, kNumSegments + 1);
        }
        if (m_showMars)
        {
            m_marsOrbit.plot(x_data, y_data, kNumSegments);
            ImPlot::PlotLine("Mars Orbit", x_data, y_data, kNumSegments + 1);
        }
    }

private:
    bool m_showMars;
    bool m_showEarth;
    CircularOrbit m_sun; // Use a circular orbit to plot the sun as a circle
    EllipticalOrbit m_marsOrbit;
    EllipticalOrbit m_earthOrbit;

    float m_transferStartArgument = 0;
    float m_transferEccentricity = 0;
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
