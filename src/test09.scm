(load "imsh.scm")
(define b (img_open "in/ngirl.jpg"))
(img_med b 9)
(img_save b "out/med.jpg")
