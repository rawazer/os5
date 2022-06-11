#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#define __USE_GNU
#include <sched.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>

#define STACK 8192

static int _numlen(int num) {
    int count = 0;
    while (num) {
        num = num / 10;
        count++;
    }
    return count;
}

static void _mkdir() {
    if(mkdir("/sys/fs", 0755) != 0) {
        fprintf(stderr, "System error: creating cgroup failed\n");
        exit(1);
    }

    if(mkdir("/sys/fs/cgroup", 0755) != 0) {
        fprintf(stderr, "System error: creating cgroup failed\n");
        exit(1);
    }

    if(mkdir("/sys/fs/cgroup/pids", 0755) != 0) {
        fprintf(stderr, "System error: creating cgroup failed\n");
        exit(1);
    }
}

// static void _mkdir(const char *dir) {
//     char tmp[256];
//     char *p = NULL;
//     size_t len;

//     snprintf(tmp, sizeof(tmp),"%s",dir);
//     len = strlen(tmp);
//     if (tmp[len - 1] == '/')
//         tmp[len - 1] = 0;
//     for (p = tmp + 1; *p; p++)
//         if (*p == '/') {
//             *p = 0;
//             if(mkdir(tmp, 0755) != 0) {
//                 fprintf(stderr, "System error: creating cgroup failed\n");
//                 exit(1);
//             }
//             *p = '/';
//         }
//     // if(mkdir(tmp, 0755) != 0) {
//     //     fprintf(stderr, "System error: creating cgroup failed\n");
//     //     exit(1);
//     // }
// }

static void _write(char* path, char *content) {
    FILE* ptr = fopen(path, "w");
    if (ptr == NULL) {
        fprintf(stderr, "System error: creating cgroups failed\n");
        exit(1);
    }
    if (fwrite(content, sizeof(char), strlen(content), ptr) != strlen(content)) {
        fprintf(stderr, "System error: writing to cgroups failed\n");
        exit(1);
    }
    if (fclose(ptr) != 0) {
        fprintf(stderr, "System error: closing cgroups failed\n");
        exit(1);
    }
}

static void create_cgroups(char* num_processes) {
    char* cgroups_path = "/sys/fs/cgroup/pids";
    _mkdir();

    char* cgroups_procs_path = "/sys/fs/cgroup/pids/cgroup.procs";
    char* cgroups_pid_max_path = "/sys/fs/cgroup/pids/pids.max";
    char* notify_on_release_path = "/sys/fs/cgroup/pids/notify_on_release";

    int pid = getpid();
    int size_pid = _numlen(pid);
    char* cur_pid = malloc((size_pid + 2) * sizeof(char));
    snprintf(cur_pid, size_pid + 2, "%d\n", pid);
    char* notify = "1";

    _write(cgroups_procs_path, cur_pid);
    _write(cgroups_pid_max_path, num_processes);
    _write(notify_on_release_path, notify);

    free(cur_pid);
}

static void remove_cgroups(char* fs) {
    fprintf(stderr, "%s\n", fs);

    char* cgroups_path = malloc((strlen(fs) + 19)*sizeof(char));
    char* cgroups_procs_path = malloc((strlen(fs) + 32)*sizeof(char));
    char* cgroups_pid_max_path = malloc((strlen(fs) + 28)*sizeof(char));
    char* notify_on_release_path = malloc((strlen(fs) + 37)*sizeof(char)); 

    strcpy(cgroups_path, fs);
    strcpy(cgroups_procs_path, fs);
    strcpy(cgroups_pid_max_path, fs);
    strcpy(notify_on_release_path, fs);

    strcpy(cgroups_path + strlen(fs), "/sys/fs/cgroup/pids");
    strcpy(cgroups_procs_path + strlen(fs), "/sys/fs/cgroup/pids/cgroup.procs");
    strcpy(cgroups_pid_max_path + strlen(fs), "/sys/fs/cgroup/pids/pids.max");
    strcpy(notify_on_release_path + strlen(fs), "/sys/fs/cgroup/pids/notify_on_release");

    printf("-----\n%s\n%s\n%s\n%s\n-----\n",
            cgroups_path, cgroups_procs_path, 
            cgroups_pid_max_path, notify_on_release_path);


    // char* cgroups_path = strcat(fs, "/sys/fs/cgroup/pids");
    // char* cgroups_procs_path = strcat(cgroups_path, "/cgroup.procs");
    // char* cgroups_pid_max_path = strcat(cgroups_path, "/pids.max");
    // char* notify_on_release_path = strcat(cgroups_path, "/notify_on_release");
    
    if(remove(cgroups_procs_path) != 0 || remove(cgroups_pid_max_path) != 0 ||
        remove(notify_on_release_path) != 0 || remove(cgroups_path) != 0) {
            fprintf(stderr, "System error: deleting cgroups failed\n");
            exit(1);
    }
    
    free(cgroups_path);
    free(cgroups_procs_path);
    free(cgroups_pid_max_path);
    free(notify_on_release_path);
}

int child(void* args) {
    char** child_args = (char**)args;
    char* hostname = child_args[0];
    char* fs = child_args[1];
    char* num_processes = child_args[2];
    char* prog_path = child_args[3];
    char** prog_args = child_args + 3;

    if (sethostname(hostname, strlen(hostname)) != 0) {
        fprintf(stderr, "System error: setting hostname failed\n");
        exit(1);
    }

    if (chroot(fs) != 0) {
        fprintf(stderr, "System error: changing root directory failed\n");
        exit(1);
    }

    create_cgroups(num_processes);

    if(chdir("/") != 0) {
        fprintf(stderr, "System error: changing to root directory failed\n");
        exit(1);
    }
    
    if(mount("proc", "/proc", "proc", 0, 0) != 0){
        fprintf(stderr, "System error: mounting proc failed\n");
        exit(1);
    }

    printf("%s\n%s %s\n", prog_path, *prog_args, *(prog_args+1));
    printf("EXEC\n");

    if (execvp(prog_path, prog_args) != 0) {
        fprintf(stderr, "System error: executing program failed - %s\n", strerror(errno));
        exit(1);
    }

}

int main(int argc, char* argv[]) {
    void* stack = malloc(STACK);
    if (stack == NULL){
        fprintf(stderr, "System error: allocating stack failed\n");
        exit(1);
    }

    char** child_args = malloc((argc) * sizeof(char*));
    memcpy(child_args, argv + 1, (argc - 1) * sizeof(char*));
    child_args[argc - 1] = "\0";

    printf("before clone\n");

    if(clone(child, stack+STACK, CLONE_NEWPID | CLONE_NEWUTS | CLONE_NEWNS | SIGCHLD, child_args) == -1) {
        fprintf(stderr, "System error: cloning failed\n");
        exit(1);
    }
    wait(NULL);

    printf("after clone\n");

    char* target = malloc((strlen(argv[2]) + 5) * sizeof(char));
    strcpy(target, argv[2]);
    strcpy(target + strlen(argv[2]), "/proc");
    if(umount(target) != 0) {
        fprintf(stderr, "System error: unmounting proc failed\n");
        exit(1);
    }

    remove_cgroups(argv[2]);

    free(stack);
    free(child_args);
}