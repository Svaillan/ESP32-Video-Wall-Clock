#include "EffectsEngine.h"

EffectsEngine::EffectsEngine(MatrixDisplayManager* display, SettingsManager* settings) 
    : display(display), settings(settings) {
}

void EffectsEngine::begin() {
    // Initialize all effects
    initializeConfetti();
    initializeMatrixRain();
    initializeRain();
    initializeTorrent();
    initializeStars();
    initializeShootingStars();
    initializeSparkles();
    initializeFireworks();
    initializeTron();
    
    Serial.println("Effects Engine initialized");
}

void EffectsEngine::updateEffects() {
    EffectMode currentEffect = settings->getEffectMode();
    
    switch (currentEffect) {
        case EFFECT_CONFETTI:
            updateConfetti();
            break;
        case EFFECT_ACID:
            updateMatrixRain();
            break;
        case EFFECT_RAIN:
            updateRain();
            break;
        case EFFECT_TORRENT:
            updateTorrent();
            break;
        case EFFECT_STARS:
            updateStars();
            break;
        case EFFECT_SPARKLES:
            updateSparkles();
            break;
        case EFFECT_FIREWORKS:
            updateFireworks();
            break;
        case EFFECT_TRON:
            updateTron();
            break;
        case EFFECT_OFF:
        default:
            // No effects
            break;
    }
}

void EffectsEngine::setMenuPreviewMode(bool isPreview, int previewTextSize) {
    isMenuPreviewMode = isPreview;
    this->previewTextSize = previewTextSize;
}

// Helper function to check if position is in text area
bool EffectsEngine::isInTextArea(int x, int y) {
    if (isMenuPreviewMode) {
        return display->isInTextArea(x, y, true, previewTextSize);
    } else {
        return display->isInTextArea(x, y, true);
    }
}

// ===============================================
// CONFETTI EFFECT
// ===============================================

void EffectsEngine::initializeConfetti() {
    int centerY = MATRIX_HEIGHT / 2;
    int yRange = (MATRIX_HEIGHT / 4);  // Keep confetti in central band
    
    for (int i = 0; i < NUM_CONFETTI; i++) {
        confetti[i].x = random(0, MATRIX_WIDTH);
        confetti[i].y = random(centerY - yRange, centerY + yRange);
        
        // Generate non-zero velocities
        confetti[i].vx = display->generateVelocity(0.1, 0.8);  // Min 0.1, max 0.8 pixels/frame
        confetti[i].vy = display->generateVelocity(0.05, 0.4); // Min 0.05, max 0.4 pixels/frame
        
        confetti[i].color = display->randomVividColor();
    }
}

void EffectsEngine::resetConfettiParticle(int index) {
    int centerY = MATRIX_HEIGHT / 2;
    int yRange = (MATRIX_HEIGHT / 4);
    
    // 50% chance to spawn from horizontal edge, 50% from vertical edge
    if (random(2) == 0) {
        confetti[index].x = random(2) == 0 ? -CONFETTI_RAD : MATRIX_WIDTH + CONFETTI_RAD;
        confetti[index].y = random(centerY - yRange, centerY + yRange);
    } else {
        confetti[index].x = random(0, MATRIX_WIDTH);
        confetti[index].y = random(2) == 0 ? -CONFETTI_RAD : MATRIX_HEIGHT + CONFETTI_RAD;
    }
    
    // Generate non-zero velocities
    confetti[index].vx = display->generateVelocity(0.1, 0.8);
    confetti[index].vy = display->generateVelocity(0.05, 0.4);
    confetti[index].color = display->randomVividColor();
}

void EffectsEngine::updateConfetti() {
    for (int i = 0; i < NUM_CONFETTI; i++) {
        confetti[i].x += confetti[i].vx;
        confetti[i].y += confetti[i].vy;
        
        // Check if particle is out of bounds or in text area
        bool outOfBounds = (confetti[i].x < -CONFETTI_RAD || confetti[i].x > MATRIX_WIDTH + CONFETTI_RAD ||
                           confetti[i].y < -CONFETTI_RAD || confetti[i].y > MATRIX_HEIGHT + CONFETTI_RAD);
        
        bool inText = (confetti[i].x >= 0 && confetti[i].x < MATRIX_WIDTH && 
                      confetti[i].y >= 0 && confetti[i].y < MATRIX_HEIGHT &&
                      isInTextArea((int)confetti[i].x, (int)confetti[i].y));
        
        if (outOfBounds || inText) {
            resetConfettiParticle(i);
        } else {
            // Draw the confetti particle as a filled circle
            display->fillCircle((int)confetti[i].x, (int)confetti[i].y, CONFETTI_RAD, confetti[i].color);
        }
    }
}

