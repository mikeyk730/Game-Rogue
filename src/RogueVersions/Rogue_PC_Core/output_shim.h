#pragma once
#include <memory>
#include <string>
#include <coord.h>

#define standend()          set_attr(0x07)
#define green()             set_attr(0x02)
#define red()               set_attr(0x04)
#define brown()             set_attr(0x06)
#define lmagenta()          set_attr(0x0d)
#define yellow()            set_attr(0x0e)
#define blue()              set_attr(0x01)
#define standout()          set_attr(0x70)
#define high()              set_attr(0x0f)
#define bold()              set_attr(0x70)

struct OutputInterface;

struct OutputShim
{
public:
    OutputShim(std::shared_ptr<OutputInterface> output);
    ~OutputShim();

    void clear();
    void redraw();

    //Turn cursor on and off
    bool cursor(bool ison);

    //get current cursor position
    void getrc(int *r, int *c);

    void clrtoeol();

    void addstr(const char *s);

    void set_attr(int bute);

    //winit(win_name): initialize window -- open disk window -- determine type of monitor -- determine screen memory location for dma
    void winit(bool narrow);

    void forcebw();

    //wdump(windex): dump the screen off to disk, the window is saved so that it can be retrieved using windex
    void wdump();

    //wrestor(windex): restore the window saved on disk
    void wrestor();

    //Some general drawing routines
    void box(int ul_r, int ul_c, int lr_r, int lr_c);

    //center a string according to how many columns there really are
    void center(int row, const char *string);

    void printw(const char* format, ...);

    //blot_out region (upper left row, upper left column) (lower right row, lower right column)
    void blot_out(int ul_row, int ul_col, int lr_row, int lr_col);

    void repchr(int chr, int cnt);

    //Clear the screen in an interesting fashion
    void implode();

    //drop_curtain: Close a door on the screen and redirect output to the temporary buffer
    void drop_curtain();

    void raise_curtain();

    void move(short y, short x);

    unsigned char curch();

    int lines() const;
    int columns() const;

    void stop_rendering();
    void resume_rendering();

    void add_text(Coord p, byte c);
    int add_text(byte c);

    void add_tile(Coord p, byte c);
    int add_tile(byte c);

    int mvinch(Coord p);
    void mvaddstr(Coord p, const std::string& s);

    void play_sound(const char* id);

    std::shared_ptr<OutputInterface> m_output_interface;
};
