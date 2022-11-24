#include "scheme.h"

#include <vector>

#include "algorithm"
#include "object.h"
#include "parser.h"
#include "sstream"
#include "tokenizer.h"

std::string Interpreter::Run(const std::string& line) {
    std::stringstream stream{line};
    Tokenizer tokenizer{&stream};

    auto input_ast = ReadAll(&tokenizer);

    //    auto function = input_ast->GetFirst();
    //
    //    auto args = ConvertAndEvalArgsToVector(input_ast->GetSecond());

    if (input_ast == nullptr) {
        throw RuntimeError("input_ast is nullptr");
    }
    auto output_ast = input_ast->Eval(&global_scope_);

    return output_ast->ToString();
}

std::optional<std::shared_ptr<Function>> GetFunction(const std::string& name) {
    auto it = name_to_function.find(name);
    if (it == name_to_function.end()) {
        return std::nullopt;
    }
    return name_to_function.at(name);  /// TODO два раза ищу по массиву, не знаю, как лечить
}

std::shared_ptr<Object> IsNumber::Apply(std::shared_ptr<Object> argument, Scope* scope) {
    if (Is<Cell>(argument)) {
        if (As<Cell>(argument)->GetSecond() != nullptr) {
            throw SyntaxError("wrong number parameter in cell");
        }
        return std::make_shared<Bool>(Is<Number>(As<Cell>(argument)->GetFirst()));
    }
    return std::make_shared<Bool>(Is<Number>(argument));
}

void CheckCellIsValid(std::shared_ptr<Object> cell) {
    if (!Is<Cell>(cell)) {
        return;
    }
    if (As<Cell>(cell)->GetFirst() == nullptr) {
        throw RuntimeError("CheckCellIsValid error");
    }
}

void ConvertAndEvalArgument(std::shared_ptr<Object> argument,
                            std::vector<std::shared_ptr<Object>>* arg_vector_ptr, Scope* scope) {
    if (argument == nullptr) {
        return;
    }
    if (!Is<Cell>(argument)) {
        arg_vector_ptr->push_back(argument);  /// TODO Надо ли тут Eval? попробовать без
        return;
    }
    CheckCellIsValid(argument);
    if (Is<Function>(As<Cell>(argument)->GetFirst()->Eval(scope))) {
        arg_vector_ptr->push_back(argument->Eval(scope));
        return;
    }
    ConvertAndEvalArgument(As<Cell>(argument)->GetFirst(), arg_vector_ptr, scope);
    ConvertAndEvalArgument(As<Cell>(argument)->GetSecond(), arg_vector_ptr, scope);
}

std::vector<std::shared_ptr<Object>> ConvertAndEvalArgsToVector(std::shared_ptr<Object> argument,
                                                                Scope* scope) {
    std::vector<std::shared_ptr<Object>> converted_args_vector;
    ConvertAndEvalArgument(argument, &converted_args_vector, scope);
    return converted_args_vector;
}

/// TODO кажется это можно переписать через if constexpr или чота такое
#define CheckArgVectorElementTypes(TargetType)                          \
    for (const auto& current_arg : arg_vector) {                        \
        if (!Is<TargetType>(current_arg)) {                             \
            throw RuntimeError("wrong element types in one operation"); \
        }                                                               \
    }

std::shared_ptr<Object> Equals::Apply(std::shared_ptr<Object> argument, Scope* scope) {
    auto arg_vector = ConvertAndEvalArgsToVector(argument, scope);
    CheckArgVectorElementTypes(Number);
    for (size_t i = 1; i < arg_vector.size(); ++i) {
        if (As<Number>(arg_vector[i - 1])->GetValue() != As<Number>(arg_vector[i])->GetValue()) {
            return std::make_shared<Bool>(false);
        }
    }
    return std::make_shared<Bool>(true);
}

std::shared_ptr<Object> Less::Apply(std::shared_ptr<Object> argument, Scope* scope) {
    auto arg_vector = ConvertAndEvalArgsToVector(argument, scope);
    CheckArgVectorElementTypes(Number);
    for (size_t i = 1; i < arg_vector.size(); ++i) {
        if (As<Number>(arg_vector[i - 1])->GetValue() >= As<Number>(arg_vector[i])->GetValue()) {
            return std::make_shared<Bool>(false);
        }
    }
    return std::make_shared<Bool>(true);
}

