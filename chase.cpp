//Code for one creature to chase another
//chase.c     1.32    (A.I. Design) 12/12/84

#include <stdlib.h>

#include "rogue.h"
#include "chase.h"
#include "fight.h"
#include "move.h"
#include "io.h"
#include "sticks.h"
#include "misc.h"
#include "curses.h"
#include "main.h"
#include "monsters.h"
#include "list.h"
#include "level.h"
#include "weapons.h"
#include "scrolls.h"
#include "pack.h"

#define DRAGONSHOT  5 //one chance in DRAGONSHOT that a dragon will flame

Coord ch_ret; //Where chasing takes you

//runners: Make all the running monsters move.
void runners()
{
  AGENT *monster, *next = NULL;
  int dist;

  for (monster = mlist; monster!=NULL; monster = next)
  {
    next = next(monster); //monster may be invalidated during iteration, save next here in case continue is hit
    if (!monster->is_held() && monster->is_running())
    {
      dist = DISTANCE(player.pos.y, player.pos.x, monster->pos.y, monster->pos.x);
      if (!(monster->is_slow() || (monster->can_divide() && dist>3)) || monster->turn)
        if(!do_chase(monster)) 
            continue;
      if (monster->is_fast()) 
        if(!do_chase(monster)) 
            continue;
      dist = DISTANCE(player.pos.y, player.pos.x, monster->pos.y, monster->pos.x);
      if (monster->is_flying() && dist>3)
        if(!do_chase(monster)) 
            continue;
      monster->turn ^= true;
    }
    next = next(monster);
  }
}

