<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version='1.0'
                xmlns="http://www.w3.org/TR/xhtml1/transitional"
                exclude-result-prefixes="#default">

<xsl:import href="http://docbook.sourceforge.net/release/xsl/current/fo/docbook.xsl"/>

<xsl:param name="use.extensions" select="1"/>
<!--<xsl:param name="fop.extensions" select="1"/>-->
<xsl:param name="xep.extensions" select="1"/>
<xsl:param name="section.autolabel">1</xsl:param>
<xsl:param name="section.label.includes.component.label">1</xsl:param>
<xsl:param name="shade.verbatim" select="1"/>
<xsl:attribute-set name="shade.verbatim.style">
  <xsl:attribute name="border">thin black solid</xsl:attribute>
  <xsl:attribute name="background-color">#f2f5f7</xsl:attribute>
</xsl:attribute-set>
<!-- Allow line breaks in verbatim code environments with a backslash
     to indicate continuation.  -->
<xsl:param name="hyphenate.verbatim">1</xsl:param>
<xsl:attribute-set
    name="monospace.verbatim.properties" 
    use-attribute-sets="verbatim.properties monospace.properties">
  <xsl:attribute name="wrap-option">wrap</xsl:attribute>
  <xsl:attribute name="hyphenation-character">\</xsl:attribute>
</xsl:attribute-set>
<xsl:param name="use.svg" select="0"></xsl:param>

<!-- Show URLs as footnotes since inserting them inline makes it hard to 
     break lines.  -->
<xsl:param name="ulink.footnotes">1</xsl:param>
<xsl:param name="segmentedlist.as.table" select="1"/>
</xsl:stylesheet>
