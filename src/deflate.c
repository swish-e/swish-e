
#include <stdio.h>
#include <stdlib.h>

#include "swish.h"
#include "mem.h"
#include "compress.h"
#include "error.h"
#include "deflate.h"

#define MemCopy memcpy
#define MemChar memchr
#define Min(a,b) (a<b?a:b);
#define MAX_DICT_ENTRIES 0x8000

struct buffer_pool *new_buffer_pool(int max_size, int min_match, int max_match)
{
struct buffer_pool *bp;
	bp=(struct buffer_pool *)emalloc(sizeof(struct buffer_pool));
	bp->dict=NULL;
	bp->lookup=NULL;
	bp->max_size=max_size;
	bp->total_size=0;
	bp->head=NULL;
	bp->tail=NULL;
	bp->min_match=min_match;
	bp->max_match=max_match;
	return bp;
}

void push_buffer_pool(struct buffer_pool *bp,struct buffer_st *buffer)
{
	buffer->next=NULL;
	if(!bp->head)
		bp->head=buffer;
	if(bp->tail)
		bp->tail->next=buffer;
	bp->tail=buffer;
	bp->total_size+=buffer->sz_str;
}

struct buffer_st *shift_buffer_pool(struct buffer_pool *bp)
{
struct buffer_st *buffer;
	if(!bp->head)
		return NULL;
	buffer=bp->head;
	bp->head=buffer->next;
	bp->total_size-=buffer->sz_str;
	if(!bp->head)bp->tail=NULL;
	return buffer;
}

/*  Just to debug - Normally commented
void print_dictionary(struct dict_lookup_st *dict)
{
int i,j,k;
struct dict_st *tmp=NULL;
	if(!dict) return;
	for(i=0;i<256;i++){
	   for(k=0;k<256;k++){
		for(tmp=dict->entries[i][k];tmp;tmp=tmp->next){
			for(j=0;j<tmp->sz_val;j++) putchar(tmp->val[j]);
			putchar('\n');
		}
            }
	}
}
*/

int best_match_dictionary(struct dict_lookup_st *dict,unsigned char *str, int *sz_str)
{
int i, hashval0, hashval1, len, len_match,index_match;
struct dict_st *tmp=NULL;
	index_match=-1;
	if(!dict) return -1;
        hashval0=(int)((unsigned char)str[0]);
        hashval1=(int)((unsigned char)str[1]);
        for(len_match=2,tmp=dict->entries[hashval0][hashval1];tmp;tmp=tmp->next)
	{
		len=Min(tmp->sz_val,*sz_str);
		for(i=2;i<len;i++)
			if(tmp->val[i] != str[i]) break;
		if(i<len_match) break;
		len_match=i;
		index_match=tmp->index;
	}
	*sz_str=len_match;
        return index_match;
}

int lookup_dictionary(struct dict_lookup_st **dict,unsigned char *str, int sz_str)
{
int i, hashval0, hashval1, len, res=0, found;
struct dict_st *tmp=NULL;
	if(!*dict) return -1;
        hashval0=(int)((unsigned char)str[0]);
        hashval1=(int)((unsigned char)str[1]);
        for(found=0,tmp=(*dict)->entries[hashval0][hashval1];tmp;tmp=tmp->next)
	{
		len=Min(tmp->sz_val,sz_str);
		for(i=2;i<len;i++)
			if((res=(int)((unsigned char)tmp->val[i])-(int)((unsigned char)str[i]))) break;
		if(!res)
		{
			if(len<(sz_str))    /* Use the longest string */
			{
				efree(tmp->val);
				tmp->sz_val=sz_str;
				tmp->val=(unsigned char *)emalloc(sz_str);
				MemCopy(tmp->val,str,sz_str);
			}
			found=1;
			break;
		} else if(res<0) break;
	}
        if(tmp && found)
                return tmp->index;
	else
		return -1;
}

