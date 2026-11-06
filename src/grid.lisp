(in-package :bramblith)

(defconstant +cols+ 80)
(defconstant +rows+ 30)

;; Two words per cell:
;;   word 0: glyph index (low 16 bits) | shade number (high 16 bits)
;;   word 1: background RGBA (alpha 0 = no background drawn)
(declaim (type (simple-array (unsigned-byte 32) (*)) *cells*))
(defvar *cells* (make-array (* +cols+ +rows+ 2) :element-type '(unsigned-byte 32)))

(defun clear-cells () (fill *cells* 0) nil)

(declaim (inline set-cell))
(defun set-cell (x y glyph shade bg)
  (declare (type fixnum x y)
           (type (unsigned-byte 16) glyph shade)
           (type (unsigned-byte 32) bg))
  (when (and (< -1 x +cols+) (< -1 y +rows+))
    (let ((i (* 2 (+ x (* y +cols+)))))
      (setf (aref *cells* i) (logior glyph (ash shade 16))
            (aref *cells* (+ i 1)) bg)))
  nil)

(defun cell-size (w h)
  (let* ((ch (min (/ h +rows+) (* 3/2 (/ w +cols+))))
         (cw (* ch 2/3)))
    (values (float cw 1f0) (float ch 1f0)
            (float (/ (- w (* cw +cols+)) 2) 1f0)
            (float (/ (- h (* ch +rows+)) 2) 1f0))))

(declaim (inline put-char))
(defun put-char (x y ch shade bg)
  (check-type ch character)
  (set-cell x y (char-glyph ch) shade bg))

(declaim (inline put-string))
(defun put-string (x y string shade bg)
  (loop for ch across string
        for cx from x
        do (put-char cx y ch shade bg)))

(defun draw-grid ()
  (multiple-value-bind (cw ch ox oy)
      (cell-size (get-screen-width) (get-screen-height))
    (sb-sys:with-pinned-objects (*cells*)
      (%draw-glyph-grid (sb-sys:vector-sap *cells*) +cols+ +rows+ cw ch ox oy))))
