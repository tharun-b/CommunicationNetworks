#include <string>
#include <iostream>
using namespace std;
#define MAXPATHSIZE 50
#define MAXTIMESIZE 50

class Entry{
public:
	string url;
	char* path;
	char* expire;
	Entry *nxt ;
	Entry *pre ;

	Entry(void): nxt(NULL), pre(NULL){
		path = (char*) calloc(MAXPATHSIZE, 1);
		expire = (char*) calloc(MAXTIMESIZE, 1);
	}

	Entry(string _url, char* tmppath, char* tmptime): url(_url), nxt(NULL), pre(NULL){
		path = (char*) calloc(MAXPATHSIZE, 1);
		strcpy(path, tmppath);

		expire = (char*) calloc(MAXTIMESIZE, 1);
		strcpy(expire, tmptime);
	}

	~Entry(void){
		free(path);
		free(expire);
	}
};

class Cache{
	// LRU: the most recent used entry is placed in head
public:
	Entry *head;
	int cnt; // count of current entries
	int maxcnt; // max count of entries allowed

	Cache(int maxcnt){
		this->maxcnt = maxcnt;
		this->cnt = 0;
		this->head = new Entry();
	}

	// add the new node to head
	void AddEnt(Entry * NewEntry){
		if(this->cnt == this->maxcnt){
			//delete the tail one
			DelTail();
		}

		char * cpath;
		cpath = (char*) calloc(MAXPATHSIZE, 1);
		cpath = get(NewEntry->url);
		if(cpath == NULL){
			this->cnt++;
			NewEntry->nxt = this->head->nxt;
			NewEntry->pre = this->head;
			if(this->head->nxt != NULL)
				this->head->nxt->pre = NewEntry;
			this->head->nxt = NewEntry;
		}else{
			Entry *tmp = this->head;
			while(tmp!=NULL){
				if(NewEntry->url.compare(tmp->url) != 0){
					tmp = tmp->nxt;
				}else{
					strcpy(tmp->expire, NewEntry->expire);
					UpgEnt(tmp);
					break;
				}
			}
		}
		free(cpath);
	}

	// delete the tail node
	void DelTail(void){
		Entry *tmp = this->head;
		while(tmp->nxt!=NULL)
			tmp = tmp->nxt;
		if(tmp != this->head){
			tmp->pre->nxt = NULL;
			this->cnt--;
		}
	}

	// print the cache
	void PrintCache(void){
		Entry *tmp = this->head;
		int i = 0;
		while(tmp != NULL){
			if(tmp != this->head){
				cout << ++i << ": " << tmp->url << " : "<< tmp->path <<endl<< "         : " << tmp->expire << endl;
			}
			tmp = tmp->nxt;
		}
		cout << cnt << " entries in cache.\n";
	}

	// replace the entry in use to head
	void UpgEnt(Entry * UsingEntry){
		if(UsingEntry == head){

		}else if(UsingEntry->nxt != NULL){
			UsingEntry->pre->nxt = UsingEntry->nxt;
			UsingEntry->nxt->pre = UsingEntry->pre;
			this->cnt--;
			AddEnt(UsingEntry);
		}else if(UsingEntry->nxt == NULL){
			UsingEntry->pre->nxt = NULL;
			this->cnt--;
			AddEnt(UsingEntry);
		}
	}

	// return the path 
	char* get(string QueryURL){
		Entry *tmp = this->head;
		char* retpath;
		retpath = (char*) calloc(MAXPATHSIZE, 1);
		while(tmp!=NULL){
			if(QueryURL.compare(tmp->url) != 0){
				tmp = tmp->nxt;
			}else{
				strcpy(retpath, tmp->path);
				cout<<"ping, path: " << retpath<<" : "<< tmp->path <<endl;
				UpgEnt(tmp); // replace this entry to head
				return retpath;
			}
		}
		return NULL;
	}

	// return the expire time
	char* getexpire(string QueryURL){
		Entry *tmp = this->head;
		char* retexp;
		retexp = (char*) calloc(MAXTIMESIZE, 1);
		strcpy(retexp , "Mon, 17-Oct-2000 19:15:16 GMT");
		while(tmp!=NULL){
			if(QueryURL.compare(tmp->url) != 0){
				tmp = tmp->nxt;
			}else{
				strcpy(retexp, tmp->expire);
				cout<<"ping, expire: " << retexp<<" : "<< tmp->expire <<endl;
				//UpgEnt(tmp); // replace this entry to head
				return retexp;
			}
		}
		return retexp;
	}

	// destructor
	~Cache(void){
		Entry *tmp = this->head;
		while(tmp != NULL){
			if(tmp != this->head){
				//free(tmp->path);
			}
			tmp = tmp->nxt;
		}
	}
};
