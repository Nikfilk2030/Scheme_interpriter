#pragma once

#include <memory>
#include <optional>
#include <string>
#include <typeinfo>
#include <unordered_map>

#include "error.h"

class Scope;

class Object : public std::enable_shared_from_this<Object> {
public:
    virtual ~Object() = default;

    virtual std::string ToString() {
        throw RuntimeError("Calling ToSting from base class");
    };

    virtual std::shared_ptr<Object> Eval(Scope* scope) {
        throw RuntimeError("Calling Eval from base class");
    };
};

template <class T>
std::shared_ptr<T> As(const std::shared_ptr<Object>& obj) {
    return std::dynamic_pointer_cast<T>(obj);
}

template <class T>
bool Is(const std::shared_ptr<Object>& obj) {
    return As<T>(obj) != nullptr;
}

class Scope {
public:
    std::shared_ptr<Object> SearchForName(const std::string& name) {
        if (current_namespace_.contains(name)) {
            return current_namespace_.at(name);
        }
        if (parent_scope_ != nullptr) {
            return parent_scope_->SearchForName(name);
        }
        return nullptr;
    }
    void Define(const std::string& name, std::shared_ptr<Object> argument) {
        current_namespace_[name] = argument;
    }
    void Set(const std::string& name, std::shared_ptr<Object> argument) {
        if (SearchForName(name) != nullptr) {
            if (current_namespace_.contains(name)) {
                current_namespace_[name] = argument;
            }
            if (parent_scope_ != nullptr) {
                return parent_scope_->Set(name, argument);
            }
        }
        throw NameError("cannot find this name in any namespace");
    }
    //    void ChangeParent(Scope* par) {
    //        parent_scope_ = par;
    //    }
    //    void Create(const std::string& name) {
    //        current_namespace_[name] = std::make_shared<Object>();
    //    }
    //    void Reset() {
    //        for (auto& [name, ptr] : current_namespace_) {
    //            ptr = std::make_shared<Object>();
    //        }
    //    }

protected:
    Scope* parent_scope_;
    std::unordered_map<std::string, std::shared_ptr<Object>> current_namespace_;
};

class Function : public Object {
public:
    virtual std::shared_ptr<Object> Apply(std::shared_ptr<Object>, Scope*) {
        throw RuntimeError("Calling Apply from base class");
    }
};


std::optional<std::shared_ptr<Function>> GetFunction(const std::string& name);

class Symbol : public Object {
public:
    Symbol(const std::string& name) : name_(name){};

    const std::string& GetName() const {
        return name_;
    }

    std::string ToString() override {
        return name_;
    }

    std::shared_ptr<Object> Eval(Scope* scope) override {
        auto func = GetFunction(name_);
        if (func != std::nullopt) {
            return *func;
        }
        throw NameError("wrong command name");
    }

protected:
    std::string name_;
};

class Bool : public Object {
public:
    Bool(bool bool_value = false) : bool_value_(bool_value){};

    bool GetBoolValue() const {
        return bool_value_;
    }

    /// TODO так?
    std::string ToString() override {
        if (bool_value_) {
            return "#t";
        } else {
            return "#f";
        }
    }

    std::shared_ptr<Object> Eval(Scope* scope) override {
        return shared_from_this();
    }

protected:
    bool bool_value_;
};

class Number : public Object {
public:
    Number(int64_t value) : value_(value){};

    int64_t GetValue() const {
        return value_;
    }

    std::string ToString() override {
        return std::to_string(value_);
    }

    std::shared_ptr<Object> Eval(Scope* scope) override {
        return shared_from_this();
    }

protected:
    int64_t value_;
};

class Cell : public Object {
public:
    Cell(std::shared_ptr<Object> first, std::shared_ptr<Object> second)
        : first_(first), second_(second){};

    std::shared_ptr<Object> GetFirst() const {
        return first_;
    }
    void SetFirst(std::shared_ptr<Object> new_first) {
        first_ = new_first;
    }
    std::shared_ptr<Object> GetSecond() const {
        return second_;
    }
    void SetSecond(std::shared_ptr<Object> new_second) {
        second_ = new_second;
    }

    /// TODO так..?
    std::string ToString() override {
        if (second_ == nullptr) {
            return first_->ToString();
        }
        return '(' + first_->ToString() + ' ' + second_->ToString() + ')';
    }

    std::shared_ptr<Object> Eval(Scope* scope) override {
        //        if (second_ == nullptr) {
        //            return first_->Eval();
        //        }
        if (first_ == nullptr) {
            throw RuntimeError("first is nullptr in cell");
        }
        /// TODO проверить на fuzzer этот ужас
        if (second_ == nullptr && Is<Cell>(first_) &&    // NOLINT
            Is<Symbol>(As<Cell>(first_)->GetFirst()) &&  // NOLINT
            As<Symbol>(As<Cell>(first_)->GetFirst())->GetName() == "quote") {
            return first_->Eval(scope);
        }
        auto func = first_->Eval(scope);
        if (!Is<Function>(func)) {
            throw RuntimeError("not a function in eval");
        }
        return As<Function>(func)->Apply(second_, scope);
    }

protected:
    std::shared_ptr<Object> first_;
    std::shared_ptr<Object> second_;
};

