#pragma once

#include "core/Module.h"
#include "raylib.h"

class ModuleIntro : public Module
{
public:
    ModuleIntro(Application* app, bool start_enabled = true);
    ~ModuleIntro();

    bool Start() override;
    update_status Update() override;
    update_status PostUpdate() override;
    bool CleanUp() override;

private:
    enum class IntroPhase {
        CompanyLogo,
        Done
    };

    void UpdateFade(float deltaTime);
    void DrawPhase();

    IntroPhase m_phase;
    float m_alpha;
    float m_timer;

    Texture2D logoTexture;

    static constexpr float FADE_IN_TIME = 1.0f;
    static constexpr float HOLD_TIME = 2.0f;
    static constexpr float FADE_OUT_TIME = 1.0f;
};
