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

/*
  partially derived from:
  Simple DirectMedia Layer
  Copyright (C) 1997-2016 Sam Lantinga <slouken@libsdl.org>
  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.
  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:
  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#ifndef UI_EVENT_H_
#define UI_EVENT_H_

#include <memory>
#include <cstdint>
#include <type_traits>
#include <string>
#include "../util/enum.h"

namespace programmerjake
{
namespace voxels
{
namespace ui
{
namespace event
{
struct AppDidEnterBackground;
struct AppDidEnterForegound;
struct AppLowMemory;
struct AppTerminating;
struct AppWillEnterBackground;
struct AppWillEnterForegound;
struct ClipboardUpdate;
struct FingerDown;
struct FingerMove;
struct FingerUp;
struct KeyDown;
struct KeyFocusGained;
struct KeyFocusLost;
struct KeyUp;
struct MouseButtonDown;
struct MouseButtonUp;
struct MouseEnter;
struct MouseLeave;
struct MouseMove;
struct MouseWheel;
struct Quit;
struct TextEditing;
struct TextInput;
struct WindowClose;
struct WindowHidden;
struct WindowMaximized;
struct WindowMinimized;
struct WindowMoved;
struct WindowResized;
struct WindowRestored;
struct WindowShown;
struct WindowSizeChanged;

struct EventHandler : public std::enable_shared_from_this<EventHandler>
{
    virtual ~EventHandler() = default;
    virtual void onAppDidEnterBackground(const AppDidEnterBackground &event) = 0;
    virtual void onAppDidEnterForegound(const AppDidEnterForegound &event) = 0;
    virtual void onAppLowMemory(const AppLowMemory &event) = 0;
    virtual void onAppTerminating(const AppTerminating &event) = 0;
    virtual void onAppWillEnterBackground(const AppWillEnterBackground &event) = 0;
    virtual void onAppWillEnterForegound(const AppWillEnterForegound &event) = 0;
    virtual void onClipboardUpdate(const ClipboardUpdate &event) = 0;
    virtual void onFingerDown(const FingerDown &event) = 0;
    virtual void onFingerMove(const FingerMove &event) = 0;
    virtual void onFingerUp(const FingerUp &event) = 0;
    virtual void onKeyDown(const KeyDown &event) = 0;
    virtual void onKeyFocusGained(const KeyFocusGained &event) = 0;
    virtual void onKeyFocusLost(const KeyFocusLost &event) = 0;
    virtual void onKeyUp(const KeyUp &event) = 0;
    virtual void onMouseButtonDown(const MouseButtonDown &event) = 0;
    virtual void onMouseButtonUp(const MouseButtonUp &event) = 0;
    virtual void onMouseEnter(const MouseEnter &event) = 0;
    virtual void onMouseLeave(const MouseLeave &event) = 0;
    virtual void onMouseMove(const MouseMove &event) = 0;
    virtual void onMouseWheel(const MouseWheel &event) = 0;
    virtual void onQuit(const Quit &event) = 0;
    virtual void onTextEditing(const TextEditing &event) = 0;
    virtual void onTextInput(const TextInput &event) = 0;
    virtual void onWindowClose(const WindowClose &event) = 0;
    virtual void onWindowHidden(const WindowHidden &event) = 0;
    virtual void onWindowMaximized(const WindowMaximized &event) = 0;
    virtual void onWindowMinimized(const WindowMinimized &event) = 0;
    virtual void onWindowMoved(const WindowMoved &event) = 0;
    virtual void onWindowResized(const WindowResized &event) = 0;
    virtual void onWindowRestored(const WindowRestored &event) = 0;
    virtual void onWindowShown(const WindowShown &event) = 0;
    virtual void onWindowSizeChanged(const WindowSizeChanged &event) = 0;
};

struct Event
{
    virtual ~Event() = default;
    virtual void dispatch(EventHandler &handler) const = 0;
    virtual const char *getEventName() const noexcept = 0;
};

struct AppDidEnterBackground final : public Event
{
    virtual const char *getEventName() const noexcept override
    {
        return "AppDidEnterBackground";
    }
    virtual void dispatch(EventHandler &handler) const override
    {
        handler.onAppDidEnterBackground(*this);
    }
};

struct AppDidEnterForegound final : public Event
{
    virtual const char *getEventName() const noexcept override
    {
        return "AppDidEnterForegound";
    }
    virtual void dispatch(EventHandler &handler) const override
    {
        handler.onAppDidEnterForegound(*this);
    }
};

struct AppLowMemory final : public Event
{
    virtual const char *getEventName() const noexcept override
    {
        return "AppLowMemory";
    }
    virtual void dispatch(EventHandler &handler) const override
    {
        handler.onAppLowMemory(*this);
    }
};

struct AppTerminating final : public Event
{
    virtual const char *getEventName() const noexcept override
    {
        return "AppTerminating";
    }
    virtual void dispatch(EventHandler &handler) const override
    {
        handler.onAppTerminating(*this);
    }
};

struct AppWillEnterBackground final : public Event
{
    virtual const char *getEventName() const noexcept override
    {
        return "AppWillEnterBackground";
    }
    virtual void dispatch(EventHandler &handler) const override
    {
        handler.onAppWillEnterBackground(*this);
    }
};

struct AppWillEnterForegound final : public Event
{
    virtual const char *getEventName() const noexcept override
    {
        return "AppWillEnterForegound";
    }
    virtual void dispatch(EventHandler &handler) const override
    {
        handler.onAppWillEnterForegound(*this);
    }
};

struct ClipboardUpdate final : public Event
{
    virtual const char *getEventName() const noexcept override
    {
        return "ClipboardUpdate";
    }
    virtual void dispatch(EventHandler &handler) const override
    {
        handler.onClipboardUpdate(*this);
    }
};

struct Finger : public Event
{
    typedef std::uint64_t DeviceId;
    typedef std::uint64_t FingerId;
    struct Id final
    {
        DeviceId device;
        FingerId finger;
        constexpr Id() : device(), finger()
        {
        }
        constexpr Id(DeviceId device, FingerId finger) : device(device), finger(finger)
        {
        }
    };
    Id id;
    float x;
    float y;
    float dx;
    float dy;
    float pressure;
    Finger(Id id, float x, float y, float dx, float dy, float pressure)
        : id(id), x(x), y(y), dx(dx), dy(dy), pressure(pressure)
    {
    }
};

struct FingerDown final : public Finger
{
    virtual const char *getEventName() const noexcept override
    {
        return "FingerDown";
    }
    virtual void dispatch(EventHandler &handler) const override
    {
        handler.onFingerDown(*this);
    }
    FingerDown(Id id, float x, float y, float dx, float dy, float pressure)
        : Finger(id, x, y, dx, dy, pressure)
    {
    }
};

struct FingerMove final : public Finger
{
    virtual const char *getEventName() const noexcept override
    {
        return "FingerMove";
    }
    virtual void dispatch(EventHandler &handler) const override
    {
        handler.onFingerMove(*this);
    }
    FingerMove(Id id, float x, float y, float dx, float dy, float pressure)
        : Finger(id, x, y, dx, dy, pressure)
    {
    }
};

struct FingerUp final : public Finger
{
    virtual const char *getEventName() const noexcept override
    {
        return "FingerUp";
    }
    virtual void dispatch(EventHandler &handler) const override
    {
        handler.onFingerUp(*this);
    }
    FingerUp(Id id, float x, float y, float dx, float dy, float pressure)
        : Finger(id, x, y, dx, dy, pressure)
    {
    }
};

enum class PhysicalKeyCode
{
    A,
    Again, // Redo
    AltErase,
    Apostrophe,
    AppCtlBack,
    AppCtlBookmarks,
    AppCtlForward,
    AppCtlHome,
    AppCtlRefresh,
    AppCtlSearch,
    AppCtlStop,
    Application, // Context Menu
    AudioMute,
    AudioNext,
    AudioPlay,
    AudioPrev,
    AudioStop,
    B,
    Backslash, // denotes the location of the backslash key in the US layout
    BrightnessDown,
    BrightnessUp,
    C,
    Calculator,
    Cancel,
    CapsLock,
    Clear,
    ClearAgain, // Clear/Again
    Comma,
    Computer, // My Computer key
    Copy,
    CrSel,
    CurrencySubunit,
    CurrencyUnit,
    Cut,
    D,
    DecimalSeparator,
    Delete,
    DisplaySwitch,
    Down,
    E,
    Eject,
    End,
    Equals,
    Escape,
    ExSel,
    F,
    F1,
    F11,
    F12,
    F13,
    F14,
    F15,
    F16,
    F17,
    F18,
    F19,
    F2,
    F21,
    F22,
    F23,
    F24,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    Find,
    ForwardSlash, // the '/' key
    G,
    Grave, // "`/~" key on US Windows layout on ANSI keyboard
    H,
    Help,
    Home,
    I,
    Insert, // insert on pc, help on some Mac keyboards
    International1,
    International2,
    International3,
    International4,
    International5,
    International6,
    International7,
    International8,
    International9,
    J,
    K,
    KeyboardIlluminationDown,
    KeyboardIlluminationToggle,
    KeyboardIlluminationUp,
    Keypad0,
    Keypad00,
    Keypad000,
    Keypad1,
    Keypad2,
    Keypad3,
    Keypad4,
    Keypad5,
    Keypad6,
    Keypad7,
    Keypad8,
    Keypad9,
    KeypadA,
    KeypadAmpersand, // &
    KeypadAt, // @
    KeypadB,
    KeypadBackspace,
    KeypadBinary,
    KeypadC,
    KeypadClear,
    KeypadClearEntry,
    KeypadColon,
    KeypadComma,
    KeypadD,
    KeypadDecimal,
    KeypadDoubleAmpersand,
    KeypadDoublePipe,
    KeypadE,
    KeypadEMark, // the ! key
    KeypadEquals,
    KeypadF,
    KeypadGreater,
    KeypadHash,
    KeypadHexadecimal,
    KeypadLBrace,
    KeypadLess,
    KeypadLParen,
    KeypadMemAdd,
    KeypadMemClear,
    KeypadMemDivide,
    KeypadMemMultiply,
    KeypadMemRecall,
    KeypadMemStore,
    KeypadMemSubtract,
    KeypadMinus,
    KeypadMultiply,
    KeypadOctal,
    KeypadPercent,
    KeypadPeriod,
    KeypadPipe,
    KeypadPlus,
    KeypadPlusMinus,
    KeypadPower, // the ^ key
    KeypadRBrace,
    KeypadRParen,
    KeypadSpace,
    KeypadTab,
    KeypadXor,
    L,
    LAlt,
    Language1,
    Language2,
    Language3,
    Language4,
    Language5,
    Language6,
    Language7,
    Language8,
    Language9,
    LBracket,
    LCtrl,
    Left,
    LGUI, // left GUI : windows key, command key, meta key
    LockingCapsLock,
    LockingNumLock,
    LockingScrollLock,
    LShift,
    M,
    Mail, // mail/email key
    MediaSelect,
    Menu,
    Minus,
    ModeSwitch,
    Mute,
    N,
    NonUSBackslash,
    NonUSHash,
    Number0,
    Number1,
    Number2,
    Number3,
    Number4,
    Number5,
    Number6,
    Number7,
    Number8,
    Number9,
    NumlockClear, // Numlock (PC) / Clear (Mac)
    O,
    Oper,
    Out,
    P,
    PageDown,
    PageUp,
    Paste,
    Pause, // Pause/Break key
    Period,
    Power,
    PrintScreen,
    Prior,
    Q,
    R,
    RAlt,
    RBracket,
    RCtrl,
    Return, // the Enter key
    Return2,
    RGUI, // right GUI : windows key, command key, meta key
    Right,
    RShift,
    S,
    ScrollLock,
    Select,
    Semicolon,
    Separator,
    Sleep,
    Space,
    Stop,
    SysReq,
    T,
    Tab,
    ThousandsSeparator,
    U,
    Undo,
    Up,
    V,
    VolumeDown,
    VolumeUp,
    W,
    WWW, // the WWW/World Wide Web key
    X,
    Y,
    Z,

    Unknown, // must be the last value
    DEFINE_ENUM_MAX(Unknown)
};

enum class VirtualKeyCode
{
    A,
    Again, // Redo
    AltErase,
    Ampersand,
    Apostrophe,
    AppCtlBack,
    AppCtlBookmarks,
    AppCtlForward,
    AppCtlHome,
    AppCtlRefresh,
    AppCtlSearch,
    AppCtlStop,
    Application, // Context Menu
    Asterisk,
    At,
    AudioMute,
    AudioNext,
    AudioPlay,
    AudioPrev,
    AudioStop,
    B,
    Backslash, // denotes the location of the backslash key in the US layout
    BrightnessDown,
    BrightnessUp,
    C,
    Calculator,
    Cancel,
    CapsLock,
    Caret,
    Clear,
    ClearAgain, // Clear/Again
    Colon,
    Comma,
    Computer, // My Computer key
    Copy,
    CrSel,
    CurrencySubunit,
    CurrencyUnit,
    Cut,
    D,
    DecimalSeparator,
    Delete,
    DisplaySwitch,
    Dollar,
    DoubleQuote,
    Down,
    E,
    Eject,
    EMark,
    End,
    Equals,
    Escape,
    ExSel,
    F,
    F1,
    F11,
    F12,
    F13,
    F14,
    F15,
    F16,
    F17,
    F18,
    F19,
    F2,
    F21,
    F22,
    F23,
    F24,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    Find,
    ForwardSlash, // the '/' key
    G,
    Grave, // "`/~" key on US Windows layout on ANSI keyboard
    Greater,
    H,
    Hash,
    Help,
    Home,
    I,
    Insert, // insert on pc, help on some Mac keyboards
    J,
    K,
    KeyboardIlluminationDown,
    KeyboardIlluminationToggle,
    KeyboardIlluminationUp,
    Keypad0,
    Keypad00,
    Keypad000,
    Keypad1,
    Keypad2,
    Keypad3,
    Keypad4,
    Keypad5,
    Keypad6,
    Keypad7,
    Keypad8,
    Keypad9,
    KeypadA,
    KeypadAmpersand, // &
    KeypadAt, // @
    KeypadB,
    KeypadBackspace,
    KeypadBinary,
    KeypadC,
    KeypadClear,
    KeypadClearEntry,
    KeypadColon,
    KeypadComma,
    KeypadD,
    KeypadDecimal,
    KeypadDoubleAmpersand,
    KeypadDoublePipe,
    KeypadE,
    KeypadEMark, // the ! key
    KeypadEquals,
    KeypadF,
    KeypadGreater,
    KeypadHash,
    KeypadHexadecimal,
    KeypadLBrace,
    KeypadLess,
    KeypadLParen,
    KeypadMemAdd,
    KeypadMemClear,
    KeypadMemDivide,
    KeypadMemMultiply,
    KeypadMemRecall,
    KeypadMemStore,
    KeypadMemSubtract,
    KeypadMinus,
    KeypadMultiply,
    KeypadOctal,
    KeypadPercent,
    KeypadPeriod,
    KeypadPipe,
    KeypadPlus,
    KeypadPlusMinus,
    KeypadPower, // the ^ key
    KeypadRBrace,
    KeypadRParen,
    KeypadSpace,
    KeypadTab,
    KeypadXor,
    L,
    LAlt,
    LBracket,
    LCtrl,
    Left,
    Less,
    LGUI, // left GUI : windows key, command key, meta key
    LParen,
    LShift,
    M,
    Mail, // mail/email key
    MediaSelect,
    Menu,
    Minus,
    ModeSwitch,
    Mute,
    N,
    Number0,
    Number1,
    Number2,
    Number3,
    Number4,
    Number5,
    Number6,
    Number7,
    Number8,
    Number9,
    NumlockClear, // Numlock (PC) / Clear (Mac)
    O,
    Oper,
    Out,
    P,
    PageDown,
    PageUp,
    Paste,
    Pause, // Pause/Break key
    Percent,
    Period,
    Plus,
    Power,
    PrintScreen,
    Prior,
    Q,
    QMark,
    R,
    RAlt,
    RBracket,
    RCtrl,
    Return, // the Enter key
    Return2,
    RGUI, // right GUI : windows key, command key, meta key
    Right,
    RParen,
    RShift,
    S,
    ScrollLock,
    Select,
    Semicolon,
    Separator,
    Sleep,
    Space,
    Stop,
    SysReq,
    T,
    Tab,
    ThousandsSeparator,
    U,
    Underline,
    Undo,
    Up,
    V,
    VolumeDown,
    VolumeUp,
    W,
    WWW, // the WWW/World Wide Web key
    X,
    Y,
    Z,

    Unknown, // must be the last value
    DEFINE_ENUM_MAX(Unknown)
};

enum class KeyModifiers : std::uint32_t
{
    None = 0x0,
    LShift = 0x1,
    RShift = 0x2,
    LCtrl = 0x4,
    RCtrl = 0x8,
    LAlt = 0x10,
    RAlt = 0x20,
    LGUI = 0x40,
    RGUI = 0x80,
    NumLock = 0x100,
    CapsLock = 0x200,
    Mode = 0x400,

    Ctrl = LCtrl | RCtrl,
    Shift = LShift | RShift,
    Alt = LAlt | RAlt,
    GUI = LGUI | RGUI,
};

constexpr KeyModifiers operator|(KeyModifiers a, KeyModifiers b) noexcept
{
    return static_cast<KeyModifiers>(
        static_cast<typename std::underlying_type<KeyModifiers>::type>(a)
        | static_cast<typename std::underlying_type<KeyModifiers>::type>(a));
}

inline KeyModifiers &operator|=(KeyModifiers &a, KeyModifiers b) noexcept
{
    return a = a | b;
}

constexpr KeyModifiers operator&(KeyModifiers a, KeyModifiers b) noexcept
{
    return static_cast<KeyModifiers>(
        static_cast<typename std::underlying_type<KeyModifiers>::type>(a)
        & static_cast<typename std::underlying_type<KeyModifiers>::type>(a));
}

inline KeyModifiers &operator&=(KeyModifiers &a, KeyModifiers b) noexcept
{
    return a = a & b;
}

constexpr KeyModifiers operator^(KeyModifiers a, KeyModifiers b) noexcept
{
    return static_cast<KeyModifiers>(
        static_cast<typename std::underlying_type<KeyModifiers>::type>(a)
        ^ static_cast<typename std::underlying_type<KeyModifiers>::type>(a));
}

constexpr KeyModifiers operator~(KeyModifiers v) noexcept
{
    return static_cast<KeyModifiers>(
        ~static_cast<typename std::underlying_type<KeyModifiers>::type>(v));
}

inline KeyModifiers &operator^=(KeyModifiers &a, KeyModifiers b) noexcept
{
    return a = a ^ b;
}

struct KeyEvent : public Event
{
    bool repeat;
    PhysicalKeyCode physicalCode;
    VirtualKeyCode virtualCode;
    KeyModifiers modifiers;
    KeyEvent(bool repeat,
             PhysicalKeyCode physicalCode,
             VirtualKeyCode virtualCode,
             KeyModifiers modifiers)
        : repeat(repeat), physicalCode(physicalCode), virtualCode(virtualCode), modifiers(modifiers)
    {
    }
};

struct KeyDown final : public KeyEvent
{
    virtual const char *getEventName() const noexcept override
    {
        return "KeyDown";
    }
    virtual void dispatch(EventHandler &handler) const override
    {
        handler.onKeyDown(*this);
    }
    KeyDown(bool repeat,
            PhysicalKeyCode physicalCode,
            VirtualKeyCode virtualCode,
            KeyModifiers modifiers)
        : KeyEvent(repeat, physicalCode, virtualCode, modifiers)
    {
    }
};

struct KeyFocusGained final : public Event
{
    virtual const char *getEventName() const noexcept override
    {
        return "KeyFocusGained";
    }
    virtual void dispatch(EventHandler &handler) const override
    {
        handler.onKeyFocusGained(*this);
    }
};

struct KeyFocusLost final : public Event
{
    virtual const char *getEventName() const noexcept override
    {
        return "KeyFocusLost";
    }
    virtual void dispatch(EventHandler &handler) const override
    {
        handler.onKeyFocusLost(*this);
    }
};

struct KeyUp final : public KeyEvent
{
    virtual const char *getEventName() const noexcept override
    {
        return "KeyUp";
    }
    virtual void dispatch(EventHandler &handler) const override
    {
        handler.onKeyUp(*this);
    }
    KeyUp(PhysicalKeyCode physicalCode, VirtualKeyCode virtualCode, KeyModifiers modifiers)
        : KeyEvent(false, physicalCode, virtualCode, modifiers)
    {
    }
};

struct Mouse : public Event
{
    typedef std::uint64_t DeviceId;
    enum class Button
    {
        Left,
        Middle,
        Right,
        X1,
        X2,
        DEFINE_ENUM_MAX(X2)
    };
    DeviceId deviceId;
    explicit Mouse(DeviceId deviceId) : deviceId(deviceId)
    {
    }
};

struct MousePositioned : public Mouse
{
    std::int32_t x;
    std::int32_t y;
    MousePositioned(DeviceId deviceId, std::int32_t x, std::int32_t y) : Mouse(deviceId), x(x), y(y)
    {
    }
};

struct MouseButton : public MousePositioned
{
    Button button;
    std::uint32_t clickCount;
    MouseButton(
        DeviceId deviceId, std::int32_t x, std::int32_t y, Button button, std::uint32_t clickCount)
        : MousePositioned(deviceId, x, y), button(button), clickCount(clickCount)
    {
    }
};

struct MouseButtonDown final : public MouseButton
{
    virtual const char *getEventName() const noexcept override
    {
        return "MouseButtonDown";
    }
    virtual void dispatch(EventHandler &handler) const override
    {
        handler.onMouseButtonDown(*this);
    }
    MouseButtonDown(
        DeviceId deviceId, std::int32_t x, std::int32_t y, Button button, std::uint32_t clickCount)
        : MouseButton(deviceId, x, y, button, clickCount)
    {
    }
};

struct MouseButtonUp final : public MouseButton
{
    virtual const char *getEventName() const noexcept override
    {
        return "MouseButtonUp";
    }
    virtual void dispatch(EventHandler &handler) const override
    {
        handler.onMouseButtonUp(*this);
    }
    MouseButtonUp(
        DeviceId deviceId, std::int32_t x, std::int32_t y, Button button, std::uint32_t clickCount)
        : MouseButton(deviceId, x, y, button, clickCount)
    {
    }
};

struct MouseEnter final : public Event
{
    virtual const char *getEventName() const noexcept override
    {
        return "MouseEnter";
    }
    virtual void dispatch(EventHandler &handler) const override
    {
        handler.onMouseEnter(*this);
    }
};

struct MouseLeave final : public Event
{
    virtual const char *getEventName() const noexcept override
    {
        return "MouseLeave";
    }
    virtual void dispatch(EventHandler &handler) const override
    {
        handler.onMouseLeave(*this);
    }
};

struct MouseMove final : public MousePositioned
{
    std::int32_t dx;
    std::int32_t dy;
    util::EnumArray<bool, Button> buttonStates;
    virtual const char *getEventName() const noexcept override
    {
        return "MouseMove";
    }
    virtual void dispatch(EventHandler &handler) const override
    {
        handler.onMouseMove(*this);
    }
    MouseMove(DeviceId deviceId,
              std::int32_t x,
              std::int32_t y,
              std::int32_t dx,
              std::int32_t dy,
              const util::EnumArray<bool, Button> &buttonStates)
        : MousePositioned(deviceId, x, y), dx(dx), dy(dy), buttonStates(buttonStates)
    {
    }
};

struct MouseWheel final : public Mouse
{
    std::int32_t dx;
    std::int32_t dy;
    virtual const char *getEventName() const noexcept override
    {
        return "MouseWheel";
    }
    virtual void dispatch(EventHandler &handler) const override
    {
        handler.onMouseWheel(*this);
    }
    MouseWheel(DeviceId deviceId, std::int32_t dx, std::int32_t dy)
        : Mouse(deviceId), dx(dx), dy(dy)
    {
    }
};

struct Quit final : public Event
{
    virtual const char *getEventName() const noexcept override
    {
        return "Quit";
    }
    virtual void dispatch(EventHandler &handler) const override
    {
        handler.onQuit(*this);
    }
};

struct Text : public Event
{
    std::string text;
    explicit Text(std::string text) : text(std::move(text))
    {
    }
};

struct TextEditing final : public Text
{
    std::size_t start;
    std::size_t length;
    virtual const char *getEventName() const noexcept override
    {
        return "TextEditing";
    }
    virtual void dispatch(EventHandler &handler) const override
    {
        handler.onTextEditing(*this);
    }
    TextEditing(std::string text, std::size_t start, std::size_t length)
        : Text(std::move(text)), start(start), length(length)
    {
    }
};

struct TextInput final : public Text
{
    virtual const char *getEventName() const noexcept override
    {
        return "TextInput";
    }
    virtual void dispatch(EventHandler &handler) const override
    {
        handler.onTextInput(*this);
    }
    explicit TextInput(std::string text) : Text(std::move(text))
    {
    }
};

struct WindowClose final : public Event
{
    virtual const char *getEventName() const noexcept override
    {
        return "WindowClose";
    }
    virtual void dispatch(EventHandler &handler) const override
    {
        handler.onWindowClose(*this);
    }
};

struct WindowHidden final : public Event
{
    virtual const char *getEventName() const noexcept override
    {
        return "WindowHidden";
    }
    virtual void dispatch(EventHandler &handler) const override
    {
        handler.onWindowHidden(*this);
    }
};

struct WindowMaximized final : public Event
{
    virtual const char *getEventName() const noexcept override
    {
        return "WindowMaximized";
    }
    virtual void dispatch(EventHandler &handler) const override
    {
        handler.onWindowMaximized(*this);
    }
};

struct WindowMinimized final : public Event
{
    virtual const char *getEventName() const noexcept override
    {
        return "WindowMinimized";
    }
    virtual void dispatch(EventHandler &handler) const override
    {
        handler.onWindowMinimized(*this);
    }
};

struct WindowMoved final : public Event
{
    std::int32_t x;
    std::int32_t y;
    virtual const char *getEventName() const noexcept override
    {
        return "WindowMoved";
    }
    virtual void dispatch(EventHandler &handler) const override
    {
        handler.onWindowMoved(*this);
    }
    WindowMoved(std::int32_t x, std::int32_t y) : x(x), y(y)
    {
    }
};

struct WindowSizeBase : public Event
{
    std::uint32_t width;
    std::uint32_t height;
    WindowSizeBase(std::uint32_t width, std::uint32_t height) : width(width), height(height)
    {
    }
};

struct WindowResized final : public WindowSizeBase
{
    virtual const char *getEventName() const noexcept override
    {
        return "WindowResized";
    }
    virtual void dispatch(EventHandler &handler) const override
    {
        handler.onWindowResized(*this);
    }
    WindowResized(std::uint32_t width, std::uint32_t height) : WindowSizeBase(width, height)
    {
    }
};

struct WindowRestored final : public Event
{
    virtual const char *getEventName() const noexcept override
    {
        return "WindowRestored";
    }
    virtual void dispatch(EventHandler &handler) const override
    {
        handler.onWindowRestored(*this);
    }
};

struct WindowShown final : public Event
{
    virtual const char *getEventName() const noexcept override
    {
        return "WindowShown";
    }
    virtual void dispatch(EventHandler &handler) const override
    {
        handler.onWindowShown(*this);
    }
};

struct WindowSizeChanged final : public WindowSizeBase
{
    virtual const char *getEventName() const noexcept override
    {
        return "WindowSizeChanged";
    }
    virtual void dispatch(EventHandler &handler) const override
    {
        handler.onWindowSizeChanged(*this);
    }
    WindowSizeChanged(std::uint32_t width, std::uint32_t height) : WindowSizeBase(width, height)
    {
    }
};
}
}
}
}

#endif /* UI_EVENT_H_ */
