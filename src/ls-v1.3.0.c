
/*
 * Operating Systems Programming Assignment 02
 * Version 1.3.0 - Horizontal Column Display (-x)
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <getopt.h>
#include <limits.h>

#include <sys/wait.h>
#include <termios.h>

#define DEFAULT_TERMINAL_WIDTH 80
#define COLUMN_SPACING 2

void do_ls(const char *dir, int long_format, int horizontal);
void print_long_entry(const char *dir, const char *name);
void format_permissions(mode_t mode, char permissions[11]);
int get_terminal_width(void);
void print_columns(char **names, size_t count,
                   size_t max_length, int horizontal);
void free_names(char **names, size_t count);

int main(int argc, char *argv[])
{
    int long_format = 0;
    int horizontal = 0;
    int option;

    /* Recognize both -l and -x options. */
    while ((option = getopt(argc, argv, "lx")) != -1)
    {
        switch (option)
        {
            case 'l':
                long_format = 1;
                break;

            case 'x':
                horizontal = 1;
                break;

            default:
                fprintf(stderr,
                        "Usage: %s [-l] [-x] [directory ...]\n",
                        argv[0]);
                return EXIT_FAILURE;
        }
    }

    if (optind == argc)
    {
        do_ls(".", long_format, horizontal);
    }
    else
    {
        int directory_count = argc - optind;

        for (int i = optind; i < argc; i++)
        {
            if (directory_count > 1)
            {
                printf("%s:\n", argv[i]);
            }

            do_ls(argv[i], long_format, horizontal);

            if (i < argc - 1)
            {
                putchar('\n');
            }
        }
    }

    return EXIT_SUCCESS;
}

void do_ls(const char *dir, int long_format, int horizontal)
{
    DIR *dp = opendir(dir);

    if (dp == NULL)
    {
        fprintf(stderr, "%s: ", dir);
        perror("cannot open directory");
        return;
    }

    struct dirent *entry;
    char **names = NULL;
    size_t count = 0;
    size_t capacity = 0;
    size_t max_length = 0;

    /* Read all visible filenames into a dynamic array. */
    while (1)
    {
        errno = 0;
        entry = readdir(dp);

        if (entry == NULL)
        {
            if (errno != 0)
            {
                perror("readdir failed");
            }
            break;
        }

        /* Skip hidden entries, as in the previous version. */
        if (entry->d_name[0] == '.')
        {
            continue;
        }

        size_t name_length = strlen(entry->d_name);

        if (name_length > max_length)
        {
            max_length = name_length;
        }

        /* Expand the array when it becomes full. */
        if (count == capacity)
        {
            size_t new_capacity =
                (capacity == 0) ? 16 : capacity * 2;

            char **temp = realloc(
                names, new_capacity * sizeof(*names));

            if (temp == NULL)
            {
                perror("realloc failed");
                free_names(names, count);
                closedir(dp);
                return;
            }

            names = temp;
            capacity = new_capacity;
        }

        names[count] = strdup(entry->d_name);

        if (names[count] == NULL)
        {
            perror("strdup failed");
            free_names(names, count);
            closedir(dp);
            return;
        }

        count++;
    }

    closedir(dp);

    if (long_format)
    {
        /* Preserve the existing v1.1.0 long listing. */
        for (size_t i = 0; i < count; i++)
        {
            print_long_entry(dir, names[i]);
        }
    }
    else
    {
        /*
         * horizontal = 0: down then across
         * horizontal = 1: left to right, then next row
         */
        print_columns(names, count, max_length, horizontal);
    }

    free_names(names, count);
}

int get_terminal_width(void)
{
    struct winsize window_size;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &window_size) == 0 &&
        window_size.ws_col > 0)
    {
        return window_size.ws_col;
    }

    return DEFAULT_TERMINAL_WIDTH;
}

