#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <dirent.h>
#include <time.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <fcntl.h>

#define MAX_INSTRN_SIZE 100
#define TOK_DELIM " \t\r\n\a"
#define ERR_NO -1

int generate_prompt(){  //to generate prompt 
	char * cwd;
	cwd = getcwd(NULL,0);
	if(cwd==NULL){
		perror("Error in getting current working directory");
		return -1;
	}
	printf("%s> ",cwd);
	return 0;
}

int cd_shell(char ** argv,int argc){	//function for changing directory
	if(argc<2){
		printf("Unexpected Number of Inputs to cd\n");
		return -1;
	}
	if(chdir(argv[1])!=0){
		perror("Error in cd");
		return -1;
	}
	return 0;
}

int pwd_shell(char ** argv, int  argc){		//function to get the current working directorty
	char * cwd;
	cwd = getcwd(NULL,0);
	if(cwd==NULL){
		perror("Error in getting current working directory");
		return -1;
	}
	printf("%s\n",cwd);
	return 0;
}

int mkdir_shell(char ** argv, int argc){		//function to make a directory
	if(argc < 2){
		printf("Unexpected Number of Inputs to mkdir\n");
		return -1;
	}
	if(mkdir(argv[1],S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == ERR_NO){
		perror("Error in making directory");
		return -1;
	}
	return(0);
}

int rmdir_shell(char ** argv, int argc){		//function to remove a directory
	if(argc < 2){
		printf("Unexpected Number of Inputs to rmdir\n");
		return -1;
	}
	if(rmdir(argv[1]) == ERR_NO){
		perror("Error in removing directory");
		return -1;
	}
	return(0);
}

int _ls_l_shell() {			//function for sub command ls-l
	struct dirent **ent;
	int no = scandir(".", &ent, NULL, alphasort);	// returns sorted :)
	if (no == -1) {
		perror("Error scanning directory");
		return -1;
	}
	int i;
	long long total_blocks = 0;
	for (i = 0; i < no; ++i) {
		if (strcmp(ent[i] -> d_name, ".") && strcmp(ent[i] -> d_name, "..") ) {
			struct stat s;
			if (stat(ent[i] -> d_name, &s) == -1) {
				perror("Stat");
				return -1;
			}
			total_blocks += s.st_blocks;
		}
	}
	printf("total %lld\n", total_blocks / 2);

	for (i = 0; i < no; ++i) {
		if (strcmp(ent[i] -> d_name, ".") && strcmp(ent[i] -> d_name, "..")) {
			struct stat s;
			if (stat(ent[i] -> d_name, &s) == -1) {
				perror("Stat");
				free(ent[i]);
				continue;
			}
			printf((S_ISDIR(s.st_mode)) ? "d" : "-");
			printf((s.st_mode & S_IRUSR) ? "r" : "-");
			printf((s.st_mode & S_IWUSR) ? "w" : "-");
			printf((s.st_mode & S_IXUSR) ? "x" : "-");
			printf((s.st_mode & S_IRGRP) ? "r" : "-");
			printf((s.st_mode & S_IWGRP) ? "w" : "-");
			printf((s.st_mode & S_IXGRP) ? "x" : "-");
			printf((s.st_mode & S_IROTH) ? "r" : "-");
			printf((s.st_mode & S_IWOTH) ? "w" : "-");
			printf((s.st_mode & S_IXOTH) ? "x" : "-");
			printf("  ");
			printf("%hu %s  ", s.st_nlink, getpwuid(s.st_uid)->pw_name);
			printf("%s %lld  ", getgrgid(s.st_gid)->gr_name, s.st_size);
			time_t t = s.st_mtime;
			struct tm lt;
			localtime_r(&t, &lt);
			char buff[20];
			strftime(buff, sizeof(buff), "%b %d %H:%M", &lt);
			printf("%s  ", buff);
			printf("%s\n", ent[i]->d_name);
		}
		free(ent[i]);
	}
	free(ent);
	return 0;
}


int ls_shell(char ** argv, int argc){ 		//function for ls
	if(argc>1 && argv[1][0]=='-' && argv[1][1]=='l'){
		return _ls_l_shell();
	}
	char * cwd;
	cwd = getcwd(NULL,0);
	if(cwd==NULL){
		perror("Error in getting current working directory");
		return -1;
	}

	DIR * curr_dir = opendir(cwd);
	if(curr_dir==NULL){
		perror("Error in getting current working directory");
		return -1;
	}
	struct dirent * curr_sub_dir;
	struct stat mystat;

	while((curr_sub_dir=readdir(curr_dir))!=NULL){
		if(strcmp(curr_sub_dir->d_name,".")==0 || strcmp(curr_sub_dir->d_name,"..")==0 || strcmp(curr_sub_dir->d_name,".DS_Store")==0 ){
			continue;
		}
		printf("%s\n",curr_sub_dir->d_name);	
	}

	closedir(curr_dir);
	return 0;
}

int cp_shell(char ** argv, int argc){  //function to copy file1 to file2
	if(argc < 3){
		printf("Unexpected Number of Inputs to cp\n");
		return -1;
	}
	if(!strcmp(argv[1],argv[2])){
		printf("Same Files\n");
		return -1;
	}
	struct stat s;
	time_t tt[2];
	int file1 = open(argv[1], O_RDONLY, S_IRUSR | S_IWUSR);
	if(file1==-1){
		perror("Error in opening read file");
		exit(-1);
	}
	if (stat(argv[1], &s)) {
			printf("Error getting time of file");
			perror("");
			return -1;
	}
	tt[0] = s.st_mtime;
	
	if (stat(argv[2], &s)) {
			printf("Error getting time of file");
			perror("");
			return -1;
	}
	tt[1] = s.st_mtime;
	int file2 = open(argv[2], O_WRONLY | O_TRUNC , S_IRUSR | S_IWUSR);
	if(file2==-1){
		perror("Error in opening write file");
		exit(-1);
	}

	if (difftime(tt[0], tt[1]) <= 0) {
		printf("Source file not recent\n");
		return -1;
	}

	int copy_bufsize = 100;
	char copy_buf[100];
	int wordsRead;

	while(1){
		if((wordsRead = read(file1,copy_buf,copy_bufsize)) == ERR_NO){
			perror("Error in read");
			return -1;
		}
		if(!wordsRead){
			break ;
		}
		if(write(file2,copy_buf,copy_bufsize)==ERR_NO){
			perror("Error in write");
			return -1;
		}

	}

	close(file1);
	close(file2);
	 return 0;
}

int  exit_shell(char ** argv, int argc){		//function to exit shell
	printf("Good Bye!!\n");
	exit(0);
}

char *builtin_str[] = {
  "cd",
  "pwd",
  "mkdir",
  "rmdir",
  "ls",
  "cp",
  "exit"
};

int (*builtin_func[]) (char **, int ) = {
  &cd_shell,
  &pwd_shell,
  &mkdir_shell,
  &rmdir_shell,
  &ls_shell,
  &cp_shell,
  &exit_shell
};

int num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}

