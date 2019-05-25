//Read and execute the user commands
//command.c   1.44    (A.I. Design)   2/14/85

#include <map>
#include <algorithm>
#include <ctype.h>
#include "random.h"
#include "game_state.h"
#include "daemons.h"
#include "daemon.h"
#include "command.h"
#include "main.h"
#include "io.h"
#include "wizard.h"
#include "misc.h"
#include "save.h"
#include "armor.h"
#include "weapons.h"
#include "stick.h"
#include "output_shim.h"
#include "mach_dep.h"
#include "move.h"
#include "ring.h"
#include "things.h"
#include "potion.h"
#include "pack.h"
#include "scroll.h"
#include "level.h"
#include "food.h"
#include "hero.h"
#include "commands.h"

namespace
{
    std::map<int, bool(*)()> s_commands =
    {
        //proper actions
        { 'e', do_eat },
        { 'w', do_wield },
        { 'W', do_wear_armor },
        { 'T', do_take_off_armor },
        { 'P', do_put_on_ring },
        { 'R', do_remove_ring },
        { 't', do_throw_item },
        { 'z', do_zap },
        { 'q', do_quaff },
        { 'r', do_read_scroll },
        { 'd', do_drop },
        { 's', do_search },
        { '.', do_rest },
        { '>', do_go_down_stairs },
        { '<', do_go_up_stairs },

        //informational/utility actions
        { 'i', do_inventory },
        { 'c', do_call },
        { 'D', do_discovered },
        { '^', do_id_trap },
        { 'F', do_record_macro },
        { 'S', do_save_game },
        { 'Q', do_quit },
        { 'v', do_print_version },
        { '/', do_object_help },
        { '?', do_command_help },
        { '!', do_fakedos },
        { 'o', do_options },

        { CTRL('T'), do_toggle_terse },
        { CTRL('F'), do_play_macro },
        { CTRL('R'), do_repeat_msg },
        //{ CTRL('L'), do_clear_screen },
        { CTRL('P'), do_toggle_wizard },
    };
    //commands that are recognized only in wizard mode
    //unmapped ctrl keys: qszvm
    std::map<int, bool(*)()> s_wizard_commands = {
        { '|', do_msg_position },
        { 'C', do_summon_object },
        { '$', do_msg_pack_count },
        { '~', do_charge_stick },
        { CTRL('I'), do_discovered },
        { CTRL('W'), do_reveal_all },
        { CTRL('D'), do_advance_level },
        { CTRL('A'), do_decrease_level },
        { CTRL('F'), do_show_map },
        { CTRL('T'), do_teleport },
        { CTRL('E'), do_msg_food },
        { CTRL('C'), do_add_passages },
        { CTRL('X'), do_toggle_detect },
        { CTRL('G'), do_add_goods },

        { CTRL('O'), do_toggle_powers },
        { CTRL('R'), do_raise_level },
    };
}


Command::Command()
{
}

int Command::decrement_count()
{
    --count;
    if (count < 0)
        count = 0;
    return count;
}

bool Command::is_move() const
{
    switch (ch) {
    case 'h': case 'j': case 'k': case 'l': case 'y': case 'u': case 'b': case 'n':
        return true;
    }
    return false;
}

bool Command::is_run() const
{
    switch (ch) {
    case 'H': case 'J': case 'K': case 'L': case 'Y': case 'U': case 'B': case 'N':
        return true;
    }
    return false;
}

void do_rings()
{
    for (int i = LEFT; i <= RIGHT; i++) {
        Ring* r = game->hero().get_ring(i);
        if (r) {
            r->OnTurn();
        }
    }
}

void advance_game()
{
    game->hero().set_num_actions(1);
    //mdk:bugfix: The potion of haste self does not work in the original code.  It's meant to
    //give you 2 - 3 actions per turn, but a bug prevented this from ever happening.
    //This bug isn't in Unix Rogue, where the player gets 2 turns while hasted
    if (game->hero().is_fast() || game->wizard().haste_self()) {
        int turns = game->options.act_like_v1_1() ? 2 : rnd(2) + 2;
        game->hero().set_num_actions(turns);
    }

    while (game->hero().decrement_num_actions())
    {
        update_status_bar();
        handle_key_state();
        if (game->hero().decrement_sleep_turns()) {
            tick_pause();
        }
        else {
            execute_player_command();
        }
        do_rings();  //mdk: This used to come after running fuses/daemons
    }
    do_fuses();
    do_daemons();
}

