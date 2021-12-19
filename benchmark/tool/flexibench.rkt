#lang racket
(require redex file/sha1 math/statistics plot)

(define-language Flexibench

  (atom :: number string)
  (cell ::= atom (atom ...))
  (key ::= variable-not-otherwise-mentioned)
  (kcp ::= (key cell))
  (row ::= (kcp ...))
  (var ::= variable-not-otherwise-mentioned)
  ; later: consider adding to the value grammar a remote reference, e.g., a hash, which should help to handle large tables.
  (val ::= var (row ...))
  (run-id ::= variable-not-otherwise-mentioned)
  (builtin ::= ++ × ÷/kcp implode explode)
  (exp ::=
       val
       (let (var ... exp) exp)
       (builtin exp ...)
       (run run-id exp)
       ((:= var exp) exp))
  (env ::= ((var val) ...))
  (E ::=
     (let (var ... E) exp)
     (builtin val ... E exp ...)
     (run run-id E)
     ((:= var E) exp)
     hole)
  (exp-env ::= (exp env))

  (run-input ::=
             ((path-to-executable string)
              (cmdline-args atom ...)
              (env-var-args (string atom) ...)
              (timeout-sec number)
              (cwd string)))
  (run-output ::=
              ((return-code integer)
               (stdout string)
               (stderr string)
               (elapsed number)))
  (trace-run ::= (run-input run-output))
  (trace ::= (trace-run ...))
  (hash ::= string)
  (store ::=
         (todo ((hash val) ...))
         (done ((hash val) ...))
         (failed ((hash val) ...))
         (done-trace-links (natural natural))
         (failed-trace-links (natural natural)))

  ; later: consider how to make an experiment a crdt
  ; in particular, it should be possible to make the benchmark expressions modifiable in much the same way as json dictionaries are in the way described by Schlepman et al
  (bench ::=
         (benchmark
          ((run-id ...)
           (var ...))
          ((var exp) ...)))
  (run-execmode ::=
                overwrite
                append
                read-only)
  (run-input-key ::=
                 path-to-executable-key
                 env-var-key
                 silent-key
                 timeout-sec-key
                 cwd-key)
  (run-config ::= ((key run-input-key) ...))
  (run-configs ::= ((run-id run-config run-execmode) ...))
  (experiment ::= (bench run-configs store))

  #:binding-forms
  (let (var ... exp_1) exp_2 #:refers-to (shadow var ...))
  (benchmark
   ((run-id ...)
    (var ...))
   ((var exp) ...) #:refers-to (shadow var ... run-id ...))
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

(define exp3
  (term
   (((x "a2"))
    ((x "b2")))))

(define-metafunction Flexibench
  Lookup-or-default : ((any any) ...) any any -> any
  [(Lookup-or-default (_ ... (any any_0) _ ...) any _) any_0]
  [(Lookup-or-default _ _ any_dflt) any_dflt])

(define-metafunction Flexibench
  Lookup : ((any any) ...) any -> any or #f
  [(Lookup ((any any_0) ...) any_1) (Lookup-or-default ((any any_0) ...) any_1 #f)])

(test-equal (term (Lookup-or-default ((a 1) (b 2)) b 300))
            (term 2))
(test-equal (term (Lookup-or-default ((a 1) (b 2)) c 300))
            (term 300))

(define-relation Flexibench
  Unique ⊆ (any ...)
  [(Unique (any_!_1 ...))])

(define-relation Flexibench
  Nonempty ⊆ (any ...)
  [(Nonempty (any_b any_a ...))])

(define-relation Flexibench
  Wf-row ⊆ row
  [(Wf-row ((key cell) ...))
   (Unique (key ...))
   (Nonempty (key ...))])

(define-relation Flexibench
  Wf-val ⊆ val
  [(Wf-val ())]
  [(Wf-val (()))]
  [(Wf-val (row ...))
   (Wf-row row) ...
   (Nonempty (row ...))])

(define-metafunction Flexibench
  Append : (val ...) -> val
  [(Append ((row ...) ...)) (row ... ...)])

(define-metafunction Flexibench
  Cross-row : (row ...) row -> (row ...)
  [(Cross-row () _)
   ()]
  [(Cross-row ((kcp_b ...) row_a ...) (kcp ...))
   ((kcp_b ... kcp ...)
    row_r ...)
   (where/error (row_r ...) (Cross-row (row_a ...) (kcp ...)))])

(define-metafunction Flexibench
  Cross-rows-2 : (row ...) (row ...) -> (row ...)
  [(Cross-rows-2 (row ...) ())
   ()]
  [(Cross-rows-2 (row ...) (row_b row_a ...))
   (row_r1 ... row_r2 ...)
   (where/error (row_r1 ...) (Cross-row (row ...) row_b))
   (where/error (row_r2 ...) (Cross-rows-2 (row ...) (row_a ...)))])

(define-metafunction Flexibench
  Cross-rows : ((row ...) ...) -> (row ...)
  [(Cross-rows ())
   ()]
  [(Cross-rows ((row ...)))
   (row ...)]
  [(Cross-rows ((row_b1 ...) (row_b2 ...) (row_a ...) ...))
   (Cross-rows ((Cross-rows-2 (row_b1 ...) (row_b2 ...)) (row_a ...) ...))])

(define-relation Flexibench
  Rows-match? ⊆ row × row
  [(Rows-match? _ ())]
  [(Rows-match? (kcp_1 ... kcp_b kcp_2 ...) (kcp_b kcp_a ...))
   (Rows-match? (kcp_1 ... kcp_2 ...) (kcp_a ...))])

(test-judgment-holds (Rows-match? ((y 4)) ((y 4))))
(test-equal (term (Rows-match? ((y 4)) ((y 5)))) #f)
(test-equal (term (Rows-match? ((y 4)) ((x 5) (y 4)))) #f)
(test-judgment-holds (Rows-match? ((y 4) (a "b")) ((y 4))))
(test-judgment-holds (Rows-match? ((y 4) (x 3) (a "b") (b "c")) ((a "b") (y 4))))

(define-relation Flexibench
  Row-matches-any-other? ⊆ row × (row ...)
  [(Row-matches-any-other? row (row_b _ ...))
   (Rows-match? row row_b)]
  [(Row-matches-any-other? row (_ row_a ...))
   (Row-matches-any-other? row (row_a ...))])

(test-equal (term (Row-matches-any-other? () ())) #f)
(test-equal (term (Row-matches-any-other? ((a "b")) ())) #f)
(test-equal (term (Row-matches-any-other? ((a "b")) (((a "b"))))) #t)
(test-equal (term (Row-matches-any-other? ((x 1) (a "b")) (((a "b"))))) #t)
(test-equal (term (Row-matches-any-other? ((x 1) (a "b")) (((a "b") (x 1))))) #t)
(test-equal (term (Row-matches-any-other? ((x 1) (a "b")) (((x 1)) ((a "b") (x 1))))) #t)
(test-equal (term (Row-matches-any-other? ((x 1) (a "b")) (((x 1)) ((a "b") (x 2))))) #t)
(test-equal (term (Row-matches-any-other? ((x 1) (a "b")) (((x 2)) ((a "b") (y 1))))) #f)

(define-metafunction Flexibench
  Split-rows-2 : (row ...) (row ...) -> ((row ...) (row ...))
  [(Split-rows-2 () _)
   (() ())]
  [(Split-rows-2 (row_b row_a ...) (row ...))
   ((row_b row_r1 ...) (row_r2 ...))
   (side-condition (term (Row-matches-any-other? row_b (row ...))))
   (where/error ((row_r1 ...) (row_r2 ...)) (Split-rows-2 (row_a ...) (row ...)))]
  [(Split-rows-2 (row_b row_a ...) (row ...))
   ((row_r1 ...) (row_b row_r2 ...))
   (side-condition (not (term (Row-matches-any-other? row_b (row ...)))))
   (where/error ((row_r1 ...) (row_r2 ...)) (Split-rows-2 (row_a ...) (row ...)))])

(test-equal (term (Split-rows-2 ,exp1 ())) (term (() (((x 1) (y 2)) ((x 2) (y 4)) ((x 3) (y 6))))))

(define-metafunction Flexibench
  Split-rows-helper : val (val ...) -> (val ...)
  [(Split-rows-helper _ ())
   ()]
  [(Split-rows-helper val_b1 (val_b2 val_a ...))
   (val_r1 val_r ...)
   (where/error (val_r1 val_r2) (Split-rows-2 val_b1 val_b2))
   (where/error (val_r ...) (Split-rows-helper val_b1 (val_a ...)))])

(define-metafunction Flexibench
  Split-rows : (val ...) -> (val ...)
  [(Split-rows ())
   ()]
  [(Split-rows (_))
   ()]
  [(Split-rows (val_b val_a ...))
   (val_r ... val_c)
   (where/error (val_r ...) (Split-rows-helper val_b (val_a ...)))
   (where/error (_ val_c) (Split-rows-2 val_b (Append (val_a ...))))])

(test-equal (term (Split-rows ( (((prog "foo_seq") (exectime 2665))
                                 ((prog "foo_seq") (exectime 2279))
                                 ((prog "foo_par") (proc 1) (exectime 2291))
                                 ((prog "foo_par") (proc 1) (exectime 2222))
                                 ((prog "foo_par") (proc 2) (exectime 2625))
                                 ((prog "foo_par") (proc 2) (exectime 2728))
                                 ((prog "foo_par") (proc 4) (exectime 3122))
                                 ((prog "foo_par") (proc 4) (exectime 2574)))
                                (((prog "foo_par") (proc 2)))
                                (((prog "foo_par") (proc 4))) )))
            (term ((((prog "foo_par") (proc 2) (exectime 2625))
                    ((prog "foo_par") (proc 2) (exectime 2728)))
                   (((prog "foo_par") (proc 4) (exectime 3122))
                    ((prog "foo_par") (proc 4) (exectime 2574)))
                   (((prog "foo_seq") (exectime 2665))
                    ((prog "foo_seq") (exectime 2279))
                    ((prog "foo_par") (proc 1) (exectime 2291))
                    ((prog "foo_par") (proc 1) (exectime 2222))))))

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

(define-metafunction Flexibench
  Shave-row : row -> (row row)
  [(Shave-row ())
   (() ())]
  [(Shave-row ((key_b ()) kcp_a ...))
   (Shave-row (kcp_a ...))]
  [(Shave-row ((key_b (atom_b atom_a ...)) kcp_a ...))
   (((key_b atom_b) kcp_1 ...) ((key_b (atom_a ...)) kcp_2 ...))
   (where/error ((kcp_1 ...) (kcp_2 ...)) (Shave-row (kcp_a ...)))]
  [(Shave-row ((key_b atom_b) kcp_a ...))
   (((key_b atom_b) kcp_1 ...) row_2)
   (where/error ((kcp_1 ...) row_2) (Shave-row (kcp_a ...)))])

(test-equal (term (Shave-row ((x (1 2))))) (term (((x 1)) ((x (2))))))
(test-equal (term (Shave-row ((x (1))))) (term (((x 1)) ((x ())))))

(define-metafunction Flexibench
  Explode-row : row -> (row ...)
  [(Explode-row ())
   ()]
  [(Explode-row row)
   ()
   (where (() ()) (Shave-row row))]
  [(Explode-row row)
   (row_1 row_3 ...)
   (where/error (row_1 row_2) (Shave-row row))
   (where/error (row_3 ...) (Explode-row row_2))])

(test-equal (term (Explode-row ((x (1 2))))) (term (((x 1)) ((x 2)))))
(test-equal (term (Explode-row ((x (1 2)) (y (3 4))))) (term (((x 1) (y 3)) ((x 2) (y 4)))))

(define-metafunction Flexibench
  Explode : (row ...) -> (row ...)
  [(Explode ())
   ()]
  [(Explode (row_b row_a ...))
   (row_b1 ... row_b2 ...)
   (where/error (row_b1 ...) (Explode-row row_b))
   (where/error (row_b2 ...) (Explode (row_a ...)))])

(define (bytes->sha1 bs)
  (sha1 (open-input-bytes bs)))

(define-metafunction Flexibench
  Hash-of-string : string -> string
  [(Hash-of-string string) ,(bytes->sha1 (string->bytes/utf-8 (term string)))])

(test-equal (term (Hash-of-string "foo"))
            (term "0beec7b5ea3f0fdbc95d0dd47f3c5bc275da8a33"))

(define (number->sha1 n)
  (bytes->sha1 (string->bytes/utf-8 (number->string n))))

(define-metafunction Flexibench
  String-of-atom : atom -> string
  [(String-of-atom string) string]
  [(String-of-atom number) ,(number->string (term number))])

(define-metafunction Flexibench
  Csv-append : string ... -> string
  [(Csv-append string ...) ,(string-join (term (string ...)) ",")])

(test-equal (term (Csv-append "a" "b" "c")) (term "a,b,c"))

(define-metafunction Flexibench
  Csv-of-cell : cell -> string
  [(Csv-of-cell atom)
   (String-of-atom atom)]
  [(Csv-of-cell (atom ...))
   ,(string-join (term (string ...)) ";")
   (where/error (string ...) ((String-of-atom atom) ...))])

(test-equal (term (Csv-of-cell (1 2 3 "a"))) (term "1;2;3;a"))

(define-metafunction Flexibench
  String-of-variable : variable -> string
  [(String-of-variable variable) ,(symbol->string (term variable))])

(define-metafunction Flexibench
  Csv-of-kcp : kcp -> string
  [(Csv-of-kcp (key cell))
   (Csv-append (String-of-variable key) (Csv-of-cell cell))])

(test-equal (term (Csv-of-kcp (a 1))) (term "a,1"))

(define-metafunction Flexibench
  Csv-of-row : row -> string
  [(Csv-of-row (kcp ...))
   (Csv-append string ...)
   (where/error (string ...) ((Csv-of-kcp kcp) ...))])

(test-equal (term (Csv-of-row ((a 1) (b "2") (c (3 4)))))
            (term "a,1,b,2,c,3;4"))

(define-metafunction Flexibench
  Sort-kvp : ((key any) ...) -> ((key any) ...)
  [(Sort-kvp ((key any) ...))
   ,(sort (term ((key any) ...)) #:key (λ (p) (symbol->string (car p))) string<?)])

(test-equal (term (Sort-kvp ((c 34) (a "b") (b "xx3") (x 1))))
            (term ((a "b") (b "xx3") (c 34) (x 1))))

(define-metafunction Flexibench
  Csv-of-run-input : (key run-input-key) -> string
  [(Csv-of-run-input (key run-input-key))
   (Csv-append (String-of-variable key) (String-of-variable run-input-key))])
   
(define-metafunction Flexibench
  Csv-of-run-config : run-config -> string
  [(Csv-of-run-config ((key run-input-key) ...))
   (Csv-append string ...)
   (where/error (string ...) ((Csv-of-run-input (key run-input-key)) ...))])

(test-equal (term (Csv-of-run-config ((prog path-to-executable-key)
                                      (foo env-var-key)
                                      (bar silent-key))))
            (term "prog,path-to-executable-key,foo,env-var-key,bar,silent-key"))

(define-metafunction Flexibench
  Csv-of-run : run-config val -> (string ...)
  [(Csv-of-run run-config (row ...))
   ((Csv-of-run-config (Sort-kvp run-config)) (Csv-of-row (Sort-kvp row)) ...)])

(test-equal (term (Csv-of-run ((prog path-to-executable-key)
                               (foo env-var-key)
                               (bar silent-key))
                              (((a 1) (b "2") (c (3 4)))
                               ((a 321) (c (3 4))))))
            (term ("bar,silent-key,foo,env-var-key,prog,path-to-executable-key"
                   "a,1,b,2,c,3;4"
                   "a,321,c,3;4")))

(define-metafunction Flexibench
  Hash-of-strings : (string ...) -> string
  [(Hash-of-strings (string ...))
   ,(foldr (λ (v h) (term (Hash-of-string ,(string-append (term (Hash-of-string ,v)) h))))
           ""
           (term (string ...)))])

(test-equal (term (Hash-of-strings ("a" "b" "c")))
            (term "9025177a9ecc6c6a7bd53c16e25a7a909f9b6e6a"))

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

(define-metafunction Flexibench
  [(substitute* any_o ((var any_n) (var_a any_na) ...))
   (substitute* any_o2 ((var_a any_na) ...))
   (where any_o2 (substitute any_o var any_n))]
  [(substitute* any_o ())
   any_o])

(define-metafunction Flexibench
  Run-builder : ((key run-input-key) ...) row -> (((run-input-key kcp) ...) row)
  [(Run-builder _ ())
   (() ())]
  [(Run-builder ((key run-input-key) ...) ((key_t cell_t) kcp_a ...))
   (((run-input-key_t (key_t cell_t)) (run-input-key_r kcp_r) ...) row)
   (where run-input-key_t (Lookup ((key run-input-key) ...) key_t))
   (where/error (((run-input-key_r kcp_r) ...) row)
                (Run-builder ((key run-input-key) ...) (kcp_a ...)))]
  [(Run-builder ((key run-input-key) ...) ((key_t cell_t) kcp_a ...))
   (((run-input-key_r kcp_r) ...) ((key_t cell_t) kcp_r2 ...))
   (where/error (((run-input-key_r kcp_r) ...) (kcp_r2 ...))
                (Run-builder ((key run-input-key) ...) (kcp_a ...)))])

(test-equal (term (Run-builder ((xxx env-var-key) (prog path-to-executable-key))
                               ((prog "foo") (aaa 321) (xxx 123) (yyy "zzz"))))
            (term (((path-to-executable-key (prog "foo"))
                    (env-var-key (xxx 123)))
                   ((aaa 321) (yyy "zzz")))))

(define-metafunction Flexibench
  Cmdline-args : row -> (atom ...)
  [(Cmdline-args ())
   ()]
  [(Cmdline-args ((key cell) kcp_a ...))
   (,(string-append "-" (symbol->string (term key))) cell atom ...)
   (where/error (atom ...) (Cmdline-args (kcp_a ...)))])

(define-metafunction Flexibench
  Env-var-args : ((run-input-key kcp) ...) -> ((string atom) ...)
  [(Env-var-args ())
   ()]
  [(Env-var-args ((env-var-key (key atom)) (run-input-key_a kcp_a) ...))
   ((string_v atom_v) (string_r atom_r) ...)
   (where/error (string_v atom_v) (,(symbol->string (term key)) atom))
   (where/error ((string_r atom_r) ...) (Env-var-args ((run-input-key_a kcp_a) ...)))]
  [(Env-var-args (_ (run-input-key_a kcp_a) ...))
   (Env-var-args ((run-input-key_a kcp_a) ...))])

(define-metafunction Flexibench
  Run-input : ((key run-input-key) ...) row -> run-input
  [(Run-input ((key run-input-key) ...) row)
   ((path-to-executable string_pte)
    (cmdline-args atom_cmdline ...)
    (env-var-args (string_env atom_env) ...)
    (timeout-sec number_timeout)
    (cwd string_cwd))
   (where/error (((run-input-key_r kcp_r) ...) row_r) (Run-builder ((key run-input-key) ...) row))
   (where/error (_ string_pte) (Lookup ((run-input-key_r kcp_r) ...) path-to-executable-key))
   (where/error (atom_cmdline ...) (Cmdline-args row_r))
   (where/error ((string_env atom_env) ...) (Env-var-args ((run-input-key_r kcp_r) ...)))
   (where/error number_timeout (Lookup-or-default ((run-input-key_r kcp_r) ...) timeout-key 200))
   (where/error string_cwd (Lookup-or-default ((run-input-key_r kcp_r) ...) cwd-key ""))])

(define run-cfg1
  (term ((xxx env-var-key) (prog path-to-executable-key))))

(define run-input1
  (term (Run-input ,run-cfg1
                   ((prog "foo") (aaa 321) (xxx 123) (yyy "zzz")))))

(test-equal run-input1
            (term ((path-to-executable "foo")
                   (cmdline-args "-aaa" 321 "-yyy" "zzz")
                   (env-var-args ("xxx" 123))
                   (timeout-sec 200)
                   (cwd ""))))

(define-metafunction Flexibench
  Runs-of-rows : ((key run-input-key) ...) val -> (run-input ...)
  [(Runs-of-rows ((key run-input-key) ...) (row ...))
   ((Run-input ((key run-input-key) ...) row) ...)])

(define-metafunction Flexibench
  Make-env-var : string atom -> string
  [(Make-env-var string atom)
   ,(string-join (term (string "=" (String-of-atom atom))) "")])

(define-metafunction Flexibench
  String-of-run-input : run-input -> string
  [(String-of-run-input ((path-to-executable string_pte)
                         (cmdline-args atom_cmdline ...)
                         (env-var-args (string_envvar atom_envval) ...)
                         (timeout-sec number_timeout)
                         (cwd string_cwd)))
   ,(string-append (term string_cwd2) (term string_env) (term string_pte) (term string_cmdline2))
   (where/error string_cwd2 ,(if (equal? (term string_cwd) "") "" (string-append "cd " (term string_cwd) "; ")))
   (where/error (string_cmdline ...) ((String-of-atom atom_cmdline) ...))
   (where/error string_cmdline2 ,(string-join (term (string_cmdline ...)) " " #:before-first " "))
   (where/error (string_envs ...) ((Make-env-var string_envvar atom_envval) ...))
   (where/error string_env ,(if (null? (term (string_envs ...)))
                                ""
                                (string-append (string-join (term (string_envs ...)) " ") " ")))])

(test-equal (term (String-of-run-input ,run-input1))
            "xxx=123 foo -aaa 321 -yyy zzz")

(define-metafunction Flexibench
  Strings-of-run-inputs : (run-input ...) -> (string ...)
  [(Strings-of-run-inputs (run-input ...)) ((String-of-run-input run-input) ...)])
  
(test-equal (term (Strings-of-run-inputs (Runs-of-rows ,run-cfg1
                                                       (((prog "foo") (aaa 321) (xxx 123) (yyy "zzz"))
                                                        ((prog "bar") (aaa 33) (xx1x 123) (yyy "z3z"))))))
            (term ("xxx=123 foo -aaa 321 -yyy zzz"
                   "bar -aaa 33 -xx1x 123 -yyy z3z")))

(define-judgment-form Flexibench
  #:mode (→ I I O O)
  #:contract (→ exp env exp env)

  [(→ exp_1 env_1 exp_3 env_2)
   ----------- "let (cong)"
   (→ (let (var_1 ... exp_1) exp_2) env_1
      (let (var_1 ... exp_3) exp_2) env_2)]

  [(where/error exp_3 (substitute exp_2 var_1 val_1))
   ----------- "let (val)"
   (→ (let (var_1 val_1) exp_2) env_1
      exp_3 env_1)]

  [(→ exp_m env_1 exp_n env_2)
   ---------------------------- "builtin (cong)"
   (→ (builtin val_b ... exp_m exp_a ...) env_1
      (builtin val_b ... exp_n exp_a ...) env_2)]

  [---------------------------- "++"
   (→ (++ val_a ...) env_1
      (Append (val_a ...)) env_1)]

  [---------------------------- "×"
   (→ (× val_c ...) env_1
      (Cross-rows (val_c ...)) env_1)]

  [(where/error (val_2 ...) (Split-rows (val_1 ...)))
   (where/error exp_4 (substitute* exp_3 ((var_1 val_2) ...)))
   ---------------------------- "÷/kcp"
   (→ (let (var_1 ... (÷/kcp val_1 ...)) exp_3) env_1
      exp_4 env_1)]

  [---------------------------- "implode"
   (→ (implode val) env_1
      ((Implode val)) env_1)]

  [---------------------------- "explode"
   (→ (explode val) env_1
      (Explode val) env_1)]

  [(→ exp_1 env_1 exp_3 env_2)
   ----------------------------------- ":= (cong)"
   (→ ((:= var exp_1) exp_2) env_1
      ((:= var exp_3) exp_2) env_2)]

  [(where/error env_2 (Merge-envs (((var val_1)) env_1)))
   ----------------------------------- ":= (val)"
   (→ ((:= var val_1) exp_2) env_1
      exp_2 env_2)]

  [(→ exp_1 env_1 exp_2 env_2)
   ----------- "run (cong)"
   (→ (run run-id exp_1) env_1
      (run run-id exp_2) env_2)]

  [(where (row_1 ...) val_1)
   (where/error (val_r ...) ((Cross-rows ((row_1) (Output-of-run row_1))) ...))
   (where/error val (Append (val_r ...)))
   ----------- "run (val)"
   (→ (run run-id val_1) env_1
      val env_1)]

  )

(define ⇒2
  (reduction-relation Flexibench
                      (--> (exp_1 env_1)
                           (exp_2 env_2)
                           (judgment-holds (→ exp_1 env_1 exp_2 env_2)))))

(define ⇒
  (reduction-relation Flexibench
                      #:domain exp-env
                      (-~> (let (var_1 val_1) exp_2) exp_3
                           (where/error exp_3 (substitute exp_2 var_1 val_1))
                           "let")
                      (-~> (++ val_a ...) (Append (val_a ...))
                           "++")
                      (-~> (× val_a ...) (Cross-rows (val_a ...))
                           "×")
                      (-~> (let (var_1 ... (÷/kcp val_1 ...)) exp_3) exp_4
                           (where/error (val_2 ...) (Split-rows (val_1 ...)))
                           (where/error exp_4 (substitute* exp_3 ((var_1 val_2) ...)))
                           "÷/kcp")
                      (-~> (implode val_a) ((Implode val_a))
                           "implode")
                      (-~> (explode val_a) (Explode val_a)
                           "explode")
                      (--> ((in-hole E ((:= var val_1) exp_2)) env_1)
                           ((in-hole E exp_2) env_1)
                           (where/error env_2 (Merge-envs (((var val_1)) env_1)))
                           ":=")
                      (-~> (run run-id (row_1 ...)) val
                           (where/error (val_r ...) ((Cross-rows ((row_1) (Output-of-run row_1))) ...))
                           (where/error val (Append (val_r ...)))
                           "run")
                      with
                      [(--> (from env_1) (to env_1)) (-+> from to)]
                      [(-+> (in-hole E_1 from) (in-hole E_1 to)) (-~> from to)]))

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
   ---------------------------- "++"
   (⇓ (++ exp_1 exp_2) val env)]

  [(⇓ exp_1 (row_1 ...) env_1)
   (⇓ exp_2 (row_2 ...) env_2)
   (where/error val (Cross-rows-2 (row_1 ...) (row_2 ...)))
   (where/error env (Merge-envs (env_1 env_2)))
   ---------------------------- "cross"
   (⇓ (× exp_1 exp_2) val env)]
  
  [(⇓ exp_1 val_1 env_1) ...
   (where/error (val_2 ...) (Split-rows (val_1 ...)))
   (where/error exp_4 (substitute* exp_2 ((var_1 val_2) ...)))
   (⇓ exp_4 val env_2)
   (where/error env (Merge-envs (env_1 ... env_2)))
   ---------------------------- "÷/kcp"
   (⇓ (let (var_1 ... (÷/kcp exp_1 ...)) exp_2) val env)]

  [(⇓ exp_1 val_1 env_1)
   (where/error row (Implode val_1))
   (where/error val (row))
   ----------- "implode"
   (⇓ (implode exp_1) val env_1)]

  [(⇓ exp_1 val_1 env_1)
   (where/error val (Explode val_1))
   ----------- "explode"
   (⇓ (explode exp_1) val env_1)]

  [(⇓ exp_1 (row_1 ...) env_1)
   (where/error (val_r ...) ((Cross-rows-2 (row_1) (Output-of-run row_1)) ...))
   (where/error ((row_r ...) ...) (val_r ...))
   (where/error val (row_r ... ...))
   ----------- "run"
   (⇓ (run run-id exp_1) val env_1)]
  
  [(⇓ exp_a val_a env_1)
   (⇓ exp_2 val env_2)
   (where/error env_3 (Merge-envs (env_1 env_2)))
   (where/error env_4 (Merge-envs (((var val_a)) env_3)))
   ----------------------------------- ":="
   (⇓ ((:= var exp_a) exp_2) val env_4)]
  
  )

(test-equal (judgment-holds (Wf-val (()))) #t)
(test-equal (judgment-holds (Wf-val (((x 1)) ()))) #f)
(test-equal (judgment-holds (Wf-val (((x 1)) ((y 1))))) #t)

(test-equal (judgment-holds (Wf-val ,exp1)) #t)
(test-equal (judgment-holds (Wf-val ,exp2)) #t)

(test-equal
 (judgment-holds
  (⇓ (implode (((x 1)) ((x 2)) ((x 3)))) val env) val)
 (term ((((x (1 2 3)))))))

(test-equal
 (judgment-holds
  (⇓ (explode (((x (1 2 3))))) val env) val)
 (term ((((x 1)) ((x 2)) ((x 3))))))

(test-equal
 (judgment-holds
  (⇓ (implode (explode (((x (1 2 3)))))) val env) val)
 (term ((((x (1 2 3)))))))

(test-equal
 (judgment-holds
  (⇓ (++ ,exp1 ,exp2) val env) val)
 (term ((((x 1) (y 2))
         ((x 2) (y 4))
         ((x 3) (y 6))
         ((z "a"))
         ((z "b"))))))

(test-equal
 (judgment-holds
  (⇓ (× ,exp1 ,exp2) val env) val)
 (term ((((x 1) (y 2) (z "a"))
         ((x 2) (y 4) (z "a"))
         ((x 3) (y 6) (z "a"))
         ((x 1) (y 2) (z "b"))
         ((x 2) (y 4) (z "b"))
         ((x 3) (y 6) (z "b"))))))

(test-equal
 (judgment-holds
  (⇓ (let (var-t var-d (÷/kcp ,exp1 (((y 4))))) var-t) val env) val)
 (term ((((x 2) (y 4))))))

(test-equal
 (judgment-holds
  (⇓ (let (var-t var-d (÷/kcp ,exp1 (((y 4))))) var-d) val env) val)
 (term ((((x 1) (y 2)) ((x 3) (y 6))))))

(test-equal
 (judgment-holds
  (⇓ (let (var-t var-d (÷/kcp ,exp1 ())) var-d) val env) val)
 (term ((((x 1) (y 2)) ((x 2) (y 4)) ((x 3) (y 6))))))

(test-equal
 (judgment-holds
  (⇓ (let (var-t var-d (÷/kcp ,exp1 ())) var-t) val env) val)
 (term (())))

(test-equal
 (judgment-holds
  (⇓ (let (var-t var-d (÷/kcp ,exp1 (()))) var-t) val env) val)
 (term (,exp1)))

(test-equal
 (judgment-holds
  (⇓ (let (x ,exp1)
       (× x ,exp2)) val env) val)
 (term ((((x 1) (y 2) (z "a"))
         ((x 2) (y 4) (z "a"))
         ((x 3) (y 6) (z "a"))
         ((x 1) (y 2) (z "b"))
         ((x 2) (y 4) (z "b"))
         ((x 3) (y 6) (z "b"))))))

(define exp-speedup
  (term
   (++
    (((prog "foo_seq")))
    (× (((prog "foo_par")))
       (explode (((proc (1 2 4)))))))))
  
(test-equal
 (judgment-holds
  (⇓ ,exp-speedup val env) val)
 (term ((((prog "foo_seq"))
         ((prog "foo_par") (proc 1))
         ((prog "foo_par") (proc 2))
         ((prog "foo_par") (proc 4))))))

(test-equal
 (judgment-holds
  (⇓ (run r ,exp-speedup) val env) val)
 (term ((((prog "foo_seq") (exectime 2665))
         ((prog "foo_seq") (exectime 2279))
         ((prog "foo_par") (proc 1) (exectime 2291))
         ((prog "foo_par") (proc 1) (exectime 2222))
         ((prog "foo_par") (proc 2) (exectime 2625))
         ((prog "foo_par") (proc 2) (exectime 2728))
         ((prog "foo_par") (proc 4) (exectime 3122))
         ((prog "foo_par") (proc 4) (exectime 2574))))))

(define exp-speedup2
  (term (let (par seq (÷/kcp (run r ,exp-speedup)
                             (((prog "foo_par")))))
          ((:= out-seq (implode seq))
           (implode par)))))

(test-equal
 (judgment-holds (⇓ ,exp-speedup2 val env) (val env))
 (term (((((prog
            ("foo_par"
             "foo_par"
             "foo_par"
             "foo_par"
             "foo_par"
             "foo_par"))
           (proc (1 1 2 2 4 4))
           (exectime (2291 2222 2625 2728 3122 2574))))
         ((out-seq
           (((prog ("foo_seq" "foo_seq"))
             (exectime (2665 2279))))))))))

;(stepper ⇒ (term (,exp-speedup2 ())))

(define (group xs n)
  (if (null? xs)
      '()
      (let ([xs1 (take xs n)]
            [xs2 (drop xs n)])
        (cons xs1 (group xs2 n)))))

(define-metafunction Flexibench
  Calculate-result : ((var (atom ...)) ...) var natural -> ((atom ...) ...)
  [(Calculate-result (_ ... (var (atom ...)) _ ...) var natural)
   ,(group (term (atom ...)) (term natural))])

(define-metafunction Flexibench
  Get-ys : ((var (atom ...)) ...) var natural -> (atom ...)
  [(Get-ys ((var (atom ...)) ...) var_t natural)
   ,(map car (term (Calculate-result ((var (atom ...)) ...) var_t natural)))])

(define-metafunction Flexibench
  Get-mean-of-ys : ((var (atom ...)) ...) var natural -> (atom ...)
  [(Get-mean-of-ys ((var (atom ...)) ...) var_t natural)
   ,(map mean (term (Calculate-result ((var (atom ...)) ...) var_t natural)))])

(define-metafunction Flexibench
  Get-speedup : env var var natural -> ((natural ...) (number ...))
  [(Get-speedup env var_par var_seq natural)
   ((natural_proc ...) (number_parexec ...))
   (where/error (natural_proc ...) (Get-ys ,(car (term (Lookup env var_par))) proc natural))
   (where/error (number_parexec ...) (Get-mean-of-ys ,(car (term (Lookup env var_par)))  exectime natural))
   (where/error (number_seqexec ...) (Get-mean-of-ys ,(car (term (Lookup env var_seq)))  exectime natural))])

(define-metafunction Flexibench
  Extend-env : env var val -> env
  [(Extend-env ((var val) ...) var_e val_e)
   ((var val) ... (var_e val_e))])

(let ([results (car (judgment-holds (⇓ ,exp-speedup2 val env) (val env)))])
  (let ([env (term (Extend-env ,(cadr results) out-par ,(car results)))])
    (let ([results2 (term (Get-speedup ,env out-par out-seq 2))])
      (printf "proc: ~a~n" (car results2))
      (printf "speedup: ~a~n" (cadr results2))
      (plot (lines (map vector (car results2) (cadr results2)))))))