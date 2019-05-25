//global variable initializaton
//@(#)extern.c5.2 (Berkeley) 6/16/82
#include <iostream>
#include <list>
#include "rogue.h"
#include "agent.h"
#include "item.h"

struct Random;
struct GameState;

#define MAXHELPC  65
const char* const helpcoms[MAXHELPC] =
{
  "F1     list of commands",
  "F2     list of symbols",
  "F3     repeat command",
  "F4     repeat message",
  "F5     rename something",
  "F6     recall what's been discovered",
  "F7     inventory of your possessions",
  "F8     <dir> identify trap type",
  "F9     The Any Key (definable)",
  "Alt F9 defines the Any Key",
  "F10    Supervisor Key (fake dos)",
  "Space  Clear -More- message",
  "\x11\xd9     the Enter Key",
  "\x1b      left",
  "\x19      down",
  "\x18      up",
  "\x1a      right",
  "Home   up & left",
  "PgUp   up & right",
  "End    down & left",
  "PgDn   down & right",
  "Scroll Fast Play mode",
  ".      rest",
  ">      go down a staircase",
  "<      go up a staircase",
  "Esc    cancel command",
  "d      drop object",
  "e      eat food",
  "f      <dir> find something",
  "q      quaff potion",
  "r      read paper",
  "s      search for trap/secret door",
  "t      <dir> throw something",
  "w      wield a weapon",
  "z      <dir> zap with a wand",
  "B      run down & left",
  "H      run left",
  "J      run down",
  "K      run up",
  "L      run right",
  "N      run down & right",
  "U      run up & right",
  "Y      run up & left",
  "W      wear armor",
  "T      take armor off",
  "P      put on ring",
  "Q      quit",
  "R      remove ring",
  "S      save game",
  "^      identify trap",
  "?      help",
  "/      key",
  "+      throw",
  "-      zap",
  "Ctrl t terse message format",
  "Ctrl r repeat message",
  "Del    search for something hidden",
  "Ins    <dir> find something",
  "a      repeat command",
  "c      rename something",
  "i      inventory",
  "v      version number",
  "!      Supervisor Key (fake DOS)",
  "D      list what has been discovered",
  0
};

#define MAXHELPO  24
const char* const helpobjs[MAXHELPO] =
{
  "\xfa: the floor",
  "\x1: the hero",
  "\x5: some food",
  "\xc: the amulet of yendor",
  "\xd: a scroll",
  "\x18: a weapon",
  "\x8: a piece of armor",
  "\xf: some gold",
  "\xe7: a magic staff",
  "\xad: a potion",
  "\x9: a magic ring",
  "\xb1: a passage",

  //make sure in 40 or 80 column none of line draw set connects. this is currently in column 1 for 80

  "\xce: a door",
  "\xc9: an upper left corner",
  "\x04: a trap",
  "\xcd: a horizontal wall",
  "\xbc: a lower right corner",
  "\xc8: a lower left corner",
  "\xba: a vertical wall",
  "\xbb: an upper right corner",
  "\xf0: a stair case",
  "$,+: safe and perilous magic",
  "A-Z: 26 different monsters",
  0
};

//Names of the various experience levels

const char* const level_titles[] =
{
  "",
  "Guild Novice",
  "Apprentice",
  "Journeyman",
  "Adventurer",
  "Fighter",
  "Warrior",
  "Rogue",
  "Champion",
  "Master Rogue",
  "Warlord",
  "Hero",
  "Guild Master",
  "Dragonlord",
  "Wizard",
  "Rogue Geek",
  "Rogue Addict",
  "Schmendrick",
  "Gunfighter",
  "Time Waster",
  "Bug Chaser"
};

const char* const intense = " of intense white light";
const char* const flash_msg = "your %s gives off a flash%s";

bool operator==(const Coord& c1, const Coord& c2)
{
    return c1.x == c2.x && c1.y == c2.y;
}

bool operator!=(const Coord & c1, const Coord & c2)
{
    return !(c1 == c2);
}

Coord operator+(const Coord & c1, const Coord & c2)
{
    return{ c1.x + c2.x, c1.y + c2.y };
}

std::ostream & operator<<(std::ostream & os, Coord c)
{
    os << "{" << c.x << "," << c.y << "}";
    return os;
}

char prbuf[MAXSTR];    //Printing buffer used everywhere

Random* g_random;
GameState* game;
