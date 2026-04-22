#ifndef REP_H
#define REP_H

#include "mkfs.h"
#include "mount.h"
#include "session.h"
#include "mkdir.h"

#include "structures.h"
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <ios>
#include <iostream>
#include <libgen.h>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <vector>

using namespace std;

namespace CmdRep {
    inline string lowerCase(const string &str) {
        string result = str;
        transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }

    inline string cleanHTML(const string& str) {
        string res = "";
        for (char c : str) {
            if (c == '\0') break; 
            if (c == '<') res += "&lt;";
            else if (c == '>') res += "&gt;";
            else if (c == '&') res += "&amp;";
            else if (c == '"') res += "&quot;";
            else if (c == '\n') res += "<BR/>";
            else if (c >= 32 && c <= 126) res += c; 
        }
        if (res.empty()) return "-";
        return res;
    }

    inline bool createDirectory(const string &path) {
        char tmp[1024];
        char *p = nullptr;
        size_t len;
        snprintf(tmp, sizeof(tmp), "%s", path.c_str());
        len = strlen(tmp);
        if (tmp[len - 1] == '/') {
            tmp[len - 1] = 0;
        }

        for (p = tmp + 1; *p; p++) {
            if (*p == '/') {
                *p = 0;
                mkdir(tmp, S_IRWXU);
                *p = '/';
            }
        }

        mkdir(tmp, S_IRWXU);
        return true;
    }

    inline string getParentPath(const string &path) {
        char tmp[1024];
        snprintf(tmp, sizeof(tmp), "%s", path.c_str());
        return string(dirname(tmp));
    }

    inline string getExtension(const string &path) {
        size_t posision = path.find_last_of('.');
        if (posision != string::npos) {
            return lowerCase(path.substr(posision + 1));
        }
        return "jpg";
    }

    inline string mbrReport(const string &path, const string &diskPath) {
        ifstream file(diskPath, ios::binary);
        if (!file.is_open()) {
            return "Error: no es posible abrir el disco " + diskPath;
        }

    MBR mbr;
    file.seekg(0, ios::beg);
    file.read(reinterpret_cast<char *>(&mbr), sizeof(MBR));

    ostringstream dot;
    dot << "digraph REPMBR {\n";
    dot << "    node[shape=plaintext]\n";
    dot << "    rankdir=TB;   \n";

    dot << "    mbr [label=<<TABLE BORDER=\"1\" CELLBORDER=\"1\" "
            "CELLSPACING=\"0\" CELLPADDING=\"4\">  \n";

    dot << "     <TR><TD COLSPAN=\"2\" BGCOLOR=\"#000000\"><FONT "
            "COLOR=\"white\"><B>MBR</B></FONT></TD></TR>\n";
    dot << "     <TR><TD><B>mbr_tamano</B></TD><TD>" << mbr.mbr_size
        << "</TD></TR>\n";

    char dateStr[100];
    struct tm *timeInfo = localtime(&mbr.mbr_creation_date);
    strftime(dateStr, sizeof(dateStr), "%d/%m/%Y %H:%M:%S", timeInfo);

    dot << "     <TR><TD><B>mbr_fecha_creacion</B></TD><TD>" << dateStr
        << "</TD></TR>\n";
    dot << "     <TR><TD><B>mbr_dsk_signature</B></TD><TD>"
        << mbr.mbr_disk_signature << "</TD></TR>\n";
    dot << "     <TR><TD><B>dsk_fit</B></TD><TD>" << mbr.disk_fit
        << "</TD></TR>\n";

    int partNum = 1;
    for (int i = 0; i < 4; i++) {
        Partition &partition = mbr.mbr_partitions[i];

        if (partition.part_status == '1') {
        dot << "        <TR><TD COLSPAN=\"2\" BGCOLOR=\"#000000\"><FONT "
                "COLOR=\"white\"><B>Partition "
            << partNum << "</B></FONT></TD></TR>\n";
        dot << "        <TR><TD><B>status</B></TD><TD>" << partition.part_status
            << "</TD></TR>\n";
        dot << "        <TR><TD><B>type</B></TD><TD>" << partition.part_type
            << "</TD></TR>\n";
        dot << "        <TR><TD><B>fit</B></TD><TD>" << partition.part_fit
            << "</TD></TR>\n";
        dot << "        <TR><TD><B>start</B></TD><TD>" << partition.part_start
            << "</TD></TR>\n";
        dot << "        <TR><TD><B>size</B></TD><TD>" << partition.part_size
            << "</TD></TR>\n";
        dot << "        <TR><TD><B>name</B></TD><TD>" << partition.part_name
            << "</TD></TR>\n";
        partNum++;
        }
    }

    dot << "    </TABLE>>];\n\n";
    dot << "}\n";

    file.close();

    string parePath = getParentPath(path);
    createDirectory(parePath);
    string dotPath = path + ".dot";
    ofstream dotFile(dotPath);

    if (!dotFile.is_open()) {
        return "Error: no se pudo crear el archivo .dot";
    }

    dotFile << dot.str();
    dotFile.close();

    string ext = getExtension(path);
    string cmd = "dot -T " + ext + " \"" + dotPath + "\" -o \"" + path + "\"";
    int res = system(cmd.c_str());

    remove(dotPath.c_str());

    if (res != 0) {
        return "Error: no se pudo generar el reporte";
    }

    return "Reporte MBR generado exitosamente en: " + path;
    }

