/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <QPainter>
#include <QTimer>
#include <QCoreApplication>
#include <sstream>
#include <fstream>
#include <thread>
#include <cstdio>
#include "qrogue.h"
#include "qrogue_input.h"
#include "qrogue_display.h"
#include "args.h"
#include "environment.h"
#include "run_game.h"
#include "game_config.h"
#include "tile_provider.h"

const unsigned char QRogue::kSaveVersion = 2;

QRogue::QRogue(QQuickItem *parent)
    : QQuickPaintedItem(parent),
      config_(),
      thread_exited_(false)
{
    connect(this, SIGNAL(render()), this, SLOT(update()), Qt::QueuedConnection);
    connect(this, SIGNAL(soundEvent(const QString&)), this, SLOT(playSound(const QString&)), Qt::QueuedConnection);

    QStringList q_args = QCoreApplication::arguments();
    std::vector<std::string> v;
    for (int i = 0; i < q_args.size(); ++i)
        v.push_back(q_args[i].toStdString());
    Args args(v);

    env_.reset(new Environment(args));

    std::string graphics;
    env_->Get("gfx", &graphics);

    display_.reset(new QRogueDisplay(this, {80,25}, graphics));

    std::string value;
    bool sound(!env_->Get("sound", &value) || value != "false");
    display_->SetSound(sound);

    std::string game;
    if (env_->Get("game", &game) && !game.empty()){
        int i = GetGameIndex(game.c_str());
        if (i == -1  && game.size() == 1 && (game[0] >= 'a' && game[0] < 'a' + (int)s_options.size())){
            i = game[0] - 'a';
        }

        if (i != -1)
            setGame(i);
        else
            restoreGame(game.c_str());
    }
}

QRogue::~QRogue()
{
    autosave();
}

QSize QRogue::screenSize() const
{
    return display_->ScreenSize();
}

QString QRogue::game() const
{
    return config_.name.c_str();
}

void QRogue::setGame(const QString &game)
{
    int i = GetGameIndex(game.toStdString());
    setGame(i);
}

void QRogue::setGame(int index)
{
    config_ = GetGameConfig(index);
    emit gameChanged(config_.name.c_str());
    game_env_ = env_;

    int seed = (int)time(0);
    std::ostringstream ss;
    ss << seed;
    game_env_->Set("seed", ss.str());

    input_.reset(new QtRogueInput(this, env_.get(), game_env_.get(), config_));

    LaunchGame();
}

bool QRogue::showTitleScreen()
{
    return config_.name == "PC Rogue 1.48" && restore_count_ == 0;
}

void QRogue::restoreGame(const QString &filename)
{
    RestoreGame(filename.toStdString());
    LaunchGame();
}

void QRogue::RestoreGame(const std::string& path)
{
    std::ifstream file(path, std::ios::binary | std::ios::in);
    if (!file) {
        throw_error("Couldn't open save file: " + path);
    }

    unsigned char version;
    Read(file, &version);
    if (version > QRogue::kSaveVersion)
        throw_error("This file is not recognized.  It may have been saved with a newer version of Rogue Collection.  Please download the latest version and try again.");

    Read(file, &restore_count_);
    ++restore_count_;

    std::string name;
    ReadShortString(file, &name);
    int i = GetGameIndex(name);
    config_ = GetGameConfig(i);
    emit gameChanged(config_.name.c_str());

    // set up game environment
    game_env_.reset(new Environment());
    game_env_->Deserialize(file);
    game_env_->Set("in_replay", "true");
    std::string value;
    if (env_->Get("logfile", &value)) {
        game_env_->Set("logfile", value);
    }
    if (version == 1){
        game_env_->Set("trap_bugfix", "false");
        game_env_->Set("room_bugfix", "false");
        game_env_->Set("confused_bugfix", "false");
    }

    input_.reset(new QtRogueInput(this, env_.get(), game_env_.get(), config_));
    input_->RestoreGame(file);

    if (env_->Get("delete_on_restore", &value) && value == "true") {
        file.close();
        std::remove(path.c_str());
    }
}

