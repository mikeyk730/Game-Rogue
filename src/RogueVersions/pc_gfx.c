#include <curses_ex.h>
#include "pc_gfx_macros.h"

#ifdef ROGUE_COLLECTION
jmp_buf exception_env;

void init_game(struct DisplayInterface* screen, struct InputInterface* input, int lines, int cols)
{
    init_curses(screen, input, lines, cols);
}
#endif

#ifdef USE_PC_STYLE
#include <pc_gfx_charmap.h>
#include <curses.h>

void setup_colors()
{
#define init_color_pair(attr) init_pair((attr), (attr) & 0x0f, ((attr) & 0xf0) >> 4)
    start_color();
    init_color(0x00, 0, 0, 0); //black
    init_color(0x01, 0, 0, 667); //blue
    init_color(0x02, 0, 667, 0); //green
    init_color(0x03, 0, 667, 667); //cyan
    init_color(0x04, 667, 0, 0); //red
    init_color(0x05, 667, 0, 667); //magenta
    init_color(0x06, 667, 333, 0); //brown
    init_color(0x07, 667, 667, 667); //grey
    init_color(0x08, 250, 250, 250); //d_grey
    init_color(0x09, 333, 333, 1000); //l_blue
    init_color(0x0a, 333, 1000, 333); //l_green
    init_color(0x0b, 100, 1000, 1000); //l_cyan
    init_color(0x0c, 1000, 333, 333); //l_red
    init_color(0x0d, 1000, 100, 1000); //l_magenta
    init_color(0x0e, 1000, 1000, 100); //yellow
    init_color(0x0f, 1000, 1000, 1000); //white
    init_color_pair(0x01);
    init_color_pair(0x02);
    init_color_pair(0x04);
    init_color_pair(0x05);
    init_color_pair(0x06);
    init_color_pair(0x07);
    init_color_pair(0x09);
    init_color_pair(0x0A);
    init_color_pair(0x0D);
    init_color_pair(0x0E);
    init_color_pair(0x70);
    init_color_pair(0x71);
    init_color_pair(0x72);
    init_color_pair(0x74);
    init_color_pair(0x78);
    init_color_pair(0x7E);
    init_color_pair(0xA0);
    curs_set(0);
}

int reverse_translate_type(int t)
{
    switch (t) {
    case GOLD:   return '*';
    case POTION: return '!';
    case SCROLL: return '?';
    case MAGIC:  return '$';
    case FOOD:   return ':';
    case WEAPON: return ')';
    case ARMOR:  return ']';
    case AMULET: return ',';
    case RING:   return '=';
    case STICK:  return '/';
    }
    return t;
}

int translate_type(int t)
{
    switch (t) {
    case '*': return GOLD;
    case '!': return POTION;
    case '?': return SCROLL;
    case '$': return MAGIC;
    case ':': return FOOD;
    case ')': return WEAPON;
    case ']': return ARMOR;
    case ',': return AMULET;
    case '=': return RING;
    case '/': return STICK;
    case '|': return VWALL;
    case '-': return HWALL;
    case '%': return STAIRS;
    case '+': return DOOR;
    case '.': return FLOOR;
    case '#': return PASSAGE;
    case '@': return PLAYER;
    case '^': return TRAP;
    }
    return t;
}

void fix_tombstone()
{
    mvchgat(19, 0, -1, 0, 0x04, 0); // red flower row
    mvchgat(20, 0, -1, 0, 0x02, 0); // green grass row

    mvchgat(10, 22, 11, 0, 0x00, 0); // default text
    mvchgat(11, 22, 11, 0, 0x00, 0);
    mvchgat(12, 22, 11, 0, 0x00, 0);
    mvchgat(16, 22, 11, 0, 0x00, 0);

    mvchgat(19, 18, 1, 0, 0x06, 0); // fix tombstone walls
    mvchgat(19, 37, 1, 0, 0x06, 0);
    mvchgat(20, 37, 1, 0, 0x06, 0);
}
#endif