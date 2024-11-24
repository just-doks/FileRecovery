#include "stdio.h"
#include "stdlib.h"
#include "limits.h"
#include <string.h>

// функция отображения данных секторами, по смещению - (file, amount of sectors, offset)
void showBytesOf(FILE *file, unsigned int sectors, unsigned int offset) {

    unsigned char a = 0; // буфер для байта
    short int flag = -7; // флаг для красивого вывода
    int i = 0, j = 0; // счётчики
    fseek(file, offset, SEEK_SET); // возвращаем указатель в файле на начало

    // здесь реализован красивый вывод
    for (i = 0; i < sectors * 0x20; i++) { // смещение по оси Y

        for (j = 0, flag = -7; j < 0x10; j++, flag++) { // смещение по оси X

            fread(&a, 1, 1, file); // Читаем байты из файла
            if (a >= 0x0 && a <= 0xF) { // Рисуем нолик для красоты, если байт меньше 16
                printf ("0%X ", a);
            } else {
                printf("%X ", a);
            }

            if (flag == 0) printf("%s", "    "); // пропуск после каждого восьмого байта
        }

        printf("%s", "\n"); // переход на новую строку каждые 16 байт
        if ((i+1) % 0x20 == 0) printf("%s", "\n"); // делаем пропуск после конца каждого сектора
        
    }

    fseek(file, 0, SEEK_SET); // возвращаем указатель в файле на начало

}

// функция рисует таблицу байт в 16-ричной записи из массива сырых данных, взятых из файла
// развер таблицы задается байтами
void paintTable(unsigned char *array, unsigned int size) {
    unsigned int i = 0;
    for (i = 0; i < size; i++) {
        if (array[i] < 0x10) {
            printf("0%X ", array[i]);
        } else {
            printf("%X ", array[i]);
        }
        
        if ((i+1) % 8 == 0) printf("  ");
        if ((i+1) % 16 == 0) printf("\n");
        if (i > 510) {
            printf("\n");
            break;
        }
    }
}

// функция поиска и вывода числа длиной от 1 до 4 байт из файла
// в действительнсоти, не актуальна
unsigned int get4BytesFromF(FILE *file, unsigned int offset, int bytes) {
    
    unsigned int buf = 0, num = 0; // буфер для байта
    switch (bytes) {
        case 4:
            fseek(file, offset+3, SEEK_SET);
            fread(&buf, 1, 1, file);
            num += buf * 0x1000000;
        case 3:
            fseek(file, offset+2, SEEK_SET);
            fread(&buf, 1, 1, file);
            num += buf * 0x10000;
        case 2:
            fseek(file, offset+1, SEEK_SET);
            fread(&buf, 1, 1, file);
            num += buf * 0x100;
        case 1:
            fseek(file, offset, SEEK_SET);
            fread(&buf, 1, 1, file);
            fseek(file, 0, SEEK_SET); // возвращаем указатель в файле на начало
            num += buf;
            break;
        default:
            break;
        }
    
    return num;
}

// функция переводит запись байтов из массива от 1 до 4 байт в числовое выражение
// актуальна при сохранении сырых данных из файла в массив
unsigned int get4BytesFromAr(unsigned char * arr, unsigned int ofst, int bytes) {
    unsigned int num = 0;
    switch (bytes) {
        case 4:
            num += arr[ofst + 3] * 0x1000000;
        case 3:
            num += arr[ofst + 2] * 0x10000;
        case 2:
            num += arr[ofst + 1] * 0x100;
        case 1:
            num += arr[ofst];
            break;
        default:
            break;
    }
    return num;
}

// функция заносит определенный отрезок данных из файла в массив
void putInArray(FILE *file, char *arr, unsigned int size, unsigned int offset) {
    unsigned int i = 0;
    unsigned char a;
    fseek(file, offset, SEEK_SET); // возвращаем указатель в файле на начало
    for (i = 0; i < size; i++) {
        fread(&a, 1, 1, file);
        *(arr + i) = a;
    }
    fseek(file, 0, SEEK_SET); // возвращаем указатель в файле на начало
}


// makes string with type of file system info
void getTypeofFS(FILE *file, char *typeOfFS, int size, unsigned int offset) {
    unsigned char a = 0;
    int i = 0;
    for (i = 0; i < size-1; i++) {
        fseek(file, offset + i, SEEK_SET);
        fread(&a, 1, 1, file);
        fseek(file, 0, SEEK_SET); // возвращаем указатель в файле на начало
        *(typeOfFS + i) = a;
    }
    *(typeOfFS + size-1) = '\0';
}

void getTypeofFile(unsigned char *arr, unsigned char *typeOfFile, int size, unsigned int offset) {
    unsigned char a = 0;
    int i = 0;
    for (i = 0; i < size-1; i++) {
        a = *(arr + offset + i);
        *(typeOfFile + i) = a;
    }
    *(typeOfFile + size-1) = '\0';
}

