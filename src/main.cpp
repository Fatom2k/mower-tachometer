#include <Arduino.h>
#include <TFT_eSPI.h>
#include "config.h"

// =====================================================
// Objets TFT
// =====================================================
TFT_eSPI    tft;
TFT_eSprite sprite(&tft);

// =====================================================
// Variables mesure RPM
// =====================================================
volatile uint32_t lastPulseTime_us  = 0;
volatile uint32_t pulseInterval_us  = 0;
volatile bool     newPulseAvailable = false;

uint32_t currentRPM  = 0;
uint32_t minRPM      = 0;
uint32_t maxRPM      = 0;
bool     engineRunning   = false;
uint32_t lastPulseMillis = 0;

// =====================================================
// ISR – Interruption capteur Hall
// =====================================================
void IRAM_ATTR hallSensorISR() {  // ISR pour pickup inductif
    const uint32_t now      = micros();
    const uint32_t interval = now - lastPulseTime_us;

    // Filtre anti-rebond : rejeter les impulsions trop courtes
    if (interval >= MIN_PULSE_INTERVAL_US) {
        pulseInterval_us  = interval;
        lastPulseTime_us  = now;
        newPulseAvailable = true;
    }
}

// =====================================================
// Calcul RPM depuis l'intervalle entre impulsions
// =====================================================
static inline uint32_t calculateRPM(uint32_t interval_us) {
    if (interval_us == 0) return 0;
    // 4 temps : 1 étincelle toutes les 2 révolutions
    // RPM = (60 000 000 µs × CYCLES_PAR_IMPULSION) / intervalle_µs
    return (uint32_t)((60000000ULL * CYCLES_PAR_IMPULSION) / interval_us);
}

// =====================================================
// Couleur de l'arc selon la zone RPM
// =====================================================
static uint16_t getArcColor(uint32_t rpm) {
    if (rpm < RPM_ZONE_IDLE)   return TFT_CYAN;
    if (rpm < RPM_ZONE_NORMAL) return TFT_GREEN;
    if (rpm < RPM_ZONE_HIGH)   return 0xFFE0; // Jaune
    return TFT_RED;
}

// =====================================================
// Dessin de l'arc de la jauge dans un sprite
//
// Convention d'angle : 0° = 12h, sens horaire
//   x = cx + r × sin(a)
//   y = cy − r × cos(a)
//
// startDeg : angle de départ (bas-gauche à 225°)
// totalDeg : étendue totale de l'arc (270°)
// fraction : valeur normalisée [0.0 … 1.0]
// =====================================================
static void drawArcGauge(float fraction, uint16_t activeColor) {
    constexpr int   CX        = SCREEN_W / 2;  // 120
    constexpr int   CY        = 82;
    constexpr int   OUTER_R   = 52;
    constexpr int   INNER_R   = 40;
    constexpr float START_DEG = 225.0f;
    constexpr float TOTAL_DEG = 270.0f;
    constexpr float STEP      = 1.5f;

    const float valueDeg = START_DEG + fraction * TOTAL_DEG;
    const float endDeg   = START_DEG + TOTAL_DEG;

    // --- Arc principal ---
    for (float a = START_DEG; a <= endDeg; a += STEP) {
        const float rad = a * (float)(M_PI / 180.0);
        const float s   = sinf(rad);
        const float c   = cosf(rad);

        const uint16_t color = (fraction > 0.001f && a <= valueDeg)
                               ? activeColor
                               : 0x2104; // gris très sombre

        for (int r = INNER_R; r <= OUTER_R; r++) {
            sprite.drawPixel(CX + (int)(r * s + 0.5f),
                             CY - (int)(r * c + 0.5f),
                             color);
        }
    }

    // --- Graduations (toutes les 500 RPM) ---
    constexpr int NUM_TICKS   = RPM_MAX_DISPLAY / 500;  // 9
    constexpr int TICK_INNER  = OUTER_R + 3;
    constexpr int TICK_OUTER  = OUTER_R + 10;

    for (int i = 0; i <= NUM_TICKS; i++) {
        const float tickFrac = (float)i / NUM_TICKS;
        const float a        = START_DEG + tickFrac * TOTAL_DEG;
        const float rad      = a * (float)(M_PI / 180.0);
        const float s        = sinf(rad);
        const float c        = cosf(rad);

        sprite.drawLine(
            CX + (int)(TICK_INNER * s + 0.5f), CY - (int)(TICK_INNER * c + 0.5f),
            CX + (int)(TICK_OUTER * s + 0.5f), CY - (int)(TICK_OUTER * c + 0.5f),
            TFT_WHITE
        );
    }
}

