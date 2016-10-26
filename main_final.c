#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h> 
#include <sys/time.h>
#include <sys/resource.h>
#include  <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "parse.h"

extern char **environ;
int des = 0;
static void prCmd(Cmd c,int p)
{
  int i,status,count = 0; 
  //pid_t pid;
  //int piped[2];
  int fdout,fdin,input_descriptor = 4,output_descriptor = 4,output_error=0,old_out =4,old_in = 4,old_error,pipe_des=5;	
  /*char str[1000];
  strcpy(str,"");	
  char *curdir = malloc(1000*sizeof(char*));
  char **env;*/		
  char *built_in[9] = {"cd","echo","logout","nice","pwd","setenv","unsetenv","where", "end"};
  if ( c ) {
    //printf("%s%s ", c->exec == Tamp ? "BG " : "", c->args[0]);
    if ( c->in == Tin )
      {
	old_in = dup(0);
	fdin = open(c->infile,O_RDWR,S_IRWXU);
	input_descriptor = dup2(fdin,0);
      }	
    		
    if ( c->out != Tnil )
      switch ( c->out ) {
      case Tout:
	old_out = dup(1);
	fdout = open(c->outfile,O_CREAT |O_TRUNC| O_RDWR,S_IRWXU);
	output_descriptor = dup2(fdout,1);
	break;
      case Tapp:
	old_out = dup(1);
	fdout = open(c->outfile,O_CREAT |O_APPEND| O_RDWR,S_IRWXU);
	output_descriptor = dup2(fdout,1);
	break;
      case ToutErr:
	old_out = dup(1);
	old_error = dup(2);
	fdout = open(c->outfile,O_CREAT |O_TRUNC| O_RDWR,S_IRWXU);
	output_descriptor = dup2(fdout,1);
	output_error = dup2(fdout,2);
	break;
      case TappErr:
	old_out = dup(1);
	old_error = dup(2);
	fdout = open(c->outfile,O_CREAT |O_APPEND| O_RDWR,S_IRWXU);
	output_descriptor = dup2(fdout,1);
	output_error = dup2(fdout,2);
	break;
      case Tpipe:                
	//printf("%s","Tpipe");
	break;
      case TpipeErr:             
	//printf("%s","TpipeErr");
	break;
      default:
	fprintf(stderr, "Shouldn't get here\n");
	exit(-1);
      }
//------------To check for built-in-----------------------
    
    if(isbuiltin(built_in,c) > 0) 	
	shellcommands(built_in,c);
    else
	forking(c,p);	 
//------------------------------------------------------------
	if(output_descriptor == 1)
	 {
	 close(fdout);
	 dup2(old_out,1);
	 close(old_out);
	 }
	else if(output_error == 2)
	 {
	 dup2(old_error,2);
	 close(old_error);
	 }
	else if(input_descriptor == 0)
	 {
	 close(fdin);
	 dup2(old_in,0);
	 close(old_in);
	 }

  }

}

int isbuiltin(char *arr[],Cmd c)
{
int count = 0,i;
for(i=0; i<9;i++)
 {
  if(!strcmp(c->args[0],arr[i]) && c->next == NULL)                    
	 count++;
}
return count;
}

