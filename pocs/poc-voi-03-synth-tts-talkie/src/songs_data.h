#ifndef VOI03_SONGS_DATA_H
#define VOI03_SONGS_DATA_H

#include <stdint.h>
#include "config.h"

struct MelodyNote {
    uint16_t freq;
    int8_t duration; // Dương: nốt thường (4=quarter, 8=eighth), Âm: nốt chấm đôi (dotted)
};

struct SongInfo {
    const char* title;
    uint16_t defaultBpm;
    const MelodyNote* notes;
    uint16_t length;
};

// 1. Super Mario Bros Theme
static const MelodyNote SONG_MARIO[] = {
    {NOTE_E5, 8}, {NOTE_E5, 8}, {NOTE_REST, 8}, {NOTE_E5, 8},
    {NOTE_REST, 8}, {NOTE_C5, 8}, {NOTE_E5, 8}, {NOTE_REST, 8},
    {NOTE_G5, 4}, {NOTE_REST, 4}, {NOTE_G4, 4}, {NOTE_REST, 4},
    {NOTE_C5, -4}, {NOTE_REST, 8}, {NOTE_G4, -4}, {NOTE_REST, 8},
    {NOTE_E4, -4}, {NOTE_REST, 8}, {NOTE_A4, 4}, {NOTE_B4, 4},
    {NOTE_AS4, 8}, {NOTE_A4, 4}, {NOTE_G4, -8}, {NOTE_E5, -8},
    {NOTE_G5, -8}, {NOTE_A5, 4}, {NOTE_F5, 8}, {NOTE_G5, 8},
    {NOTE_REST, 8}, {NOTE_E5, 4}, {NOTE_C5, 8}, {NOTE_D5, 8}, {NOTE_B4, 4}
};

// 2. Star Wars Imperial March
static const MelodyNote SONG_STAR_WARS[] = {
    {NOTE_A4, 4}, {NOTE_A4, 4}, {NOTE_A4, 4}, {NOTE_F4, -8}, {NOTE_C5, 16},
    {NOTE_A4, 4}, {NOTE_F4, -8}, {NOTE_C5, 16}, {NOTE_A4, 2},
    {NOTE_E5, 4}, {NOTE_E5, 4}, {NOTE_E5, 4}, {NOTE_F5, -8}, {NOTE_C5, 16},
    {NOTE_GS4, 4}, {NOTE_F4, -8}, {NOTE_C5, 16}, {NOTE_A4, 2},
    {NOTE_A5, 4}, {NOTE_A4, -8}, {NOTE_A4, 16}, {NOTE_A5, 4}, {NOTE_GS5, -8}, {NOTE_G5, 16},
    {NOTE_FS5, 16}, {NOTE_F5, 16}, {NOTE_FS5, 8}, {NOTE_REST, 8}, {NOTE_AS4, 8}, {NOTE_DS5, 4}
};

// 3. Tetris Theme (Korobeiniki)
static const MelodyNote SONG_TETRIS[] = {
    {NOTE_E5, 4}, {NOTE_B4, 8}, {NOTE_C5, 8}, {NOTE_D5, 4}, {NOTE_C5, 8}, {NOTE_B4, 8},
    {NOTE_A4, 4}, {NOTE_A4, 8}, {NOTE_C5, 8}, {NOTE_E5, 4}, {NOTE_D5, 8}, {NOTE_C5, 8},
    {NOTE_B4, -4}, {NOTE_C5, 8}, {NOTE_D5, 4}, {NOTE_E5, 4},
    {NOTE_C5, 4}, {NOTE_A4, 4}, {NOTE_A4, 4}, {NOTE_REST, 4},
    {NOTE_D5, -4}, {NOTE_F5, 8}, {NOTE_A5, 4}, {NOTE_G5, 8}, {NOTE_F5, 8},
    {NOTE_E5, -4}, {NOTE_C5, 8}, {NOTE_E5, 4}, {NOTE_D5, 8}, {NOTE_C5, 8},
    {NOTE_B4, 4}, {NOTE_B4, 8}, {NOTE_C5, 8}, {NOTE_D5, 4}, {NOTE_E5, 4},
    {NOTE_C5, 4}, {NOTE_A4, 4}, {NOTE_A4, 2}
};

// 4. Ode to Joy (Beethoven)
static const MelodyNote SONG_ODE_TO_JOY[] = {
    {NOTE_E4, 4}, {NOTE_E4, 4}, {NOTE_F4, 4}, {NOTE_G4, 4},
    {NOTE_G4, 4}, {NOTE_F4, 4}, {NOTE_E4, 4}, {NOTE_D4, 4},
    {NOTE_C4, 4}, {NOTE_C4, 4}, {NOTE_D4, 4}, {NOTE_E4, 4},
    {NOTE_E4, -4}, {NOTE_D4, 8}, {NOTE_D4, 2},
    {NOTE_E4, 4}, {NOTE_E4, 4}, {NOTE_F4, 4}, {NOTE_G4, 4},
    {NOTE_G4, 4}, {NOTE_F4, 4}, {NOTE_E4, 4}, {NOTE_D4, 4},
    {NOTE_C4, 4}, {NOTE_C4, 4}, {NOTE_D4, 4}, {NOTE_E4, 4},
    {NOTE_D4, -4}, {NOTE_C4, 8}, {NOTE_C4, 2}
};

static const SongInfo SONG_CATALOG[] = {
    {"Super Mario Theme", 140, SONG_MARIO, sizeof(SONG_MARIO) / sizeof(SONG_MARIO[0])},
    {"Star Wars Imperial", 108, SONG_STAR_WARS, sizeof(SONG_STAR_WARS) / sizeof(SONG_STAR_WARS[0])},
    {"Tetris Korobeiniki", 130, SONG_TETRIS, sizeof(SONG_TETRIS) / sizeof(SONG_TETRIS[0])},
    {"Ode To Joy", 120, SONG_ODE_TO_JOY, sizeof(SONG_ODE_TO_JOY) / sizeof(SONG_ODE_TO_JOY[0])}
};
static const uint8_t TOTAL_SONGS = sizeof(SONG_CATALOG) / sizeof(SONG_CATALOG[0]);

#endif // VOI03_SONGS_DATA_H
