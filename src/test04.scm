(load "imsh.scm")
(define b (img_open "in/ngirl.jpg"))
(img_conv b edge-matrix 3 60)
(img_save b "out/edge.jpg")
