(defpackage :rl-ffi
  (:use :cl :sb-alien))

(defpackage :bramblith
  (:use :cl :rl-ffi)
  (:export #:main))
