#ifndef EFFECTS_ENGINE_H
#define EFFECTS_ENGINE_H

#include <Arduino.h>

#include "AppState.h"
#include "MatrixDisplayManager.h"
#include "SettingsManager.h"

// Effect Settings
#define NUM_CONFETTI 40
#define CONFETTI_RAD 1

#define NUM_MATRIX_DROPS 12
#define MATRIX_CHAR_DELAY 80

#define NUM_TORRENT_DROPS 30
#define TORRENT_CHAR_DELAY 60

#define NUM_STARS 45
#define STAR_TWINKLE_CHANCE 5

#define NUM_SHOOTING_STARS 2
#define SHOOTING_STAR_SPEED 0.8f
#define SHOOTING_STAR_TRAIL_LENGTH 8

#define NUM_SPARKLES 200
#define SPARKLE_DURATION 800

#define NUM_FIREWORKS 8
#define FIREWORK_PARTICLES 15
#define FIREWORK_LIFE 1500

#define NUM_TRON_TRAILS 12
#define TRON_MIN_LENGTH 8
#define TRON_MAX_LENGTH 20
#define TRON_MIN_SPEED 80
#define TRON_MAX_SPEED 200

// Effect Data Structures
struct Confetti {
    float x, y, vx, vy;
    uint16_t color;
};

struct MatrixDrop {
    float x, y;
    uint8_t length;
    uint8_t speed;
    uint32_t lastUpdate;
};

struct Star {
    float x, y;  // Current position (can be off-screen)
    uint8_t brightness;
    uint8_t twinkleState;
    uint32_t lastTwinkle;
    uint32_t twinkleInterval;  // Individual twinkle timing for each star
    bool shouldTwinkle;        // Whether this star twinkles or stays steady
};

struct ShootingStar {
    float x, y;
    float speedX, speedY;
    bool active;
    uint8_t trailLength;
    uint32_t spawnTime;
};

struct Sparkle {
    uint8_t x, y;
    uint8_t brightness;
    uint16_t color;
    uint32_t startTime;
    uint16_t duration;
};

struct Firework {
    float x, y;
    float vx[FIREWORK_PARTICLES], vy[FIREWORK_PARTICLES];
    uint8_t px[FIREWORK_PARTICLES], py[FIREWORK_PARTICLES];
    uint16_t color;
    uint32_t startTime;
    bool active;
    bool exploded;
    int explosionHeight;  // Height at which this firework will explode
};

struct TronTrail {
    uint8_t x, y;
    uint8_t direction;                           // 0=right, 1=down, 2=left, 3=up
    uint8_t trailPositions[TRON_MAX_LENGTH][2];  // Store trail segment positions
    uint8_t currentLength;
    uint16_t color;
    uint32_t lastMove;
    uint16_t speed;
    bool active;
};

class EffectsEngine {
   public:
    // Constructor
    EffectsEngine(MatrixDisplayManager* display, SettingsManager* settings);

    // Initialization
    void begin();

    // Effect Control
    void updateEffects();
    void setMenuPreviewMode(bool isPreview, int previewTextSize = 1);
    void setDisplayMode(AppState displayMode);

    // Individual effect controls
    void initializeConfetti();
    void updateConfetti();
    void resetConfettiParticle(int index);

    void initializeMatrixRain();
    void updateMatrixRain();

    void initializeRain();
    void updateRain();

    void initializeTorrent();
    void updateTorrent();

    void initializeStars();
    void updateStars();
    void initializeShootingStars();
    void spawnShootingStar(int index);

    void initializeSparkles();
    void updateSparkles();

    void initializeFireworks();
    void updateFireworks();

    void initializeTron();
    void updateTron();

    // Effect names accessor
    static const char* getEffectNames() {
        return "Confetti,Acid,Rain,Torrent,Stars,Sparkles,Fireworks,Tron,Off";
    }

   private:
    MatrixDisplayManager* display;
    SettingsManager* settings;

    // Effect particle arrays
    Confetti confetti[NUM_CONFETTI];
    MatrixDrop matrixDrops[NUM_MATRIX_DROPS];
    MatrixDrop torrentDrops[NUM_TORRENT_DROPS];
    Star stars[NUM_STARS];
    ShootingStar shootingStars[NUM_SHOOTING_STARS];
    Sparkle sparkles[NUM_SPARKLES];
    Firework fireworks[NUM_FIREWORKS];
    TronTrail tronTrails[NUM_TRON_TRAILS];

    // Shooting star timing variables
    uint32_t lastShootingStarTime = 0;
    bool waitingForSecondStar = false;
    uint32_t secondStarTimer = 0;
    bool waitingForThirdStar = false;
    uint32_t thirdStarTimer = 0;

    // Menu preview mode
    bool isMenuPreviewMode = false;
    int previewTextSize = 1;

    // Display mode tracking
    AppState currentDisplayMode = SHOW_TIME;

    // Helper functions
    bool isInTextArea(int x, int y);
};

#endif  // EFFECTS_ENGINE_H
