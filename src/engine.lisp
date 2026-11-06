(in-package :bramblith)

(defvar *render-thread* nil)
(defvar *running* nil)
(defvar *jobs* '())
(defvar *job-lock* (sb-thread:make-mutex :name "render jobs"))

(defvar *frame-hook*
  (lambda ()
    (%draw-rectangle 40 40 240 80 (rgba 90 140 70))))

(defun on-render-thread (fn)
  (sb-thread:with-mutex (*job-lock*) (push fn *jobs*))
  nil)

(defun call-on-render-thread (fn)
  (unless *running* (error "The render thread is not running!"))
  (if (eq sb-thread:*current-thread* *render-thread*)
      (funcall fn)
      (let ((result nil) (err nil) (done (sb-thread:make-semaphore)))
        (on-render-thread
         (lambda ()
           (handler-case (setf result (multiple-value-list (funcall fn)))
             (sb-sys:memory-fault-error (e) (setf err e *running* nil))
             (error (e) (setf err e)))
           (sb-thread:signal-semaphore done)))
        (unless (sb-thread:wait-on-semaphore done :timeout 5)
          (error "The render thread did not respond"))
        (when err (error err))
        (values-list result))))

(defmacro with-render (&body body)
  `(call-on-render-thread (lambda () ,@body)))

(defun run-jobs ()
  (let ((todo (sb-thread:with-mutex (*job-lock*)
                (prog1 (reverse *jobs*) (setf *jobs* '())))))
    (dolist (fn todo)
      (handler-case (funcall fn)
        (sb-sys:memory-fault-error (e)
          (format t "~&[FATAL: memory fault in a job] ~a~%" e)
          (setf *running* nil))
        (error (e) (format t "~&[job error] ~a~%" e))))))

(defun run-hook ()
  (handler-case (funcall *frame-hook*)
    (sb-sys:memory-fault-error (e)
      (format t "~&[FATAL: memory fault inside foreign code] ~a~%" e)
      (setf *running* nil))
    (error (e)
      (format t "~&[frame hook error] ~a~%[hook disabled]~%" e)
      (setf *frame-hook* (lambda ())))))

(defun run-window (width height title)
  (sb-int:with-float-traps-masked
      (:overflow :invalid :divide-by-zero :inexact)
    (set-config-flags 4)
    (init-window width height title)
    (set-target-fps 60)
    (unwind-protect
	 (loop while (and *running* (zerop (%window-should-close)))
               do (run-jobs)
		  (begin-drawing)
                  (%clear-background *c-background*)
                  (run-hook)
                  (end-drawing))
      (shutdown-handles)
      (close-window)
      (setf *running* nil))))

(defun start (&key (width 1280) (height 720) (title "Bramblith"))
  (when *running* (error "Already running!"))
  (setf *running* t
        *render-thread*
        (sb-thread:make-thread
         (lambda ()
           (handler-case (run-window width height title)
             (error (e)
               (format t "~&[render thread error] ~a~%" e)
               (setf *running* nil))))
         :name "render"))
  :started)

(defun stop ()
  (setf *running* nil)
  (when *render-thread*
    (sb-thread:join-thread *render-thread* :default nil)
    (setf *render-thread* nil))
  :stopped)

(defun wait-for-window ()
  (when *render-thread*
    (sb-thread:join-thread *render-thread* :default nil)))