// ===============================================
// MATRIX RAIN EFFECT (Acid Rain - Green)
// ===============================================

void EffectsEngine::initializeMatrixRain() {
    for (int i = 0; i < NUM_MATRIX_DROPS; i++) {
        matrixDrops[i].x = random(0, MATRIX_WIDTH);
        matrixDrops[i].y = random(-20, 0);  // Start above screen
        matrixDrops[i].length = random(3, 8);
        matrixDrops[i].speed = random(1, 4);
        matrixDrops[i].lastUpdate = millis();
    }
}

void EffectsEngine::updateMatrixRain() {
    uint32_t currentTime = millis();
    
    for (int i = 0; i < NUM_MATRIX_DROPS; i++) {
        if (currentTime - matrixDrops[i].lastUpdate > MATRIX_CHAR_DELAY / matrixDrops[i].speed) {
            matrixDrops[i].y += matrixDrops[i].speed;
            matrixDrops[i].lastUpdate = currentTime;
            
            // Reset when drop goes off screen
            if (matrixDrops[i].y > MATRIX_HEIGHT + matrixDrops[i].length) {
                matrixDrops[i].x = random(0, MATRIX_WIDTH);
                matrixDrops[i].y = random(-20, -5);
                matrixDrops[i].length = random(3, 8);
                matrixDrops[i].speed = random(1, 4);
            }
        }
        
        // Draw the drop trail
        for (int j = 0; j < matrixDrops[i].length && (matrixDrops[i].y - j) >= 0; j++) {
            int y = matrixDrops[i].y - j;
            if (y < MATRIX_HEIGHT && !isInTextArea(matrixDrops[i].x, y)) {
                uint8_t intensity = 255 - (j * 40);  // Fade as we go up
                if (intensity < 50) intensity = 50;
                uint16_t color = display->scaledEffectColor565(0, intensity, 0);  // Green rain with brightness scaling
                display->drawPixel(matrixDrops[i].x, y, color);
            }
        }
    }
}

// ===============================================
// RAIN EFFECT (Blue Rain)
// ===============================================

void EffectsEngine::initializeRain() {
    for (int i = 0; i < NUM_MATRIX_DROPS; i++) {
        matrixDrops[i].x = random(0, MATRIX_WIDTH);
        matrixDrops[i].y = random(-20, 0);  // Start above screen
        matrixDrops[i].length = random(3, 8);
        matrixDrops[i].speed = random(1, 4);
        matrixDrops[i].lastUpdate = millis();
    }
}

void EffectsEngine::updateRain() {
    uint32_t currentTime = millis();
    
    for (int i = 0; i < NUM_MATRIX_DROPS; i++) {
        if (currentTime - matrixDrops[i].lastUpdate > MATRIX_CHAR_DELAY / matrixDrops[i].speed) {
            matrixDrops[i].y += matrixDrops[i].speed;
            matrixDrops[i].lastUpdate = currentTime;
            
            // Reset when drop goes off screen
            if (matrixDrops[i].y > MATRIX_HEIGHT + matrixDrops[i].length) {
                matrixDrops[i].x = random(0, MATRIX_WIDTH);
                matrixDrops[i].y = random(-20, -5);
                matrixDrops[i].length = random(3, 8);
                matrixDrops[i].speed = random(1, 4);
            }
        }
        
        // Draw the drop trail in blue
        for (int j = 0; j < matrixDrops[i].length && (matrixDrops[i].y - j) >= 0; j++) {
            int y = matrixDrops[i].y - j;
            if (y < MATRIX_HEIGHT && !isInTextArea(matrixDrops[i].x, y)) {
                uint8_t intensity = 255 - (j * 40);  // Fade as we go up
                if (intensity < 50) intensity = 50;
                uint16_t color = display->scaledEffectColor565(0, 0, intensity);  // Blue rain with brightness scaling
                display->drawPixel(matrixDrops[i].x, y, color);
            }
        }
    }
}

// ===============================================
// TORRENT EFFECT (Heavy Rain)
// ===============================================

