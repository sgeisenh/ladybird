/*
 * Copyright (c) 2020, the SerenityOS developers.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <LibWeb/DOM/ARIARoleNames.h>
#include <LibWeb/HTML/HTMLElement.h>

namespace Web::HTML {

class HTMLTimeElement final : public HTMLElement {
    WEB_PLATFORM_OBJECT(HTMLTimeElement, HTMLElement);

public:
    virtual ~HTMLTimeElement() override;

    // https://www.w3.org/TR/html-aria/#el-time
    virtual DeprecatedFlyString default_role() const override { return DOM::ARIARoleNames::time; }

private:
    HTMLTimeElement(DOM::Document&, DOM::QualifiedName);

    virtual void initialize(JS::Realm&) override;
};

}
