/*++ 
Copyright (c) 2013 WonderMedia Technologies, Inc.

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software 
Foundation, either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details. You
should have received a copy of the GNU General Public License along with this
program. If not, see http://www.gnu.org/licenses/>.

WonderMedia Technologies, Inc.
4F, 531, Chung-Cheng Road, Hsin-Tien, Taipei 231, R.O.C.
--*/
#include <common.h>
#include <command.h>
#include "op_list.h"

#define DEBUG_WMT_CMD_SYNCENV 0
#if DEBUG_WMT_CMD_SYNCENV
#define dbg(fmt, args...) printf("[%s:%d]:" fmt, __FUNCTION__ ,__LINE__, ## args)
#define dbgc(fmt, args...) printf(fmt,## args)
#else
#define dbg(fmt, args...) 
#endif

#ifndef errlog
#define errlog(fmt, args...) printf("[%s:%d]:" fmt, __FUNCTION__,__LINE__, ## args)
#endif


#define BLACKLIST_LINE_MAX	512

typedef struct _EnvNode{
    char * varName;
    char * varValue;
    struct list_head theNode;
}EnvNode;

static int getNilIdx(char *env, int startIdx){     
     while(env[startIdx] != '\0')
         startIdx++;
     return startIdx;
}

static int getFirstEqlIdx(char *env, int startIdx){
     int idx = startIdx;
     while(env[idx] != '='){
         if(env[idx] == '\0'){
             return -1;
         }
         idx++;
     }
     return idx;
}

static int deleteEnvNode(EnvNode *node){
     if(node->varName)
        free(node->varName);
     if(node->varValue)
        free(node->varValue);
     free(node);
     return 0;
}

static EnvNode * newEnvNode(){
     EnvNode * node = (EnvNode *)malloc(sizeof(EnvNode));
     if(node == NULL){
		 errlog("Memory error\n");
         return NULL;
     }
     node->varName = NULL;
     node->varValue = NULL;
     return node;
}


static EnvNode * getNode(char *p, int idx, int eqlidx, int nilidx){
     //assert(eqlidx <= nilidx && idx < eqlidx);
     EnvNode * node = newEnvNode();
     int nameLen = eqlidx - idx;
     int varLen = nilidx - eqlidx - 1;
     if(node == NULL)
         return NULL;
     
     if(varLen == 0 || eqlidx == -1){ 
        node->varValue = NULL;
     }else{
        node->varValue = (char*)malloc(varLen+1);
        if(node->varValue == NULL){
           deleteEnvNode(node);
           return NULL;
        }        
        memset(node->varValue, 0, varLen+1);   
        //p + eqlidx + 1 to escape '=' sign        
        memcpy(node->varValue, p+eqlidx+1, varLen);
     }
     
     node->varName = (char*)malloc(nameLen+1);
     if(node->varName == NULL){
        deleteEnvNode(node);
        return NULL;
     }             
     memset(node->varName, 0, nameLen+1);        
     memcpy(node->varName, p+idx, nameLen);     
     return node;
}


static int parse_envfile(unsigned char *p,struct list_head *envNodes){
     int idx = 0;
	 int ret = 0;
 
     list_init(envNodes);
     
     while(1){
         int nilidx = getNilIdx(p, idx);
         int eqlidx = getFirstEqlIdx(p, idx); 

         //end of env file
         if(nilidx <= idx) break;
         if(eqlidx <= idx){
             errlog("Invalid env equation in %s(%d, %d, %d)\n", p+idx, idx, eqlidx, nilidx);
             ret = -1;
             goto Exit;
         }
        
         EnvNode * eNode = getNode(p, idx, eqlidx, nilidx);
         if(eNode == NULL){
             ret = -1;
             goto Exit;
         }
         list_add(&eNode->theNode, envNodes);
         //there are at least 2 '\0's at the end of env file, as a result idx will never be larger than flen-5
         idx = nilidx + 1;
     }     
     
Exit:
     return ret;
}

static void dump_envNode(EnvNode * enode){
	int i = 0;
	dbg("%s=%s\n", enode->varName, enode->varValue == NULL ? "NULL":enode->varValue);
}

