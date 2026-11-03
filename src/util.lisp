(in-package :bramblith)

(declaim (inline rgba))
(defun rgba (r g b &optional (a 255))
  (logior r (ash g 8) (ash b 16) (ash a 24)))
