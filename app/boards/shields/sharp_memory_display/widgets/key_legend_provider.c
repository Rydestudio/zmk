#include "key_legend_provider.h"

typedef struct {
    uint16_t key_code;
    const char *base;
    const char *shifted;
} key_map_entry;

static const key_map_entry key_map[] = {
    // Letters
    {4, "a", "A"},
    {5, "b", "B"},
    {6, "c", "C"},
    {7, "d", "D"},
    {8, "e", "E"},
    {9, "f", "F"},
    {10, "g", "G"},
    {11, "h", "H"},
    {12, "i", "I"},
    {13, "j", "J"},
    {14, "k", "K"},
    {15, "l", "L"},
    {16, "m", "M"},
    {17, "n", "N"},
    {18, "o", "O"},
    {19, "p", "P"},
    {20, "q", "Q"},
    {21, "r", "R"},
    {22, "s", "S"},
    {23, "t", "T"},
    {24, "u", "U"},
    {25, "v", "V"},
    {26, "w", "W"},
    {27, "x", "X"},
    {28, "y", "Y"},
    {29, "z", "Z"},

    // Numbers
    {30, "1", "!"},
    {31, "2", "@"},
    {32, "3", "#"},
    {33, "4", "$"},
    {34, "5", "%"},
    {35, "6", "^"},
    {36, "7", "&"},
    {37, "8", "*"},
    {38, "9", "("},
    {39, "0", ")"},

    // Function Keys
    {58, "F1", "F1"},
    {59, "F2", "F2"},
    {60, "F3", "F3"},
    {61, "F4", "F4"},
    {62, "F5", "F5"},
    {63, "F6", "F6"},
    {64, "F7", "F7"},
    {65, "F8", "F8"},
    {66, "F9", "F9"},
    {67, "F10", "F10"},
    {68, "F11", "F11"},
    {69, "F12", "F12"},

    // Common Keys
    {40, "ENT", "ENT"},
    {41, "ESC", "ESC"},
    {42, "BSPC", "BSPC"},
    {43, "TAB", "TAB"},
    {44, "SPC", "SPC"},
    {45, "-", "_"},
    {46, "=", "+"},
    {47, "[", "{"},
    {48, "]", "}"},
    {49, "\\", "|"},
    {51, ";", ":"},
    {52, "'", "\""},
    {53, "`", "~"},
    {54, ",", "<"},
    {55, ".", ">"},
    {56, "/", "?"},
    {76, "DEL", "DEL"},

    // Modifiers
    {224, "LCTL", "LCTL"},
    {225, "LSFT", "LSFT"},
    {226, "LALT", "LALT"},
    {227, "LGUI", "LGUI"},
    {228, "RCTL", "RCTL"},
    {229, "RSFT", "RSFT"},
    {230, "RALT", "RALT"},
    {231, "RGUI", "RGUI"},

    // Navigation
    {74, "HOME", "HOME"},
    {75, "PG_UP", "PG_UP"},
    {77, "END", "END"},
    {78, "PG_DN", "PG_DN"},
    {79, "RIGHT", "RIGHT"},
    {80, "LEFT", "LEFT"},
    {81, "DOWN", "DOWN"},
    {82, "UP", "UP"},

    // Keypad
    {83, "NUM", "NUM"},
    {84, "KPSLH", "KPSLH"},
    {85, "KPAST", "KPAST"},
    {86, "KPMIN", "KPMIN"},
    {87, "KPPLS", "KPPLS"},
    {88, "KPENT", "KPENT"},
    {89, "KP1", "KP1"},
    {90, "KP2", "KP2"},
    {91, "KP3", "KP3"},
    {92, "KP4", "KP4"},
    {93, "KP5", "KP5"},
    {94, "KP6", "KP6"},
    {95, "KP7", "KP7"},
    {96, "KP8", "KP8"},
    {97, "KP9", "KP9"},
    {98, "KP0", "KP0"},
    {99, "KPDOT", "KPDOT"},

    // Extra Keys
    {70, "PRNT", "PRNT"},
    {71, "SLCK", "SLCK"},
    {72, "PAUS", "PAUS"},
    {73, "INS", "INS"},
    {101, "APP", "APP"},
    {57, "CAPS", "CAPS"},
};

const char *get_key_name(uint16_t key_code, bool shift) {
    for (size_t i = 0; i < sizeof(key_map) / sizeof(key_map[0]); i++) {
        if (key_map[i].key_code == key_code) {
            return shift ? key_map[i].shifted : key_map[i].base;
        }
    }
    return NULL;
}