#lang racket
(require redex)
(require file/sha1)

(define-language Flexibench

  (atom :: number string)
  (cell ::= atom (atom ...))
  (key ::= variable-not-otherwise-mentioned)
  (kcp ::= (key cell))
  (row ::= (kcp ...))
  (var ::= variable-not-otherwise-mentioned)
  (val ::= var (row ...))
  (exp ::=
       val
       (let (var ... exp) exp)
       (append exp exp)
       (crossprd exp exp)
       (split-by-kcp exp exp)
       (implode exp)
       (explode exp)
       (run exp)
       ((:= var exp) exp))
  (mod ::=
       (module
           (var ...)
         (var ...)
         exp))

  (env ::= ((var val) ...))

  #:binding-forms
  (let (var ... exp_1) exp_2 #:refers-to (shadow var ...))
  (module (var_i ...) (var_o ...) exp #:refers-to (shadow var_i ... var_o ...))

  )

(define exp1
  (term
   (((x 1) (y 2))
    ((x 2) (y 4))
    ((x 3) (y 6)))))

(define exp2
  (term
   (((z "a"))
    ((z "b")))))

(define-metafunction Flexibench
  Unique : (any ...) -> boolean
  [(Unique (any_!_1 ...)) #t]
  [(Unique (_ ...)) #f])

(define-metafunction Flexibench
  Nonempty : (any ...) -> boolean
  [(Nonempty ())
   #f]
  [(Nonempty _)
   #t])

(define-judgment-form Flexibench
  #:mode (Wf-row I)
  #:contract (Wf-row row)

  [(side-condition (Nonempty (key ...)))
   (side-condition (Unique (key ...)))
   ------------------------- "row"
   (Wf-row ((key cell) ...))])

(define-judgment-form Flexibench
  #:mode (Wf-val I)
  #:contract (Wf-val val)

  [----------- "empty"
   (Wf-val ())]
  
  [----------- "unit"
   (Wf-val (()))]
  
  [(Wf-row row) ...
   (side-condition (Nonempty (row ...)))
   ------------------------- "populated"
   (Wf-val (row ...))])

(define-metafunction Flexibench
  Cross-row : (row ...) row -> (row ...)
  [(Cross-row () _)
   ()]
  [(Cross-row ((kcp_b ...) row_a ...) (kcp ...))
   ((kcp_b ... kcp ...)
    row_r ...)
   (where/error (row_r ...) (Cross-row (row_a ...) (kcp ...)))])

(define-metafunction Flexibench
  Cross-rows : (row ...) (row ...) -> (row ...)
  [(Cross-rows (row ...) ())
   ()]
  [(Cross-rows (row ...) (row_b row_a ...))
   (row_r1 ... row_r2 ...)
   (where/error (row_r1 ...) (Cross-row (row ...) row_b))
   (where/error (row_r2 ...) (Cross-rows (row ...) (row_a ...)))])

(define-metafunction Flexibench
  Match-rows : row row -> boolean
  [(Match-rows _ ())
   #t]
  [(Match-rows (kcp_1 ... kcp_b kcp_2 ...) (kcp_b kcp_a ...))
   (Match-rows (kcp_1 ... kcp_2 ...) (kcp_a ...))]
  [(Match-rows _ _)
   #f])

(test-equal (term (Match-rows ((y 4)) ((y 4)))) #t)
(test-equal (term (Match-rows ((y 4)) ((y 5)))) #f)
(test-equal (term (Match-rows ((y 4)) ((x 5) (y 4)))) #f)
(test-equal (term (Match-rows ((y 4) (a "b")) ((y 4)))) #t)
(test-equal (term (Match-rows ((y 4) (x 3) (a "b") (b "c")) ((a "b") (y 4)))) #t)

(define-metafunction Flexibench
  Match-any-rows : row (row ...) -> boolean
  [(Match-any-rows _ ())
   #f]
  [(Match-any-rows row (row_b _ ...))
   #t
   (side-condition (term (Match-rows row row_b)))]
  [(Match-any-rows row (_ row_a ...))
   (Match-any-rows row (row_a ...))])

(test-equal (term (Match-any-rows () ())) #f)
(test-equal (term (Match-any-rows ((a "b")) ())) #f)
(test-equal (term (Match-any-rows ((a "b")) (((a "b"))))) #t)
(test-equal (term (Match-any-rows ((x 1) (a "b")) (((a "b"))))) #t)
(test-equal (term (Match-any-rows ((x 1) (a "b")) (((a "b") (x 1))))) #t)
(test-equal (term (Match-any-rows ((x 1) (a "b")) (((x 1)) ((a "b") (x 1))))) #t)
(test-equal (term (Match-any-rows ((x 1) (a "b")) (((x 1)) ((a "b") (x 2))))) #t)
(test-equal (term (Match-any-rows ((x 1) (a "b")) (((x 2)) ((a "b") (y 1))))) #f)

(define-metafunction Flexibench
  Split-rows : (row ...) (row ...) -> ((row ...) (row ...))
  [(Split-rows () _)
   (() ())]
  [(Split-rows (row_b row_a ...) (row ...))
   ((row_b row_r1 ...) (row_r2 ...))
   (side-condition (term (Match-any-rows row_b (row ...))))
   (where/error ((row_r1 ...) (row_r2 ...)) (Split-rows (row_a ...) (row ...)))]
  [(Split-rows (row_b row_a ...) (row ...))
   ((row_r1 ...) (row_b row_r2 ...))
   (side-condition (not (term (Match-any-rows row_b (row ...)))))
   (where/error ((row_r1 ...) (row_r2 ...)) (Split-rows (row_a ...) (row ...)))])

(test-equal (term (Split-rows ,exp1 ())) (term (() (((x 1) (y 2)) ((x 2) (y 4)) ((x 3) (y 6))))))

(define-metafunction Flexibench
  Merge-envs : (env ...) -> env
  [(Merge-envs ())
   ()]
  [(Merge-envs (((var_b val_b) ...) env_a ...))
   ((var_b val_b) ... (var_r val_r) ...)
   (where/error ((var_r val_r) ...) (Merge-envs (env_a ...)))])

(define-metafunction Flexibench
  List-of-cell : cell -> cell
  [(List-of-cell atom)
   (atom)]
  [(List-of-cell cell)
   cell])

(define-metafunction Flexibench
  Merge-two-rows : row row -> row
  [(Merge-two-rows () ((key cell) ...))
   row
   (where/error row ((key (List-of-cell cell)) ...))]
  [(Merge-two-rows ((key cell_1) kcp_a1 ...) (kcp_b2 ... (key cell_2) kcp_a2 ...))
   ((key cell_3) kcp_r ...)
   (where/error (atom_1 ...) (List-of-cell cell_1))
   (where/error (atom_2 ...) (List-of-cell cell_2))
   (where/error cell_3 (atom_1 ... atom_2 ...))
   (where/error (kcp_r ...) (Merge-two-rows (kcp_a1 ...) (kcp_b2 ... kcp_a2 ...)))]
  [(Merge-two-rows ((key cell_1) kcp_a1 ...) (kcp_2 ...))
   ((key cell_2) kcp_r ...)
   (where/error cell_2 (List-of-cell cell_1))
   (where/error (kcp_r ...) (Merge-two-rows (kcp_a1 ...) (kcp_2 ...)))])   
  
(define-metafunction Flexibench
  Implode : (row ...) -> row
  [(Implode (row ...))
   ,(foldr (λ (r1 r2) (term (Merge-two-rows ,r1 ,r2))) '() (term (row ...)))])

(define (bytes->sha1 bs)
  (sha1 (open-input-bytes bs)))

(define (number->sha1 n)
  (bytes->sha1 (string->bytes/utf-8 (number->string n))))

(define (char->hex-digit c)
  ;; Turn a character into a hex digit
  (cond [(char<=? #\0 c #\9)
         (- (char->integer c) (char->integer #\0))]
        [(char<=? #\A c #\F)
         (+ 10 (- (char->integer c) (char->integer #\A)))]
        [(char<=? #\a c #\f)
         (+ 10 (- (char->integer c) (char->integer #\a)))]
        [else
         (error 'char->hex-digit "~A is not a hex character" c)]))

(define (hex-string->byte-list hs)
  (for/list ([h (in-string hs 0 #f 2)]
             [l (in-string hs 1 #f 2)])
    (+ (* (char->hex-digit h) 16) (char->hex-digit l))))

(define (random-of-number n)
  (apply + (hex-string->byte-list (number->sha1 n))))

(define (random-of-string s)
  (apply + (bytes->list (string->bytes/utf-8 s))))

(define-metafunction Flexibench
  Random-of-cell : cell -> number
  [(Random-of-cell number)
   ,(random-of-number (term number))]
  [(Random-of-cell string)
   ,(random-of-string (term string))]
  [(Random-of-cell _)
   123])

(define-metafunction Flexibench
  Random-of-row : row number -> number
  [(Random-of-row ((key cell) ...) number_1)
   ,(random-of-number (apply + (term (number_1 number_r ...))))
   (where/error (number_r ...) ((Random-of-cell cell) ...))])

(define-metafunction Flexibench
  Output-of-run : row -> val
  [(Output-of-run row_1)
   (((exectime (Random-of-row row_1 0)))
    ((exectime (Random-of-row row_1 (Random-of-row row_1 0)))))])

(define-judgment-form Flexibench
  #:mode (⇓ I O O)
  #:contract (⇓ exp val env)

  [(Wf-val val)
   ----------- "val"
   (⇓ val val ())]

  [(⇓ exp_1 val_1 env_1)
   (where/error exp_3 (substitute exp_2 var_1 val_1))
   (⇓ exp_3 val env)
   ----------- "let"
   (⇓ (let (var_1 exp_1) exp_2) val env)]

  [(⇓ exp_1 (row_1 ...) env_1)
   (⇓ exp_2 (row_2 ...) env_2)
   (where/error val (row_1 ... row_2 ...))
   (where/error env (Merge-envs (env_1 env_2)))
   ---------------------------- "append"
   (⇓ (append exp_1 exp_2) val env)]

  [(⇓ exp_1 (row_1 ...) env_1)
   (⇓ exp_2 (row_2 ...) env_2)
   (where/error val (Cross-rows (row_1 ...) (row_2 ...)))
   (where/error env (Merge-envs (env_1 env_2)))
   ---------------------------- "cross"
   (⇓ (crossprd exp_1 exp_2) val env)]

  [(⇓ exp_1 (row_1 ...) env_1)
   (⇓ exp_2 (row_2 ...) env_2)
   (where/error ((row_t ...) (row_d ...)) (Split-rows (row_1 ...) (row_2 ...)))
   (where/error exp_4 (substitute exp_3 var_t (row_t ...)))
   (where/error exp_5 (substitute exp_4 var_d (row_d ...)))
   (⇓ exp_5 val env_3)
   (where/error env (Merge-envs (env_1 env_2 env_3)))
   ---------------------------- "split-by-kcp"
   (⇓ (let (var_t var_d (split-by-kcp exp_1 exp_2)) exp_3) val env)]

  [(⇓ exp_1 val_1 env_1)
   (where/error row (Implode val_1))
   (where/error val (row))
   ----------- "implode"
   (⇓ (implode exp_1) val env_1)]

  [(⇓ exp_1 (row_1 ...) env_1)
   (where/error (val_r ...) ((Cross-rows (row_1) (Output-of-run row_1)) ...))
   (where/error ((row_r ...) ...) (val_r ...))
   (where/error val (row_r ... ...))
   ----------- "run"
   (⇓ (run exp_1) val env_1)]
  
  [(⇓ exp_a val_a env_1)
   (⇓ exp_2 val env_2)
   (where/error env (Merge-envs (env_1 env_2)))
   ----------------------------------- ":="
   (⇓ ((:= var exp_a) exp_2) val env)]
  
  )

(test-equal (judgment-holds (Wf-val (()))) #t)
(test-equal (judgment-holds (Wf-val (((x 1)) ()))) #f)
(test-equal (judgment-holds (Wf-val (((x 1)) ((y 1))))) #t)

(test-equal (judgment-holds (Wf-val ,exp1)) #t)
(test-equal (judgment-holds (Wf-val ,exp2)) #t)

(test-equal
 (judgment-holds
  (⇓ (append ,exp1 ,exp2) val env) val)
 (term ((((x 1) (y 2))
         ((x 2) (y 4))
         ((x 3) (y 6))
         ((z "a"))
         ((z "b"))))))

(test-equal
 (judgment-holds
  (⇓ (crossprd ,exp1 ,exp2) val env) val)
 (term ((((x 1) (y 2) (z "a"))
         ((x 2) (y 4) (z "a"))
         ((x 3) (y 6) (z "a"))
         ((x 1) (y 2) (z "b"))
         ((x 2) (y 4) (z "b"))
         ((x 3) (y 6) (z "b"))))))

(test-equal
 (judgment-holds
  (⇓ (let (var-t var-d (split-by-kcp ,exp1 (((y 4))))) var-t) val env) val)
 (term ((((x 2) (y 4))))))

(test-equal
 (judgment-holds
  (⇓ (let (var-t var-d (split-by-kcp ,exp1 (((y 4))))) var-d) val env) val)
 (term ((((x 1) (y 2)) ((x 3) (y 6))))))

(test-equal
 (judgment-holds
  (⇓ (let (var-t var-d (split-by-kcp ,exp1 ())) var-d) val env) val)
 (term ((((x 1) (y 2)) ((x 2) (y 4)) ((x 3) (y 6))))))

(test-equal
 (judgment-holds
  (⇓ (let (var-t var-d (split-by-kcp ,exp1 ())) var-t) val env) val)
 (term (())))

(test-equal
 (judgment-holds
  (⇓ (let (var-t var-d (split-by-kcp ,exp1 (()))) var-t) val env) val)
 (term (,exp1)))

(test-equal
 (judgment-holds
  (⇓ (let (x ,exp1)
       (crossprd x ,exp2)) val env) val)
 (term ((((x 1) (y 2) (z "a"))
         ((x 2) (y 4) (z "a"))
         ((x 3) (y 6) (z "a"))
         ((x 1) (y 2) (z "b"))
         ((x 2) (y 4) (z "b"))
         ((x 3) (y 6) (z "b"))))))

(define exp-speedup
  (term
   (let (prog-par (((prog "foo_par"))))
     (let (procs (((proc 1)) ((proc 2)) ((proc 4))))
       (append
        (((prog "foo_seq")))
        (crossprd prog-par procs))))))
  
(test-equal
 (judgment-holds
  (⇓ ,exp-speedup val env) val)
 (term ((((prog "foo_seq"))
         ((prog "foo_par") (proc 1))
         ((prog "foo_par") (proc 2))
         ((prog "foo_par") (proc 4))))))

(test-equal
 (judgment-holds
  (⇓ (run ,exp-speedup) val env) val)
 (term ((((prog "foo_seq") (exectime 2665))
         ((prog "foo_seq") (exectime 2279))
         ((prog "foo_par") (proc 1) (exectime 2291))
         ((prog "foo_par") (proc 1) (exectime 2222))
         ((prog "foo_par") (proc 2) (exectime 2625))
         ((prog "foo_par") (proc 2) (exectime 2728))
         ((prog "foo_par") (proc 4) (exectime 3122))
         ((prog "foo_par") (proc 4) (exectime 2574))))))