#pragma once
#include "item.h"

struct Monster;
struct Coord;

struct Weapon : public Item
{
    Weapon(int which);
    Weapon(int which, int hit, int damage);

    virtual Item* Clone() const override;
    virtual std::string TypeName() const override;
    virtual std::string InventoryName() const override;
    virtual bool IsMagic() const override;
    virtual bool IsEvil() const override;
    virtual int Worth() const override;

    void enchant_weapon();

    bool vorpalize();
    bool zap_vorpalized_weapon(Coord dir);
    bool is_vorpalized() const;
    bool is_vorpalized_against(Monster* monster) const;    
    bool did_flash() const;
    void set_flashed();
    std::string get_vorpalized_name() const;

private:
    void initialize_weapon(byte type);

protected:
    std::string m_name;
private:
    char enemy = 0; 
    bool m_flashed = false;
};

Item* create_weapon();

//do_throw_item: Fire a projectile in a given direction
bool do_throw_item();

//do_motion: Do the actual motion on the screen done by an object travelling across the room
void do_motion(Item *obj, Coord delta);

//projectile_hit: Does the projectile hit the monster?  Returns pointer to monster that may have been invalidated.  //todo:nix return value
Monster* projectile_hit(Coord p, Item *obj);

//num: Figure out the plus number for armor/weapons
char *num(int n1, int n2, char type);


