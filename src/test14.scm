(define b1 (img_open "in/boo.jpg"))
(define b2 (img_open "in/boo.jpg"))
(img_bw b1 50)
(img_bw b2 50)

(define matrix '(0 1 0 1 1 1 0 1 0))

(img_binerode b2 matrix)
(img_binsub b1 b2)
(img_save b1 "out/morphedge.jpg")
