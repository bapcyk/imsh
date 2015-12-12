(load "imsh.scm")
(define b (img_open "in/ngirl.jpg"))
(img_pix b 10 1)
(img_save b "out/pix1.jpg")

(define b (img_open "in/ngirl.jpg"))
(img_pix b 5 0)
(img_save b "out/pix2.jpg")
