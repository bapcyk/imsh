(load "imsh.scm")
(define b (img_open "in/ngirl.jpg"))
(img_conv b emboss-matrix emboss-div emboss-shift)
(img_save b "out/emboss.jpg")
