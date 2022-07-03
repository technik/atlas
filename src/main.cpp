// Molecular dynamics playground

#include "imgui.h"
#include "implot.h"
#include <cmath>
#include "app.h"
#include <math/vector.h>
#include <numbers>
#include <random>

static constexpr auto Pi = std::numbers::pi_v<double>;
static constexpr auto TwoPi = 2 * std::numbers::pi_v<double>;

using namespace math;

class ArgonSimulation : public App
{
public:
    bool show = true;
    bool freeze = true;
    ArgonSimulation()
    {
        // Init the simulation pool
        scatterParticles();
    }

    void update() override
    {
        // Update simulation
        step();
        // Plot state
        if(ImGui::Begin("particles"))
        {
            if (ImGui::Button("Scatter"))
            {
                scatterParticles();
            }
            ImGui::Checkbox("Freeze", &freeze);
            drawParticles(m_particles);
        }
        ImGui::End();
    }

private:
    struct AtomBuffer
    {
        AtomBuffer()
        {
            pos = new Vec3d[N];
            vel = new Vec3d[N];
            acc = new Vec3d[N];

            for(int i = 0; i < N; ++i)
            {
                vel[i] = Vec3d{0,0,0};
                acc[i] = Vec3d{0,0,0};
            }
        }
        ~AtomBuffer()
        {
            delete[] pos;
            delete[] vel;
            delete[] acc;
        }

        Vec3d* pos = nullptr;
        Vec3d* vel = nullptr;
        Vec3d* acc = nullptr;

        static constexpr auto N = 15;
    } m_particles;

    static constexpr double BoxSize = 10.f;

    void scatterParticles()
    {
        for(int i = 0; i < AtomBuffer::N; ++i)
        {
            m_particles.pos[i] = noise3d();
        }
    }

    void step()
    {
        constexpr auto h = 5e-3; // Dimensionless time step
        // Update positions
        if(!freeze)
            updatePositions(h);
        // Compute accelerations
        computeAccelerations();
        // Update speeds
        updateSpeeds(h);
    }

    void drawParticles(AtomBuffer& particles)
    {
        ImPlot::BeginPlot("Simulation", ImVec2(-1, -1), ImPlotFlags_Equal);
        ImPlot::PlotScatter("Atoms", &particles.pos[0].x(), &particles.pos[0].y(), AtomBuffer::N, 0, sizeof(Vec3d));
        ImPlot::EndPlot();
    }

    void updatePositions(double h)
    {
        for(int i = 0; i < AtomBuffer::N; ++i)
        {
            auto& pos = m_particles.pos[i];
            pos += (m_particles.vel[i] + 0.5 * m_particles.acc[i] * h) * h;
            constexpr double halfBoxSize = BoxSize / 2;
            // Keep it in the box
            Vec3d normPos = (pos + halfBoxSize) / BoxSize;
            Vec3d delta = Vec3d(std::floor(normPos.x()), std::floor(normPos.y()), std::floor(normPos.z()));
            pos = (normPos - delta) * BoxSize - halfBoxSize;
        }
    }

    void computeAccelerations()
    {
        // Clear previous accelerations
        for(int i = 0; i < AtomBuffer::N; ++i)
        {
            m_particles.acc[i] = Vec3d{0,0,0};
        }

        // Iterate over every particle pair
        // TODO:
        // Add periodic boundary conditions
        // Add cut off radius
        for(int i = 0; i < AtomBuffer::N; ++i)
        {
            for(int j = 0; j < i; ++j)
            {
                // Compute the force exerted by j into i
                //for(int k = 0; k < 27; ++k)
                //{ }
                auto xij = m_particles.pos[j] - m_particles.pos[i];
                auto inv_rij2 = 1/dot(xij, xij);
                auto inv_rij6 = inv_rij2 * inv_rij2 * inv_rij2;
                auto inv_rij12 = inv_rij6 * inv_rij6;
                auto f = 4*(12*inv_rij12 - 6*inv_rij6)*inv_rij2*xij;
                // Using adimensional units, f=a for the particles because m=1;
                m_particles.acc[i] -= f;
                m_particles.acc[j] += f;
            }
        }
    }

    void updateSpeeds(double h)
    {
        for(int i = 0; i < AtomBuffer::N; ++i)
        {
            m_particles.vel[i] += h*m_particles.acc[i];
        }
    }

    static int squirrelNoise(int position, int seed = 0)
    {
        constexpr unsigned int BIT_NOISE1 = 0xB5297A4D;
        constexpr unsigned int BIT_NOISE2 = 0x68E31DA4;
        constexpr unsigned int BIT_NOISE3 = 0x1B56C4E9;

        int mangled = position;
        mangled *= BIT_NOISE1;
        mangled += seed;
        mangled ^= (mangled >> 8);
        mangled *= BIT_NOISE2;
        mangled ^= (mangled << 8);
        mangled *= BIT_NOISE3;
        mangled ^= (mangled >> 8);
        return mangled;
    }

    struct squirrelRng
    {
        int rand()
        {
            return squirrelNoise(state++);
        }

        int state = 0;
    } m_rng;

    Vec3d noise3d()
    {
        Vec3d result;
        auto k = 0xffffff; // 2e24
        auto r = m_rng.rand();
        result.x() = double(r%k)/k*BoxSize-(BoxSize/2);
        result.y() = double(m_rng.rand()%k)/k*BoxSize-(BoxSize/2);
        result.z() = double(m_rng.rand()%k)/k*BoxSize-(BoxSize/2);
        return result;
    }
};

// Main code
int main(int, char**)
{
    ArgonSimulation app;
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
