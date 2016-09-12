/*
 * Copyright (C) 2012-2016 Jacob R. Lifshay
 * This file is part of Voxels.
 *
 * Voxels is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Voxels is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Voxels; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */
#include "sdl2_driver.h"
#include "../../threading/threading.h"
#include <system_error>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <cstdlib>
#include <sstream>
#include "../../logging/logging.h"

namespace programmerjake
{
namespace voxels
{
namespace graphics
{
namespace drivers
{
struct SDL2Driver::RunningState final
{
    SDL2Driver &driver;
    std::mutex mutex;
    std::condition_variable cond;
    std::exception_ptr returnedException;
    bool done;
    const threading::Thread::Id mainThreadId = threading::thisThread::getId();
    SDL_Window *window;
    struct ScheduledCallback final
    {
        util::FunctionReference<void()> fn;
        bool done = false;
        std::exception_ptr exception;
        ScheduledCallback(util::FunctionReference<void()> fn) : fn(fn)
        {
        }
    };
    std::deque<ScheduledCallback *> scheduledCallbacks;
    util::FunctionReference<std::shared_ptr<CommandBuffer>()> renderCallback;
    util::FunctionReference<void(const ui::event::Event &event)> eventCallback;
    std::atomic_int width{1};
    std::atomic_int height{1};
    explicit RunningState(
        SDL2Driver &driver,
        util::FunctionReference<std::shared_ptr<CommandBuffer>()> renderCallback,
        util::FunctionReference<void(const ui::event::Event &event)> eventCallback)
        : driver(driver),
          mutex(),
          cond(),
          returnedException(),
          done(false),
          window(),
          scheduledCallbacks(),
          renderCallback(renderCallback),
          eventCallback(eventCallback)
    {
    }
    static ui::event::PhysicalKeyCode translatePhysicalKeyCode(SDL_Scancode code) noexcept
    {
        constexpr auto SDL_SCANCODE_LOCKINGCAPSLOCK = 130;
        constexpr auto SDL_SCANCODE_LOCKINGNUMLOCK = 131;
        constexpr auto SDL_SCANCODE_LOCKINGSCROLLLOCK = 132;
        switch(static_cast<std::uint32_t>(code))
        {
        case SDL_SCANCODE_A:
            return ui::event::PhysicalKeyCode::A;
        case SDL_SCANCODE_AGAIN:
            return ui::event::PhysicalKeyCode::Again;
        case SDL_SCANCODE_ALTERASE:
            return ui::event::PhysicalKeyCode::AltErase;
        case SDL_SCANCODE_APOSTROPHE:
            return ui::event::PhysicalKeyCode::Apostrophe;
        case SDL_SCANCODE_AC_BACK:
            return ui::event::PhysicalKeyCode::AppCtlBack;
        case SDL_SCANCODE_AC_BOOKMARKS:
            return ui::event::PhysicalKeyCode::AppCtlBookmarks;
        case SDL_SCANCODE_AC_FORWARD:
            return ui::event::PhysicalKeyCode::AppCtlForward;
        case SDL_SCANCODE_AC_HOME:
            return ui::event::PhysicalKeyCode::AppCtlHome;
        case SDL_SCANCODE_AC_REFRESH:
            return ui::event::PhysicalKeyCode::AppCtlRefresh;
        case SDL_SCANCODE_AC_SEARCH:
            return ui::event::PhysicalKeyCode::AppCtlSearch;
        case SDL_SCANCODE_AC_STOP:
            return ui::event::PhysicalKeyCode::AppCtlStop;
        case SDL_SCANCODE_APPLICATION:
            return ui::event::PhysicalKeyCode::Application;
        case SDL_SCANCODE_AUDIOMUTE:
            return ui::event::PhysicalKeyCode::AudioMute;
        case SDL_SCANCODE_AUDIONEXT:
            return ui::event::PhysicalKeyCode::AudioNext;
        case SDL_SCANCODE_AUDIOPLAY:
            return ui::event::PhysicalKeyCode::AudioPlay;
        case SDL_SCANCODE_AUDIOPREV:
            return ui::event::PhysicalKeyCode::AudioPrev;
        case SDL_SCANCODE_AUDIOSTOP:
            return ui::event::PhysicalKeyCode::AudioStop;
        case SDL_SCANCODE_B:
            return ui::event::PhysicalKeyCode::B;
        case SDL_SCANCODE_BACKSLASH:
            return ui::event::PhysicalKeyCode::Backslash;
        case SDL_SCANCODE_BRIGHTNESSDOWN:
            return ui::event::PhysicalKeyCode::BrightnessDown;
        case SDL_SCANCODE_BRIGHTNESSUP:
            return ui::event::PhysicalKeyCode::BrightnessUp;
        case SDL_SCANCODE_C:
            return ui::event::PhysicalKeyCode::C;
        case SDL_SCANCODE_CALCULATOR:
            return ui::event::PhysicalKeyCode::Calculator;
        case SDL_SCANCODE_CANCEL:
            return ui::event::PhysicalKeyCode::Cancel;
        case SDL_SCANCODE_CAPSLOCK:
            return ui::event::PhysicalKeyCode::CapsLock;
        case SDL_SCANCODE_CLEAR:
            return ui::event::PhysicalKeyCode::Clear;
        case SDL_SCANCODE_CLEARAGAIN:
            return ui::event::PhysicalKeyCode::ClearAgain;
        case SDL_SCANCODE_COMMA:
            return ui::event::PhysicalKeyCode::Comma;
        case SDL_SCANCODE_COMPUTER:
            return ui::event::PhysicalKeyCode::Computer;
        case SDL_SCANCODE_COPY:
            return ui::event::PhysicalKeyCode::Copy;
        case SDL_SCANCODE_CRSEL:
            return ui::event::PhysicalKeyCode::CrSel;
        case SDL_SCANCODE_CURRENCYSUBUNIT:
            return ui::event::PhysicalKeyCode::CurrencySubunit;
        case SDL_SCANCODE_CURRENCYUNIT:
            return ui::event::PhysicalKeyCode::CurrencyUnit;
        case SDL_SCANCODE_CUT:
            return ui::event::PhysicalKeyCode::Cut;
        case SDL_SCANCODE_D:
            return ui::event::PhysicalKeyCode::D;
        case SDL_SCANCODE_DECIMALSEPARATOR:
            return ui::event::PhysicalKeyCode::DecimalSeparator;
        case SDL_SCANCODE_DELETE:
            return ui::event::PhysicalKeyCode::Delete;
        case SDL_SCANCODE_DISPLAYSWITCH:
            return ui::event::PhysicalKeyCode::DisplaySwitch;
        case SDL_SCANCODE_DOWN:
            return ui::event::PhysicalKeyCode::Down;
        case SDL_SCANCODE_E:
            return ui::event::PhysicalKeyCode::E;
        case SDL_SCANCODE_EJECT:
            return ui::event::PhysicalKeyCode::Eject;
        case SDL_SCANCODE_END:
            return ui::event::PhysicalKeyCode::End;
        case SDL_SCANCODE_EQUALS:
            return ui::event::PhysicalKeyCode::Equals;
        case SDL_SCANCODE_ESCAPE:
            return ui::event::PhysicalKeyCode::Escape;
        case SDL_SCANCODE_EXSEL:
            return ui::event::PhysicalKeyCode::ExSel;
        case SDL_SCANCODE_F:
            return ui::event::PhysicalKeyCode::F;
        case SDL_SCANCODE_F1:
            return ui::event::PhysicalKeyCode::F1;
        case SDL_SCANCODE_F11:
            return ui::event::PhysicalKeyCode::F11;
        case SDL_SCANCODE_F12:
            return ui::event::PhysicalKeyCode::F12;
        case SDL_SCANCODE_F13:
            return ui::event::PhysicalKeyCode::F13;
        case SDL_SCANCODE_F14:
            return ui::event::PhysicalKeyCode::F14;
        case SDL_SCANCODE_F15:
            return ui::event::PhysicalKeyCode::F15;
        case SDL_SCANCODE_F16:
            return ui::event::PhysicalKeyCode::F16;
        case SDL_SCANCODE_F17:
            return ui::event::PhysicalKeyCode::F17;
        case SDL_SCANCODE_F18:
            return ui::event::PhysicalKeyCode::F18;
        case SDL_SCANCODE_F19:
            return ui::event::PhysicalKeyCode::F19;
        case SDL_SCANCODE_F2:
            return ui::event::PhysicalKeyCode::F2;
        case SDL_SCANCODE_F21:
            return ui::event::PhysicalKeyCode::F21;
        case SDL_SCANCODE_F22:
            return ui::event::PhysicalKeyCode::F22;
        case SDL_SCANCODE_F23:
            return ui::event::PhysicalKeyCode::F23;
        case SDL_SCANCODE_F24:
            return ui::event::PhysicalKeyCode::F24;
        case SDL_SCANCODE_F3:
            return ui::event::PhysicalKeyCode::F3;
        case SDL_SCANCODE_F4:
            return ui::event::PhysicalKeyCode::F4;
        case SDL_SCANCODE_F5:
            return ui::event::PhysicalKeyCode::F5;
        case SDL_SCANCODE_F6:
            return ui::event::PhysicalKeyCode::F6;
        case SDL_SCANCODE_F7:
            return ui::event::PhysicalKeyCode::F7;
        case SDL_SCANCODE_F8:
            return ui::event::PhysicalKeyCode::F8;
        case SDL_SCANCODE_F9:
            return ui::event::PhysicalKeyCode::F9;
        case SDL_SCANCODE_FIND:
            return ui::event::PhysicalKeyCode::Find;
        case SDL_SCANCODE_SLASH:
            return ui::event::PhysicalKeyCode::ForwardSlash;
        case SDL_SCANCODE_G:
            return ui::event::PhysicalKeyCode::G;
        case SDL_SCANCODE_GRAVE:
            return ui::event::PhysicalKeyCode::Grave;
        case SDL_SCANCODE_H:
            return ui::event::PhysicalKeyCode::H;
        case SDL_SCANCODE_HELP:
            return ui::event::PhysicalKeyCode::Help;
        case SDL_SCANCODE_HOME:
            return ui::event::PhysicalKeyCode::Home;
        case SDL_SCANCODE_I:
            return ui::event::PhysicalKeyCode::I;
        case SDL_SCANCODE_INSERT:
            return ui::event::PhysicalKeyCode::Insert;
        case SDL_SCANCODE_INTERNATIONAL1:
            return ui::event::PhysicalKeyCode::International1;
        case SDL_SCANCODE_INTERNATIONAL2:
            return ui::event::PhysicalKeyCode::International2;
        case SDL_SCANCODE_INTERNATIONAL3:
            return ui::event::PhysicalKeyCode::International3;
        case SDL_SCANCODE_INTERNATIONAL4:
            return ui::event::PhysicalKeyCode::International4;
        case SDL_SCANCODE_INTERNATIONAL5:
            return ui::event::PhysicalKeyCode::International5;
        case SDL_SCANCODE_INTERNATIONAL6:
            return ui::event::PhysicalKeyCode::International6;
        case SDL_SCANCODE_INTERNATIONAL7:
            return ui::event::PhysicalKeyCode::International7;
        case SDL_SCANCODE_INTERNATIONAL8:
            return ui::event::PhysicalKeyCode::International8;
        case SDL_SCANCODE_INTERNATIONAL9:
            return ui::event::PhysicalKeyCode::International9;
        case SDL_SCANCODE_J:
            return ui::event::PhysicalKeyCode::J;
        case SDL_SCANCODE_K:
            return ui::event::PhysicalKeyCode::K;
        case SDL_SCANCODE_KBDILLUMDOWN:
            return ui::event::PhysicalKeyCode::KeyboardIlluminationDown;
        case SDL_SCANCODE_KBDILLUMTOGGLE:
            return ui::event::PhysicalKeyCode::KeyboardIlluminationToggle;
        case SDL_SCANCODE_KBDILLUMUP:
            return ui::event::PhysicalKeyCode::KeyboardIlluminationUp;
        case SDL_SCANCODE_KP_0:
            return ui::event::PhysicalKeyCode::Keypad0;
        case SDL_SCANCODE_KP_00:
            return ui::event::PhysicalKeyCode::Keypad00;
        case SDL_SCANCODE_KP_000:
            return ui::event::PhysicalKeyCode::Keypad000;
        case SDL_SCANCODE_KP_1:
            return ui::event::PhysicalKeyCode::Keypad1;
        case SDL_SCANCODE_KP_2:
            return ui::event::PhysicalKeyCode::Keypad2;
        case SDL_SCANCODE_KP_3:
            return ui::event::PhysicalKeyCode::Keypad3;
        case SDL_SCANCODE_KP_4:
            return ui::event::PhysicalKeyCode::Keypad4;
        case SDL_SCANCODE_KP_5:
            return ui::event::PhysicalKeyCode::Keypad5;
        case SDL_SCANCODE_KP_6:
            return ui::event::PhysicalKeyCode::Keypad6;
        case SDL_SCANCODE_KP_7:
            return ui::event::PhysicalKeyCode::Keypad7;
        case SDL_SCANCODE_KP_8:
            return ui::event::PhysicalKeyCode::Keypad8;
        case SDL_SCANCODE_KP_9:
            return ui::event::PhysicalKeyCode::Keypad9;
        case SDL_SCANCODE_KP_A:
            return ui::event::PhysicalKeyCode::KeypadA;
        case SDL_SCANCODE_KP_AMPERSAND:
            return ui::event::PhysicalKeyCode::KeypadAmpersand;
        case SDL_SCANCODE_KP_AT:
            return ui::event::PhysicalKeyCode::KeypadAt;
        case SDL_SCANCODE_KP_B:
            return ui::event::PhysicalKeyCode::KeypadB;
        case SDL_SCANCODE_KP_BACKSPACE:
            return ui::event::PhysicalKeyCode::KeypadBackspace;
        case SDL_SCANCODE_KP_BINARY:
            return ui::event::PhysicalKeyCode::KeypadBinary;
        case SDL_SCANCODE_KP_C:
            return ui::event::PhysicalKeyCode::KeypadC;
        case SDL_SCANCODE_KP_CLEAR:
            return ui::event::PhysicalKeyCode::KeypadClear;
        case SDL_SCANCODE_KP_CLEARENTRY:
            return ui::event::PhysicalKeyCode::KeypadClearEntry;
        case SDL_SCANCODE_KP_COLON:
            return ui::event::PhysicalKeyCode::KeypadColon;
        case SDL_SCANCODE_KP_COMMA:
            return ui::event::PhysicalKeyCode::KeypadComma;
        case SDL_SCANCODE_KP_D:
            return ui::event::PhysicalKeyCode::KeypadD;
        case SDL_SCANCODE_KP_DECIMAL:
            return ui::event::PhysicalKeyCode::KeypadDecimal;
        case SDL_SCANCODE_KP_DBLAMPERSAND:
            return ui::event::PhysicalKeyCode::KeypadDoubleAmpersand;
        case SDL_SCANCODE_KP_DBLVERTICALBAR:
            return ui::event::PhysicalKeyCode::KeypadDoublePipe;
        case SDL_SCANCODE_KP_E:
            return ui::event::PhysicalKeyCode::KeypadE;
        case SDL_SCANCODE_KP_EXCLAM:
            return ui::event::PhysicalKeyCode::KeypadEMark;
        case SDL_SCANCODE_KP_EQUALS:
            return ui::event::PhysicalKeyCode::KeypadEquals;
        case SDL_SCANCODE_KP_F:
            return ui::event::PhysicalKeyCode::KeypadF;
        case SDL_SCANCODE_KP_GREATER:
            return ui::event::PhysicalKeyCode::KeypadGreater;
        case SDL_SCANCODE_KP_HASH:
            return ui::event::PhysicalKeyCode::KeypadHash;
        case SDL_SCANCODE_KP_HEXADECIMAL:
            return ui::event::PhysicalKeyCode::KeypadHexadecimal;
        case SDL_SCANCODE_KP_LEFTBRACE:
            return ui::event::PhysicalKeyCode::KeypadLBrace;
        case SDL_SCANCODE_KP_LESS:
            return ui::event::PhysicalKeyCode::KeypadLess;
        case SDL_SCANCODE_KP_LEFTPAREN:
            return ui::event::PhysicalKeyCode::KeypadLParen;
        case SDL_SCANCODE_KP_MEMADD:
            return ui::event::PhysicalKeyCode::KeypadMemAdd;
        case SDL_SCANCODE_KP_MEMCLEAR:
            return ui::event::PhysicalKeyCode::KeypadMemClear;
        case SDL_SCANCODE_KP_MEMDIVIDE:
            return ui::event::PhysicalKeyCode::KeypadMemDivide;
        case SDL_SCANCODE_KP_MEMMULTIPLY:
            return ui::event::PhysicalKeyCode::KeypadMemMultiply;
        case SDL_SCANCODE_KP_MEMRECALL:
            return ui::event::PhysicalKeyCode::KeypadMemRecall;
        case SDL_SCANCODE_KP_MEMSTORE:
            return ui::event::PhysicalKeyCode::KeypadMemStore;
        case SDL_SCANCODE_KP_MEMSUBTRACT:
            return ui::event::PhysicalKeyCode::KeypadMemSubtract;
        case SDL_SCANCODE_KP_MINUS:
            return ui::event::PhysicalKeyCode::KeypadMinus;
        case SDL_SCANCODE_KP_MULTIPLY:
            return ui::event::PhysicalKeyCode::KeypadMultiply;
        case SDL_SCANCODE_KP_OCTAL:
            return ui::event::PhysicalKeyCode::KeypadOctal;
        case SDL_SCANCODE_KP_PERCENT:
            return ui::event::PhysicalKeyCode::KeypadPercent;
        case SDL_SCANCODE_KP_PERIOD:
            return ui::event::PhysicalKeyCode::KeypadPeriod;
        case SDL_SCANCODE_KP_VERTICALBAR:
            return ui::event::PhysicalKeyCode::KeypadPipe;
        case SDL_SCANCODE_KP_PLUS:
            return ui::event::PhysicalKeyCode::KeypadPlus;
        case SDL_SCANCODE_KP_PLUSMINUS:
            return ui::event::PhysicalKeyCode::KeypadPlusMinus;
        case SDL_SCANCODE_KP_POWER:
            return ui::event::PhysicalKeyCode::KeypadPower;
        case SDL_SCANCODE_KP_RIGHTBRACE:
            return ui::event::PhysicalKeyCode::KeypadRBrace;
        case SDL_SCANCODE_KP_RIGHTPAREN:
            return ui::event::PhysicalKeyCode::KeypadRParen;
        case SDL_SCANCODE_KP_SPACE:
            return ui::event::PhysicalKeyCode::KeypadSpace;
        case SDL_SCANCODE_KP_TAB:
            return ui::event::PhysicalKeyCode::KeypadTab;
        case SDL_SCANCODE_KP_XOR:
            return ui::event::PhysicalKeyCode::KeypadXor;
        case SDL_SCANCODE_L:
            return ui::event::PhysicalKeyCode::L;
        case SDL_SCANCODE_LALT:
            return ui::event::PhysicalKeyCode::LAlt;
        case SDL_SCANCODE_LANG1:
            return ui::event::PhysicalKeyCode::Language1;
        case SDL_SCANCODE_LANG2:
            return ui::event::PhysicalKeyCode::Language2;
        case SDL_SCANCODE_LANG3:
            return ui::event::PhysicalKeyCode::Language3;
        case SDL_SCANCODE_LANG4:
            return ui::event::PhysicalKeyCode::Language4;
        case SDL_SCANCODE_LANG5:
            return ui::event::PhysicalKeyCode::Language5;
        case SDL_SCANCODE_LANG6:
            return ui::event::PhysicalKeyCode::Language6;
        case SDL_SCANCODE_LANG7:
            return ui::event::PhysicalKeyCode::Language7;
        case SDL_SCANCODE_LANG8:
            return ui::event::PhysicalKeyCode::Language8;
        case SDL_SCANCODE_LANG9:
            return ui::event::PhysicalKeyCode::Language9;
        case SDL_SCANCODE_LEFTBRACKET:
            return ui::event::PhysicalKeyCode::LBracket;
        case SDL_SCANCODE_LCTRL:
            return ui::event::PhysicalKeyCode::LCtrl;
        case SDL_SCANCODE_LEFT:
            return ui::event::PhysicalKeyCode::Left;
        case SDL_SCANCODE_LGUI:
            return ui::event::PhysicalKeyCode::LGUI;
        case SDL_SCANCODE_LOCKINGCAPSLOCK:
            return ui::event::PhysicalKeyCode::LockingCapsLock;
        case SDL_SCANCODE_LOCKINGNUMLOCK:
            return ui::event::PhysicalKeyCode::LockingNumLock;
        case SDL_SCANCODE_LOCKINGSCROLLLOCK:
            return ui::event::PhysicalKeyCode::LockingScrollLock;
        case SDL_SCANCODE_LSHIFT:
            return ui::event::PhysicalKeyCode::LShift;
        case SDL_SCANCODE_M:
            return ui::event::PhysicalKeyCode::M;
        case SDL_SCANCODE_MAIL:
            return ui::event::PhysicalKeyCode::Mail;
        case SDL_SCANCODE_MEDIASELECT:
            return ui::event::PhysicalKeyCode::MediaSelect;
        case SDL_SCANCODE_MENU:
            return ui::event::PhysicalKeyCode::Menu;
        case SDL_SCANCODE_MINUS:
            return ui::event::PhysicalKeyCode::Minus;
        case SDL_SCANCODE_MODE:
            return ui::event::PhysicalKeyCode::ModeSwitch;
        case SDL_SCANCODE_MUTE:
            return ui::event::PhysicalKeyCode::Mute;
        case SDL_SCANCODE_N:
            return ui::event::PhysicalKeyCode::N;
        case SDL_SCANCODE_NONUSBACKSLASH:
            return ui::event::PhysicalKeyCode::NonUSBackslash;
        case SDL_SCANCODE_NONUSHASH:
            return ui::event::PhysicalKeyCode::NonUSHash;
        case SDL_SCANCODE_0:
            return ui::event::PhysicalKeyCode::Number0;
        case SDL_SCANCODE_1:
            return ui::event::PhysicalKeyCode::Number1;
        case SDL_SCANCODE_2:
            return ui::event::PhysicalKeyCode::Number2;
        case SDL_SCANCODE_3:
            return ui::event::PhysicalKeyCode::Number3;
        case SDL_SCANCODE_4:
            return ui::event::PhysicalKeyCode::Number4;
        case SDL_SCANCODE_5:
            return ui::event::PhysicalKeyCode::Number5;
        case SDL_SCANCODE_6:
            return ui::event::PhysicalKeyCode::Number6;
        case SDL_SCANCODE_7:
            return ui::event::PhysicalKeyCode::Number7;
        case SDL_SCANCODE_8:
            return ui::event::PhysicalKeyCode::Number8;
        case SDL_SCANCODE_9:
            return ui::event::PhysicalKeyCode::Number9;
        case SDL_SCANCODE_NUMLOCKCLEAR:
            return ui::event::PhysicalKeyCode::NumlockClear;
        case SDL_SCANCODE_O:
            return ui::event::PhysicalKeyCode::O;
        case SDL_SCANCODE_OPER:
            return ui::event::PhysicalKeyCode::Oper;
        case SDL_SCANCODE_OUT:
            return ui::event::PhysicalKeyCode::Out;
        case SDL_SCANCODE_P:
            return ui::event::PhysicalKeyCode::P;
        case SDL_SCANCODE_PAGEDOWN:
            return ui::event::PhysicalKeyCode::PageDown;
        case SDL_SCANCODE_PAGEUP:
            return ui::event::PhysicalKeyCode::PageUp;
        case SDL_SCANCODE_PASTE:
            return ui::event::PhysicalKeyCode::Paste;
        case SDL_SCANCODE_PAUSE:
            return ui::event::PhysicalKeyCode::Pause;
        case SDL_SCANCODE_PERIOD:
            return ui::event::PhysicalKeyCode::Period;
        case SDL_SCANCODE_POWER:
            return ui::event::PhysicalKeyCode::Power;
        case SDL_SCANCODE_PRINTSCREEN:
            return ui::event::PhysicalKeyCode::PrintScreen;
        case SDL_SCANCODE_PRIOR:
            return ui::event::PhysicalKeyCode::Prior;
        case SDL_SCANCODE_Q:
            return ui::event::PhysicalKeyCode::Q;
        case SDL_SCANCODE_R:
            return ui::event::PhysicalKeyCode::R;
        case SDL_SCANCODE_RALT:
            return ui::event::PhysicalKeyCode::RAlt;
        case SDL_SCANCODE_RIGHTBRACKET:
            return ui::event::PhysicalKeyCode::RBracket;
        case SDL_SCANCODE_RCTRL:
            return ui::event::PhysicalKeyCode::RCtrl;
        case SDL_SCANCODE_RETURN:
            return ui::event::PhysicalKeyCode::Return;
        case SDL_SCANCODE_RETURN2:
            return ui::event::PhysicalKeyCode::Return2;
        case SDL_SCANCODE_RGUI:
            return ui::event::PhysicalKeyCode::RGUI;
        case SDL_SCANCODE_RIGHT:
            return ui::event::PhysicalKeyCode::Right;
        case SDL_SCANCODE_RSHIFT:
            return ui::event::PhysicalKeyCode::RShift;
        case SDL_SCANCODE_S:
            return ui::event::PhysicalKeyCode::S;
        case SDL_SCANCODE_SCROLLLOCK:
            return ui::event::PhysicalKeyCode::ScrollLock;
        case SDL_SCANCODE_SELECT:
            return ui::event::PhysicalKeyCode::Select;
        case SDL_SCANCODE_SEMICOLON:
            return ui::event::PhysicalKeyCode::Semicolon;
        case SDL_SCANCODE_SEPARATOR:
            return ui::event::PhysicalKeyCode::Separator;
        case SDL_SCANCODE_SLEEP:
            return ui::event::PhysicalKeyCode::Sleep;
        case SDL_SCANCODE_SPACE:
            return ui::event::PhysicalKeyCode::Space;
        case SDL_SCANCODE_STOP:
            return ui::event::PhysicalKeyCode::Stop;
        case SDL_SCANCODE_SYSREQ:
            return ui::event::PhysicalKeyCode::SysReq;
        case SDL_SCANCODE_T:
            return ui::event::PhysicalKeyCode::T;
        case SDL_SCANCODE_TAB:
            return ui::event::PhysicalKeyCode::Tab;
        case SDL_SCANCODE_THOUSANDSSEPARATOR:
            return ui::event::PhysicalKeyCode::ThousandsSeparator;
        case SDL_SCANCODE_U:
            return ui::event::PhysicalKeyCode::U;
        case SDL_SCANCODE_UNDO:
            return ui::event::PhysicalKeyCode::Undo;
        case SDL_SCANCODE_UP:
            return ui::event::PhysicalKeyCode::Up;
        case SDL_SCANCODE_V:
            return ui::event::PhysicalKeyCode::V;
        case SDL_SCANCODE_VOLUMEDOWN:
            return ui::event::PhysicalKeyCode::VolumeDown;
        case SDL_SCANCODE_VOLUMEUP:
            return ui::event::PhysicalKeyCode::VolumeUp;
        case SDL_SCANCODE_W:
            return ui::event::PhysicalKeyCode::W;
        case SDL_SCANCODE_WWW:
            return ui::event::PhysicalKeyCode::WWW;
        case SDL_SCANCODE_X:
            return ui::event::PhysicalKeyCode::X;
        case SDL_SCANCODE_Y:
            return ui::event::PhysicalKeyCode::Y;
        case SDL_SCANCODE_Z:
            return ui::event::PhysicalKeyCode::Z;

        case SDL_NUM_SCANCODES:
        case SDL_SCANCODE_UNKNOWN:
            break;
        }
        return ui::event::PhysicalKeyCode::Unknown;
    }
    static ui::event::VirtualKeyCode translateVirtualKeyCode(SDL_Keycode code) noexcept
    {
        switch(code)
        {
        case SDLK_a:
            return ui::event::VirtualKeyCode::A;
        case SDLK_AGAIN:
            return ui::event::VirtualKeyCode::Again;
        case SDLK_ALTERASE:
            return ui::event::VirtualKeyCode::AltErase;
        case SDLK_AMPERSAND:
            return ui::event::VirtualKeyCode::Ampersand;
        case SDLK_QUOTE:
            return ui::event::VirtualKeyCode::Apostrophe;
        case SDLK_AC_BACK:
            return ui::event::VirtualKeyCode::AppCtlBack;
        case SDLK_AC_BOOKMARKS:
            return ui::event::VirtualKeyCode::AppCtlBookmarks;
        case SDLK_AC_FORWARD:
            return ui::event::VirtualKeyCode::AppCtlForward;
        case SDLK_AC_HOME:
            return ui::event::VirtualKeyCode::AppCtlHome;
        case SDLK_AC_REFRESH:
            return ui::event::VirtualKeyCode::AppCtlRefresh;
        case SDLK_AC_SEARCH:
            return ui::event::VirtualKeyCode::AppCtlSearch;
        case SDLK_AC_STOP:
            return ui::event::VirtualKeyCode::AppCtlStop;
        case SDLK_APPLICATION:
            return ui::event::VirtualKeyCode::Application;
        case SDLK_ASTERISK:
            return ui::event::VirtualKeyCode::Asterisk;
        case SDLK_AT:
            return ui::event::VirtualKeyCode::At;
        case SDLK_AUDIOMUTE:
            return ui::event::VirtualKeyCode::AudioMute;
        case SDLK_AUDIONEXT:
            return ui::event::VirtualKeyCode::AudioNext;
        case SDLK_AUDIOPLAY:
            return ui::event::VirtualKeyCode::AudioPlay;
        case SDLK_AUDIOPREV:
            return ui::event::VirtualKeyCode::AudioPrev;
        case SDLK_AUDIOSTOP:
            return ui::event::VirtualKeyCode::AudioStop;
        case SDLK_b:
            return ui::event::VirtualKeyCode::B;
        case SDLK_BACKSLASH:
            return ui::event::VirtualKeyCode::Backslash;
        case SDLK_BRIGHTNESSDOWN:
            return ui::event::VirtualKeyCode::BrightnessDown;
        case SDLK_BRIGHTNESSUP:
            return ui::event::VirtualKeyCode::BrightnessUp;
        case SDLK_c:
            return ui::event::VirtualKeyCode::C;
        case SDLK_CALCULATOR:
            return ui::event::VirtualKeyCode::Calculator;
        case SDLK_CANCEL:
            return ui::event::VirtualKeyCode::Cancel;
        case SDLK_CAPSLOCK:
            return ui::event::VirtualKeyCode::CapsLock;
        case SDLK_CARET:
            return ui::event::VirtualKeyCode::Caret;
        case SDLK_CLEAR:
            return ui::event::VirtualKeyCode::Clear;
        case SDLK_CLEARAGAIN:
            return ui::event::VirtualKeyCode::ClearAgain;
        case SDLK_COLON:
            return ui::event::VirtualKeyCode::Colon;
        case SDLK_COMMA:
            return ui::event::VirtualKeyCode::Comma;
        case SDLK_COMPUTER:
            return ui::event::VirtualKeyCode::Computer;
        case SDLK_COPY:
            return ui::event::VirtualKeyCode::Copy;
        case SDLK_CRSEL:
            return ui::event::VirtualKeyCode::CrSel;
        case SDLK_CURRENCYSUBUNIT:
            return ui::event::VirtualKeyCode::CurrencySubunit;
        case SDLK_CURRENCYUNIT:
            return ui::event::VirtualKeyCode::CurrencyUnit;
        case SDLK_CUT:
            return ui::event::VirtualKeyCode::Cut;
        case SDLK_d:
            return ui::event::VirtualKeyCode::D;
        case SDLK_DECIMALSEPARATOR:
            return ui::event::VirtualKeyCode::DecimalSeparator;
        case SDLK_DELETE:
            return ui::event::VirtualKeyCode::Delete;
        case SDLK_DISPLAYSWITCH:
            return ui::event::VirtualKeyCode::DisplaySwitch;
        case SDLK_DOLLAR:
            return ui::event::VirtualKeyCode::Dollar;
        case SDLK_QUOTEDBL:
            return ui::event::VirtualKeyCode::DoubleQuote;
        case SDLK_DOWN:
            return ui::event::VirtualKeyCode::Down;
        case SDLK_e:
            return ui::event::VirtualKeyCode::E;
        case SDLK_EJECT:
            return ui::event::VirtualKeyCode::Eject;
        case SDLK_EXCLAIM:
            return ui::event::VirtualKeyCode::EMark;
        case SDLK_END:
            return ui::event::VirtualKeyCode::End;
        case SDLK_EQUALS:
            return ui::event::VirtualKeyCode::Equals;
        case SDLK_ESCAPE:
            return ui::event::VirtualKeyCode::Escape;
        case SDLK_EXSEL:
            return ui::event::VirtualKeyCode::ExSel;
        case SDLK_f:
            return ui::event::VirtualKeyCode::F;
        case SDLK_F1:
            return ui::event::VirtualKeyCode::F1;
        case SDLK_F11:
            return ui::event::VirtualKeyCode::F11;
        case SDLK_F12:
            return ui::event::VirtualKeyCode::F12;
        case SDLK_F13:
            return ui::event::VirtualKeyCode::F13;
        case SDLK_F14:
            return ui::event::VirtualKeyCode::F14;
        case SDLK_F15:
            return ui::event::VirtualKeyCode::F15;
        case SDLK_F16:
            return ui::event::VirtualKeyCode::F16;
        case SDLK_F17:
            return ui::event::VirtualKeyCode::F17;
        case SDLK_F18:
            return ui::event::VirtualKeyCode::F18;
        case SDLK_F19:
            return ui::event::VirtualKeyCode::F19;
        case SDLK_F2:
            return ui::event::VirtualKeyCode::F2;
        case SDLK_F21:
            return ui::event::VirtualKeyCode::F21;
        case SDLK_F22:
            return ui::event::VirtualKeyCode::F22;
        case SDLK_F23:
            return ui::event::VirtualKeyCode::F23;
        case SDLK_F24:
            return ui::event::VirtualKeyCode::F24;
        case SDLK_F3:
            return ui::event::VirtualKeyCode::F3;
        case SDLK_F4:
            return ui::event::VirtualKeyCode::F4;
        case SDLK_F5:
            return ui::event::VirtualKeyCode::F5;
        case SDLK_F6:
            return ui::event::VirtualKeyCode::F6;
        case SDLK_F7:
            return ui::event::VirtualKeyCode::F7;
        case SDLK_F8:
            return ui::event::VirtualKeyCode::F8;
        case SDLK_F9:
            return ui::event::VirtualKeyCode::F9;
        case SDLK_FIND:
            return ui::event::VirtualKeyCode::Find;
        case SDLK_SLASH:
            return ui::event::VirtualKeyCode::ForwardSlash;
        case SDLK_g:
            return ui::event::VirtualKeyCode::G;
        case SDLK_BACKQUOTE:
            return ui::event::VirtualKeyCode::Grave;
        case SDLK_GREATER:
            return ui::event::VirtualKeyCode::Greater;
        case SDLK_h:
            return ui::event::VirtualKeyCode::H;
        case SDLK_HASH:
            return ui::event::VirtualKeyCode::Hash;
        case SDLK_HELP:
            return ui::event::VirtualKeyCode::Help;
        case SDLK_HOME:
            return ui::event::VirtualKeyCode::Home;
        case SDLK_i:
            return ui::event::VirtualKeyCode::I;
        case SDLK_INSERT:
            return ui::event::VirtualKeyCode::Insert;
        case SDLK_j:
            return ui::event::VirtualKeyCode::J;
        case SDLK_k:
            return ui::event::VirtualKeyCode::K;
        case SDLK_KBDILLUMDOWN:
            return ui::event::VirtualKeyCode::KeyboardIlluminationDown;
        case SDLK_KBDILLUMTOGGLE:
            return ui::event::VirtualKeyCode::KeyboardIlluminationToggle;
        case SDLK_KBDILLUMUP:
            return ui::event::VirtualKeyCode::KeyboardIlluminationUp;
        case SDLK_KP_0:
            return ui::event::VirtualKeyCode::Keypad0;
        case SDLK_KP_00:
            return ui::event::VirtualKeyCode::Keypad00;
        case SDLK_KP_000:
            return ui::event::VirtualKeyCode::Keypad000;
        case SDLK_KP_1:
            return ui::event::VirtualKeyCode::Keypad1;
        case SDLK_KP_2:
            return ui::event::VirtualKeyCode::Keypad2;
        case SDLK_KP_3:
            return ui::event::VirtualKeyCode::Keypad3;
        case SDLK_KP_4:
            return ui::event::VirtualKeyCode::Keypad4;
        case SDLK_KP_5:
            return ui::event::VirtualKeyCode::Keypad5;
        case SDLK_KP_6:
            return ui::event::VirtualKeyCode::Keypad6;
        case SDLK_KP_7:
            return ui::event::VirtualKeyCode::Keypad7;
        case SDLK_KP_8:
            return ui::event::VirtualKeyCode::Keypad8;
        case SDLK_KP_9:
            return ui::event::VirtualKeyCode::Keypad9;
        case SDLK_KP_A:
            return ui::event::VirtualKeyCode::KeypadA;
        case SDLK_KP_AMPERSAND:
            return ui::event::VirtualKeyCode::KeypadAmpersand;
        case SDLK_KP_AT:
            return ui::event::VirtualKeyCode::KeypadAt;
        case SDLK_KP_B:
            return ui::event::VirtualKeyCode::KeypadB;
        case SDLK_KP_BACKSPACE:
            return ui::event::VirtualKeyCode::KeypadBackspace;
        case SDLK_KP_BINARY:
            return ui::event::VirtualKeyCode::KeypadBinary;
        case SDLK_KP_C:
            return ui::event::VirtualKeyCode::KeypadC;
        case SDLK_KP_CLEAR:
            return ui::event::VirtualKeyCode::KeypadClear;
        case SDLK_KP_CLEARENTRY:
            return ui::event::VirtualKeyCode::KeypadClearEntry;
        case SDLK_KP_COLON:
            return ui::event::VirtualKeyCode::KeypadColon;
        case SDLK_KP_COMMA:
            return ui::event::VirtualKeyCode::KeypadComma;
        case SDLK_KP_D:
            return ui::event::VirtualKeyCode::KeypadD;
        case SDLK_KP_DECIMAL:
            return ui::event::VirtualKeyCode::KeypadDecimal;
        case SDLK_KP_DBLAMPERSAND:
            return ui::event::VirtualKeyCode::KeypadDoubleAmpersand;
        case SDLK_KP_DBLVERTICALBAR:
            return ui::event::VirtualKeyCode::KeypadDoublePipe;
        case SDLK_KP_E:
            return ui::event::VirtualKeyCode::KeypadE;
        case SDLK_KP_EXCLAM:
            return ui::event::VirtualKeyCode::KeypadEMark;
        case SDLK_KP_EQUALS:
            return ui::event::VirtualKeyCode::KeypadEquals;
        case SDLK_KP_F:
            return ui::event::VirtualKeyCode::KeypadF;
        case SDLK_KP_GREATER:
            return ui::event::VirtualKeyCode::KeypadGreater;
        case SDLK_KP_HASH:
            return ui::event::VirtualKeyCode::KeypadHash;
        case SDLK_KP_HEXADECIMAL:
            return ui::event::VirtualKeyCode::KeypadHexadecimal;
        case SDLK_KP_LEFTBRACE:
            return ui::event::VirtualKeyCode::KeypadLBrace;
        case SDLK_KP_LESS:
            return ui::event::VirtualKeyCode::KeypadLess;
        case SDLK_KP_LEFTPAREN:
            return ui::event::VirtualKeyCode::KeypadLParen;
        case SDLK_KP_MEMADD:
            return ui::event::VirtualKeyCode::KeypadMemAdd;
        case SDLK_KP_MEMCLEAR:
            return ui::event::VirtualKeyCode::KeypadMemClear;
        case SDLK_KP_MEMDIVIDE:
            return ui::event::VirtualKeyCode::KeypadMemDivide;
        case SDLK_KP_MEMMULTIPLY:
            return ui::event::VirtualKeyCode::KeypadMemMultiply;
        case SDLK_KP_MEMRECALL:
            return ui::event::VirtualKeyCode::KeypadMemRecall;
        case SDLK_KP_MEMSTORE:
            return ui::event::VirtualKeyCode::KeypadMemStore;
        case SDLK_KP_MEMSUBTRACT:
            return ui::event::VirtualKeyCode::KeypadMemSubtract;
        case SDLK_KP_MINUS:
            return ui::event::VirtualKeyCode::KeypadMinus;
        case SDLK_KP_MULTIPLY:
            return ui::event::VirtualKeyCode::KeypadMultiply;
        case SDLK_KP_OCTAL:
            return ui::event::VirtualKeyCode::KeypadOctal;
        case SDLK_KP_PERCENT:
            return ui::event::VirtualKeyCode::KeypadPercent;
        case SDLK_KP_PERIOD:
            return ui::event::VirtualKeyCode::KeypadPeriod;
        case SDLK_KP_VERTICALBAR:
            return ui::event::VirtualKeyCode::KeypadPipe;
        case SDLK_KP_PLUS:
            return ui::event::VirtualKeyCode::KeypadPlus;
        case SDLK_KP_PLUSMINUS:
            return ui::event::VirtualKeyCode::KeypadPlusMinus;
        case SDLK_KP_POWER:
            return ui::event::VirtualKeyCode::KeypadPower;
        case SDLK_KP_RIGHTBRACE:
            return ui::event::VirtualKeyCode::KeypadRBrace;
        case SDLK_KP_RIGHTPAREN:
            return ui::event::VirtualKeyCode::KeypadRParen;
        case SDLK_KP_SPACE:
            return ui::event::VirtualKeyCode::KeypadSpace;
        case SDLK_KP_TAB:
            return ui::event::VirtualKeyCode::KeypadTab;
        case SDLK_KP_XOR:
            return ui::event::VirtualKeyCode::KeypadXor;
        case SDLK_l:
            return ui::event::VirtualKeyCode::L;
        case SDLK_LALT:
            return ui::event::VirtualKeyCode::LAlt;
        case SDLK_LEFTBRACKET:
            return ui::event::VirtualKeyCode::LBracket;
        case SDLK_LCTRL:
            return ui::event::VirtualKeyCode::LCtrl;
        case SDLK_LEFT:
            return ui::event::VirtualKeyCode::Left;
        case SDLK_LESS:
            return ui::event::VirtualKeyCode::Less;
        case SDLK_LGUI:
            return ui::event::VirtualKeyCode::LGUI;
        case SDLK_LEFTPAREN:
            return ui::event::VirtualKeyCode::LParen;
        case SDLK_LSHIFT:
            return ui::event::VirtualKeyCode::LShift;
        case SDLK_m:
            return ui::event::VirtualKeyCode::M;
        case SDLK_MAIL:
            return ui::event::VirtualKeyCode::Mail;
        case SDLK_MEDIASELECT:
            return ui::event::VirtualKeyCode::MediaSelect;
        case SDLK_MENU:
            return ui::event::VirtualKeyCode::Menu;
        case SDLK_MINUS:
            return ui::event::VirtualKeyCode::Minus;
        case SDLK_MODE:
            return ui::event::VirtualKeyCode::ModeSwitch;
        case SDLK_MUTE:
            return ui::event::VirtualKeyCode::Mute;
        case SDLK_n:
            return ui::event::VirtualKeyCode::N;
        case SDLK_0:
            return ui::event::VirtualKeyCode::Number0;
        case SDLK_1:
            return ui::event::VirtualKeyCode::Number1;
        case SDLK_2:
            return ui::event::VirtualKeyCode::Number2;
        case SDLK_3:
            return ui::event::VirtualKeyCode::Number3;
        case SDLK_4:
            return ui::event::VirtualKeyCode::Number4;
        case SDLK_5:
            return ui::event::VirtualKeyCode::Number5;
        case SDLK_6:
            return ui::event::VirtualKeyCode::Number6;
        case SDLK_7:
            return ui::event::VirtualKeyCode::Number7;
        case SDLK_8:
            return ui::event::VirtualKeyCode::Number8;
        case SDLK_9:
            return ui::event::VirtualKeyCode::Number9;
        case SDLK_NUMLOCKCLEAR:
            return ui::event::VirtualKeyCode::NumlockClear;
        case SDLK_o:
            return ui::event::VirtualKeyCode::O;
        case SDLK_OPER:
            return ui::event::VirtualKeyCode::Oper;
        case SDLK_OUT:
            return ui::event::VirtualKeyCode::Out;
        case SDLK_p:
            return ui::event::VirtualKeyCode::P;
        case SDLK_PAGEDOWN:
            return ui::event::VirtualKeyCode::PageDown;
        case SDLK_PAGEUP:
            return ui::event::VirtualKeyCode::PageUp;
        case SDLK_PASTE:
            return ui::event::VirtualKeyCode::Paste;
        case SDLK_PAUSE:
            return ui::event::VirtualKeyCode::Pause;
        case SDLK_PERCENT:
            return ui::event::VirtualKeyCode::Percent;
        case SDLK_PERIOD:
            return ui::event::VirtualKeyCode::Period;
        case SDLK_PLUS:
            return ui::event::VirtualKeyCode::Plus;
        case SDLK_POWER:
            return ui::event::VirtualKeyCode::Power;
        case SDLK_PRINTSCREEN:
            return ui::event::VirtualKeyCode::PrintScreen;
        case SDLK_PRIOR:
            return ui::event::VirtualKeyCode::Prior;
        case SDLK_q:
            return ui::event::VirtualKeyCode::Q;
        case SDLK_QUESTION:
            return ui::event::VirtualKeyCode::QMark;
        case SDLK_r:
            return ui::event::VirtualKeyCode::R;
        case SDLK_RALT:
            return ui::event::VirtualKeyCode::RAlt;
        case SDLK_RIGHTBRACKET:
            return ui::event::VirtualKeyCode::RBracket;
        case SDLK_RCTRL:
            return ui::event::VirtualKeyCode::RCtrl;
        case SDLK_RETURN:
            return ui::event::VirtualKeyCode::Return;
        case SDLK_RETURN2:
            return ui::event::VirtualKeyCode::Return2;
        case SDLK_RGUI:
            return ui::event::VirtualKeyCode::RGUI;
        case SDLK_RIGHT:
            return ui::event::VirtualKeyCode::Right;
        case SDLK_RIGHTPAREN:
            return ui::event::VirtualKeyCode::RParen;
        case SDLK_RSHIFT:
            return ui::event::VirtualKeyCode::RShift;
        case SDLK_s:
            return ui::event::VirtualKeyCode::S;
        case SDLK_SCROLLLOCK:
            return ui::event::VirtualKeyCode::ScrollLock;
        case SDLK_SELECT:
            return ui::event::VirtualKeyCode::Select;
        case SDLK_SEMICOLON:
            return ui::event::VirtualKeyCode::Semicolon;
        case SDLK_SEPARATOR:
            return ui::event::VirtualKeyCode::Separator;
        case SDLK_SLEEP:
            return ui::event::VirtualKeyCode::Sleep;
        case SDLK_SPACE:
            return ui::event::VirtualKeyCode::Space;
        case SDLK_STOP:
            return ui::event::VirtualKeyCode::Stop;
        case SDLK_SYSREQ:
            return ui::event::VirtualKeyCode::SysReq;
        case SDLK_t:
            return ui::event::VirtualKeyCode::T;
        case SDLK_TAB:
            return ui::event::VirtualKeyCode::Tab;
        case SDLK_THOUSANDSSEPARATOR:
            return ui::event::VirtualKeyCode::ThousandsSeparator;
        case SDLK_u:
            return ui::event::VirtualKeyCode::U;
        case SDLK_UNDERSCORE:
            return ui::event::VirtualKeyCode::Underline;
        case SDLK_UNDO:
            return ui::event::VirtualKeyCode::Undo;
        case SDLK_UP:
            return ui::event::VirtualKeyCode::Up;
        case SDLK_v:
            return ui::event::VirtualKeyCode::V;
        case SDLK_VOLUMEDOWN:
            return ui::event::VirtualKeyCode::VolumeDown;
        case SDLK_VOLUMEUP:
            return ui::event::VirtualKeyCode::VolumeUp;
        case SDLK_w:
            return ui::event::VirtualKeyCode::W;
        case SDLK_WWW:
            return ui::event::VirtualKeyCode::WWW;
        case SDLK_x:
            return ui::event::VirtualKeyCode::X;
        case SDLK_y:
            return ui::event::VirtualKeyCode::Y;
        case SDLK_z:
            return ui::event::VirtualKeyCode::Z;
        }
        return ui::event::VirtualKeyCode::Unknown;
    }
    static ui::event::KeyModifiers translateKeyModifiers(std::uint16_t modifiers) noexcept
    {
        ui::event::KeyModifiers retval = ui::event::KeyModifiers::None;
        if(modifiers & KMOD_LSHIFT)
            retval |= ui::event::KeyModifiers::LShift;
        if(modifiers & KMOD_RSHIFT)
            retval |= ui::event::KeyModifiers::RShift;
        if(modifiers & KMOD_LCTRL)
            retval |= ui::event::KeyModifiers::LCtrl;
        if(modifiers & KMOD_RCTRL)
            retval |= ui::event::KeyModifiers::RCtrl;
        if(modifiers & KMOD_LALT)
            retval |= ui::event::KeyModifiers::LAlt;
        if(modifiers & KMOD_RALT)
            retval |= ui::event::KeyModifiers::RAlt;
        if(modifiers & KMOD_LGUI)
            retval |= ui::event::KeyModifiers::LGUI;
        if(modifiers & KMOD_RGUI)
            retval |= ui::event::KeyModifiers::RGUI;
        if(modifiers & KMOD_NUM)
            retval |= ui::event::KeyModifiers::NumLock;
        if(modifiers & KMOD_CAPS)
            retval |= ui::event::KeyModifiers::CapsLock;
        if(modifiers & KMOD_MODE)
            retval |= ui::event::KeyModifiers::Mode;
        return retval;
    }
    static bool translateMouseButton(std::uint8_t buttonIn, ui::event::Mouse::Button &buttonOut)
    {
        switch(buttonIn)
        {
        case SDL_BUTTON_LEFT:
            buttonOut = ui::event::Mouse::Button::Left;
            return true;
        case SDL_BUTTON_MIDDLE:
            buttonOut = ui::event::Mouse::Button::Middle;
            return true;
        case SDL_BUTTON_RIGHT:
            buttonOut = ui::event::Mouse::Button::Right;
            return true;
        case SDL_BUTTON_X1:
            buttonOut = ui::event::Mouse::Button::X1;
            return true;
        case SDL_BUTTON_X2:
            buttonOut = ui::event::Mouse::Button::X2;
            return true;
        }
        return false;
    }
    void dispatchEvent(const SDL_Event &event)
    {
        constexpr auto SDL_AUDIODEVICEADDED = 0x1100;
        constexpr auto SDL_AUDIODEVICEREMOVED = SDL_AUDIODEVICEADDED + 1;
        constexpr auto SDL_KEYMAPCHANGED = SDL_TEXTINPUT + 1;
        constexpr auto SDL_RENDER_DEVICE_RESET = SDL_RENDER_TARGETS_RESET + 1;
        switch(event.type)
        {
        case SDL_APP_DIDENTERBACKGROUND:
            driver.setGraphicsContextRecreationNeeded();
            eventCallback(ui::event::AppDidEnterBackground());
            return;
        case SDL_APP_DIDENTERFOREGROUND:
            eventCallback(ui::event::AppDidEnterForegound());
            return;
        case SDL_APP_LOWMEMORY:
            eventCallback(ui::event::AppLowMemory());
            return;
        case SDL_APP_TERMINATING:
            eventCallback(ui::event::AppTerminating());
            return;
        case SDL_APP_WILLENTERBACKGROUND:
            eventCallback(ui::event::AppWillEnterBackground());
            return;
        case SDL_APP_WILLENTERFOREGROUND:
            eventCallback(ui::event::AppWillEnterForegound());
            return;
        case SDL_AUDIODEVICEADDED:
            // TODO: finish
            return;
        case SDL_AUDIODEVICEREMOVED:
            // TODO: finish
            return;
        case SDL_CLIPBOARDUPDATE:
            eventCallback(ui::event::ClipboardUpdate());
            return;
        case SDL_CONTROLLERAXISMOTION:
            // TODO: finish
            return;
        case SDL_CONTROLLERBUTTONDOWN:
            // TODO: finish
            return;
        case SDL_CONTROLLERBUTTONUP:
            // TODO: finish
            return;
        case SDL_CONTROLLERDEVICEADDED:
            // TODO: finish
            return;
        case SDL_CONTROLLERDEVICEREMAPPED:
            // TODO: finish
            return;
        case SDL_CONTROLLERDEVICEREMOVED:
            // TODO: finish
            return;
        case SDL_DOLLARGESTURE:
        case SDL_DOLLARRECORD:
            return;
        case SDL_DROPFILE:
            // TODO: finish
            return;
        case SDL_FINGERDOWN:
            eventCallback(ui::event::FingerDown(
                ui::event::Finger::Id(event.tfinger.touchId, event.tfinger.fingerId),
                event.tfinger.x,
                event.tfinger.y,
                event.tfinger.dx,
                event.tfinger.dy,
                event.tfinger.pressure));
            return;
        case SDL_FINGERMOTION:
            eventCallback(ui::event::FingerMove(
                ui::event::Finger::Id(event.tfinger.touchId, event.tfinger.fingerId),
                event.tfinger.x,
                event.tfinger.y,
                event.tfinger.dx,
                event.tfinger.dy,
                event.tfinger.pressure));
            return;
        case SDL_FINGERUP:
            eventCallback(ui::event::FingerUp(
                ui::event::Finger::Id(event.tfinger.touchId, event.tfinger.fingerId),
                event.tfinger.x,
                event.tfinger.y,
                event.tfinger.dx,
                event.tfinger.dy,
                event.tfinger.pressure));
            return;
        case SDL_JOYAXISMOTION:
            // TODO: finish
            return;
        case SDL_JOYBALLMOTION:
            // TODO: finish
            return;
        case SDL_JOYBUTTONDOWN:
            // TODO: finish
            return;
        case SDL_JOYBUTTONUP:
            // TODO: finish
            return;
        case SDL_JOYDEVICEADDED:
            // TODO: finish
            return;
        case SDL_JOYDEVICEREMOVED:
            // TODO: finish
            return;
        case SDL_JOYHATMOTION:
            // TODO: finish
            return;
        case SDL_KEYDOWN:
            eventCallback(ui::event::KeyDown(event.key.repeat,
                                             translatePhysicalKeyCode(event.key.keysym.scancode),
                                             translateVirtualKeyCode(event.key.keysym.sym),
                                             translateKeyModifiers(event.key.keysym.mod)));
            return;
        case SDL_KEYMAPCHANGED:
            // TODO: finish
            return;
        case SDL_KEYUP:
            eventCallback(ui::event::KeyUp(translatePhysicalKeyCode(event.key.keysym.scancode),
                                           translateVirtualKeyCode(event.key.keysym.sym),
                                           translateKeyModifiers(event.key.keysym.mod)));
            return;
        case SDL_MOUSEBUTTONDOWN:
            if(event.button.which != SDL_TOUCH_MOUSEID)
            {
                ui::event::Mouse::Button button{};
                if(translateMouseButton(event.button.button, button))
                    eventCallback(ui::event::MouseButtonDown(event.button.which,
                                                             event.button.x,
                                                             event.button.y,
                                                             button,
                                                             event.button.clicks));
            }
            return;
        case SDL_MOUSEBUTTONUP:
            if(event.button.which != SDL_TOUCH_MOUSEID)
            {
                ui::event::Mouse::Button button{};
                if(translateMouseButton(event.button.button, button))
                    eventCallback(ui::event::MouseButtonUp(event.button.which,
                                                           event.button.x,
                                                           event.button.y,
                                                           button,
                                                           event.button.clicks));
            }
            return;
        case SDL_MOUSEMOTION:
            if(event.button.which != SDL_TOUCH_MOUSEID)
            {
                util::EnumArray<bool, ui::event::Mouse::Button> buttonStates = {};
                for(ui::event::Mouse::Button button :
                    util::EnumTraits<ui::event::Mouse::Button>::values)
                {
                    buttonStates[button] = false;
                    decltype(event.motion.state) mask = 0;
                    switch(button) // switch inside for so we don't miss anything
                    {
                    case ui::event::Mouse::Button::Left:
                        mask = SDL_BUTTON_LMASK;
                        break;
                    case ui::event::Mouse::Button::Middle:
                        mask = SDL_BUTTON_MMASK;
                        break;
                    case ui::event::Mouse::Button::Right:
                        mask = SDL_BUTTON_RMASK;
                        break;
                    case ui::event::Mouse::Button::X1:
                        mask = SDL_BUTTON_X1MASK;
                        break;
                    case ui::event::Mouse::Button::X2:
                        mask = SDL_BUTTON_X2MASK;
                        break;
                    }
                    if(event.motion.state & mask)
                        buttonStates[button] = true;
                }
                eventCallback(ui::event::MouseMove(event.motion.which,
                                                   event.motion.x,
                                                   event.motion.y,
                                                   event.motion.xrel,
                                                   event.motion.yrel,
                                                   buttonStates));
            }
            return;
        case SDL_MOUSEWHEEL:
            if(event.wheel.which != SDL_TOUCH_MOUSEID)
            {
                auto x = event.wheel.x;
                auto y = event.wheel.y;
#if SDL_VERSION_ATLEAST(2, 0, 4)
                if(event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED)
                {
                    x = -x;
                    y = -y;
                }
#endif
                eventCallback(ui::event::MouseWheel(event.wheel.which, x, y));
            }
            return;
        case SDL_MULTIGESTURE:
            return;
        case SDL_QUIT:
            eventCallback(ui::event::Quit());
            return;
        case SDL_RENDER_DEVICE_RESET:
        case SDL_RENDER_TARGETS_RESET:
            driver.setGraphicsContextRecreationNeeded();
            return;
        case SDL_SYSWMEVENT:
            return;
        case SDL_TEXTEDITING:
            eventCallback(
                ui::event::TextEditing(event.edit.text, event.edit.start, event.edit.length));
            return;
        case SDL_TEXTINPUT:
            eventCallback(ui::event::TextInput(event.text.text));
            return;
        case SDL_WINDOWEVENT:
        {
            switch(event.window.event)
            {
            case SDL_WINDOWEVENT_SHOWN:
                eventCallback(ui::event::WindowShown());
                return;
            case SDL_WINDOWEVENT_HIDDEN:
                eventCallback(ui::event::WindowHidden());
                return;
            case SDL_WINDOWEVENT_EXPOSED:
                return;
            case SDL_WINDOWEVENT_MOVED:
                eventCallback(ui::event::WindowMoved(event.window.data1, event.window.data2));
                return;
            case SDL_WINDOWEVENT_RESIZED:
                eventCallback(ui::event::WindowResized(event.window.data1, event.window.data2));
                return;
            case SDL_WINDOWEVENT_SIZE_CHANGED:
                eventCallback(ui::event::WindowSizeChanged(event.window.data1, event.window.data2));
                return;
            case SDL_WINDOWEVENT_MINIMIZED:
                eventCallback(ui::event::WindowMinimized());
                return;
            case SDL_WINDOWEVENT_MAXIMIZED:
                eventCallback(ui::event::WindowMaximized());
                return;
            case SDL_WINDOWEVENT_RESTORED:
                eventCallback(ui::event::WindowRestored());
                return;
            case SDL_WINDOWEVENT_ENTER:
                eventCallback(ui::event::MouseEnter());
                return;
            case SDL_WINDOWEVENT_LEAVE:
                eventCallback(ui::event::MouseLeave());
                return;
            case SDL_WINDOWEVENT_FOCUS_GAINED:
                eventCallback(ui::event::KeyFocusGained());
                return;
            case SDL_WINDOWEVENT_FOCUS_LOST:
                eventCallback(ui::event::KeyFocusLost());
                return;
            case SDL_WINDOWEVENT_CLOSE:
                eventCallback(ui::event::WindowClose());
                return;
            case SDL_WINDOWEVENT_NONE:
                break;
            }
            std::ostringstream ss;
            ss << "unrecognized window event: " << event.window.event;
            logging::log(logging::Level::Warning, "SDL2Driver", ss.str());
            return;
        }
        case SDL_FIRSTEVENT:
        case SDL_LASTEVENT:
        case SDL_USEREVENT:
            break;
        }
        std::ostringstream ss;
        ss << "unrecognized event: " << event.type;
        logging::log(logging::Level::Warning, "SDL2Driver", ss.str());
    }
    void updateWindowSize() noexcept
    {
        int w, h;
        SDL_GL_GetDrawableSize(window, &w, &h);
        if(w <= 0)
            w = 1;
        if(h <= 0)
            h = 1;
        width.store(w, std::memory_order_relaxed);
        height.store(h, std::memory_order_release);
    }
    void gameLoop() noexcept
    {
        std::unique_lock<std::mutex> lockIt(mutex);
        try
        {
            while(!window && !done)
                cond.wait(lockIt);
            while(!done)
            {
                lockIt.unlock();
                auto commandBuffer = renderCallback();
                bool didRender = false;
                if(commandBuffer)
                {
                    didRender = true;
                    driver.runOnMainThread([&]()
                                           {
                                               driver.renderFrame(std::move(commandBuffer));
                                           });
                }
                SDL_Event event;
                while(true)
                {
                    bool gotEvent = false;
                    driver.runOnMainThread([&]()
                                           {
                                               if(didRender)
                                                   gotEvent = SDL_PollEvent(&event);
                                               else
                                                   gotEvent = SDL_WaitEventTimeout(&event, 250);
                                               updateWindowSize();
                                           });
                    if(!gotEvent)
                        break;
                    if(!isFilteredEvent(event))
                        dispatchEvent(event);
                }
                lockIt.lock();
            }
        }
        catch(...)
        {
            if(!lockIt.owns_lock())
                lockIt.lock();
            done = true;
            returnedException = std::current_exception();
            cond.notify_all();
        }
    }
    static bool isFilteredEvent(const SDL_Event &event) noexcept
    {
        switch(event.type)
        {
        case SDL_APP_DIDENTERBACKGROUND:
        case SDL_APP_DIDENTERFOREGROUND:
        case SDL_APP_LOWMEMORY:
        case SDL_APP_TERMINATING:
        case SDL_APP_WILLENTERBACKGROUND:
        case SDL_APP_WILLENTERFOREGROUND:
            return true;
        default:
            return false;
        }
    }
    void eventFilter(const SDL_Event &event) noexcept
    {
        if(isFilteredEvent(event))
            dispatchEvent(event);
    }
    void run()
    {
        threading::Thread gameLoopThread([this]()
                                         {
                                             gameLoop();
                                         });
        const SDL_EventFilter eventFilterFn = [](void *arg, SDL_Event *event) -> int
        {
            static_cast<RunningState *>(arg)->eventFilter(*event);
            return 0;
        };
        bool createdGraphicsContext = false;
        try
        {
            std::unique_lock<std::mutex> lockIt(mutex);
            SDL_AddEventWatch(eventFilterFn, static_cast<void *>(this));
            std::uint32_t flags = SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE;
#if 0
            flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
#endif
            window = driver.createWindow(
                SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1024, 768, flags);
            driver.createGraphicsContext();
            createdGraphicsContext = true;
            cond.notify_all();
            while(true)
            {
                if(returnedException)
                {
                    done = true;
                    cond.notify_all();
                    lockIt.unlock();
                    std::rethrow_exception(returnedException);
                    constexprAssert(false);
                    continue;
                }
                if(!scheduledCallbacks.empty())
                {
                    auto *callback = scheduledCallbacks.front();
                    scheduledCallbacks.pop_front();
                    lockIt.unlock();
                    try
                    {
                        callback->fn();
                    }
                    catch(...)
                    {
                        lockIt.lock();
                        callback->done = true;
                        callback->exception = std::current_exception();
                        cond.notify_all();
                        continue;
                    }
                    lockIt.lock();
                    callback->done = true;
                    cond.notify_all();
                    continue;
                }
                if(done)
                    break;
                cond.wait(lockIt);
            }
        }
        catch(...)
        {
            std::unique_lock<std::mutex> lockIt(mutex);
            done = true;
            cond.notify_all();
            lockIt.unlock();
            gameLoopThread.join();
            if(createdGraphicsContext)
                driver.destroyGraphicsContext();
            if(window)
                SDL_DestroyWindow(window);
            SDL_DelEventWatch(eventFilterFn, static_cast<void *>(this));
            throw;
        }
        std::unique_lock<std::mutex> lockIt(mutex);
        done = true;
        cond.notify_all();
        lockIt.unlock();
        gameLoopThread.join();
        if(createdGraphicsContext)
            driver.destroyGraphicsContext();
        if(window)
            SDL_DestroyWindow(window);
        SDL_DelEventWatch(eventFilterFn, static_cast<void *>(this));
    }
    static int initSDL() noexcept
    {
        if(0 != SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO))
        {
            std::string message = std::string("Initializing SDL failed: ") + SDL_GetError();
            logging::log(logging::Level::Fatal, "SDL2Driver", message);
            SDL_ShowSimpleMessageBox(
                SDL_MESSAGEBOX_ERROR, "SDL_Init failed", message.c_str(), nullptr);
            std::exit(1);
        }
        std::atexit(SDL_Quit);
        SDL_SetHint(SDL_HINT_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK, "1");
        return 0;
    }
};

SDL_Window *SDL2Driver::getWindow() const noexcept
{
    constexprAssert(runningState);
    return runningState->window;
}

void SDL2Driver::runOnMainThread(util::FunctionReference<void()> fn)
{
    constexprAssert(runningState != nullptr);
    if(threading::thisThread::getId() == runningState->mainThreadId)
    {
        fn();
    }
    else
    {
        RunningState::ScheduledCallback callback(fn);
        std::unique_lock<std::mutex> lockIt(runningState->mutex);
        runningState->scheduledCallbacks.push_back(&callback);
        runningState->cond.notify_all();
        while(!runningState->done)
        {
            if(callback.done)
                break;
            runningState->cond.wait(lockIt);
        }
        lockIt.unlock();
        if(callback.exception)
            std::rethrow_exception(callback.exception);
    }
}

void SDL2Driver::run(util::FunctionReference<std::shared_ptr<CommandBuffer>()> renderCallback,
                     util::FunctionReference<void(const ui::event::Event &event)> eventCallback)
{
    constexprAssert(runningState == nullptr);
    RunningState runningStateObject(*this, renderCallback, eventCallback);
    runningState = &runningStateObject;
    try
    {
        runningStateObject.run();
    }
    catch(...)
    {
        runningState = nullptr;
        throw;
    }
    runningState = nullptr;
}

void SDL2Driver::initSDL() noexcept
{
    static int unused = RunningState::initSDL();
    static_cast<void>(unused);
}

std::pair<std::size_t, std::size_t> SDL2Driver::getOutputSize() const noexcept
{
    if(!runningState)
        return {1, 1};
    int height = runningState->height.load(std::memory_order_acquire);
    int width = runningState->width.load(std::memory_order_relaxed);
    return {width, height};
}

void SDL2Driver::setRelativeMouseMode(bool enabled)
{
    constexprAssert(runningState);
    runOnMainThread([enabled]()
                    {
                        SDL_SetRelativeMouseMode(enabled ? SDL_TRUE : SDL_FALSE);
                    });
}
}
}
}
}