static void dump_envNodes(struct list_head* envNodes){
        struct list_head * node, *node2;
        EnvNode * enode;
        list_for_each_safe(node, node2, envNodes) {
            enode = list_entry(node, EnvNode, theNode);
            dump_envNode(enode);
        }
}



static int parse_reserveFile(unsigned char* listfileaddr, struct list_head *envNodes)
{
	int ret = -1;
    unsigned char* line = NULL;
	int lineno = 0;
	unsigned char* pointer = listfileaddr;
	EnvNode * eNode = NULL;	

	
	line = malloc( BLACKLIST_LINE_MAX );
	if( NULL == line ){
		ret = -1;
		goto exit;
	}
	memset(line,0,BLACKLIST_LINE_MAX);
	lineno = 0;
    list_init(envNodes);

	//dbg("new2 blacklist:\n");
	pointer = listfileaddr;
	do{
		//dbgc("0x%X ",*pointer);
		if( ! ( *pointer == '\r' || *pointer == ' ' || *pointer == '\t' ) )
		{
			if( *pointer == '\n' ){
				line[lineno] = '\0';
				//dbgc("\n");
				if(line[0] != '#' && lineno > 0 ){
					//dbg("line[0]:0x%X  lineno:%d \n",line[0],lineno);
					eNode = newEnvNode();
					if(!eNode){
						ret = -1;
						errlog("eNode malloc error\n");
						goto exit;
					}
					eNode->varName = (char*)malloc(lineno + 1);
					if(! eNode->varName ){
						ret = -1;
						errlog(" nNode varName malloc error\n");
						goto exit;
					}
					memset(eNode->varName,0,lineno + 1);
					strcpy(eNode->varName,line);
					list_add(&eNode->theNode, envNodes);
					eNode = NULL;					
					//dbg("eNode->varName:%s\n",eNode->varName);
				}
				memset(line,0,BLACKLIST_LINE_MAX);
				lineno = 0;				
			}else{
				line[lineno++] = *pointer;
				//dbgc("%c",line[lineno]);
			}
			
		}
		pointer++;
	}while( *pointer != '*' || *(pointer+1) != '*' || *(pointer+2) != '*' );

exit:
	if(eNode){
		free(eNode);
		eNode = NULL;
	}
	if(line){
		free(line);
		line = NULL;
	}
    return ret;
}



static void compareEnvNodes(struct list_head *envNodes, struct list_head *changedEnvNodes){
    struct list_head * node, *node2;
    struct list_head * cNode, * cNode2;
    EnvNode * eNode, *cEnvNode;

    //update env which exists both in envNodes and changedEnvNodes but may have different values
    list_for_each_safe(node, node2, envNodes) {
         eNode = list_entry(node, EnvNode, theNode);
         list_for_each_safe(cNode, cNode2, changedEnvNodes) {
              cEnvNode = list_entry(cNode, EnvNode, theNode);
              if(!strcmp(eNode->varName, cEnvNode->varName)){
                  //found same name env   
                  if(cEnvNode->varValue != NULL && eNode->varValue != NULL){
                      if(!strcmp(eNode->varValue, cEnvNode->varValue)){
                          //same value, remove from list
                          list_del(&eNode->theNode);
                          list_del(&cEnvNode->theNode);
                          deleteEnvNode(eNode);
                          deleteEnvNode(cEnvNode);
                      }else{
                          //different value, use value in changedEnvNodes                         
                          list_del(&eNode->theNode);
                          deleteEnvNode(eNode);
                      }
                  }else{
                      //value is nil, this is strange, let it be.
                  }
                  break;                                        
              }
         }
    }

    //what remains in envNodes should be deleted, what remains in changedEnvNodes should be added
    list_for_each_safe(node, node2, envNodes){
         eNode = list_entry(node, EnvNode, theNode);
         list_del(&eNode->theNode);
         if(eNode->varValue) free(eNode->varValue);
         eNode->varValue = NULL;
         list_add(&eNode->theNode, changedEnvNodes);
    }
}

