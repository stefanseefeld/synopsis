<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version='1.0'
                xmlns="http://www.w3.org/TR/xhtml1/transitional"
                exclude-result-prefixes="#default">

<!--<xsl:import href="docbook.xsl"/>-->

<xsl:template name="user.header.navigation">
    <div class="page-heading">
    Synopsis 0.5 - User Manual
    </div>
</xsl:template>

<xsl:param name="html.stylesheet" select="'synopsis-html.css'"/>
<xsl:param name="use.id.as.filename" select="1"/>
<xsl:param name='chunk.first.sections' select="1"/>

</xsl:stylesheet>
