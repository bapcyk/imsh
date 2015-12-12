;;-*-mode:lisp-*-
;; Name:    find-files.scm
;; Purpose: create lists of file directory trees,
;;          with status information and md5 checksums.
;;  
;; $Id: find-files.scm,v 1.3 1998/10/02 18:57:52 gjcarret Exp $

(cond ((eq? 'win32 (os-classification))
       (define file-status stat)
       (define (opendir-arg x) (string-append x "/*")))
      ('else
       (define file-status lstat)
       (define (opendir-arg x) x)))

(define (list-files dir)
  (let ((s nil)
        (result nil)
	(f nil)
	(e nil))
    (set! e (*catch 'errobj
		    (set! s (opendir (opendir-arg dir)))
		    (while (set! f (readdir s))
		      (set! result (cons f result)))))
    (if (and (pair? e) (string? (car e)))
	(writes nil "** error in " dir "\n"))
    (if s (closedir s))
    (nreverse result)))

(define (find-files start)
  (find-files-filter start nil))

(define (find-files-filter start fcn)
  (let ((l (list-files start))
	(result nil)
	(f nil)
	(s nil))
    (while l
      (cond ((not (equal? (car l) ".."))
	     (set! f (string-append start "/" (car l)))
	     (set! s (file-status f))
	     (cond ((or (not fcn) (fcn f s))
		    (set! result (cons (cons f s) result))
		    (if (and (memq 'DIR (assq 'mode s))
			     (not (equal? (car l) ".")))
			(set! result (nconc (nreverse
					     (find-files-filter f fcn))
					    result)))))))
      (set! l (cdr l)))
    (nreverse result)))

(define (get-file-info l md5?)
  ;; used on the result of a find-files.
  (let ((mode (assq 'mode l)))
    (cond ((memq 'LNK mode)
	   (let ((result (*catch 'errobj (readlink (car l)))))
	     (if (string? result)
		 (set-cdr! l
			   (cons (cons 'readlink result)
				 (cdr l))))))
	  ((and md5? (memq 'REG mode))
	   (let ((f nil)
		 (c (md5-init))
		 (b (cons-array 4096 'string)))
	     (*catch 'errobj
		     (begin (set! f (fopen (car l) "rb"))
			    (md5-update c b f)
			    ;;(writes nil "New md5 " (car l) "\n")
			    (set-cdr! l
				      (cons (cons 'md5
						  (array->hexstr
						   (md5-final c)))
					    (cdr l)))))
	     (and f (fclose f))))))
  l)

(define (get-file-info-cache l md5-cache md5?)
  (let ((o (and md5? md5-cache
		(href md5-cache (car l)))))
    (cond ((and o
		(memq 'REG (assq 'mode l))
		(equal? (cdr (assq 'dev l)) (cdr (assq 'dev o)))
		(equal? (cdr (assq 'ino l)) (cdr (assq 'ino o)))
		(equal? (cdr (assq 'uid l)) (cdr (assq 'uid o)))
		(equal? (cdr (assq 'gid l)) (cdr (assq 'gid o)))
		(equal? (cdr (assq 'size l)) (cdr (assq 'size o)))
		(equal? (cdr (assq 'mtime l)) (cdr (assq 'mtime o)))
		(equal? (cdr (assq 'ctime l)) (cdr (assq 'ctime o)))
		(equal? (cdr (assq 'gen l)) (cdr (assq 'gen o))))
	   (set-cdr! l (cons (assq 'md5 o) (cdr l)))
	   l)
	  ('else
	   (get-file-info l md5?)))))

(define (get-directory-snapshot dir md5-cache md5? only-dev)
  (mapcar (lambda (f) (get-file-info-cache f md5-cache md5?))
	  (find-files-filter dir
			     (if only-dev
				 (lambda (fname status)
				   (equal? only-dev (cdr (assq 'dev status))))))))

;(trace get-directory-snapshot find-files-filter list-files)

(define (snapshot save-file dir)
  (save-forms save-file (get-directory-snapshot dir nil t nil)))
    
    

(define (get-filesystem-snapshots directories
				  old-db
				  crossdev
				  md5?)
  (let ((result nil)
	(l nil)
	(s nil)
	(only-dev nil)
	(md5-cache nil))
    (cond (old-db
	   (set! md5-cache (cons-array (length old-db)))
	   (set! l old-db)
	   (while l
	     (if (assq 'md5 (car l))
		 (hset md5-cache (caar l) (cdar l)))
	     (set! l (cdr l)))))
    (set! l directories)
    (while l
      (set! s (file-status (car l)))
      (and s (not crossdev) (not only-dev)
	   (set! only-dev (cdr (assq 'dev s))))
      (cond ((not s))
	    ((and only-dev (not (equal? only-dev (cdr (assq 'dev s))))))
	    ((memq 'DIR (assq 'mode s))
	     (set! result (nconc result (get-directory-snapshot (car l)
								md5-cache
								md5?
								only-dev))))
	    ('else
	     (set! result (nconc result
				 (list (get-file-info-cache
					(cons (car l) s)
					md5?
					md5-cache))))))
      (set! l (cdr l)))
    result))

(define (test1)
  (save-forms "test1.output"
	      (get-filesystem-snapshots '("/")
					nil
					t
					nil)))

(define (test2)
  (save-forms "test2.output"
	      (get-filesystem-snapshots '("/")
					nil
					nil
					nil)))


						
			      