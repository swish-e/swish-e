/*
** Copyright (C) 1995, 1996, 1997, 1998 Hewlett-Packard Company
** Originally by Kevin Hughes, kev@kevcom.com, 3/11/94
**
** This program and library is free software; you can redistribute it and/or
** modify it under the terms of the GNU (Library) General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU (Library) General Public License for more details.
**
** You should have received a copy of the GNU (Library) General Public License
**  long with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
**--------------------------------------------------------------------
** All the code in this file added by Ron Klachko ron@ckm.ucsf.edu 9/98
**
** change sprintf to snprintf to avoid corruption
** SRE 11/17/99
**
** fixed cast to int problems pointed out by "gcc -Wall"
** SRE 2/22/00
** 
*/

/*
** httpserver.c
*/

#ifndef _WIN32
#include <unistd.h>
#endif

#include <time.h>
#include <stdarg.h>

#include "swish.h"
#include "string.h"
#include "mem.h"

#include "http.h"
#include "httpserver.h"


/* The list of servers that we are acting on.
**/
static httpserverinfo *servers = 0;


static void parserobotstxt(FILE *fp, httpserverinfo *server);
static char *isolatevalue(char *line, char *keyword, int *plen);
static int serverinlist(char *url, struct swline *list);



/* Find the robot rules for this URL.  If haven't retrieved them
** yet, do so now.
**/
httpserverinfo *getserverinfo(SWISH *sw, char *url)
{
    httpserverinfo *server;
    char *method;
    int methodlen;
    char *serverport;
    int serverportlen;
    static int lencontenttype=0;
    static char *contenttype=NULL;
    static int lenbuffer=0;
    static char *buffer=NULL;
    FILE *fp;
	
    if(!lenbuffer)buffer=emalloc((lenbuffer=MAXSTRLEN)+1);
    if(!lencontenttype)contenttype=emalloc((lencontenttype=MAXSTRLEN)+1);

    if ((method = url_method(url, &methodlen)) == 0) {
		return 0;
    }
    if ((serverport = url_serverport(url, &serverportlen)) == 0) {
		return 0;
    }
	
    /* Search for the rules
    **/
    for (server = servers; server; server = server->next) {
		if (equivalentserver(sw, url, server->baseurl)) {
			return server;
		}
    }
    
    /* Create a new entry for this server and add it to the list.
    **/
    server = (httpserverinfo *)emalloc(sizeof(httpserverinfo));
	
    /* +3 for the ://, +1 for the trailing /, +1 for the terminating null
    **/
    server->baseurl = (char *)emalloc(methodlen + serverportlen + 5);
    /* These 4 lines to avoid a call to non ANSI snprintf . May not be the
     best way but it ensures no buffer overruns */
    memcpy (server->baseurl,method,methodlen);
    memcpy (server->baseurl+methodlen,"://",3);
    memcpy (server->baseurl+methodlen+3,serverport,serverportlen);
    strcpy (server->baseurl+methodlen+3+serverportlen,"/");
    
    server->lastretrieval = 0;
    server->robotrules = 0;
    server->next = servers;
    servers = server;
	
    /* Only http(s) servers can full rules, all the other ones just get dummies
    ** (this is useful for holding last retrieval)
    **
    ** http://info.webcrawler.com/mak/projects/robots/norobots.html holds what
    ** many people consider the official web exclusion rules.  Unfortunately,
    ** the rules are not consistent about how records are formed.  One line
    ** states "the file consists of one or more records separated by one or more
    ** blank lines" while another states "the record starts with one or more User-agent
    ** lines, followed by one or more Disallow lines."
    **
    ** So, does a blank line after a User-agent line end a record?  The spec is
    ** unclear on this matter.  If the next legal line afer the blank line is
    ** a Disallow line, the blank line should most likely be ignored.  But what
    ** if the next line is another User-agent line?  For example:
    **
    ** User-agent: MooBot
    **
    ** User-agent: CreepySpider
    ** Disallow: /cgi-bin
    **
    ** One interpretation (based on blank lines termination records) is that MooBot
    ** may visit any location (since there are no Disallows for it).  Another
    ** interpretation (based on records needing both User-agent and Disallow lines)
    ** is that MooBot may not visit /cgi-bin
    **
    ** While poking around, I found at least one site (www.sun.com) that uses blank
    ** lines within records.  Because of that, I have decided to rely on records
    ** having both User-agent and Disallow lines (the second interpretation above).
    **/
    if (strncmp(server->baseurl, "http", 4) == 0) {
		if((int)(strlen(server->baseurl)+20)>=lenbuffer) {
			lenbuffer=strlen(server->baseurl)+20+200;
			buffer=erealloc(buffer,lenbuffer+1);
		}
		sprintf(buffer, "%srobots.txt", server->baseurl);
		if (get(sw,contenttype, &server->lastretrieval, buffer) == 200) {
			if((int)(strlen(sw->tmpdir)+MAXPIDLEN+30)>=lenbuffer) {
				lenbuffer=strlen(sw->tmpdir)+MAXPIDLEN+30+200;
				buffer=erealloc(buffer,lenbuffer+1);
			}
			sprintf(buffer, "%s/swishspider@%ld.contents", sw->tmpdir, (long)lgetpid());
			fp = fopen(buffer, "r");
			parserobotstxt(fp, server);
			fclose(fp);
		}
		
		cmdf(unlink, "%s/swishspider@%ld.response", sw->tmpdir, lgetpid());
		cmdf(unlink, "%s/swishspider@%ld.contents", sw->tmpdir, lgetpid());
		cmdf(unlink, "%s/swishspider@%ld.links", sw->tmpdir, lgetpid());
    }
	
    return server;
}


