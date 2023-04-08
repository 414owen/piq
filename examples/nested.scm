(sig snd (Fn (I32, I32) I32))
(fun snd ((a, b)) b)

(sig sndA (Fn (I8, I32) I32))
(fun sndA ((a, b)) b)

(sig test (Fn I32))
(fun test () (snd (1, (snd (2, 3)))))
