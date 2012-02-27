var isNav4 = false, isIE4 = false;
if (parseInt(navigator.appVersion.charAt(0)) == 4)
{
  isNav4 = (navigator.appName == "Netscape") ? true : false;
}
else if (parseInt(navigator.appVersion.charAt(0)) >= 4) 
{
  isIE4 = (navigator.appName.indexOf("Microsoft") != -1) ? true : false;
}
var isMoz = (isNav4 || isIE4) ? false : true;

var showImage = new Image(); 
var hideImage = new Image();

/* The 'className' member only returns the entire (whitespace-separated) list
   of classes, while CSS selectors use individual classes. has_class allows to
   check for an individual class within such a list.*/
function has_class(object, class_name)
{
  if (!object.className) return false;
  return (object.className.search('(^|\\s)' + class_name + '(\\s|$)') != -1);
}

function get_children_with_class(parent, cl)
{
  var result = new Array();
  for (var i = 0; i < parent.childNodes.length; ++i)
  {
    if (has_class(parent.childNodes[i], cl))
      result[result.length] = parent.childNodes[i];
  }
  return result;
}

function get_child_with_class(parent, cl)
{
  var children = get_children_with_class(parent, cl);
  if (children.length != 0) return children[0];
  else return false; /*??*/
}

function tree_init(show_src, hide_src) 
{
  showImage.src = show_src; hideImage.src = hide_src;
}
function tree_node_toggle(id) 
{
  if (isMoz)
  {
    var section = document.getElementById(id);
    var image = document.getElementById(id+"_img");
    if (section.style.display == "none") 
    {
      section.style.display = "";
      image.src = showImage.src;
    }
    else
    {
      section.style.display = "none";
      image.src = hideImage.src;
    }
  }
  else if (isIE4)
  {
    section = document.items[id];
    image = document.images[id+"_img"];
    if (section.style.display == "none") 
    {
      section.style.display = "";
      image.src = showImage.src;
    }
    else
    {
      section.style.display = "none";
      image.src = hideImage.src;
    }
  }
  else if (isNav4) 
  {
    section = document.items[id];
    image = document.images[id+"_img"];
    if (section.display == "none") 
    {
      section.style.display = "";
      image.src = showImage.src;
    }
    else
    {
      section.display = "none";
      image.src = hideImage.src;
    }
  }
}

function expand_all(tree)
{
  var nodes = document.getElementById(tree).getElementsByTagName('ul');
  for (var i = 1; i <= nodes.length; i++)
  {
    var id = "tree"+i;
    var section = document.getElementById(id);
    var image = document.getElementById(id+"_img");
    section.style.display = "";
    image.src = showImage.src;
  }
}
function collapse_all(tree)
{
  var nodes = document.getElementById(tree).getElementsByTagName('ul');
  for (var i = 1; i <= nodes.length; i++)
  {
    var id = "tree"+i;
    var section = document.getElementById(id);
    var image = document.getElementById(id+"_img");
    section.style.display = "none";
    image.src = hideImage.src;
  }
}

tree_init("tree_open.png", "tree_close.png");

function go(frame1, url1, frame2, url2)
{
  window.parent.frames[frame1].location=url1;
  window.parent.frames[frame2].location=url2;
  return false;
}

/* A body section is a section inside a Body part. */

function body_section_expand(id) 
{
  var section = document.getElementById(id);
  section.style.display = 'block';
  var toggle = document.getElementById('toggle_' + id);
  toggle.firstChild.data = '-';
 }

function body_section_collapse(id) 
{
  var section = document.getElementById(id);
  section.style.display = 'none';
  var toggle = document.getElementById('toggle_' + id);
  toggle.firstChild.data = '+';
  toggle.style.display = 'inline';
}

function body_section_toggle(id) 
{
  var section = document.getElementById(id);
  var toggle = document.getElementById('toggle_' + id);
  if (section.style.display == 'none') 
  {
    section.style.display = 'block';
    toggle.firstChild.data = '-';
  }
  else
  {
    section.style.display = 'none';
    toggle.firstChild.data = '+';
  }
}

function body_section_collapse_all()
{
  var divs = document.getElementsByTagName('div');
  for (var i = 0; i < divs.length; ++i)
  {
    if (has_class(divs[i], 'expanded') && divs[i].hasAttribute('id'))
    {
      body_section_collapse(divs[i].getAttribute('id'));
    }
  }
}

function body_section_init()
{
  var divs = document.getElementsByTagName('div');
  for (var i = 0; i < divs.length; ++i)
  {
    if (has_class(divs[i], 'heading'))
    {
      var toggle = get_child_with_class(divs[i], 'toggle');
      if (toggle) toggle.style.display='inline'
    }
  }
}

function decl_doc_expand(id) 
{
  var doc = document.getElementById(id);
  var collapsed = get_children_with_class(doc, 'collapsed');
  for (var i = 0; i < collapsed.length; ++i)
  {
    collapsed[i].style.display='none';
  }
  var expanded = get_children_with_class(doc, 'expanded');
  for (var i = 0; i < expanded.length; ++i)
  {
    expanded[i].style.display='block';
  }
  var collapse_toggle = get_child_with_class(doc, 'collapse-toggle');
  collapse_toggle.style.display='block';
}
function decl_doc_collapse(id) 
{
  var doc = document.getElementById(id);
  var expanded = get_children_with_class(doc, 'expanded');
  for (var i = 0; i < expanded.length; ++i)
  {
    expanded[i].style.display='none';
  }
  var collapse_toggle = get_child_with_class(doc, 'collapse-toggle');
  collapse_toggle.style.display='none';
  var collapsed = get_children_with_class(doc, 'collapsed');
  for (var i = 0; i < collapsed.length; ++i)
  {
    collapsed[i].style.display='block';
  }
}

function decl_doc_collapse_all()
{
  var divs = document.getElementsByTagName('div');
  for (var i = 0; i < divs.length; ++i)
  {
    if (has_class(divs[i], 'collapsible'))
    {
      decl_doc_collapse(divs[i].getAttribute('id'));
    }
  }
}

function load()
{
  decl_doc_collapse_all();
  body_section_init();
}