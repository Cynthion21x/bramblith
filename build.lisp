(require :asdf)

(asdf:load-asd (truename "bramblith.asd"))
(asdf:load-system "bramblith")

(ensure-directories-exist "build/")

(setf bramblith::*release-p* t
      bramblith::*app-root* nil
      bramblith::*raylib-loaded* nil) 

(sb-ext:save-lisp-and-die
 "build/bramblith"
 :toplevel #'bramblith:main
 :executable t
 :compression t)
