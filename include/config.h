#pragma once

// =====================================================
// Tachymètre Tracteur Tondeuse
// Moteur : Briggs & Stratton 217707 (monocylindre 4T)
// Carte  : LilyGo TTGO T-Display (ESP32 + ST7789 1.14")
// =====================================================


// -------------------------------------------------------
// Capteur inductif – bobine autour du fil de bougie
// -------------------------------------------------------
// Enrouler 20 à 30 spires de fil fin (Ø 0.2–0.5 mm) autour
// du câble haute tension (fil de bougie).
// Circuit de conditionnement (3 composants) :
//
//   [bobine pickup] ──┬── D1 (1N4148, anode côté bobine) ──┬── GPIO 27
//                     │                                      │
//                    R1 (10 kΩ)                             R2 (10 kΩ) vers GND
//                     │                                      │
//                    GND                               D2 (Zener 3.3V, cathode côté GPIO)
//                                                            │
//                                                           GND
//
// D1 : redresse le pic positif de la bobine
// D2 : écrête à 3.3V pour protéger l'ESP32
// R1 : amortit les oscillations de la bobine
// R2 : pull-down (assure un niveau bas au repos)
// -------------------------------------------------------
#define PICKUP_PIN          27
#define PICKUP_EDGE         RISING   // Le pic induit est positif


// -------------------------------------------------------
// Configuration mesure RPM – moteur 4 temps monocylindre
// -------------------------------------------------------
// Sur un 4 temps, la bougie s'allume 1 fois toutes les
// 2 révolutions du vilebrequin.
// Formule : RPM = (60 000 000 µs × CYCLES_PAR_IMPULSION) / intervalle_µs
#define CYCLES_PAR_IMPULSION  2      // 4 temps = 1 étincelle / 2 tours

// Intervalle minimum entre deux impulsions valides (µs).
// À 4 500 RPM/4T : intervalle = 2×60 000 000/4500 ≈ 26 700 µs
// Seuil à 20 000 µs ≈ 6 000 RPM → protection anti-rebond.
#define MIN_PULSE_INTERVAL_US  20000UL

// Délai sans impulsion pour déclarer le moteur arrêté (ms)
#define ENGINE_STOPPED_TIMEOUT_MS  2000


// -------------------------------------------------------
// Plages RPM – Briggs & Stratton 217707
// -------------------------------------------------------
#define RPM_MAX_DISPLAY   4500   // Limite supérieure de la jauge

#define RPM_ZONE_IDLE     1500   // En-dessous : ralenti       (cyan)
#define RPM_ZONE_NORMAL   3200   // En-dessous : fonctionnement normal (vert)
#define RPM_ZONE_HIGH     3600   // En-dessous : vitesse élevée (jaune)
                                 // Au-dessus  : survitesse   (rouge)


// -------------------------------------------------------
// Affichage
// -------------------------------------------------------
// Dimensions en mode paysage (rotation = 1)
#define SCREEN_W  240
#define SCREEN_H  135
