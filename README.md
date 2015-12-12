# imsh

## Images processing shell

This is the educational project for image processing with implementation of
several [algorithms](#algo) in C for MinGW on Win32. All algorithms are scriptable in SIOD Scheme,
so main program - imsh - is the SIOD Scheme shell with C algorithms in backend.

To build use command:

`make imsh`

to build and test:

`make test`

which runs test1.scm, test2.scm, ...

or only one test:

`make test4`

Each test is Scheme script, for example:

```scheme
(load "imsh.scm")
(define b (bmp_open "ngirl.bmp"))
(bmp_conv b emboss-matrix emboss-div emboss-shift)
(bmp_save b "out/emboss.bmp")
```

## <a name="algo"></a>Algorithms

* Black and White
* Greyscale
* Negative
* Convolution Matrix
* Channel Mask
* Greyscale Approximation
* Pixalization
* Median
* Dilate
* Erode
* Set (binary image) operations
* Morphological edge detection
* Dithering 

## Credits

Imsh uses FreeImage library: http://freeimage.sourceforge.net

Also Imsh uses SIOD Scheme: http://people.delphiforums.com/gjc/siod.html

## Another repo

Original code is: http://chiselapp.com/user/p4v31/repository/imsh/home
