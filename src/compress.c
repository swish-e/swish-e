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
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "swish.h"
#include "compress.h"
#include "mem.h"
#include "error.h"
#include "merge.h"
#include "docprop.h"
#include "index.h"
#include "search.h"
#include "hash.h"



/* 09/00 Jose Ruiz 
** Function to compress location data in memory 
** and use less memory while indexing */
unsigned char *compress_location(SWISH *sw,IndexFILE *indexf,LOCATION *l)
{
unsigned char *p,*q;
int i,index,index_structure,index_structfreq,max_size,vars[2];
	/* check if the buffer is long enough */
	/* just to avoid bufferoverruns */
	/* In the worst case and integer will need 5 bytes */
	/* but fortunatelly this is very uncommon */
	max_size=((sizeof(LOCATION)+(l->frequency-1)*sizeof(int))*5)/sizeof(int);
	if(max_size>sw->len_compression_buffer)
	{
		sw->len_compression_buffer=max_size+200;
		sw->compression_buffer=erealloc(sw->compression_buffer,sw->len_compression_buffer);
	}
	
	p=sw->compression_buffer+sw->len_compression_buffer-1;
	for(i=l->frequency-1;i>=0;i--) {
		compress2(l->position[i],p);
	}
	compress2(l->filenum,p);
	/* Pack metaID frequency and structure in just one integer */
	/* Pack structure in just one smaller integer */
	vars[0]=l->structure;
	index_structure=get_lookup_index(&indexf->structurelookup,1,vars)+1;
	vars[0]=l->frequency;
	vars[1]=index_structure;
	index_structfreq=get_lookup_index(&indexf->structfreqlookup,2,vars)+1;
	/* Pack and add 1 to index to avoid problems with compress 0 */
	vars[0]=l->metaID;
	vars[1]=index_structfreq;
	index=get_lookup_index(&indexf->locationlookup,2,vars)+1;
	compress2(index,p);
		/* Get the length of all the data */
	i=sw->compression_buffer+sw->len_compression_buffer-p-1;
		/* Swap info to file ? */
		/* If IgnoreLimit is set then no swap is done */
	if(sw->swap_flag && sw->plimit==NO_PLIMIT)
	{
		q=(unsigned char *)SwapLocData(sw,++p,i);
	} else {
		q=(unsigned char *) emalloc(i);
		memcpy(q,++p,i);
	}
	efree(l);
	return q;
}

/* 09/00 Jose Ruiz 
** Extract compressed location
** Just needed by merge and removestops (IgnoreLimit option)
*/
LOCATION *uncompress_location(SWISH *sw,IndexFILE *indexf, unsigned char *p)
{
LOCATION *loc;
int i,tmp,metaID,filenum,structure,frequency,index,index_structfreq,index_structure;
	uncompress2(index,p);
	uncompress2(filenum,p);
	metaID=indexf->locationlookup->all_entries[index-1]->val[0];
	index_structfreq=indexf->locationlookup->all_entries[index-1]->val[1];
	frequency=indexf->structfreqlookup->all_entries[index_structfreq-1]->val[0];
	index_structure=indexf->structfreqlookup->all_entries[index_structfreq-1]->val[1];
	structure=indexf->structurelookup->all_entries[index_structure-1]->val[0];
	loc=(LOCATION *)emalloc(sizeof(LOCATION)+(frequency-1)*sizeof(int));
	loc->metaID=metaID;
	loc->filenum=filenum;
	loc->structure=structure;
	loc->frequency=frequency;
	for(i=0;i<frequency;i++)
	{
		uncompress2(tmp,p);
		loc->position[i]=tmp;
	}
	return loc;
}

/* 09/00 Jose Ruiz
** Compress all location data of an entry except for 
** the last one (the current filenum)
*/
void CompressPrevLocEntry(SWISH *sw,IndexFILE *indexf,ENTRY *e)
{
int i,max_locations;
	max_locations=e->u1.max_locations;
	for(i=e->currentlocation;i<max_locations;i++)
	{
			/* Compress until current filenum */
		if(e->locationarray[i]->filenum==sw->filenum) return;
		e->locationarray[i]=(LOCATION *)compress_location(sw,indexf,e->locationarray[i]);
	}
}

