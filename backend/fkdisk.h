#ifndef FKDISK_H
#define FKDISK_H
#include <exception>
#include <ios>
#include <cstdlib>
#include <string>
#include <iostream>
#include <cstring>
#include <fstream>
#include "structures.h"

using namespace std;

namespace CmdFkDisk {
    inline string expandPath(const string& path){
        if(path.empty() || path[0] != '~'){
            return path;
        }

        const char* home = getenv("HOME");

        if(!home){
            cerr << "Error: el directorio HOME no existe" << endl;
            return path;
        }

        return string(home) + path.substr(1);
    }

    inline string deletePartition(const string& path, const string& name, const string& deleteType){
        fstream diskFile(path, ios::binary | ios::in | ios::out);
        if(!diskFile.is_open()) return "Error: no se pudo encontrar el disco";

        MBR mbr = {};

        diskFile.seekg(0, ios::beg);
        diskFile.read(reinterpret_cast<char*>(&mbr), sizeof(MBR));

        int partIndex = -1;
        int extIndex = -1;

        for(int i = 0; i < 4; i++){
            if(mbr.mbr_partitions[i].part_status == '1'){
                if(strcmp(mbr.mbr_partitions[i].part_name, name.c_str()) == 0)  {
                    partIndex = i;
                }
                if(mbr.mbr_partitions[i].part_type == 'E'){
                    extIndex = i;
                }
            }
        }

        if(partIndex != -1){
            mbr.mbr_partitions[partIndex].part_status = '0';

            if(deleteType == "full"){
                char zero = '\0';
                diskFile.seekp(mbr.mbr_partitions[partIndex].part_start, ios::beg);
                for( int i = 0; i < mbr.mbr_partitions[partIndex].part_size;  i++) {
                    diskFile.write(&zero, 1);
                }
            }

            diskFile.seekp(0, ios::beg);

            diskFile.write(reinterpret_cast<char*>(&mbr), sizeof(MBR));
            diskFile.close();
            return "Particion '" + name + "' eliminada (Modo: " + deleteType + ") exitosamente.";
        }

        if(extIndex != -1 ){
            int currentEBRPos = mbr.mbr_partitions[extIndex].part_start;
            EBR currentEBR = {};

            while(currentEBRPos != -1){
                diskFile.seekg(currentEBRPos, ios::beg);
                diskFile.read(reinterpret_cast<char*>(&currentEBR), sizeof(EBR));

                if(currentEBR.part_status == '1' && strcmp(currentEBR.part_name, name.c_str() ) == 0){
                    currentEBR.part_status = '0';

                    diskFile.seekp(currentEBRPos, ios::beg);
                    diskFile.write(reinterpret_cast<char*>(&currentEBR), sizeof(EBR));

                    if(deleteType == "full"){
                        char zero = '\0';
                        diskFile.seekp(currentEBR.part_start, ios::beg);
                        for(int i = 0; i < currentEBR.part_size; i++){
                            diskFile.write(&zero, 1);
                        }
                    }

                    diskFile.close();
                    return "Particion logica '" + name + "' eliminada (Modo: " + deleteType + ") exitosamente.";
                
                }

                currentEBRPos = currentEBR.part_next;
            }
        }

        diskFile.close();
        return "Error: no se encontro la particion '" + name + "' para eliminar.";
    }