// =====================================================
// Rendu complet de l'interface (via sprite → sans scintillement)
// =====================================================
static void updateDisplay() {
    sprite.fillSprite(TFT_BLACK);

    // ---- Titre ----
    sprite.setTextDatum(TC_DATUM);
    sprite.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    sprite.setTextFont(2);
    sprite.drawString("TACHYMETRE", SCREEN_W / 2, 2);

    // ---- Jauge arc ----
    const float fraction = constrain(
        (float)currentRPM / RPM_MAX_DISPLAY, 0.0f, 1.0f
    );
    drawArcGauge(fraction, getArcColor(currentRPM));

    // ---- Valeur RPM (centré dans l'arc) ----
    sprite.setTextDatum(MC_DATUM);
    sprite.setTextColor(engineRunning ? TFT_WHITE : 0x4208, TFT_BLACK);
    sprite.setTextFont(7);  // police 7-segments
    sprite.drawNumber((int32_t)currentRPM, SCREEN_W / 2, 78);

    // ---- Label "RPM" ----
    sprite.setTextFont(2);
    sprite.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    sprite.drawString("RPM", SCREEN_W / 2, 100);

    // ---- MIN / MAX ----
    sprite.setTextFont(1);
    sprite.setTextDatum(BL_DATUM);
    sprite.setTextColor(TFT_CYAN, TFT_BLACK);
    sprite.drawString("MIN", 5, SCREEN_H - 8);
    sprite.setTextColor(TFT_WHITE, TFT_BLACK);
    sprite.drawNumber((int32_t)minRPM, 24, SCREEN_H - 8);

    sprite.setTextDatum(BR_DATUM);
    sprite.setTextColor(TFT_RED, TFT_BLACK);
    sprite.drawString("MAX", SCREEN_W - 5, SCREEN_H - 8);
    sprite.setTextColor(TFT_WHITE, TFT_BLACK);
    sprite.drawNumber((int32_t)maxRPM, SCREEN_W - 24, SCREEN_H - 8);

    // ---- Statut moteur arrêté ----
    if (!engineRunning) {
        sprite.setTextDatum(BC_DATUM);
        sprite.setTextColor(TFT_YELLOW, TFT_BLACK);
        sprite.setTextFont(2);
        sprite.drawString("MOTEUR ARRETE", SCREEN_W / 2, SCREEN_H - 2);
    }

    sprite.pushSprite(0, 0);
}

// =====================================================
// Setup
// =====================================================
void setup() {
    Serial.begin(115200);
    Serial.println("\n=== Tachymetre Tracteur Tondeuse ===");
    Serial.println("Moteur : Briggs & Stratton 217707");

    // --- Ecran ---
    tft.init();
    tft.setRotation(1);       // Paysage : 240 x 135
    tft.fillScreen(TFT_BLACK);

    // Rétroéclairage (pleine luminosité)
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);

    // Sprite pour affichage sans scintillement
    sprite.createSprite(SCREEN_W, SCREEN_H);
    sprite.setColorDepth(16);

    // --- Capteur inductif (bobine autour fil de bougie) ---
    pinMode(PICKUP_PIN, INPUT);   // pas de pull-up interne : R2 externe fait le pull-down
    attachInterrupt(
        digitalPinToInterrupt(PICKUP_PIN),
        hallSensorISR,
        PICKUP_EDGE
    );

    Serial.printf("Pickup inductif : GPIO %d\n", PICKUP_PIN);
    Serial.printf("Cycles/impulsion: %d (4 temps)\n", CYCLES_PAR_IMPULSION);
    Serial.println("Pret.");

    updateDisplay();
}

// =====================================================
// Loop
// =====================================================
void loop() {
    static uint32_t lastDisplayMs = 0;
    static uint32_t lastLoggedRPM = UINT32_MAX;

    // --- Traitement de la nouvelle impulsion ---
    if (newPulseAvailable) {
        noInterrupts();
        const uint32_t interval = pulseInterval_us;
        newPulseAvailable = false;
        interrupts();

        currentRPM       = calculateRPM(interval);
        lastPulseMillis  = millis();

        if (!engineRunning) {
            // Premier démarrage : initialiser minRPM
            engineRunning = true;
            minRPM        = currentRPM;
            maxRPM        = currentRPM;
        }

        if (currentRPM < minRPM && currentRPM > 0) minRPM = currentRPM;
        if (currentRPM > maxRPM)                    maxRPM = currentRPM;
    }

    // --- Détection arrêt moteur ---
    if (engineRunning &&
        (millis() - lastPulseMillis > ENGINE_STOPPED_TIMEOUT_MS)) {
        engineRunning = false;
        currentRPM    = 0;
        Serial.println("Moteur arrete.");
    }

    // --- Mise à jour de l'affichage (max 10 Hz) ---
    const uint32_t now = millis();
    if (now - lastDisplayMs >= 100) {
        lastDisplayMs = now;
        updateDisplay();
    }

    // --- Log série (uniquement si RPM change de plus de 10) ---
    if (engineRunning && abs((int32_t)currentRPM - (int32_t)lastLoggedRPM) > 10) {
        lastLoggedRPM = currentRPM;
        Serial.printf("RPM: %4lu  |  MIN: %4lu  |  MAX: %4lu\n",
                       currentRPM, minRPM, maxRPM);
    }
}
