(in-package :bramblith)

;;; Loop Control
(defun new-game ()
  (setf *frame-hook* 'game-loop))

(defun compose-hooks (&rest functions)
  (lambda (&rest args)
    (dolist (function functions)
      (funcall function args))))

(defvar *update* (compose-hooks))

(defun game-loop ()
  (clear-cells)
  (funcall *update* (poll-input) 0.16)
  (draw-grid))

;;; Useful functions

(defun draw-box (x y w h c b)

  ;; Corners
  (set-cell x y (glyph :line-tl) (shade c) b)
  (set-cell x (+ y (- h 1)) (glyph :line-bl) (shade c) b)
  (set-cell (+ x (- w 1)) y (glyph :line-tr) (shade c) b)
  (set-cell (+ x (- w 1)) (+ y (- h 1)) (glyph :line-br) (shade c) b)

  ;; Top horizontal
  (loop for i from 1 below (- w 1)
        do (set-cell (+ x i) y (glyph :line-h) (shade c) b))

  ;; Bottom horizontal
  (loop for i from 1 below (- w 1)
        do (set-cell (+ x i) (+ y (- h 1)) (glyph :line-h) (shade c) b))

  ;; Left vertical
  (loop for i from 1 below (- h 1)
        do (set-cell x (+ y i) (glyph :line-v) (shade c) b))

  ;; Right vertical
  (loop for i from 1 below (- h 1)
        do (set-cell (+ x (- w 1)) (+ y i) (glyph :line-v) (shade c) b)))