    inline string addPartition(const string& path, const string& name, int addBytes) {
        fstream diskFile(path, ios::binary | ios::in | ios::out);
        if(!diskFile.is_open()) return "Error: no se pudo abrir el disco";

        MBR mbr = {};
        diskFile.seekg(0, ios::beg);
        diskFile.read(reinterpret_cast<char*>(&mbr), sizeof(MBR));

        int partIndex = -1;
        for(int i = 0; i < 4; i++) {
            if(mbr.mbr_partitions[i].part_status == '1' && strcmp(mbr.mbr_partitions[i].part_name, name.c_str()) == 0) {
                partIndex = i;
                break;
            }
        }

        if(partIndex != -1){
            int currentSize = mbr.mbr_partitions[partIndex].part_size;
            int startPos = mbr.mbr_partitions[partIndex].part_start;

            if(addBytes < 0 ){
                if(currentSize + addBytes <= 0) {
                    diskFile.close();
                    return "Error: El tamaño a quitar deja la particion en 0 o negativo.";
                }
                mbr.mbr_partitions[partIndex].part_size += addBytes;
            } else {
                int nextStart = mbr.mbr_size; 
                for(int i = 0; i < 4; i++) {
                    if(mbr.mbr_partitions[i].part_status == '1' && mbr.mbr_partitions[i].part_start > startPos) {
                        if(mbr.mbr_partitions[i].part_start < nextStart) {
                            nextStart = mbr.mbr_partitions[i].part_start;
                        }
                    }
                }
                
                int availableSpace = nextStart - (startPos + currentSize);
                if(availableSpace < addBytes) {
                    diskFile.close();
                    return "Error: Espacio insuficiente. Libres: " + to_string(availableSpace) + " bytes. Requeridos: " + to_string(addBytes) + " bytes.";
                }
                mbr.mbr_partitions[partIndex].part_size += addBytes;
            }
            diskFile.seekp(0, ios::beg);
            diskFile.write(reinterpret_cast<char*>(&mbr), sizeof(MBR));
            diskFile.close();
            return "Particion '" + name + "' redimensionada exitosamente. Nuevo tamaño: " + to_string(mbr.mbr_partitions[partIndex].part_size) + " bytes.";
        }


        diskFile.close();
        return "Error: la particion '" + name + "' no existe";

    }



    inline string createPrimaryExtendedPartition(const string& path, int size, char type, char fit, const string& name){
        fstream diskFile(path, ios::binary | ios::in | ios::out);

        if(!diskFile.is_open()){
            return "Error: no se pudo abrir el disco";
        }

        

        MBR mbr = {}; 
        diskFile.seekg(0, ios::beg);
        diskFile.read(reinterpret_cast<char*>(&mbr), sizeof(MBR));

        for (int i = 0; i < 4; i++){
            if(mbr.mbr_partitions[i].part_status == '1' && strcmp(mbr.mbr_partitions[i].part_name, name.c_str()) == 0){
                diskFile.close();
                return "Error: ya existe una particion con ese nombre";
            }
        }

        int partCount = 0;
        bool hasExtended = false;

        for(int i = 0; i < 4; i++){
            if(mbr.mbr_partitions[i].part_status == '1'){
                partCount++;
                if(mbr.mbr_partitions[i].part_type == 'E'){
                    hasExtended = true;
                }
            }
        }

        if(partCount >= 4){
            diskFile.close();
            return "Error: Ya existen 4 particiones ";
        }

        if(type == 'E' && hasExtended){
            diskFile.close();
            return "Error: ya existe una particion extendida";
        }

        int selectedSlot = -1;
        int bestStart = -1;

        if(fit == 'F'){
            for(int i = 0; i < 4; i++){
                if(mbr.mbr_partitions[i].part_status == '0'){
                    int currentPos = sizeof(MBR);
                    bool changed = true;
                    while(changed){
                        changed = false;
                        for(int j = 0; j < 4; j++){
                            if(mbr.mbr_partitions[j].part_status == '1' && mbr.mbr_partitions[j].part_start == currentPos ){
                                currentPos = mbr.mbr_partitions[j].part_start + mbr.mbr_partitions[j].part_size;
                                changed = true;
                            }
                        }
                    }

                    int spaceAvaible = mbr.mbr_size - currentPos;
                    if(spaceAvaible >= size){
                        selectedSlot = i;
                        bestStart = currentPos;
                        break;
                    }
                }
            }
        } else if(fit == 'B'){
            int minWaste = mbr.mbr_size;
            for(int i = 0; i < 4; i++){
                if(mbr.mbr_partitions[i].part_status == '0'){
                    int currentPos = sizeof(MBR);
                    bool changed = true;
                    while(changed){
                        changed = false;
                        for(int j = 0; j < 4; j++){
                            if(mbr.mbr_partitions[j].part_status == '1' && mbr.mbr_partitions[j].part_start == currentPos ){
                                currentPos = mbr.mbr_partitions[j].part_start + mbr.mbr_partitions[j].part_size;
                                changed = true;
                            }
                        }
                    }

                    int spaceAvaible = mbr.mbr_size - currentPos;
                    int waste = spaceAvaible - size;
                    if(spaceAvaible >= size && waste < minWaste){
                        selectedSlot = i;
                        bestStart = currentPos;
                        minWaste = waste;
                    }
                }
            }
        } else {
            int maxSpace = 0;
            for(int i = 0; i < 4; i++){
                if(mbr.mbr_partitions[i].part_status == '0'){
                    int currentPos = sizeof(MBR);
                    bool changed = true;
                    while(changed){
                        changed = false;
                        for(int j = 0; j < 4; j++){
                            if(mbr.mbr_partitions[j].part_status == '1' && mbr.mbr_partitions[j].part_start == currentPos ){
                                currentPos = mbr.mbr_partitions[j].part_start + mbr.mbr_partitions[j].part_size;
                                changed = true;
                            }
                        }
                    }

                    int spaceAvaible = mbr.mbr_size - currentPos;
                    if(spaceAvaible >= size && spaceAvaible > maxSpace){
                        selectedSlot = i;
                        bestStart = currentPos;
                        maxSpace = spaceAvaible;
                    }
                }
            }
        }

        if(selectedSlot == -1 || bestStart == -1){
            diskFile.close();
            return "Error: no hay espacio sufieciente en el disco";
        }

        mbr.mbr_partitions[selectedSlot].part_status = '1';
        mbr.mbr_partitions[selectedSlot].part_type = type;
        mbr.mbr_partitions[selectedSlot].part_fit = fit;
        mbr.mbr_partitions[selectedSlot].part_start = bestStart;
        mbr.mbr_partitions[selectedSlot].part_size = size;
        strncpy(mbr.mbr_partitions[selectedSlot].part_name, name.c_str(), 16);

        if(type == 'E'){
            EBR ebr = {};
            ebr.part_status = '0';
            ebr.part_fit = fit;
            ebr.part_start = bestStart; 
            ebr.part_size = 0;
            ebr.part_next = -1;
            memset(ebr.part_name, 0 , 16);

            diskFile.seekp(bestStart, ios::beg);
            diskFile.write(reinterpret_cast<char*>(&ebr), sizeof(EBR));
        }

        diskFile.seekp(0, ios::beg);
        diskFile.write(reinterpret_cast<char*>(&mbr), sizeof(MBR));
        diskFile.close();

        return "Partición " + string(1, type) + " '" + name + "' creada exitosamente\n" +
               "  Inicio: " + to_string(bestStart) + "\n" +
               "  Tamaño: " + to_string(size) + " bytes\n" +
               "  Ajuste: " + string(1, fit);
    }

