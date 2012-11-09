(require-extension bind)
(bind* "double cpu_burn(double);")

(use srfi-1) ; filter

(define proc
	(foreign-lambda double "cpu_burn" double))

(define (get-cc lst) (caar lst))
(define (get-name elm) (cadr elm))
(define (get-val elm) (cddr elm))
(define rest cdr)
(define (this-cc)
  (call/cc (lambda (cc) (cc cc))))

(define *tq* '())
(define *stop* '())

(define (add-task ts)
	(let ((cc (this-cc)))
		(if (procedure? cc)
			(set! *tq* (append *tq* `(,(cons cc (cons #f #f)))))
			(ts)
			)))
(define (start-tasks)
	(let ((cc (this-cc)))
		(if cc
			(begin
				(set! *stop* (lambda () (cc #f))
				((get-cc *tq*) '())))))
(define (switch-task name val)
	(let ((cc (this-cc)))
		(if (procedure? cc)
			(begin
			(set! *tq* (append (rest *tq*) `(,(cons cc (cons name val)))))
			((get-cc *tq*) '())))))

(define (doit end? a . x)
	(set! *tq* '())

	(define (do-proc x)
		(let ((func (lambda ()
			(let loop ((lim end?) (acc (proc x)))
			(if (eq? lim 1)
				(switch-task x acc)
					(begin
						(switch-task x #f)
						(loop (- lim 1) (+ acc (proc x)))))))))
		func))
	(define (supervisor)
		(let ((func (lambda ()
			(let loop ((seconds '()))
				(let ((lst (filter (lambda (x) (not (eq? (get-name x) #f))) *tq*)))
					(let ((secs (modulo (current-seconds) 4)))
						(if (and (not (equal? secs seconds)) (equal? secs 0.0))
								(for-each (lambda (x) (print "task "(get-name x)" "(if (get-val x) 'ready 'busy))) lst))
					(if (null? (filter (lambda (x) (not (get-val x))) lst))
						(begin
							(print "done: " (map (lambda (x) (cons (get-name x) (get-val x))) lst))
							(*stop*))
						(begin 
							(switch-task #f #f)
							(loop secs)))))))))
		func))

	(for-each (lambda (x) (add-task (do-proc x))) (cons a x))
	(add-task (supervisor))
	(start-tasks)
)

(doit 11000 0.1 0.2)
(doit 13000 0.3233)
(doit 10000 1.4444 1.222 0.22)