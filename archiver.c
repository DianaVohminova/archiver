#include <unistd.h>
#include <stdio.h>
#include <dirent.h> //просмотр содержимого папки
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h> //выделение памяти, преобразование типов
#include <time.h>
#include <libgen.h>

#define MAX_PATH_SIZE 4096
#define BUFFER_SIZE 4096

// функция архивирования файлов
void write_in_archive(char *path, FILE *output) {
    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;

    if ((dp = opendir(path)) == NULL) {
        fprintf(stderr, "cannot open directory: %s\n", path);
        return;
    }

    chdir(path); // смена каталога

    while ((entry = readdir(dp)) != NULL) {
        lstat(entry->d_name, &statbuf);
        char fullpath[MAX_PATH_SIZE];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);

        if (stat(fullpath, &statbuf) == -1) {
            perror("Error stating file");
            continue;
        }

        if (S_ISDIR(statbuf.st_mode)) {
            // если текущая папка или родительская
            if (strcmp(".", entry->d_name) == 0 || strcmp("..", entry->d_name) == 0)
                continue;

            write_in_archive(fullpath, output); // рекурсивный запуск функции
        } else {
            FILE *file = fopen(fullpath, "r"); // открыть файл для чтения
            if (file == NULL) {
                perror("Error opening file");
                continue;
            }

            // Определяем длину файла
            fseek(file, 0, SEEK_END);
            long length = ftell(file);
            fseek(file, 0, SEEK_SET); // читаем файл с начала

            // Записываем информацию о файле в архив
            fprintf(output, "%s|%ld<\n", fullpath, length);

            char buffer[BUFFER_SIZE];
            size_t bytes_read;
            while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
                if (fwrite(buffer, 1, bytes_read, output) != bytes_read) {
                    perror("Error writing to output file"); // ошибка записи в выходной файл
                    fclose(file);
                    exit(1);
                }
            }
            fprintf(output, "\n"); // добавляем разделитель между файлами

            printf("Файл: %s\tДлина: %ld\n", fullpath, length);
            fclose(file);
        }
    }

    chdir(".."); // возвращаемся в родительский каталог
    closedir(dp); // закрываем поток каталога
}

// функция с реализацией архиватора
void archive() {
    //путь к папке с входными файлами
    char path[MAX_PATH_SIZE] = "/home/diana/Документы/files";
    // путь к файлу с архивами
    const char* archivePath = "/home/diana/Документы/archive/archive.arch";
    FILE* archiveFile = fopen(archivePath, "w"); //открыть для изменений, укоротить до нулевой длины
    if (archiveFile == NULL) {
        perror("Error opening output file");
        exit(1);
    }
    printf("Архивация файлов...\n\n");
    write_in_archive(path, archiveFile);
    fclose(archiveFile);
    printf("Архивация завершена.\n\n");
}//archive