    inline string reportDisk(const string &path, const string &diskPath) {
        ifstream file(diskPath, ios::binary);

        if (!file.is_open()) {
            return "Error: no se pudo abrir el disco " + diskPath;
        }

        MBR mbr;
        file.seekg(0, ios::beg);
        file.read(reinterpret_cast<char *>(&mbr), sizeof(MBR));

        int sizeDisk = mbr.mbr_size;

        struct DiskStruct {
            string type;
            string name;
            int start;
            int size;
            double percent;
            bool isExt;
        };

        vector<DiskStruct> allDiskSections;

        DiskStruct SecMbr;

        SecMbr.type = "mbr";
        SecMbr.name = "MBR";
        SecMbr.start = 0;
        SecMbr.size = sizeof(MBR);
        SecMbr.percent = (sizeof(MBR) * 100.0) / sizeDisk;
        SecMbr.isExt = false;
        allDiskSections.push_back(SecMbr);

        for (int i = 0; i < 4; i++) {
            Partition &partition = mbr.mbr_partitions[i];
            if (partition.part_status == '1') {
            if (partition.part_type == 'E' || partition.part_type == 'e') {
                int ebr_s = partition.part_start;
                int ebr_e = partition.part_start + partition.part_size;
                int current_p = partition.part_start;

                while (ebr_s != -1 && ebr_s < ebr_e) {
                EBR ebr;
                file.seekg(ebr_s, ios::beg);
                file.read(reinterpret_cast<char *>(&ebr), sizeof(EBR));

                if (ebr_s > current_p) {
                    DiskStruct freeSection;
                    freeSection.type = "free";
                    freeSection.name = "libre";
                    freeSection.start = current_p;
                    freeSection.size = ebr_s - current_p;
                    freeSection.percent = (freeSection.size * 100.0) / sizeDisk;
                    freeSection.isExt = true;
                    allDiskSections.push_back(freeSection);
                }

                if (ebr.part_status == '1') {
                    DiskStruct ebrSection;
                    ebrSection.type = "ebr";
                    ebrSection.name = "EBR";
                    ebrSection.start = ebr_s;
                    ebrSection.size = sizeof(EBR);
                    ebrSection.percent = (sizeof(EBR) * 100.0) / sizeDisk;
                    ebrSection.isExt = true;
                    allDiskSections.push_back(ebrSection);

                    DiskStruct logSection;
                    logSection.type = "logical";
                    logSection.name = string(ebr.part_name);
                    logSection.start = ebr_s + sizeof(EBR);
                    logSection.size = ebr.part_size;
                    logSection.percent = (ebr.part_size * 100.0) / sizeDisk;
                    logSection.isExt = true;
                    allDiskSections.push_back(logSection);

                    current_p = ebr_s + sizeof(EBR) + ebr.part_size;
                }

                ebr_s = ebr.part_next;
                if (ebr_s <= 0) {
                    break;
                }
                }

                if (current_p < ebr_e) {
                DiskStruct freeSection;
                freeSection.type = "free";
                freeSection.name = "libre";
                freeSection.start = current_p;
                freeSection.size = ebr_s - current_p;
                freeSection.percent = (freeSection.size * 100.0) / sizeDisk;
                freeSection.isExt = true;
                allDiskSections.push_back(freeSection);
                }
            } else {
                DiskStruct primSec;
                primSec.type = "partition";
                primSec.name = string(partition.part_name);
                primSec.start = partition.part_start;
                primSec.size = partition.part_size;
                primSec.percent = (partition.part_size * 100.0) / sizeDisk;
                primSec.isExt = false;
                allDiskSections.push_back(primSec);
            }
            }
        }

        sort(allDiskSections.begin(), allDiskSections.end(),
            [](const DiskStruct &a, const DiskStruct &b) {
                return a.start < b.start;
            });

        vector<DiskStruct> finalSections;

        int currentPos = 0;

        for (const auto &section : allDiskSections) {
            if (!section.isExt && section.start > currentPos && section.type != "mbr") {
            DiskStruct freeSection;
            freeSection.type = "free";
            freeSection.name = "libre";
            freeSection.start = currentPos;
            freeSection.size = section.start - currentPos;
            freeSection.percent = (freeSection.size * 100.0) / sizeDisk;
            freeSection.isExt = false;
            finalSections.push_back(freeSection);
            }

            finalSections.push_back(section);

            if (!section.isExt) {
            currentPos = section.start + section.size;
            }
        }

        if (currentPos < sizeDisk) {
            DiskStruct freeSection;
            freeSection.type = "free";
            freeSection.name = "libre";
            freeSection.start = currentPos;
            freeSection.size = sizeDisk - currentPos;
            freeSection.percent = (freeSection.size * 100.0) / sizeDisk;
            freeSection.isExt = false;
            finalSections.push_back(freeSection);
        }

        ostringstream dot;
        dot << "digraph REPORTEDISK {\n";
        dot << "    node [shape=plaintext] \n";
        dot << "    rankdir=LR;\n\n";

        dot << "    disk [label=<<TABLE BORDER=\"1\" CELLBORDER=\"1\" "
                "CELLSPACING=\"0\" CELLPADDING=\"4\">\n";
        dot << "        <TR>\n";
        bool isExt = false;
        int extCols = 0;

        for (const auto &section : finalSections) {
            if (section.isExt && !isExt) {
                isExt = true;
                dot << "        <TD COLSPAN=\"" << extCols
                    << "\" BGCOLOR=\"#FFFFF\"><B>Extendida</B></TD>\n";

            } else if (!section.isExt && isExt) {
                isExt = false;
                dot << "        <TD></TD>\n";
            } else {
                dot << "        <TD></TD>\n";
            }
        }

        dot << "        </TR>\n";

        dot << "        <TR>\n";
       for (const auto &section : finalSections) {
            string color;
            if (section.type == "mbr") {
                color = "#CCCCCC";
                dot << "        <TD BGCOLOR=\"" << color << "\"><B>" << section.name << "</B></TD>\n";
            } else if(section.type == "free"){
                color = "#FFFFFF";
                dot << "        <TD BGCOLOR=\"" << color << "\"><B>" << section.name << "<BR/>" << fixed << setprecision(1) << section.percent << "%</B></TD>\n";
            } else if(section.type == "ebr"){
                color = "#B0BEC5";
                dot << "        <TD BGCOLOR=\"" << color << "\"><B>" << section.name << "</B></TD>\n";
            } else if(section.type == "logical"){
                color = "#E3F2FD";
                dot << "        <TD BGCOLOR=\"" << color << "\"><B>Logica<BR/>" << fixed << setprecision(1) << section.percent << "%</B></TD>\n";
            } else if(section.type == "partition"){
                color = "#C8E6C9";
                dot << "        <TD BGCOLOR=\"" << color << "\"><B>Primaria<BR/>" << fixed << setprecision(1) << section.percent << "%</B></TD>\n";
            }
        }

        dot << "        </TR>\n";
        dot << "    </TABLE>>];\n\n";
        dot << "}\n";

        file.close();
        string parePath = getParentPath(path);
        createDirectory(parePath);
        string dotPath = path + ".dot";

        ofstream dotFile(dotPath);

        if(!dotFile.is_open()){
            return "Error: no se pudo crear el archivo .dot";
        }

        dotFile << dot.str();
        dotFile.close();

        string ext = getExtension(path);
        string cmd = "dot -T" + ext + " \"" + dotPath + "\" -o \"" + path + "\"";
        int res = system(cmd.c_str());
        remove(dotPath.c_str());
        
        if(res != 0 ){
            return "Error: no se pudo generar el reporte";
        }

        return "Reporte DISK generado con exito en la carpeta: " + path;

    }

