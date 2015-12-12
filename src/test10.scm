(define b1 (img_open "in/foo.jpg"))
(img_bw b1 50)

(define matrix1 '(1 1 1 1 1 1 1 1 1 1 1
                  1 1 1 1 1 1 1 1 1 1 1
                  1 1 1 1 1 1 1 1 1 1 1
                  1 1 1 1 1 1 1 1 1 1 1
                  1 1 1 1 1 1 1 1 1 1 1
                  1 1 1 1 1 1 1 1 1 1 1
                  1 1 1 1 1 1 1 1 1 1 1
                  1 1 1 1 1 1 1 1 1 1 1
                  1 1 1 1 1 1 1 1 1 1 1
                  1 1 1 1 1 1 1 1 1 1 1
                  1 1 1 1 1 1 1 1 1 1 1))

(define matrix2 '(1 1 1 1 1 1 1 1 1
                  1 1 1 1 1 1 1 1 1
                  1 1 1 1 1 1 1 1 1
                  1 1 1 1 1 1 1 1 1
                  1 1 1 1 1 1 1 1 1
                  1 1 1 1 1 1 1 1 1
                  1 1 1 1 1 1 1 1 1
                  1 1 1 1 1 1 1 1 1
                  1 1 1 1 1 1 1 1 1))

(img_bindilate b1 matrix1)
(img_save b1 "out/bindilate.jpg")

(define b2 (img_open "in/boo.jpg"))
(img_bw b1 50)
(img_binerode b2 matrix2)
(img_save b2 "out/binerode.jpg")
