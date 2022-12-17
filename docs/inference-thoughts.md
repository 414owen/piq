Initial thought:

* Every variable has a list of constraints
* Usage refines possibilities

Here, the type variable associated with a variable is represented as the
variable's name primed (').

```
(let a 3)   // a' is Num from assignment
(let b a)   // b' ~ a' from assignment
(sig c I32) // c' = I32 from sig
(let c a')  // c' = a' from assignment =>  a' = I32 from assignment  =>  b' = I32
```

```
(let a 3)                   // a' is Num from assignment
(let b 4)                   // b' is Num from assignment
(sig + (Num a) (Fn a a a))
(fun + <elided>)
(fun f (+ a b))             // a' ~ b' from unification of 'a's in +
(let c "hi")                // c' = string
(+ b c)                    
            // error: string isn't an instance of number from constraint on 'a's in +
            // what's required to get this right?
            // 1: check all type variables against constraints of +
            // 2: create unification constraints between parameters
```