    inline string reportInode(const string &path, const string &id){

        // se busca la particion con el nombre de id
        CmdMount::MountedPartition partition;
        if(!CmdMount::getMountedPar(id,partition)){
            return "Error: la particion  no existe";
        }

        string expantedPath = CmdMount::expandPath(partition.path);
        fstream file(expantedPath, ios::in | ios::out | ios::binary);

        if(!file.is_open()){
            return "Error: no se pudo abrir el disco " + partition.path;  
        }

        Superblock sb;
        file.seekg(partition.start, ios::beg);
        file.read(reinterpret_cast<char*>(&sb), sizeof(Superblock));

        ostringstream dot;
        dot << "digraph G {\n"; 
        dot << "    rankdir=LR;\n";
        dot << "    node[shape=plaintext];\n";

        Inode rootInode;
        file.seekg(sb.s_inode_start, ios::beg);
        file.read(reinterpret_cast<char*>(&rootInode), sizeof(Inode));

        int prev_inode = -1;
     
        for(int i = 0; i < sb.s_inodes_count; i++){
            char bit;
            file.clear();
            file.seekg(sb.s_bm_inode_start + i , ios::beg);
            file.read(&bit, 1);

            if(bit == '1'){
                Inode inode;
                file.seekg(sb.s_inode_start + i * sizeof(Inode), ios::beg);
                file.read(reinterpret_cast<char*>(&inode), sizeof(Inode));

                char atime[20], ctime[20], mtime[20];
                strftime(atime, 20, "%Y-%m-%d %H:%M", localtime(&inode.i_atime));
                strftime(ctime, 20, "%Y-%m-%d %H:%M", localtime(&inode.i_ctime));
                strftime(mtime, 20, "%Y-%m-%d %H:%M", localtime(&inode.i_mtime));

                dot << "    inode_" + to_string(i) + " [label=<\n";
                dot << "      <TABLE BORDER=\"1\" CELLBORDER=\"1\" CELLSPACING=\"0\">\n";
                dot << "        <TR><TD COLSPAN=\"2\" BGCOLOR=\"#006699\"><FONT COLOR=\"white\"><B>Inodo " + to_string(i) + "</B></FONT></TD></TR>\n";
                dot << "        <TR><TD>i_uid</TD><TD>" + to_string(inode.i_uid) + "</TD></TR>\n";
                dot << "        <TR><TD>i_gid</TD><TD>" + to_string(inode.i_gid) + "</TD></TR>\n";
                dot << "        <TR><TD>i_size</TD><TD>" + to_string(inode.i_size) + "</TD></TR>\n";
                dot << "        <TR><TD>i_atime</TD><TD>" + string(atime) + "</TD></TR>\n";
                dot << "        <TR><TD>i_ctime</TD><TD>" + string(ctime) + "</TD></TR>\n";
                dot << "        <TR><TD>i_mtime</TD><TD>" + string(mtime) + "</TD></TR>\n";
                dot << "        <TR><TD>i_type</TD><TD>" + string(1, inode.i_type) + "</TD></TR>\n";
                dot << "        <TR><TD>i_perm</TD><TD>" + to_string(inode.i_perm) + "</TD></TR>\n";


                for(int b = 0; b < 15; b++){
                    dot << "        <TR><TD>i_block[" + to_string(b) + "]</TD><TD>" + to_string(inode.i_block[b]) + "</TD></TR>\n";
                }

                dot << "      </TABLE>\n";
                dot << "    >];\n\n";

                if(prev_inode != -1){
                    dot << "    inode_" + to_string(prev_inode) + " -> inode_" + to_string(i) + ";\n";
                }
                
                prev_inode = i;
            }
        }

        dot << "}\n";
        file.close();

        string parePath = getParentPath(path);
        createDirectory(parePath);
        string dotPath = path + ".dot";
        ofstream dotFile(dotPath);

        if (!dotFile.is_open()) {
            return "Error: no se pudo crear el archivo .dot";
        }

        dotFile << dot.str();
        dotFile.close();

        string ext = getExtension(path);
        string cmd = "dot -T " + ext + " \"" + dotPath + "\" -o \"" + path + "\"";
        int res = system(cmd.c_str());

        remove(dotPath.c_str());

        if (res != 0) {
            return "Error: no se pudo generar el reporte inodo";
        }

        return "Reporte INODE generado exitosamente en: " + path;
    }

    inline string reportBlock(const string &path, const string &id){
        // se busca la particion con el nombre de id
        CmdMount::MountedPartition partition;
        if(!CmdMount::getMountedPar(id,partition)){
            return "Error: la particion  no existe";
        }

        string expantedPath = CmdMount::expandPath(partition.path);
        fstream file(expantedPath, ios::in | ios::out | ios::binary);

        if(!file.is_open()){
            return "Error: no se pudo abrir el disco " + partition.path;  
        }

        Superblock sb;
        file.seekg(partition.start, ios::beg);
        file.read(reinterpret_cast<char*>(&sb), sizeof(Superblock));

        int* blockTypes = new int[sb.s_blocks_count];
        for(int i = 0; i < sb.s_blocks_count; i++){
            blockTypes[i] = -1;
        }

        for(int i = 0; i < sb.s_inodes_count; i++){
            char bit;
            file.clear();
            file.seekg(sb.s_bm_inode_start + i, ios::beg); 
            file.read(&bit, 1);
            if(bit == '1'){
                Inode inode;
                file.seekg(sb.s_inode_start + i * sizeof(Inode), ios::beg);
                file.read(reinterpret_cast<char*>(&inode), sizeof(Inode));

                for(int j = 0; j < 15; j++){
                    if(inode.i_block[j] != -1){
                        if(j < 12){
                            blockTypes[inode.i_block[j]] = (inode.i_type == '0') ? 0 : 1;
                        } else {
                            blockTypes[inode.i_block[j]] = 2;
                        }  
                    }  
                } 
            }
        }
        ostringstream dot;
        dot << "digraph ReporteBlock {\n";
        dot << "    rankdir=LR;\n";
        dot << "    node[shape=plaintext];\n";

        int prev_block = -1;
     
        for(int i = 0; i < sb.s_blocks_count; i++){
            char bit;
            file.clear();
            file.seekg(sb.s_bm_block_start + i , ios::beg);
            file.read(&bit, 1);

            if(bit == '1'){
                int type = (blockTypes[i] != -1) ? blockTypes[i] : 1;

                dot << "    block_" + to_string(i) + " [label=<\n";
                dot << "      <TABLE BORDER=\"1\" CELLBORDER=\"1\" CELLSPACING=\"0\">\n";

                if(type == 0){
                    FolderBlock fblock;
                    file.seekg(sb.s_block_start + i * 64, ios::beg);
                    file.read(reinterpret_cast<char*>(&fblock), sizeof(FolderBlock));

                    dot << "        <TR><TD COLSPAN=\"2\" BGCOLOR=\"#339933\"><FONT COLOR=\"white\"><B>Bloque Carpeta " + to_string(i) + "</B></FONT></TD></TR>\n";
                    dot << "        <TR><TD>b_name</TD><TD>b_inodo</TD></TR>\n";

                    for(int j = 0; j < 4; j++){
                        string name(fblock.b_content[j].b_name);
                        dot << "        <TR><TD>" + cleanHTML(name) + "</TD><TD>" + to_string(fblock.b_content[j].b_inodo) + "</TD></TR>\n";
                    }
                } else if(type == 1){
                    FileBlock fileBlock;
                    file.seekg(sb.s_block_start + i * 64, ios::beg);
                    file.read(reinterpret_cast<char*>(&fileBlock), sizeof(FileBlock));

                    string rawContent = "";
                    for(int j=0; j<64; j++) rawContent += fileBlock.b_content[j];
                    
                    dot << "        <TR><TD>" + cleanHTML(rawContent) + "</TD></TR>\n";

                } else if(type == 2){
                    int pointers[16];
                    file.seekg(sb.s_block_start + i * 64, ios::beg);
                    file.read(reinterpret_cast<char*>(&pointers), 64);

                    dot << "        <TR><TD BGCOLOR=\"#336699\"><FONT COLOR=\"white\"><B>Bloque Apuntador " + to_string(i) + "</B></FONT></TD></TR>\n";
                    string ptrStr = "";
                    for(int j = 0; j < 16; j++){
                        ptrStr += to_string(pointers[j]);
                        if(j != 15) ptrStr += ", ";
                        if(j == 7) ptrStr += "<BR/>"; 
                    }
                    dot << "        <TR><TD>" + ptrStr + "</TD></TR>\n";
                }

                dot << "      </TABLE>\n";
                dot << "    >];\n\n";

                if (prev_block != -1) {
                    dot << "    block_" + to_string(prev_block) + " -> block_" + to_string(i) + ";\n";
                }
                prev_block = i;
                
            }
        }

        dot << "}\n";
        file.close();

        delete[] blockTypes;

        string parePath = getParentPath(path);
        createDirectory(parePath);
        string dotPath = path + ".dot";
        ofstream dotFile(dotPath);

        if (!dotFile.is_open()) {
            return "Error: no se pudo crear el archivo .dot";
        }

        dotFile << dot.str();
        dotFile.close();

        string ext = getExtension(path);
        string cmd = "dot -T " + ext + " \"" + dotPath + "\" -o \"" + path + "\"";
        int res = system(cmd.c_str());

        remove(dotPath.c_str());

        if (res != 0) {
            return "Error: no se pudo generar el reporte Block";
        }

        return "Reporte Block generado exitosamente en: " + path;

    }

