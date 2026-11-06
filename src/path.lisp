(in-package :bramblith)

(defvar *release-p* nil)
(defvar *app-root* nil)
(defvar *raylib-loaded* nil)

(defun directory-of (file)
  (make-pathname :name nil :type nil :version nil :defaults file))

(defun find-app-root ()
  (let ((override (sb-ext:posix-getenv "BRAMBLITH_HOME")))
    (cond ((and override (plusp (length override)))
           (sb-ext:parse-native-namestring override nil *default-pathname-defaults*
					   :as-directory t))
          (*release-p*
           (directory-of (sb-ext:parse-native-namestring sb-ext:*runtime-pathname*)))
          (t (asdf:system-source-directory :bramblith)))))

(defun app-root ()
  (or *app-root* (setf *app-root* (find-app-root))))

(defun app-path (relative)
  (merge-pathnames relative (app-root)))

(defun init-raylib ()
  (unless *raylib-loaded*
    (let ((lib (app-path "libraylib-modified.so.6.2.0")))
      (unless (probe-file lib)
        (error "Cannot find ~a (app root is ~a)" lib (app-root)))
      (sb-alien:load-shared-object lib :dont-save t)
      (setf *raylib-loaded* t)))
  t)

