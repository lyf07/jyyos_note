#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <regex.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <time.h>
#include <ctype.h>
#include <signal.h>

#define SIZE 1024

typedef struct {
    char name[SIZE];
    double time;
} Syscall;

Syscall allCalls[SIZE];
int cur = 0;
double total_sum = 0;
clock_t start_time, end_time;
double cpu_time_used;


char **initenv() {
    char path[SIZE] = "PATH=";
    strcat(path, getenv("PATH"));
    char **env = (char**)malloc(sizeof(char*) * SIZE);
    env[0] = path;
    env[1] = NULL;
    return env;
}

char **initargs(int argc, char *argv[]) {
    char **myargs = (char**)malloc(sizeof(char*) * SIZE);
    myargs[0] = "strace";
    myargs[1] = "-T";
    for (int i = 1; i < argc; i++) {
        myargs[1 + i] = argv[i];
    }
    myargs[1 + argc] = NULL;
    return myargs;
}

void reorient(int pipe_output) {
    // get rid of the output of arg file
    int fd = open("/dev/null", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("文件竟然打不开？！");
        exit(2333);
    }
    if (dup2(fd, STDOUT_FILENO) == -1) {
        perror("stdout重定向都能错啊");
        close(fd);
        exit(EXIT_FAILURE);
    }
    // redirect the output to the pipe
    if (dup2(pipe_output, STDERR_FILENO) == -1) {
        perror("stderr重定向都能错啊");
        close(pipe_output);
        exit(EXIT_FAILURE);
    }

    close(fd);
}

// char *match_name(char *data, bool isName) {
//     // printf("step 1\n");
//     if (!((data[0] >= 'a' && data[0] <= 'z') || (data[0] >= 'A' && data[0] <= 'Z'))) {
//         // printf("in if\n");
//         return NULL;
//     }
//     // printf("step 2\n");
//     int max_matches = 1;
//     regex_t regex;   // 用于存储编译后的正则表达式
//     regmatch_t matches;   // 用于存储匹配结果
//     const char *pattern = isName ? "\\w+\\(" : " <.*>";   // 要匹配的正则表达式模式
//     char *text = data;
//     // printf("pattern = %s\n", pattern);
//     // 编译正则表达式
//     if (regcomp(&regex, pattern, REG_EXTENDED) != 0) {
//         perror("正则都能编译错啊，孩子！");
//         exit(2333);
//     }

//     // 进行匹配
//     if (regexec(&regex, text, max_matches, &matches, 0) == 0) {
//         // 打印匹配到的字符串
//         if (matches.rm_so != -1) {
//             // perror("success");
//             char *buf = (char*)malloc(sizeof(char) * SIZE);
//             int start = matches.rm_so;
//             int end = matches.rm_eo;
//             int length = isName ? end - start - 1 : end - start - 3;
//             char *begin = isName ? text + start : text + start + 2;
//             snprintf(buf, SIZE, "%.*s", length, begin);
//             regfree(&regex);
//             // printf("buf = %s\n", buf);
//             return buf;
//         }

//     } else {
//         // fprintf(stderr, "No match found\n");
//         // perror("failed\n");
//         regfree(&regex);
//         // free(buf);
//         return NULL;
//     }

//     // 释放正则表达式
//     assert(0);
//     return NULL;
// }

void match_name(char *data, char *target, bool isName) {
    int length = strlen(data);
    // printf("data[0] = %c, data[length - 2] = %c\n", data[0], data[length - 2]);
    if (!(length > 0 && isalpha(data[0]) && data[length - 1] == '>')) {
        target = NULL;
        return;
    }
    // printf("here\n");
    // static buf[SIZE];
    int fix = -1;
    // <0.000075>
    for (int i = 0; i < length; i++) {
        if (isName) {
            if (data[i] == '(') {
                target[i] = '\0';
                break;
            }
            else {
                target[i] = data[i];
            }
        }
        else {
            bool flag = false;
            int begin = -1;
            for (int i = length - 12; i < length - 1; i++) {
                // printf("%d: data[i] = %c\n", i, data[i]);
                if (data[i] == '<') {
                    flag = true;
                    begin = i + 1;
                    continue;
                }
                if (!flag) {
                    continue;
                }
                if (data[i] == '>') {
                    target[i - begin] = '\0';
                    break;
                }
                else {
                    target[i - begin] = data[i];
                }
            }
        }
    }
}