int add_dictionary(struct dict_lookup_st **dict,unsigned char *str, int sz_str)
{
int i,j,hashval0,hashval1,len,res=0;
struct dict_st *is=NULL,*tmp=NULL,*tmp2=NULL;
	hashval0=(int)((unsigned char)str[0]);
	hashval1=(int)((unsigned char)str[1]);
        if (!*dict)
        {
                *dict=(struct dict_lookup_st *)emalloc(sizeof(struct dict_lookup_st));
                (*dict)->n_entries=1;
                for(i=0;i<256;i++) 
                	for(j=0;j<256;j++) 
				(*dict)->entries[i][j]=NULL;
                is=(struct dict_st *)emalloc(sizeof(struct dict_st));
                is->index=0;
                is->next=NULL;
                is->val=(unsigned char *)emalloc(sz_str);
                is->sz_val=sz_str;
		MemCopy(is->val,str,sz_str);
                (*dict)->all_entries[0]=is;
                (*dict)->entries[hashval0][hashval1]=is;
        } else {
                for(tmp2=NULL,tmp=(*dict)->entries[hashval0][hashval1];tmp;tmp=tmp->next)
		{
			len=Min(tmp->sz_val,sz_str);
        		res=memcmp(tmp->val+2,str+2,len-2);
			if(!res)
			{
				if(len<sz_str)    /* Use the longest string */
				{
					efree(tmp->val);
					tmp->sz_val=sz_str;
					tmp->val=(unsigned char *)emalloc(sz_str);
					MemCopy(tmp->val,str,sz_str);
				}
				break;
			} else if(res<0) {
				tmp2=tmp;
			} else break;
		}
                if(tmp && !res)
                {
                        return tmp->index;
                } else {
                        is=(struct dict_st *)emalloc(sizeof(struct dict_st));
                        is->index=(*dict)->n_entries;
                	is->val=(unsigned char *)emalloc(sz_str);
                	is->sz_val=sz_str;
			MemCopy(is->val,str,sz_str);
			if(tmp2)
                        	tmp2->next=is;
			else
                        	(*dict)->entries[hashval0][hashval1]=is;
                       	is->next=tmp;
                        *dict=(struct dict_lookup_st *)erealloc(*dict,sizeof(struct dict_lookup_st)+(*dict)->n_entries*sizeof(struct dict_st *));
                        (*dict)->all_entries[(*dict)->n_entries++]=is;
                }
        }
        return ((*dict)->n_entries-1);
}

/* Obsolete - Not used
int find_best_match(struct dict_lookup_st **dict, struct buffer_st **buffer, unsigned char *current,int min_match, int max_match, int *sz_best_match, int use_dict)
{
unsigned char *scan;
int i, j, dict_id,dict_id_best_match, start_match,end_match, res,rest;
unsigned char *best_match;
	*sz_best_match=0;
	dict_id_best_match=-1;
	if(use_dict)
		for(i=min_match;i<=max_match;i++)
		{
			if((dict_id=lookup_dictionary(dict,current,i))>=0)
			{
				if((*dict)->all_entries[dict_id]->sz_val>*sz_best_match)
				{
					*sz_best_match=(*dict)->all_entries[dict_id]->sz_val;
					dict_id_best_match=dict_id;
				}
			} 
		}
	if((*sz_best_match))
		start_match=(*sz_best_match)+1;
	else
		start_match=min_match;
	for(best_match=NULL,scan=(*buffer)->str;;scan+=2)
	{
		rest=(*buffer)->sz_str-(scan-(*buffer)->str);
		if(!(scan=MemChar(scan,*current,rest-start_match+1))) break;
		end_match=Min(max_match,rest);
		if (start_match>end_match)break;
		if(scan[1] != current[1]) continue;
		end_match--;
		for(i=start_match-2;i<end_match;i++)
		{
			for(j=0,res=0;j<i;j++)
				if((res=(int)((unsigned char)scan[j+2])-(int)((unsigned char)current[j+2]))) break;
			if(!res)
			{
				*sz_best_match=i+2;
				best_match=scan;
				start_match=i+2;
			} else break;
		}
	}
	if (best_match)
		dict_id_best_match=add_dictionary(dict,best_match,*sz_best_match);
	return dict_id_best_match;
}
*/

/* Since the memmem function has a couple o bugs and it is not ANSI
I have made my own */
unsigned char *MemMem(unsigned char *str,int sz_str,unsigned char *patt,int sz_patt)
{
unsigned char *p;
register int i;
int rest;
        for(p=str;;)
        {
                rest=sz_str-(p-str);
                if(!(p=MemChar(p,patt[sz_patt-1],rest)) || (((p-str)+1)<sz_patt)) return NULL;
                for(i=1;i<sz_patt;i++)
                {
                        if(*(p-i)!=patt[sz_patt-i-1])
                        {
                                p++;
                                break;
                        }
                }
                if(i==sz_patt) return (p-sz_patt+1);
        }
}

