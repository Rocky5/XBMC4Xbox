#! /usr/bin/python

#############################################################
# This tool is used to generate the Platinum version info file #
#############################################################
import sys
import os

# ensure that PLATINUM_HOME has been set and exists
if not os.environ.has_key('PLATINUM_HOME'):
    print 'ERROR: PLATINUM_HOME not set'
    sys.exit(1)
PLATINUM_HOME = os.environ['PLATINUM_HOME']
    
# get the SVN repo version
version = os.popen('svnversion -n').readlines()[0]
print 'current VERSION =',version
if version.endswith('P'):
     version = version[0:-1]
if version.endswith('MP'):
     version = version[0:-2]
try:
    version_int = int(version)+1 ## add one, because when we check it in, the rev will be incremented by one
except:
    print 'ERROR: you cannot run this on a modified working copy'
    sys.exit(1)
    

output = open(PLATINUM_HOME+'/Source/Platinum/PltSvnVersion.h', 'w+')
output.write('/* DO NOT EDIT. This file was automatically generated by GenSvnVersionHeader.py */\n')
output.write('#define PLT_SVN_VERSION '+str(version_int)+'\n')
output.write('#define PLT_SVN_VERSION_STRING "'+str(version_int)+'"\n')
output.close()
print 'upon check-in, version will be', str(version_int)