/* 09/00 Jose Ruiz
** Compress all non yet compressed location data of an entry
*/
void CompressCurrentLocEntry(SWISH *sw,IndexFILE *indexf,ENTRY *e)
{
int i,max_locations;
	max_locations=e->u1.max_locations;
	for(i=e->currentlocation;i<max_locations;i++)
	{
		e->locationarray[i]=(LOCATION *)compress_location(sw,indexf,e->locationarray[i]);
	}
}

/* 09/00 Jose Ruiz
** Function to swap location data to a temporal file to save
** memory. Unfortunately we cannot use it with IgnoreLimit option
** enabled.
** The data has been compressed previously in memory.
** Returns the pointer to the file.
*/
long SwapLocData(SWISH *sw,unsigned char *buf,int lenbuf)
{
long pos;
		/* Check if we have IgnoreLimit active */
		/* If it is active, we can not swap the location info */
		/* to disk because it can be modified later */
	if(sw->plimit!=NO_PLIMIT) 
		return (long)buf;  /* do nothing */
	if(!sw->fp_loc_write)sw->fp_loc_write=fopen(sw->swap_location_name,FILEMODE_WRITE);
	if (!sw->fp_loc_write)
	{
		sw->errorstr=BuildErrorString(sw->errorstr,&sw->lenerrorstr,"Error: Could not create temp file %s",sw->swap_location_name);
		progerr(sw->errorstr);
	}
	pos=ftell(sw->fp_loc_write);
	if(fwrite(&lenbuf,1,sizeof(int),sw->fp_loc_write)!=sizeof(int))
		progerr("Cannot write to swap file");	
	if(fwrite(buf,1,lenbuf,sw->fp_loc_write)!=(unsigned int)lenbuf)
		progerr("Cannot write to swap file");	
	return pos;
}

/* 09/00 Jose Ruiz
** Gets the location data from the swap file
** Returns a memory compressed location data
*/
unsigned char *unSwapLocData(SWISH *sw,long pos)
{
unsigned char *buf;
int lenbuf;
		/* Check if we have IgnoreLimit active */
		/* If it is active, we can not swap the location info */
		/* to disk because it can be modified later */
	if(sw->plimit!=NO_PLIMIT) 
		return (unsigned char *)pos;  /* do nothing */
	if(!sw->fp_loc_read)
	{
		fclose(sw->fp_loc_write);
		sw->fp_loc_read=fopen(sw->swap_location_name,FILEMODE_READ);
		if (!sw->fp_loc_read)
		{
			sw->errorstr=BuildErrorString(sw->errorstr,&sw->lenerrorstr,"Error: Could not open temp file %s",sw->swap_location_name);
			progerr(sw->errorstr);
		}
	}
	fseek(sw->fp_loc_read,pos,SEEK_SET);
	fread(&lenbuf,sizeof(int),1,sw->fp_loc_read);	
	buf=(unsigned char *)emalloc(lenbuf);
	fread(buf,lenbuf,1,sw->fp_loc_read);	
	return buf;
}

/* 09/00 Jose Ruiz
** Function to save all file info and properties of a document to disk
** to save memory while indexing
*/
void SwapFileData(SWISH *sw, struct file *filep)
{
unsigned char *buffer;
int sz_buffer,tmp;
	if (!sw->fp_file_write) sw->fp_file_write=fopen(sw->swap_file_name,FILEMODE_WRITE);
	if (!sw->fp_file_write)
	{
		sw->errorstr=BuildErrorString(sw->errorstr,&sw->lenerrorstr,"Error: Could not create temp file %s",sw->swap_file_name);
		progerr(sw->errorstr);
	}
		
	buffer=buildFileEntry(filep->fi.filename, sw->fp_file_write, &filep->docProperties, filep->fi.lookup_path,&sz_buffer);
	tmp=sz_buffer+1;
	compress1(tmp,sw->fp_file_write);   /* Write len */
	fwrite(buffer,sz_buffer,1,sw->fp_file_write);
	fputc(0,sw->fp_file_write);    /* Important delimiter -> No more props */
	free(buffer);
	freefileinfo(filep);
}