static int needReserve(const char *name, struct list_head *reservedEnvNodes){
    struct list_head *node, *node2;
    EnvNode *envNode;
    list_for_each_safe(node, node2, reservedEnvNodes){
        envNode = list_entry(node, EnvNode, theNode);
        if(!strcmp(envNode->varName, name))
           return 1;
    }
    return 0;
}

static int set_env(char *var, char *val)
{
	char *setenv[4]  = { "setenv", NULL, NULL, NULL, };

	setenv[1] = var;
    if(val != NULL){
		setenv[2] = val;
		return do_setenv(NULL, 0, 3, setenv);
    }else{
        return do_setenv(NULL, 0, 2, setenv);
	}
	
}

static int save_env()
{
	char arg[] = "saveenv";
	return do_saveenv(NULL,0,1,arg);
}



static void updateNode2Env(struct list_head *changedEnvNodes, struct list_head *reservedEnvNodes){
    struct list_head * node, *node2;
    EnvNode * envNode;
    list_for_each_safe(node, node2, changedEnvNodes){
         envNode = list_entry(node, EnvNode, theNode);
         if(!needReserve(envNode->varName, reservedEnvNodes)){
             errlog("setenv: %s=%s\n", envNode->varName, envNode->varValue==NULL?"NULL":envNode->varValue);
             if(set_env(envNode->varName, envNode->varValue)){
                  errlog("input parameter error (%s=%s) \n", envNode->varName, envNode->varValue==NULL?"NULL":envNode->varValue);
             }
         }else{
             dbg("skip %s=%s\n", envNode->varName, envNode->varValue==NULL?"NULL":envNode->varValue);
         }
    }
}

static void free_envNodes(struct list_head * envNodes){
        struct list_head * node, *node2;
        EnvNode * enode;
        list_for_each_safe(node, node2, envNodes) {
            enode = list_entry(node, EnvNode, theNode);
            list_del(&enode->theNode);
            deleteEnvNode(enode);
        }
}


int do_syncenv(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int i = 0;
	int ret = 0;
	int result = 0;
	unsigned char* envfileaddr = NULL;
	unsigned char* listfileaddr = NULL;
	unsigned char* pointer = NULL;
	char *endp;
	
	for( i = 0; i < argc; i++){
		dbg("argv[%d]:%s\n",i,argv[i]);
	}

	if( argc >= 3 ){
		envfileaddr = (unsigned char*)simple_strtoul(argv[1], &endp, 16);
		dbg("envfileaddr:0x%X\n",envfileaddr);
		listfileaddr =(unsigned char*)simple_strtoul(argv[2], &endp, 16);
		dbg("listfileaddr:0x%X\n",listfileaddr);
	}

	struct list_head * node, *node2;
	struct list_head * changedNode, * changedNode2;
	EnvNode * envNode, *changedEnvNode;
	LIST_HEAD(envNodes);
	LIST_HEAD(changedEnvNodes);
	LIST_HEAD(reservedEnvNodes);

	parse_envfile( env_get_addr(0), &envNodes);
    parse_envfile( envfileaddr+5, &changedEnvNodes);
	parse_reserveFile(listfileaddr, &reservedEnvNodes);


	dbg("\nenv file:\n");
	dump_envNodes(&envNodes);
	dbg("\nchanged env file:\n");
	dump_envNodes(&changedEnvNodes);
    dbg("\n 2nd time: env to be reserved:\n");
    dump_envNodes(&reservedEnvNodes);
	dbg("\n\n");
	
	compareEnvNodes(&envNodes, &changedEnvNodes);
    updateNode2Env(&changedEnvNodes, &reservedEnvNodes);	

    free_envNodes(&envNodes);
    free_envNodes(&changedEnvNodes);
    free_envNodes(&reservedEnvNodes);

	saveenv();
	
end:
	return ret;	
}

U_BOOT_CMD(
	syncenv,	CFG_MAXARGS,	1,	do_syncenv,
	"syncenv fileaddress\n",
	"syncenv fileaddress filelength\n"
	"	fileaddress: u-boot env file load into the RAM\n"
	"	thd \"\0\0\" is the end of the env file.\n"
);

