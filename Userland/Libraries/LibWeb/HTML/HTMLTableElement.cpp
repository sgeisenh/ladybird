/*
 * Copyright (c) 2020, Andreas Kling <kling@serenityos.org>
 * Copyright (c) 2021, Adam Hodgen <ant1441@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibWeb/Bindings/Intrinsics.h>
#include <LibWeb/CSS/Parser/Parser.h>
#include <LibWeb/DOM/ElementFactory.h>
#include <LibWeb/DOM/HTMLCollection.h>
#include <LibWeb/HTML/HTMLTableColElement.h>
#include <LibWeb/HTML/HTMLTableElement.h>
#include <LibWeb/HTML/HTMLTableRowElement.h>
#include <LibWeb/HTML/Parser/HTMLParser.h>
#include <LibWeb/Namespace.h>

namespace Web::HTML {

HTMLTableElement::HTMLTableElement(DOM::Document& document, DOM::QualifiedName qualified_name)
    : HTMLElement(document, move(qualified_name))
{
}

HTMLTableElement::~HTMLTableElement() = default;

void HTMLTableElement::initialize(JS::Realm& realm)
{
    Base::initialize(realm);
    set_prototype(&Bindings::ensure_web_prototype<Bindings::HTMLTableElementPrototype>(realm, "HTMLTableElement"));
}

void HTMLTableElement::visit_edges(Cell::Visitor& visitor)
{
    Base::visit_edges(visitor);
    visitor.visit(m_rows);
    visitor.visit(m_t_bodies);
}

void HTMLTableElement::apply_presentational_hints(CSS::StyleProperties& style) const
{
    for_each_attribute([&](auto& name, auto& value) {
        if (name == HTML::AttributeNames::width) {
            if (auto parsed_value = parse_nonzero_dimension_value(value))
                style.set_property(CSS::PropertyID::Width, parsed_value.release_nonnull());
            return;
        }
        if (name == HTML::AttributeNames::height) {
            if (auto parsed_value = parse_nonzero_dimension_value(value))
                style.set_property(CSS::PropertyID::Height, parsed_value.release_nonnull());
            return;
        }
        if (name == HTML::AttributeNames::bgcolor) {
            auto color = Color::from_string(value);
            if (color.has_value())
                style.set_property(CSS::PropertyID::BackgroundColor, CSS::ColorStyleValue::create(color.value()));
            return;
        }
    });
}

// https://html.spec.whatwg.org/multipage/tables.html#dom-table-caption
JS::GCPtr<HTMLTableCaptionElement> HTMLTableElement::caption()
{
    // The caption IDL attribute must return, on getting, the first caption element child of the table element,
    // if any, or null otherwise.
    return first_child_of_type<HTMLTableCaptionElement>();
}

// https://html.spec.whatwg.org/multipage/tables.html#dom-table-caption
void HTMLTableElement::set_caption(HTMLTableCaptionElement* caption)
{
    // On setting, the first caption element child of the table element, if any, must be removed,
    // and the new value, if not null, must be inserted as the first node of the table element.
    delete_caption();

    if (caption)
        MUST(pre_insert(*caption, first_child()));
}

// https://html.spec.whatwg.org/multipage/tables.html#dom-table-createcaption
JS::NonnullGCPtr<HTMLTableCaptionElement> HTMLTableElement::create_caption()
{
    auto maybe_caption = caption();
    if (maybe_caption) {
        return *maybe_caption;
    }

    auto caption = DOM::create_element(document(), TagNames::caption, Namespace::HTML);
    MUST(pre_insert(caption, first_child()));
    return static_cast<HTMLTableCaptionElement&>(*caption);
}

// https://html.spec.whatwg.org/multipage/tables.html#dom-table-deletecaption
void HTMLTableElement::delete_caption()
{
    auto maybe_caption = caption();
    if (maybe_caption) {
        maybe_caption->remove(false);
    }
}

// https://html.spec.whatwg.org/multipage/tables.html#dom-table-thead
JS::GCPtr<HTMLTableSectionElement> HTMLTableElement::t_head()
{
    // The tHead IDL attribute must return, on getting, the first thead element child of the table element,
    // if any, or null otherwise.
    for (auto* child = first_child(); child; child = child->next_sibling()) {
        if (is<HTMLTableSectionElement>(*child)) {
            auto table_section_element = &verify_cast<HTMLTableSectionElement>(*child);
            if (table_section_element->local_name() == TagNames::thead)
                return table_section_element;
        }
    }

    return nullptr;
}

// https://html.spec.whatwg.org/multipage/tables.html#dom-table-thead
WebIDL::ExceptionOr<void> HTMLTableElement::set_t_head(HTMLTableSectionElement* thead)
{
    // If the new value is neither null nor a thead element, then a "HierarchyRequestError" DOMException must be thrown instead.
    if (thead && thead->local_name() != TagNames::thead)
        return WebIDL::HierarchyRequestError::create(realm(), "Element is not thead");

    // On setting, if the new value is null or a thead element, the first thead element child of the table element,
    // if any, must be removed,
    delete_t_head();

    if (!thead)
        return {};

    // and the new value, if not null, must be inserted immediately before the first element in the table element
    // that is neither a caption element nor a colgroup element, if any,
    // or at the end of the table if there are no such elements.

    // We insert the new thead after any <caption> or <colgroup> elements
    DOM::Node* child_to_insert_before = nullptr;
    for (auto* child = first_child(); child; child = child->next_sibling()) {
        if (!is<HTMLElement>(*child))
            continue;
        if (is<HTMLTableCaptionElement>(*child))
            continue;
        if (is<HTMLTableColElement>(*child)) {
            auto table_col_element = &verify_cast<HTMLTableColElement>(*child);
            if (table_col_element->local_name() == TagNames::colgroup)
                continue;
        }

        // We have found an element which is not a <caption> or <colgroup>, we'll insert before this
        child_to_insert_before = child;
        break;
    }

    TRY(pre_insert(*thead, child_to_insert_before));

    return {};
}

// https://html.spec.whatwg.org/multipage/tables.html#dom-table-createthead
JS::NonnullGCPtr<HTMLTableSectionElement> HTMLTableElement::create_t_head()
{
    auto maybe_thead = t_head();
    if (maybe_thead)
        return *maybe_thead;

    auto thead = DOM::create_element(document(), TagNames::thead, Namespace::HTML);

    // We insert the new thead after any <caption> or <colgroup> elements
    DOM::Node* child_to_insert_before = nullptr;
    for (auto* child = first_child(); child; child = child->next_sibling()) {
        if (!is<HTMLElement>(*child))
            continue;
        if (is<HTMLTableCaptionElement>(*child))
            continue;
        if (is<HTMLTableColElement>(*child)) {
            auto table_col_element = &verify_cast<HTMLTableColElement>(*child);
            if (table_col_element->local_name() == TagNames::colgroup)
                continue;
        }

        // We have found an element which is not a <caption> or <colgroup>, we'll insert before this
        child_to_insert_before = child;
        break;
    }

    MUST(pre_insert(thead, child_to_insert_before));

    return static_cast<HTMLTableSectionElement&>(*thead);
}

// https://html.spec.whatwg.org/multipage/tables.html#dom-table-deletethead
void HTMLTableElement::delete_t_head()
{
    auto maybe_thead = t_head();
    if (maybe_thead) {
        maybe_thead->remove(false);
    }
}

// https://html.spec.whatwg.org/multipage/tables.html#dom-table-tfoot
JS::GCPtr<HTMLTableSectionElement> HTMLTableElement::t_foot()
{
    // The tFoot IDL attribute must return, on getting, the first tfoot element child of the table element,
    // if any, or null otherwise.
    for (auto* child = first_child(); child; child = child->next_sibling()) {
        if (is<HTMLTableSectionElement>(*child)) {
            auto table_section_element = &verify_cast<HTMLTableSectionElement>(*child);
            if (table_section_element->local_name() == TagNames::tfoot)
                return table_section_element;
        }
    }

    return nullptr;
}

// https://html.spec.whatwg.org/multipage/tables.html#dom-table-tfoot
WebIDL::ExceptionOr<void> HTMLTableElement::set_t_foot(HTMLTableSectionElement* tfoot)
{
    // If the new value is neither null nor a tfoot element, then a "HierarchyRequestError" DOMException must be thrown instead.
    if (tfoot && tfoot->local_name() != TagNames::tfoot)
        return WebIDL::HierarchyRequestError::create(realm(), "Element is not tfoot");

    // On setting, if the new value is null or a tfoot element, the first tfoot element child of the table element,
    // if any, must be removed,
    delete_t_foot();

    // and the new value, if not null, must be inserted at the end of the table.
    if (tfoot) {
        TRY(append_child(*tfoot));
    }

    return {};
}

// https://html.spec.whatwg.org/multipage/tables.html#dom-table-createtfoot
JS::NonnullGCPtr<HTMLTableSectionElement> HTMLTableElement::create_t_foot()
{
    auto maybe_tfoot = t_foot();
    if (maybe_tfoot)
        return *maybe_tfoot;

    auto tfoot = DOM::create_element(document(), TagNames::tfoot, Namespace::HTML);
    MUST(append_child(tfoot));
    return static_cast<HTMLTableSectionElement&>(*tfoot);
}

// https://html.spec.whatwg.org/multipage/tables.html#dom-table-deletetfoot
void HTMLTableElement::delete_t_foot()
{
    auto maybe_tfoot = t_foot();
    if (maybe_tfoot) {
        maybe_tfoot->remove(false);
    }
}

// https://html.spec.whatwg.org/multipage/tables.html#dom-table-tbodies
JS::NonnullGCPtr<DOM::HTMLCollection> HTMLTableElement::t_bodies()
{
    // The tBodies attribute must return an HTMLCollection rooted at the table node,
    // whose filter matches only tbody elements that are children of the table element.
    if (!m_t_bodies) {
        m_t_bodies = DOM::HTMLCollection::create(*this, [](DOM::Element const& element) {
            return element.local_name() == TagNames::tbody;
        });
    }
    return *m_t_bodies;
}

// https://html.spec.whatwg.org/multipage/tables.html#dom-table-createtbody
JS::NonnullGCPtr<HTMLTableSectionElement> HTMLTableElement::create_t_body()
{
    auto tbody = DOM::create_element(document(), TagNames::tbody, Namespace::HTML);

    // We insert the new tbody after the last <tbody> element
    DOM::Node* child_to_insert_before = nullptr;
    for (auto* child = last_child(); child; child = child->previous_sibling()) {
        if (!is<HTMLElement>(*child))
            continue;
        if (is<HTMLTableSectionElement>(*child)) {
            auto table_section_element = &verify_cast<HTMLTableSectionElement>(*child);
            if (table_section_element->local_name() == TagNames::tbody) {
                // We have found an element which is a <tbody> we'll insert after this
                child_to_insert_before = child->next_sibling();
                break;
            }
        }
    }

    MUST(pre_insert(tbody, child_to_insert_before));

    return static_cast<HTMLTableSectionElement&>(*tbody);
}

// https://html.spec.whatwg.org/multipage/tables.html#dom-table-rows
JS::NonnullGCPtr<DOM::HTMLCollection> HTMLTableElement::rows()
{
    HTMLTableElement* table_node = this;
    // FIXME:  The elements in the collection must be ordered such that those elements whose parent is a thead are
    //         included first, in tree order, followed by those elements whose parent is either a table or tbody
    //         element, again in tree order, followed finally by those elements whose parent is a tfoot element,
    //         still in tree order.
    // How do you sort HTMLCollection?

    if (!m_rows) {
        m_rows = DOM::HTMLCollection::create(*this, [table_node](DOM::Element const& element) {
            // Only match TR elements which are:
            // * children of the table element
            // * children of the thead, tbody, or tfoot elements that are themselves children of the table element
            if (!is<HTMLTableRowElement>(element)) {
                return false;
            }
            if (element.parent_element() == table_node)
                return true;

            if (element.parent_element() && (element.parent_element()->local_name() == TagNames::thead || element.parent_element()->local_name() == TagNames::tbody || element.parent_element()->local_name() == TagNames::tfoot)
                && element.parent()->parent() == table_node) {
                return true;
            }

            return false;
        });
    }
    return *m_rows;
}

// https://html.spec.whatwg.org/multipage/tables.html#dom-table-insertrow
WebIDL::ExceptionOr<JS::NonnullGCPtr<HTMLTableRowElement>> HTMLTableElement::insert_row(long index)
{
    auto rows = this->rows();
    auto rows_length = rows->length();

    if (index < -1 || index > (long)rows_length) {
        return WebIDL::IndexSizeError::create(realm(), "Index is negative or greater than the number of rows");
    }
    auto& tr = static_cast<HTMLTableRowElement&>(*DOM::create_element(document(), TagNames::tr, Namespace::HTML));
    if (rows_length == 0 && !has_child_of_type<HTMLTableRowElement>()) {
        auto tbody = DOM::create_element(document(), TagNames::tbody, Namespace::HTML);
        TRY(tbody->append_child(tr));
        TRY(append_child(tbody));
    } else if (rows_length == 0) {
        auto tbody = last_child_of_type<HTMLTableRowElement>();
        TRY(tbody->append_child(tr));
    } else if (index == -1 || index == (long)rows_length) {
        auto parent_of_last_tr = rows->item(rows_length - 1)->parent_element();
        TRY(parent_of_last_tr->append_child(tr));
    } else {
        rows->item(index)->parent_element()->insert_before(tr, rows->item(index));
    }
    return JS::NonnullGCPtr(tr);
}

// https://html.spec.whatwg.org/multipage/tables.html#dom-table-deleterow
WebIDL::ExceptionOr<void> HTMLTableElement::delete_row(long index)
{
    auto rows = this->rows();
    auto rows_length = rows->length();

    // 1. If index is less than −1 or greater than or equal to the number of elements in the rows collection, then throw an "IndexSizeError" DOMException.
    if (index < -1 || index >= (long)rows_length)
        return WebIDL::IndexSizeError::create(realm(), "Index is negative or greater than or equal to the number of rows");

    // 2. If index is −1, then remove the last element in the rows collection from its parent, or do nothing if the rows collection is empty.
    if (index == -1) {
        if (rows_length == 0)
            return {};

        auto row_to_remove = rows->item(rows_length - 1);
        row_to_remove->remove(false);
        return {};
    }

    // 3. Otherwise, remove the indexth element in the rows collection from its parent.
    auto row_to_remove = rows->item(index);
    row_to_remove->remove(false);
    return {};
}

}
