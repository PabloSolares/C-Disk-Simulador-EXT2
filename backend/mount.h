
#ifndef MOUNT_H
#define MOUNT_H
#include <cctype>
#include <cstdlib>
#include <ios>
#include <sstream>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <cstring>
#include <cctype>
#include <map>
#include <iostream>
#include "structures.h"


using namespace std;

namespace CmdMount {
	
	struct MountedPartition {
		string path;
		string name;
		string id;
		char type;
		int start;
		int size;
		
	};

	
	static map<string, MountedPartition> mountedPartitions;
	
	static map<string, char> diskLetters;

	

	static char nxtDiskLetter = 'a';
	inline void registrarDiscoNuevo(const string& path){
        if(diskLetters.find(path) == diskLetters.end()){
            diskLetters[path] = nxtDiskLetter++;
        }
    }

	inline string expandPath(const string& path){
		if(path.empty() || path[0] != '~'){
			return path;
		}

		const char* home = getenv("HOME");

		if(!home){
			home = getenv("USERPROFILE");
			
		}
		
		if(home){
			return string(home) + path.substr(1);
		}
		
		return path;

	}

	inline string unmount(const string& id){
		if (id.empty()) {
            return "Error: el comando unmount requiere el parametro -id";
        }

		for(auto it = mountedPartitions.begin(); it != mountedPartitions.end(); ++it){
			if(it->first == id){
				mountedPartitions.erase(it);
				return "Particion con ID" + id + " desmontada existosamente";
			}
		}

		return "Error: No se encontro ninguna particion montada con el ID '" + id + "'";
	}
	
	inline bool findPartitionMbr(const string& path, const string& name, char& type, int& start, int& size){
		ifstream file(path, ios::binary);
		
		if(!file.is_open()){
			return false;
		}
		
		MBR mbr;
		file.read(reinterpret_cast<char*>(&mbr), sizeof(MBR));

		for (int i = 0; i<4; i++) {
			if(mbr.mbr_partitions[i].part_status == '1'){
				string partName(mbr.mbr_partitions[i].part_name);

				partName.erase(remove_if(partName.begin(), partName.end(), ::isspace), partName.end());
				partName.erase(find(partName.begin(), partName.end(), '\0'), partName.end());

				if(partName == name){
					type = mbr.mbr_partitions[i].part_type;
					start = mbr.mbr_partitions[i].part_start;
					size = mbr.mbr_partitions[i].part_size;
					file.close();
					return true;
				}

				if(mbr.mbr_partitions[i].part_type == 'E'){
					int ebrPos = mbr.mbr_partitions[i].part_start;
					while (ebrPos != -1){
						EBR ebr;
						file.seekg(ebrPos, ios::beg);
						file.read(reinterpret_cast<char*>(&ebr), sizeof(EBR));
						
						if(ebr.part_status == '1'){
							string ebrName(ebr.part_name);
							ebrName.erase(remove_if(ebrName.begin(), ebrName.end(), ::isspace), ebrName.end());
							ebrName.erase(find(ebrName.begin(), ebrName.end(), '\0'), ebrName.end());

							if(ebrName == name){
								type = 'L';
                                start = ebr.part_start;
                                size = ebr.part_size;
                                file.close();
                                return true;
							}
						}

						ebrPos = ebr.part_next;
					}
				}
			}
		}

		file.close();
		return false;
	}

	inline string generateMountedID(const string& path){
		char diskLetter;

		auto it = diskLetters.find(path);

		if(it != diskLetters.end()){
			diskLetter = it->second;
		} else {
			diskLetter = nxtDiskLetter++;
			diskLetters[path] = diskLetter;
		}

		int partitionNumber = 1;
		for(const auto& [id, partition] : mountedPartitions){
			if(partition.path == path){
				partitionNumber++;
			}
		}
		string mountId = "44";
	
		mountId += to_string(partitionNumber);
		mountId += toupper(diskLetter);
		return mountId;
	}

