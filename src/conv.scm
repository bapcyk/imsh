(define blur-matrix '(1 1 1 1 1 1 1 1 1))
(define blur-div 9)
(define blur-shift 0)

(define motion-matrix
  '(1 0 0 0 0 0 0 0 0
    0 1 0 0 0 0 0 0 0
    0 0 1 0 0 0 0 0 0
    0 0 0 1 0 0 0 0 0
    0 0 0 0 1 0 0 0 0
    0 0 0 0 0 1 0 0 0
    0 0 0 0 0 0 1 0 0
    0 0 0 0 0 0 0 1 0
    0 0 0 0 0 0 0 0 1))
(define motion-div 10)
(define motion-shift 0)

(define edge-matrix '(-1 -1 -1 -1 8 -1 -1 -1 -1))
(define edge-div 1)
(define edge-shift 0)

(define edge1-matrix '(0 1 0 1 -4 1 0 1 0))
(define edge1-div 1)
(define edge1-shift 0)

(define edge2-matrix '(0 9 0 9 -36 9 0 9 0))
(define edge2-div 9)
(define edge2-shift 0)

(define sharpen-matrix '(-1 -1 -1 -1 9 -1 -1 -1 -1))
(define sharpen-div 1)
(define sharpen-shift 0)

(define dilat-matrix '(0 1 0 1 1 1 0 1 0))
(define dilat-div 1)
(define dilat-shift 0)

(define sharpen1-matrix '(0 -3 0 -3 21 -3 0 -3 0))
(define sharpen1-div 9)
(define sharpen1-shift 0)

(define sharpen2-matrix '(1 1 1 1 -7 1 1 1 1))
(define sharpen2-div 1)
(define sharpen2-shift 0)

(define lighten-matrix '(0 0 0 0 12 0 0 0 0))
(define lighten-div 9)
(define lighten-shift 0)

(define darken-matrix '(0 0 0 0 6 0 0 0 0))
(define darken-div 9)
(define darken-shift 0)

(define unsharpen-matrix '(-1 -1 -1 -1 17 -1 -1 -1 -1))
(define unsharpen-div 9)
(define unsharpen-shift 0)

(define emboss-matrix '(-2 0 0 0 1 0 0 0 2))
(define emboss-div 1)
(define emboss-shift 0)

(define emboss1-matrix '(-2 -1 0 -1 1 1 0 1 2))
(define emboss1-div 1)
(define emboss1-shift 0)

(define gauss-matrix
  '(2 4 5 4 2
    4 9 12 9 4
    5 12 15 12 5
    4 9 12 9 4
    2 4 5 4 1))
(define gauss-div 159)
(define gauss-shift 0)
