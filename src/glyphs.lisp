(in-package :bramblith)

(defvar *glyph-index* (make-hash-table :test 'eq))

(defun glyph (id)
  (or (gethash id *glyph-index*) (error "Unknown glyph ~s" id)))

(defun glyph-names ()
  (sort (loop for k being the hash-keys of *glyph-index* collect k) #'string<))

(defun parse-glyph-name (stem)
  (let ((at (position #\@ stem)))
    (if (null at)
        (values (intern (string-upcase stem) :keyword) 0 0)
        (let* ((x (position #\x stem :start at))
               (cols (parse-integer stem :start (1+ at) :end x))
               (rows (parse-integer stem :start (1+ x))))
          (values (intern (string-upcase (subseq stem 0 at)) :keyword) cols rows)))))

(defun filter-code (filter) (ecase filter (:point 0) (:bilinear 1) (:trilinear 2)))

(defun glyph-files (dir)
  (sort (directory (merge-pathnames "*.png" dir)) #'string< :key #'namestring))

(defun load-glyph-folder (dir &key (shaded t) (filter :point) mipmaps)
  (let ((files (glyph-files dir)) (found '()))
    (when files
      (%glyph-batch-begin (if shaded 1 0))
      (dolist (file files)
        (multiple-value-bind (id cols rows) (parse-glyph-name (pathname-name file))
          (when (or (gethash id *glyph-index*) (assoc id found))
            (error "Duplicate glyph name ~s (~a)" id file))
          (let ((idx (%glyph-batch-add (sb-ext:native-namestring file) cols rows)))
            (when (minusp idx)
              (error "Could not load glyph image ~a (see the log lines above)" file))
            (push (cons id idx) found))))
      (when (minusp (%glyph-batch-pack (filter-code filter) (if mipmaps 1 0)))
        (error "Could not pack glyphs from ~a" dir))
      (loop for (id . idx) in found do (setf (gethash id *glyph-index*) idx)))
    (length files)))

(defun load-glyphs ()
  (with-render
    (shutdown-glyphs)
    (clrhash *glyph-index*)
    (let ((base (load-glyph-folder (app-path "assets/glyphs/")))
          (art  (load-glyph-folder (app-path "assets/art/") :shaded nil :filter :bilinear :mipmaps t))
	  (text (load-glyph-folder (app-path "assets/art/") :filter :bilinear)))
      (format t "~&[glyphs] ~d base, ~d art~%" base art)
      t)))

(defvar *shade-defs* '())
(defvar *shade-index* (make-hash-table :test 'eq))

(defmacro defshade (name &rest colours)
  `(progn (setf *shade-defs* (append (remove ',name *shade-defs* :key #'car)
                                     (list (cons ',name ',colours))))
          ',name))

(defun register-shades ()
  (with-render
    (dolist (def *shade-defs*)
      (destructuring-bind (name . colours) def
        (let ((s (or (gethash name *shade-index*)
                     (let ((new (%shade-new (length colours))))
                       (when (minusp new)
                         (error "Could not create shade ~s (more than 16 colours, or too many shades)" name))
                       (setf (gethash name *shade-index*) new)))))
          (loop for (r g b) in colours
                for rank from 0
                do (%shade-set-color s rank (rgba r g b))))))
    t))

(defun shade (name)
  (or (gethash name *shade-index*) (error "Unknown shade ~s" name)))
