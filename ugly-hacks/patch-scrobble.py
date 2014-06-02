$dukzcry$
Fix Feature #607 (http://bugs.foocorp.net/issues/607) for Libre.fm
--- scrobble.py.orig	2011-07-20 14:34:32.000000000 +0400
+++ scrobble.py	2011-10-20 15:35:55.000000000 +0400
@@ -8,7 +8,7 @@ except ImportError:
 from optparse import OptionParser
 import time
 from urllib import urlencode
-from urllib2 import urlopen
+from urllib2 import urlopen, URLError
 
 
 class ScrobbleException(Exception):
@@ -47,6 +47,14 @@ class ScrobbleServer(object):
         self.session_id = lines[1]
         self.submit_url = lines[3]
 
+    def subsubmit(self, data):
+	try:
+	    response = urlopen(self.submit_url, urlencode(data)).read()
+	except URLError:
+	    return False
+	    
+	return response
+
     def submit(self, sleep_func=time.sleep):
         if len(self.post_data) == 0:
             return
@@ -56,9 +64,9 @@ class ScrobbleServer(object):
             data += track.get_tuples(i)
             i += 1
         data += [('s', self.session_id)]
-        response = urlopen(self.submit_url, urlencode(data)).read()
-        if response != "OK\n":
-            raise ScrobbleException("Server returned: %s" % (response,))
+        while self.subsubmit(data) != "OK\n":
+	    pass	    	    
+
         self.post_data = []
         sleep_func(1)
 
