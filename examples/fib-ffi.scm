(#abi c)
(fun fib (n)
  (if
    (i64-lt? n 2)
    n
    (i64-add
      (fib (i64-sub n 1))
      (fib (i64-sub n 2)))))

(fun main () (fib 10))