    inline string reportBmInode(const string &path, const string &id){
        CmdMount::MountedPartition partition;
        if(!CmdMount::getMountedPar(id, partition)){
            return "Error: la particion no existe";
        }

        string expantedPath = CmdMount::expandPath(partition.path);
        fstream file(expantedPath, ios::in | ios::binary);

        if(!file.is_open()){
            return "Error: no se pudo abrir el disco " + partition.path;  
        }

        Superblock sb;
        file.seekg(partition.start, ios::beg);
        file.read(reinterpret_cast<char*>(&sb), sizeof(Superblock));

        string parePath = getParentPath(path);
        createDirectory(parePath);

        ofstream repFile(path);
        if(!repFile.is_open()){
            return "Error: No se pudo crear el archivo de reporte en " + path;
        }

        file.seekg(sb.s_bm_inode_start, ios::beg);
        int jumpCount = 0;

        for(int i = 0; i < sb.s_inodes_count; i++){
            char bit;
            file.read(&bit, 1);

            repFile << bit << " ";
            jumpCount++;
            if(jumpCount == 20){
                repFile << "\n";
                jumpCount = 0;
            }
        }
        repFile.close();
        file.close();

        return "Reporte bm_inode creado exitosamente en: " + path;
    }
    inline string reportBmBlock(const string &path, const string &id){
        CmdMount::MountedPartition partition;
        if(!CmdMount::getMountedPar(id, partition)){
            return "Error: la particion no existe";
        }

        string expantedPath = CmdMount::expandPath(partition.path);
        fstream file(expantedPath, ios::in | ios::binary);

        if(!file.is_open()){
            return "Error: no se pudo abrir el disco " + partition.path;  
        }

        Superblock sb;
        file.seekg(partition.start, ios::beg);
        file.read(reinterpret_cast<char*>(&sb), sizeof(Superblock));

        string parePath = getParentPath(path);
        createDirectory(parePath);

        ofstream repFile(path);
        if(!repFile.is_open()){
            return "Error: No se pudo crear el archivo de reporte en " + path;
        }

        file.seekg(sb.s_bm_block_start, ios::beg);
        int jumpCount = 0;

        for(int i = 0; i < sb.s_blocks_count; i++){
            char bit;
            file.read(&bit, 1);

            repFile << bit << " ";
            jumpCount++;
            if(jumpCount == 20){
                repFile << "\n";
                jumpCount = 0;
            }
        }
        repFile.close();
        file.close();

        return "Reporte bm_block creado exitosamente en: " + path;
    }