std::shared_ptr<Object> Greater::Apply(std::shared_ptr<Object> argument, Scope* scope) {
    auto arg_vector = ConvertAndEvalArgsToVector(argument, scope);
    CheckArgVectorElementTypes(Number);
    for (size_t i = 1; i < arg_vector.size(); ++i) {
        if (As<Number>(arg_vector[i - 1])->GetValue() <= As<Number>(arg_vector[i])->GetValue()) {
            return std::make_shared<Bool>(false);
        }
    }
    return std::make_shared<Bool>(true);
}

std::shared_ptr<Object> LEquals::Apply(std::shared_ptr<Object> argument, Scope* scope) {
    auto arg_vector = ConvertAndEvalArgsToVector(argument, scope);
    CheckArgVectorElementTypes(Number);
    for (size_t i = 1; i < arg_vector.size(); ++i) {
        if (As<Number>(arg_vector[i - 1])->GetValue() > As<Number>(arg_vector[i])->GetValue()) {
            return std::make_shared<Bool>(false);
        }
    }
    return std::make_shared<Bool>(true);
}

std::shared_ptr<Object> GEquals::Apply(std::shared_ptr<Object> argument, Scope* scope) {
    auto arg_vector = ConvertAndEvalArgsToVector(argument, scope);
    CheckArgVectorElementTypes(Number);
    for (size_t i = 1; i < arg_vector.size(); ++i) {
        if (As<Number>(arg_vector[i - 1])->GetValue() < As<Number>(arg_vector[i])->GetValue()) {
            return std::make_shared<Bool>(false);
        }
    }
    return std::make_shared<Bool>(true);
}

std::shared_ptr<Object> Add::Apply(std::shared_ptr<Object> argument, Scope* scope) {
    auto arg_vector = ConvertAndEvalArgsToVector(argument, scope);
    CheckArgVectorElementTypes(Number);
    int64_t operation_result = 0;
    for (auto arg : arg_vector) {
        operation_result += As<Number>(arg)->GetValue();
    }
    return std::make_shared<Number>(operation_result);
}

std::shared_ptr<Object> Subtract::Apply(std::shared_ptr<Object> argument, Scope* scope) {
    auto arg_vector = ConvertAndEvalArgsToVector(argument, scope);
    CheckArgVectorElementTypes(Number);
    int64_t operation_result;
    bool reached_first_num = false;
    for (auto arg : arg_vector) {
        if (!reached_first_num) {
            operation_result = As<Number>(arg)->GetValue();
            reached_first_num = true;
            continue;
        }
        operation_result -= As<Number>(arg)->GetValue();
    }
    if (!reached_first_num) {
        throw RuntimeError("need argument in subtract");
    }
    return std::make_shared<Number>(operation_result);
}

std::shared_ptr<Object> Multiply::Apply(std::shared_ptr<Object> argument, Scope* scope) {
    auto arg_vector = ConvertAndEvalArgsToVector(argument, scope);
    CheckArgVectorElementTypes(Number);
    int64_t operation_result = 1;
    for (auto arg : arg_vector) {
        if (!Is<Number>(arg)) {
            throw RuntimeError("wrong element types in one operation (Add)");
        }
        operation_result *= As<Number>(arg)->GetValue();
    }
    return std::make_shared<Number>(operation_result);
}

std::shared_ptr<Object> Divide::Apply(std::shared_ptr<Object> argument, Scope* scope) {
    auto arg_vector = ConvertAndEvalArgsToVector(argument, scope);
    CheckArgVectorElementTypes(Number);
    int64_t operation_result;
    bool reached_first_num = false;
    for (auto arg : arg_vector) {
        if (!reached_first_num) {
            operation_result = As<Number>(arg)->GetValue();
            reached_first_num = true;
            continue;
        }
        operation_result /= As<Number>(arg)->GetValue();
    }
    if (!reached_first_num) {
        throw RuntimeError("need argument in divide");
    }
    return std::make_shared<Number>(operation_result);
}