/* 09/00 Jose Ruiz
** Function to get all the file info and properties of a document
** from the swap file
*/
struct file *unSwapFileData(SWISH *sw)
{
struct file *fi;
int len,len1,len2,len3,mtime,begin,bytes,lookup_path;
char *buffer,*p;
char *buf1,*buf2,*buf3;
	fi=(struct file *)emalloc(sizeof(struct file));
	if (!sw->fp_file_read)
	{
		fclose(sw->fp_file_write);
		sw->fp_file_read=fopen(sw->swap_file_name,FILEMODE_READ);
		if (!sw->fp_file_read)
		{
			sw->errorstr=BuildErrorString(sw->errorstr,&sw->lenerrorstr,"Error: Could not open temp file %s",sw->swap_file_name);
			progerr(sw->errorstr);
		}
	}
	uncompress1(len,sw->fp_file_read);
	p=buffer=emalloc(len);
        fread(buffer,len,1,sw->fp_file_read);   /* Read all data */
	uncompress2(lookup_path,p);
	lookup_path--;
        uncompress2(len1,p);   /* Read length of filename */
        buf1 = emalloc(len1);
        memcpy(buf1,p,len1);   /* Read filename */
	p+=len1;

	fi->fi.lookup_path=lookup_path;
        fi->fi.filename = buf1;
        fi->fi.title = NULL;
        fi->fi.summary = NULL;

	fi->docProperties = fetchDocProperties(p);

	efree(buffer);
	return fi;
}

int get_lookup_index(struct int_lookup_st **lst,int n,int *values)
{
unsigned int i,hash;
struct int_st *is=NULL,*tmp=NULL;
	for(i=0,hash=0;i<n;i++)
		hash=hash*17+(values[i]-1);
	hash=hash%HASHSIZE;
	
	if (!*lst)
	{
		*lst=(struct int_lookup_st *)emalloc(sizeof(struct int_lookup_st));
		(*lst)->n_entries=1;
		for(i=0;i<HASHSIZE;i++) (*lst)->hash_entries[i]=NULL;
		is=(struct int_st *)emalloc(sizeof(struct int_st)+sizeof(int)*(n-1));
		is->index=0;
		is->next=NULL;
		for(i=0;i<n;i++) is->val[i]=values[i];
		(*lst)->all_entries[0]=is;
		(*lst)->hash_entries[hash]=is;
	} else {
		for(tmp=(*lst)->hash_entries[hash];tmp;tmp=tmp->next)
		{
			for(i=0;i<n;i++)
				if(tmp->val[i]!=values[i]) break;
			if(i==n) break;
		}
		if(tmp)
		{
			return tmp->index;
		} else {
			is=(struct int_st *)emalloc(sizeof(struct int_st)+sizeof(int)*(n-1));
			is->index=(*lst)->n_entries;
			for(i=0;i<n;i++) is->val[i]=values[i];
			is->next=(*lst)->hash_entries[hash];
			(*lst)->hash_entries[hash]=is;
			*lst=(struct int_lookup_st *)erealloc(*lst,sizeof(struct int_lookup_st)+(*lst)->n_entries*sizeof(struct int_st *));
			(*lst)->all_entries[(*lst)->n_entries++]=is;
		}
	}
	return ((*lst)->n_entries-1);
}

	
int get_lookup_path(struct char_lookup_st **lst,char *path)
{
unsigned int i,hashval;
struct char_st *is=NULL,*tmp=NULL;
	hashval=hash(path);
	
	if (!*lst)
	{
		*lst=(struct char_lookup_st *)emalloc(sizeof(struct char_lookup_st));
		(*lst)->n_entries=1;
		for(i=0;i<HASHSIZE;i++) (*lst)->hash_entries[i]=NULL;
		is=(struct char_st *)emalloc(sizeof(struct char_st));
		is->index=0;
		is->next=NULL;
		is->val=estrdup(path);
		(*lst)->all_entries[0]=is;
		(*lst)->hash_entries[hashval]=is;
	} else {
		for(tmp=(*lst)->hash_entries[hashval];tmp;tmp=tmp->next)
			if((strlen(tmp->val)==strlen(path)) && (strcmp(tmp->val,path)==0)) break;
		if(tmp)
		{
			return tmp->index;
		} else {
			is=(struct char_st *)emalloc(sizeof(struct char_st));
			is->index=(*lst)->n_entries;
			is->val=estrdup(path);
			is->next=(*lst)->hash_entries[hashval];
			(*lst)->hash_entries[hashval]=is;
			*lst=(struct char_lookup_st *)erealloc(*lst,sizeof(struct char_lookup_st)+(*lst)->n_entries*sizeof(struct char_st *));
			(*lst)->all_entries[(*lst)->n_entries++]=is;
		}
	}
	return ((*lst)->n_entries-1);
}