int read_split_line(char *** ptr_argv,int * argc)  //return number of token read
{
  char ** argv = *ptr_argv;
  argv = (char * *)malloc(MAX_INSTRN_SIZE*sizeof(char *));
  char *line = NULL;
  size_t instr_size = 0; // have getline allocate a buffer for us
  getline(&line, &instr_size, stdin);
  int bufsize = MAX_INSTRN_SIZE;
  int position = 0;
  char *token, * brk;

  token = strtok_r(line, TOK_DELIM,&brk);
  while (token != NULL) {
  	argv[position] = (char *)malloc(strlen(token+1)*sizeof(char));
  	strcpy(argv[position],token);
    position++;

    if (position >= bufsize) {
      bufsize += MAX_INSTRN_SIZE;
      argv = realloc(argv, bufsize * sizeof(char*));
    }

    token = strtok_r(NULL, TOK_DELIM,&brk);
  }
  argv[position] = NULL;
  *argc = position;
  * ptr_argv = argv;

  return position;
}

int _copy_argument_list(char ** argv,int argc,int start,int stop,char *** ptr_new_argv,int * ptr_new_argc){  //function to copy argument list from start to stop
	int i=0,count=0;
	char ** new_argv = *ptr_new_argv;
	int new_argc = *ptr_new_argc;

	new_argv = (char **)malloc((stop-start+1)*sizeof(char *));
	for(i=start;i<=stop;++i,++count){
		new_argv[count]=(char *)malloc((strlen(argv[i])+1)*sizeof(char));
		strcpy(new_argv[count],argv[i]);
	}
	new_argc = count;
	new_argv[count]=NULL;
	* ptr_new_argv = new_argv;
	* ptr_new_argc = new_argc;

	return 0;
}