int urldisallowed(SWISH *sw, char *url)
{
    httpserverinfo *server;
    robotrules *rule;
    char *uri;
    int urilen;
	
    if ((server = getserverinfo(sw, url)) == 0) {
		return 1;
    }
    if ((uri = url_uri(url, &urilen)) == 0) {
		return 1;
    }
	
    for (rule = server->robotrules; rule; rule = rule->next) {
		if (strncmp(uri, rule->disallow, strlen(rule->disallow)) == 0) {
			return 1;
		}
    }
	
    return 0;
}


static char useragent[] = "user-agent:";
static char disallow[] = "disallow:";
static char swishspider[] = "swishspider";

static void parserobotstxt(FILE *fp, httpserverinfo *server)
{
    char buffer[MAXSTRLEN];
    enum {START, USERAGENT, DISALLOW} state = START;
    enum {SPECIFIC, GENERIC, SKIPPING} useragentstate = SKIPPING;
    char *p;
    int len;
    robotrules *entry;
    robotrules *entry2;
	
    server->useragent = 0;
    
    while (fgets(buffer, sizeof buffer, fp) != 0) {
	/* Remove end of line indicators
		**/
		while ((*(buffer + strlen(buffer) - 1) == '\r') ||
			(*(buffer + strlen(buffer) - 1) == '\n')) {
			*(buffer + strlen(buffer) - 1) = '\0';
		}
		
		if ((*buffer == '#') || (*buffer == '\0'))
			continue;
		
		if (strncasecmp(buffer, useragent, sizeof(useragent) - 1) == 0) {
			switch (state) {
			case DISALLOW:
			/* Since we found our specific user-agent, we can
			** skip the rest of the file.
				**/
				if (useragentstate == SPECIFIC) {
					return;
				}
				
				useragentstate = SKIPPING;
				
				/* explict fallthrough */
				
			case START:
			case USERAGENT:
				state = USERAGENT;
				
				if (useragentstate != SPECIFIC) {
					p = isolatevalue(buffer, useragent, &len);
					
					if ((len == (sizeof(swishspider) - 1)) &&
						(strncasecmp(p, swishspider, sizeof(swishspider) - 1) == 0) ) {
						useragentstate = SPECIFIC;
						
						/* We might have already parsed generic rules,
						** so clean them up if necessary.
						*/
						if (server->useragent) {
							efree(server->useragent);
						}
						for (entry = server->robotrules; entry; ) {
							entry2 = entry->next;
							efree(entry);
							entry = entry2;
						}
						server->robotrules = 0;
						
						server->useragent = (char *)emalloc(len + 1);
						strncpy(server->useragent, p, len);
						*(server->useragent + len) = '\0';
						
					}
					else if ((len == 1) && (*p == '*')) {
						useragentstate = GENERIC;
						server->useragent = (char *)emalloc(2);
						strcpy(server->useragent, "*"); /* emalloc'd 2 bytes, no safestrcpy */
					}
					
				}
				
				
				break;
				
			}
		}
		
		if (strncasecmp(buffer, disallow, sizeof(disallow) - 1) == 0) {
			state = DISALLOW;
			if (useragentstate != SKIPPING) {
				p = isolatevalue(buffer, disallow, &len);
				if (len) {
					entry = (robotrules *)emalloc(sizeof(robotrules));
					entry->next = server->robotrules;
					server->robotrules = entry;
					entry->disallow = (char *)emalloc(len + 1);
					strncpy(entry->disallow, p, len);
					*(entry->disallow + len) = '\0';
				}
			}
		}
    }
}


