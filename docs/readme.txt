Building XBMC:
--------------

You'll need the following to build XBMC
- Visual Studio.NET 2003
- XDK 5778 or higher
- XBMC sources offcourse 
  You can download the latest snapshot of XBMC here: 
  http://xbmc.sourceforge.net/xbmc.tar.gz  or 
  use CVS to get the sources. Look here: http://sourceforge.net/cvs/?group_id=87054

Next 
  - Install Visual Studio.NET
      When installing VS.NET. Make sure to install all C++ stuff
  - Install XDK 5778 (or higher)
      Make sure to do a full install of the XDK!!! Minimal or customized install wont work
  - use winrar or a similar program to uncompress xbmc.tar.gz into a folder

Building
-----------------
Start Visual Studio.NET and open the xbmc.sln solution file
Next build the solution with Build->Build Solution (Ctrl+Shift+B)
NOTE:  Dont worry about the following warnings which appear at the end of the build. 
They are normal and can be ignored
  Creating Xbox Image...
  IMAGEBLD : warning IM1029: library XONLINE is unapproved
  IMAGEBLD : warning IM1030: this image may not be accepted for certification
  Copying files to the Xbox...


Then when all is build, its time to install XBMC on your xbox!
(You can also use the build.bat file to make a build of xbmc)


Installing XBMC:
-----------------

edit xboxmediacenter.xml and fill in all the details for your installation

After editting xboxmediacenter.xml you need to copy the following files & folder to your xbox
lets say you're installing XBMC on your xbox in the folder e:\apps\xbmc then:

1.create e:\apps\xbmc on your xbox
2. copy following files from pc->xbox

   PC                       XBOX
-------------------------------------------------------------------------------   
XboxMediaCenter.xml ->  e:\apps\xbmc\XboxMediaCenter.xml
keymap.xml          ->  e:\apps\xbmc\keymap.xml
FileZilla Server.xml->  e:\apps\xbmc\FileZilla Server.xml
release/default.xbe ->  e:\apps\xbmc\default.xbe
system/             ->  e:\apps\xbmc\system         (copy all files & subdirs)
skin/               ->  e:\apps\xbmc\skin           (copy all files & subdirs)
sounds/             ->  e:\apps\xbmc\sounds         (copy all files & subdirs)
language/           ->  e:\apps\xbmc\language       (copy all files & subdirs)
weather/            ->  e:\apps\xbmc\weather        (copy all files & subdirs)
media               ->  e:\apps\xbmc\media          (copy all files & subdirs)
screensavers        ->  e:\apps\xbmc\screensavers   (copy all files & subdirs)
visualisations      ->  e:\apps\xbmc\visualisations (copy all files & subdirs)
scripts/            ->  e:\apps\xbmc\scripts        (these are just samples, only extract if you want to experiment with it)
python              ->  e:\apps\xbmc\python         (unpack the python.rar file)
web                 ->  e:\apps\xbmc\web            (unpack the xbmc.rar file)
credits             ->  e:\apps\xbmc\credits        (copy all files but not subdirs)

!!! please not that you unpack the .rar files in scripts/ python/ and web/ !!!


