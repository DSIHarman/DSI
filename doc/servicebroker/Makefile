all: clean docbook 

docbook:
	doxygen
	cd xml ; xsltproc combine.xslt index.xml >all_in_one.xml
	cd xml ; xsltproc ../boostxsl/doxygen/doxygen2boostbook.xsl all_in_one.xml > boost.xml
	cd xml ; xsltproc ../boostxsl/docbook.xsl boost.xml > docbook.xml
	cp xml/docbook.xml doxygen.xml 
	cat doxygen.xml |sed 's/\/home\/florentin\/genivi\/sb_doc\/include\///g' > doxygen_new.xml
	mv doxygen_new.xml doxygen.xml

clean:
	rm -rf xml
