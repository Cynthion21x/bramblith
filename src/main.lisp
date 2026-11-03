(in-package :bramblith)

(defun main ()
  (setf *app-root* nil)
  (init-raylib)
  (start)
  (load-glyphs)
  (register-shades)
  (wait-for-window))
