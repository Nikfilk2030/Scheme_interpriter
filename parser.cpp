#include <parser.h>

#define CheckToken(TokenType, token, behaviour)            \
    if (TokenType* ptr = std::get_if<TokenType>(&token)) { \
        behaviour;                                         \
    }

std::shared_ptr<Object> Read(Tokenizer* tokenizer) {
    Token token = tokenizer->GetToken();
    tokenizer->Next();
    if (BooleanToken* ptr = std::get_if<BooleanToken>(&token)) {
        if (*ptr == BooleanToken::TRUE) {
            return std::make_shared<Bool>(true);
        } else {
            return std::make_shared<Bool>(false);
        }
    }
    CheckToken(ConstantToken, token, return std::make_shared<Number>(ptr->value));
    if (BracketToken* ptr = std::get_if<BracketToken>(&token)) {
        if (*ptr == BracketToken::CLOSE) {
            throw SyntaxError("Close bracket without open");
        }
        return ReadList(tokenizer);
    }
    if (QuoteToken* ptr = std::get_if<QuoteToken>(&token)) {
        return std::make_shared<Cell>(std::make_shared<Symbol>("quote"), Read(tokenizer));
    }
    /// TODO стереть

    CheckToken(SymbolToken, token, return std::make_shared<Symbol>(ptr->name));
    throw SyntaxError("undefined token");
}

bool IsCloseBracket(Tokenizer* tokenizer) {
    Token token = tokenizer->GetToken();
    if (BracketToken* bracket_ptr = std::get_if<BracketToken>(&token)) {
        if (*bracket_ptr == BracketToken::CLOSE) {
            return true;
        }
    }
    return false;
}

/// TODO где-то тут мне надо кидаться ошибкой
std::shared_ptr<Object> ReadList(Tokenizer* tokenizer) {
    if (IsCloseBracket(tokenizer)) {
        tokenizer->Next();
        return nullptr;
    }
    std::shared_ptr<Object> first = Read(tokenizer);

    Token dot_token_ptr = tokenizer->GetToken();
    if (!std::get_if<DotToken>(&dot_token_ptr)) {
        return std::make_shared<Cell>(first, ReadList(tokenizer));
    }

    tokenizer->Next();
    std::shared_ptr<Object> second = Read(tokenizer);
    Token token = tokenizer->GetToken();
    if (BracketToken* bracket = std::get_if<BracketToken>(&token)) {
        if (*bracket == BracketToken::CLOSE) {
            tokenizer->Next();
            return std::make_shared<Cell>(first, second);
        }
    }
    throw SyntaxError("Need close bracket");
}

std::shared_ptr<Object> ReadAll(Tokenizer* tokenizer) {
    auto object_ptr = Read(tokenizer);
    if (!tokenizer->IsEnd()) {
        throw SyntaxError("tokenizer did not end");
    }
    return object_ptr;
}