    inline string createLogicalPartition(const string& path, int size, char fit, const string& name){
        fstream diskFile(path, ios::binary | ios::in | ios::out);
        if(!diskFile.is_open()){
            return "Error: no se pudo abrir el disco";
        }

        MBR mbr = {};
        diskFile.seekg(0, ios::beg);
        diskFile.read(reinterpret_cast<char*>(&mbr), sizeof(MBR));

        int exIndex = -1;
        for(int i = 0 ; i < 4; i++){
            if(mbr.mbr_partitions[i].part_status == '1' && mbr.mbr_partitions[i].part_type == 'E'){
                exIndex = i;
                break;
            }
        }

        if(exIndex == -1){
            diskFile.close();
            return "Error: No existe una particion extendida para crear particiones logicas";
        }

        Partition& extended = mbr.mbr_partitions[exIndex];
        int extStart = extended.part_start;
        int extEnd = extended.part_start + extended.part_size;

        EBR currentEBR = {};
        diskFile.seekg(extStart, ios::beg);
        diskFile.read(reinterpret_cast<char*>(&currentEBR), sizeof(EBR));

        if(currentEBR.part_status == '0'){
            currentEBR.part_status = '1';
            currentEBR.part_fit = fit;
            currentEBR.part_start = extStart + sizeof(EBR);
            currentEBR.part_size = size;
            currentEBR.part_next = -1;
            strncpy(currentEBR.part_name, name.c_str(), 16);

            diskFile.seekp(extStart, ios::beg);
            diskFile.write(reinterpret_cast<char*>(&currentEBR), sizeof(EBR));
            diskFile.close();

            return "Particion logica '" + name + "' creada exitosamente \n" +
                "  Inicio: " + to_string(currentEBR.part_start) + " \n" + 
                "  Tamaño: " + to_string(size) + " bytes";
        }

        int currentEBRPos = extStart;
        while(true){
            diskFile.seekg(currentEBRPos, ios::beg);
            diskFile.read(reinterpret_cast<char*>(&currentEBR), sizeof(EBR));
            
            if(currentEBR.part_status == '1' && strcmp(currentEBR.part_name,name.c_str()) == 0){
                diskFile.close();
                return "Error: Ya existe una particion logica con ese nombre";
            }

            if(currentEBR.part_next == -1){
                int nxtEBRPos = currentEBR.part_start + currentEBR.part_size;
                int spaceAvaible = extEnd - nxtEBRPos - sizeof(EBR);

                if(spaceAvaible < size){
                    diskFile.close();
                    return "Error: No hay espacio suficiente en la particion extendida";
                }

                EBR newEbr = {};
                newEbr.part_status = '1';
                newEbr.part_fit = fit;
                newEbr.part_start = nxtEBRPos + sizeof(EBR);
                newEbr.part_size = size;
                newEbr.part_next = -1;
                strncpy(newEbr.part_name, name.c_str() , 16);

                currentEBR.part_next = nxtEBRPos;
                diskFile.seekp(currentEBRPos, ios::beg);
                diskFile.write(reinterpret_cast<char*>(&currentEBR), sizeof(EBR));

                diskFile.seekp(nxtEBRPos, ios::beg);
                diskFile.write(reinterpret_cast<char*>(&newEbr), sizeof(EBR));
                
                diskFile.close();

                return "Particion logica '" + name + "' creada exitosamente \n" +
                "  Inicio: " + to_string(newEbr.part_start) + " \n" + 
                "  Tamaño: " + to_string(size ) + " bytes";
            }
            currentEBRPos = currentEBR.part_next;
        }
    }

