#pragma once
#include "item.h"

struct Agent;

struct Potion : public Item
{
    Potion(int which);

    virtual Item* Clone() const;
    virtual std::string Name() const;
    virtual std::string InventoryName() const;
    virtual bool IsEvil() const;
    virtual int Worth() const;

    //todo: each potion should be own derived class with quaff as virtual function
    void quaff_confusion();
    void quaff_paralysis();
    void quaff_poison();
    void quaff_gain_strength();
    void quaff_see_invisible();
    void quaff_healing();
    void quaff_monster_detection();
    void quaff_magic_detection();
    void quaff_raise_level();
    void quaff_extra_healing();
    void quaff_haste_self();
    void quaff_restore_strength();
    void quaff_blindness();
    void quaff_thirst_quenching();

};

Item* create_potion();

//quaff: Quaff a potion from the pack
bool quaff();

//show_invisible: Turn on the ability to see invisible
void show_invisible();

//turn_see: Put on or off seeing monsters on this m_level
bool turn_see(bool turn_off);
void turn_see_wrapper(int turn_off);

//th_effect: Compute the effect of this potion hitting a monster.
void affect_monster(Item *obj, Monster *monster);