// need free: return true
// don't need free: return false
bool store_data(char *name, double time) {
    for (int i = 0; i < cur; i++) {
        if (strcmp(name, allCalls[i].name) == 0) {
            allCalls[i].time += time;
            return true;
        }
    }
    // (allCalls + cur)->name = name;
    strcpy((allCalls + cur)->name, name);
    (allCalls + cur)->time = time;
    cur++;
    return false;
}

int compare(const void *a, const void *b) {
    double temp = ((Syscall*)a)->time - ((Syscall*)b)->time;
    return temp > 0 ? -1 : 1;
}

void print_log() {
    // printf("call success!\n");
    // printf("cur = %d\n", cur);
    qsort(allCalls, cur, sizeof(Syscall), compare);
    // printf("===================\n");
    printf("Time: %fs\n", (double)((double)end_time / CLOCKS_PER_SEC));
    for (int i = 0; i < 5; i++) {
        printf("%s (%d%%)\n", allCalls[i].name, (int)(allCalls[i].time / total_sum * 100));
    }
    for (int i = 0; i < 80; i++) {
        printf("%c", '\0');
    }
    fflush(stdout);
}

int main(int argc, char *argv[], char *env[]) {
    assert(!argv[argc]);
    // char **env = initenv();
    char **myargs = initargs(argc, argv);
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("管道都创建不好！");
        exit(2333);
    }
    char buf[SIZE];
    pid_t pid = fork();
    if (pid < 0) {
        perror("见鬼了！fork不了？？");
        exit(2333);
    }
    else if (pid == 0) {
        close(pipefd[0]);
        // reorient the output stream
        reorient(pipefd[1]);
        execve("strace", myargs, env);
        execve("/bin/strace", myargs, env);
        execve("/usr/bin/strace", myargs, env);
        perror("见鬼了！execve能返回？？");
    }
    else {
        // wait(NULL);
        close(pipefd[1]);
        // 启动定时器
        dup2(pipefd[0], STDIN_FILENO);
        start_time = clock();
        // setitimer(ITIMER_REAL, &timer, NULL);
        
        static char name[SIZE];
        static char t[SIZE];

        while (scanf("%[^\n]\n", buf) == 1) {
            // printf("cur = %d\n", cur);
            // printf("1: buf = %s\n", buf);
            memset(name, 0, sizeof(name));
            memset(t, 0, sizeof(t));
            // printf("before: name = %s, t = %s\n", name, t);
            match_name(buf, name, true);     // isName is true
            match_name(buf, t, false);
            // printf("after: name = %s, t = %s\n", name, t);
            // bool need_to_free = false;
            if (name != NULL && t != NULL && strlen(name) != 0 && strlen(t) != 0) {
                double time = strtod(t, NULL);
                // need_to_free = store_data(name, time);
                // printf("will be stored\n");
                store_data(name, time);
                total_sum += time;
            }
            end_time = clock();
            cpu_time_used = ((double) (end_time - start_time)) / CLOCKS_PER_SEC;
            if (cpu_time_used >= 0.1) {
                start_time = end_time;
                print_log();
            }
            memset(buf, 0, sizeof(buf));
        }

        print_log();
        // printf("total_sum = %f\n", total_sum);
        wait(NULL);

        // printf("buf = %s\n", buf);
    }

    // free(env);
    free(myargs);
    // for (int i = 0; i < cur; i++) {
    //     free(allCalls[i].name);
    // }
    return 0;
}