    inline string reportTree(const string &path, const string &id){
        CmdMount::MountedPartition partition;
        if(!CmdMount::getMountedPar(id, partition)){
            return "Error: la particion no existe";
        }

        string expantedPath = CmdMount::expandPath(partition.path);
        fstream file(expantedPath, ios::in | ios::binary);

        if(!file.is_open()){
            return "Error: no se pudo abrir el disco " + partition.path;  
        }

        Superblock sb;
        file.seekg(partition.start, ios::beg);
        file.read(reinterpret_cast<char*>(&sb), sizeof(Superblock));

        
        Inode rootInode;
        file.seekg(sb.s_inode_start, ios::beg);
        file.read(reinterpret_cast<char*>(&rootInode), sizeof(Inode));

        if (rootInode.i_type != '0' && rootInode.i_type != '1') {
            string dot = "digraph G {\n";
            dot += "  node [shape=note, style=filled, fillcolor=\"#da3633\", fontcolor=\"white\", fontname=\"Helvetica-Bold\"];\n";
            dot += "  Loss [label=\"SISTEMA DE ARCHIVOS DAÑADO\\n(Simulación LOSS ejecutada)\"];\n";
            dot += "}\n";

            file.close();

            string parePath = getParentPath(path);
            createDirectory(parePath);
            string dotPath = path + ".dot";
            ofstream dotFile(dotPath);

            if (!dotFile.is_open()) return "Error: no se pudo crear el archivo .dot";
            
            dotFile << dot;
            dotFile.close();

            string ext = getExtension(path);
            string cmd = "dot -T " + ext + " \"" + dotPath + "\" -o \"" + path + "\"";
            int res = system(cmd.c_str());
            remove(dotPath.c_str());

            if (res != 0) return "Error: no se pudo generar el reporte Tree del disco dañado";
            
            return "Reporte Tree generado: El disco se encuentra vacio o dañado por LOSS.";
        }

        int* blockTypes = new int[sb.s_blocks_count];
        bool* blockVisited = new bool[sb.s_blocks_count];
        for(int k = 0; k < sb.s_blocks_count; k++){
            blockTypes[k] = -1;
            blockVisited[k] = false; 
        }

        bool* inodeVisited = new bool[sb.s_inodes_count];
        for(int k = 0; k < sb.s_inodes_count; k++){
            inodeVisited[k] = false; 
        }
        
        inodeVisited[0] = true; 

        string dotNodes = "";
        string dotEdges = "";

        for(int i = 0; i < sb.s_inodes_count; i++){
            char bit;
            file.clear();
            file.seekg(sb.s_bm_inode_start + i, ios::beg);
            file.read(&bit, 1);

            if(bit == '1'){
                Inode inode;
                file.seekg(sb.s_inode_start + i * sizeof(Inode), ios::beg);
                file.read(reinterpret_cast<char*>(&inode), sizeof(Inode));

                char atime[20], ctime[20], mtime[20];
                strftime(atime, 20, "%Y-%m-%d %H:%M", localtime(&inode.i_atime));
                strftime(ctime, 20, "%Y-%m-%d %H:%M", localtime(&inode.i_ctime));
                strftime(mtime, 20, "%Y-%m-%d %H:%M", localtime(&inode.i_mtime));

                dotNodes += "    inode_" + to_string(i) + " [label=<\n";
                dotNodes += "      <TABLE BORDER=\"1\" CELLBORDER=\"1\" CELLSPACING=\"0\">\n";
                dotNodes += "        <TR><TD COLSPAN=\"2\" BGCOLOR=\"#006699\"><FONT COLOR=\"white\"><B>Inodo " + to_string(i) + "</B></FONT></TD></TR>\n";
                dotNodes += "        <TR><TD>i_uid</TD><TD>" + to_string(inode.i_uid) + "</TD></TR>\n";
                dotNodes += "        <TR><TD>i_gid</TD><TD>" + to_string(inode.i_gid) + "</TD></TR>\n";
                dotNodes += "        <TR><TD>i_size</TD><TD>" + to_string(inode.i_size) + "</TD></TR>\n";
                dotNodes += "        <TR><TD>i_atime</TD><TD>" + string(atime) + "</TD></TR>\n";
                dotNodes += "        <TR><TD>i_ctime</TD><TD>" + string(ctime) + "</TD></TR>\n";
                dotNodes += "        <TR><TD>i_mtime</TD><TD>" + string(mtime) + "</TD></TR>\n";
                dotNodes += "        <TR><TD>i_type</TD><TD>" + string(1, inode.i_type) + "</TD></TR>\n";
                dotNodes += "        <TR><TD>i_perm</TD><TD>" + to_string(inode.i_perm) + "</TD></TR>\n";

                for(int b = 0; b < 15; b++){
                    dotNodes += "       <TR><TD>i_block[" + to_string(b) + "]</TD><TD PORT=\"b" + to_string(b) + "\">" + to_string(inode.i_block[b]) + "</TD></TR>\n"; 

                    int targetBlock = inode.i_block[b];
                    
                    if(targetBlock != -1){
                        if (targetBlock >= 0 && targetBlock < sb.s_blocks_count) {
                            if(b < 12){
                                blockTypes[targetBlock] = (inode.i_type == '0') ? 0 : 1;
                            } else {
                                blockTypes[targetBlock] = 2;
                            }

                            if (!blockVisited[targetBlock]) {
                                dotEdges += "   inode_" + to_string(i) + ":b" + to_string(b) + " -> block_" + to_string(targetBlock) + ";\n"; 
                                blockVisited[targetBlock] = true;
                            }
                        }
                    }
                }

                dotNodes += "   </TABLE>\n  >]; \n\n";
            }
        }

       
        for(int i = 0; i < sb.s_blocks_count; i++){
            char bit;
            file.clear();
            file.seekg(sb.s_bm_block_start + i , ios::beg);
            file.read(&bit, 1);

            if(bit == '1'){
                int type = (blockTypes[i] != -1) ? blockTypes[i] : 1;

                dotNodes += "    block_" + to_string(i) + " [label=<\n";
                dotNodes += "      <TABLE BORDER=\"1\" CELLBORDER=\"1\" CELLSPACING=\"0\">\n";

                if(type == 0){
                    FolderBlock fblock;
                    file.seekg(sb.s_block_start + i * 64, ios::beg);
                    file.read(reinterpret_cast<char*>(&fblock), sizeof(FolderBlock));

                    dotNodes += "        <TR><TD COLSPAN=\"2\" BGCOLOR=\"#339933\"><FONT COLOR=\"white\"><B>Bloque Carpeta " + to_string(i) + "</B></FONT></TD></TR>\n";
                    dotNodes += "        <TR><TD>b_name</TD><TD>b_inodo</TD></TR>\n";

                    for(int j = 0; j < 4; j++){
                        int targetInode = fblock.b_content[j].b_inodo;

                        if (targetInode < 0 || targetInode >= sb.s_inodes_count) {
                            targetInode = -1;
                        }

                        string name = "";
                        if (targetInode != -1) {
                            for(int c = 0; c < 12; c++){
                                char ch = fblock.b_content[j].b_name[c];
                                if(ch == '\0') break;
                                if(ch >= 32 && ch <= 126) name += ch;
                            }
                            
                            if (j == 0 && name.length() < 3) name = ".";
                            else if (j == 1 && name.length() < 3) name = "..";
                            else if (name == "") name = "???"; 
                            
                        } else {
                            name = "--";
                        }

                        dotNodes += "        <TR><TD>" + cleanHTML(name) + "</TD><TD PORT=\"p" + to_string(j) + "\">" + to_string(targetInode) + "</TD></TR>\n";

                        if(targetInode != -1){
                        
                            if (name != "." && name != "..") {
                                if (!inodeVisited[targetInode]) {
                                    dotEdges += "    block_" + to_string(i) + ":p" + to_string(j) + " -> inode_" + to_string(targetInode) + ";\n";
                                    inodeVisited[targetInode] = true;
                                }
                            }
                        }
                    }
                } else if(type == 1){
                    FileBlock fileBlock;
                    file.seekg(sb.s_block_start + i * 64, ios::beg);
                    file.read(reinterpret_cast<char*>(&fileBlock), sizeof(FileBlock));

                    dotNodes += "        <TR><TD BGCOLOR=\"#999933\"><FONT COLOR=\"white\"><B>Bloque Archivo " + to_string(i) + "</B></FONT></TD></TR>\n";

                    string rawContent = "";
                    for(int j=0; j<64; j++) {
                        char ch = fileBlock.b_content[j];
                        if(ch >= 32 && ch <= 126) rawContent += ch;
                    }
                    if (rawContent == "") rawContent = "--";
                    
                    dotNodes += "        <TR><TD>" + cleanHTML(rawContent) + "</TD></TR>\n";
                } else if(type == 2){
                    int pointers[16];
                    file.seekg(sb.s_block_start + i * 64, ios::beg);
                    file.read(reinterpret_cast<char*>(&pointers), 64);

                    dotNodes += "        <TR><TD BGCOLOR=\"#336699\"><FONT COLOR=\"white\"><B>Bloque Apuntador " + to_string(i) + "</B></FONT></TD></TR>\n";
                    for(int j = 0; j < 16; j++){
                        int targetBlock = pointers[j];
                        
                        if (targetBlock < 0 || targetBlock >= sb.s_blocks_count) {
                            targetBlock = -1;
                        }

                        dotNodes += "       <TR><TD PORT=\"p" + to_string(j) + "\">" + to_string(targetBlock) + "</TD></TR>\n";
                        
                        if(targetBlock != -1){
                            if (!blockVisited[targetBlock]) {
                                dotEdges += "       block_" + to_string(i) + ":p" + to_string(j) + " -> block_" + to_string(targetBlock) + ";\n";
                                blockVisited[targetBlock] = true;
                            }
                        }
                    }
                }

                dotNodes += "      </TABLE>\n >];\n\n";
            }
        }

        delete[] blockTypes;
        delete[] blockVisited;
        delete[] inodeVisited;

        string dot = "digraph G {\n";
        dot += "    rankdir=LR;\n";
        dot += "    node [shape=plaintext];\n";
        dot += dotNodes; 
        dot += dotEdges;
        dot += "}\n";

        file.close();

        string parePath = getParentPath(path);
        createDirectory(parePath);
        string dotPath = path + ".dot";
        ofstream dotFile(dotPath);

        if (!dotFile.is_open()) {
            return "Error: no se pudo crear el archivo .dot";
        }

        dotFile << dot;
        dotFile.close();

        string ext = getExtension(path);
        string cmd = "dot -T " + ext + " \"" + dotPath + "\" -o \"" + path + "\"";
        int res = system(cmd.c_str());

        remove(dotPath.c_str());

        if (res != 0) {
            return "Error: no se pudo generar el reporte Tree";
        }

        return "Reporte Tree generado exitosamente en: " + path;
    }

