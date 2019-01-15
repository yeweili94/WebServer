#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>

struct person
{
    int age;
    // char name[32];
};

int main()
{
    struct person p;
    p.age = 1000;
    int size = sizeof(struct person);
    // printf("the age is %d\n", p.age);
    // printf("the name is %s\n", p.name);
    FILE* file = fopen("/home/yeweili/github/WebServer/WebServer/aaa", "aeb+");
    fwrite(&p, 1, size, file);
    fclose(file);

    FILE* readfile = fopen("/home/yeweili/github/WebServer/WebServer/aaa", "rb");
    struct person p1;
    fread(&p1, size, 1, readfile);
    printf("the age is %d\n", p1.age);

}