void EffectsEngine::initializeTorrent() {
    for (int i = 0; i < NUM_TORRENT_DROPS; i++) {
        torrentDrops[i].x = random(0, MATRIX_WIDTH);
        torrentDrops[i].y = random(-30, 0);  // Start above screen
        torrentDrops[i].length = random(1, 4);  // Smaller drops
        torrentDrops[i].speed = random(2, 6);   // Faster speed
        torrentDrops[i].lastUpdate = millis();
    }
}

void EffectsEngine::updateTorrent() {
    uint32_t currentTime = millis();
    
    for (int i = 0; i < NUM_TORRENT_DROPS; i++) {
        if (currentTime - torrentDrops[i].lastUpdate > TORRENT_CHAR_DELAY / torrentDrops[i].speed) {
            torrentDrops[i].y += torrentDrops[i].speed;
            torrentDrops[i].lastUpdate = currentTime;
            
            // Reset when drop goes off screen
            if (torrentDrops[i].y > MATRIX_HEIGHT + torrentDrops[i].length) {
                torrentDrops[i].x = random(0, MATRIX_WIDTH);
                torrentDrops[i].y = random(-30, -5);
                torrentDrops[i].length = random(1, 4);  // Smaller drops
                torrentDrops[i].speed = random(2, 6);   // Faster speed
            }
        }
        
        // Draw the drop trail in white/light blue
        for (int j = 0; j < torrentDrops[i].length && (torrentDrops[i].y - j) >= 0; j++) {
            int y = torrentDrops[i].y - j;
            if (y < MATRIX_HEIGHT && !isInTextArea(torrentDrops[i].x, y)) {
                uint8_t intensity = 255 - (j * 60);  // Faster fade for smaller drops
                if (intensity < 80) intensity = 80;
                uint16_t color = display->scaledEffectColor565(intensity/2, intensity/2, intensity);  // Light blue/white torrent
                display->drawPixel(torrentDrops[i].x, y, color);
            }
        }
    }
}

// ===============================================
// STARS EFFECT
// ===============================================

void EffectsEngine::initializeStars() {
    for (int i = 0; i < NUM_STARS; i++) {
        stars[i].x = random(0, MATRIX_WIDTH * 100) / 100.0f;  // Sub-pixel precision
        stars[i].y = random(0, MATRIX_HEIGHT * 100) / 100.0f;
        stars[i].brightness = random(50, 255);
        stars[i].twinkleState = random(0, 2);
        stars[i].lastTwinkle = millis() + random(0, 2000);
        stars[i].twinkleInterval = random(800, 2000);  // Set individual twinkle timing
        
        // 40% chance for a star to be a steady background star (doesn't twinkle)
        stars[i].shouldTwinkle = (random(0, 100) < 60);  // 60% twinkle, 40% steady
        
        // Background stars are dimmer and stay at low brightness
        if (!stars[i].shouldTwinkle) {
            stars[i].brightness = random(30, 80);  // Dimmer range for background stars
            stars[i].twinkleState = 0;  // Always dim
        }
    }
}

void EffectsEngine::initializeShootingStars() {
    for (int i = 0; i < NUM_SHOOTING_STARS; i++) {
        shootingStars[i].active = false;
    }
    lastShootingStarTime = millis();
    waitingForSecondStar = false;
    waitingForThirdStar = false;
}

void EffectsEngine::spawnShootingStar(int index) {
    // Start from random position at the top or left edge
    if (random(0, 2) == 0) {
        // Start from top edge
        shootingStars[index].x = random(0, MATRIX_WIDTH);
        shootingStars[index].y = 0;
    } else {
        // Start from left edge
        shootingStars[index].x = 0;
        shootingStars[index].y = random(0, MATRIX_HEIGHT);
    }
    
    // Diagonal movement (northwest to southeast)
    shootingStars[index].speedX = SHOOTING_STAR_SPEED;
    shootingStars[index].speedY = SHOOTING_STAR_SPEED * 0.7f;  // Slightly less vertical speed
    shootingStars[index].active = true;
    shootingStars[index].trailLength = SHOOTING_STAR_TRAIL_LENGTH;
    shootingStars[index].spawnTime = millis();
}

