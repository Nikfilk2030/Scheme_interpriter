#include <tokenizer.h>

#include <cctype>
#include <set>

Tokenizer::Tokenizer(std::istream *in) : stream_(in), token_(ReadNextToken()){};

bool Tokenizer::IsEnd() {
    return token_ == Token(EOFToken());
}

void Tokenizer::Next() {
    if (IsEnd()) {
        throw SyntaxError("fall in GetToken");
    }
    token_ = ReadNextToken();
}

Token Tokenizer::GetToken() {
    if (IsEnd()) {
        throw SyntaxError("fall in GetToken");
    }
    return token_;
}

static std::set<char> single_symbols = {'(', ')', '\'', '.'};
static std::set<char> legal_special_symbols = {'<', '=', '>', '*', '/', '#', '+', '-', '!', '?'};

void CheckSymbolLegality(char chr) {
    if (legal_special_symbols.contains(chr) || isalpha(chr) || isdigit(chr) ||
        single_symbols.contains(chr)) {
        return;
    }
    throw SyntaxError("Invalid symbol");
}

#define ReturnTokenIf(element, target, result) \
    if (element == target) {                   \
        return Token(result);                  \
    }

Token Tokenizer::ReadNextToken() {
    while (stream_->peek() != EOF && std::isspace(stream_->peek())) {
        stream_->get();
    }
    if (stream_->peek() == EOF) {
        return Token(EOFToken());
    }

    std::string raw_token;
    bool is_digit = false;

    while (stream_->peek() != EOF && !std::isspace(stream_->peek())) {
        if (!raw_token.empty() && single_symbols.contains(stream_->peek())) {
            break;
        }
        if (is_digit && !isdigit(stream_->peek())) {
            break;
        }

        char chr = stream_->get();
        CheckSymbolLegality(chr);

        ReturnTokenIf(chr, '(', BracketToken::OPEN);
        ReturnTokenIf(chr, ')', BracketToken::CLOSE);
        ReturnTokenIf(chr, '\'', QuoteToken());
        ReturnTokenIf(chr, '.', DotToken());

        // digit check
        if (isdigit(chr) &&
            (raw_token.empty() ||
             (raw_token.size() == 1 && (raw_token.back() == '+' || raw_token.back() == '-')))) {
            is_digit = true;
        }

        raw_token.push_back(chr);
    }
    ReturnTokenIf(raw_token, "#t", BooleanToken::TRUE);
    ReturnTokenIf(raw_token, "#f", BooleanToken::FALSE);
    ReturnTokenIf(raw_token, "quote", QuoteToken());  /// TODO добавил
    if (!is_digit) {
        return Token(SymbolToken(raw_token));
    }
    // Only constant token is left
    return Token(ConstantToken(std::stoi(raw_token)));
}

bool SymbolToken::operator==(const SymbolToken &other) const {
    return name == other.name;
}

bool QuoteToken::operator==(const QuoteToken &) const {
    return true;
}

bool DotToken::operator==(const DotToken &) const {
    return true;
}

bool ConstantToken::operator==(const ConstantToken &other) const {
    return value == other.value;
}