std::shared_ptr<Object> Max::Apply(std::shared_ptr<Object> argument, Scope* scope) {
    auto arg_vector = ConvertAndEvalArgsToVector(argument, scope);
    CheckArgVectorElementTypes(Number);
    int64_t operation_result;
    bool reached_first_num = false;
    for (auto arg : arg_vector) {
        if (!reached_first_num) {
            reached_first_num = true;
            operation_result = As<Number>(arg)->GetValue();
            continue;
        }
        operation_result = std::max(operation_result, As<Number>(arg)->GetValue());
    }
    if (!reached_first_num) {
        throw RuntimeError("need argument in max");
    }
    return std::make_shared<Number>(operation_result);
}

std::shared_ptr<Object> Min::Apply(std::shared_ptr<Object> argument, Scope* scope) {
    auto arg_vector = ConvertAndEvalArgsToVector(argument, scope);
    CheckArgVectorElementTypes(Number);
    int64_t operation_result;
    bool reached_first_num = false;
    for (auto arg : arg_vector) {
        if (!reached_first_num) {
            reached_first_num = true;
            operation_result = As<Number>(arg)->GetValue();
            continue;
        }
        operation_result = std::min(operation_result, As<Number>(arg)->GetValue());
    }
    if (!reached_first_num) {
        throw RuntimeError("need argument in min");
    }
    return std::make_shared<Number>(operation_result);
}

std::shared_ptr<Object> Abs::Apply(std::shared_ptr<Object> argument, Scope* scope) {
    auto arg_vector = ConvertAndEvalArgsToVector(argument, scope);
    CheckArgVectorElementTypes(Number);
    if (arg_vector.size() > 1 || arg_vector.empty()) {
        throw RuntimeError("incorrect argument number in abs");
    }
    return std::make_shared<Number>(std::abs(As<Number>(arg_vector[0])->GetValue()));
}

bool CellIsNumPair(std::shared_ptr<Object> argument) {
    std::shared_ptr<Object> first = As<Cell>(argument)->GetFirst();
    std::shared_ptr<Object> second = As<Cell>(argument)->GetSecond();
    return Is<Number>(first) && Is<Number>(second) && first != nullptr && second != nullptr;
}

void ConvertArgument(std::shared_ptr<Object> argument,
                     std::vector<std::shared_ptr<Object>>* arg_vector_ptr) {
    if (argument == nullptr) {
        return;
    }
    if (!Is<Cell>(argument)) {
        arg_vector_ptr->push_back(argument);
        return;
    }

    if (CellIsNumPair(argument)) {
        arg_vector_ptr->push_back(std::make_shared<NumPair>(As<Cell>(argument)));
        return;
    }
    ConvertArgument(As<Cell>(argument)->GetFirst(), arg_vector_ptr);
    ConvertArgument(As<Cell>(argument)->GetSecond(), arg_vector_ptr);
}

std::vector<std::shared_ptr<Object>> ConvertArgsToVector(std::shared_ptr<Object> argument) {
    std::vector<std::shared_ptr<Object>> converted_args_vector;
    ConvertArgument(argument, &converted_args_vector);
    return converted_args_vector;
}

std::shared_ptr<Object> Quote::Apply(std::shared_ptr<Object> argument, Scope* scope) {
    if (argument == nullptr) {
        return std::make_shared<Symbol>("()");
    }
    if (!Is<Cell>(argument)) {
        return std::make_shared<Symbol>(argument->ToString());
    }

    auto arg_vector = ConvertArgsToVector(argument);
    std::string operation_result_str = "(";
    /// TODO сомнительная часть кода
    try {
        CheckCellIsValid(As<Cell>(argument));
    } catch (RuntimeError) {
        operation_result_str += "()";
    } catch (...) {
        throw RuntimeError("undefined error in quote");
    }
    /// сомнительная часть окончена
    for (size_t i = 0; i < arg_vector.size(); ++i) {
        if (Is<Cell>(arg_vector[i])) {
            operation_result_str += Quote::Apply(arg_vector[i], scope)->ToString();
        } else {
            operation_result_str += arg_vector[i]->ToString();
            if (i != arg_vector.size() - 1) {
                operation_result_str += ' ';
            }
        }
    }
    operation_result_str += ')';
    return std::make_shared<Symbol>(operation_result_str);
}