// функция разархивирования
void unpacking(const char *archivePath) {
    FILE *archiveFile = fopen(archivePath, "rb");
    if (!archiveFile) {
        perror("Failed to open archive");
        return;
    }
    // Получаем директорию, в которой находится архив
    char dirPath[512];
    strncpy(dirPath, archivePath, sizeof(dirPath));
    dirPath[sizeof(dirPath) - 1] = '\0'; // Обеспечиваем нуль-терминатор
    char *directory = dirname(dirPath); // Получаем директорию

    char currentPath[256];
    size_t bytesRead;
    unsigned char buffer[1024];

    while (fscanf(archiveFile, "%255[^|]|%zu<\n", currentPath, &bytesRead) == 2) {
        // Извлекаем только имя файла без пути
        char *fileName = basename(currentPath);

        // Создаем полный путь с учетом директории архива
        char fullPath[512];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", directory, fileName);

        // Открываем файл для записи
        FILE *outputFile = fopen(fullPath, "wb");
        if (!outputFile) {
            perror("Failed to create output file");
            continue;
        }

        // Читаем данные из архива и записываем в файл
        size_t totalBytesRead = 0;
        while (totalBytesRead < bytesRead && (bytesRead - totalBytesRead) > 0) {
            size_t toRead = (bytesRead - totalBytesRead) > sizeof(buffer) ? sizeof(buffer) : (bytesRead - totalBytesRead);
            size_t n = fread(buffer, 1, toRead, archiveFile);
            fwrite(buffer, 1, n, outputFile);
            totalBytesRead += n;
        }

        fclose(outputFile);
        printf("Файл: %s/%s\tДлина: %ld\n", directory, fileName, totalBytesRead);
    }

    fclose(archiveFile);
}//unpacking
/*
void read_from_archive(char* path) {
    FILE *archive = fopen(path, "r"); //открыть файл для чтения
    if (archive == NULL) {
        perror("Error opening archive");
    }
    char currentPath[MAX_PATH_SIZE];
    char prevPath[MAX_PATH_SIZE];
    
    int length;
    while (fscanf(archive, "%[^|]|%d<\n", currentPath, &length) == 2) {
        //убираем перенос строки при необходимости
        if (currentPath[0] == '\n') {
            memmove(currentPath, currentPath + 1, strlen(currentPath));
        }
        strcpy(prevPath, currentPath);
        printf("Файл: %s; Длина: %d\n", currentPath, length);

        char* folder = strdup(currentPath); //указатель на дублируемую строку пути
        char* fileName = strrchr(folder, '/'); //имя файла
        if (fileName != NULL) {
            *fileName = '\0';  
        }
        char* token;
        char* copy = strdup(folder);
        char* delimiter = "/";
        char builtPath[MAX_PATH_SIZE] = "";

        token = strtok(copy, delimiter); //извлечение / из строки
        struct stat st;
        while (token != NULL) {
            strcat(builtPath, "/");
            strcat(builtPath, token);

            if (stat(builtPath, &st) == -1) {
                int status = mkdir(builtPath, 0777); //создание каталога по умолчанию
                if (status != 0){ //если каталог неуспешно создан
                    printf("Error creating folder\n");
                }
            }
            token = strtok(NULL, delimiter); //продолжить работу с последней указанной строкой
        }
        free(copy); //освобождение блока памяти

        int symbolsToWrite = length;
        int bufferSize;
        char buffer[BUFFER_SIZE];
        size_t bytes_read;
        FILE* currentFile = fopen(currentPath, "w"); //открыть файл для записи
        if (currentFile == NULL) {
            perror("Error opening output file");
            fclose(archive);
        }
 
        while (symbolsToWrite > 0) {
            if (BUFFER_SIZE > symbolsToWrite) {
                bufferSize = symbolsToWrite;
            }
            else {
                bufferSize = BUFFER_SIZE;
            }
            bytes_read = fread(buffer, 1, bufferSize, archive); //считываем по 1 байту в буффер
            if (bytes_read == 0) { //если ничего не считано
                break;
            }
            fwrite(buffer, 1, bytes_read, currentFile); //запись побайтно в текущий файл
            symbolsToWrite -= bytes_read;
            
        }
    
     fclose(currentFile);
    }
    fclose(archive);
}*/
void read_from_archive(const char* path) {
    FILE *archive = fopen(path, "r"); // открыть файл для чтения
    if (archive == NULL) {
        perror("Error opening archive");
        return;
    }

    char currentPath[MAX_PATH_SIZE];
    int length;

    // Создаем директорию /home/diana/Документы/archives, если она не существует
    const char *outputDir = "/home/diana/Документы/archives";
    mkdir(outputDir, 0777);

    while (fscanf(archive, "%[^|]|%d<\n", currentPath, &length) == 2) {
        // Убираем перенос строки при необходимости
        if (currentPath[0] == '\n') {
            memmove(currentPath, currentPath + 1, strlen(currentPath));
        }

        // Извлекаем путь относительно директории files
        const char *baseDir = "files/";
        char *relativePath = strstr(currentPath, baseDir);
        if (relativePath != NULL) {
            relativePath += strlen(baseDir); // Пропускаем 'files/'
        } else {
            relativePath = currentPath; // Если baseDir не найден, используем полный путь
        }

        // Полный путь для записи файла
        char fullPath[MAX_PATH_SIZE];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", outputDir, relativePath);

        printf("Файл: %s; Длина: %d\n", fullPath, length);

        // Создание директорий для файла
        char *folder = strdup(fullPath);
        char *fileName = strrchr(folder, '/'); // имя файла
        if (fileName != NULL) {
            *fileName = '\0';  
        }
        
        char *token;
        char *copy = strdup(folder);
        char builtPath[MAX_PATH_SIZE] = "";

        token = strtok(copy, "/"); // извлечение / из строки
        struct stat st;
        while (token != NULL) {
            strcat(builtPath, "/");
            strcat(builtPath, token);

            if (stat(builtPath, &st) == -1) {
                int status = mkdir(builtPath, 0777); // создание каталога по умолчанию
                if (status != 0) { // если каталог неуспешно создан
                    perror("Error creating folder");
                }
            }
            token = strtok(NULL, "/"); // продолжить работу с последней указанной строкой
        }
        
        free(copy); // освобождение блока памяти

        // Открываем файл для записи в папку archives
        FILE* currentFile = fopen(fullPath, "w"); // открыть файл для записи
        if (currentFile == NULL) {
            perror("Error opening output file");
            free(folder); // освобождаем память
            continue; // пропускаем текущий файл и продолжаем с следующим
        }

        int symbolsToWrite = length;
        size_t bytes_read;
        char buffer[BUFFER_SIZE];

        while (symbolsToWrite > 0) {
            int bufferSize = (symbolsToWrite < BUFFER_SIZE) ? symbolsToWrite : BUFFER_SIZE;
            bytes_read = fread(buffer, 1, bufferSize, archive); // считываем из архива

            if (bytes_read == 0) { // если ничего не считано
                break;
            }
            fwrite(buffer, 1, bytes_read, currentFile); // запись в текущий файл
            symbolsToWrite -= bytes_read;
        }

        fclose(currentFile);
        free(folder); // освобождаем память
    }

    fclose(archive);
}


void extract() {
    char path[MAX_PATH_SIZE] = "/home/diana/Документы/archive/archive.arch";
    char* extension = strrchr(path, '.');
    if (extension == NULL || strcmp(extension, ".arch") != 0) { //если не расширение архива
        printf("Ошибка. Данный файл не является архивом.\n");
        return;
    }
    printf("\nРазархивация...\n");
    //unpacking(path);
    read_from_archive(path);
    printf("\nРазархивация завершена.\n");
}// extract()

int main() {
    while(1) {
        printf("Выберите вариант:\n1 - архивация,\n2 - разархивация,\n0 - завершить работу: ");

        int option = -1;
        if (scanf("%d", &option) != 1) {
            printf("Введен неверный параметр. Попробуйте еще раз.\n");
            while (getchar() != '\n');
            continue;
        }
        switch(option) {
            case 0:
            printf("Завершение работы...\n");
            return 0;
            case 1:
                //архивация
                archive();
                break;
            case 2:
                //разархивация
                extract();
                break;
            default:
                printf("Введен неверный параметр. Попробуйте еще раз.\n");
                break;
        }//switch
    }//while
}//main()