    inline string executeCmd (int size, int add_val, const string& unit, const string& path, const string& type, const string& fit, const string& deleteType, const string& name){
        try {
            string expandedPath = expandPath(path);
                
            ifstream checkFile(expandedPath);
            
            if(!checkFile.good()){
                checkFile.close();
                return "Error: el disco no existe";
            }
            checkFile.close();

            if (deleteType.empty() && add_val == 0) {
                if (size <= 0) {
                    return "Error: el tamaño debe ser mayor a 0";
                }
            }

            if(!deleteType.empty()){
                string lowerDelete = deleteType;
                for(auto& c : lowerDelete) c = tolower(c);
                if(lowerDelete != "fast" && lowerDelete != "full") return "Error: -delete solo acepta 'fast' o 'full'";
                return deletePartition(expandedPath, name, lowerDelete);
            }

            if(name.empty()){
                return "Error: se requiere el parametro -name";
            }

            
            int sizeInBytes = 0;
            int addInBytes = 0;
            
            string u = unit.empty() ? "k" : unit; 

            if(u == "b" || u == "B"){
                sizeInBytes = size;
                addInBytes = add_val;
            } else if(u == "k" || u == "K"){
                sizeInBytes = size * 1024;
                addInBytes = add_val * 1024;
            } else if(u == "m" || u == "M"){
                sizeInBytes = size * 1024 * 1024;
                addInBytes = add_val * 1024 * 1024;
            } else {
                return "Error: unidad no valida. Use B, K o M";
            }
            
            if(add_val != 0) {
                return addPartition(expandedPath, name, addInBytes);
            }

            char partType = 'P';
            if(!type.empty()){
                if(type == "p" || type == "P"){
                    partType = 'P';
                } else if(type == "e" || type == "E"){
                    partType = 'E';
                } else if(type == "l" || type == "L"){
                    partType = 'L';
                } else {
                    return "Error: Tipo invalido. Use P: primario, E extendida, L logica";
                }
            }

            char partFit = 'W';
            if(!fit.empty()){
                if(fit == "bf" || fit == "BF"){
                    partFit = 'B';
                } else if (fit == "ff" || fit == "FF"){
                    partFit = 'F';
                } else if (fit == "wf" || fit == "WF"){
                    partFit = 'W';
                } else {
                    return "Error: Fit invalido. Use WF, FF, BF";
                }
            }
            if(partType == 'L'){
                return createLogicalPartition(expandedPath, sizeInBytes, partFit, name);
            } else {
                return createPrimaryExtendedPartition(expandedPath, sizeInBytes, partType, partFit, name);
            }

        } catch (const exception& e) {
            return string("Error en fdisk: " ) + e.what();
        }
    }
}

#endif // !FKDISK_H