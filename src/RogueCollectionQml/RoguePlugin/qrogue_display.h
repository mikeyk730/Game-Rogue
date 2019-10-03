﻿#ifndef QT_DISPLAY_H
#define QT_DISPLAY_H

#include <memory>
#include <mutex>
#include <vector>
#include <QSize>
#include <QFont>
#include <QPainter>
#include <QSoundEffect>
#include <coord.h>
#include <display_interface.h>
#include "game_config.h"
#include "colors.h"

class QRogue;
class QKeyEvent;
class ITileProvider;
class FontProvider;
class TileProvider;
class TextProvider;
struct Environment;

class QRogueDisplay : public DisplayInterface
{
public:
    QRogueDisplay(QRogue* parent, Coord screen_size, const std::string& graphics, bool rogomatic_server);

    QFont Font() const;
    void SetFont(const QFont& font);

    bool Monochrome() const;
    void SetMonochrome(bool enable);

    bool Sound() const;
    void SetSound(bool enable);

    QString Graphics() const;
    void SetGraphics(const QString &gfx);
    bool ApplyGraphics();
    void SetScreenSize(Coord screen_size);
    void SetGameConfig(const GameConfig &config, Environment* env);
    const GraphicsConfig& Gfx() const;
    bool HandleKeyEvent(QKeyEvent *event);
    void NextGfxMode();

    QSize TileSize() const;
    QSize ScreenSize() const;
    QSize ScreenPixelSize() const;
    QRect ScreenRect() const;

    void Render(QPainter *painter);
    void RenderRegion(QPainter *painter, uint32_t* data, Region rect);
    void RenderCursor(QPainter *painter, Coord cursor_pos);
    void RenderCounterOverlay(QPainter *painter, const std::string& label, int n);
    void Animate();
    void PostRenderEvent(bool rerender);
    void PlaySoundMainThread(const QString& id);

    virtual void SetDimensions(Coord dimensions) override;

    virtual void UpdateRegion(uint32_t* info, char* dirty) override;
    virtual void MoveCursor(Coord pos) override;
    virtual void SetCursor(bool enable) override;
    virtual void PlaySound(const std::string& id) override;

    Region FullRegion() const;
    int TotalChars() const;

private:
    void LoadAssets();
    void UpdateRegion(uint32_t* buf, Region rect);
    void PaintChar(QPainter *painter, int x, int y, int ch, int color, bool is_text);
    int TranslateChar(int ch, bool is_text) const;
    int DefaultColor() const;
    int TranslateColor(int color, bool is_text) const;
    int Index(int x, int y) const;

    void WriteRogomaticPosition(Coord pos);
    void WriteRogomaticScreen(uint32_t* data, char* dirty, int rows, int cols);

    QPainter& ScreenPainter();

    ITileProvider* TilePainter() const;
    ITileProvider* TextPainter() const;

    std::unique_ptr<FontProvider> font_provider_;
    std::unique_ptr<TileProvider> tile_provider_;
    std::unique_ptr<TextProvider> text_provider_;
    QSize screen_size_;

    QRogue* parent_;
    std::unique_ptr<GameConfig> config_;
    bool monochrome_ = false;
    bool sound_ = false;
    int gfx_index_ = 0;
    std::string gfx_mode_;
    int frame_ = 0;
    std::unique_ptr<QPixmap> screen_buffer_;
    std::map<std::string, QSoundEffect*> sounds_;

    std::ofstream rogomatic_stream_;

    struct ThreadData
    {
        ThreadData() = default;
        ThreadData(ThreadData& other);

        Coord dimensions = { 0, 0 };
        std::unique_ptr<uint32_t[]> data = 0;
        std::vector<Region> render_regions;

        bool show_cursor = false;
        Coord cursor_pos = { 0, 0 };
    };
    ThreadData shared_;
    std::mutex mutex_;
};

#endif