int _get_input_output(char ** argv, int * ptr_argc, int *ptr_file_in, int *ptr_file_out ){		//function to get input output files for a command
	int i=0;
	int file_in=-1, file_out=-1, new_argc=*ptr_argc, in_done=0, out_done=0;
	for(i=0;i<*ptr_argc;++i){
		if(!strcmp(argv[i],"<") && !in_done){
			in_done=1;
			//printf("in-%s\n",argv[i+1] );
			file_in = open(argv[i+1], O_RDONLY, S_IRUSR | S_IWUSR);
			if(file_in==-1){
				perror("Error in opening read file");
				return -1;
			}
			if(i<new_argc) new_argc=i;
		}
		if(!strcmp(argv[i],">") && !out_done){
			out_done=1;
			file_out = open(argv[i+1], O_WRONLY | O_CREAT | O_TRUNC , S_IRUSR | S_IWUSR);
			//printf("out-%s\n",argv[i+1] );
			if(file_out==-1){
				perror("Error in opening write file");
				return -1;
			}
			if(i<new_argc) new_argc=i;
		}
	}
	argv[new_argc]=NULL;  //if no input output files exist use stdin as input as stdout as output file
	*ptr_argc=new_argc;
	if(file_in==-1){
		*ptr_file_in=dup(0);
	}else{
		*ptr_file_in=file_in;
	}
	if(file_out==-1){
		*ptr_file_out=dup(1);
	}else{
		*ptr_file_out=file_out;
	}
	//printf("%d %d\n",*ptr_file_in,*ptr_file_out );
	return 0;
}

int _exec_pipe(char ** argv,int argc, int file_in, int file_out){  //to execute commands
	int i=0;
	// printf("%d\n",argc );
	// printf("%s\n", argv[0]);
	// int j=0;
	// for(j=0;argv[j]!=NULL;++j){
	// 	printf("%s\n",argv[j] );
	// }
	// printf("\n");
	if(argc==0){
		printf("Stray '|' character  in command\n");
		exit(-1);
	}
	for(i=0;i<argc;++i){
		if(!strcmp(argv[i],"|")){
			break;
		}
	}
	//printf("%d %d %d %d\n",i,argc,file_in,file_out );
	if(i==argc){
		//printf("Hello\n");
		close(0);
		dup(file_in);
		close(file_in);
		close(1);
		dup(file_out);
		close(file_out);
		argv[argc]=NULL;
		execvp(argv[0],argv);
		perror("Error in exec");
		exit(-1);
	}

	int fd[2];
	if(pipe(fd)==-1){
		perror("Error in pipe");
		exit(-1);
	}


	int child_id;
	if((child_id=fork())==0){
		char ** new_argv;
		int new_argc;
		_copy_argument_list(argv,argc,0,i-1,&new_argv,&new_argc);
		//printf("%d %s\n",new_argc,new_argv[new_argc-1] );
		close(0);
		dup(file_in);
		close(1);
		dup(fd[1]);
		close(fd[0]);
		close(file_in);
		close(fd[1]);
		execvp(new_argv[0],new_argv);
		perror("Error in Exec");
		exit(-1);
	}else{
		int status;
		waitpid(child_id,&status,0);
		close(fd[1]);
		char ** new_argv;
		int new_argc;
		//_copy_argument_list(argv,argc,i+1,argc-1,&new_argv,&new_argc);
		exit (_exec_pipe(argv+i+1,argc-i-1,fd[0],file_out));
	}
	return 0;
}


int parse_execute_Line(char  * * argv,int argc,int * is_builtin,int * run_parallel){  //to parse the commands and execute apt functions
	if(!argc){
		return 0;
	}
	int i=0;
	for(i=0;i<num_builtins();++i){
		if(strcmp(argv[0],builtin_str[i])==0){
			return (*builtin_func[i])(argv,argc);
		}
	}

	if(!strcmp(argv[argc-1],"&")){
		argv[argc-1]=NULL;
		--argc;
		*run_parallel=1;
	}

	int child_id;
	if((child_id=fork())==0){
		if(argc==1){
			execvp(argv[0],argv);
			perror("Error in execution");
			exit(ERR_NO);
		}else{
			int file_in,file_out;
			if(_get_input_output(argv,&argc,&file_in,&file_out)==-1){
				exit(-1);
			}
			exit(_exec_pipe(argv,argc,file_in,file_out));
		}
	}else{
		if(!(*run_parallel)){
			int status;
			waitpid(child_id,&status,0);
		}
	}

	return 0;
}

void cleanup(char * * * ptr_argv,int * argc,int * run_parallel,int * is_builtin){ //function for clean up
	*run_parallel=0;
	*is_builtin=0;
	*argc=0;
	free(*ptr_argv);
}

int main(){
	int status=0, argc=0;
	char  * * argv;
	int run_parallel=0,is_builtin=0;

	do{	
		generate_prompt();
		read_split_line(&argv,&argc);
		parse_execute_Line(argv,argc,&is_builtin,&run_parallel);
		cleanup(&argv,&argc,&run_parallel,&is_builtin);
	}while(!status);
	return 0;
}