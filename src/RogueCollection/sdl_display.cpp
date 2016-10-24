#include <sstream>
#include <SDL_image.h>
#include <pc_gfx_charmap.h>
#include "sdl_display.h"
#include "sdl_input.h"
#include "sdl_rogue.h"
#include "text_provider.h"
#include "tile_provider.h"
#include "window_sizer.h"
#include "environment.h"
#include "utility.h"

namespace
{
    std::map<int, int> unix_chars = {
        { PASSAGE,   '#' },
        { DOOR,      '+' },
        { FLOOR,     '.' },
        { PLAYER,    '@' },
        { TRAP,      '^' },
        { STAIRS,    '%' },
        { GOLD,      '*' },
        { POTION,    '!' },
        { SCROLL,    '?' },
        { FOOD,      ':' },
        { STICK,     '/' },
        { ARMOR,     ']' },
        { AMULET,    ',' },
        { RING,      '=' },
        { WEAPON,    ')' },
        { VWALL,     '|' },
        { HWALL,     '-' },
        { ULWALL,    '-' },
        { URWALL,    '-' },
        { LLWALL,    '-' },
        { LRWALL,    '-' },
        { 204,       '|' },
        { 185,       '|' },
    };

    uint32_t CharText(uint32_t ch)
    {
        return ch & 0x0000ffff;
    }

    uint32_t CharColor(uint32_t ch)
    {
        return (ch >> 24) & 0xff;
    }

    bool IsText(uint32_t ch)
    {
        return (ch & 0x010000) == 0;
    }

    uint32_t RENDER_EVENT = 0;
    uint32_t TIMER_EVENT = 0;

    Uint32 PostTimerMsg(Uint32 interval, void *type)
    {
        static bool parity = true;
        parity = !parity;

        SDL_Event e;
        SDL_zero(e);
        e.type = *static_cast<uint32_t*>(type);
        e.user.code = parity;
        SDL_PushEvent(&e);

        return interval;
    }
}

SdlDisplay::SdlDisplay(SDL_Window* window, SDL_Renderer* renderer, Environment* current_env, Environment* game_env, const GameConfig& options, SdlInput* input) :
    m_window(window),
    m_renderer(renderer),
    m_current_env(current_env),
    m_game_env(game_env),
    m_options(options),
    m_input(input),
    m_sizer(window, renderer, current_env)
{
    std::string title(SdlRogue::kWindowTitle);
    title += " - ";
    title += m_options.name;
    SetTitle(title);

    std::string gfx_pref;
    if (m_current_env->Get("gfx", &gfx_pref)) {
        for (int i = 0; i < (int)m_options.gfx_options.size(); ++i)
        {
            if (m_options.gfx_options[i].name == gfx_pref) {
                m_gfx_mode = i;
                break;
            }
        }
    }

    SDL_ShowWindow(window);
    LoadAssets();
}

void SdlDisplay::LoadAssets()
{
    if (current_gfx().font_cfg && !current_gfx().font_cfg->fontfile.empty())
        m_text_provider.reset(new TextGenerator(*(current_gfx().font_cfg), m_renderer));
    else if (current_gfx().text_cfg->generate_colors)
        m_text_provider.reset(new TextGenerator(*(current_gfx().text_cfg), m_renderer));
    else
        m_text_provider.reset(new TextProvider(*(current_gfx().text_cfg), m_renderer));
    m_block_size = m_text_provider->Dimensions();

    m_tile_provider.reset();
    if (current_gfx().tile_cfg)
    {
        m_tile_provider.reset(new TileProvider(*(current_gfx().tile_cfg), m_renderer));
        m_block_size = m_tile_provider->dimensions();
    }

    m_sizer.SetWindowSize(m_block_size.x * m_game_env->Columns(), m_block_size.y * m_game_env->Lines());
    SDL_RenderClear(m_renderer);
}

