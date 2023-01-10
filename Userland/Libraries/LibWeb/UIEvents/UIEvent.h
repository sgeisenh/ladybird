/*
 * Copyright (c) 2020-2022, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/RefPtr.h>
#include <LibWeb/DOM/Event.h>
#include <LibWeb/HTML/Window.h>

namespace Web::UIEvents {

struct UIEventInit : public DOM::EventInit {
    JS::GCPtr<HTML::Window> view;
    int detail { 0 };
};

class UIEvent : public DOM::Event {
    WEB_PLATFORM_OBJECT(UIEvent, DOM::Event);

public:
    static UIEvent* create(JS::Realm&, DeprecatedFlyString const& type);
    static UIEvent* construct_impl(JS::Realm&, DeprecatedFlyString const& event_name, UIEventInit const& event_init);

    virtual ~UIEvent() override;

    HTML::Window const* view() const { return m_view.ptr(); }
    int detail() const { return m_detail; }
    virtual u32 which() const { return 0; }

    void init_ui_event(DeprecatedString const& type, bool bubbles, bool cancelable, HTML::Window* view, int detail)
    {
        init_event(type, bubbles, cancelable);
        m_view = view;
        m_detail = detail;
    }

protected:
    UIEvent(JS::Realm&, DeprecatedFlyString const& event_name);
    UIEvent(JS::Realm&, DeprecatedFlyString const& event_name, UIEventInit const& event_init);

    virtual void initialize(JS::Realm&) override;
    virtual void visit_edges(Cell::Visitor&) override;

    JS::GCPtr<HTML::Window> m_view;
    int m_detail { 0 };
};

}
