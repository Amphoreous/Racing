#include "modules/ModuleIntro.h"
#include "core/Application.h"
#include "modules/ModuleResources.h"
#include "modules/ModuleMainMenu.h"
#include "raylib.h"

ModuleIntro::ModuleIntro(Application* app, bool start_enabled) 
    : Module(app, start_enabled),
    m_phase(IntroPhase::CompanyLogo),
    m_alpha(0.0f),
    m_timer(0.0f)
{
    LOG("Intro module constructor");
}

ModuleIntro::~ModuleIntro()
{
}

bool ModuleIntro::Start()
{
    LOG("Intro module started");
    m_phase = IntroPhase::CompanyLogo;
    m_alpha = 0.0f;
    m_timer = 0.0f;
    
    logoTexture = App->resources->LoadTexture("assets/ui/intro/logo.png");
    
    return true;
}

update_status ModuleIntro::Update()
{
    if (m_phase == IntroPhase::Done)
    {
        return UPDATE_CONTINUE;
    }

    float deltaTime = GetFrameTime();
    
    // Allow skipping with SPACE or ENTER
    if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER))
    {
        m_phase = IntroPhase::Done;
        this->Disable();
        App->state = GAME_MENU;
        App->mainMenu->Enable();
        return UPDATE_CONTINUE;
    }

    UpdateFade(deltaTime);
    
    return UPDATE_CONTINUE;
}

void ModuleIntro::UpdateFade(float deltaTime)
{
    m_timer += deltaTime;

    if (m_timer < FADE_IN_TIME)
    {
        // Fade in
        m_alpha = m_timer / FADE_IN_TIME;
    }
    else if (m_timer < FADE_IN_TIME + HOLD_TIME)
    {
        // Hold at full opacity
        m_alpha = 1.0f;
    }
    else if (m_timer < FADE_IN_TIME + HOLD_TIME + FADE_OUT_TIME)
    {
        // Fade out
        m_alpha = 1.0f - (m_timer - (FADE_IN_TIME + HOLD_TIME)) / FADE_OUT_TIME;
    }
    else
    {
        // Phase complete
        m_timer = 0.0f;
        m_alpha = 0.0f;
        m_phase = IntroPhase::Done;
        
        // Transition to main menu
        this->Disable();
        App->state = GAME_MENU;
        App->mainMenu->Enable();
    }
}

update_status ModuleIntro::PostUpdate()
{
    if (m_phase == IntroPhase::Done)
    {
        return UPDATE_CONTINUE;
    }

    DrawPhase();
    
    return UPDATE_CONTINUE;
}

void ModuleIntro::DrawPhase()
{
    // Clear to black background
    ClearBackground(BLACK);
    
    float centerX = GetScreenWidth() / 2.0f;
    float centerY = GetScreenHeight() / 2.0f;

    // Draw logo centered and scaled appropriately
    float logoScale = 0.5f; // Adjust scale as needed
    float logoWidth = logoTexture.width * logoScale;
    float logoHeight = logoTexture.height * logoScale;
    
    Rectangle logoSource = { 0, 0, (float)logoTexture.width, (float)logoTexture.height };
    Rectangle logoDest = {
        centerX - logoWidth / 2,
        centerY - logoHeight / 2 - 40, // Slightly above center to make room for text
        logoWidth,
        logoHeight
    };
    
    DrawTexturePro(logoTexture, logoSource, logoDest, Vector2{0, 0}, 0.0f, 
                   ColorAlpha(WHITE, m_alpha));

    // Draw company name below the logo
    const char* companyName = "Amphoreous";
    int fontSize = 60;
    int textWidth = MeasureText(companyName, fontSize);
    
    float textY = logoDest.y + logoDest.height + 30;
    
    // Draw shadow for better readability
    DrawText(companyName, 
             (int)(centerX - textWidth / 2) + 2, 
             (int)textY + 2, 
             fontSize, 
             ColorAlpha(BLACK, m_alpha * 0.5f));
    
    // Draw main text
    DrawText(companyName, 
             (int)(centerX - textWidth / 2), 
             (int)textY, 
             fontSize, 
             ColorAlpha(WHITE, m_alpha));
}

bool ModuleIntro::CleanUp()
{
    LOG("Intro module cleanup");
    return true;
}
