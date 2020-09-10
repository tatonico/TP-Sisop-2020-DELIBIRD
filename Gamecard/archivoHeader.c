/*
 * archivoHeader.c
 *
 *  Created on: 26 jun. 2020
 *      Author: utnso
 */

#include "gamecard.h"

bool estaAbierto(archivoHeader* metadata){
//	if(metadata->tipo==ARCHIVO){
//			t_config* config=config_create(metadata->pathArchivo);
//			char* valorOpen=config_get_string_value(config,"OPEN");
//			config_destroy(config);
//			if(strcmp(valorOpen, "Y")==0){
//				return true;
//			}
//
//
//		}
	return metadata->estaAbierto;
}

void cerrarArchivo(archivoHeader* metadata, FILE* archivo){
	if(metadata->tipo==ARCHIVO){
		fclose(archivo);

		pthread_mutex_lock(metadata->mutex);

		t_config* config=config_create(metadata->pathArchivo);
		config_set_value(config,"OPEN","N");
		config_save(config);
		config_destroy(config);
		metadata->estaAbierto=false;

		pthread_mutex_unlock(metadata->mutex);
	}
}

FILE* abrirArchivo(archivoHeader* metadata){
	//uint32_t i=1;


	//pthread_mutex_lock(metadata->mutex);
	//pthread_mutex_lock(metadata->mutex);
	if(metadata->tipo==ARCHIVO){



		pthread_mutex_lock(metadata->mutex);
		while(estaAbierto(metadata)){
					pthread_mutex_unlock(metadata->mutex);
					log_info(gamecardLogger, "Reintento abrir el archivo en %i sengundos. ", tiempoReintentoOperacion);
					log_info(gamecardLogger2, "Reintento abrir el archivo en %i sengundos. ", tiempoReintentoOperacion);
					obtenerListaBloquesConfig(metadata, "ABRIR ARCHIVO");
					sleep(tiempoReintentoOperacion);
					pthread_mutex_lock(metadata->mutex);
				}
		obtenerListaBloquesConfig(metadata, "ABRIR ARCHIVO FUERA 1");
		metadata->estaAbierto=true;

		t_config* config=config_create(metadata->pathArchivo);
		config_set_value(config,"OPEN","Y");
		config_save(config);
		config_destroy(config);
		obtenerListaBloquesConfig(metadata, "ABRIR ARCHIVO FUERA 2");
		pthread_mutex_unlock(metadata->mutex);

		return fopen(metadata->pathArchivo,"r+");

	}
	return NULL;
}

void modificarSize(archivoHeader* metadata,uint32_t size){
	if(metadata->tipo==ARCHIVO){
		t_config* config=config_create(metadata->pathArchivo);
		metadata->tamanioArchivo=size;
		config_set_value(config,"SIZE",string_itoa(size));
		config_save(config);
		config_destroy(config);
	}
}

void obtenerListaBloquesConfig(archivoHeader* archivo, char* estado){
	t_config* config=config_create(archivo->pathArchivo);
	//log_info(gamecardLogger2, "----------------------------Lista bloques del config del archivo %s: %s. %s", archivo->nombreArchivo, config_get_string_value(config, "BLOCKS"), estado);
	config_destroy(config);
}

void agregarBloque(archivoHeader* metadata, blockHeader* bloque){
	if(metadata->tipo==ARCHIVO){

		ocuparBloque(bloque->id);
		pthread_mutex_lock(metadata->mutex);
		t_config* config=config_create(metadata->pathArchivo);
		list_add(metadata->bloquesUsados,(void*) bloque);
		char* listaB=obtenerStringListaBloques(metadata);
		log_info(gamecardLogger2, "LISTA BLOQUES: %s.",listaB);
		config_set_value(config,"BLOCKS",listaB);
		config_save(config);
		char* listaAgregada= config_get_string_value(config, "BLOCKS");
		log_info(gamecardLogger2, "LISTA BLOQUES AGREGADA: %s.",listaAgregada);
		config_destroy(config);
		free(listaB);
		pthread_mutex_unlock(metadata->mutex);
	}

}

