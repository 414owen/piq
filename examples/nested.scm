(sig snd (Fn I32 I8 I8))
(fun snd (a b) b)

(sig (Fn I8))
(fun test () (snd 1 (snd 2 3)))
