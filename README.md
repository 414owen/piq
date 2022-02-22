![example workflow](https://github.com/414owen/lang-c/actions/workflows/ci.yml/badge.svg) [![codecov](https://codecov.io/gh/414owen/lang-c/branch/master/graph/badge.svg?token=81Y2QKFFF6)](https://codecov.io/gh/414owen/lang-c)


# Goal

A language that does everything right

* Fast compile times
* Low memory footprint
  * Have you every compiled a large template-heavy C++/D project?
    It's horrible. You need 128gb of ram just for it to terminate.
* Fast runtimes
* Portable compiler
* Compiler doesn't stackoverflow
  * On my machine, parsing nested additions (`1 + (1 + (...`), 
    fails on every compiler I've tried.
    | Compiler | Version    | Max depth |
    | -------- | ---------- | --------- |
    | clang    | 13.0.0     | 1701      |
    | gcc      | 10.3.0     | 33268     |
    | tcc      | 2021-10-09 | 253       |
    | zig      |            | 1700      |
* Strongly typed
* Correct abstractions
  * Sum types
  * Traits / typeclasses / interfaces
* Consistent syntax
* Beautiful (at least with rainbow parens)
* Explicit
  * Source code resembles a tree
    * Easier for machines and humans to parse
  * Types are specified
    * They're documentation
* Memory
  * Explicit allocation API
    * Arenas / manual / gc


# Measures

* Compiler:
  * Written in C
  * Struct-Of-Arrays used over Array-Of-Structs in key places
    * Representing ASTs / Types.
  * Equal types are only represented once
    * TODO should do same with AST?
  * CI enforces pleasing valgrind
  * Bunch of unit tests
  * No C extensions
    * Current exceptions (will replace eventually):
      * `memmem`
  * Doesn't use recursion
    * Parsing: `LR(1)` grammar, state transition table compiled by `lemon`
    * Everything else recursive:
      * Maintain a heap-allocated stack of actions
* Language
  * Lisp-like syntax
  * Most forms are decideable based on their opening token
  * TODO...



# Ideas

* ~Search index arrays instead of adding to them?~
* make ss_init return struct, with a char**, rather than a struct
* Get rid of PT_TOP_LEVEL, it's just more indirection