    inline string reportSb(const string &path, const string &id){
        CmdMount::MountedPartition partition;
        if(!CmdMount::getMountedPar(id, partition)){
            return "Error: la particion no existe";
        }

        string expantedPath = CmdMount::expandPath(partition.path);
        fstream file(expantedPath, ios::in | ios::binary);

        if(!file.is_open()){
            return "Error: no se pudo abrir el disco " + partition.path;  
        }

        Superblock sb;
        file.seekg(partition.start, ios::beg);
        file.read(reinterpret_cast<char*>(&sb), sizeof(Superblock));

        char mtime[20], umtime[20];
        strftime(mtime, 20, "%Y-%m-%d %H:%M", localtime(&sb.s_mtime));
        strftime(umtime, 20, "%Y-%m-%d %H:%M", localtime(&sb.s_umtime));

        ostringstream dot;
        dot << "digraph G {\n";
        dot << "    node[shape=plaintext]\n";
        dot << "    rankdir=TB;\n";

        dot << "    sb [label=<<TABLE BORDER=\"1\" CELLBORDER=\"1\" CELLSPACING=\"0\" CELLPADDING=\"4\">\n";

        dot << "     <TR><TD COLSPAN=\"2\" BGCOLOR=\"#000000\"><FONT COLOR=\"white\"><B>Reporte de SUPERBLOQUE</B></FONT></TD></TR>\n";
        
        dot << "     <TR><TD><B>s_filesystem_type</B></TD><TD>" << sb.s_filesystem_type << "</TD></TR>\n";
        dot << "     <TR><TD><B>s_inodes_count</B></TD><TD>" << sb.s_inodes_count << "</TD></TR>\n";
        dot << "     <TR><TD><B>s_blocks_count</B></TD><TD>" << sb.s_blocks_count << "</TD></TR>\n";
        dot << "     <TR><TD><B>s_free_blocks_count</B></TD><TD>" << sb.s_free_blocks_count << "</TD></TR>\n";
        dot << "     <TR><TD><B>s_free_inodes_count</B></TD><TD>" << sb.s_free_inodes_count << "</TD></TR>\n";
        dot << "     <TR><TD><B>s_mtime</B></TD><TD>" << mtime << "</TD></TR>\n";
        dot << "     <TR><TD><B>s_umtime</B></TD><TD>" << umtime << "</TD></TR>\n";
        dot << "     <TR><TD><B>s_mnt_count</B></TD><TD>" << sb.s_mnt_count << "</TD></TR>\n";        
        dot << "     <TR><TD><B>s_magic</B></TD><TD>0x" << std::hex << sb.s_magic << std::dec << "</TD></TR>\n";
        dot << "     <TR><TD><B>s_inode_size</B></TD><TD>" << sb.s_inode_size << "</TD></TR>\n";
        dot << "     <TR><TD><B>s_block_size</B></TD><TD>" << sb.s_block_size << "</TD></TR>\n";
        dot << "     <TR><TD><B>s_first_ino</B></TD><TD>" << sb.s_first_ino << "</TD></TR>\n";
        dot << "     <TR><TD><B>s_first_blo</B></TD><TD>" << sb.s_first_blo << "</TD></TR>\n";
        dot << "     <TR><TD><B>s_bm_inode_start</B></TD><TD>" << sb.s_bm_inode_start << "</TD></TR>\n";
        dot << "     <TR><TD><B>s_bm_block_start</B></TD><TD>" << sb.s_bm_block_start << "</TD></TR>\n";
        dot << "     <TR><TD><B>s_inode_start</B></TD><TD>" << sb.s_inode_start << "</TD></TR>\n";
        dot << "     <TR><TD><B>s_block_start</B></TD><TD>" << sb.s_block_start << "</TD></TR>\n";

        dot << "    </TABLE>>];\n";
        dot << "}\n";
        file.close();

        string parePath = getParentPath(path);
        createDirectory(parePath);
        string dotPath = path + ".dot";
        ofstream dotFile(dotPath);

        if (!dotFile.is_open()) {
            return "Error: no se pudo crear el archivo .dot";
        }

        dotFile << dot.str();
        dotFile.close();

        string ext = getExtension(path);
        string cmd = "dot -T " + ext + " \"" + dotPath + "\" -o \"" + path + "\"";
        int res = system(cmd.c_str());

        remove(dotPath.c_str());

        if (res != 0) {
            return "Error: no se pudo generar el reporte SB";
        }

        return "Reporte SB generado exitosamente en: " + path;
    }