void print_columns(char **names, size_t count,
                   size_t max_length, int horizontal)
{
    if (count == 0)
    {
        return;
    }

    size_t column_width = max_length + COLUMN_SPACING;
    int terminal_width = get_terminal_width();

    size_t columns = (size_t)terminal_width / column_width;

    if (columns == 0)
    {
        columns = 1;
    }

    if (columns > count)
    {
        columns = count;
    }

    if (horizontal)
    {
        /*
         * Horizontal layout (-x):
         * Print filenames from left to right.
         * Start a new row after reaching the column limit.
         */
        size_t current_column = 0;

        for (size_t i = 0; i < count; i++)
        {
            if (current_column == columns)
            {
                putchar('\n');
                current_column = 0;
            }

            if (current_column == columns - 1 || i == count - 1)
            {
                printf("%s", names[i]);
            }
            else
            {
                printf("%-*s", (int)column_width, names[i]);
            }

            current_column++;
        }

        putchar('\n');
    }
    else
    {
        /*
         * Default layout:
         * Display entries down each column, then across.
         */
        size_t rows = (count + columns - 1) / columns;

        for (size_t row = 0; row < rows; row++)
        {
            for (size_t column = 0; column < columns; column++)
            {
                size_t index = row + column * rows;

                if (index >= count)
                {
                    continue;
                }

                if (column == columns - 1 ||
                    index + rows >= count)
                {
                    printf("%s", names[index]);
                }
                else
                {
                    printf("%-*s",
                           (int)column_width, names[index]);
                }
            }

            putchar('\n');
        }
    }
}

void format_permissions(mode_t mode, char permissions[11])
{
    if (S_ISDIR(mode))
        permissions[0] = 'd';
    else if (S_ISLNK(mode))
        permissions[0] = 'l';
    else if (S_ISCHR(mode))
        permissions[0] = 'c';
    else if (S_ISBLK(mode))
        permissions[0] = 'b';
    else if (S_ISFIFO(mode))
        permissions[0] = 'p';
    else if (S_ISSOCK(mode))
        permissions[0] = 's';
    else
        permissions[0] = '-';

    permissions[1] = (mode & S_IRUSR) ? 'r' : '-';
    permissions[2] = (mode & S_IWUSR) ? 'w' : '-';
    permissions[3] = (mode & S_IXUSR) ? 'x' : '-';

    permissions[4] = (mode & S_IRGRP) ? 'r' : '-';
    permissions[5] = (mode & S_IWGRP) ? 'w' : '-';
    permissions[6] = (mode & S_IXGRP) ? 'x' : '-';

    permissions[7] = (mode & S_IROTH) ? 'r' : '-';
    permissions[8] = (mode & S_IWOTH) ? 'w' : '-';
    permissions[9] = (mode & S_IXOTH) ? 'x' : '-';

    if (mode & S_ISUID)
        permissions[3] = (mode & S_IXUSR) ? 's' : 'S';

    if (mode & S_ISGID)
        permissions[6] = (mode & S_IXGRP) ? 's' : 'S';

    if (mode & 01000)
        permissions[9] = (mode & S_IXOTH) ? 't' : 'T';

    permissions[10] = '\0';
}

void print_long_entry(const char *dir, const char *name)
{
    char path[PATH_MAX];
    struct stat file_info;

    int length = snprintf(path, sizeof(path), "%s/%s", dir, name);

    if (length < 0 || (size_t)length >= sizeof(path))
    {
        fprintf(stderr, "Path too long: %s/%s\n", dir, name);
        return;
    }

    if (lstat(path, &file_info) == -1)
    {
        perror(path);
        return;
    }

    struct passwd *owner = getpwuid(file_info.st_uid);
    struct group *group = getgrgid(file_info.st_gid);

    char permissions[11];
    format_permissions(file_info.st_mode, permissions);

    char *modified_time = ctime(&file_info.st_mtime);

    if (modified_time == NULL)
    {
        modified_time = "??? ??? ?? ??:??\n";
    }

    printf("%s %2lu %-8s %-8s %8lld %.12s %s\n",
           permissions,
           (unsigned long)file_info.st_nlink,
           owner ? owner->pw_name : "unknown",
           group ? group->gr_name : "unknown",
           (long long)file_info.st_size,
           modified_time,
           name);
}

void free_names(char **names, size_t count)
{
    for (size_t i = 0; i < count; i++)
    {
        free(names[i]);
    }

    free(names);
}