void QRogue::LaunchGame()
{
    if (config_.name == "PC Rogue 1.1") {
        game_env_->Set("emulate_version", "1.1");
    }

    display_->SetGameConfig(config_, game_env_.get());

    if (!game_env_->WriteToOs(config_.is_unix))
        throw_error("Couldn't write environment");

    std::string screen;
    if (game_env_->Get("small_screen", &screen) && screen == "true")
    {
        display_->SetScreenSize(config_.small_screen);
        screenSizeChanged(config_.small_screen.x, config_.small_screen.y);
    }

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(onTimer()));
    timer->start(250);

    //start rogue engine on a background thread
    char* argv[] = {0};
    std::thread rogue(RunGame<QRogue>, config_.dll_name, 0, argv, this, std::ref(thread_exited_));
    rogue.detach(); //todo: how do we want threading to work?
}

void QRogue::saveGame(const QString &filename)
{
    SaveGame(filename.toStdString(), true);
}

void QRogue::nextGraphicsMode()
{
    display_->NextGfxMode();
}

void QRogue::autosave()
{
    std::string value;
    if (input_ && env_->Get("autosave", &value)){
        if (value == "true" && !thread_exited_ || value == "force"){
            std::string name = "autosave-" + GetTimeString() + ".sav";
            SaveGame(name, false);
        }
    }
}

QString QRogue::graphics() const
{
    return display_->Graphics();
}

void QRogue::setGraphics(const QString &gfx)
{
    display_->SetGraphics(gfx);
}

void QRogue::SaveGame(std::string path, bool notify)
{
    std::ofstream file(path, std::ios::binary | std::ios::out);
    if (!file) {
        DisplayMessage("Error", "Save Game", "Couldn't open save file: " + path);
        return;
    }

    Write(file, QRogue::kSaveVersion);
    Write(file, restore_count_);
    WriteShortString(file, config_.name);
    game_env_->Serialize(file);
    input_->SaveGame(file);

    if (!file) {
        DisplayMessage("Error", "Save Game", "Error writing to file: " + path);
        return;
    }

    if (notify) {
        std::string value;
        bool quit(env_->Get("exit_on_save", &value) && value == "true");

        std::string msg = "Your game was saved successfully.";
        if (quit){
            msg += "  Come back soon!";
        }
        DisplayMessage("Info", "Save Game", msg);

        if (quit)
            QuitApplication();
    }
}

QFont QRogue::font() const
{
    return display_->Font();
}

void QRogue::setFont(const QFont &font)
{
    display_->SetFont(font);
}

bool QRogue::sound() const
{
    return display_->Sound();
}

void QRogue::setSound(bool enable)
{
    display_->SetSound(enable);
}

bool QRogue::monochrome() const
{
    return display_->Monochrome();
}

void QRogue::setMonochrome(bool enable)
{
    display_->SetMonochrome(enable);
}

QSize QRogue::fontSize() const
{
    return display_->TileSize();
}

void QRogue::paint(QPainter *painter)
{
    display_->Render(painter);
    emit rendered();
}

void QRogue::onTimer()
{
    display_->Animate();
}

void QRogue::postRender()
{
    emit render();
}

void QRogue::tileSizeChanged()
{
    emit fontSizeChanged(fontSize().width(), fontSize().height());
}

void QRogue::playSound(const QString &id)
{
    display_->PlaySoundMainThread(id);
}

Environment *QRogue::GameEnv() const
{
    return game_env_.get();
}

QtRogueInput *QRogue::Input() const
{
    return input_.get();
}

QRogueDisplay *QRogue::Display() const
{
    return display_.get();
}

int QRogue::Lines() const
{
    return screenSize().height();
}

int QRogue::Columns() const
{
    return screenSize().width();
}

void QRogue::keyPressEvent(QKeyEvent *event)
{
    if (display_->HandleKeyEvent(event))
        return;
    else if (input_->HandleKeyEvent(event))
        return;

    QQuickPaintedItem::keyPressEvent(event);
}

InputInterface::~InputInterface(){}
DisplayInterface::~DisplayInterface(){}