void agregarUnBloque(archivoHeader* metadata){
	log_info(gamecardLogger2,"Agrego un bloque.");
	blockHeader* bloqueLibre= encontrarBloqueLibre();
	agregarBloque(metadata, bloqueLibre);
}

char* obtenerStringListaBloques(archivoHeader* metadata){
	t_list* listaBloques=metadata->bloquesUsados;
	char* buffer=malloc(500);
	if(list_size(listaBloques)==0){
		sprintf(buffer,"[]");
	}else{
		for(uint32_t i=0; i<list_size(listaBloques);i++){
			uint32_t idBloqueActual=((blockHeader*) list_get(listaBloques,i))->id;

			if(i==0){
				sprintf(buffer, "[%i]", idBloqueActual);
			}else{
					buffer=agregarIdBloque(buffer,idBloqueActual);
			}
		}
	}

	return buffer;
}

t_list* arrayBloquesStringToList(char** listaArray){

	t_list* lista=list_create();
	for(uint32_t i=0; *(listaArray + i)!=NULL;i++){
		uint32_t idBloqueActual= atoi(*(listaArray +i));
		blockHeader* bloqueActual= obtenerBloquePorId(idBloqueActual);
		list_add(lista, (void*) bloqueActual);
	}
	return lista;
}

char* agregarIdBloque(char* string, uint32_t id){
	uint32_t pos = strlen(string) - 1;
	char* buffer=malloc(500);
	uint32_t i=0;
	while(1){
		if(i!=pos){
			buffer[i]=string[i];
		}else{
			sprintf((buffer+i), ",%d]", id);
			free(string);
			return buffer;
		}
		i++;
	}
}

void removerBloque(archivoHeader* metadata, uint32_t idBloque){
	log_info(gamecardLogger2, "REMUEVO UN BLOQUE");

	pthread_mutex_lock(metadata->mutex);

	t_config* config=config_create(metadata->pathArchivo);
	int32_t pos= obtenerPosicionBloqueEnLista(metadata->bloquesUsados, idBloque);
	if(pos!=-1){
		list_remove(metadata->bloquesUsados,pos);
	}

	char* listaB=obtenerStringListaBloques(metadata);
	config_set_value(config,"BLOCKS",listaB);
	config_save(config);
	config_destroy(config);

	pthread_mutex_unlock(metadata->mutex);

	free(listaB);
	liberarBloque(idBloque);
	reiniciarArchivoBloque(idBloque);
}

void removerUnBloque(archivoHeader* metadata){
	blockHeader* bloque=list_get(metadata->bloquesUsados,0);
	removerBloque(metadata, bloque->id);
}

int32_t obtenerPosicionBloqueEnLista(t_list* listaBloques , uint32_t idBloque){
	for(uint32_t i=0; i<list_size(listaBloques);i++){
		blockHeader* bloqueActual= (blockHeader*) list_get(listaBloques, i);
		if(bloqueActual->id==idBloque){
			return i;
		}
	}
	return -1;
}

void disminuirCapacidad(blockHeader* bloque, int32_t bytes){
	bloque->bytesLeft-=bytes;
}

bool tieneCapacidad(blockHeader* bloque, int32_t capacidad){
	return bloque->bytesLeft>=capacidad;
}

char* obtenerStringArchivo(char* pokemon){
	char* buffer=string_new();
	archivoHeader* headerPoke= buscarArchivoHeaderPokemon(pokemon);
	for(uint32_t i=0; i<list_size(headerPoke->bloquesUsados);i++){
		blockHeader* actual= list_get(headerPoke->bloquesUsados,i);
		char* bufferBloque=leerBloque(actual);
		string_append(&buffer, bufferBloque);
		free(bufferBloque);
	}

	return buffer;
}

t_list* obtenerPosicionesDeArchivo(char* pokemon){
	return obtenerListaPosicionCantidadDeString(obtenerStringArchivo(pokemon));
}

