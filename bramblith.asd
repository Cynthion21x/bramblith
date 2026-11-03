(asdf:defsystem "bramblith"
		:serial t
		:author "cynthion21x (cynth@cynthion21x.co.uk)"
		:version "0.0.0"
		:pathname "src/"
		
		:components ((:file "package")
			     (:file "ffi")
			     (:file "util")
			     (:file "path")
			     (:file "engine")
			     (:file "glyphs")
			     (:file "shades")
			     (:file "main")))