    inline string reportFile(const string &path, const string &id, const string &filePath){
        // se busca la particion con el nombre de id
        CmdMount::MountedPartition partition;
        if(!CmdMount::getMountedPar(id,partition)){
            return "Error: la particion  no existe";
        }

        string expantedPath = CmdMount::expandPath(partition.path);
        fstream file(expantedPath, ios::in | ios::out | ios::binary);

        if(!file.is_open()){
            return "Error: no se pudo abrir el disco " + partition.path;  
        }


        Superblock sb;
        file.seekg(partition.start, ios::beg);
        file.read(reinterpret_cast<char*>(&sb), sizeof(Superblock));

        // Separar la ruta por diagonales /
        char pathCopy[256];
        strncpy(pathCopy, filePath.c_str(), 255);
        pathCopy[255] = '\0';

        char* context;
        char* token = strtok_r(pathCopy, "/", &context);

        int currentInodeN = 0;
        Inode currentInode; 

        while(token != nullptr){
            file.clear();
            file.seekg(sb.s_inode_start + currentInodeN * sizeof(Inode), ios::beg);
            file.read(reinterpret_cast<char*>(&currentInode), sizeof(Inode));

            char* peek = strtok_r(nullptr, "/", &context);

            if(peek == nullptr){
                int fileInodeNum = CmdMkdir::findInParent(file, sb.s_block_start, currentInode, token);

                if(fileInodeNum == -1){
                    file.close();
                    return "Error: el archivo " + string(token) + " no existe";

                }
                currentInodeN = fileInodeNum;
                break;
            } else {
                int nxtInodeNum = CmdMkdir::findInParent(file, sb.s_block_start, currentInode, token);
                if(nxtInodeNum == -1){
                    file.close();
                    return "Error: La ruta intermedia '" + string(token) + "' no existe.";
                }
                currentInodeN = nxtInodeNum;
                token = peek;
            }
        }

        Inode targetFielInode;
        file.clear();
        file.seekg(sb.s_inode_start + currentInodeN * sizeof(Inode), ios::beg);
        file.read(reinterpret_cast<char*>(&targetFielInode), sizeof(Inode));

        if(targetFielInode.i_type != '1'){
            file.close();
            return "Error: La ruta indicada apunta a una carpeta, no a un archivo de texto.";
        }

        

        string fileContent = "";
        int bytesRead = 0;

        for(int i = 0; i < 12; i++){
            if(targetFielInode.i_block[i] != -1){
                FileBlock fblock;
                file.clear();
                file.seekg(sb.s_block_start + targetFielInode.i_block[i] * 64, ios::beg);
                file.read(reinterpret_cast<char*>(&fblock), sizeof(FileBlock));

                for(int j = 0; j < 64 && bytesRead < targetFielInode.i_size; j++){
                    fileContent += fblock.b_content[j];
                    bytesRead++;
                }
            }
        }

        file.close();

        string parePath = getParentPath(path);
        createDirectory(parePath);
        ofstream txtFile(path);
        if(!txtFile.is_open()){
            return "Error: no se pudo crear el archivo de reporte en " + path;
        }

        txtFile << fileContent;
        txtFile.close();

        return "Reporte file generado exitosamente en: " + path;
    }

    inline string getPerm(char type, int perm){
        string res = (type == '0') ? "d" : "-"; 
        int owner = perm / 100;
        int group = (perm / 10) % 10;
        int other = perm % 10;

        for (int i = 0; i < 3; i++) {
            int currentPerm = 0;
            if (i == 0) currentPerm = owner;
            else if (i == 1) currentPerm = group;
            else currentPerm = other;

            switch(currentPerm) {
                case 0: res += "---"; break;
                case 1: res += "--x"; break;
                case 2: res += "-w-"; break;
                case 3: res += "-wx"; break;
                case 4: res += "r--"; break;
                case 5: res += "r-x"; break;
                case 6: res += "rw-"; break;
                case 7: res += "rwx"; break;
                default: res += "---"; break;
            }
        }
        return res;
    }