//do_chase: Make one thing chase another.
bool do_chase(AGENT *monster)
{
  int mindist = 32767, i, dist;
  bool door;
  ITEM *obj;
  struct Room *oroom;
  struct Room *monster_room, *dest_room; //room of chaser, room of chasee
  Coord tempdest; //Temporary destination for chaser

  monster_room = monster->room; //Find room of chaser
  if (monster->is_greedy() && monster_room->goldval == 0) 
    monster->dest = &player.pos; //If gold has been taken, run after hero

  dest_room = player.room;
  if (monster->dest != &player.pos) 
    dest_room = roomin(monster->dest); //Find room of chasee
  if (dest_room==NULL) 
    return true;

  //We don't count doors as inside rooms for this routine
  door = (get_tile(monster->pos.y, monster->pos.x)==DOOR);
  //If the object of our desire is in a different room, and we are not in a maze, run to the door nearest to our goal.

over:

  if (monster_room!=dest_room && (monster_room->flags&IS_MAZE)==0)
  {
    //loop through doors
    for (i = 0; i<monster_room->num_exits; i++)
    {
      dist = DISTANCE(monster->dest->y, monster->dest->x, monster_room->exits[i].y, monster_room->exits[i].x);
      if (dist<mindist) {tempdest = monster_room->exits[i]; mindist = dist;}
    }
    if (door)
    {
      monster_room = &passages[get_flags(monster->pos.y, monster->pos.x)&F_PNUM];
      door = false;
      goto over;
    }
  }
  else
  {
    tempdest = *monster->dest;
    //For monsters which can fire bolts at the poor hero, we check to see if (a) the hero is on a straight line from it, and (b) that it is within shooting distance, but outside of striking range.
    if ((monster->shoots_fire() || monster->shoots_ice()) && 
        (monster->pos.y==player.pos.y || monster->pos.x==player.pos.x || abs(monster->pos.y-player.pos.y)==abs(monster->pos.x-player.pos.x)) &&
        ((dist = DISTANCE(monster->pos.y, monster->pos.x, player.pos.y, player.pos.x))>2 && dist<=BOLT_LENGTH*BOLT_LENGTH) && !monster->powers_cancelled() && rnd(DRAGONSHOT)==0)
    {
      running = false;
      delta.y = sign(player.pos.y-monster->pos.y);
      delta.x = sign(player.pos.x-monster->pos.x);
      return fire_bolt(&monster->pos, &delta, monster->shoots_fire()?"flame":"frost");
    }
  }
  //This now contains what we want to run to this time so we run to it. If we hit it we either want to fight it or stop running
  chase(monster, &tempdest);
  if (equal(ch_ret, player.pos)) {
    return attack(monster); 
  }
  else if (equal(ch_ret, *monster->dest))
  {
    for (obj = lvl_obj; obj!=NULL; obj = next(obj)) if (monster->dest==&obj->pos)
    {
      byte oldchar;

      detach_item(&lvl_obj, obj);
      attach_item(&monster->pack, obj);
      oldchar = (monster->room->flags&IS_GONE)?PASSAGE:FLOOR;
      set_tile(obj->pos.y, obj->pos.x, oldchar);
      if (can_see(obj->pos.y, obj->pos.x)) mvaddch(obj->pos.y, obj->pos.x, oldchar);
      monster->dest = find_dest(monster);
      break;
    }
  }
  if (monster->is_stationary()) 
      return true;
  //If the chasing thing moved, update the screen
  if (monster->oldch!='@')
  {
    if (monster->oldch==' ' && can_see(monster->pos.y, monster->pos.x) && get_tile(monster->pos.y, monster->pos.x)==FLOOR)
      mvaddch(monster->pos.y, monster->pos.x, (char)FLOOR);
    else if (monster->oldch == FLOOR && !can_see(monster->pos.y, monster->pos.x) && !player.detects_others())
      mvaddch(monster->pos.y, monster->pos.x, ' ');
    else
      mvaddch(monster->pos.y, monster->pos.x, monster->oldch);
  }
  oroom = monster->room;
  if (!equal(ch_ret, monster->pos))
  {
    if ((monster->room = roomin(&ch_ret))==NULL) {
        monster->room = oroom; 
        return true;
    }
    if (oroom!=monster->room) monster->dest = find_dest(monster);
    monster->pos = ch_ret;
  }
  if (can_see_monst(monster))
  {
    if (get_flags(ch_ret.y, ch_ret.x)&F_PASS) standout();
    monster->oldch = mvinch(ch_ret.y, ch_ret.x);
    mvaddch(ch_ret.y, ch_ret.x, monster->disguise);
  }
  else if (player.detects_others())
  {
    standout();
    monster->oldch = mvinch(ch_ret.y, ch_ret.x);
    mvaddch(ch_ret.y, ch_ret.x, monster->type);
  }
  else 
    monster->oldch = '@';
  if (monster->oldch==FLOOR && oroom->flags&IS_DARK) monster->oldch = ' ';
  standend();
  return true;
}

//see_monst: Return true if the hero can see the monster
bool can_see_monst(AGENT *monster)
{
  // player is blind
  if (player.is_blind())
    return false;

  //monster is invisible, and can't see invisible
  if (monster->is_invisible() && !player.sees_invisible())
    return false;
  
  if (DISTANCE(monster->pos.y, monster->pos.x, player.pos.y, player.pos.x) >= LAMP_DIST &&
    ((monster->room != player.room || (monster->room->flags & IS_DARK) || (monster->room->flags & IS_MAZE)))) 
    return false;
  
  //If we are seeing the enemy of a vorpally enchanted weapon for the first time, 
  //give the player a hint as to what that weapon is good for.
  if (is_vorpalized(get_current_weapon(), monster) && !(get_current_weapon()->flags & DID_FLASH))
  {
    get_current_weapon()->flags |= DID_FLASH;
    msg(flash, get_weapon_name(get_current_weapon()->which), short_msgs()?"":intense);
  }
  return true;
}

//start_run: Set a monster running after something or stop it from running (for when it dies)
void start_run(AGENT* monster)
{
  //Start the beastie running
  monster->flags |= IS_RUN;
  monster->flags &= ~IS_HELD;
  monster->dest = find_dest(monster);
}

