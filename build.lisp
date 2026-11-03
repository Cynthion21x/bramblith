(require :asdf)

(asdf:load-asd (truename "bramblith.asd"))
(asdf:load-system "bramblith")

(ensure-directories-exist "build/")

(sb-ext:save-lisp-and-die
 "build/bramblith"
 :toplevel #'bramblith:main
 :executable t
 :compression t)
