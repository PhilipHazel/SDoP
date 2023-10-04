# Makefile for SDoP documentation. The fancy gs command ensures that the fonts
# are embedded in the PDF. Otherwise some of the rarer characters may not be
# present in fonts used by acroread or other PDF displayers.

sdop.pdf:   sdop.ps
	    gs -q -dNOPAUSE -dBATCH -dPDFSETTINGS=/prepress -sDEVICE=pdfwrite -sOutputFile=sdop.pdf sdop.ps

sdop.ps:    sdop.xml
	    sdop sdop.xml

sdop.xml:   sdop.xfpt
	    xfpt sdop.xfpt

# End
