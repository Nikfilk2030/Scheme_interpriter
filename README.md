# Scheme
- [Scheme](#scheme)
  - [Выполнение выражений](#выполнение-выражений)
    - [Пример](#пример)
  - [Дополнительные материалы](#дополнительные-материалы)

Язык будет состоять из:
 - Примитивных типов: целых чисел, bool-ов и _символов_ (идентификаторов).
 - Составных типов: пар и списков.
 - Переменных с синтаксической областью видимости.
 - Функций и лямбда-выражений.

```
    1 => 1
    (+ 1 2) => 3
```
Обозначение `=>` в примерах здесь и далее разделяет выражение и результат его выполнения.

## Выполнение выражений
Выполнение языка происходит в 3 этапа:

**Токенизация** - преобразует текст программы в последовательность атомарных лексем. 

**Синтаксический анализ** - преобразует последовательность токенов в [AST](https://en.wikipedia.org/wiki/Abstract_syntax_tree).  AST в LISP-подобных языках программирования представляется в виде списков. 
   
**Вычисление** - рекурсивно обходит AST программы и преобразует его в соответствии с набором правил.

### Пример

Выражение 
```
    (+ 2 (/ -3 +4))
``` 
в результате токенизации превратится в список токенов:
```
    { 
        OpenParen(),
        Symbol("+"),
        Number(2),
        OpenParen(),
        Symbol("/"),
        Number(-3),
        Number(4),
        CloseParen(),
        CloseParen()
    }
```
     
 Последовательность токенов в результате синтаксического анализа
 превратится в дерево:
     
```
    Cell{
        Symbol("+"),
        Cell{
            Number(2),
            Cell{
                Cell{
                    Symbol("/"),
                    Cell{
                        Number(-3),
                        Cell{
                            Number(4),
                            nullptr
                        }
                    }
                }
                nullptr
            }
        }
    }
```
Результатом же выполнения выражения будет 

```
    (+ 2 (/ -3 +4)) => 1
```

## Дополнительные материалы

* Видеоурок [введение в scheme](https://www.youtube.com/watch?v=AqBxU-Zmx00) объяснит базовые конструкции языка.

* Книга [Build Your Own Lisp](http://www.buildyourownlisp.com/) разбирает детали реализации интерпретатора на языке C.

* Книга [Crafting Interpreters](http://craftinginterpreters.com/) разбирает реализацию интерпретатора для более сложного языка, чем LISP.