char* pathBloque(uint32_t idBloque){
	return string_from_format("%s/Blocks/%d.bin", puntoMontaje, idBloque);
}
char* leerBloque(blockHeader* bloque){
	char* path=pathBloque(bloque->id);
	FILE* archivoBloque= fopen(path,"r");


	fseek(archivoBloque, 0, SEEK_END);
	uint32_t pos= ftell(archivoBloque);
	fseek(archivoBloque, 0, SEEK_SET);
	char* resultado= malloc(pos+1);
	//fread(buffer, pos, 1, archivoBloque);
	log_info(gamecardLogger2, "Posicion en la lectura del bloque %i: %i", bloque->id, pos);
	fread(resultado, 1, pos, archivoBloque);
//	if(string_contains(resultado, "\n")){
//		log_info(gamecardLogger2, "Encontre el  barra n.");
	*(resultado+pos)='\0';
	fclose(archivoBloque);
	free(path);
//	char* resultado=string_new();
//	string_append(&resultado,buffer);
//	string_append(&resultado,"\0");
	//free(buffer);
	return resultado;
	//return resultado;

}



uint32_t posBloque(blockHeader* bloque){
	char* path=pathBloque(bloque->id);
	FILE* archivoBloque= fopen(path,"r");
	fseek(archivoBloque, 0, SEEK_END);
	uint32_t pos= ftell(archivoBloque);
	fclose(archivoBloque);
	return pos;
}

t_list* obtenerListaPosicionesString(char* posiciones){
	t_list* lista=list_create();
	char* buffer=string_new();
	log_info(gamecardLogger2, "char*posiciones: %s", posiciones);
	for(uint32_t i=0;i<(strlen(posiciones)+1);i++){

		char caracterActual= posiciones[i];
		if(caracterActual=='\n'){
			log_info(gamecardLogger2, "BUFFER FUNCION LISTA STRING: %s", buffer);
			list_add(lista, (void*) buffer);
			buffer=string_new();
		}else{
			char cadena [2]=" \0";
			cadena[0]=caracterActual;
			string_append(&buffer, cadena);
			//free(cadena);
		}
	}
	return lista;
}

posicionCantidad* obtenerPosicionCantidadDeString(char* stringPos){
	char* bufferPosX=string_new();
	char* bufferPosY=string_new();
	char* bufferCantidad=string_new();
	bool posX=true;
	bool posY=false;
	bool cantidad=false;

		for(uint32_t i=0;i<(strlen(stringPos));i++){
			char caracterActual= stringPos[i];
			if(posX){
				if(caracterActual=='-'){
					posX=false;
					posY=true;
				}else{
					char cadena [2]=" \0";
					cadena[0]=caracterActual;
					string_append(&bufferPosX, cadena);
					//free(cadena);
				}
			}else if(posY){
				if(caracterActual=='='){
					posY=false;
					cantidad=true;
				}else{
					char cadena [2]=" \0";
					cadena[0]=caracterActual;
					string_append(&bufferPosY, cadena);
					//free(cadena);
				}
			}else if(cantidad){
				char cadena [2]=" \0";
				cadena[0]=caracterActual;
				string_append(&bufferCantidad, cadena);
				//free(cadena);
			}

		}

	posicionCantidad* pos=malloc(sizeof(posicionCantidad));
	(pos->posicion).x=atoi(bufferPosX);
	(pos->posicion).y=atoi(bufferPosY);
	pos->cantidad=atoi(bufferCantidad);

	return pos;

}

t_list * obtenerListaPosicionCantidad(t_list* listaString){
	t_list* lista= list_create();
	for(uint32_t i=0; i<list_size(listaString);i++){
		char* stringActual= (char*) list_get(listaString,i);
		posicionCantidad* posActual=obtenerPosicionCantidadDeString(stringActual);
		log_info(gamecardLogger2, "I DE LA FUNCION DE LISTAS: %i", i);
		list_add(lista, (void*) posActual);
	}
	list_destroy_and_destroy_elements(listaString, free);
	return lista;
}

