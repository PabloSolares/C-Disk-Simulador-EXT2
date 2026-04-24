#ifndef MKDISK_H
#define MKDISK_H
#include <string>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cstdio>
#include <stdio.h>
#include <filesystem>
#include <cstring>
#include <fstream>
#include "mount.h"
#include "structures.h"

using namespace std;


namespace CmdMkDisk {
	
	
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

	inline bool createDirectories(const string& path){
		try {
			filesystem::path filePath(path);
			filesystem::path parentPath = filePath.parent_path();

			if(!parentPath.empty() && !filesystem::exists(parentPath)){
				filesystem::create_directories(parentPath);
				cout << "Carpetas creadas: " << parentPath <<  endl;
					
			}
			return true;
		}
		catch (const std::exception& e) {
			cerr << "Error creando carpetas: " << e.what() << endl;
			return false;
		}
	}
	

	inline string mkdisk(int size, const string& unit , const string& path){
		try{
			string expandedPath = expandPath(path);
			if (size <= 0) {
				return "Error: El tamaño debe ser mayor a 0";
			}

			if (expandedPath.length() < 4 || expandedPath.substr(expandedPath.length() - 4) != ".mia") {
                return "Error: El archivo del disco debe tener la extension obligatoria .mia";
            }
			
			int SizeB = size;
			
			if(unit == "k" || unit == "K"){
				SizeB = size * 1024;
			} else if(unit == "m" || unit == "M"){
				SizeB = size * 1024* 1024;
			} else  {
				return "Error: el tipo de dato no es permitido";
				
			}

			if(!createDirectories(expandedPath)){ 
				return "Error: no se pudieron crear los directorios";
			}
			
			ifstream checkFile(expandedPath);
			
			if(checkFile.good()){
				checkFile.close();
				return "Error: en el disco ya existe la ruta ";
			}
			
			ofstream diskFile(expandedPath, ios::binary);
			if(!diskFile.is_open()){ 
				return "Error: no se pudo crear el archivo del disco ";
			}
			
			char zero = '\0';
			for(int i = 0; i < SizeB; i++) { 
				diskFile.write(&zero, 1);
			}
			
			MBR mbr = {};
			
			mbr.mbr_size = SizeB;
			mbr.mbr_creation_date = time(nullptr);
			mbr.mbr_disk_signature = rand();
			mbr.disk_fit = 'F';
			
			for(int i = 0; i < 4 ; i++){ 
				mbr.mbr_partitions[i].part_status = '0';
				mbr.mbr_partitions[i].part_type = '\0';
				mbr.mbr_partitions[i].part_fit = '\0';
				mbr.mbr_partitions[i].part_start = -1;
				mbr.mbr_partitions[i].part_size = 0;
				memset(mbr.mbr_partitions[i].part_name, 0 , 16);
				
			}
			
			diskFile.seekp(0, ios::beg);
			diskFile.write(reinterpret_cast<char*>(&mbr), sizeof(MBR));
			diskFile.close();
			
			char timeStr[20];
			struct tm* timeInfo = localtime(&mbr.mbr_creation_date);
			strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", timeInfo);
			
			CmdMount::registrarDiscoNuevo(expandedPath);
			return "Disco creado exitosamente\n" +
				string("  Ruta: ") + expandedPath + "\n" +
				string("  Fecha: ") + timeStr + "\n" +
				string("  Tamaño: ") + to_string(size) + " " + unit + 
				" (" + to_string(SizeB) + " bytes)\n" +
				string("  Firma: ") + to_string(mbr.mbr_disk_signature);
		}
		catch(const exception& e){
			return string("Error al intentar crear el disco: ") + e.what();
		}
		
	}
		
}
#endif // !MKDISK_H