    inline string reportLs(const string &path, const string &id, const string &filePath){
        // se busca la particion con el nombre de id
        CmdMount::MountedPartition partition;
        if(!CmdMount::getMountedPar(id,partition)){
            return "Error: la particion  no existe";
        }

        string expantedPath = CmdMount::expandPath(partition.path);
        fstream file(expantedPath, ios::in | ios::out | ios::binary);

        if(!file.is_open()){
            return "Error: no se pudo abrir el disco " + partition.path;  
        }


        Superblock sb;
        file.seekg(partition.start, ios::beg);
        file.read(reinterpret_cast<char*>(&sb), sizeof(Superblock));

        Inode rootInode;
        file.seekg(sb.s_inode_start, ios::beg);
        file.read(reinterpret_cast<char*>(&rootInode), sizeof(Inode));

        int usersInodeN = CmdMkdir::findInParent(file, sb.s_block_start, rootInode, "users.txt");
        string usersContent = "";

        if(usersInodeN != -1){
            Inode usersInode;
            file.seekg(sb.s_inode_start + usersInodeN * sizeof(Inode), ios::beg);
            file.read(reinterpret_cast<char*>(&usersInode), sizeof(Inode));

            int bytesRead = 0;
            for(int i = 0; i < 12; i++){
                if(usersInode.i_block[i] != -1){
                    FileBlock fblock;
                    file.seekg(sb.s_block_start + usersInode.i_block[i] * 64, ios::beg);
                    file.read(reinterpret_cast<char*>(&fblock), sizeof(FileBlock));
                    for(int j = 0; j < 64 && bytesRead < usersInode.i_size; j++ ){
                        usersContent += fblock.b_content[j];
                        bytesRead++;
                    }
                }
            }
        }

        int sysUids[50]; string sysUsernames[50]; int userCount = 0;
        int sysGids[50]; string sysGroupnames[50]; int groupCount = 0;

        char contentCopy[2048];
        strncpy(contentCopy, usersContent.c_str(), sizeof(contentCopy));
        char* lineContext;
        char* line = strtok_r(contentCopy, "\n", &lineContext);

        while(line != nullptr){
            char lineCopy[256];
            strncpy(lineCopy, line, sizeof(lineCopy));
            char* dataContext;
            char* token = strtok_r(lineCopy, ",", &dataContext);
            string data[5]; 
            int numData = 0;
            
            while(token != nullptr && numData < 5){
                data[numData] = string(token);
                size_t start = data[numData].find_first_not_of(" \r");
                size_t end = data[numData].find_last_not_of(" \r");
                if(start != string::npos) data[numData] = data[numData].substr(start, end - start + 1);
                
                numData++;
                token = strtok_r(nullptr, ",", &dataContext);
            }

            if(numData >= 3 && data[1] == "G" && data[0] != "0"){
                sysGids[groupCount] = stoi(data[0]);
                sysGroupnames[groupCount] = data[2];
                groupCount++;
            } else if (numData >= 5 && data[1] == "U" && data[0] != "0"){
                sysUids[userCount] = stoi(data[0]);
                sysUsernames[userCount] = data[3];
                userCount++;
            }
            line = strtok_r(nullptr, "\n", &lineContext);
        }

        int currentInodeN = 0; 
        Inode targetInode = rootInode;

        if (filePath != "/") {
            char pathCopy2[256];
            strncpy(pathCopy2, filePath.c_str(), 255);
            char* context2;
            char* token2 = strtok_r(pathCopy2, "/", &context2);

            while(token2 != nullptr){
                int nxtInodeNum = CmdMkdir::findInParent(file, sb.s_block_start, targetInode, token2);
                if(nxtInodeNum == -1){
                    file.close();
                    return "Error: La ruta " + filePath + " no existe.";
                }
                currentInodeN = nxtInodeNum;
                file.seekg(sb.s_inode_start + currentInodeN * sizeof(Inode), ios::beg);
                file.read(reinterpret_cast<char*>(&targetInode), sizeof(Inode));
                token2 = strtok_r(nullptr, "/", &context2);
            }
        }

        if(targetInode.i_type != '0'){
            file.close();
            return "Error: la ruta debe apuntar a una carpeta para hacer ls.";
        }

        string dot = "digraph G {\n";
        dot += "    node [shape=plaintext];\n";
        dot += "    ls [label=<\n";
        dot += "      <TABLE BORDER=\"1\" CELLBORDER=\"1\" CELLSPACING=\"0\" CELLPADDING=\"4\">\n";
        dot += "        <TR BGCOLOR=\"#DDDDDD\">\n";
        dot += "          <TD><B>Permisos</B></TD><TD><B>Owner</B></TD><TD><B>Grupo</B></TD>\n";
        dot += "          <TD><B>Size (en Bytes)</B></TD><TD><B>Fecha</B></TD><TD><B>Hora</B></TD>\n";
        dot += "          <TD><B>Tipo</B></TD><TD><B>Name</B></TD>\n";
        dot += "        </TR>\n";

        for(int i = 0; i < 12; i++){
            if(targetInode.i_block[i] != -1){
                FolderBlock fblock;
                file.seekg(sb.s_block_start + targetInode.i_block[i] * 64, ios::beg);
                file.read(reinterpret_cast<char*>(&fblock), sizeof(FolderBlock));

                for(int j = 0; j < 4; j++){
                    if(fblock.b_content[j].b_inodo != -1){
                        string itemName = fblock.b_content[j].b_name;
                        
                        Inode childInode;
                        file.seekg(sb.s_inode_start + fblock.b_content[j].b_inodo * sizeof(Inode), ios::beg);
                        file.read(reinterpret_cast<char*>(&childInode), sizeof(Inode));

                        string permStr = getPerm(childInode.i_type, childInode.i_perm);
                        
                        string ownerStr = to_string(childInode.i_uid); 
                        if (childInode.i_uid == 1) {
                            ownerStr = "root";
                        } else {
                            for(int k = 0; k < userCount; k++){
                                if(sysUids[k] == childInode.i_uid){
                                    ownerStr = sysUsernames[k];
                                    break; 
                                }
                            }
                        }

                        string groupStr = to_string(childInode.i_gid); 
                        if (childInode.i_gid == 1) {
                            groupStr = "root";
                        } else {
                            for(int k = 0; k < groupCount; k++){
                                if(sysGids[k] == childInode.i_gid){
                                    groupStr = sysGroupnames[k];
                                    break; 
                                }
                            }
                        }
                        
                        char dateStr[20], timeStr[20];
                        strftime(dateStr, 20, "%d/%m/%Y", localtime(&childInode.i_mtime));
                        strftime(timeStr, 20, "%H:%M", localtime(&childInode.i_mtime));
                        
                   
                        string tipoStr = (childInode.i_type == '0') ? "Carpeta" : "Archivo";

                        dot += "        <TR>\n";
                        dot += "          <TD>" + permStr + "</TD><TD>" + ownerStr + "</TD><TD>" + groupStr + "</TD>\n";
                        dot += "          <TD>" + to_string(childInode.i_size) + "</TD><TD>" + string(dateStr) + "</TD><TD>" + string(timeStr) + "</TD>\n";

                        dot += "          <TD>" + tipoStr + "</TD><TD>" + cleanHTML(itemName) + "</TD>\n";
                        dot += "        </TR>\n";
                    }
                }
            }
        }

        dot += "      </TABLE>\n    >];\n}\n";
        file.close();

        string parePath = getParentPath(path);
        createDirectory(parePath);
        string dotPath = path + ".dot";
        ofstream dotFile(dotPath);

        if (!dotFile.is_open()) {
            return "Error: no se pudo crear el archivo .dot";
        }

        dotFile << dot;
        dotFile.close();

        string ext = getExtension(path);
        string cmd = "dot -T " + ext + " \"" + dotPath + "\" -o \"" + path + "\"";
        int res = system(cmd.c_str());

        remove(dotPath.c_str());

        if (res != 0) {
            return "Error: no se pudo generar el reporte";
        }

        return "Reporte LS generado exitosamente en: " + path;
            
    }

    inline std::string execute(const std::string& name, const std::string& path, const std::string& id, const std::string& pathFileLs) {
        if (name.empty()) {
            return "Error: rep requiere el parametro -name";
        }
        if (path.empty()) {
            return "Error: rep requiere el parametro -path";
        }
        if (id.empty()) {
            return "Error: rep requiere el parametro -id";
        }
        
        std::string reportType = CmdMkfs::toLowerString(name);
        
        if (reportType != "mbr" && reportType != "disk" && reportType != "inode" && 
            reportType != "block" && reportType != "bm_inode" && reportType != "bm_block" && 
            reportType != "tree" && reportType != "sb" && reportType != "file" && reportType != "ls") {
            return "Error: tipo de reporte no valido. Valores permitidos: mbr, disk, inode, block, bm_inode, bm_block, tree, sb, file, ls";
        }
        
        CmdMount::MountedPartition partition;
        if (!CmdMount::getMountedPar(id, partition)) {
            return "Error: la particion con ID '" + id + "' no esta montada";
        }
        
        std::ostringstream result;
        result << "\n--------- REP ---------- \n";
        result << "Generando reporte " << reportType << "...\n";
        
        if (reportType == "mbr") {
            result << mbrReport(path, partition.path);
        } 
        else if (reportType == "disk") {
            result << reportDisk(path, partition.path);
        } 
        else if (reportType == "inode") {
            result << reportInode(path, id);
        } 
        else if (reportType == "block") {
            result << reportBlock(path, id);
        } 
        else if (reportType == "bm_inode") {
            result << reportBmInode(path, id);
        } 
        else if (reportType == "bm_block") {
            result << reportBmBlock(path, id);
        } 
        else if (reportType == "tree") {
            result << reportTree(path, id);
        } 
        else if (reportType == "sb") {
            result << reportSb(path, id);
        } 
        else if (reportType == "file") {
            if (pathFileLs.empty()) {
                return "Error: el reporte file requiere el parametro -path_file_ls.";
            }
            result << reportFile(path, id, pathFileLs);
        } 
        else if (reportType == "ls") {
            if (pathFileLs.empty()) {
                return "Error: el reporte ls requiere el parametro -path_file_ls.";
            }
            result << reportLs(path, id, pathFileLs);
        } 
        else {
            return "Error: el tipo de reporte " + reportType + " no es valido.";
        }
        
        return result.str();
    }

} // namespace CmdRep
#endif // !REP_H