void shellcommands(char *arr[],Cmd c)
{
int i;
char str[1000];
strcpy(str,"");	
char *curdir = malloc(1000*sizeof(char*));
char **env;
int val;
if(!strcmp(c->args[0],"nice"))
  {
    if(c->nargs == 1)
	setpriority(PRIO_PROCESS,0,4);
    else if(c->nargs == 2)
	{
	  val = atoi(c->args[1]);
	  if(val!= 0 || c->args[1]=="0")
		setpriority(PRIO_PROCESS,0,val);
	  else
	    {
		for ( i = 0; c->args[i+1] != NULL; i++ )
                    strcpy(c->args[i],c->args[i+1]);
		c->nargs--;
                c->args[i]=NULL;
	        if(isbuiltin(arr,c) > 0)
		    setpriority(PRIO_PROCESS,0,4);
		prCmd(c,4);		
	    }			
	}
    else
	{
	 val = atoi(c->args[1]);
	   if(val!= 0 || c->args[1]=="0")
	     {
	      for ( i = 0; c->args[i+2] != NULL; i++ )
                    strcpy(c->args[i],c->args[i+2]);
              c->nargs = c->nargs-2;  
	      c->args[i]=NULL;
              if(isbuiltin(arr,c) > 0)
	         setpriority(PRIO_PROCESS,0,val);			
	     }
	    else
	     {
	      val = 4;
	      for ( i = 0; c->args[i+1] != NULL; i++ )
                 strcpy(c->args[i],c->args[i+1]);
	      c->nargs--; 
              c->args[i]=NULL;
	      if(isbuiltin(arr,c) > 0)
 	   	setpriority(PRIO_PROCESS,0,val);	
	     }
	  prCmd(c,val);			
	}	 		
  }	
else if(!strcmp(c->args[0],"cd"))
	{
	 if( c->nargs > 1 ){
	   if(chdir(c->args[1])!=0)
		perror("Permission denied");}
	 else
	   chdir(getenv("HOME"));
	}	
else if(!strcmp(c->args[0],"echo"))			 		
	{
	for(i = 1; c->args[i] != NULL;i++)
	   {
	    strcat(str,c->args[i]);
	    strcat(str," ");		
	   }    
	  printf("%s\n",str);
	  
    	}
else if(!strcmp(c->args[0],"pwd"))					
	{
	 getcwd(curdir,1000);
	 printf("%s\n",curdir);
	}	
else if(!strcmp(c->args[0],"logout") || !strcmp(c->args[0], "end"))
	{
	 exit(0);
	}	
     else if(!strcmp(c->args[0],"setenv"))
	{
	 if(c->nargs == 1)
	   {
	    for(env = environ; *env != NULL; env++)
		{
		 char *thisenv = *env;
		 printf("%s\n",thisenv);
		}
	   }
	 else if(c->nargs > 2)
	   setenv(c->args[1],c->args[2],1);
	 else
	   setenv(c->args[1],NULL,1);
	}
      else if(!strcmp(c->args[0],"unsetenv"))
	{
	 unsetenv(c->args[1]);
	 putchar('\n');
	}
else if(!strcmp(c->args[0],"where"))
	{
	  if(c->nargs>1)
	  { 
	    for(i=0; i<8;i++)
	    {
	     if(!strcmp(c->args[1],arr[i]))
	     	printf("%s is a built-in\n",arr[i]);
	    }
	  char *path_value = (char*)malloc(1000*sizeof(char));
	  path_value = getenv("PATH");
	  char *path_temp = (char*)malloc(1000*sizeof(char));
          strcpy(path_temp,path_value);
	  char *temp = 	(char*)malloc(1000*sizeof(char));
	  char *tok = (char*)malloc(1000*sizeof(char));
          tok = strtok(path_value,":");
	  while(tok != NULL)
		{
		 strcpy(temp,tok);
		 strcat(temp,"/");
		 strcat(temp,c->args[1]);
		 if(access(temp,F_OK)==0)
		  printf("%s\n",temp);
		 //else
		  //perror("Unable to access");
		 tok = strtok(NULL,":");	
		}
	   setenv("PATH",path_temp,1);
	   }
	   
	}
}

void handler()
{

}

void childprocess(Cmd c, int p)
{
pid_t pid = getpid();
if(p != 1000)
 {
 setpriority(PRIO_PROCESS,pid,p);
 }

if(execvp(c->args[0],c->args) == -1)
      perror("Unable to run");
signal(SIGTERM,handler);
exit(EXIT_FAILURE);
}

void forking(Cmd c, int p)
{

pid_t pid;
int status;
int piped[2];
if(c->out == Tpipe || c->in == Tpipe)
  pipe(piped);
  pid = fork();
  if(pid < 0)
     perror("Unable to fork");
//Child processes
  else if(pid == 0)
   {
    if(c->out == Tpipe || c->in == Tpipe)	   	
	{
	dup2(des,0);
	if(c->next !=NULL)
   	   dup2(piped[1],1);
	close(piped[0]); 
   	//close(piped[1]);
   	}
        childprocess(c,p);
   }
//parent process
  else
  {
   waitpid(pid,&status,0);
   if(c->out == Tpipe || c->in == Tpipe)	
     {des = piped[0];
      close(piped[1]);
      //close(piped[0]);	
     }
  }   
}

static void prPipe(Pipe p)
{
  int i = 0;
  Cmd c;

  if ( p == NULL )
    return;

  //printf("Begin pipe%s\n", p->type == Pout ? "" : " Error");
  for ( c = p->head; c != NULL; c = c->next ) {
    //printf("  Cmd #%d: ", ++i);
    prCmd(c,1000);
  }
  //printf("End pipe\n");
  prPipe(p->next);
}

int main(int argc, char *argv[])
{
  Pipe p;
  //char *host = "armadillo";
  char hostname[100];
  hostname[99] = '\0';
  signal(SIGQUIT,SIG_IGN);
  signal(SIGTERM,handler);
  signal(SIGINT, SIG_IGN); 
  int fd,old;
  old = dup(0);
  char *home = (char*)malloc(1000*sizeof(char));
  home = getenv("HOME");
  char *temp = (char*)malloc(1000*sizeof(char));
  strcpy(temp,home);
  strcat(home,"/.ushrc");
  fd = open(home,O_RDONLY,S_IRWXU);        
  setenv("HOME",temp,1);
  if(fd < 0)	
	perror("Unable to open");
  dup2(fd,0);
  //close(fd);
while(fd)
{  
  p = parse();
  if(p==NULL)
    break;
  prPipe(p);
  freePipe(p);
}

  close(fd);
  fflush(stdout);
  dup2(old,0);
  close(old);
  
  while ( 1 ) {
    gethostname(hostname,100);  
    printf("%s%%", hostname);
     fflush(stdout);    
    p = parse();
    prPipe(p);
    freePipe(p);
  }
}






