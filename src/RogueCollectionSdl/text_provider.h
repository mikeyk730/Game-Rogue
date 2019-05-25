#pragma once
#include <vector>
#include <map>
#include <coord.h>
#include <SDL.h>
#include "sdl_utility.h"
#include "game_config.h"

struct ITextProvider
{
    virtual ~ITextProvider();
    virtual Coord Dimensions() const = 0;
    virtual void GetTexture(int ch, int color, SDL_Texture** texture, SDL_Rect* rect) = 0;
};

struct TextProvider : ITextProvider
{
    TextProvider(const TextConfig& config, SDL_Renderer* renderer);
    ~TextProvider();
    Coord Dimensions() const override;
    void GetTexture(int ch, int color, SDL_Texture** texture, SDL_Rect* rect) override;

private:
    int GetTextIndex(unsigned short attr);
    SDL_Rect GetTextRect(unsigned char ch, int i);

    TextConfig m_cfg;
    SDL_Texture* m_text = nullptr;
    Coord m_text_dimensions = { 0, 0 };
    std::map<int, int> m_attr_index;
};

struct TextGenerator : ITextProvider
{
    TextGenerator(const TextConfig& config, SDL_Renderer* renderer);
    TextGenerator(const FontConfig& config, SDL_Renderer* renderer);
    ~TextGenerator();
    Coord Dimensions() const override;
    void GetTexture(int ch, int color, SDL_Texture** texture, SDL_Rect* rect) override;

private:
    SDL_Rect GetTextRect(unsigned char ch);
    void Init();

    TextConfig m_cfg;
    SDL_Renderer* m_renderer;
    SDL::Scoped::Surface m_text;
    Coord m_text_dimensions = { 0, 0 };
    std::map<int, SDL_Texture*> m_textures;
    std::vector<SDL_Color> m_colors;
};

std::unique_ptr<ITextProvider> CreateTextProvider(FontConfig* font_cfg, TextConfig* text_cfg, SDL_Renderer* renderer);
