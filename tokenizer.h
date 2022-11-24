#pragma once

#include <istream>
#include <optional>
#include <variant>

#include "error.h"

struct SymbolToken {
    SymbolToken(std::string new_name) : name(new_name){};

    std::string name;

    bool operator==(const SymbolToken& other) const;
};

struct QuoteToken {
    bool operator==(const QuoteToken&) const;
};

struct DotToken {
    bool operator==(const DotToken&) const;
};

enum class BracketToken { OPEN, CLOSE };

struct ConstantToken {
    ConstantToken(int new_value) : value(new_value){};

    int value;

    bool operator==(const ConstantToken& other) const;
};

enum class BooleanToken { TRUE, FALSE };

struct EOFToken {
    bool operator==(const EOFToken&) const {
        return true;
    }
};

using Token = std::variant<ConstantToken, BracketToken, SymbolToken, QuoteToken, DotToken,
                           BooleanToken, EOFToken>;

class Tokenizer {
public:
    Tokenizer(std::istream* in);

    bool IsEnd();

    void Next();

    Token GetToken();

protected:
    Token ReadNextToken();

protected:
    std::istream* stream_;
    Token token_;
};