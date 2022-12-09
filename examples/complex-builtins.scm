(sig test (Fn I32 I32))
(fun test a
  (let f (if (i32-lte? (a, 12)) i32-add i32-sub))
  (f (3, 4)))
