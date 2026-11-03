(in-package :rl-ffi)

(defvar *raylib-loaded* nil)

(defun raylib-pathname ()
  (let ((library-name "libraylib.so.6.2.0-custom"))
    (if (asdf:find-system :bramblith nil)
        (asdf:system-relative-pathname
         :bramblith
         (format nil "build/~a" library-name))
        (merge-pathnames
         library-name
         (uiop:pathname-directory-pathname
          sb-ext:*runtime-pathname*)))))

(defun initialize-raylib ()
  (if *raylib-loaded*
      (format t "Raylib already loaded!")
      (progn
	(load-shared-object (namestring (raylib-pathname)))
        (setf *raylib-loaded* t))))

(export 'initialize-raylib)

(defmacro defcfun ((c-name lisp-name) result &rest args)
  `(progn (define-alien-routine (,c-name ,lisp-name) ,result ,@args)
          (export ',lisp-name)))

;;; window
(defcfun ("SetConfigFlags" set-config-flags) void (flags unsigned-int))
(defcfun ("InitWindow" init-window) void (w int) (h int) (title c-string))
(defcfun ("CloseWindow" close-window) void)
(defcfun ("WindowShouldClose" %window-should-close) (unsigned 8))
(defcfun ("SetTargetFPS" set-target-fps) void (fps int))
(defcfun ("GetScreenWidth" get-screen-width) int)
(defcfun ("GetScreenHeight" get-screen-height) int)
(defcfun ("ToggleFullscreen" toggle-fullscreen) void)
(defcfun ("BeginDrawing" begin-drawing) void)
(defcfun ("EndDrawing" end-drawing) void)
(defcfun ("GetFrameTime" get-frame-time) float)
(defcfun ("IsWindowReady" %window-ready) (unsigned 8))

;;; input
(defcfun ("IsKeyPressed" %key-pressed) (unsigned 8) (key int))
(defcfun ("IsKeyDown" %key-down) (unsigned 8) (key int))
(defcfun ("GetCharPressed" get-char-pressed) int)

;;; colour
(defcfun ("ClearBackgroundRGBA" %clear-background) void (rgba (unsigned 32)))
(defcfun ("DrawRectangleRGBA" %draw-rectangle) void
  (x int) (y int) (w int) (h int) (rgba (unsigned 32)))
(defcfun ("ShutdownHandles" shutdown-handles) void)
(defcfun ("LoadTextureHandle" %load-texture) int
  (path c-string) (filter int) (mipmaps int))
(defcfun ("TextureHandleWidth" texture-width) int (h int))
(defcfun ("TextureHandleHeight" texture-height) int (h int))
(defcfun ("DrawTextureRegion" %draw-texture-region) void
  (h int) (sx float) (sy float) (sw float) (sh float)
  (dx float) (dy float) (dw float) (dh float) (tint (unsigned 32)))

;;; glyphs
(defcfun ("GlyphBatchBegin" %glyph-batch-begin) void (shaded int))
(defcfun ("GlyphBatchAdd" %glyph-batch-add) int
  (path c-string) (cols int) (rows int))
(defcfun ("GlyphBatchPack" %glyph-batch-pack) int (filter int) (mipmaps int))
(defcfun ("GlyphCols" glyph-cols) int (g int))
(defcfun ("GlyphRows" glyph-rows) int (g int))
(defcfun ("ShadeNew" %shade-new) int (count int))
(defcfun ("ShadeSetColor" %shade-set-color) void
  (shade int) (rank int) (rgba (unsigned 32)))
(defcfun ("GlyphShadeBegin" glyph-shade-begin) void)
(defcfun ("GlyphShadeEnd" glyph-shade-end) void)
(defcfun ("DrawGlyphAt" %draw-glyph-at) void
  (g int) (shade int) (tint (unsigned 32)) (cx int) (cy int)
  (cw float) (ch float) (ox float) (oy float))
(defcfun ("DrawGlyphGrid" %draw-glyph-grid) void
  (cells system-area-pointer) (cols int) (rows int)
  (cw float) (ch float) (ox float) (oy float))
(defcfun ("ShutdownGlyphs" shutdown-glyphs) void)
