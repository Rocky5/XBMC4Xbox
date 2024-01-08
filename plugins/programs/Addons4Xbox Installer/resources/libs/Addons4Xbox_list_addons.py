

import os
import sys
import time

from traceback import print_exc

__language__     = sys.modules[ "__main__" ].__language__


# Custom modules
try:
    from globalvars import DIR_CACHE, DIR_ADDON_REPO, PARAM_REPO_ID, PARAM_TYPE, PARAM_INSTALL_FROM_REPO, PARAM_ADDON_NAME, PARAM_ADDON_ID, PARAM_URL, PARAM_DATADIR, VALUE_LIST_ALL_ADDONS
    from PluginMgr import PluginMgr
    from Item import supportedAddonList #TYPE_ADDON_SCRIPT, TYPE_ADDON_MUSIC, TYPE_ADDON_PICTURES, TYPE_ADDON_PROGRAMS, TYPE_ADDON_VIDEO, TYPE_ADDON_WEATHER, TYPE_ADDON_MODULE
    from utilities import readURL, PersistentDataCreator, fileOlderThan
    from XmlParser import ListItemFromXML, parseAddonXml
    from AddonsMgr import getInstalledAddonInfo
except:
    print_exc()


class Main:
    """
    Main plugin class
    """

    def __init__( self, *args, **kwargs ):

        self.pluginMgr = PluginMgr()
        self.parameters = self.pluginMgr.parse_params()

        repoId = self.parameters[ PARAM_REPO_ID ]
        addonCat = self.parameters[ PARAM_TYPE ]
        self._createAddonsDir( repoId, addonCat )

        self.pluginMgr.add_sort_methods( True )
        self.pluginMgr.end_of_directory( True )


    def _createAddonsDir ( self, repoId, cat ):
        """
        Display the addons to install for a repository
        """
        # Retrieve info from  addon.xml for the repository
        repoInfo = getInstalledAddonInfo( os.path.join( DIR_ADDON_REPO, repoId) )

        addonDic = {}

        # Retrieving addons.xml from remote repository
        xmlInfofPath = os.path.join( DIR_CACHE, repoId + ".xml")
        if fileOlderThan(xmlInfofPath, 60 * 30):
            data = readURL( repoInfo [ "repo_url" ], save=True, localPath=xmlInfofPath )

        if ( os.path.exists( xmlInfofPath ) ):
            try:
                xmlData = open( os.path.join( xmlInfofPath ), "r" )
                listAddonsXml = ListItemFromXML(xmlData)
                xmlData.close()
            except:
                print_exc()

            filter = "False"
            if cat == VALUE_LIST_ALL_ADDONS:
                filter = "item['type'] in supportedAddonList"
            elif cat in supportedAddonList:
                filter = "item['type'] == '%s'"%cat

            keepParsing = True
            while (keepParsing):
                item = listAddonsXml.getNextItem()
                if item:
                    if eval(filter):
                        endRepoChar = "/"
                        if repoInfo [ "repo_datadir" ].endswith( "/" ):
                            endRepoChar = ""

                        if repoInfo [ "repo_format" ] ==  'zip':
                            downloadUrl = (repoInfo [ "repo_datadir" ] + endRepoChar + item["id"] + '/' + item["id"]  + '-' + item["version"] + ".zip").replace(' ', '%20')
                            changelog   = (repoInfo [ "repo_datadir" ] + endRepoChar + item["id"] + '/' + "changelog" + '-' + item["version"] + ".txt").replace(' ', '%20')
                            iconimage   = (repoInfo [ "repo_datadir" ] + endRepoChar + item["id"] + '/' + "icon.png").replace(' ', '%20')
                        else:
                            downloadUrl = (repoInfo [ "repo_datadir" ] + endRepoChar + item["id"] + '/').replace(' ', '%20')
                            changelog   = (repoInfo [ "repo_datadir" ] + endRepoChar + item["id"] + '/' + "changelog" + ".txt").replace(' ', '%20')
                            iconimage   = (repoInfo [ "repo_datadir" ] + endRepoChar + item["id"] + '/' + "icon.png").replace(' ', '%20')
                        item["ImageUrl"] = iconimage
                        item["changelog"] = changelog

                        paramsAddons = {}
                        paramsAddons[PARAM_INSTALL_FROM_REPO]   = "true"
                        paramsAddons[PARAM_ADDON_ID]            = item[ "id" ]
                        paramsAddons[PARAM_ADDON_NAME]          = item['name']
                        paramsAddons[PARAM_URL]                 = downloadUrl
                        paramsAddons[PARAM_DATADIR]             = repoInfo[ "repo_datadir" ]
                        paramsAddons[PARAM_TYPE]                = repoInfo[ "repo_format" ]
                        paramsAddons[PARAM_REPO_ID]             = repoId
                        url = self.pluginMgr.create_param_url( paramsAddons )

                        if ( url ):
                            #self._addLink( item['name'], url, iconimage=iconimage)
                            item["PluginUrl"] = url
                            self.pluginMgr.addItemLink( item )
                            #addonList.append( item )
                            addonDic[ item[ "id" ]] = item
                else:
                    keepParsing = False
            # Save the list of addons
            PersistentDataCreator( addonDic, os.path.join( DIR_CACHE, repoId + ".txt" ) )