void copyFAT(unsigned char *from, unsigned char *to, unsigned int size) {
    unsigned int i = 0;
    for (i = 0; i < size; i++) {
        *(to + i) = *(from + i);
    }
}

// Основная функция, восстанавливающая цепочку кластеров в таблице ФАТ
int restoreFAT(unsigned char * fat, unsigned int sizeOfFAT, unsigned int adress, unsigned int fileSize) {
    
    // Если по адресу уже что-то находится - завершить работу функции и вернуть 1
    if (fat[adress*2] != 0 || fat[adress*2+1] != 0) return 1;
    
    // Вернуть 2, если файл ничего не весит
    if (fileSize == 0) return 2;

    // получаем расчетное количество кластеров, занимаемых файлов
    unsigned int amount_of_clusters = fileSize / 16384;
    if (fileSize % 16384 != 0) amount_of_clusters++;
    

    printf("amount of clusters = %d\n", amount_of_clusters);
    unsigned int flag = adress;
  
    unsigned int j = adress;

    unsigned int i = 0;

    // Если файл занимает больше одного кластера - выстраиваем цепочку
    for (i = 1; i < amount_of_clusters; i++) {
        j++;
        if (j == sizeOfFAT) j = 0;

        if (j == adress) return 3;

        if (fat[j*2] == 0 && fat[j*2 + 1] == 0) {
            fat[flag*2] = j%0x100;
            fat[flag*2 + 1] = j/0x100;
            flag = j;
        }
    }
    printf("marker2\n");
    // Последнюю ячейку помечаем конечным кодом
    fat[j*2] = 0xFF;
    fat[j*2+1] = 0xFF;
             
    return 0;
}

// Функция проверяет кластеры в области данных в соответствии с цепочкой адресов в таблице ФАТ
// чтобы определить количество байт данных
unsigned int bytesAmountOf(unsigned int start_adress, unsigned char *fat, unsigned int offset, FILE *file) {
    unsigned int i = 0;
    unsigned int amount = 0;
    unsigned int adress = start_adress;
    unsigned char a = 0;

    do {
        // перемещаем указатель в конец первого кластера файла
        // если байт не нулевой - засчитываем 16 килобайт к весу файла и движемся к след кластеру
        fseek(file, offset + 0x4000 * (adress - 1) + 0x3FFF, SEEK_SET);
        fread(&a, 1, 1, file);
        if (a != 0 && adress != 0xFFFF) {
            amount  += 0x4000;
            adress = get4BytesFromAr(fat, adress * 2, 2);
            continue;
        }

        // Если последний байт кластера не нулевой - считаем ненулевые байты
        for (i = 0; i < 0x4000; i++) {
            fseek(file, offset + i + 0x4000 * (adress - 1), SEEK_SET);
            fread(&a, 1, 1, file);
            if (a == 0) break;
            amount++;
        }
         
        adress = get4BytesFromAr(fat, adress * 2, 2);

    } while (adress != 0xFFFF);

    fseek(file, 0, SEEK_SET);
    return amount;
}