	inline bool isPartitionMounted(const string& path, const string& name){
		for(const auto& [id, partition] : mountedPartitions){
			if(partition.path == path && partition.name == name){
				return true;
			}

		}
		return false;
	}

	inline void mount (const map<string, string>& params){
		std::cout << "\n******** MOUNT ********* \n" << std::endl;

		auto pathIt = params.find("-path");
		auto nameIt = params.find("-name");

		if(pathIt == params.end() || nameIt == params.end()){
			cerr << "Error: faltan parametros obligatorios (-path y -name)" << endl;
			return;
		}

		string path = expandPath(pathIt->second);
		string name = nameIt->second;

		ifstream file(path);

		if(!file.good()){ 
			cerr << "Error: en el disco " << path << " no existe" << endl;
			return;
		}
		file.close();

		if(isPartitionMounted(path, name)){
			cerr << "Error: la particion " << name << " en " << path << " ya esta montada";
			return;
		}

		char type;
		int start, size;

		if(!findPartitionMbr(path,name, type, start, size)){
			cerr << "Error: no se encontro la particion " << name << " en el disco " << path << endl;
			return;
		}

		string mountID = generateMountedID(path);

		MountedPartition mounted;
		mounted.path = path;
		mounted.name = name;
		mounted.id = mountID;
		mounted.type = type;
		mounted.start = start;
		mounted.size = size;

		mountedPartitions[mountID] = mounted;


		cout << "Partición montada exitosamente" << endl;
        cout << "  ID: " << mountID << endl;
       	cout << "  Disco: " << path << endl;
        cout << "  Partición: " << name << endl;
        cout << "  Tipo: " << type << endl;
        cout << "  Inicio: " << start << " bytes" << endl;
        cout << "  Tamaño: " << size << " bytes" << endl;

	}

	inline string mount(const string& pathP, const string& nameP){
		string path = expandPath(pathP);
		string name = nameP;

		ifstream file(path);

		if(!file.good()){ 
			return "Error: en el disco " + path + " no existe";
		}
		file.close();

		if(isPartitionMounted(path,name)){
			return "Error: la particion" + name + " en " + path + " ya esta montada";
		}

		char type;
		int start, size;
		if(!findPartitionMbr(path,name, type, start, size)){
			return "Error: no se encontro la particion " + name + " en el disco " + path ;
			
		}

		string mountID = generateMountedID(path);

		MountedPartition mounted;
		mounted.path = path;
		mounted.name = name;
		mounted.id = mountID;
		mounted.type = type;
		mounted.start = start;
		mounted.size = size;

		mountedPartitions[mountID] = mounted;
		std::ostringstream result;
		result << "\n************* MOUNT *************** \n" ;

		result << "Partición montada exitosamente" ;
        result << "  ID: " << mountID ;
       	result << "  Disco: " << path ;
        result << "  Partición: " << name ;
        result << "  Tipo: " << type ;
        result << "  Inicio: " << start << " bytes";
        result << "  Tamaño: " << size << " bytes" ;
		return result.str();
	}

	inline string listMountedPart(){
		if(mountedPartitions.empty()){
			return "No hay particiones montadas";
		}

		ostringstream result;

		result << "\n*********** Particiones Montadas *****************\n";
		for (const auto& [id, partition] : mountedPartitions){
			result << "ID: " << id << " \n";
			result << " Disco: " << partition.path << " \n";

			result << " Particion: " << partition.name << " \n";
			result << " Tipo: " << partition.type << " \n";
			result << " Inicio: " << partition.start << " bytes \n";
			result << " Tamaño: " << partition.size << " bytes \n";
			result << "************************************************ \n";

		}

		return result.str();
		
	}

	inline void showMountedPar(){
		std::cout << listMountedPart() << std::endl;
	}

	inline bool getMountedPar(const string& id, MountedPartition& partition){
		auto it = mountedPartitions.find(id);
		if(it != mountedPartitions.end()){
			partition = it->second;
			return true;

		}

		return false;
	}
}

#endif // !MOUNT_H