std::shared_ptr<Object> IsBool::Apply(std::shared_ptr<Object> argument, Scope* scope) {
    auto arg_vector = ConvertAndEvalArgsToVector(argument, scope);
    if (arg_vector.size() > 1 || arg_vector.empty()) {
        throw RuntimeError("incorrect argument number in IsBool");
    }
    return std::make_shared<Bool>(Is<Bool>(arg_vector[0]));
}

bool GetBoolValueFromAnyType(std::shared_ptr<Object> argument) {
    if (Is<Bool>(argument)) {
        return As<Bool>(argument)->GetBoolValue();
    }
    return true;
}

std::shared_ptr<Object> Not::Apply(std::shared_ptr<Object> argument, Scope* scope) {
    auto arg_vector = ConvertAndEvalArgsToVector(argument, scope);
    if (arg_vector.size() > 1 || arg_vector.empty()) {
        throw RuntimeError("incorrect argument number in Not");
    }
    return std::make_shared<Bool>(!GetBoolValueFromAnyType(arg_vector[0]));
}

void ConvertCompactArgument(std::shared_ptr<Object> argument,
                            std::vector<std::shared_ptr<Object>>* arg_vector_ptr) {

    if (!Is<Cell>(argument)) {
        arg_vector_ptr->push_back(argument);
        return;
    }
    auto first = As<Cell>(argument)->GetFirst();
    arg_vector_ptr->push_back(first);
    if (As<Cell>(argument)->GetSecond() == nullptr) {
        return;
    }
    ConvertCompactArgument(As<Cell>(argument)->GetSecond(), arg_vector_ptr);
}

std::vector<std::shared_ptr<Object>> ConvertCompactArgsToVector(std::shared_ptr<Object> argument) {
    std::vector<std::shared_ptr<Object>> arg_vector;
    ConvertCompactArgument(argument, &arg_vector);
    return arg_vector;
}

std::shared_ptr<Object> And::Apply(std::shared_ptr<Object> argument, Scope* scope) {
    auto arg_vector = ConvertCompactArgsToVector(argument);
    if (arg_vector.size() <= 1) {
        return std::make_shared<Bool>(true);
    }
    for (auto& arg : arg_vector) {
        arg = arg->Eval(scope);
        if (!GetBoolValueFromAnyType(arg)) {
            return arg;
        }
    }
    return arg_vector.back();
}

std::shared_ptr<Object> Or::Apply(std::shared_ptr<Object> argument, Scope* scope) {
    auto arg_vector = ConvertCompactArgsToVector(argument);
    if (arg_vector.size() <= 1) {
        return std::make_shared<Bool>(false);
    }
    for (auto& arg : arg_vector) {
        arg = arg->Eval(scope);
        if (GetBoolValueFromAnyType(arg)) {
            return arg;
        }
    }
    return arg_vector.back();
}

std::shared_ptr<Object> UnpackLine(std::shared_ptr<Object> argument) {
    if (!Is<Symbol>(argument)) {
        throw RuntimeError("cannot unpack not a symbol");
    }
    std::stringstream stream{As<Symbol>(argument)->ToString()};
    Tokenizer tokenizer{&stream};
    return ReadAll(&tokenizer);
}

/// договорились, что на вход на приходит Cell
std::shared_ptr<Object> IsPair::Apply(std::shared_ptr<Object> argument, Scope* scope) {
    auto packed_arg_vector = ConvertAndEvalArgsToVector(argument, scope);
    std::shared_ptr<Object> unpacked_argument = UnpackLine(packed_arg_vector[0]);
    auto unpacked_arg_vector = ConvertAndEvalArgsToVector(unpacked_argument, scope);
    if (unpacked_arg_vector.size() == 2) {
        for (auto arg : unpacked_arg_vector) {
            if (!Is<Number>(arg)) {
                return std::make_shared<Bool>(false);
            }
        }
        return std::make_shared<Bool>(true);
    }
    return std::make_shared<Bool>(false);
}

