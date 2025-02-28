/*
 * Copyright (c) 2019-2020, Sergey Bugaev <bugaevc@serenityos.org>
 * Copyright (c) 2021, Peter Elliott <pelliott@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Noncopyable.h>
#include <AK/NonnullOwnPtrVector.h>
#include <AK/OwnPtr.h>
#include <AK/String.h>

namespace Markdown {

class Text final {
public:
    class Node {
    public:
        virtual void render_to_html(StringBuilder& builder) const = 0;
        virtual void render_for_terminal(StringBuilder& builder) const = 0;
        virtual size_t terminal_length() const = 0;

        virtual ~Node() { }
    };

    class EmphasisNode : public Node {
    public:
        bool strong;
        NonnullOwnPtr<Node> child;

        EmphasisNode(bool strong, NonnullOwnPtr<Node> child)
            : strong(strong)
            , child(move(child))
        {
        }

        virtual void render_to_html(StringBuilder& builder) const override;
        virtual void render_for_terminal(StringBuilder& builder) const override;
        virtual size_t terminal_length() const override;
    };

    class CodeNode : public Node {
    public:
        NonnullOwnPtr<Node> code;

        CodeNode(NonnullOwnPtr<Node> code)
            : code(move(code))
        {
        }

        virtual void render_to_html(StringBuilder& builder) const override;
        virtual void render_for_terminal(StringBuilder& builder) const override;
        virtual size_t terminal_length() const override;
    };

    class BreakNode : public Node {
    public:
        virtual void render_to_html(StringBuilder& builder) const override;
        virtual void render_for_terminal(StringBuilder& builder) const override;
        virtual size_t terminal_length() const override;
    };

    class TextNode : public Node {
    public:
        String text;
        bool collapsible;

        TextNode(StringView const& text)
            : text(text)
            , collapsible(true)
        {
        }

        TextNode(StringView const& text, bool collapsible)
            : text(text)
            , collapsible(collapsible)
        {
        }

        virtual void render_to_html(StringBuilder& builder) const override;
        virtual void render_for_terminal(StringBuilder& builder) const override;
        virtual size_t terminal_length() const override;
    };

    class LinkNode : public Node {
    public:
        bool is_image;
        NonnullOwnPtr<Node> text;
        NonnullOwnPtr<Node> href;

        LinkNode(bool is_image, NonnullOwnPtr<Node> text, NonnullOwnPtr<Node> href)
            : is_image(is_image)
            , text(move(text))
            , href(move(href))
        {
        }

        virtual void render_to_html(StringBuilder& builder) const override;
        virtual void render_for_terminal(StringBuilder& builder) const override;
        virtual size_t terminal_length() const override;
    };

    class MultiNode : public Node {
    public:
        NonnullOwnPtrVector<Node> children;

        virtual void render_to_html(StringBuilder& builder) const override;
        virtual void render_for_terminal(StringBuilder& builder) const override;
        virtual size_t terminal_length() const override;
    };

    size_t terminal_length() const;

    String render_to_html() const;
    String render_for_terminal() const;

    static Text parse(StringView const&);

private:
    struct Token {
        String data;
        // Flanking basically means that a delimiter run has a non-whitespace,
        // non-punctuation character on the corresponsing side. For a more exact
        // definition, see the CommonMark spec.
        bool left_flanking;
        bool right_flanking;
        bool punct_before;
        bool punct_after;
        // is_run indicates that this token is a 'delimiter run'. A delimiter
        // run occurs when several of the same sytactical character ('`', '_',
        // or '*') occur in a row.
        bool is_run;

        char run_char() const
        {
            VERIFY(is_run);
            return data[0];
        }
        char run_length() const
        {
            VERIFY(is_run);
            return data.length();
        }
        bool is_space() const
        {
            return data[0] == ' ';
        }
        bool operator==(StringView const& str) const { return str == data; }
    };

    static Vector<Token> tokenize(StringView const&);

    static bool can_open(Token const& opening);
    static bool can_close_for(Token const& opening, Token const& closing);

    static NonnullOwnPtr<MultiNode> parse_sequence(Vector<Token>::ConstIterator& tokens, bool in_link);
    static NonnullOwnPtr<Node> parse_break(Vector<Token>::ConstIterator& tokens);
    static NonnullOwnPtr<Node> parse_newline(Vector<Token>::ConstIterator& tokens);
    static NonnullOwnPtr<Node> parse_emph(Vector<Token>::ConstIterator& tokens, bool in_link);
    static NonnullOwnPtr<Node> parse_code(Vector<Token>::ConstIterator& tokens);
    static NonnullOwnPtr<Node> parse_link(Vector<Token>::ConstIterator& tokens);

    OwnPtr<Node> m_node;
};

}
