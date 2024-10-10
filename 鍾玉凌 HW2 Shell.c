#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <sys/wait.h>

#define MAX_ARGUMENTS 20
#define MAX_HISTORY_SIZE 50

char *history_path = "./his.txt"; // 存儲命令歷史的文件名
// 函数
char *set_prompt(void);

void change_directory(char *path)
{
    if (path[0] == '~')
    {
        // 如果目錄以 '~' 開頭，表示使用者想要切換到主目錄
        const char *home_dir = getenv("HOME");
        if (home_dir != NULL)
        {
            // 如果 HOME 環境變數存在，則切換到主目錄
            if (chdir(home_dir) != 0)
            {
                perror("cd");
            }
        }
        else
        {
            fprintf(stderr, "cd: HOME environment variable not set\n");
        }
    }
    else
    {
        // 否則，正常切換到指定目錄
        if (chdir(path) != 0)
        {
            perror("cd");
        }
    }

    set_prompt();
}
char *set_prompt(void)
{
    char *user = getenv("USER");
    char hostname[HOST_NAME_MAX];
    gethostname(hostname, sizeof(hostname));

    char *home_directory = getenv("HOME");
    char *current_directory = getcwd(NULL, 0);

    char *prompt = NULL;
    if (current_directory != NULL)
    {
        size_t common_len = 0;
        while (home_directory[common_len] == current_directory[common_len] &&
               home_directory[common_len] != '\0' && current_directory[common_len] != '\0')
        {
            common_len++;
        }

        // Calculate the length needed for prompt
        size_t prompt_len;
        if (common_len > 0)
        {
            prompt_len = snprintf(NULL, 0, "%s@%s:~%s$ ", user, hostname, current_directory + common_len);
        }
        else
        {
            prompt_len = snprintf(NULL, 0, "%s@%s:%s$ ", user, hostname, current_directory);
        }

        // Allocate memory for prompt
        prompt = malloc(prompt_len + 1);
        if (prompt != NULL)
        {
            if (common_len > 0)
            {
                snprintf(prompt, prompt_len + 1, "%s@%s:~%s$ ", user, hostname, current_directory + common_len);
            }
            else
            {
                snprintf(prompt, prompt_len + 1, "%s@%s:%s$ ", user, hostname, current_directory);
            }
        }

        free(current_directory);
    }
    else
    {
        // Handle the case where getcwd() fails
        prompt = malloc(snprintf(NULL, 0, "%s@%s:$ ", user, hostname) + 1);
        if (prompt != NULL)
        {
            snprintf(prompt, strlen(prompt) + 1, "%s@%s:$ ", user, hostname);
        }
    }

    return prompt;
}
// Function to handle export command
void handle_export(const char *append_path)
{
    // 獲取現有的 PATH 值
    char *existing_path = getenv("PATH");

    if (existing_path != NULL)
    {
        // 檢查是否已經包含 "PATH="，如果包含，去除它
        const char *prefix = "PATH=$PATH:";
        size_t prefix_len = strlen(prefix);

        if (strncmp(append_path, prefix, prefix_len) == 0)
        {
            append_path += prefix_len;
        }
        // 分配足夠的空間來存儲新的值
        char *new_path = malloc(strlen(existing_path) + strlen(append_path) + 1);

        // 將現有值和新的路徑組合到新的字符串中
        strcpy(new_path, existing_path);
        strcat(new_path, ":"); // 添加分隔符
        strcat(new_path, append_path);

        // 更新 PATH 環境變數
        if (setenv("PATH", new_path, 1) != 0)
        {
            perror("setenv");
            free(new_path);
            return;
        }

        // 釋放分配的內存
        free(new_path);
    }
    else
    {
        fprintf(stderr, "PATH environment variable not found\n");
    }
}
int main()
{
    char *input;
    char *arguments[MAX_ARGUMENTS];
    char *token;

    using_history(); // 启用历史记录扩展
    read_history("his.txt");

    while (1)
    {
        char *prompt = set_prompt();
        input = readline(prompt); // line指向字符串的起始位置
        if (input == NULL)
        {
            // Handle a Ctrl+D or an error (e.g., reaching the end of input)
            break; // Exit the loop
        }
        // 移除换行符
        input[strcspn(input, "\n")] = '\0';
        // 将命令添加到历史记录
        add_history(strdup(input));

        // 将输入分割为命令和参数
        int arg_count = 0;
        token = strtok(input, " ");
        while (token != NULL && arg_count < MAX_ARGUMENTS - 1)
        {
            arguments[arg_count++] = token;
            token = strtok(NULL, " ");
        }
        arguments[arg_count] = NULL;

        // 处理命令
        if (arg_count > 0)
        {
            // cd指令
            if (strcmp(arguments[0], "cd") == 0)
            {
                // 如果命令是cd，调用change_directory函数
                if (arg_count > 1)
                {
                    change_directory(arguments[1]);
                }
                else
                {
                    fprintf(stderr, "cd: missing argument\n");
                }
            }
            // echo指令
            else if (strcmp(arguments[0], "echo") == 0)
            {
                for (int i = 1; i < arg_count; ++i)
                {
                    if (arguments[i][0] == '$')
                    {
                        // 如果參數以$開頭，表示可能是環境變數
                        const char *env_variable = arguments[i] + 1; // 跳過$
                        char *env_value = getenv(env_variable);
                        if (env_value != NULL)
                        {
                            printf("%s ", env_value);
                        }
                        else
                        {
                            fprintf(stderr, "Unknown environment variable: %s\n", env_variable);
                        }
                    }
                    else
                    {
                        printf("%s ", arguments[i]);
                    }
                }
                printf("\n");
            }
            // pwd指令
            else if (strcmp(arguments[0], "pwd") == 0)
            {
                char cwd[PATH_MAX];
                if (getcwd(cwd, sizeof(cwd)) != NULL)
                {
                    printf("%s\n", cwd);
                }
                else
                {
                    perror("getcwd");
                }
            }
            else if (strcmp(arguments[0], "export") == 0)
            {
                if (arg_count > 1)
                {
                    handle_export(arguments[1]);
                }
                else
                {
                    fprintf(stderr, "export: missing argument\n");
                }
            }
            else if (strcmp(arguments[0], "exit") == 0)
            {
                // 如果命令是exit，退出循环
                free(input);
                break;
            }
            else
            {
                // 其他命令的处理
                int pid = fork();
                if (pid == 0)
                {
                    // 在子进程中执行命令
                    execvp(arguments[0], arguments);
                    // 如果execvp失败，打印错误信息
                    perror(arguments[0]);
                    exit(EXIT_FAILURE);
                }
                else if (pid < 0)
                {
                    // 如果fork失败，打印错误信息
                    perror("fork");
                }
                else
                {
                    // 在父进程中等待子进程结束
                    wait(NULL);
                }
            }
            free(input); // Free the line allocated by readline
            free(prompt);
        }

        // 将历史记录追加到文件
        if (append_history(1, "history.txt") != 0)
        {
            fprintf(stderr, "Error appending to history file\n");
        }
    }
    write_history(history_path);
    return 0; // Return 0 after breaking out of the loop
}