// MAIN FUNCTION
int main(void) {
    FILE *file;

    printf("Enter file's name: ");
    unsigned char nameOfFile[20];
    scanf("%s", nameOfFile);
    file = fopen(nameOfFile, "rb+");
    
    // проверяем, открылся ли файл
    printf("File accessing: ");
    if (file == NULL) {
        printf("Error.\n");
        exit(2);
    } else {
        printf("%s\n", "Success!\n");
    }

    int i = 0, j = 0;
    
    // 4 байта на переменную - достаточно для работы с фат16
    unsigned int size_of_sector = 512; // задаем по умолчанию bytes
    unsigned int bootSector_Offset = 0x4000; // ЗС имеет смещение в 16кб

    unsigned int typeOfFS_offset = 0x36; // 8 bytes
    unsigned char typeOfFS[9]; // 8 bytes
    getTypeofFS(file, typeOfFS, 9, bootSector_Offset + typeOfFS_offset);

    
    if (typeOfFS[0] != 'F' || typeOfFS[1] != 'A' || typeOfFS[2] != 'T' || typeOfFS[3] != '1' || typeOfFS[4] != '6') {
        printf("Wrong File System or size of Cluster. Should be FAT16 with 16Kb cluster size.\n");
        return 0;
    }

    unsigned int secPerClustr_offset = 0x0D; // 1 byte
    unsigned int reservedSec_offset = 0x0E; // 2 bytes
    unsigned int secPerFAT_offset = 0x16; // 2 bytes
    unsigned int sizeofRootDir_offset = 0x11; // 2 bytes
    
    unsigned int sectorsPerCluster = get4BytesFromF(file, bootSector_Offset + secPerClustr_offset, 1);

    unsigned int reservedSectors = 0;
    fseek(file, bootSector_Offset + reservedSec_offset, SEEK_SET);
    fread(&reservedSectors, 1, 2, file);

    unsigned int sectors_per_FAT = get4BytesFromF(file, bootSector_Offset + secPerFAT_offset, 2);

    unsigned int size_of_FAT = sectors_per_FAT * size_of_sector;
    unsigned int size_of_RootDirectory = get4BytesFromF(file, bootSector_Offset + sizeofRootDir_offset, 2) * 32;

    printf("Type of File System: %s\n", typeOfFS);
    printf("Sectors per cluster = %d\n", sectorsPerCluster); // выводим кол-во секторов в кластере
    printf("Reserved sectors = %d\n", reservedSectors);

    printf("Sectors per FAT = %d\n", sectors_per_FAT); // выводим кол-во секторов в FAT1/2
    printf("size of FAT = %d KBytes\n", size_of_FAT/1024);
    printf("Size of root directory = 0x%X\n", size_of_RootDirectory);
    printf("_____________________________\n\n\n");
    
    // помещаем таблицу ФАТ1 в массив для последующий взаимодействий
    unsigned int FAT1_offset = bootSector_Offset + reservedSectors * size_of_sector;
    unsigned int FAT2_offset = FAT1_offset + size_of_FAT;

    unsigned char FAT1[size_of_FAT];
    putInArray(file, FAT1, size_of_FAT, FAT1_offset);
    unsigned char FAT2[size_of_FAT];
    copyFAT(FAT1, FAT2, size_of_FAT);

    // помещаем кластер с корневой директорией целиком в массив - поскольку на него выделена область,
    // следующая сразу после таблиц ФАТ, то не нужно выяснять, в каких кластерах директория располагается
    unsigned char RootDir[size_of_RootDirectory];

    unsigned int dataSegment_offset = FAT2_offset + size_of_FAT;
    unsigned int rD_offset = dataSegment_offset;
    putInArray(file, RootDir, size_of_RootDirectory, rD_offset);

    // Восстановление данных происходит ниже

    unsigned int amount_of_delFiles = 0;
    unsigned int flag = 0; // флаг конца "списка элементов" корневого каталога

    for (i = 0; i < size_of_RootDirectory; i = i + 32) {
        if (RootDir[i] == 0xE5 && (RootDir[i + 0x1A] != 0 || RootDir[i + 0x1B] != 0)) {
            amount_of_delFiles++;
        }
        if (RootDir[i] == 0) {
            flag = i; // получает адрес нулевого байта (код последнего эл) в корневом каталоге
            break;
        }
    }

    // Завершить программу, если нету удалённых элементов
    if (amount_of_delFiles == 0) {
        printf("nothing to restore.\n");
        return 0;
    }

    printf("amount of deleted Files = %d\n", amount_of_delFiles);
    printf("_____________________________\n\n\n");

    unsigned int size_test = 0;
    unsigned int delFile_Adress;
    unsigned int size_of_delFile;
    unsigned char type_of_file[4];
    for (i = flag - 32; i >= 0 ; i = i - 32) { // -32

         if (RootDir[i] == 0xE5 && get4BytesFromAr(RootDir, i + 0x1A, 2) != 0) {

            delFile_Adress = get4BytesFromAr(RootDir, i + 0x1A, 2);
            size_of_delFile = get4BytesFromAr(RootDir, i + 0x1C, 4);
            printf("Deleted File's Adress = 0x%X\n", delFile_Adress);

            getTypeofFile(RootDir, type_of_file, 4, i + 0x8);
            printf("Type of file: %s\n", type_of_file);
            printf("_____________________________\n");
            
            if (restoreFAT(FAT2, size_of_FAT, delFile_Adress, size_of_delFile) != 0) {
                printf("Error. Impossible to restore the file.\n");
                printf("The sequense of clusters cannot be restored.\n");
                printf("//////////////////////////////\n\n\n");
                copyFAT(FAT1, FAT2, size_of_FAT);
                continue;
            }
            
            size_test = bytesAmountOf(delFile_Adress, FAT2, dataSegment_offset, file);

            if (size_test != size_of_delFile) {
                    printf("Error. different sizes: must be - %d Bytes, checked - %d Bytes\n", size_of_delFile, size_test);
                    printf("//////////////////////////////\n\n\n");
                    copyFAT(FAT1, FAT2, size_of_FAT);
              continue;
            }

            printf("Size of File = %d Bytes\n", size_of_delFile);
            printf("Checked size: %d Bytes\n", size_test);

            copyFAT(FAT2, FAT1, size_of_FAT);
            RootDir[i] = 0x5F;

            printf("-----File restored-----\n");
            printf("//////////////////////////////\n\n\n");
        }
    }
    
    // запись результатов в файл
    
    fseek(file, FAT1_offset, SEEK_SET);
    fwrite(FAT1, 1, size_of_FAT, file);

    fseek(file, FAT2_offset, SEEK_SET);
    fwrite(FAT1, 1, size_of_FAT, file);

    fseek(file, rD_offset, SEEK_SET);
    fwrite(RootDir, 1, size_of_RootDirectory, file);


    fclose(file);
    return 0;
}