void EffectsEngine::updateStars() {
    uint32_t currentTime = millis();
    static uint32_t lastDriftUpdate = 0;
    static float globalDriftX = 0.005f;  // Slowed down to 10% of original speed
    static float globalDriftY = 0.003f;  // Slowed down to 10% of original speed
    
    // Slow drift movement - update every 150ms
    if (currentTime - lastDriftUpdate > 150) {
        float groupDriftX = globalDriftX;  // Completely fixed drift
        float groupDriftY = globalDriftY;
        
        // Apply the SAME movement to all stars as a unified group
        for (int i = 0; i < NUM_STARS; i++) {
            stars[i].x += groupDriftX;
            stars[i].y += groupDriftY;
            
            // If star has drifted off the right or bottom edge, respawn it on the opposite edges
            if (stars[i].x > MATRIX_WIDTH + 2 || stars[i].y > MATRIX_HEIGHT + 2) {
                if (stars[i].x > MATRIX_WIDTH + 2) {
                    // Respawn on left side
                    stars[i].x = -1.5f;
                    stars[i].y = (i * MATRIX_HEIGHT / NUM_STARS) + (i % 3 - 1);
                }
                if (stars[i].y > MATRIX_HEIGHT + 2) {
                    // Respawn on top side
                    stars[i].y = -1.5f;
                    stars[i].x = (i * MATRIX_WIDTH / NUM_STARS) + (i % 3 - 1);
                }
            }
        }
        lastDriftUpdate = currentTime;
    }
    
    // Shooting star management
    if (currentTime - lastShootingStarTime > random(300000, 600000)) {  // 5-10 minutes
        // Find an inactive shooting star to spawn
        for (int i = 0; i < NUM_SHOOTING_STARS; i++) {
            if (!shootingStars[i].active) {
                spawnShootingStar(i);
                lastShootingStarTime = currentTime;
                
                // 50% chance for a second shooting star
                if (random(0, 100) < 50) {
                    waitingForSecondStar = true;
                    secondStarTimer = currentTime + random(500, 5000);  // 0.5-5 seconds later
                }
                break;
            }
        }
    }
    
    // Check for second shooting star
    if (waitingForSecondStar && currentTime >= secondStarTimer) {
        for (int i = 0; i < NUM_SHOOTING_STARS; i++) {
            if (!shootingStars[i].active) {
                spawnShootingStar(i);
                waitingForSecondStar = false;
                
                // 15% chance for a third shooting star
                if (random(0, 100) < 15) {
                    waitingForThirdStar = true;
                    thirdStarTimer = currentTime + random(500, 5000);  // 0.5-5 seconds later
                }
                break;
            }
        }
    }
    
    // Check for third shooting star
    if (waitingForThirdStar && currentTime >= thirdStarTimer) {
        for (int i = 0; i < NUM_SHOOTING_STARS; i++) {
            if (!shootingStars[i].active) {
                spawnShootingStar(i);
                waitingForThirdStar = false;
                break;
            }
        }
    }
    
    // Update shooting stars
    for (int i = 0; i < NUM_SHOOTING_STARS; i++) {
        if (shootingStars[i].active) {
            shootingStars[i].x += shootingStars[i].speedX;
            shootingStars[i].y += shootingStars[i].speedY;
            
            // Deactivate if off screen
            if (shootingStars[i].x > MATRIX_WIDTH + 10 || shootingStars[i].y > MATRIX_HEIGHT + 10) {
                shootingStars[i].active = false;
            } else {
                // Draw shooting star trail
                for (int t = 0; t < shootingStars[i].trailLength; t++) {
                    int trailX = (int)(shootingStars[i].x - t * shootingStars[i].speedX * 0.5f);
                    int trailY = (int)(shootingStars[i].y - t * shootingStars[i].speedY * 0.5f);
                    
                    if (trailX >= 0 && trailX < MATRIX_WIDTH && trailY >= 0 && trailY < MATRIX_HEIGHT) {
                        if (!isInTextArea(trailX, trailY)) {
                            uint8_t brightness = 255 - (t * 32);  // Fade trail
                            uint16_t color = display->scaledEffectColor565(brightness, brightness, brightness);
                            display->drawPixel(trailX, trailY, color);
                        }
                    }
                }
            }
        }
    }
    
    // Draw regular stars
    for (int i = 0; i < NUM_STARS; i++) {
        // Handle twinkling - only for stars that should twinkle
        if (stars[i].shouldTwinkle && currentTime - stars[i].lastTwinkle > stars[i].twinkleInterval) {
            stars[i].twinkleState = !stars[i].twinkleState;
            stars[i].lastTwinkle = currentTime;
            // Set new random interval for next twinkle
            stars[i].twinkleInterval = random(800, 2000);
        }
        
        // Draw star if on screen and not in text area
        int pixelX = (int)round(stars[i].x);
        int pixelY = (int)round(stars[i].y);
        
        if (pixelX >= 0 && pixelX < MATRIX_WIDTH && pixelY >= 0 && pixelY < MATRIX_HEIGHT) {
            if (!isInTextArea(pixelX, pixelY)) {
                uint8_t brightness;
                if (stars[i].shouldTwinkle) {
                    // Twinkling stars: bright when on, dim when off
                    brightness = stars[i].twinkleState ? stars[i].brightness : stars[i].brightness / 3;
                } else {
                    // Steady background stars: always at their base dim brightness
                    brightness = stars[i].brightness;
                }
                uint16_t color = display->scaledEffectColor565(brightness, brightness, brightness);
                display->drawPixel(pixelX, pixelY, color);
            }
        }
    }
}

