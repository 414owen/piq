# Syntax

## Types

```
// primitives
I8, I16, I32, I64
U8, U16, U32, U64


// tuples
(I32, U8)


// variable length arrays
[U8]


// static sized arrays
[U8 12]


// functions
(fn U8 U8)
```


## Terms

```
// ints
1


// strings
"hello"


// chars (U8)
'a'
'\0'


// bindings
a
aBc


// tuple literals
(1, 2, 3)
(a, 2, c)


// list literals
[1, 2, 3]
[a, 2, c]


// ternary expressions
(if a c d)


// blocks
(do
  (print "hi")
  (other statement))


// pattern matching
(case a
  ("this" "hi")
  ((Pat b) b)
  ("test" "yes"))

// lambdas
(fn a a)
(fn (a, b) (+ a b))
```


## Declarations

```
// signatures and functions, pattern matching and fall-through
(sig a (fn (I32, I32) I32))
(fun a (1, b)
  (a (2, b)))
(fun a (b, c)
  (+ b c))

// which is the same as
(fun pa
  (case ps
    ((b, c) (+ b c))))

// constants
(sig b I64)
(let b 1)


// variables
(sig c I64)
(var c 12)


// sum types
(type Animal
  (Dog (breed String))
  (Cat (type String)))
```