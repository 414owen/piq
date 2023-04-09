# piq

![ci](https://github.com/414owen/piq/actions/workflows/tests.yml/badge.svg)

<img alt="logo" src="https://user-images.githubusercontent.com/1714287/230741705-009d4511-9423-426f-b31b-f7254fc38884.svg" height="100">

piq (/piːk/) is a fast, minimal, statically typed, easily bootstrappable,
compiled programming language. It is written `piq`, with a lowercase `p`,
because this has a nice symmetry. If it doesn't look symmetrical to you,
consider changing your font.

## Fast

The language is fast. It's close to the metal, there's no indirection.

The compiler is fast. The single-threaded performance is currently at about
170_000 sloc/s on my laptop.[^1]

The compiler itself is fast to compile.[^2]

## Minimal

The piq syntax is a direct representation of the AST, where all compound
constructs have an introducer keyword, and are parenthesized. For example `(if
cond a b)` for ternary expressions, and `(fn () 3)` for lambdas.[^3] This is
easy to parse, for humans and computers, without backtracking.

## Statically typed

All piq types are known at compile time. It has full type inference, so if you
have enough type constraints in your compilation unit such that there is only
one solution, the compiler will be satisfied.

## Compiled

It currently has working LLVM codegen, with JIT'd code running in our test
suites, but the plan is to support libgccjit and others.

## Easily bootstrappable

The piq stage0 compiler is written in C99.[^4] This is currently the only compiler.

## Usage

Please see the (incomplete) [usage guide](docs/guide.md).

[^1]: This is on laptop with an AMD Ryzen 5 5625U. The compiler performance can
be greatly improved by multi-threading, and by a tonne of optimizations I haven't
yet had time to implement.

[^2]: Takes about 3.4 seconds on my laptop. This can be improved, see [^4].

[^3]: The current tuple syntax doesn't follow this rule, but this is a bug I
will fix soon.

[^4]: There's a smidgen of C++, written because LLVM's C interface is missing a
feature I want. This is trivial to get rid of, but requires a malloc somewhere
to do so, so I haven't brought myself to :fire: do it yet.
