; immutable realization of AVL trees for >= R5RS implementations 
; mostly copy-paste from http://swizard.info/articles/functional-data-structures.html#link12

; srfi instead of non-standard (define-record)
(use srfi-9) ; record types

(define-record-type avl-tree (make-avl-tree root less? equ?) avl-tree?
 (root avl-root)
 (less? avl-less)
 (equ? avl-equ)
)
(define-record-type avl-node (make-avl-node key value l-child r-child l-depth r-depth) 
 avl-node?
 (key avl-key)
 (value avl-value)
 (l-child avl-l-child) (r-child avl-r-child)
 (l-depth avl-l-depth) (r-depth avl-r-depth)
)
(define (with-avl-tree tree func) (
 func (avl-root tree) (avl-less tree) (avl-equ tree)
))
(define (with-avl-node node func) (
 func (avl-key node) (avl-value node) (avl-l-child node) (avl-r-child node) 
 (avl-l-depth node) (avl-r-depth node)
))

(define (make-empty-avl-tree less? equ?)
 (make-avl-tree '() less? equ?))


(define (:make-left node child depth)
 (with-avl-node node
         (lambda (k v lc rc ld rd) (make-avl-node k v child rc depth rd))))
(define (:make-right node child depth)
 (with-avl-node node
         (lambda (k v lc rc ld rd) (make-avl-node k v lc child ld depth))))


(define (calc-depth node)
 (with-avl-node node (lambda (k v lc rc ld rd) (+ (max ld rd) 1))))


(define (:avl-subtree-insert-node make-proc child node . args)
 (let ((new-child (apply :avl-tree-insert-node child args)))
  ; add new child, compose subtree
  (make-proc node new-child (calc-depth new-child))))

(define :rotate-right-set (list :make-right
                avl-l-child
                :make-left
                avl-r-child
                avl-r-depth))
(define :rotate-left-set  (list :make-left
                avl-r-child
                :make-right
                avl-l-child
                avl-l-depth))
(define (:avl-tree-rotate node make-a get-child-a make-b get-child-b get-depth-b)
 (let* ((child-a (get-child-a node))
     (child-b (get-child-b child-a))
     (depth-b (get-depth-b child-a)))
  ; new-child = foredad -> child ~ dad dir
  ; new tree = dad -> new-child, modify dad depth
  (let ((new-node (make-b node child-b depth-b)))
   (make-a child-a new-node (calc-depth new-node)))))
(define (:avl-tree-check-rotate node)
 (with-avl-node node
         (lambda (k v lc rc ld rd) (cond ((and (> ld rd)
               (> (- ld rd) 1))
            (apply :avl-tree-rotate node :rotate-right-set))
            ((and (> rd ld)
               (> (- rd ld) 1))
            (apply :avl-tree-rotate node :rotate-left-set))
            (else node)))))

(define (:avl-tree-insert-node node ckey value-proc less? equ?) (
 if (not (avl-node? node))
 (make-avl-node ckey (value-proc '()) '() '() 0 0)
  (:avl-tree-check-rotate
  (let ((args (list node ckey value-proc less? equ?)))
    (with-avl-node node (lambda (k v lc rc ld rd) (
              cond ((equ? ckey k)
                 (make-avl-node k (value-proc v) lc rc ld rd)) 
                ((less? ckey k)
                 (apply :avl-subtree-insert-node
                     :make-left lc args))
                 (else
                 (apply :avl-subtree-insert-node
                     :make-right rc args)))))
  ))
))


(define (avl-tree-replace tree key value-proc)
 (with-avl-tree tree
         (lambda (r l e) (make-avl-tree
         (:avl-tree-insert-node r key value-proc l e)
         l
         e))))
(define (avl-tree-insert tree key value)
 (avl-tree-replace tree key (lambda (x) value)))
(define (avl-tree-lookup tree ckey) (
 let ((equ? (avl-equ tree))
   (less? (avl-less tree)))
 (define (lookup node) (
  if (not (avl-node? node))
  #f
  (with-avl-node node (lambda (k v lc rc ld rd) (
   cond ((equ? ckey k) v)
     ((less? ckey k) (lookup lc))
     (else (lookup rc)))))
 ))
 (lookup (avl-root tree))
))

(define (avl-tree-fold tree proc acc)
 (define (tree-fold node acc) (
 if (not (avl-node? node))
  acc
  (with-avl-node node (lambda (k v lc rc ld rd) (
  let* ((l-val (tree-fold lc acc))
      (c-val (proc k v l-val))
      (r-val (tree-fold rc c-val))) r-val)))
 ))
 (tree-fold (avl-root tree) acc)
)
(define (avl-tree-max-value tree)
 (avl-tree-fold tree (lambda (k v a) (max v a)) 0))
(define (avl-tree-remove-key tree key)
 (avl-tree-fold tree (lambda (k v t) (
  if (not (eq? k key)) (avl-tree-insert t k v) t))
         (make-empty-avl-tree (avl-less tree)
                   (avl-equ tree))))
(define (avl-tree-print-flatten tree)
 (avl-tree-fold tree (lambda (k v a) (display k) (display " ")) 0)
)