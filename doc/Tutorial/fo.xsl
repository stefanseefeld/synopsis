<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version='1.0'
                xmlns="http://www.w3.org/TR/xhtml1/transitional"
                exclude-result-prefixes="#default">

<xsl:import href="http://docbook.sourceforge.net/release/xsl/1.65.1/fo/docbook.xsl"/>

<xsl:param name="use.extensions" select="1"/>
<xsl:param name="fop.extensions" select="1"/>
<xsl:param name="shade.verbatim" select="1"/>
<!-- I'd prefer tables, but fop does a bad job with the layout -->
<!-- <xsl:param name="segmentedlist.as.table" select="1"/> -->
</xsl:stylesheet>
