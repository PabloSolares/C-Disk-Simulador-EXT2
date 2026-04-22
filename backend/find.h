#ifndef FIND_H
#define FIND_H

#include "mount.h"
#include "structures.h"
#include "session.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <vector>

using namespace std;

namespace CmdFind {
    
    struct FindResult {
        bool hasMatch;
        string output;
    };

    inline bool matchWildcard(const char* str, const char* pattern) {
        if (*pattern == '\0') return *str == '\0';
        if (*pattern == '*') {
            if (*str == '\0') return matchWildcard(str, pattern + 1);
            return matchWildcard(str + 1, pattern) || matchWildcard(str, pattern + 1);
        }
        if (*pattern == '?' && *str != '\0') return matchWildcard(str + 1, pattern + 1);
        if (*str == *pattern) return matchWildcard(str + 1, pattern + 1);
        return false;
    }

    inline FindResult traverse(fstream& file, Superblock& sb, int inodeNum, const string& name, const string& pattern, int depth) {
        FindResult res = {false, ""};
        Inode currentInode;
        file.seekg(sb.s_inode_start + inodeNum * sizeof(Inode), ios::beg);
        file.read(reinterpret_cast<char*>(&currentInode), sizeof(Inode));

        bool matches = matchWildcard(name.c_str(), pattern.c_str());
        
        string indent = "";
        for(int i = 0; i < depth; i++) indent += "   ";

        string currentLine = "" + indent;
        if (depth > 0) currentLine += "|_ ";
        currentLine += name;

        if (currentInode.i_type == '1') { 
            if (matches) {
                res.hasMatch = true;
                res.output = currentLine + "\n";
            }
            return res;
        }

        string childrenOutput = "";
        bool childMatched = false;

        for (int i = 0; i < 12; i++) {
            if (currentInode.i_block[i] != -1) {
                FolderBlock fblock;
                file.seekg(sb.s_block_start + currentInode.i_block[i] * 64, ios::beg);
                file.read(reinterpret_cast<char*>(&fblock), sizeof(FolderBlock));

                for (int j = 0; j < 4; j++) {
                    if (fblock.b_content[j].b_inodo != -1) {
                        string childName = fblock.b_content[j].b_name;
                        if (childName != "." && childName != "..") {
                            FindResult childRes = traverse(file, sb, fblock.b_content[j].b_inodo, childName, pattern, depth + 1);
                            if (childRes.hasMatch) {
                                childMatched = true;
                                childrenOutput += childRes.output;
                            }
                        }
                    }
                }
            }
        }

        if (matches || childMatched) {
            res.hasMatch = true;
            res.output = currentLine + "\n" + childrenOutput;
        }

        return res;
    }

    inline string executeCmd(const string& path, const string& name) {
        if (!currentUser.active) return "Error: no hay una sesion activa";

        CmdMount::MountedPartition partition;
        if (!CmdMount::getMountedPar(currentUser.partitionId, partition)) {
            return "Error: la particion no existe";
        }

        string expantedPath = CmdMount::expandPath(partition.path);
        fstream file(expantedPath, ios::in | ios::out | ios::binary);

        if (!file.is_open()) return "Error: no se pudo abrir el disco " + partition.path;

        Superblock sb;
        file.seekg(partition.start, ios::beg);
        file.read(reinterpret_cast<char*>(&sb), sizeof(Superblock));

        char pathCopy[256];
        strncpy(pathCopy, path.c_str(), 255);
        pathCopy[255] = '\0';

        char* context;
        char* token = strtok_r(pathCopy, "/", &context);
        
        int currentInodeN = 0; 
        string targetName = "/";

        if (path != "/") {
            while (token != nullptr) {
                Inode currentInode;
                file.seekg(sb.s_inode_start + currentInodeN * sizeof(Inode), ios::beg);
                file.read(reinterpret_cast<char*>(&currentInode), sizeof(Inode));

                bool found = false;
                for (int i = 0; i < 12; i++) {
                    if (currentInode.i_block[i] != -1) {
                        FolderBlock fblock;
                        file.seekg(sb.s_block_start + currentInode.i_block[i] * 64, ios::beg);
                        file.read(reinterpret_cast<char*>(&fblock), sizeof(FolderBlock));

                        for (int j = 0; j < 4; j++) {
                            if (fblock.b_content[j].b_inodo != -1 && strcmp(fblock.b_content[j].b_name, token) == 0) {
                                currentInodeN = fblock.b_content[j].b_inodo;
                                targetName = token;
                                found = true;
                                break;
                            }
                        }
                    }
                    if (found) break;
                }

                if (!found) {
                    file.close();
                    return "Error: La ruta '" + path + "' no existe en el sistema de archivos.";
                }
                token = strtok_r(nullptr, "/", &context);
            }
        }

        FindResult result = traverse(file, sb, currentInodeN, targetName, name, 0);
        file.close();

        if (!result.hasMatch) {
            return "No se encontraron resultados para '" + name + "' en la ruta '" + path + "'";
        }

        return "find -path=\"" + path + "\" -name=\"" + name + "\"\n# Arbol de Búsqueda:\n" + result.output;
    }
}
#endif // !FIND_H