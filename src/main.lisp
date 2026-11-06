(in-package :bramblith)

(defun main ()
  (setf *app-root* nil)
  (init-raylib)
  (start)
  
  ;; probably some loading screen here
  ;; for the asset loading shenanigans
  
  (load-glyphs)
  (register-shades)

  ;; main loop

  (new-game)
  (setf *update* #'main-menu)
  (wait-for-window))

(defun repl-setup ()
  (init-raylib)
  (start)
  (load-glyphs)
  (register-shades)
  (new-game))