//chase: Find the spot for the chaser(er) to move closer to the chasee(ee). Returns true if we want to keep on chasing later. false if we reach the goal.
void chase(AGENT *monster, Coord *chasee_pos)
{
  int x, y;
  int dist, thisdist;
  ITEM *obj;
  Coord *chaser_pos;
  byte ch;
  int plcnt = 1;

  chaser_pos = &monster->pos;
  //If the thing is confused, let it move randomly. Phantoms are slightly confused all of the time, and bats are quite confused all the time
  if (monster->is_monster_confused_this_turn())
  {
    //get a valid random move
    rndmove(monster, &ch_ret);
    dist = DISTANCE(ch_ret.y, ch_ret.x, chasee_pos->y, chasee_pos->x);
    //Small chance that it will become un-confused
    if (rnd(30)==17) monster->flags &= ~IS_HUH;
  }
  //Otherwise, find the empty spot next to the chaser that is closest to the chasee.
  else
  {
    int ey, ex;

    //This will eventually hold where we move to get closer. If we can't find an empty spot, we stay where we are.
    dist = DISTANCE(chaser_pos->y, chaser_pos->x, chasee_pos->y, chasee_pos->x);
    ch_ret = *chaser_pos;
    ey = chaser_pos->y+1;
    ex = chaser_pos->x+1;
    for (x = chaser_pos->x-1; x<=ex; x++)
    {
      for (y = chaser_pos->y-1; y<=ey; y++)
      {
        Coord tryp;

        tryp.x = x;
        tryp.y = y;
        if (offmap(y, x) || !diag_ok(chaser_pos, &tryp)) continue;
        ch = get_tile_or_monster(y, x);
        if (step_ok(ch))
        {
          //If it is a scroll, it might be a scare monster scroll so we need to look it up to see what type it is.
          if (ch==SCROLL)
          {
            for (obj = lvl_obj; obj!=NULL; obj = next(obj))
            {
              if (y==obj->pos.y && x==obj->pos.x) break;
            }
            if (is_scare_monster_scroll(obj)) continue;
          }
          //If we didn't find any scrolls at this place or it wasn't a scare scroll, then this place counts
          thisdist = DISTANCE(y, x, chasee_pos->y, chasee_pos->x);
          if (thisdist<dist) {plcnt = 1; ch_ret = tryp; dist = thisdist;}
          else if (thisdist==dist && rnd(++plcnt)==0) {ch_ret = tryp; dist = thisdist;}
        }
      }
    }
  }
}

//diag_ok: Check to see if the move is legal if it is diagonal
int diag_ok( Coord *sp, Coord *ep )
{
  if (ep->x==sp->x || ep->y==sp->y) return true;
  return (step_ok(get_tile(ep->y, sp->x)) && step_ok(get_tile(sp->y, ep->x)));
}

//can_see: Returns true if the hero can see a certain coordinate.
int can_see(int y, int x)
{
  struct Room *room;
  Coord tp;

  if (player.is_blind()) return false;
  if (DISTANCE(y, x, player.pos.y, player.pos.x)<LAMP_DIST) return true;
  //We can only see if the hero in the same room as the coordinate and the room is lit or if it is close.
  tp.y = y;
  tp.x = x;
  room = roomin(&tp);
  return (room==player.room && !(room->flags&IS_DARK));
}

//find_dest: find the proper destination for the monster
Coord *find_dest(AGENT *monster)
{
  ITEM *obj;
  int prob;
  struct Room *room;

  if ((prob = monster->get_monster_carry_prob()) <= 0 || monster->room == player.room || can_see_monst(monster))
    return &player.pos;
  room = monster->room;
  for (obj = lvl_obj; obj!=NULL; obj = next(obj))
  {
    if (is_scare_monster_scroll(obj)) continue;
    if (roomin(&obj->pos)==room && rnd(100)<prob)
    {
      for (monster = mlist; monster!=NULL; monster = next(monster)) if (monster->dest==&obj->pos) break;
      if (monster==NULL) return &obj->pos;
    }
  }
  return &player.pos;
}