void match_buffers(struct dict_lookup_st **dict,struct buffer_st **buffer1,struct buffer_st **buffer2, int min_match, int max_match, int use_dict)
{
unsigned char *current;
int best_match_id;
int sz_best_match;
unsigned char *scan,*scan1;
int i,start_match,end_match, res,rest;
unsigned char *best_match;
	for(current=(*buffer2)->str;((current-(*buffer2)->str)+min_match)<=(*buffer2)->sz_str;)
	{
		max_match=Min(max_match,(*buffer2)->sz_str-(current-(*buffer2)->str));
		sz_best_match=0;
		best_match_id=-1;
		if(sz_best_match)
			start_match=sz_best_match+1;
		else
			start_match=min_match;
		/* Now look at the buffer */
		for(best_match=NULL,scan=(*buffer1)->str,scan1=NULL;;)
		{
			rest=(*buffer1)->sz_str-(scan-(*buffer1)->str);
			end_match=Min(max_match,rest);
			if (start_match>end_match)break;
			if(!(scan=MemMem(scan,rest,current,start_match))) break;
			sz_best_match=start_match;
			best_match=scan;
			for(i=start_match,res=0;i<end_match;i++)
			{
				if((res=(int)((unsigned char)scan[i])-(int)((unsigned char)current[i]))) break;
				else
				{
					sz_best_match=i;
					start_match=i;
				} 
			}
			scan+=i;
		}
		if (best_match)
			best_match_id=add_dictionary(dict,best_match,sz_best_match);
		if(best_match_id>=0)
		{
			current+=sz_best_match;
		} else current++;
	}

}

struct buffer_st *init_buffer(unsigned char *str,int sz_str, long *pos)
{
struct buffer_st *buffer;
	buffer=(struct buffer_st *)emalloc(sizeof(struct buffer_st));
	buffer->next=NULL;
	buffer->str=str;
	buffer->sz_str=sz_str;
	buffer->pos=pos;
	return buffer;
}

void compress_buffer(struct buffer_pool *bp,struct buffer_st **buffer,int min_match,int max_match, int use_dict)
{
struct buffer_st *tmp;
	for(tmp=bp->head;tmp;tmp=tmp->next)
	{
		match_buffers(&bp->dict,&tmp, buffer,min_match, max_match, use_dict);
	}
}


void compress_buffer_to_file(struct dict_lookup_st *dict,struct int_lookup_st **ilookup,struct buffer_st *buffer, int min_match, int max_match,FILE *fp)
{
unsigned char *current,*currentNoComp;
int i, j, lookup, ivar[2], dict_id, end_match,offset,len;
int no_compressed,len_no_comp;
	for(current=buffer->str,len=buffer->sz_str,no_compressed=0,currentNoComp=NULL;;)
	{
		offset=current-buffer->str;
		end_match=Min((len-offset),max_match);
		if(end_match<min_match) 
		{
			if(len>offset)
			{
				if(!no_compressed)
				{
					no_compressed=1;
					currentNoComp=current;
				}
				current=buffer->str+len;
			}
			break;
		}
 	        if((dict_id=best_match_dictionary(dict,current,&end_match))>=0)
        	{
			ivar[0]=dict_id;
			ivar[1]=end_match;
			if(end_match>=min_match && (lookup=get_lookup_index(ilookup,2,ivar))<MAX_DICT_ENTRIES)
			{
				if(no_compressed)
				{
					len_no_comp=current-currentNoComp;
					for(i=len_no_comp,j=0;i;i--,j++)
					{
						if(!(j%127))
						{
							if(i>=127) fputc(127,fp);
							else fputc(i,fp);
						}
						fputc((int)currentNoComp[j],fp);
					}
					no_compressed=0;
					currentNoComp=NULL;
				}
				fputc(((lookup>>8)|128),fp);
				fputc((lookup & 0xFF),fp);
				current += end_match;
        	        } else {
				if(!no_compressed)
				{
					no_compressed=1;
					currentNoComp=current;
				}
				current++;
			}
        	} else {
			if(!no_compressed)
			{
				no_compressed=1;
				currentNoComp=current;
			}
			current++;
		}
 	}
	if(no_compressed)
	{
		len_no_comp=current-currentNoComp;
		for(i=len_no_comp,j=0;i;i--,j++)
		{
			if(!(j%127))
			{
				if(i>=127) fputc(127,fp);
				else fputc(i,fp);
			}
			fputc((int)currentNoComp[j],fp);
		}
	}
	fputc(0,fp);
}