/// отчаяние завело меня в специальный тип - гарантируется, что тут валидные символы тусят
class NumPair : public Object {
public:
    NumPair(std::shared_ptr<Object> first, std::shared_ptr<Object> second)
        : first_(first), second_(second){};

    NumPair(std::shared_ptr<Object> cell) {
        if (!Is<Cell>(cell)) {
            throw RuntimeError("Need cell in NumPair constructor");
        }
        first_ = As<Cell>(cell)->GetFirst();
        second_ = As<Cell>(cell)->GetSecond();
        if (!Is<Number>(first_) || !Is<Number>(second_)) {
            throw RuntimeError("NumPair arguments should be nums");
        }
    }
    /// TODO написать GetFirst, GetSecond
    std::shared_ptr<Object> Eval(Scope* scope) override {
        return shared_from_this();
    }

    std::string ToString() override {
        if (second_ == nullptr) {
            return first_->ToString();
        }
        return first_->ToString() + " . " + second_->ToString();
    }

protected:
    std::shared_ptr<Object> first_;
    std::shared_ptr<Object> second_;
};

#define MAKE_BASIC_FUNCTION(classname)                                                          \
    class classname : public Function {                                                         \
    public:                                                                                     \
        std::shared_ptr<Object> Apply(std::shared_ptr<Object> argument, Scope* scope) override; \
    };

MAKE_BASIC_FUNCTION(IsBool);
MAKE_BASIC_FUNCTION(Quote);
MAKE_BASIC_FUNCTION(IsNumber);
MAKE_BASIC_FUNCTION(Equals);
MAKE_BASIC_FUNCTION(Less);
MAKE_BASIC_FUNCTION(Greater);
MAKE_BASIC_FUNCTION(LEquals);
MAKE_BASIC_FUNCTION(GEquals);
MAKE_BASIC_FUNCTION(Add);
MAKE_BASIC_FUNCTION(Subtract);
MAKE_BASIC_FUNCTION(Multiply);
MAKE_BASIC_FUNCTION(Divide);
MAKE_BASIC_FUNCTION(Max);
MAKE_BASIC_FUNCTION(Min);
MAKE_BASIC_FUNCTION(Abs);
MAKE_BASIC_FUNCTION(Not);
MAKE_BASIC_FUNCTION(And);
MAKE_BASIC_FUNCTION(Or);
MAKE_BASIC_FUNCTION(IsPair);
MAKE_BASIC_FUNCTION(IsNull);
MAKE_BASIC_FUNCTION(IsList);
MAKE_BASIC_FUNCTION(Cons);
MAKE_BASIC_FUNCTION(Car);
MAKE_BASIC_FUNCTION(Cdr);
MAKE_BASIC_FUNCTION(MakeList);
MAKE_BASIC_FUNCTION(ListRef);
MAKE_BASIC_FUNCTION(ListTail);
MAKE_BASIC_FUNCTION(IsSymbol);
MAKE_BASIC_FUNCTION(Define);
MAKE_BASIC_FUNCTION(Set);

class Variable : public Function {
public:
    Variable(std::shared_ptr<Object> value) : value_(value){};

    std::shared_ptr<Object> Apply(std::shared_ptr<Object> argument, Scope* scope) override {
        return value_;
    }

    std::string ToString() override {
        return value_->ToString();
    }

protected:
    std::shared_ptr<Object> value_;
};

static std::unordered_map<std::string, std::shared_ptr<Function>> name_to_function = {
    {"boolean?", std::make_shared<IsBool>()},
    {"quote", std::make_shared<Quote>()},
    {"number?", std::make_shared<IsNumber>()},
    {"=", std::make_shared<Equals>()},
    {"<", std::make_shared<Less>()},
    {">", std::make_shared<Greater>()},
    {"<=", std::make_shared<LEquals>()},
    {">=", std::make_shared<GEquals>()},
    {"+", std::make_shared<Add>()},
    {"-", std::make_shared<Subtract>()},
    {"*", std::make_shared<Multiply>()},
    {"/", std::make_shared<Divide>()},
    {"max", std::make_shared<Max>()},
    {"min", std::make_shared<Min>()},
    {"abs", std::make_shared<Abs>()},
    {"not", std::make_shared<Not>()},
    {"and", std::make_shared<And>()},
    {"or", std::make_shared<Or>()},
    {"pair?", std::make_shared<IsPair>()},
    {"null?", std::make_shared<IsNull>()},
    {"list?", std::make_shared<IsList>()},
    {"cons", std::make_shared<Cons>()},
    {"car", std::make_shared<Car>()},
    {"cdr", std::make_shared<Cdr>()},
    {"list", std::make_shared<MakeList>()},
    {"list-ref", std::make_shared<ListRef>()},
    {"list-tail", std::make_shared<ListTail>()},
    {"symbol?", std::make_shared<IsSymbol>()},
    {"define", std::make_shared<Define>()},
    {"set!", std::make_shared<Set>()}};
