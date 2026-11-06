#define _GNU_SOURCE
#include <sched.h>
#include <sys/mount.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
static void fail(const char *what){perror(what);exit(1);}
static void write_file(const char *path,const char *value){int fd=open(path,O_WRONLY);if(fd<0)fail(path);size_t n=strlen(value);if(write(fd,value,n)!=(ssize_t)n)fail("write");if(close(fd))fail("close");}
static void limit(int which,rlim_t value){struct rlimit r={value,value};if(setrlimit(which,&r))fail("setrlimit");}
int main(int argc,char **argv){
 if(argc<2){fprintf(stderr,"Usage: sandbox STATIC_EXECUTABLE [ARGS...]\nRequires unprivileged user namespaces. Only use trusted test programs.\n");return 2;}
 // A separate user namespace permits namespaced mounts without host root.
 uid_t uid=getuid();gid_t gid=getgid();char program[PATH_MAX];
 if(!realpath(argv[1],program))fail("executable");
 int executable=open(program,O_RDONLY|O_CLOEXEC);if(executable<0)fail("open executable");
 char root[]="/tmp/sandbox-XXXXXX";if(!mkdtemp(root))fail("mkdtemp");
 pid_t supervisor=fork();if(supervisor<0)fail("fork");
 if(supervisor>0){int status;while(waitpid(supervisor,&status,0)<0){if(errno!=EINTR)fail("waitpid");}if(rmdir(root))perror("remove temporary mountpoint");return WIFEXITED(status)?WEXITSTATUS(status):128+WTERMSIG(status);}
 const char *group=getenv("SANDBOX_CGROUP");
 if(group){
  char resolved[PATH_MAX];if(!realpath(group,resolved))fail("cgroup path");
  if(strncmp(resolved,"/sys/fs/cgroup/",15)!=0){fprintf(stderr,"Cgroup must be a delegated leaf beneath /sys/fs/cgroup\n");return 2;}
  struct stat st;if(stat(resolved,&st)||st.st_uid!=uid){fprintf(stderr,"Cgroup leaf must be owned by the invoking user\n");return 2;}
  const char *files[]={"memory.max","pids.max","cpu.max","cgroup.procs"};
  char pid[32];snprintf(pid,sizeof pid,"%ld",(long)getpid());
  const char *values[]={"134217728","16","50000 100000",pid};
  for(size_t i=0;i<4;i++){char path[PATH_MAX+32];snprintf(path,sizeof path,"%s/%s",resolved,files[i]);write_file(path,values[i]);}
 }
 if(unshare(CLONE_NEWUSER))fail("unshare user namespace");
 if(access("/proc/self/setgroups",F_OK)==0)write_file("/proc/self/setgroups","deny");
 char mapping[80];snprintf(mapping,sizeof mapping,"0 %u 1\n",(unsigned)uid);write_file("/proc/self/uid_map",mapping);snprintf(mapping,sizeof mapping,"0 %u 1\n",(unsigned)gid);write_file("/proc/self/gid_map",mapping);
 if(unshare(CLONE_NEWNS|CLONE_NEWNET|CLONE_NEWUTS|CLONE_NEWIPC|CLONE_NEWPID))fail("unshare isolation namespaces");
 if(mount(NULL,"/",NULL,MS_REC|MS_PRIVATE,NULL))fail("private mounts");
 if(mount("tmpfs",root,"tmpfs",MS_NOSUID|MS_NODEV,"size=32m,mode=755"))fail("tmpfs root");
 // Reopen the selected executable through a read-only bind mount inside the new root.
 char target[PATH_MAX];snprintf(target,sizeof target,"%s/program",root);int fd=open(target,O_CREAT|O_WRONLY,0700);if(fd<0)fail("program target");close(fd);
 if(mount(program,target,NULL,MS_BIND,NULL))fail("bind executable");
 if(mount(NULL,target,NULL,MS_BIND|MS_REMOUNT|MS_RDONLY|MS_NOSUID|MS_NODEV,NULL))fail("read-only executable");
 close(executable);
 pid_t child=fork();if(child<0)fail("PID namespace fork");
 if(child==0){
  if(chroot(root)||chdir("/"))fail("chroot");
  limit(RLIMIT_CPU,2);limit(RLIMIT_AS,128*1024*1024);limit(RLIMIT_FSIZE,8*1024*1024);limit(RLIMIT_NOFILE,16);limit(RLIMIT_CORE,0);
  if(prctl(PR_SET_NO_NEW_PRIVS,1,0,0,0))fail("no new privileges");
  // Clear inherited descriptors. Standard streams remain intentionally connected.
  long maxfd=sysconf(_SC_OPEN_MAX);if(maxfd<0)maxfd=65536;for(int i=3;i<maxfd;i++)close(i);
  char *env[]={"PATH=/","LANG=C",NULL};argv[1]="/program";execve("/program",argv+1,env);fail("execve (use a static executable)");
 }
 int status;unsigned seconds=0;for(;;){pid_t r=waitpid(child,&status,WNOHANG);if(r==child)break;if(r<0&&errno!=EINTR)fail("waitpid");if(seconds++>=5){kill(child,SIGKILL);if(waitpid(child,&status,0)<0)fail("wait timeout");fprintf(stderr,"Wall-time limit exceeded\n");break;}sleep(1);}
 return WIFEXITED(status)?WEXITSTATUS(status):128+WTERMSIG(status);
}
