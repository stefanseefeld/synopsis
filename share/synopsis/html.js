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

showImage = new Image(); hideImage = new Image();
function init_tree(show_src, hide_src) 
{
  showImage.src = show_src; hideImage.src = hide_src;
}
function toggle(id) 
{
  if (isMoz)
  {
    section = document.getElementById(id);
    image = document.getElementById(id+"_img");
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
var tree_max_node = 0;
function open_all()
{
  for (i = 1; i <= tree_max_node; i++)
  {
    id = "tree"+i;
    section = document.getElementById(id);
    image = document.getElementById(id+"_img");
    section.style.display = "";
    image.src = showImage.src;
  }
}
function close_all()
{
  for (i = 1; i <= tree_max_node; i++)
  {
    id = "tree"+i;
    section = document.getElementById(id);
    image = document.getElementById(id+"_img");
    section.style.display = "none";
    image.src = hideImage.src;
  }
}

init_tree("tree_open.png", "tree_close.png");

function go(frame1, url1, frame2, url2)
{
  window.parent.frames[frame1].location=url1;
  window.parent.frames[frame2].location=url2;
  return false;
}
