#pragma once

// =====================================================
// Tachymètre Tracteur Tondeuse
// Moteur : Briggs & Stratton 217707 (monocylindre 4T)
// Carte  : LilyGo TTGO T-Display (ESP32 + ST7789 1.14")
// =====================================================


// -------------------------------------------------------
// Brochage capteur Hall
// -------------------------------------------------------
// Connecter le capteur Hall (ex. A3144 / SS49E) sur GPIO 27.
// Le capteur doit être positionné face aux aimants du volant
// moteur (volant d'allumage / magnéto).
// Alimentation capteur : 3.3V ou 5V selon modèle.
// -------------------------------------------------------
#define HALL_SENSOR_PIN     27
// FALLING : capteur actif-bas (sortie open-drain + pull-up interne)
// RISING  : capteur actif-haut
#define HALL_SENSOR_EDGE    FALLING


// -------------------------------------------------------
// Configuration mesure RPM
// -------------------------------------------------------
// Nombre d'impulsions générées par tour de vilebrequin.
// Le volant B&S 217707 possède un aimant principal pour la
// bobine d'allumage → 1 impulsion / tour.
// Si votre capteur détecte 2 aimants, mettez 2.
#define PULSES_PER_REVOLUTION  1

// Intervalle minimum entre deux impulsions valides (µs).
// Sert de filtre anti-rebond matériel.
// 8 000 µs ≈ 7 500 RPM max → largement au-dessus du B&S.
#define MIN_PULSE_INTERVAL_US  8000UL

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