void SdlDisplay::RenderGame(bool force)
{
    std::vector<Region> regions;
    Coord dimensions;
    std::unique_ptr<uint32_t[]> data;
    Coord cursor_pos;
    bool show_cursor;

    //locked region
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        dimensions = m_shared_data.m_dimensions;
        if (dimensions.x == 0 || dimensions.y == 0)
            return;

        if (m_shared_data.m_render_regions.empty() && !force)
            return;

        uint32_t* temp = new uint32_t[dimensions.x*dimensions.y];
        memcpy(temp, m_shared_data.m_data, dimensions.x*dimensions.y * sizeof(uint32_t));
        data.reset(temp);

        regions = m_shared_data.m_render_regions;
        m_shared_data.m_render_regions.clear();

        show_cursor = m_shared_data.m_cursor;
        cursor_pos = m_shared_data.m_cursor_pos;
    }

    if (force) {
        SDL_RenderClear(m_renderer);
        regions.push_back({ 0,0,dimensions.x - 1,dimensions.y - 1 });
    }

    for (auto i = regions.begin(); i != regions.end(); ++i)
    {
        RenderRegion(data.get(), dimensions, *i);
    }

    if (show_cursor) {
        RenderCursor(cursor_pos);
    }

    std::string counter;
    if (m_input && m_input->GetRenderText(&counter))
        RenderCounterOverlay(counter, 0, dimensions);

    SDL_RenderPresent(m_renderer);
}

void SdlDisplay::Animate()
{
    bool update = false;
    if (current_gfx().animate) {

        Coord dimensions;
        std::unique_ptr<uint32_t[]> data;
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            dimensions = m_shared_data.m_dimensions;
            if (dimensions.x == 0 || dimensions.y == 0)
                return;

            uint32_t* temp = new uint32_t[dimensions.x*dimensions.y];
            memcpy(temp, m_shared_data.m_data, dimensions.x*dimensions.y * sizeof(uint32_t));
            data.reset(temp);
        }

        for (int i = 0; i < dimensions.x*dimensions.y; ++i) {
            auto c = CharText(data[i]);
            if (c != STAIRS)
                continue;

            int x = i % dimensions.x;
            int y = i / dimensions.x;
            SDL_Rect r = ScreenRegion({ x, y });
            RenderText(c, CharColor(data[i]), r, true);
            update = true;
        }
    }

    bool show_cursor;
    Coord cursor_pos;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        show_cursor = m_shared_data.m_cursor;
        cursor_pos = m_shared_data.m_cursor_pos;
    }

    if (show_cursor) {
        RenderCursor(cursor_pos);
        update = true;
    }

    if (update)
        SDL_RenderPresent(m_renderer);
}

void SdlDisplay::RenderRegion(uint32_t* data, Coord dimensions, Region rect)
{
    for (int y = rect.Top; y <= rect.Bottom; ++y) {
        for (int x = rect.Left; x <= rect.Right; ++x) {

            SDL_Rect r = ScreenRegion({ x, y });

            uint32_t info = data[y*dimensions.x + x];

            if (!m_tile_provider || IsText(info))
            {
                int color = CharColor(info);
                if (y == 0 && color == 0x70) {
                    // Hack for consistent standout in msg lines.  Unix versions use '-'.
                    // PC uses ' ' with background color.  We want consistent behavior.
                    if (current_gfx().use_standout) {
                        if (CharText(info) == '-')
                            info = ' ';
                    }
                    else {
                        if (CharText(info) == ' ')
                            info = '-';
                        color = 0x07;
                    }
                }
                RenderText(info, color, r, !IsText(info));
            }
            else {
                RenderTile(info, r);
            }
        }
    }
}

unsigned int GetColor(int chr, int attr)
{
    //if it is inside a room
    if (attr == 0x07 || attr == 0) switch (chr)
    {
    case DOOR:
    case VWALL: case HWALL:
    case ULWALL: case URWALL: case LLWALL: case LRWALL:
        return 0x06; //brown
    case FLOOR:
        return 0x0a; //light green
    case STAIRS:
        return 0x20; //black on light green
    case TRAP:
        return 0x05; //magenta
    case GOLD:
    case PLAYER:
        return 0x0e; //yellow
    case POTION:
    case SCROLL:
    case STICK:
    case ARMOR:
    case AMULET:
    case RING:
    case WEAPON:
        return 0x09; //light blue
    case FOOD:
        return 0x04; //red
    }
    //if inside a passage or a maze
    else if (attr == 0x70) switch (chr)
    {
    case FOOD:
        return 0x74; //red on grey
    case GOLD: case PLAYER:
        return 0x7e; //yellow on grey
    case POTION: case SCROLL: case STICK: case ARMOR: case AMULET: case RING: case WEAPON:
        return 0x71; //blue on grey
    }

    return attr;
}

