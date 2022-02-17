# Syntax


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

// tuples
(1, 2, 3)
(a, b, c)

// lists
[1, 2, 3]
[a, b, c]


(if a c d)
```


## Types

```
// primitives
I8, I16, I32, I64
U8, U16, U32, U64

// tuples
(I32, U8)

// arrays
[U8]

// functions
(U8 -> U8)
```


## Statements

### Declarations

```
// function
(a : I32 -> I32)
(fn a b b)

(a : (I32, I32) -> I32)
(fn a (b, c) (+ b c))

// constant
(b : I64)
(let b 1)

// variable
(c : I64)
(var c 12)


(do 
  (print "hi")
  (other statement))


// probably doesn't typecheck
(case a
  ("this" "hi")
  ((Pat b) b)
  ("test" "yes"))


(sig fib (U16 -> U64))
(fn fib n
  (if (lt n 2)
    n
    (+ (fib (- n 1))
       (fib (- n 2)))))


(type Animal
  (Dog String)
  (Cat String))


// gadts
(type (Vec
```