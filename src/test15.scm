(load "imsh.scm")

(define i (img_open "in/stalin.jpg"))
(img_gendith i sierra-dith-matrix)
(img_save i "out/dithgen_sierra.jpg")

(define i (img_open "in/stalin.jpg"))
(img_gendith i sierra2-dith-matrix)
(img_save i "out/dithgen_sierra2.jpg")

(define i (img_open "in/stalin.jpg"))
(img_gendith i sierralow-dith-matrix)
(img_save i "out/dithgen_sierralow.jpg")

(define i (img_open "in/stalin.jpg"))
(img_gendith i floyd-dith-matrix)
(img_save i "out/dithgen_floyd.jpg")

(define i (img_open "in/stalin.jpg"))
(img_gendith i falsefloyd-dith-matrix)
(img_save i "out/dithgen_falsefloyd.jpg")

(define i (img_open "in/stalin.jpg"))
(img_gendith i jarvis-dith-matrix)
(img_save i "out/dithgen_jarvis.jpg")

(define i (img_open "in/stalin.jpg"))
(img_gendith i stucki-dith-matrix)
(img_save i "out/dithgen_stucki.jpg")

(define i (img_open "in/stalin.jpg"))
(img_gendith i atkinson-dith-matrix)
(img_save i "out/dithgen_atkinson.jpg")

(define i (img_open "in/stalin.jpg"))
(img_gendith i burkes-dith-matrix)
(img_save i "out/dithgen_burkes.jpg")

(define i (img_open "in/stalin.jpg"))
(img_dith i 0)
(img_save i "out/dith_atkinson.jpg")

(define i (img_open "in/stalin.jpg"))
(img_dith i 1)
(img_save i "out/dith_floyd.jpg")

(define i (img_open "in/stalin.jpg"))
(img_dith i 2)
(img_save i "out/dith_ordered.jpg")

(define i (img_open "in/stalin.jpg"))
(img_dith i 3)
(img_save i "out/dith_random.jpg")
