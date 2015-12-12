(load "imsh.scm")
(define b (img_open "in/boo.jpg"))
(img_conv b edge-matrix 3 60)
(img_bw b 50)
(img_save b "out/booedge.jpg")