//handle aliases for commands
int translate_command(int ch)
{
    switch (ch)
    {
    //case '\b':
    //    return 'h';
    case '+':
        return 't';
    case '-':
        return 'z';
    }
    return ch;
}

int get_translated_char()
{
    int ch;
    ch = readchar();
    clear_msg();
    return translate_command(ch);
}

void process_prefixes(int ch, Command* command, bool* fast_mode)
{
    switch (ch)
    {
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
    {
        int n = command->count * 10;
        if ((n += ch - '0') > 0 && n < 10000)
            command->count = n;
        show_count(command->count);
        break;
    }
    case 'f': // f: turn on fast mode for this turn
        *fast_mode = true;
        break;
    case 'm': // m: move onto an item without picking it up
        if (game->options.act_like_v1_1())
            command->ch = ch;
        else
            command->can_pick_up = false;
        break;
    case 'a': //a: repeat last command
        *command = game->last_turn.command;
        game->repeat_last_action = true;
        break;
    case ' ': //eat spaces
        break;
    case ESCAPE:
        game->m_stop_at_door = false; //todo: why is this here?
        command->count = 0;
        show_count(command->count);
        break;
    default:
        command->ch = ch;
        break;
    }
}

void get_command_from_user(Command* command)
{
    bool fast_mode = false;

    while (command->ch == 0)
    {
        int ch = get_translated_char();
        process_prefixes(ch, command, &fast_mode);
    }

    fast_mode |= game->fast_play();

    switch (command->ch)
    {
    case 'h': case 'j': case 'k': case 'l': case 'y': case 'u': case 'b': case 'n':
        if (fast_mode && command->count == 0)
        {
            if (!game->hero().is_blind()) {
                game->m_stop_at_door = true;
                game->m_first_move = true;
            }
            command->ch = toupper(command->ch);
        }
        break;

        //actions that are repeatable
    case 'q': case 'r': case 's': case 'z': case 't': case '.': case 'C':
        break;

    default:
        command->count = 0;
    }
}

//Read a command, setting things up according to prefix like devices. Return the command character to be executed.
Command get_command()
{
    game->repeat_last_action = false;
    look(true);

    if (!game->in_run_cmd())
        game->m_stop_at_door = false;

    Command command;
    command.count = game->last_turn.command.count;
    show_count(command.count);

    // If we have a repeat count, use the last command
    if (command.count > 0) {
        command = game->last_turn.command;
    }
    // If we're running, use the global run character which indicates the direction
    // the hero should go next
    else if (game->in_run_cmd()) {
        command = game->last_turn.command;
        command.ch = game->run_character;
    }
    // Otherwise get the command from the user
    else {
        get_command_from_user(&command);
    }

    game->last_turn.command = command;
    game->last_turn.command.decrement_count();
    return command;
}

void show_count(int n)
{
    const int COLS = game->screen().columns();
    const int LINES = game->screen().lines();

    game->screen().move(LINES - 2, COLS - 4);
    if (n > 0)
        game->screen().printw("%-4d", n);
    else
        game->screen().addstr("    ");
}

bool dispatch_command(Command c)
{
    //handle directional movement commands
    if (c.is_move())
        return do_move(c);
    else if (c.is_run())
        return do_run(c);

    //if we're a wizard look at wizard commands first
    if (game->wizard().enabled()) {
        auto i = s_wizard_commands.find(c.ch);
        if (i != s_wizard_commands.end()) {
            return (*i->second)();
        }
    }

    //try executing the command from the map
    auto i = s_commands.find(c.ch);
    if (i != s_commands.end()) {
        return (*i->second)();
    }

    msg("illegal command '%s'", unctrl(c.ch));
    game->cancel_repeating_cmd();
    return false;
}


void execute_player_command()
{
    bool counts_as_turn;
    do
    {
        Command c = get_command();
        counts_as_turn = dispatch_command(c);

        //todo: why is this here?
        if (!game->in_run_cmd())
            game->m_stop_at_door = false;

    } while (!counts_as_turn);
}
