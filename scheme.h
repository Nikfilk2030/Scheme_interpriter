#pragma once

#include <string>
#include "object.h"

class Interpreter {
public:
    std::string Run(const std::string&);

protected:
    Scope global_scope_;
};