t_list* obtenerListaPosicionCantidadDeString(char* string){
	return obtenerListaPosicionCantidad(obtenerListaPosicionesString(string));
}

void setearSize(archivoHeader* archivo,uint32_t size){

	archivo->tamanioArchivo = size;

	pthread_mutex_lock(archivo->mutex);

	char* sizeArchivo = string_itoa(size);
	t_config* config=config_create(archivo->pathArchivo);
	config_set_value(config,"SIZE",sizeArchivo);
	config_save(config);
	config_destroy(config);
	free(sizeArchivo);
	pthread_mutex_unlock(archivo->mutex);
}

void reescribirArchivo(char* pokemon, char* stringAEscribir){
	archivoHeader* headerPoke= buscarArchivoHeaderPokemon(pokemon);
	uint32_t bytesAEscribir=strlen(stringAEscribir);
	//log_info(gamecardLogger2, "CANTIDAD DE BYTES A ESCRIBIR: %i", bytesAEscribir);
	uint32_t bloquesNecesarios;
	setearSize(headerPoke,bytesAEscribir);
	obtenerListaBloquesConfig(headerPoke, "COMIENZO");
	if (bytesAEscribir == 0){
		 bloquesNecesarios=0;
	}else{
		 bloquesNecesarios=(bytesAEscribir/tallGrass.block_size)+1;
	}
	//log_info(gamecardLogger2,"-------------BLOQUES NECESARIOS= %i",bloquesNecesarios);
	while(bloquesNecesarios<cantidadBloquesArchivo(headerPoke)){
		removerUnBloque(headerPoke);
	}
	while(bytesAEscribir>capacidadTotalArchivo(headerPoke)){
		agregarUnBloque(headerPoke);
	}

	reiniciarBloquesDeArchivo(headerPoke);
	t_list* listaBloques= headerPoke->bloquesUsados;
	for(uint32_t i=0;i<list_size(listaBloques);i++){
		blockHeader* bloqueActual=(blockHeader*) list_get(listaBloques,i);
		char* subString;
		if(bytesAEscribir<=tallGrass.block_size){
			subString= string_substring(stringAEscribir, i* tallGrass.block_size, bytesAEscribir);
			//log_info(gamecardLogger2, "SUBSTRING: %s", subString);

			escribirBloque(bloqueActual->id, 0, bytesAEscribir, subString);

			bytesAEscribir=0;
		}else{
			subString= string_substring(stringAEscribir, i* tallGrass.block_size, tallGrass.block_size);

			escribirBloque(bloqueActual->id, 0, tallGrass.block_size, subString);

			bytesAEscribir-=tallGrass.block_size;
		}

		free(subString);

	}
	obtenerListaBloquesConfig(headerPoke, "FINAL");


}

uint32_t cantidadBloquesArchivo(archivoHeader* archivo){
	return list_size(archivo->bloquesUsados);
}

uint32_t capacidadTotalArchivo(archivoHeader* archivo){
	return cantidadBloquesArchivo(archivo)*(tallGrass.block_size);
}

char* leerArchivo(char* pokemon){
	archivoHeader* headerPoke= buscarArchivoHeaderPokemon(pokemon);


	char* buffer=string_new();

	for(uint32_t i=0; i<list_size(headerPoke->bloquesUsados);i++){
		//log_info(gamecardLogger2, "hola3.");
		blockHeader* bloqueActual= (blockHeader*) list_get(headerPoke->bloquesUsados,i);
		char* stringActual= leerBloque(bloqueActual);
		log_info(gamecardLogger2,"LECTURA BLOQUE: %s.", stringActual);
		//log_info(gamecardLogger2, "LecturaArchivo: %s.", stringActual);
		string_append(&buffer, stringActual);
		free(stringActual);
	}
	return buffer;
}

posicionCantidad* buscarPosicionCantidad(t_list* lista, posicion pos){
	for(uint32_t i=0; i< list_size(lista);i++ ){
		posicionCantidad* actual= (posicionCantidad*) list_get(lista,i);

		if((actual->posicion).x==pos.x && (actual->posicion).y==pos.y){
			return actual;
		}
	}
	return NULL;
}

