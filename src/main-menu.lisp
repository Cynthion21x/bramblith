(in-package :bramblith)

(defstruct menu)

(defun main-menu (input dt)

  (draw-box 0 0 +cols+ +rows+ :white 0)

  (draw-box 5 5 20 20 :white 0)

  (put-string 6 5 "[Menu]" (shade :white) 0)
  
  )
