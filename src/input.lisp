(in-package :bramblith)

(defparameter *named-keys*
  '(:space 32 :escape 256 :enter 257 :tab 258 :backspace 259
    :right 262 :left 263 :down 264 :up 265 :f11 300))

(defun key-code (key)
  (etypecase key
    (integer key)
    (character (char-code (char-upcase key)))
    (keyword (or (getf *named-keys* key) (error "Unknown key ~s" key)))))

(defun key-pressed-p (key) (/= 0 (%key-pressed (key-code key))))
(defun key-down-p (key)    (/= 0 (%key-down (key-code key))))

(defparameter *keymap*
  '((:up . :north) (:down . :south) (:left . :west) (:right . :east)
    (#\k . :north) (#\j . :south) (#\h . :west) (#\l . :east)
    (#\y . :north-west) (#\u . :north-east) (#\b . :south-west) (#\n . :south-east)
    (#\. . :wait)))

(defun poll-input ()
  (loop for (key . action) in *keymap*
        when (key-pressed-p key) return action))