unsigned char flip_color(unsigned char c)
{
    return ((c & 0x0f) << 4) | ((c & 0xf0) >> 4);
}

void SdlDisplay::RenderText(uint32_t info, unsigned char color, SDL_Rect r, bool is_tile)
{
    unsigned char c = CharText(info);

    // Tiles from Unix versions come in with either color=0x00 (for regular state)
    // or color=0x70 (for standout).  We need to translate these into more diverse
    // colors.  Tiles from PC versions already have the correct color, so we
    // technically don't need to do anything here, but it doesn't hurt to call
    // GetColor.
    if (is_tile) {
        color = GetColor(c, color);
    }
    if (!color || !current_gfx().use_colors) {
        bool standout(color > 0x0f);
        color = current_gfx().text_cfg->colors.front();
        if (standout && current_gfx().use_standout)
            color = flip_color(color);
    }

    if (current_gfx().animate && c == STAIRS && m_frame_number == 1)
        c = ' ';

    if (current_gfx().use_unix_gfx && is_tile)
    {
        auto i = unix_chars.find(c);
        if (i != unix_chars.end())
            c = i->second;
    }

    SDL_Rect clip;
    SDL_Texture* text;
    m_text_provider->GetTexture(c, color, &text, &clip);

    SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(m_renderer, &r);
    SDL_RenderCopy(m_renderer, text, &clip, &r);
}

void SdlDisplay::RenderTile(uint32_t info, SDL_Rect r)
{
    SDL_Texture* tiles;
    SDL_Rect clip;
    if (m_tile_provider->GetTexture(CharText(info), CharColor(info), &tiles, &clip)) {
        SDL_RenderCopy(m_renderer, tiles, &clip, &r);
    }
    else {
        //draw a black tile if we don't have a tile for this character
        SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
        SDL_RenderFillRect(m_renderer, &r);
    }
}

void SdlDisplay::RenderCursor(Coord pos)
{
    pos = ScreenPosition(pos);

    SDL_Rect r;
    r.x = pos.x;
    r.y = pos.y + (m_block_size.y * 3 / 4);
    r.w = m_block_size.x;
    r.h = m_block_size.y / 8;

    int color = m_frame_number ? 0x00 : 0x07;

    SDL_Rect clip;
    SDL_Texture* text;
    m_text_provider->GetTexture(0xdb, color, &text, &clip);

    SDL_RenderCopy(m_renderer, text, &clip, &r);
}

void SdlDisplay::RenderCounterOverlay(const std::string& label, int n, Coord dimensions)
{
    std::ostringstream ss;
    ss << label;
    if (n > 0)
        ss << n;
    std::string s(ss.str());
    int len = (int)s.size();
    for (int i = 0; i < len; ++i) {
        SDL_Rect r = ScreenRegion({ dimensions.x - (len - i) - 1, dimensions.y - 1 });
        RenderText(s[i], 0x70, r, false);
    }
}

const GraphicsConfig & SdlDisplay::current_gfx() const
{
    return m_options.gfx_options[m_gfx_mode];
}

Coord SdlDisplay::ScreenPosition(Coord buffer_pos)
{
    Coord p;
    p.x = buffer_pos.x * m_block_size.x;
    p.y = buffer_pos.y * m_block_size.y;
    return p;
}

SDL_Rect SdlDisplay::ScreenRegion(Coord buffer_pos)
{
    Coord p = ScreenPosition(buffer_pos);

    // We always render using the tile size.  Text will be scaled if it doesn't match
    SDL_Rect r;
    r.x = p.x;
    r.y = p.y;
    r.w = m_block_size.x;
    r.h = m_block_size.y;

    return r;
}

void SdlDisplay::SetDimensions(Coord dimensions)
{
    //todo: this function is not needed now that we have env.  we can do
    //this logic in the ctor, and take dimensions out of shared data
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_shared_data.m_dimensions = dimensions;
        m_shared_data.m_data = new uint32_t[dimensions.x*dimensions.y];
    }
    //set_window_size(m_block_size.x*dimensions.x, m_block_size.y*dimensions.y);
}

