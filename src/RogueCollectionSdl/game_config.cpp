#include "game_config.h"
#include "environment.h"
#include "../Shared/Frontend/dll_constants.h"

TextConfig pc_text =         { "text.bmp",  { 256, 1 }, { 0x07 }, false };
TextConfig pc_colored_text = { "text.bmp",  { 256, 1 }, { 0x07 }, true  };
TextConfig alt_text =        { "tilemap_text.bmp", { 16, 16 }, { 0x0F }, true  };
TextConfig boxy_text =       { "boxy.bmp",  { 16, 16 }, { 0x0F }, true  };

FontConfig placeholder = { "", 16 };
                                 //name          text_cfg          font_cfg      tile_cfg      unix    color  stndout animate
GraphicsConfig unix_gfx =        { "unix",       &pc_text,         &placeholder, 0,            true,   false,  false,  false };
GraphicsConfig color_unix_gfx =  { "unix_color", &pc_colored_text, &placeholder, 0,            true,   true,   true,   true  };
GraphicsConfig pc_gfx =          { "pc",         &pc_colored_text, &placeholder, 0,            false,  true,   true,   true  };
GraphicsConfig tilemap_v1_gfx =  { "tiles",      &alt_text,        0,            &tilemap_v1,  false,  false,  true,   false };
GraphicsConfig tilemap_v2_gfx =  { "tiles",      &alt_text,        0,            &tilemap_v2,  false,  false,  true,   false };
GraphicsConfig tilemap_v3_gfx =  { "tiles",      &alt_text,        0,            &tilemap_v3,  false,  false,  true,   false };
GraphicsConfig tilemap_v4_gfx =  { "tiles",      &alt_text,        0,            &tilemap_v4,  false,  false,  true,   false };
GraphicsConfig boxy_gfx =        { "boxy",       &boxy_text,       0,            0,            false,  true,   true,   true  };

#include "../Shared/Frontend/options.h"

void SetFontFile(const std::string & filename)
{
    placeholder.fontfile = filename;
}

void SetFontSize(int size)
{
    placeholder.size = size;
}

void InitGameConfig(Environment* env)
{
    std::string value;
    if (env->Get("font", &value)) {
        SetFontFile(value);
        if (env->Get("font_size", &value)) {
            int size = atoi(value.c_str());
            if (size) {
                SetFontSize(size);
            }
        }
    }
}