// ===============================================
// SPARKLES EFFECT
// ===============================================

void EffectsEngine::initializeSparkles() {
    for (int i = 0; i < NUM_SPARKLES; i++) {
        sparkles[i].x = random(0, MATRIX_WIDTH);
        sparkles[i].y = random(0, MATRIX_HEIGHT);
        sparkles[i].brightness = 0;
        sparkles[i].color = display->randomVividColor();  // Assign random vivid color
        sparkles[i].startTime = millis() + random(0, 3000);
        sparkles[i].duration = random(400, SPARKLE_DURATION);
    }
}

void EffectsEngine::updateSparkles() {
    uint32_t currentTime = millis();
    
    for (int i = 0; i < NUM_SPARKLES; i++) {
        uint32_t elapsed = currentTime - sparkles[i].startTime;
        
        if (elapsed < sparkles[i].duration) {
            // Calculate sparkle brightness (fade in and out)
            float progress = (float)elapsed / sparkles[i].duration;
            float brightness = sin(progress * PI) * 255;
            sparkles[i].brightness = (uint8_t)brightness;
            
            // Draw sparkle if not in text area
            if (!isInTextArea(sparkles[i].x, sparkles[i].y)) {
                // Scale the stored color by brightness and apply global brightness
                uint8_t r = ((sparkles[i].color >> 11) & 0x1F) * sparkles[i].brightness / 255;
                uint8_t g = ((sparkles[i].color >> 5) & 0x3F) * sparkles[i].brightness / 255;
                uint8_t b = (sparkles[i].color & 0x1F) * sparkles[i].brightness / 255;
                uint16_t scaledColor = display->scaledEffectColor565(r << 3, g << 2, b << 3);
                display->drawPixel(sparkles[i].x, sparkles[i].y, scaledColor);
            }
        } else {
            // Reset sparkle to new position with new color
            sparkles[i].x = random(0, MATRIX_WIDTH);
            sparkles[i].y = random(0, MATRIX_HEIGHT);
            sparkles[i].color = display->randomVividColor();  // New random color
            sparkles[i].startTime = currentTime + random(0, 2000);
            sparkles[i].duration = random(400, SPARKLE_DURATION);
        }
    }
}

// ===============================================
// FIREWORKS EFFECT
// ===============================================

void EffectsEngine::initializeFireworks() {
    for (int i = 0; i < NUM_FIREWORKS; i++) {
        fireworks[i].active = false;
        fireworks[i].exploded = false;
        fireworks[i].startTime = millis() + random(0, 3000);  // Stagger initial launches
    }
}