std::shared_ptr<Object> IsNull::Apply(std::shared_ptr<Object> argument, Scope* scope) {
    auto packed_arg_vector = ConvertAndEvalArgsToVector(argument, scope);
    std::shared_ptr<Object> unpacked_argument = UnpackLine(packed_arg_vector[0]);
    auto unpacked_arg_vector = ConvertAndEvalArgsToVector(unpacked_argument, scope);
    return std::make_shared<Bool>(unpacked_arg_vector.empty());
}

/// TODO перепиши
std::shared_ptr<Object> IsList::Apply(std::shared_ptr<Object> argument, Scope* scope) {
    auto packed_arg_vector = ConvertAndEvalArgsToVector(argument, scope);
    std::shared_ptr<Object> unpacked_argument = UnpackLine(packed_arg_vector[0]);
    if (unpacked_argument == nullptr) {
        return std::make_shared<Bool>(true);
    }
    int counter = 0;
    while (counter <= 100) {
        ++counter;
        if (!Is<Cell>(unpacked_argument)) {
            return std::make_shared<Bool>(false);
        }
        if (As<Cell>(unpacked_argument)->GetSecond() == nullptr) {
            return std::make_shared<Bool>(true);
        }
        unpacked_argument = As<Cell>(unpacked_argument)->GetSecond();
    }
    throw RuntimeError("Undefined IsList error");
}

std::shared_ptr<Object> Cons::Apply(std::shared_ptr<Object> argument, Scope* scope) {
    auto arg_vector = ConvertAndEvalArgsToVector(argument, scope);
    if (arg_vector.size() != 2) {
        throw RuntimeError("Undefined cons behaviour");
    }
    return std::make_shared<Symbol>('(' + arg_vector[0]->ToString() + " . " +
                                    arg_vector[1]->ToString() + ')');
}

std::shared_ptr<Object> Car::Apply(std::shared_ptr<Object> argument, Scope* scope) {
    auto packed_arg_vector = ConvertAndEvalArgsToVector(argument, scope);
    std::shared_ptr<Object> unpacked_argument = UnpackLine(packed_arg_vector[0]);
    if (unpacked_argument == nullptr) {
        throw RuntimeError("car to empty cell");
    }
    return std::make_shared<Cell>(std::make_shared<Symbol>("quote"),
                                  As<Cell>(unpacked_argument)->GetFirst())
        ->Eval(scope);
}

std::shared_ptr<Object> Cdr::Apply(std::shared_ptr<Object> argument, Scope* scope) {
    auto packed_arg_vector = ConvertAndEvalArgsToVector(argument, scope);
    std::shared_ptr<Object> unpacked_argument = UnpackLine(packed_arg_vector[0]);
    if (unpacked_argument == nullptr) {
        throw RuntimeError("cdr to empty cell");
    }
    if (As<Cell>(unpacked_argument)->GetSecond() == nullptr) {
        return std::make_shared<Symbol>("()");
    } else {
        return std::make_shared<Cell>(std::make_shared<Symbol>("quote"),
                                      As<Cell>(unpacked_argument)->GetSecond())
            ->Eval(scope);
    }
}

std::shared_ptr<Object> MakeCellFromVector(std::vector<std::shared_ptr<Object>>& arg_vec,
                                           size_t start = 0) {
    if (arg_vec.empty()) {
        return nullptr;
    }
    std::shared_ptr<Cell> cell = std::make_shared<Cell>(nullptr, nullptr);
    std::shared_ptr<Object> current_cell_ptr = cell;
    for (size_t i = start; i < arg_vec.size(); ++i) {
        As<Cell>(current_cell_ptr)->SetFirst(arg_vec[i]);
        if (i == arg_vec.size() - 1) {
            break;
        }
        As<Cell>(current_cell_ptr)->SetSecond(std::make_shared<Cell>(nullptr, nullptr));
        current_cell_ptr = As<Cell>(current_cell_ptr)->GetSecond();
    }
    return cell;
}