void SdlDisplay::UpdateRegion(uint32_t * info)
{
    UpdateRegion(info, SharedDataFullRegion());
}

void SdlDisplay::UpdateRegion(uint32_t* info, Region rect)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    //todo: If we're adding a full render to the queue, we can ignore any previous regions.

    //If we're behind on rendering, clear the queue and do a single full render.
    if (m_shared_data.m_render_regions.size() > MAX_QUEUE_SIZE)
    {
        m_shared_data.m_render_regions.clear();
        m_shared_data.m_render_regions.push_back(SharedDataFullRegion());
    }
    else {
        m_shared_data.m_render_regions.push_back(rect);
        PostRenderMsg(0);
    }

    memcpy(m_shared_data.m_data, info, m_shared_data.m_dimensions.x * m_shared_data.m_dimensions.y * sizeof(int32_t));
}

void SdlDisplay::MoveCursor(Coord pos)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_shared_data.m_cursor_pos = pos;
}

void SdlDisplay::SetCursor(bool enable)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_shared_data.m_cursor = enable;
}

void SdlDisplay::SetTitle(const std::string & title)
{
    SDL_SetWindowTitle(m_window, title.c_str());
}

void SdlDisplay::NextGfxMode()
{
    m_gfx_mode = (m_gfx_mode + 1) % m_options.gfx_options.size();
    LoadAssets();
    PostRenderMsg(1);
}

bool SdlDisplay::GetSavePath(std::string& path)
{
    return ::GetSavePath(m_window, path);
}

void SdlDisplay::RegisterEvents()
{
    RENDER_EVENT = SDL_RegisterEvents(2);
    TIMER_EVENT = RENDER_EVENT + 1;

    SDL_AddTimer(250, PostTimerMsg, &TIMER_EVENT);
}

void SdlDisplay::PostRenderMsg(int force)
{
    SDL_Event e;
    SDL_zero(e);
    e.type = RENDER_EVENT;
    e.user.code = force;
    SDL_PushEvent(&e);
}

bool SdlDisplay::HandleEvent(const SDL_Event & e)
{
    if (e.type == SDL_TEXTINPUT) {
        return HandleEventText(e);
    }
    else if (e.type == SDL_KEYDOWN) {
        return HandleEventKeyDown(e);
    }
    else if (e.type == SDL_WINDOWEVENT) {
        return HandleWindowEvent(e);
    }
    else if (e.type == RENDER_EVENT) {
        return HandleRenderEvent(e);
    }
    else if (e.type == TIMER_EVENT) {
        return HandleTimerEvent(e);
    }
    return false;
}

bool SdlDisplay::HandleWindowEvent(const SDL_Event & e)
{
    switch (e.window.event) {
    case SDL_WINDOWEVENT_SIZE_CHANGED:
    case SDL_WINDOWEVENT_EXPOSED:
        PostRenderMsg(1);
        return true;
    }
    return false;
}

bool SdlDisplay::HandleRenderEvent(const SDL_Event & e)
{
    SDL_FlushEvent(RENDER_EVENT);
    RenderGame(e.user.code != 0);
    return true;
}

bool SdlDisplay::HandleTimerEvent(const SDL_Event & e)
{
    m_frame_number = e.user.code;
    Animate();
    return true;
}

bool SdlDisplay::HandleEventKeyDown(const SDL_Event & e)
{
    if (m_sizer.ConsumeEvent(e))
        return true;
    return false;
}

bool SdlDisplay::HandleEventText(const SDL_Event & e)
{
    if (e.text.text[0] == '`')
    {
        NextGfxMode();
        return true;
    }
    return false;
}

Region SdlDisplay::SharedDataFullRegion()
{
    Region r;
    r.Left = 0;
    r.Top = 0;
    r.Right = short(m_shared_data.m_dimensions.x - 1);
    r.Bottom = short(m_shared_data.m_dimensions.y - 1);
    return r;
}

bool SdlDisplay::SharedDataIsNarrow()
{
    return m_shared_data.m_dimensions.x == 40;
}