void EffectsEngine::updateFireworks() {
    uint32_t currentTime = millis();
    
    for (int i = 0; i < NUM_FIREWORKS; i++) {
        if (!fireworks[i].active) {
            // Launch new firework randomly
            if (currentTime > fireworks[i].startTime) {
                // Choose launch and explosion position to avoid text area
                int textCenterX = MATRIX_WIDTH / 2;
                
                // Launch from sides or center, but explode away from text
                if (random(0, 2) == 0) {
                    // Launch from left side, explode on left
                    fireworks[i].x = random(5, MATRIX_WIDTH / 3);
                } else {
                    // Launch from right side, explode on right  
                    fireworks[i].x = random(2 * MATRIX_WIDTH / 3, MATRIX_WIDTH - 5);
                }
                
                fireworks[i].y = MATRIX_HEIGHT;  // Start from bottom
                fireworks[i].color = display->randomVividColor();  // Color for explosion
                fireworks[i].active = true;
                fireworks[i].exploded = false;
                fireworks[i].startTime = currentTime;
            }
        } else if (!fireworks[i].exploded) {
            // Rising phase - draw white rocket moving up
            uint32_t elapsed = currentTime - fireworks[i].startTime;
            float progress = elapsed / 800.0f;  // 800ms rise time
            
            if (progress >= 1.0f) {
                // Time to explode - initialize particles
                fireworks[i].exploded = true;
                fireworks[i].startTime = currentTime;
                
                for (int j = 0; j < FIREWORK_PARTICLES; j++) {
                    float angle = (j * 2.0f * PI) / FIREWORK_PARTICLES;
                    float speed = random(100, 250) / 100.0f;  // Increased speed for more dramatic spread
                    fireworks[i].vx[j] = cos(angle) * speed;
                    fireworks[i].vy[j] = sin(angle) * speed;
                }
            } else {
                // Draw rising white rocket
                int rocketY = fireworks[i].y - (progress * (MATRIX_HEIGHT / 2));
                if (rocketY >= 0 && rocketY < MATRIX_HEIGHT && !isInTextArea(fireworks[i].x, rocketY)) {
                    // White rocket trail
                    uint16_t whiteColor = display->applyEffectBrightness(display->color565(255, 255, 255));
                    display->drawPixel(fireworks[i].x, rocketY, whiteColor);
                }
            }
        } else {
            // Explosion phase - draw expanding particles
            uint32_t elapsed = currentTime - fireworks[i].startTime;
            float progress = elapsed / 1000.0f;  // Reduced from 1500ms to 1000ms for faster fade
            
            if (progress >= 1.0f) {
                // Firework finished, reset for next launch
                fireworks[i].active = false;
                fireworks[i].startTime = currentTime + random(2000, 5000);  // Wait 2-5 seconds
            } else {
                // Draw explosion particles with more dramatic spread
                float centerX = fireworks[i].x;
                float centerY = fireworks[i].y - (MATRIX_HEIGHT / 2);
                
                for (int j = 0; j < FIREWORK_PARTICLES; j++) {
                    // Increased spread speed and reduced gravity effect
                    float px = centerX + (fireworks[i].vx[j] * elapsed / 50.0f);  // Increased from 100 to 50 for faster spread
                    float py = centerY + (fireworks[i].vy[j] * elapsed / 50.0f) + (0.1f * elapsed * elapsed / 10000.0f);  // Much reduced gravity
                    
                    if (px >= 0 && px < MATRIX_WIDTH && py >= 0 && py < MATRIX_HEIGHT) {
                        if (!isInTextArea(px, py)) {
                            // More dramatic fade - particles disappear while still in sky
                            float fade = (1.0f - progress) * (1.0f - progress);  // Exponential fade for more dramatic effect
                            if (fade > 0.1f) {  // Only draw if fade is significant
                                uint16_t fadedColor = display->applyEffectBrightness(display->scaleBrightness(fireworks[i].color, fade));
                                display->drawPixel((int)px, (int)py, fadedColor);
                            }
                        }
                    }
                }
            }
        }
    }
}

// ===============================================
// TRON EFFECT
// ===============================================

void EffectsEngine::initializeTron() {
    for (int i = 0; i < NUM_TRON_TRAILS; i++) {
        tronTrails[i].active = false;
        tronTrails[i].currentLength = 0;
        tronTrails[i].lastMove = millis() + random(0, 2000);  // Stagger initial starts
    }
}