std::shared_ptr<Object> MakeList::Apply(std::shared_ptr<Object> argument, Scope* scope) {
    auto arg_vector = ConvertAndEvalArgsToVector(argument, scope);
    auto potential_cell = MakeCellFromVector(arg_vector);
    if (potential_cell == nullptr) {
        return std::make_shared<Symbol>("()");
    } else {
        return std::make_shared<Cell>(std::make_shared<Symbol>("quote"), potential_cell)
            ->Eval(scope);
    }
}

std::shared_ptr<Object> ListRef::Apply(std::shared_ptr<Object> argument, Scope* scope) {
    auto packed_arg_vector = ConvertAndEvalArgsToVector(argument, scope);
    if (packed_arg_vector.size() <= 1) {
        throw RuntimeError("undefined ListRef error");
    }
    std::shared_ptr<Object> unpacked_argument = UnpackLine(packed_arg_vector[0]);
    auto unpacked_arg_vector = ConvertAndEvalArgsToVector(unpacked_argument, scope);
    size_t position = As<Number>(packed_arg_vector[1])->GetValue();
    if (position >= unpacked_arg_vector.size()) {
        throw RuntimeError("position is bigger than arg_vector size in ListRef");
    }
    return unpacked_arg_vector[position];
}

std::shared_ptr<Object> ListTail::Apply(std::shared_ptr<Object> argument, Scope* scope) {
    auto packed_arg_vector = ConvertAndEvalArgsToVector(argument, scope);
    if (packed_arg_vector.size() <= 1) {
        throw RuntimeError("undefined ListRef error");
    }
    std::shared_ptr<Object> unpacked_argument = UnpackLine(packed_arg_vector[0]);
    auto unpacked_arg_vector = ConvertAndEvalArgsToVector(unpacked_argument, scope);
    size_t position = As<Number>(packed_arg_vector[1])->GetValue();
    if (position == unpacked_arg_vector.size()) {
        return std::make_shared<Symbol>("()");
    }
    if (position > unpacked_arg_vector.size()) {
        throw RuntimeError("position is bigger than arg_vector size in ListTail");
    }
    auto tail = MakeCellFromVector(unpacked_arg_vector, position);
    return std::make_shared<Cell>(std::make_shared<Symbol>("quote"), tail)->Eval(scope);
}

std::shared_ptr<Object> IsSymbol::Apply(std::shared_ptr<Object> argument, Scope* scope) {
    auto arg_vector = ConvertAndEvalArgsToVector(argument, scope);
    if (arg_vector.size() != 1) {
        throw RuntimeError("undefined symbol? error");
    }
    return std::make_shared<Bool>(Is<Symbol>(arg_vector[0]));
}

std::shared_ptr<Object> Define::Apply(std::shared_ptr<Object> argument, Scope* scope) {
    if (argument == nullptr) {
        throw SyntaxError("argument is nullptr in define");
    }
    auto cell = As<Cell>(argument);
    if (!Is<Symbol>(cell->GetFirst())) {
        throw SyntaxError("define error");
    }
    std::string variable_name = As<Symbol>(cell->GetFirst())->GetName();
    auto arg_vector = ConvertAndEvalArgsToVector(cell->GetSecond(), scope);
    if (arg_vector.size() != 1) {
        throw SyntaxError("define arg error");
    }
    auto obj = std::make_shared<Variable>(arg_vector[0]);
    name_to_function[variable_name] = obj;
    return std::make_shared<Symbol>("defined!");
}

std::shared_ptr<Object> Set::Apply(std::shared_ptr<Object> argument, Scope* scope) {
    if (argument == nullptr) {
        throw SyntaxError("argument is nullptr in set");
    }
    auto cell = As<Cell>(argument);
    if (!Is<Symbol>(cell->GetFirst())) {
        throw SyntaxError("define error");
    }
    auto arg_vector = ConvertAndEvalArgsToVector(cell->GetSecond(), scope);
    if (arg_vector.size() != 1) {
        throw SyntaxError("define arg error");
    }
    std::string variable_name = As<Symbol>(cell->GetFirst())->GetName();
    if (!name_to_function.contains(variable_name)) {
        throw NameError("invalid symbol to set");
    }
    auto obj = std::make_shared<Variable>(arg_vector[0]);
    name_to_function[variable_name] = obj;
    return std::make_shared<Symbol>("set!");
}