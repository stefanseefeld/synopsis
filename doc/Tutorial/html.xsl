<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version='1.0'
                xmlns="http://www.w3.org/TR/xhtml1/transitional"
                exclude-result-prefixes="#default">

<xsl:import href="http://docbook.sourceforge.net/release/xsl/1.65.1/html/chunk.xsl"/>

<xsl:param name="html.stylesheet" select="'synopsis.css'"/>
<xsl:param name="use.extensions" select="'1'"/>
<xsl:param name="use.id.as.filename" select="1"/>
<xsl:param name="chunk.first.sections" select="1"/>
<xsl:param name="tablecolumns.extension" select="0"/>
<xsl:param name="use.svg" select="0"/>
<xsl:param name="header.rule" select="0"/>
<xsl:param name="footer.rule" select="0"/>
<xsl:param name="table.borders.with.css" select="1"/>
<xsl:param name="segmentedlist.as.table" select="1"/>
</xsl:stylesheet>
