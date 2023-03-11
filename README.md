[![workflow](https://github.com/414owen/lang-c/actions/workflows/ci.yml/badge.svg)](https://github.com/414owen/lang-c/actions)

# Possible names

* YAMLLL - Yet Another Modern Low Level Language
* FAFL - Fast and friendly language

attributes:

* fast
* homoiconic
* bootstrappable
* minimal

# Goal

A language that does everything right

* Fast compiler compile times
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
* ~make ss_init return struct, with a char**, rather than a struct~, did stack-allocated stringstream
* ~Get rid of PT_TOP_LEVEL, it's just more indirection~
* ~Higher order tuples are syntactic sugar over 2-tuples, so combinators work~
* Generics: Generics introduce a block, where everything in the block
  is generic. Think it'll look clean.
* Use mermaid in doc strings to produce diagrams
* Add tcc to CI
* Don't have a cabal/cargo/pip/whatever style thing to declare dependencies,
  have a universal namespace for packages, and when one is used in source,
  its dependency is implicit.
* Instrument stage timings
* Add [gcc function attributes](https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html)
* Profile guided optimization
* Replace FUN_BODY parse node with BLOCK
* Replace all temporary allocated vec_node_ind with a single one, and a vector of lengths
* Unify wildcard and placeholder terminology
* Add all EXPR parse tree enums to STMT, as all expressions are valid statements.
* Optimize some units (eg arg parsing) for size, others for speed
* Maybe all product types should be records?
* We know the max-depth of the tree after parsing, so all other phases should use fixed size stacks
* There's something good about C's tagged unions. You can slice and dice the enum, and union them.
  (see types.h, parse_tree.h), that lets you support more cases in certain program stages.
  For example, when type checking, I have a T_VAR type, but when I'm doing codegen, I need to
  know what types I have, so my enum doesn't include that.
* Wrap index types in structs to protect us from bad changes
* Precalculate stack depth in binding resolution or its own step, then use arrays instead of vectors
  in other traversals

## Thoughts on pointers

Pointers are sometimes too memory-intensive, it's rare that any given part of your
one's program needs to work with 2^64 bytes of memory. If you are actually working
with >2^32 bytes of memory, you should probably consider dividing the workload
and multithreading your program to work on smaller chunks.

## Relative pointers

Jai has relative pointers, which allow the use of 32-bit pointers in
the same programs as 64-bit pointers. They're also serializable.

### Typed indices

Pointer types are conceptually a (haskell) newtype around an existing type (size_t),
we could do the same to indices, so that they can only index arrays of a certain
type...

There can be a builtin index type that's generic, and can access any type of array,
`IxAny :: Type`, and there can be another builtin type constructor
`Ix :: Type -> Type`, which is type-safe.

This would bring indices up to safety-par with pointers.

### Indices bound to a specific array

Another compile-time check is that we could associate an index with a *specific*
array. If I have an index into `arr1` I want to make sure it can't be used
on `arr2`. This type of index wouldn't need to store the element type (as
the one above would). I'm not sure what type-level feature this maps onto.
It's not a dependent type, because there isn't a term in the type, the
check is that a value derived from a thing can only be used on that thing.
I guess it has to be called a `provenance type`, which sounds cool.

Questions:

* If I rebind `arr1` as `arr2`, can I still use the index derived from `arr1`?
* If I make an index into `arr1`, do I want to be able to use it to index a
  slice derived from `arr1`?

Example:

```
(let arr1 [1, 2, 3])
(let arr2 [1, 2, 3])

(sig i (Ix arr1))
(let i 2)
(ix arr1 i)              // 3
(ix arr2 i)              // provenance error
```

Taking this to its logical conclusion, are there any other values for which
we want to check that their usage is connected to another value?

How is this modeled? It seems like every (expression?/binding?) should
conceptually create a singleton type, which can initially only be used by
builtins, but can eventually be exposed to the user?

# Mistakes

* Having sigs as separate syntactic constructs makes the compiler
  unnecessarily complicated. We should just encode the relationship
  in the grammar, with a `sigfun` node or something...

# Building

```
$ nix build
$ ./result/bin/lang
```

# Developing

```
$ nix develop
$ tup dbconfig   # generate compile_commands.json so the language server works
$ tup
$ ./repl
```

# Without nix

## Requirements

* A C/C++ compiler

### Headers

* [hedley](https://github.com/nemequ/hedley)
* [predef](https://github.com/natefoo/predef)

### Libraries

* [xxHash](https://cyan4973.github.io/xxHash)
* [libllvm](https://llvm.org/)

# Inspiration taken from

* [Haskell](https://www.haskell.org/)
* [C](https://fr.wikipedia.org/wiki/C_(langage))

# Cool features other languages have

## [Rust](https://www.rust-lang.org/fr)

Disjunctive patterns?

## [Jai](https://inductive.no/jai/)

Relative pointers?

## [OCaml](https://ocaml.org/)

https://v2.ocaml.org/api/Ephemeron.html
I'll probably use something similar to OCaml's compilation model for generics


## [hare](https://harelang.org/)

Looking here: https://git.sr.ht/~sircmpwn/hare

Directory structure is used to define platform-dependent modules,
although currently `diff unix/+linux/nice.ha unix/+freebsd/nice.ha` is empty.

## [Zig](https://ziglang.org/)

Allocation API, everything that needs dynamic memory takes an allocator.

## [Austral](https://austral-lang.org/)

[introduction blog post](https://borretti.me/article/introducing-austral)

This language looks very close to what I want.
I'm not sure what it has against @annotations though. I was considering adding them to this language.
Linearity via kinds looks great!

I think that type inference is good to have, but there should be a mode
that tells the compiler to error if (top-level) annotations are left out.

# Language watchlist

I'll probably get around to stealing feature ideas from these at some point.

* [C3](https://c3-lang.org/)
* [garnet](https://hg.sr.ht/~icefox/garnet)
* [go](https://go.dev/)
  TODO check out their compilation model for generics, I've heard it doesn't
  specialize but gets okay performance?

# Type inference resources

https://www.youtube.com/watch?v=il3gD7XMdmA (also goes through typing judgement notation)

Wand's algorithm: https://www.win.tue.nl/~hzantema/semssm.pdf

# Performance

To lookup:

* nallocx
* xallocx
* sdallocx
* malloc_info
* valgrind dhat
* VTune
* opt-viewer

# Review of https://buttondown.email/hillelwayne/archive/microfeatures-id-like-to-see-in-more-languages/

* Balanced string literals
  * Use french quotes «because balanced is better»?
  * Or maybe something recognizable, but balanced, like <"this">?
* Main arguments are command line arguments: https://lobste.rs/s/ilmbgu/microfeatures_i_d_like_see_more_languages#c_l56bvw

# C container libraries

* https://rurban.github.io/ctl/
* https://github.com/ludocode/pottery
* https://github.com/matrixjoeq/c_container
* https://github.com/cher-nov/Gena
* https://github.com/springkim/OpenCSTL
* https://github.com/LeoVen/C-Macro-Collections
* https://github.com/nothings/stb

# Flags

## Debug

```
-std=c99 -pedantic -O0 -ggdb -Wall -Wextra -DDEBUG -DTIME_ALL
```

## Release

```
-std=c99 -pedantic -O2 -s -flto -Wall -Wextra -DNDEBUG
```