int uncompress_buffer_from_file(unsigned char **dict,struct buffer_st **buffer, FILE *fp)
{
int c;
int cur_offset,max_cur_len, dict_id, size;
unsigned char *p,*q;
	p=(unsigned char *)emalloc((max_cur_len=300));
	for(cur_offset=0;;)
	{
		c=fgetc(fp);
		if(!c || c==EOF) 
		{
			break;
		}
		if(c & 128) 
		{
			dict_id = ((c & 127)<<8) + fgetc(fp);
			size = (int)((unsigned char)dict[dict_id][0]);
			if(size>(max_cur_len-cur_offset))
				p=erealloc(p,(max_cur_len+=size));
			MemCopy(p+cur_offset,dict[dict_id]+1,size);
		} else {
			size = c;
			if(size>(max_cur_len-cur_offset))
				p=erealloc(p,(max_cur_len+=size));
			fread(p+cur_offset,1,size,fp);
		}
		cur_offset+=size;
	}
	q=emalloc(cur_offset);
	MemCopy(q,p,cur_offset);
	efree(p);
	*buffer=init_buffer(q,cur_offset,NULL);
	return cur_offset;
}

struct buffer_pool *zfwrite(struct buffer_pool *bp, unsigned char *buf, int sz_buf, long *pos, FILE *fp)
{
struct buffer_st *buffer=NULL;
int min_match=DEFLATE_MIN_PATTERN_SIZE;
int max_match=DEFLATE_MAX_PATTERN_SIZE;
int use_dict=0;
unsigned char *str;
	if(!bp)
		bp=new_buffer_pool(DEFLATE_MAX_WINDOW_SIZE,min_match,max_match);
	str=(unsigned char *)emalloc(sz_buf);
	MemCopy(str,buf,sz_buf);
	buffer=init_buffer(str,sz_buf,pos);
	if(bp->head!=bp->tail)
		compress_buffer(bp,&buffer,min_match,max_match, use_dict);
	push_buffer_pool(bp,buffer);
	while(bp->total_size>bp->max_size)
	{
		if(!(buffer=shift_buffer_pool(bp))) break;
		*(buffer->pos) = ftell(fp);
		compress_buffer_to_file(bp->dict,&bp->lookup,buffer,min_match,max_match,fp);
		efree(buffer->str);
		efree(buffer);
	}
	return bp;
}

void zfflush(struct buffer_pool *bp, FILE *fp)
{
struct buffer_st *buffer;
	while((buffer=shift_buffer_pool(bp))) 
	{
		*(buffer->pos) = ftell(fp);
		compress_buffer_to_file(bp->dict,&bp->lookup,buffer,bp->min_match,bp->max_match,fp);
		efree(buffer->str);
		efree(buffer);
	}

}

unsigned char *zfread(unsigned char **dict,int *sz_size,FILE *fp)
{
struct buffer_st *buffer;
char *data;
	*sz_size=uncompress_buffer_from_file(dict,&buffer,fp);
	data=buffer->str;
	efree(buffer);
	return data;
}

void printdeflatedictionary(struct buffer_pool *bp,IndexFILE *indexf)
{
int i,n,dict_id,length;
FILE *fp=indexf->fp;
	indexf->offsets[DEFLATEDICTPOS] = ftell(fp);
	if(!bp->lookup)		/* Empty dictionary */
	{			
		fputc(0,fp);
		return;
	}
	/* First write the lookup table */
	n=bp->lookup->n_entries;
        compress1(n,fp);
        for(i=0;i<n;i++)
        {
                dict_id=bp->lookup->all_entries[i]->val[0];   /* dict_id */
                length=bp->lookup->all_entries[i]->val[1];   /* length */
                compress1(length,fp);
		fwrite(bp->dict->all_entries[dict_id]->val,length,1,fp);
        }
}

void readdeflatepatterns(IndexFILE *indexf)
{
int i,n,len;
FILE *fp=indexf->fp;
unsigned char **dict;
	fseek(fp,0,0);
	fseek(fp,indexf->offsets[DEFLATEDICTPOS],0);
        uncompress1(n,fp);
	if(n)
	{
		dict=(unsigned char **)emalloc(n*sizeof(unsigned char *));
		for(i=0;i<n;i++)
		{
			uncompress1(len,fp);
			if(len>0xFF)
				progerr("Internal error in readdeflatepatterns\n.\n");
			dict[i]=emalloc(len+1);
			dict[i][0]=(unsigned char)len;
			fread(dict[i]+1,len,1,fp);
		}
	} else dict=NULL;
	indexf->dict=dict;
}