static char *isolatevalue(char *line, char *keyword, int *plen)
{
	/* Find the beginning of the value
    **/
    for (line += strlen(keyword); isspace((int)((unsigned char)*line)); line++ ) { /* cast to int 2/22/00 */
    }
	
    /* Strip off trailing spaces
    **/
    for (*plen = strlen(line); isspace((int)((unsigned char)*(line + *plen - 1))); (*plen)--) { /* cast to int 2/22/00 */
    }
	
    return line;
}


int equivalentserver(SWISH *sw, char *url, char *baseurl)
{
char *method;
int methodlen;
char *serverport;
int serverportlen;
char *basemethod;
int basemethodlen;
char *baseserverport;
int baseserverportlen;
struct multiswline *walk=NULL;
	
    method = url_method(url, &methodlen);
    serverport = url_serverport(url, &serverportlen);
    basemethod = url_method(baseurl, &basemethodlen);
    baseserverport = url_serverport(baseurl, &baseserverportlen);
	
    if (!method || !serverport || !basemethod || !baseserverport) {
		return 0;
    }
	
    /* If this is the same server, we just go for it
    **/
    if ((methodlen == basemethodlen) && (serverportlen == baseserverportlen) &&
		(strncasecmp(method, basemethod, methodlen) == 0) &&
		(strncasecmp(serverport, baseserverport, serverportlen) == 0)) {
		return 1;
    }
	
    /* Do we find the method/server info for this and the base url
    ** in the same equivalence list?
    **/
    for (walk = sw->equivalentservers; walk; walk = walk->next ) {
		if (serverinlist(url, walk->list) &&
			serverinlist(baseurl, walk->list)) {
			return 1;
		}
    }
	
    return 0;
}


static int serverinlist(char *url, struct swline *list)
{
    char *method;
    int methodlen;
    char *serverport;
    int serverportlen;
    char *listmethod;
    int listmethodlen;
    char *listserverport;
    int listserverportlen;
    
    method = url_method(url, &methodlen);
    serverport = url_serverport(url, &serverportlen);
    if (!method || !serverport) {
		return 0;
    }
	
    for ( ; list; list = list->next) {
		listmethod = url_method(list->line, &listmethodlen);
		listserverport = url_serverport(list->line, &listserverportlen);
		if (listmethod && listserverport) {
			if ((methodlen == listmethodlen) && (serverportlen == listserverportlen) &&
				(strncasecmp(method, listmethod, methodlen) == 0) &&
				(strncasecmp(serverport, listserverport, serverportlen) == 0)) {
				return 1;
			}
		}
    }
    return 0;
}

