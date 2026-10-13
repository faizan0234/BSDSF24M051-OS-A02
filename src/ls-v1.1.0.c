
/*
 * Programming Assignment 02
 * Version 1.1.0 - Long Listing Format
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <getopt.h>
#include <limits.h>

void do_ls(const char *dir, int long_format);
void print_long_entry(const char *dir, const char *name);
void format_permissions(mode_t mode, char permissions[11]);

int main(int argc, char *argv[])
{
    int long_format = 0;
    int option;

    while ((option = getopt(argc, argv, "l")) != -1)
    {
        switch (option)
        {
            case 'l':
                long_format = 1;
                break;

            default:
                fprintf(stderr, "Usage: %s [-l] [directory ...]\n",
                        argv[0]);
                return EXIT_FAILURE;
        }
    }

    if (optind == argc)
    {
        do_ls(".", long_format);
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

            do_ls(argv[i], long_format);

            if (i < argc - 1)
            {
                putchar('\n');
            }
        }
    }

    return EXIT_SUCCESS;
}

void do_ls(const char *dir, int long_format)
{
    DIR *dp = opendir(dir);

    if (dp == NULL)
    {
        fprintf(stderr, "%s: ", dir);
        perror("cannot open directory");
        return;
    }

    struct dirent *entry;

    errno = 0;

    while ((entry = readdir(dp)) != NULL)
    {
        /* Hidden entries are excluded, like ordinary ls by default. */
        if (entry->d_name[0] == '.')
        {
            continue;
        }

        if (long_format)
        {
            print_long_entry(dir, entry->d_name);
        }
        else
        {
            printf("%s\n", entry->d_name);
        }

        errno = 0;
    }

    if (errno != 0)
    {
        perror("readdir failed");
    }

    closedir(dp);
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

    /* Display special permission bits. */
    if (mode & S_ISUID)
        permissions[3] = (mode & S_IXUSR) ? 's' : 'S';

    if (mode & S_ISGID)
        permissions[6] = (mode & S_IXGRP) ? 's' : 'S';

    if (mode & S_ISVTX)
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

    /* lstat() obtains metadata about the link itself, if it is a symlink. */
    if (lstat(path, &file_info) == -1)
    {
        perror(path);
        return;
    }

    struct passwd *owner = getpwuid(file_info.st_uid);
    struct group *group = getgrgid(file_info.st_gid);

    char permissions[11];
    format_permissions(file_info.st_mode, permissions);

    /* ctime() includes a newline; print only the date/time portion. */
    char *modified_time = ctime(&file_info.st_mtime);

    if (modified_time == NULL)
    {
        modified_time = "??? ??? ?? ??:??:?? ????\n";
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
