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
    void update() override
    {
        constexpr int kNumSegments = 1000;
        float x_data[kNumSegments+1];
        float y_data[kNumSegments+1];
        float x_data_mars[kNumSegments + 1];
        float y_data_mars[kNumSegments + 1];

        double meanEarthRad = EllipticalOrbit::meanRadius(EarthPerihelion, EarthEccentricity);
        CircularOrbit earthCircularOrbit(meanEarthRad, SolarMass, EarthMass);

        double meanMarsRad = EllipticalOrbit::meanRadius(MarsPerihelion, MarsEccentricity);
        CircularOrbit marsCircularOrbit(meanMarsRad, SolarMass, MarsMass);

        ImGui::Begin("Orbit control");
        ImGui::SliderFloat("transfer", &transfer, 0, 1);
        ImGui::End();

        EllipticalOrbit transferOrbit(earthCircularOrbit.gravitationalConstant(), meanEarthRad, meanEarthRad + (meanMarsRad - meanEarthRad) * transfer);

        ImGui::Begin("Orbit viewer");   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)

        if (ImPlot::BeginPlot("Orbits", ImVec2(-1, -1), ImPlotFlags_Equal)) {
            ImPlot::SetupAxis(ImAxis_X1, NULL, ImPlotAxisFlags_AuxDefault);
            ImPlot::SetupAxis(ImAxis_Y1, NULL, ImPlotAxisFlags_AuxDefault);
            earthCircularOrbit.plot(x_data, y_data, kNumSegments);
            ImPlot::PlotLine("Earth orbit", x_data, y_data, kNumSegments + 1);
            marsCircularOrbit.plot(x_data, y_data, kNumSegments);
            ImPlot::PlotLine("Mars Orbit", x_data, y_data, kNumSegments + 1);
            transferOrbit.plot(x_data, y_data, kNumSegments, transfer==1? 0.5 : 1);
            ImPlot::PlotLine("Transfer", x_data, y_data, kNumSegments + 1);
            ImPlot::EndPlot();
        }
        ImGui::End();

        //ImPlot::ShowDemoWindow();
    }

private:
    float transfer = 1;
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