void EffectsEngine::updateTron() {
    uint32_t currentTime = millis();
    
    for (int i = 0; i < NUM_TRON_TRAILS; i++) {
        if (!tronTrails[i].active) {
            // Start new trail
            if (currentTime > tronTrails[i].lastMove) {
                tronTrails[i].active = true;
                tronTrails[i].currentLength = 0;
                tronTrails[i].speed = random(TRON_MIN_SPEED, TRON_MAX_SPEED + 1);
                tronTrails[i].direction = random(0, 4);  // 0=right, 1=down, 2=left, 3=up
                
                // Choose starting position based on direction
                switch (tronTrails[i].direction) {
                    case 0: // Moving right
                        tronTrails[i].x = 0;
                        tronTrails[i].y = random(0, MATRIX_HEIGHT);
                        break;
                    case 1: // Moving down
                        tronTrails[i].x = random(0, MATRIX_WIDTH);
                        tronTrails[i].y = 0;
                        break;
                    case 2: // Moving left
                        tronTrails[i].x = MATRIX_WIDTH - 1;
                        tronTrails[i].y = random(0, MATRIX_HEIGHT);
                        break;
                    case 3: // Moving up
                        tronTrails[i].x = random(0, MATRIX_WIDTH);
                        tronTrails[i].y = MATRIX_HEIGHT - 1;
                        break;
                }
                
                // Choose Tron-like colors (cyan, blue, white)
                int colorChoice = random(0, 3);
                switch (colorChoice) {
                    case 0: tronTrails[i].color = display->color565(0, 255, 255); break;    // Cyan
                    case 1: tronTrails[i].color = display->color565(0, 150, 255); break;   // Light blue
                    case 2: tronTrails[i].color = display->color565(255, 255, 255); break; // White
                }
                
                // Add starting position to trail
                tronTrails[i].trailPositions[0][0] = tronTrails[i].x;
                tronTrails[i].trailPositions[0][1] = tronTrails[i].y;
                tronTrails[i].currentLength = 1;
                tronTrails[i].lastMove = currentTime;
            }
        } else {
            // Update active trail
            if (currentTime - tronTrails[i].lastMove >= tronTrails[i].speed) {
                // Move trail head
                switch (tronTrails[i].direction) {
                    case 0: tronTrails[i].x++; break; // Right
                    case 1: tronTrails[i].y++; break; // Down
                    case 2: tronTrails[i].x--; break; // Left
                    case 3: tronTrails[i].y--; break; // Up
                }
                
                // Add new head position to trail if on screen
                if (tronTrails[i].x >= 0 && tronTrails[i].x < MATRIX_WIDTH && 
                    tronTrails[i].y >= 0 && tronTrails[i].y < MATRIX_HEIGHT) {
                    
                    // Shift trail positions
                    if (tronTrails[i].currentLength >= TRON_MAX_LENGTH) {
                        // Remove tail
                        for (int j = 0; j < TRON_MAX_LENGTH - 1; j++) {
                            tronTrails[i].trailPositions[j][0] = tronTrails[i].trailPositions[j + 1][0];
                            tronTrails[i].trailPositions[j][1] = tronTrails[i].trailPositions[j + 1][1];
                        }
                        tronTrails[i].currentLength = TRON_MAX_LENGTH - 1;
                    }
                    
                    // Add new head
                    tronTrails[i].trailPositions[tronTrails[i].currentLength][0] = tronTrails[i].x;
                    tronTrails[i].trailPositions[tronTrails[i].currentLength][1] = tronTrails[i].y;
                    tronTrails[i].currentLength++;
                }
                
                // Check if trail is completely off screen
                if (tronTrails[i].x < -TRON_MAX_LENGTH || tronTrails[i].x >= MATRIX_WIDTH + TRON_MAX_LENGTH ||
                    tronTrails[i].y < -TRON_MAX_LENGTH || tronTrails[i].y >= MATRIX_HEIGHT + TRON_MAX_LENGTH) {
                    tronTrails[i].active = false;
                    tronTrails[i].lastMove = currentTime + random(1000, 3000);  // Wait before next trail
                } else {
                    tronTrails[i].lastMove = currentTime;
                }
            }
            
            // Always draw the trail for active trails (whether moving or not)
            for (int j = 0; j < tronTrails[i].currentLength; j++) {
                uint8_t segX = tronTrails[i].trailPositions[j][0];
                uint8_t segY = tronTrails[i].trailPositions[j][1];
                
                if (segX < MATRIX_WIDTH && segY < MATRIX_HEIGHT) {
                    if (!isInTextArea(segX, segY)) {
                        // Fade from dim at tail to bright at head
                        float brightness = (float)(j + 1) / tronTrails[i].currentLength;
                        uint16_t fadedColor = display->applyEffectBrightness(display->scaleBrightness(tronTrails[i].color, brightness));
                        display->drawPixel(segX, segY, fadedColor);
                    }
                }
            }
        }
    }
}