uint32_t buscarIdCantidad(t_list* lista, posicion pos){
	for(uint32_t i=0; i< list_size(lista);i++ ){
			posicionCantidad* actual= (posicionCantidad*) list_get(lista,i);

			if((actual->posicion).x==pos.x && (actual->posicion).y==pos.y){
				return i;
			}
		}
		return -1;
}

t_list* obtenerListaPosicionCantidadDeArchivo(archivoHeader* archivo){

	char* stringPosCant=leerArchivo(archivo->nombreArchivo);
	log_info(gamecardLogger2, "LecturaArchivo: %s.", stringPosCant);
	return obtenerListaPosicionCantidadDeString(stringPosCant);
}

char* posicionCantidadToString(posicionCantidad* pos){

		char* posX=string_itoa((pos->posicion).x);
		char* posY=string_itoa((pos->posicion).y);
		char* cantidad=string_itoa(pos->cantidad);
		char * buffer=string_new();
		string_append(&buffer,posX );
		string_append(&buffer,"-");
		string_append(&buffer,posY );
		string_append(&buffer, "=" );
		string_append(&buffer,cantidad);
		string_append(&buffer,"\n" );

		free(posX);
		free(posY);
		free(cantidad);

		return buffer;
}

char* listaPosicionCantidadToString(t_list* lista){
	char* buffer=string_new();
			for(uint32_t i=0; i<list_size(lista);i++){
				posicionCantidad* actual= (posicionCantidad*) list_get(lista, i);
				char* stringActual=posicionCantidadToString(actual);
				//log_info(gamecardLogger2, "stringActual: %s", stringActual);
				string_append(&buffer, stringActual);
				free(stringActual);
			}
	return buffer;
}

void actualizarPosicionesArchivo(archivoHeader* archivo, t_list* listaPosicionCantidad){//listaPosicionCantidad
	if(archivo->tipo==ARCHIVO){
		char* listaString=listaPosicionCantidadToString(listaPosicionCantidad);
		//log_info(gamecardLogger2, "string de lista: %s", listaString);
		reescribirArchivo(archivo->nombreArchivo, listaString);
	}
}

void reiniciarArchivoBloque(uint32_t idBloque){
	char* path=pathBloque(idBloque);

	remove(path);

	FILE* archivoBloque = fopen(path,"w+");
	fclose(archivoBloque);

	uint32_t blockfd = open(path,O_RDWR| S_IRUSR | S_IWUSR,0777);
			ftruncate(blockfd,tallGrass.block_size);
			close(blockfd);

	free(path);
}

void reiniciarBloquesDeArchivo(archivoHeader* headerPoke){

	for(uint32_t i=0;i<list_size(headerPoke->bloquesUsados);i++){
		blockHeader* bloque = list_get(headerPoke->bloquesUsados,i);
		reiniciarArchivoBloque(bloque->id);
	}

}


posicion* conseguirPosicionesCantidad(t_list* lista){
	log_info(gamecardLogger2, "Entro a conseguirPosicionesCantidad");
//	t_list* listaPosCantidad=obtenerListaPosicionCantidadDeArchivo(pokeArchivo);
	uint32_t cantPosiciones = list_size(lista);
	posicion* arrayPosicion = malloc(cantPosiciones*sizeof(uint32_t)*2);

	for(uint32_t i=0;i<cantPosiciones;i++){
		//uint32_t offset=2*sizeof(uint32_t)*i;
		posicionCantidad* auxPosCan = (posicionCantidad*) list_get(lista,i);
		posicion auxPos = auxPosCan->posicion;

		//memcpy(arrayPosicion+offset,&auxPos,2*sizeof(uint32_t));

		*(arrayPosicion+i) = auxPos;
		log_info(gamecardLogger2, "Localized. Pos x: %i. Pos y: %i.", (arrayPosicion+i)->x, (arrayPosicion+i)->y);
	}
	return arrayPosicion;
}


