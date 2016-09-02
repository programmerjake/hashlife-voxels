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

#ifndef UI_EVENT_H_
#define UI_EVENT_H_

#include <memory>

namespace programmerjake
{
namespace voxels
{
namespace ui
{
struct QuitEvent;
struct AppTerminatingEvent;
struct AppLowMemoryEvent;
struct AppWillEnterBackgroundEvent;
struct AppDidEnterBackgroundEvent;
struct AppWillEnterForegoundEvent;
struct AppDidEnterForegoundEvent;
struct WindowEvent;
struct KeyDownEvent;
struct KeyUpEvent;
struct TextEditingEvent;
struct TextInputEvent;
struct MouseMoveEvent;
struct MouseButtonDownEvent;
struct MouseButtonUpEvent;
struct MouseWheelEvent;
struct FingerDownEvent;
struct FingerMoveEvent;
struct FingerUpEvent;
struct ClipboardUpdateEvent;

struct EventHandler : public std::enable_shared_from_this<EventHandler>
{
    virtual ~EventHandler() = default;
    virtual void onQuit(const QuitEvent &event) = 0;
    virtual void onAppTerminating(const AppTerminatingEvent &event) = 0;
    virtual void onAppLowMemory(const AppLowMemoryEvent &event) = 0;
    virtual void onAppWillEnterBackground(const AppWillEnterBackgroundEvent &event) = 0;
    virtual void onAppDidEnterBackground(const AppDidEnterBackgroundEvent &event) = 0;
    virtual void onAppWillEnterForegound(const AppWillEnterForegoundEvent &event) = 0;
    virtual void onAppDidEnterForegound(const AppDidEnterForegoundEvent &event) = 0;
    virtual void onWindow(const WindowEvent &event) = 0;
    virtual void onKeyDown(const KeyDownEvent &event) = 0;
    virtual void onKeyUp(const KeyUpEvent &event) = 0;
    virtual void onTextEditing(const TextEditingEvent &event) = 0;
    virtual void onTextInput(const TextInputEvent &event) = 0;
    virtual void onMouseMove(const MouseMoveEvent &event) = 0;
    virtual void onMouseButtonDown(const MouseButtonDownEvent &event) = 0;
    virtual void onMouseButtonUp(const MouseButtonUpEvent &event) = 0;
    virtual void onMouseWheel(const MouseWheelEvent &event) = 0;
    virtual void onFingerDown(const FingerDownEvent &event) = 0;
    virtual void onFingerMove(const FingerMoveEvent &event) = 0;
    virtual void onFingerUp(const FingerUpEvent &event) = 0;
    virtual void onClipboardUpdate(const ClipboardUpdateEvent &event) = 0;
};

struct Event
{
    virtual ~Event() = default;
    virtual void dispatch(EventHandler &handler) const = 0;
};

struct QuitEvent final : public Event
{
    virtual void dispatch(EventHandler &handler) const override
    {
        handler.onQuit(*this);
    }
};

struct AppTerminatingEvent final : public Event
{
    virtual void dispatch(EventHandler &handler) const override
    {
        handler.onAppTerminating(*this);
    }
};

struct AppLowMemoryEvent final : public Event
{
    virtual void dispatch(EventHandler &handler) const override
    {
        handler.onAppLowMemory(*this);
    }
};

struct AppWillEnterBackgroundEvent final : public Event
{
    virtual void dispatch(EventHandler &handler) const override
    {
        handler.onAppWillEnterBackground(*this);
    }
};

struct AppDidEnterBackgroundEvent final : public Event
{
    virtual void dispatch(EventHandler &handler) const override
    {
        handler.onAppDidEnterBackground(*this);
    }
};

struct AppWillEnterForegoundEvent final : public Event
{
    virtual void dispatch(EventHandler &handler) const override
    {
        handler.onAppWillEnterForegound(*this);
    }
};

struct AppDidEnterForegoundEvent final : public Event
{
    virtual void dispatch(EventHandler &handler) const override
    {
        handler.onAppDidEnterForegound(*this);
    }
};

struct WindowEvent final : public Event
{
    virtual void dispatch(EventHandler &handler) const override
    {
        handler.onWindow(*this);
    }
};

struct KeyDownEvent final : public Event
{
    virtual void dispatch(EventHandler &handler) const override
    {
        handler.onKeyDown(*this);
    }
};

struct KeyUpEvent final : public Event
{
    virtual void dispatch(EventHandler &handler) const override
    {
        handler.onKeyUp(*this);
    }
};

struct TextEditingEvent final : public Event
{
    virtual void dispatch(EventHandler &handler) const override
    {
        handler.onTextEditing(*this);
    }
};

struct TextInputEvent final : public Event
{
    virtual void dispatch(EventHandler &handler) const override
    {
        handler.onTextInput(*this);
    }
};

struct MouseMoveEvent final : public Event
{
    virtual void dispatch(EventHandler &handler) const override
    {
        handler.onMouseMove(*this);
    }
};

struct MouseButtonDownEvent final : public Event
{
    virtual void dispatch(EventHandler &handler) const override
    {
        handler.onMouseButtonDown(*this);
    }
};

struct MouseButtonUpEvent final : public Event
{
    virtual void dispatch(EventHandler &handler) const override
    {
        handler.onMouseButtonUp(*this);
    }
};

struct MouseWheelEvent final : public Event
{
    virtual void dispatch(EventHandler &handler) const override
    {
        handler.onMouseWheel(*this);
    }
};

struct FingerDownEvent final : public Event
{
    virtual void dispatch(EventHandler &handler) const override
    {
        handler.onFingerDown(*this);
    }
};

struct FingerMoveEvent final : public Event
{
    virtual void dispatch(EventHandler &handler) const override
    {
        handler.onFingerMove(*this);
    }
};

struct FingerUpEvent final : public Event
{
    virtual void dispatch(EventHandler &handler) const override
    {
        handler.onFingerUp(*this);
    }
};

struct ClipboardUpdateEvent final : public Event
{
    virtual void dispatch(EventHandler &handler) const override
    {
        handler.onClipboardUpdate(*this);
    }
};
}
}
}

#endif /* UI_EVENT_H_ */
