#!/usr/bin/env python

# test.py - swish-python example script.  Tests basic API functionality.
#
# Copyright (C) 2005 David L Norris <dave@webaugur.com>
#

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Library General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

import sys
import time
from swish import * 


class swishtest:
    # initialize the application
    def __init__(self, argv):
        if argv[1:]:
            query=" ".join(argv[1:])
            self.search(query)
        else:
            print('test.py <search query>')
    
    
    def search(self, query):
        file = "index.swish-e"
        # create a new Swish-e instance
        handle = SwishInit(file)
        
        # check for errors opening index file(s)
        if SwishError(handle):
            print( 'err: ' + SwishErrorString(handle) + ': Could not open the index file \'' + file + '\'' )
            print('.')
            sys.exit(1)
        
        # create a search object
        search = New_Search_Object(handle, query)
        
        # time query
        before=time.time()
        
        # fetch results object
        results = SwishExecute(search, query)
        
        #time query
        taken=time.time() - before
        
        # get number of hits
        print('# Search words: ' + query )
        print("# Number of hits: " + str(SwishHits(results)) )
        print("# Search time: "+str(taken) + " seconds")
        if SwishHits(results) == 0: print('err: no results')
        while 1:
            # seek to the next result object
            result=SwishNextResult(results)
            if not result: break
            
            # print some properties for each result
            print( SwishResultPropertyStr(result, "swishrank") +" "+ \
                SwishResultPropertyStr(result, "swishdocpath") + ": \"" + \
                SwishResultPropertyStr(result, "swishtitle") + "\" " + \
                SwishResultPropertyStr(result, "swishdocsize") \
                )
        
        print('.')
        
        # free results object
        Free_Results_Object(results)
        # destroy the handle to free memory
        SwishClose(handle)
        
def main():
    swishtest(sys.argv)

if __name__ == '__main__':
    main()
