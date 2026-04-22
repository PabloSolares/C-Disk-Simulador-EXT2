#ifndef JOURNAL_H
#define JOURNAL_H

#include <iostream>
#include <fstream>
#include <cstring>
#include <ctime>
#include <sstream>
#include <iomanip>
#include "structures.h"
#include "mount.h"

using namespace std;

namespace FsJournal {

    inline void addJournalEntry(const string& id, const string& operation, const string& path, const string& content){
        CmdMount::MountedPartition partition;
        if(!CmdMount::getMountedPar(id, partition)) return;

        fstream file(partition.path, ios::binary | ios::in | ios::out);
        if(!file.is_open()){
            return;
        }

        Superblock sb;
        file.seekg(partition.start, ios::beg);
        file.read(reinterpret_cast<char*>(&sb), sizeof(Superblock));

        if(sb.s_filesystem_type != 3){
            file.close();
            return;
        }

        if(sb.s_filesystem_type != 3){
            file.close();
            return;
        }

        int journal_start = partition.start + sizeof(Superblock);
        Journal currentJournal;
        int targetPos = -1;
        int nextCount = 1;

        for(int i = 0; i < 50; i++){
            file.seekg(journal_start + (i*sizeof(Journal)), ios::beg);
            file.read(reinterpret_cast<char*>(&currentJournal), sizeof(Journal));

            if(currentJournal.j_count == 0 && targetPos == -1){
                targetPos = journal_start + (i * sizeof(Journal));
            } else if(currentJournal.j_count > 0){
                nextCount = currentJournal.j_count + 1;
            }
        }

        if(targetPos == -1){
            targetPos = journal_start;
        }

        Journal newJournal;
        newJournal.j_count = nextCount;
        strncpy(newJournal.j_content.i_operation, operation.c_str(), 9);
        strncpy(newJournal.j_content.i_path, path.c_str(), 31);
        strncpy(newJournal.j_content.i_content, content.c_str(), 63);
        newJournal.j_content.i_date = static_cast<time_t>(time(nullptr));

        file.seekp(targetPos, ios::beg);
        file.write(reinterpret_cast<char*>(&newJournal), sizeof(Journal));
        file.close();
    }

    inline string exucuteJournaling(const string& id){
        if (id.empty()) return "Error: El comando journaling requiere el parametro -id";

        CmdMount::MountedPartition partition;
        if (!CmdMount::getMountedPar(id, partition)) return "Error: La particion con ID '" + id + "' no esta montada.";

        ifstream file(partition.path, ios::binary);
        if (!file.is_open()) return "Error: No se pudo abrir el disco.";

        Superblock sb;
        file.seekg(partition.start, ios::beg);
        file.read(reinterpret_cast<char*>(&sb), sizeof(Superblock));

        if (sb.s_filesystem_type != 3) {
            file.close();
            return "Error: La particion '" + id + "' no esta formateada en EXT3. No tiene Journaling.";
        }

        int journal_start = partition.start + sizeof(Superblock);
        Journal currentJournal;
        
        ostringstream result;
        result << "\n***************** JOURNALING *****************\n";
        result << left << setw(12) << "Operacion" 
               << setw(30) << "Path" 
               << setw(20) << "Contenido" 
               << "Fecha\n";
        result << "--------------------------------------------------------------------------------\n";

        bool hasData = false;

        for (int i = 0; i < 50; i++) {
            file.seekg(journal_start + (i * sizeof(Journal)), ios::beg);
            file.read(reinterpret_cast<char*>(&currentJournal), sizeof(Journal));

            if (currentJournal.j_count > 0) {
                hasData = true;
                
                time_t rawtime = static_cast<time_t>(currentJournal.j_content.i_date);
                struct tm* timeinfo = localtime(&rawtime);
                char dateBuffer[20];
                strftime(dateBuffer, sizeof(dateBuffer), "%d/%m/%Y %H:%M", timeinfo);

                result << left << setw(12) << currentJournal.j_content.i_operation
                       << setw(30) << currentJournal.j_content.i_path
                       << setw(20) << currentJournal.j_content.i_content
                       << dateBuffer << "\n";
            }
        }

        file.close();

        if (!hasData) {
            return "El Journaling de esta particion esta limpio";
        }

        return result.str();
    }
}

#endif // !JOURNAL_H