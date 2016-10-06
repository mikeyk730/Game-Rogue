#pragma once
#include "item.h"

struct Food : public Item
{
    static Item* CreateFood(); //factory

    Food(int which);

    virtual Item* Clone() const;
    virtual std::string Name() const;
    virtual std::string InventoryName() const;
    virtual bool IsMagic() const;
    virtual bool IsEvil() const;
    virtual int Worth() const;
};